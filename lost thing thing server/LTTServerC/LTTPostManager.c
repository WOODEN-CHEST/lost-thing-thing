#include "LTTServerC.h"
#include "LTTPostManager.h"
#include "Directory.h"
#include "GHDF.h"
#include "File.h"
#include <time.h>
#include "Logger.h"
#include "Memory.h"
#include "Image.h"
#include "LTTString.h"
#include "LTTChar.h"
#include <limits.h>

// Macros.
#define CACHED_POST_COUNT 128
#define CACHED_POST_UNLOADED_TIME -1

#define GENERIC_LIST_CAPACITY 4
#define GENERIC_LIST_GROWTH 2

#define UNFINISHED_POST_LIFETIME 60


/* Directories and paths. */
#define ENTRY_FOLDER_NAME_DIVIDER 1000
#define DIR_NAME_POSTS "posts"
#define DIR_NAME_ENTRIES "entries"
#define POST_METAINFO_FILENAME "post_meta" GHDF_FILE_EXTENSION
#define FILE_NAME_THUMBNAIL "thumbnail" FILE_EXTENSION_PNG


/* Meta-info */
#define DEFAULT_AVAILABLE_POST_ID 1
#define ENTRY_ID_METAINFO_AVAIlABLE_POST_ID 1 // ulong



/* Post data. */
#define POST_MAX_IMAGES 10
#define POST_MAX_IMAGE_SIZE 2048
#define POST_MAX_THUMBNAIL_SIZE 256
#define POST_MAX_TITLE_LENGTH_CODEPOINTS 512
#define POST_MAX_DESCRIPTION_LENGTH_CODEPOINTS 8192


/* Post compound. */
#define ENTRY_ID_POST_ID // ULong
#define ENTRY_ID_POST_TITLE // String
#define ENTRY_ID_POST_DESCRIPTION // String
#define ENTRY_ID_POST_TAGS // Int
#define ENTRY_ID_POST_CLAIMER_ID // ULong
#define ENTRY_ID_POST_REQUESTER_IDS // ULong array
#define ENTRY_ID_POST_REQUESTER_IDS // ULong array


// Types.
typedef struct CachedPostStruct
{
	time_t LastAccessTime;
	Post TargetPost;
} CachedPost;

typedef struct UnfinishedPostImageStruct
{
	const char* Data;
	size_t Length;
} UnfinishedPostImage;

typedef struct UnfinishedPostStruct
{
	time_t CreationStartTime;
	unsigned long long AuthorID;
	const char* Title;
	const char* Description;
	PostTagBitFlags Tags;

	UnfinishedPostImage Images[POST_MAX_IMAGES];
	size_t ImageCount;
} UnfinishedPost;


// Static functions.
static unsigned long long GetAndUsePostID(DBPostContext* context)
{
	unsigned long long ID = context->AvailablePostID;
	context->AvailablePostID++;
	return ID;
}

static unsigned long long GetAvailablePostID(DBPostContext* context)
{
	return context->AvailablePostID;
}


/* Paths. */
static void BuildPathToPostDirectory(DBPostContext* context, StringBuilder* builder, unsigned long long id)
{
	StringBuilder_Append(builder, context->PostRootPath);

	StringBuilder_AppendChar(builder, PATH_SEPARATOR);
	StringBuilder_Append(builder, DIR_NAME_ENTRIES);

	StringBuilder_AppendChar(builder, PATH_SEPARATOR);
	char NumberString[32];
	sprintf(NumberString, "%llu", id / ENTRY_FOLDER_NAME_DIVIDER);
	StringBuilder_Append(builder, NumberString);
	StringBuilder_AppendChar(builder, 'k');

	StringBuilder_AppendChar(builder, PATH_SEPARATOR);
	sprintf(NumberString, "%llu", id);
	StringBuilder_Append(builder, NumberString);
}

static const char* GetPathToIDFile(DBPostContext* context, unsigned long long id, const char* extension)
{
	StringBuilder Builder;
	StringBuilder_Construct(&Builder, DEFAULT_STRING_BUILDER_CAPACITY);
	BuildPathToPostDirectory(context, &Builder, id);
	
	char NumberString[32];
	snprintf(NumberString, sizeof(NumberString), "%llu", id);
	StringBuilder_AppendChar(&Builder, PATH_SEPARATOR);
	StringBuilder_Append(&Builder, NumberString);
	StringBuilder_Append(&Builder, extension);

	return Builder.Data;
}

static const char* GetPathToThumbnail(DBPostContext* context, unsigned long long id)
{
	StringBuilder Builder;
	StringBuilder_Construct(&Builder, DEFAULT_STRING_BUILDER_CAPACITY);
	BuildPathToPostDirectory(context, &Builder, id);

	StringBuilder_AppendChar(&Builder, PATH_SEPARATOR);
	StringBuilder_Append(&Builder, FILE_NAME_THUMBNAIL);

	return Builder.Data;
}


/* Data verification. */
static bool IsValidTextChar(const char* character)
{
	return Char_IsLetter(character) || Char_IsDigit(character) || ((*character >= 32) && (*character <= 126))
		|| (*character == '\n') | (*character == '\t');
}

static bool VerifyText(const char* title, size_t maxLengthCodepoints)
{
	size_t CodepointLength = String_LengthCodepointsUTF8(title);
	if (CodepointLength > maxLengthCodepoints)
	{
		return false;
	}

	for (size_t i = 0; title[i] != '\0'; i += Char_GetByteCount(title + i))
	{
		if (!IsValidTextChar(title + i))
		{
			return false;
		}
	}

	return true;
}

static bool VerifyPostTitle(const char* title)
{
	return VerifyText(title, POST_MAX_TITLE_LENGTH_CODEPOINTS);
}

static bool VerifyPostDescription(const char* description)
{
	return VerifyText(description, POST_MAX_DESCRIPTION_LENGTH_CODEPOINTS);
}

/* Post. */
static void PostDeconstruct(Post* post)
{
	if (post->Title)
	{
		Memory_Free(post->Title);
	}
	if (post->Description)
	{
		Memory_Free(post->Description);
	}
	if (post->ThumbnailData)
	{
		Memory_Free(post->ThumbnailData);
	}
	if (post->RequesterIDs)
	{
		Memory_Free(post->RequesterIDs);
	}
}

static void PostSetDefaultValues(Post* post)
{
	Memory_Set((char*)post, sizeof(Post), 0);
}

static void PostEnsureRequesterCapacity(Post* post, size_t capacity)
{
	if (post->_requesterCapacity >= capacity)
	{
		return;
	}

	while (post->_requesterCapacity < capacity)
	{
		post->_requesterCapacity *= GENERIC_LIST_GROWTH;
	}
	post->RequesterIDs = (unsigned long long*)Memory_SafeRealloc(post->RequesterIDs,
		sizeof(unsigned long long) * post->_requesterCapacity);
}

static void PostCreateNew(DBPostContext* context,
	Post* post,
	const char* title,
	const char* description,
	unsigned long long authorID,
	PostTagBitFlags tags,
	size_t imageCount)
{
	post->ID = GetAndUsePostID(context);
	post->Title = String_CreateCopy(title);
	post->Description = String_CreateCopy(description);
	post->AuthorID = authorID;
	post->Tags = tags;
	post->ImageCount = imageCount;
	post->ThumbnailData = NULL;

	post->ClaimerID = 0;
	post->RequesterCount = 0;
	post->_requesterCapacity = GENERIC_LIST_CAPACITY;
	post->RequesterIDs = (unsigned long long*)Memory_SafeMalloc(sizeof(unsigned long long) * post->_requesterCapacity);
}


/* Unfinished post. */
static void EnsureUnfinishedPostListCapacity(DBPostContext* context, size_t capacity)
{
	if (context->_unfinishedPostCapacity >= capacity)
	{
		return;
	}

	while (context->_unfinishedPostCapacity < capacity)
	{
		context->_unfinishedPostCapacity *= GENERIC_LIST_GROWTH;
	}
	context->UnfinishedPosts = (UnfinishedPost*)Memory_SafeRealloc(
		context->UnfinishedPosts, sizeof(UnfinishedPost) * context->_unfinishedPostCapacity);
}

static void AddUnfinishedPost(DBPostContext* context,
	unsigned long long authorID,
	const char* title,
	const char* description,
	PostTagBitFlags tags)
{
	EnsureUnfinishedPostListCapacity(context, context->UnfinishedPostCount + 1);
	UnfinishedPost* Post = context->UnfinishedPosts + context->UnfinishedPostCount;

	Post->CreationStartTime = time(NULL);
	Post->AuthorID = authorID;
	Post->Title = String_CreateCopy(title);
	Post->Description = String_CreateCopy(description);
	Post->Tags = tags;
	Post->ImageCount = 0;
}

static void UnfinishedPostDeconstruct(UnfinishedPost* post)
{
	Memory_Free(post->Title);
	Memory_Free(post->Description);
}

static void RemoveUnfinishedPostByIndex(DBPostContext* context, size_t index)
{
	UnfinishedPostDeconstruct(context->UnfinishedPosts + index);

	for (size_t i = index + 1; i < context->UnfinishedPostCount; i++)
	{
		context->UnfinishedPosts[i - 1] = context->UnfinishedPosts[i];
	}
	context->UnfinishedPostCount -= 1;
}

static void RemoveUnfinishedPostByAuthorID(DBPostContext* context, unsigned long long authorID)
{
	for (size_t i = 0; i < context->UnfinishedPostCount; i++)
	{
		if (context->UnfinishedPosts[i].AuthorID == authorID)
		{
			RemoveUnfinishedPostByIndex(context, i);
		}
	}
}

static UnfinishedPost* GetUnfinishedPostByAuthorID(DBPostContext* context, unsigned long long authorID)
{
	for (size_t i = 0; i < context->UnfinishedPostCount; i++)
	{
		if (context->UnfinishedPosts[i].AuthorID == authorID)
		{
			return context->UnfinishedPosts + i;
		}
	}
	return NULL;
}

static void UnfinishedPostsRefresh(DBPostContext* context)
{
	time_t CurrentTime = time(NULL);
	for (size_t i = 0; i < context->UnfinishedPostCount; i++)
	{
		if (CurrentTime - context->UnfinishedPosts[i].CreationStartTime >= UNFINISHED_POST_LIFETIME)
		{
			// Dangerous, removing items while iterating collection.
			RemoveUnfinishedPostByIndex(context, i);
			i--;
		}
	}
}


/* Post cache. */
static void InitializePostCache(DBPostContext* context)
{
	context->CachedPosts = (CachedPost*)Memory_SafeMalloc(sizeof(CachedPost) * CACHED_POST_COUNT);
	for (int i = 0; i < CACHED_POST_COUNT; i++)
	{
		context->CachedPosts->LastAccessTime = CACHED_POST_UNLOADED_TIME;
	}
}

static CachedPost* GetCacheSlotForPost(DBPostContext* context)
{
	time_t LowestTime = LLONG_MAX;
	int LowestTimeIndex = 0;

	for (int i = 0; i < CACHED_POST_COUNT; i++)
	{
		if (context->CachedPosts[i].LastAccessTime < LowestTime)
		{
			LowestTime = context->CachedPosts[i].LastAccessTime;
			LowestTimeIndex = i;
		}
	}

	return context->CachedPosts + LowestTimeIndex;
}

static Post* GetPostFromCacheByID(DBPostContext* context, unsigned long long id)
{
	for (int i = 0; i < CACHED_POST_COUNT; i++)
	{
		CachedPost* Post = context->CachedPosts + i;
		if ((Post->LastAccessTime != CACHED_POST_UNLOADED_TIME) && (Post->TargetPost.ID == id))
		{
			return &Post->TargetPost;
		}
	}
}


/* ID Hashmap. */
static void GenerateMetaInfoFroSinglePost(DBPostContext* context, Post* post)
{
	
}

static void GenerateMetaInfoForPosts(DBPostContext* context)
{
	for (unsigned long long id = 0; id < context->AvailablePostID; id++)
	{
		GenerateMetaInfoFroSinglePost(context, id);
	}
}


static void ClearMetaInfoForSinglePost(DBPostContext* context, Post* post)
{

}


/* Loading and saving posts. */
static Error WritePostToDatabase(DBPostContext* context, Post* post)
{

}

static Error CreatePostThumbnailInDatabase(DBPostContext* context, UnfinishedPostImage* image)
{
	Image ReadImage;
	ReadImage.Data = stbi_load_from_memory(image->Data, image->Length, &ReadImage.SizeX,
		&ReadImage.SizeY, &ReadImage.ColorChannels, STBI_rgb);
	if (!ReadImage.Data)
	{
		return Error_CreateError(ErrorCode_InvalidRequest, "Failed to create post thumbnail, couldn't read image.");
	}

	char* ResizedImageData = (char*)Memory_SafeMalloc(POST_MAX_THUMBNAIL_SIZE * POST_MAX)
	stbir_resize()

}

static Error SaveSinglePostImage(DBPostContext* context, unsigned long long postID, UnfinishedPostImage* image)
{
	
}

static Error CreatePostImagesInDatabase(DBPostContext* context, unsigned long long postID, UnfinishedPostImage* images, size_t imageCount)
{
	if (imageCount == 0)
	{
		return;
	}

	Error ReturnedError = CreatePostThumbnailInDatabase(context, images);
	if (ReturnedError.Code != ErrorCode_Success)
	{
		return ReturnedError;
	}

	for (size_t i = 1; i < imageCount; i++)
	{

	}
}

static Error ReadPostFromDatabase(DBPostContext* context, Post* post)
{

}


/* Context loading and saving helper functions. */
static void LoadDefaultMetaInfo(DBPostContext* context)
{
	context->AvailablePostID = DEFAULT_AVAILABLE_POST_ID;
}

static Error LoadMetainfoFromCompound(DBPostContext* context, GHDFCompound* compound)
{
	GHDFEntry* Entry;
	
	Error ReturnedError = GHDFCompound_GetVerifiedEntry(compound, ENTRY_ID_METAINFO_AVAIlABLE_POST_ID,
		&Entry, GHDFType_ULong, "Available account ID");
	if (ReturnedError.Code != ErrorCode_Success)
	{
		return ReturnedError;
	}

	return Error_CreateSuccess();
}

static Error LoadMetainfo(DBPostContext* context)
{
	LoadDefaultMetaInfo(context);

	const char* FilePath = Directory_CombinePaths(context->PostRootPath, POST_METAINFO_FILENAME);
	if (!File_Exists(FilePath))
	{
		Memory_Free((char*)FilePath);
	}
	GHDFCompound Compound;
	Error ReturnedError = GHDFCompound_ReadFromFile(FilePath, &Compound);
	Memory_Free(FilePath);

	if (ReturnedError.Code != ErrorCode_Success)
	{
		return ReturnedError;
	}
	ReturnedError = LoadMetainfoFromCompound(context, &Compound);
	GHDFCompound_Deconstruct(&Compound);
	
	return ReturnedError;
}




// Functions.
Error PostManager_Construct(ServerContext* serverContext)
{
	DBPostContext* Context = serverContext->PostContext;

	Context->PostRootPath = Directory_CombinePaths(serverContext->ServerRootPath, DIR_NAME_POSTS);

	Context->_unfinishedPostCapacity = GENERIC_LIST_CAPACITY;
	Context->UnfinishedPosts = (UnfinishedPost*)Memory_SafeMalloc(sizeof(UnfinishedPost) * Context->_unfinishedPostCapacity);
	Context->UnfinishedPostCount = 0;

	Context->CachedPosts = (CachedPost*)Memory_SafeMalloc(sizeof(CachedPost) * CACHED_POST_COUNT);
	InitializePostCache(Context);
	
	LoadMetainfo(Context);

	IDCodepointHashMap_Construct(&Context->TitleMap);
	GenerateMetaInfoForPosts(Context);
}

void PostManager_Deconstruct(DBPostContext* context)
{

}


/* Creating posts. */
bool PostManager_BeginPostCreation(DBPostContext* context,
	UserAccount* author,
	const char* title,
	const char* description,
	PostTagBitFlags tags,
	Error* error)
{
	*error = Error_CreateSuccess();
	if (!VerifyPostTitle(title) || !VerifyPostDescription(description))
	{
		return false;
	}

	AddUnfinishedPost(context, author->ID, title, description, tags);
}

Error PostManager_UploadPostImage(DBPostContext* context, unsigned long long authorID, const char* imageData, size_t imageDataLength)
{
	UnfinishedPostsRefresh(context);
	UnfinishedPost* Post = GetUnfinishedPostByAuthorID(context, authorID);
	if (!Post)
	{
		return Error_CreateError(ErrorCode_InvalidRequest, "Couldn't find the requested unfinished post for image upload.");
	}

	Post->Images[Post->ImageCount].Data = (char*)Memory_SafeMalloc(imageDataLength);
	Memory_Copy(imageData, (char*)Post->Images[Post->ImageCount].Data, imageDataLength);
	Post->Images[Post->ImageCount].Length = imageDataLength;
	Post->ImageCount++; 

	return Error_CreateSuccess();
}

Post* PostManager_FinishPostCreation(DBPostContext* context, unsigned long long authorID, Error* error)
{
	UnfinishedPostsRefresh(context);
	UnfinishedPost* UnfPost = GetUnfinishedPostByAuthorID(context, authorID);
	if (!UnfPost)
	{
		*error = Error_CreateError(ErrorCode_InvalidRequest, "Couldn't find the requested unfinished post for creation finish.");
		return NULL;
	}

	Post CreatedPost;
	PostCreateNew(&CreatedPost, UnfPost->Title, UnfPost->Description, UnfPost->AuthorID,
		UnfPost->Tags, UnfPost->Images, UnfPost->ImageCount);

	*error = CreatePostImagesInDatabase(context, &UnfPost->Images, UnfPost->ImageCount);
	if (error->Code != ErrorCode_Success)
	{
		return NULL;
	}

	RemoveUnfinishedPostByAuthorID(context, authorID);
	*error = Error_CreateSuccess();
}

Error PostManager_CancelPostCreation(DBPostContext* context, unsigned long long authorID)
{

}


/* Deleting posts. */
Error PostManager_DeletePost(DBPostContext context, Post* post)
{

}

Error PostManager_DeleteAllPosts(ServerContext* serverContext)
{

}


/* Retrieving data. */
Post* PostManager_GetPostByID(DBPostContext* context, unsigned long long id, Error* error)
{

}

Post** PostManager_GetPostsByTitle(DBPostContext* context, const char* title, size_t* postCount)
{

}

char* PostManager_GetImageFromPost(DBPostContext* context, Post* post, int imageIndex)
{

}