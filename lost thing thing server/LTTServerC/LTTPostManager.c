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
#include "LTTServerResourceManager.h"

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
#define POST_MAX_COMMENT_LENGTH_CODEPOINTS 8192
#define POST_UNCLAIMED_ID 0

#define DEFAULT_COMMENT_ID 1
#define COMMENT_NO_PARENT_COMMENT_ID 0


/* Post compound. */
#define ENTRY_ID_POST_ID 1 // ULong
#define ENTRY_ID_POST_TITLE 2 // String
#define ENTRY_ID_POST_DESCRIPTION 3 // String
#define ENTRY_ID_POST_TAGS 4 // Int
#define ENTRY_ID_POST_CLAIMER_ID 5 // ULong
#define ENTRY_ID_POST_REQUESTER_IDS 6 // ULong array, MAY NOT EXIST
#define ENTRY_ID_POST_COMMENT_ARRAY 7 // Compound array, MAY NOT EXIST
#define ENTRY_ID_POST_AUTHOR_ID 8 // ULong
#define ENTRY_ID_POST_CREATION_TIME 9 // Long
#define ENTRY_ID_POST_IMAGE_COUNT 10 // ulong

#define ENTRY_ID_COMMENT_ID 1 // ULong
#define ENTRY_ID_COMMENT_POST_ID 2 // ULong
#define ENTRY_ID_COMMENT_AUTHOR_ID 3 // ULong
#define ENTRY_ID_COMMENT_PARENT_COMMENT_ID 4 // Long
#define ENTRY_ID_COMMENT_CONTENTS 5 // String
#define ENTRY_ID_COMMENT_CREATION_TIME 6 // Long
#define ENTRY_ID_COMMENT_LAST_EDIT_TIME 7 // Long



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

static const char* GetPathToPostImageFile(DBPostContext* context, unsigned long long id, int imageIndex)
{
	StringBuilder Builder;
	StringBuilder_Construct(&Builder, DEFAULT_STRING_BUILDER_CAPACITY);
	BuildPathToPostDirectory(context, &Builder, id);

	char NumberString[32];
	snprintf(NumberString, sizeof(NumberString), "%d", imageIndex);
	StringBuilder_AppendChar(&Builder, PATH_SEPARATOR);
	StringBuilder_Append(&Builder, NumberString);
	StringBuilder_Append(&Builder, FILE_EXTENSION_PNG);

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

static bool VerifyText(const char* text, size_t maxLengthCodepoints)
{
	size_t CodepointLength = String_LengthCodepointsUTF8(text);
	if (CodepointLength > maxLengthCodepoints)
	{
		return false;
	}

	for (size_t i = 0; text[i] != '\0'; i += Char_GetByteCount(text + i))
	{
		if (!IsValidTextChar(text + i))
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

static bool VerifyComment(const char* comment)
{
	return VerifyText(comment, POST_MAX_COMMENT_LENGTH_CODEPOINTS);
}


/* Comment. */
static void CommentDeconstruct(PostComment* comment)
{
	Memory_Free((char*)comment->Contents);
}


/* Post. */
static void PostDeconstruct(Post* post)
{
	if (post->Title)
	{
		Memory_Free((char*)post->Title);
	}
	if (post->Description)
	{
		Memory_Free((char*)post->Description);
	}
	if (post->ThumbnailData)
	{
		Memory_Free((char*)post->ThumbnailData);
	}
	if (post->RequesterIDs)
	{
		Memory_Free(post->RequesterIDs);
	}
	if (post->Comments)
	{
		for (size_t i = 0; i < post->CommentCount; i++)
		{
			Memory_Free((char*)post->Comments[i].Contents);
		}
		Memory_Free(post->Comments);
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

	if (post->_requesterCapacity == 0)
	{
		post->_requesterCapacity = GENERIC_LIST_CAPACITY;
		post->RequesterIDs = (unsigned long long*)Memory_SafeMalloc(sizeof(unsigned long long) * post->_requesterCapacity);
	}

	while (post->_requesterCapacity < capacity)
	{
		post->_requesterCapacity *= GENERIC_LIST_GROWTH;
	}
	post->RequesterIDs = (unsigned long long*)Memory_SafeRealloc(post->RequesterIDs,
		sizeof(unsigned long long) * post->_requesterCapacity);
}

static bool PostHasRequesterID(Post* post, unsigned long long id)
{
	for (size_t i = 0; i < post->RequesterCount; i++)
	{
		if (post->RequesterIDs[i] == id)
		{
			return true;
		}
	}

	return false;
}

static bool PostAddRequesterID(Post* post, unsigned long long id)
{
	if (PostHasRequesterID(post, id))
	{
		return false;
	}

	PostEnsureRequesterCapacity(post, post->RequesterCount + 1);
	post->RequesterIDs[post->RequesterCount] = id;
	post->RequesterCount += 1;

	return true;
}

static void PostRemoveRequesterIDByIndex(Post* post, size_t index)
{
	if (post->RequesterCount == 0)
	{
		return;
	}

	for (size_t i = index + 1; i < post->RequesterCount; i++)
	{
		post->RequesterIDs[i - 1] = post->RequesterIDs[i];
	}
	post->RequesterIDs -= 1;
}

static bool PostRemoveRequesterID(Post* post, unsigned long long id)
{
	for (size_t i = 0; i < post->RequesterCount; i++)
	{
		if (post->RequesterIDs[i] == id)
		{
			PostRemoveRequesterIDByIndex(post, i);
			return true;
		}
	}

	return false;
}

static void PostClearRequesterIDs(Post* post)
{
	post->RequesterCount = 0;
}

static void PostEnsureCommentCapacity(Post* post, size_t capacity)
{
	if (post->_commentCapacity >= capacity)
	{
		return;
	}

	if (post->_commentCapacity == 0)
	{
		post->_commentCapacity = GENERIC_LIST_CAPACITY;
		post->Comments = (PostComment*)Memory_SafeMalloc(sizeof(PostComment) * post->_commentCapacity);
	}

	while (post->_commentCapacity < capacity)
	{
		post->_commentCapacity *= GENERIC_LIST_GROWTH;
	}
	post->Comments = (PostComment*)Memory_SafeRealloc(post->Comments,
		sizeof(PostComment) * post->_commentCapacity);
}

static void PostCreateNew(DBPostContext* context,
	Post* post,
	const char* title,
	const char* description,
	unsigned long long authorID,
	PostTagBitFlags tags,
	size_t imageCount)
{
	PostSetDefaultValues(post);

	post->ID = GetAndUsePostID(context);
	post->Title = String_CreateCopy(title);
	post->Description = String_CreateCopy(description);
	post->AuthorID = authorID;
	post->Tags = tags;
	post->ImageCount = imageCount;
}

static unsigned long long PostGetAvailableCommentID(Post* post)
{
	unsigned long long HighestID = 0;
	for (size_t i = 0; i < post->CommentCount; i++)
	{
		if (post->Comments[i].ID > HighestID)
		{
			HighestID = post->Comments[i].ID;
		}
	}
	return HighestID + 1;
}

static PostComment* PostGetCommentByID(Post* post, unsigned long long id)
{
	for (size_t i = 0; i < post->CommentCount; i++)
	{
		if (post->Comments[i].ID == id)
		{
			return post->Comments + i;
		}
	}
	return NULL;
}

static void RemovePostCommentByIndex(Post* post, size_t index)
{
	if (post->CommentCount == 0)
	{
		return;
	}

	CommentDeconstruct(post->Comments + index);

	for (size_t i = index + 1; i < post->CommentCount; i++)
	{
		post->Comments[i - 1] = post->Comments[i];
	}
	post->CommentCount--;
}

static bool RemovePostCommentByID(Post* post, unsigned long long id)
{
	for (size_t i = 0; i < post->CommentCount; i++)
	{
		if (post->Comments[i].ID == id)
		{
			RemovePostCommentByIndex(post, i);
			return true;
		}
	}
	return false;
}

static void ClearPostComments(Post* post)
{
	for (size_t i = 0; i < post->CommentCount; i++)
	{
		CommentDeconstruct(post->Comments + i);
	}
	post->CommentCount = 0;
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

	context->UnfinishedPostCount += 1;
}

static void UnfinishedPostDeconstruct(UnfinishedPost* post)
{
	Memory_Free((char*)post->Title);
	Memory_Free((char*)post->Description);
	for (size_t i = 0; i < post->ImageCount; i++)
	{
		Memory_Free((char*)post->Images[i].Data);
	}
}

static void RemoveUnfinishedPostByIndex(DBPostContext* context, size_t index)
{
	if (context->UnfinishedPostCount == 0)
	{
		return;
	}

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


/* Loading and saving posts. */
static void WriteSingleCommentToCompound(PostComment* comment, GHDFCompound* compound)
{
	GHDFPrimitive Value;

	Value.ULong = comment->ID;
	GHDFCompound_AddSingleValueEntry(compound, GHDFType_ULong, ENTRY_ID_COMMENT_ID, Value);
	Value.ULong = comment->PostID;
	GHDFCompound_AddSingleValueEntry(compound, GHDFType_ULong, ENTRY_ID_COMMENT_POST_ID, Value);
	Value.ULong = comment->AuthorID;
	GHDFCompound_AddSingleValueEntry(compound, GHDFType_ULong, ENTRY_ID_COMMENT_AUTHOR_ID, Value);
	Value.ULong = comment->ParentCommentID;
	GHDFCompound_AddSingleValueEntry(compound, GHDFType_ULong, ENTRY_ID_COMMENT_PARENT_COMMENT_ID, Value);
	Value.String = String_CreateCopy(comment->Contents);
	GHDFCompound_AddSingleValueEntry(compound, GHDFType_String, ENTRY_ID_COMMENT_CONTENTS, Value);
	Value.Long = comment->CreationTime;
	GHDFCompound_AddSingleValueEntry(compound, GHDFType_Long, ENTRY_ID_COMMENT_CREATION_TIME, Value);
	Value.Long = comment->LastEditTime;
	GHDFCompound_AddSingleValueEntry(compound, GHDFType_Long, ENTRY_ID_COMMENT_LAST_EDIT_TIME, Value);
}

static void WriteCommentsToCompound(Post* post, GHDFCompound* baseCompound)
{
	if (post->CommentCount == 0)
	{
		return;
	}

	GHDFPrimitive* CompoundArray = (GHDFPrimitive*)Memory_SafeMalloc(sizeof(GHDFPrimitive) * post->CommentCount);
	for (size_t i = 0; i < post->CommentCount; i++)
	{
		GHDFCompound* CommentCompound = (GHDFCompound*)Memory_SafeMalloc(sizeof(GHDFCompound));
		CompoundArray[i].Compound = CommentCompound;
		GHDFCompound_Construct(CommentCompound, COMPOUND_DEFAULT_CAPACITY);
		WriteSingleCommentToCompound(post->Comments + i, CommentCompound);
	}

	GHDFCompound_AddArrayEntry(baseCompound, GHDFType_Compound, ENTRY_ID_POST_COMMENT_ARRAY,
		CompoundArray, (unsigned int)post->CommentCount);
}

static void WriteRequestersToCompound(Post* post, GHDFCompound* compound)
{
	if (post->RequesterCount == 0)
	{
		return;
	}

	GHDFPrimitive* ValueArray = (GHDFPrimitive*)Memory_SafeMalloc(sizeof(GHDFPrimitive) * post->RequesterCount);
	for (unsigned int i = 0; i < (unsigned int)post->RequesterCount; i++)
	{
		ValueArray[i].ULong = post->RequesterIDs[i];
	}

	GHDFCompound_AddArrayEntry(compound, GHDFType_ULong, ENTRY_ID_POST_REQUESTER_IDS, ValueArray,
		(unsigned int)post->RequesterCount);
}

static void WritePostToCompound(Post* post, GHDFCompound* compound)
{
	GHDFPrimitive Value;
	Value.ULong = post->ID;
	GHDFCompound_AddSingleValueEntry(compound, GHDFType_ULong, ENTRY_ID_POST_ID, Value);
	Value.ULong = post->AuthorID;
	GHDFCompound_AddSingleValueEntry(compound, GHDFType_ULong, ENTRY_ID_POST_AUTHOR_ID, Value);
	Value.String = String_CreateCopy(post->Title);
	GHDFCompound_AddSingleValueEntry(compound, GHDFType_String, ENTRY_ID_POST_TITLE, Value);
	Value.String = String_CreateCopy(post->Description);
	GHDFCompound_AddSingleValueEntry(compound, GHDFType_String, ENTRY_ID_POST_DESCRIPTION, Value);
	Value.Long = post->CreationTime;
	GHDFCompound_AddSingleValueEntry(compound, GHDFType_Long, ENTRY_ID_POST_CREATION_TIME, Value);
	Value.Int = post->Tags;
	GHDFCompound_AddSingleValueEntry(compound, GHDFType_Int, ENTRY_ID_POST_TAGS, Value);
	Value.ULong = post->ImageCount;
	GHDFCompound_AddSingleValueEntry(compound, GHDFType_ULong, ENTRY_ID_POST_IMAGE_COUNT, Value);
	Value.ULong = post->ClaimerID;
	GHDFCompound_AddSingleValueEntry(compound, GHDFType_ULong, ENTRY_ID_POST_CLAIMER_ID, Value);

	WriteRequestersToCompound(post, compound);
	WriteCommentsToCompound(post, compound);
}

static Error WritePostToDatabase(DBPostContext* context, Post* post)
{
	GHDFCompound Compound;
	GHDFCompound_Construct(&Compound, COMPOUND_DEFAULT_CAPACITY);
	WritePostToCompound(post, &Compound);

	const char* FilePath = GetPathToIDFile(context, post->ID, GHDF_FILE_EXTENSION);
	const char* DirectoryPath = Directory_GetParentDirectory(FilePath);
	Directory_CreateAll(DirectoryPath);
	Memory_Free((char*)DirectoryPath);

	Error ReturnedError = GHDFCompound_WriteToFile(FilePath, &Compound);
	Memory_Free((char*)FilePath);
	GHDFCompound_Deconstruct(&Compound);
	return ReturnedError;
}

static Error CreatePostThumbnailInDatabase(DBPostContext* context, UnfinishedPostImage* image, unsigned long long postID)
{
	Image ReadImage;
	ReadImage.Data = stbi_load_from_memory((const unsigned char*)image->Data, (int)image->Length, &ReadImage.SizeX,
		&ReadImage.SizeY, &ReadImage.ColorChannels, STBI_rgb);

	if (!ReadImage.Data)
	{
		return Error_CreateError(ErrorCode_InvalidRequest, "Failed to create post thumbnail, couldn't read image.");
	}

	Image_ScaleImageToFit(&ReadImage, POST_MAX_THUMBNAIL_SIZE);


	const char* FilePath = GetPathToThumbnail(context, postID);
	int Result = stbi_write_png(FilePath, ReadImage.SizeX, ReadImage.SizeY, STBI_rgb, ReadImage.Data, 0);

	Memory_Free((char*)FilePath);
	Memory_Free((char*)ReadImage.Data);

	if (!Result)
	{
		return Error_CreateError(ErrorCode_IO, "Failed to write thumbnail to database.");
	}

	return Error_CreateSuccess();
}

static Error SaveSinglePostImageToDatabase(DBPostContext* context, unsigned long long postID, UnfinishedPostImage* image, int imageIndex)
{
	Image ReadImage;
	ReadImage.Data = stbi_load_from_memory((const unsigned char*)image->Data, (int)image->Length, &ReadImage.SizeX,
		&ReadImage.SizeY, &ReadImage.ColorChannels, STBI_rgb);

	if (!ReadImage.Data)
	{
		return Error_CreateError(ErrorCode_InvalidRequest, "Failed to create post image, couldn't read image.");
	}

	if ((ReadImage.SizeX > POST_MAX_IMAGE_SIZE) || (ReadImage.SizeY > POST_MAX_IMAGE_SIZE))
	{
		Image_ScaleImageToFit(&ReadImage, POST_MAX_THUMBNAIL_SIZE);
	}

	const char* FilePath = GetPathToPostImageFile(context, postID, imageIndex);
	int Result = stbi_write_png(FilePath, ReadImage.SizeX, ReadImage.SizeY, STBI_rgb, ReadImage.Data, 0);

	Memory_Free((char*)FilePath);
	Memory_Free((char*)ReadImage.Data);

	if (!Result)
	{
		return Error_CreateError(ErrorCode_IO, "Failed to write image to database.");
	}

	return Error_CreateSuccess();
}

static Error CreatePostImagesInDatabase(DBPostContext* context, unsigned long long postID, UnfinishedPostImage* images, size_t imageCount)
{
	if (imageCount == 0)
	{
		return Error_CreateSuccess();
	}

	Error ReturnedError = CreatePostThumbnailInDatabase(context, images, postID);
	if (ReturnedError.Code != ErrorCode_Success)
	{
		return ReturnedError;
	}

	for (size_t i = 0; i < imageCount; i++)
	{
		ReturnedError = SaveSinglePostImageToDatabase(context, postID, images + i, (int)i);
		if (ReturnedError.Code != ErrorCode_Success)
		{
			return ReturnedError;
		}
	}

	return Error_CreateSuccess();
}

static Error ReadSingleCommentFromCompound(PostComment* comment, GHDFCompound* compound)
{
	GHDFEntry* Entry;

	Error ReturnedError = GHDFCompound_GetVerifiedEntry(compound, ENTRY_ID_COMMENT_ID, &Entry, GHDFType_ULong, "Comment ID");
	if (ReturnedError.Code != ErrorCode_Success)
	{
		return ReturnedError;
	}
	comment->ID = Entry->Value.SingleValue.ULong;

	ReturnedError = GHDFCompound_GetVerifiedEntry(compound, ENTRY_ID_COMMENT_AUTHOR_ID, &Entry, GHDFType_ULong, "Comment Author ID");
	if (ReturnedError.Code != ErrorCode_Success)
	{
		return ReturnedError;
	}
	comment->AuthorID = Entry->Value.SingleValue.ULong;

	ReturnedError = GHDFCompound_GetVerifiedEntry(compound, ENTRY_ID_COMMENT_POST_ID, &Entry, GHDFType_ULong, "Comment Post ID");
	if (ReturnedError.Code != ErrorCode_Success)
	{
		return ReturnedError;
	}
	comment->PostID = Entry->Value.SingleValue.ULong;

	ReturnedError = GHDFCompound_GetVerifiedEntry(compound, ENTRY_ID_COMMENT_CREATION_TIME, &Entry,
		GHDFType_Long, "Comment Creation Time");
	if (ReturnedError.Code != ErrorCode_Success)
	{
		return ReturnedError;
	}
	comment->CreationTime = Entry->Value.SingleValue.Long;

	ReturnedError = GHDFCompound_GetVerifiedEntry(compound, ENTRY_ID_COMMENT_LAST_EDIT_TIME, &Entry,
		GHDFType_Long, "Comment Last Edit Time");
	if (ReturnedError.Code != ErrorCode_Success)
	{
		return ReturnedError;
	}
	comment->LastEditTime = Entry->Value.SingleValue.Long;

	ReturnedError = GHDFCompound_GetVerifiedEntry(compound, ENTRY_ID_COMMENT_PARENT_COMMENT_ID, &Entry,
		GHDFType_ULong, "Comment Parent Comment ID");
	if (ReturnedError.Code != ErrorCode_Success)
	{
		return ReturnedError;
	}
	comment->ParentCommentID = Entry->Value.SingleValue.ULong;

	ReturnedError = GHDFCompound_GetVerifiedEntry(compound, ENTRY_ID_COMMENT_CONTENTS, &Entry,
		GHDFType_String, "Comment Contents");
	if (ReturnedError.Code != ErrorCode_Success)
	{
		return ReturnedError;
	}
	comment->Contents = String_CreateCopy(Entry->Value.SingleValue.String);

	return Error_CreateSuccess();
}

static Error ReadCommentsFromCompound(Post* post, GHDFCompound* compound)
{
	GHDFEntry* Entry;
	Error ReturnedError = GHDFCompound_GetVerifiedOptionalEntry(compound, ENTRY_ID_POST_COMMENT_ARRAY, &Entry,
		GHDFType_Compound | GHDF_TYPE_ARRAY_BIT, "Post Comments");
	if (!Entry)
	{
		return Error_CreateSuccess();
	}
	if (ReturnedError.Code != ErrorCode_Success)
	{
		return ReturnedError;
	}

	PostEnsureCommentCapacity(post, Entry->Value.ValueArray.Size);
	post->CommentCount = Entry->Value.ValueArray.Size;
	for (unsigned int i = 0; i < post->CommentCount; i++)
	{
		ReturnedError = ReadSingleCommentFromCompound(post->Comments + i, Entry->Value.ValueArray.Array[i].Compound);
		if (ReturnedError.Code != ErrorCode_Success)
		{
			return ReturnedError;
		}
	}

	return Error_CreateSuccess();
}

static Error ReadRequestersFromCompound(Post* post, GHDFCompound* compound)
{
	GHDFEntry* Entry;
	Error ReturnedError = GHDFCompound_GetVerifiedOptionalEntry(compound, ENTRY_ID_POST_REQUESTER_IDS, &Entry,
		GHDFType_ULong | GHDF_TYPE_ARRAY_BIT, "Post Requesters");
	if (!Entry)
	{
		return Error_CreateSuccess();
	}
	if (ReturnedError.Code != ErrorCode_Success)
	{
		return ReturnedError;
	}

	PostEnsureRequesterCapacity(post, Entry->Value.ValueArray.Size);
	post->RequesterCount = Entry->Value.ValueArray.Size;
	for (unsigned int i = 0; i < post->RequesterCount; i++)
	{
		post->RequesterIDs[i] = Entry->Value.ValueArray.Array[i].ULong;
	}

	return Error_CreateSuccess();
}

static Error ReadPostFromCompound(Post* post, GHDFCompound* compound)
{
	GHDFEntry* Entry;

	Error ReturnedError = GHDFCompound_GetVerifiedEntry(compound, ENTRY_ID_POST_ID, &Entry, GHDFType_ULong, "Post ID");
	if (ReturnedError.Code != ErrorCode_Success)
	{
		return ReturnedError;
	}
	post->ID = Entry->Value.SingleValue.ULong;

	ReturnedError = GHDFCompound_GetVerifiedEntry(compound, ENTRY_ID_POST_AUTHOR_ID, &Entry, GHDFType_ULong, "Post Author ID");
	if (ReturnedError.Code != ErrorCode_Success)
	{
		return ReturnedError;
	}
	post->AuthorID = Entry->Value.SingleValue.ULong;

	ReturnedError = GHDFCompound_GetVerifiedEntry(compound, ENTRY_ID_POST_TITLE, &Entry, GHDFType_String, "Post Title");
	if (ReturnedError.Code != ErrorCode_Success)
	{
		return ReturnedError;
	}
	post->Title = String_CreateCopy(Entry->Value.SingleValue.String);

	ReturnedError = GHDFCompound_GetVerifiedEntry(compound, ENTRY_ID_POST_DESCRIPTION, &Entry, GHDFType_String, "Post Description");
	if (ReturnedError.Code != ErrorCode_Success)
	{
		return ReturnedError;
	}
	post->Description = String_CreateCopy(Entry->Value.SingleValue.String);

	ReturnedError = GHDFCompound_GetVerifiedEntry(compound, ENTRY_ID_POST_IMAGE_COUNT, &Entry, GHDFType_ULong, "Post Image Count");
	if (ReturnedError.Code != ErrorCode_Success)
	{
		return ReturnedError;
	}
	post->ImageCount = Entry->Value.SingleValue.ULong;

	ReturnedError = GHDFCompound_GetVerifiedEntry(compound, ENTRY_ID_POST_TAGS, &Entry, GHDFType_Int, "Post Tags");
	if (ReturnedError.Code != ErrorCode_Success)
	{
		return ReturnedError;
	}
	post->Tags = Entry->Value.SingleValue.Int;

	ReturnedError = GHDFCompound_GetVerifiedEntry(compound, ENTRY_ID_POST_CLAIMER_ID, &Entry, GHDFType_ULong, "Post Claimer ID");
	if (ReturnedError.Code != ErrorCode_Success)
	{
		return ReturnedError;
	}
	post->ClaimerID = Entry->Value.SingleValue.ULong;

	ReturnedError = ReadRequestersFromCompound(post, compound);
	if (ReturnedError.Code != ErrorCode_Success)
	{
		return ReturnedError;
	}

	ReturnedError = ReadCommentsFromCompound(post, compound);
	if (ReturnedError.Code != ErrorCode_Success)
	{
		return ReturnedError;
	}

	return Error_CreateSuccess();
}

static Error ReadPostThumbnailData(DBPostContext* context, Post* post)
{
	post->ThumbnailData = NULL;
	const char* ThumbnailPath = GetPathToThumbnail(context, post->ID);
	if (!File_Exists(ThumbnailPath))
	{
		Memory_Free((char*)ThumbnailPath);
		return Error_CreateSuccess();
	}

	Error ReturnedError;
	FILE* File = File_Open(ThumbnailPath, FileOpenMode_ReadBinary, &ReturnedError);
	Memory_Free((char*)ThumbnailPath);
	if (ReturnedError.Code != ErrorCode_Success)
	{
		return ReturnedError;
	}
	post->ThumbnailData = File_ReadAllData(File, &post->ThumbnailDataLength, &ReturnedError);
	File_Close(File);
	return ReturnedError;
}

static bool ReadPostFromDatabase(DBPostContext* context, Post* post, unsigned long long id, Error* error)
{
	*error = Error_CreateSuccess();
	const char* FilePath = GetPathToIDFile(context, id, GHDF_FILE_EXTENSION);
	if (!File_Exists(FilePath))
	{
		Memory_Free((char*)FilePath);
		return false;
	}

	GHDFCompound Compound;
	*error = GHDFCompound_ReadFromFile(FilePath, &Compound);
	Memory_Free((char*)FilePath);
	if (error->Code != ErrorCode_Success)
	{
		return false;
	}

	PostSetDefaultValues(post);
	*error = ReadPostFromCompound(post, &Compound);
	GHDFCompound_Deconstruct(&Compound);
	if (error->Code != ErrorCode_Success)
	{
		PostDeconstruct(post);
		return false;
	}

	*error = ReadPostThumbnailData(context, post);
	if (error->Code != ErrorCode_Success)
	{
		PostDeconstruct(post);
		return false;
	}

	return true;
}

static void DeletePostFromDatabase(DBPostContext* context, unsigned long long id)
{
	StringBuilder PathBuilder;
	StringBuilder_Construct(&PathBuilder, DEFAULT_STRING_BUILDER_CAPACITY);
	BuildPathToPostDirectory(context, &PathBuilder, id);

	Directory_Delete(PathBuilder.Data);
	StringBuilder_Deconstruct(&PathBuilder);
}


/* Post cache. */
static void InitializePostCache(DBPostContext* context)
{
	context->CachedPosts = (CachedPost*)Memory_SafeMalloc(sizeof(CachedPost) * CACHED_POST_COUNT);
	for (int i = 0; i < CACHED_POST_COUNT; i++)
	{
		context->CachedPosts[i].LastAccessTime = CACHED_POST_UNLOADED_TIME;
	}
}

static Error ClearCacheSpotByIndex(DBPostContext* context, int index, bool savePost)
{
	CachedPost* PostCached = context->CachedPosts + index;
	if (PostCached->LastAccessTime != CACHED_POST_UNLOADED_TIME && savePost)
	{
		if (savePost)
		{
			Error ReturnedError = WritePostToDatabase(context, &PostCached->TargetPost);
			if (ReturnedError.Code != ErrorCode_Success)
			{
				return ReturnedError;
			}
		}
		PostDeconstruct(&PostCached->TargetPost);
		PostCached->LastAccessTime = CACHED_POST_UNLOADED_TIME;
	}

	return Error_CreateSuccess();
}

static CachedPost* GetCacheSpotForPost(DBPostContext* context, Error* error, bool savePost)
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

	*error = ClearCacheSpotByIndex(context, LowestTimeIndex, savePost);
	return context->CachedPosts + LowestTimeIndex;
}

static CachedPost* LoadPostIntoCache(DBPostContext* context, unsigned long long postID, Error* error)
{
	CachedPost* PostCached = GetCacheSpotForPost(context, error, true);
	if (error->Code != ErrorCode_Success)
	{
		return NULL;
	}

	PostCached->LastAccessTime = time(NULL);
	if (!ReadPostFromDatabase(context, &PostCached->TargetPost, postID, error))
	{
		return NULL;
	}

	return PostCached;
}

static Post* TryGetPostFromCache(DBPostContext* context, unsigned long long id)
{
	for (int i = 0; i < CACHED_POST_COUNT; i++)
	{
		CachedPost* Post = context->CachedPosts + i;
		if ((Post->TargetPost.ID == id) && (Post->LastAccessTime != CACHED_POST_UNLOADED_TIME))
		{
			return &Post->TargetPost;
		}
	}

	return NULL;
}

static Error SaveAllCachedPostsToDatabase(DBPostContext* context)
{
	for (int i = 0; i < CACHED_POST_COUNT; i++)
	{
		if (context->CachedPosts[i].LastAccessTime != CACHED_POST_UNLOADED_TIME)
		{
			Error ReturnedError = WritePostToDatabase(context, &context->CachedPosts[i].TargetPost);
			if (ReturnedError.Code != ErrorCode_Success)
			{
				return ReturnedError;
			}
		}
	}

	return Error_CreateSuccess();
}

static Error RemovePostFromCacheByID(DBPostContext* context, unsigned long long id, bool savePost)
{
	for (int i = 0; i < CACHED_POST_COUNT; i++)
	{
		if (context->CachedPosts[i].TargetPost.ID == id)
		{
			return ClearCacheSpotByIndex(context, i, savePost);
		}
	}

	return Error_CreateSuccess();
}


/* ID Hashmap. */
static void GenerateMetaInfoFroSinglePost(DBPostContext* context, Post* post)
{
	IDCodepointHashMap_AddID(&context->TitleMap, post->Title, post->ID);
}

static Error GenerateMetaInfoForPosts(DBPostContext* context, size_t* readPostCount)
{
	*readPostCount = 0;
	size_t ReadPostCount = 0;

	for (unsigned long long id = DEFAULT_AVAILABLE_POST_ID; id < context->AvailablePostID; id++)
	{
		Error ReturnedError;
		
		Post TargetPost;
		if (!ReadPostFromDatabase(context, &TargetPost, id, &ReturnedError))
		{
			if (ReturnedError.Code != ErrorCode_Success)
			{
				return ReturnedError;
			}
			continue;
		}
		ReadPostCount++;
		
		GenerateMetaInfoFroSinglePost(context, &TargetPost);
		PostDeconstruct(&TargetPost);
	}

	*readPostCount = ReadPostCount;
	return Error_CreateSuccess();
}

static void ClearMetaInfoForSinglePost(DBPostContext* context, Post* post)
{
	IDCodepointHashMap_RemoveID(&context->TitleMap, post->Title, post->ID);
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
	context->AvailablePostID = Entry->Value.SingleValue.ULong;

	return Error_CreateSuccess();
}

static Error LoadMetainfo(DBPostContext* context)
{
	LoadDefaultMetaInfo(context);

	const char* FilePath = Directory_CombinePaths(context->PostRootPath, POST_METAINFO_FILENAME);
	if (!File_Exists(FilePath))
	{
		Memory_Free((char*)FilePath);
		return Error_CreateSuccess();
	}
	GHDFCompound Compound;
	Error ReturnedError = GHDFCompound_ReadFromFile(FilePath, &Compound);
	Memory_Free((char*)FilePath);

	if (ReturnedError.Code != ErrorCode_Success)
	{
		return ReturnedError;
	}
	ReturnedError = LoadMetainfoFromCompound(context, &Compound);
	GHDFCompound_Deconstruct(&Compound);
	
	return ReturnedError;
}

static Error SaveMetaInfo(DBPostContext* context)
{
	GHDFCompound Compound;
	GHDFCompound_Construct(&Compound, COMPOUND_DEFAULT_CAPACITY);
	GHDFPrimitive Value;

	Value.ULong = context->AvailablePostID;
	GHDFCompound_AddSingleValueEntry(&Compound, GHDFType_ULong, ENTRY_ID_METAINFO_AVAIlABLE_POST_ID, Value);

	Directory_CreateAll(context->PostRootPath);
	const char* FilePath = Directory_CombinePaths(context->PostRootPath, POST_METAINFO_FILENAME);
	Error ReturnedError = GHDFCompound_WriteToFile(FilePath, &Compound);

	Memory_Free((char*)FilePath);
	GHDFCompound_Deconstruct(&Compound);

	return ReturnedError;
}


// Functions.
Error PostManager_Construct(ServerContext* serverContext)
{
	DBPostContext* Context = serverContext->PostContext;

	Context->PostRootPath = Directory_CombinePaths(serverContext->Resources->DatabaseRootPath, DIR_NAME_POSTS);

	Context->_unfinishedPostCapacity = GENERIC_LIST_CAPACITY;
	Context->UnfinishedPosts = (UnfinishedPost*)Memory_SafeMalloc(sizeof(UnfinishedPost) * Context->_unfinishedPostCapacity);
	Context->UnfinishedPostCount = 0;

	InitializePostCache(Context);
	
	Error ReturnedError = LoadMetainfo(Context);
	if (ReturnedError.Code != ErrorCode_Success)
	{
		return ReturnedError;
	}

	IDCodepointHashMap_Construct(&Context->TitleMap);

	size_t ReadPostCount;
	ReturnedError = GenerateMetaInfoForPosts(Context, &ReadPostCount);
	if (ReturnedError.Code != ErrorCode_Success)
	{
		return ReturnedError;
	}
	char Message[128];
	snprintf(Message, sizeof(Message), "Read %llu posts while creating ID hashes.", ReadPostCount);
	Logger_LogInfo(serverContext->Logger, Message);

	return ReturnedError;
}

Error PostManager_Deconstruct(DBPostContext* context)
{
	Error ReturnedError = SaveAllCachedPostsToDatabase(context);
	if (ReturnedError.Code != ErrorCode_Success)
	{
		return ReturnedError;
	}

	ReturnedError = SaveMetaInfo(context);
	if (ReturnedError.Code != ErrorCode_Success)
	{
		return ReturnedError;
	}

	Memory_Free((char*)context->PostRootPath);
	IDCodepointHashMap_Deconstruct(&context->TitleMap);
	Memory_Free(context->CachedPosts);
	Memory_Free(context->UnfinishedPosts);

	return Error_CreateSuccess();
}


/* Creating posts. */
bool PostManager_BeginPostCreation(DBPostContext* context,
	UserAccount* author,
	const char* title,
	const char* description,
	PostTagBitFlags tags)
{
	if (!VerifyPostTitle(title) || !VerifyPostDescription(description))
	{
		return false;
	}

	AddUnfinishedPost(context, author->ID, title, description, tags);
	return true;
}

Error PostManager_UploadPostImage(DBPostContext* context, unsigned long long authorID, const char* imageData, size_t imageDataLength)
{
	UnfinishedPostsRefresh(context);
	UnfinishedPost* Post = GetUnfinishedPostByAuthorID(context, authorID);
	if (!Post)
	{
		return Error_CreateError(ErrorCode_InvalidRequest, "Couldn't find the requested unfinished post for image upload.");
	}
	if (Post->ImageCount >= POST_MAX_IMAGES)
	{
		return Error_CreateSuccess();
	}

	Post->Images[Post->ImageCount].Data = (char*)Memory_SafeMalloc(imageDataLength);
	Memory_Copy(imageData, (char*)Post->Images[Post->ImageCount].Data, imageDataLength);
	Post->Images[Post->ImageCount].Length = imageDataLength; 

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
	PostCreateNew(context, &CreatedPost, UnfPost->Title, UnfPost->Description, UnfPost->AuthorID,
		UnfPost->Tags, UnfPost->ImageCount);

	*error = WritePostToDatabase(context, &CreatedPost);
	if (error->Code != ErrorCode_Success)
	{
		PostDeconstruct(&CreatedPost);
		return NULL;
	}

	*error = CreatePostImagesInDatabase(context, CreatedPost.ID, UnfPost->Images, UnfPost->ImageCount);
	if (error->Code != ErrorCode_Success)
	{
		DeletePostFromDatabase(context, CreatedPost.ID);
		PostDeconstruct(&CreatedPost);
		return NULL;
	}

	GenerateMetaInfoFroSinglePost(context, &CreatedPost);

	unsigned long long PostID = CreatedPost.ID;
	PostDeconstruct(&CreatedPost);
	RemoveUnfinishedPostByAuthorID(context, authorID);
	return PostManager_GetPostByID(context, PostID, error);
}

void PostManager_CancelPostCreation(DBPostContext* context, unsigned long long authorID)
{
	UnfinishedPostsRefresh(context);
	RemoveUnfinishedPostByAuthorID(context, authorID);
}


/* Deleting posts. */
Error PostManager_DeletePost(ServerContext* serverContext, Post* post)
{
	char Message[128];
	snprintf(Message, sizeof(Message), "Deleting post with ID %llu (author id %llu)", post->ID, post->AuthorID);
	Logger_LogInfo(serverContext->Logger, Message);

	
	Error ReturnedError = RemovePostFromCacheByID(serverContext->PostContext, post->ID, false);
	if (ReturnedError.Code != ErrorCode_Success)
	{
		return ReturnedError;
	}
	DeletePostFromDatabase(serverContext->PostContext, post->ID);
	ClearMetaInfoForSinglePost(serverContext->PostContext, post);
	return Error_CreateSuccess();
}

Error PostManager_DeleteAllPosts(ServerContext* serverContext)
{
	for (unsigned long id = 0; id < serverContext->PostContext->AvailablePostID; id++)
	{
		Error ReturnedError;
		Post* ReturnedPost = PostManager_GetPostByID(serverContext->PostContext, id, &ReturnedError);
		if (ReturnedError.Code != ErrorCode_Success)
		{
			return ReturnedError;
		}

		ReturnedError = PostManager_DeletePost(serverContext, ReturnedPost);
		if (ReturnedError.Code != ErrorCode_Success)
		{
			return ReturnedError;
		}
	}

	return Error_CreateSuccess();
}


/* Retrieving data. */
Post* PostManager_GetPostByID(DBPostContext* context, unsigned long long id, Error* error)
{
	Post* CreatedPost = TryGetPostFromCache(context, id);
	if (CreatedPost)
	{
		return CreatedPost;
	}

	CachedPost* PostCached = LoadPostIntoCache(context, id, error);
	return PostCached ? &PostCached->TargetPost : NULL;
}

Post** PostManager_GetPostsByTitle(DBPostContext* context, const char* title, size_t* postCount, Error* error)
{
	*error = Error_CreateSuccess();
	*postCount = 0;
	size_t IDCount;
	unsigned long long* IDs = IDCodepointHashMap_FindByString(&context->TitleMap, title, true, &IDCount);

	if (IDCount == 0)
	{
		return NULL;
	}

	Post** FoundPosts = (Post**)Memory_SafeMalloc(sizeof(Post*) * IDCount);
	size_t MatchedPostCount = 0;

	for (size_t i = 0; i < IDCount; i++)
	{
		Post* TargetPost = PostManager_GetPostByID(context, IDs[i], error);
		if (error->Code != ErrorCode_Success)
		{
			Memory_Free(FoundPosts);
			Memory_Free(IDs);
			return NULL;
		}
		if(!TargetPost)
		{
			continue;
		}
		if (String_IsFuzzyMatched(TargetPost->Title, title, true))
		{
			FoundPosts[MatchedPostCount] = TargetPost;
			MatchedPostCount++;
		}
	}

	Memory_Free(IDs);
	if (MatchedPostCount == 0)
	{
		Memory_Free(FoundPosts);
		return NULL;
	}
	*postCount = MatchedPostCount;
	return FoundPosts;
}

char* PostManager_GetImageFromPost(DBPostContext* context, Post* post, int imageIndex, size_t* dataLength, Error* error)
{
	*error = Error_CreateSuccess();
	if (imageIndex >= post->ImageCount)
	{
		return NULL;
	}

	const char* ImagePath = GetPathToPostImageFile(context, post->ID, imageIndex);
	FILE* File = File_Open(ImagePath, FileOpenMode_ReadBinary, error);
	if (error->Code != ErrorCode_Success)
	{
		Memory_Free((char*)ImagePath);
		return NULL;
	}

	char* Data = File_ReadAllData(File, dataLength, error);
	Memory_Free((char*)ImagePath);
	File_Close(File);
	return Data;
}

bool PostManager_AddComment(Post* post,
	unsigned long long commentAuthorID,
	unsigned long long parentCommentID,
	const char* commentContents)
{
	if (!VerifyComment(commentContents))
	{
		return false;
	}

	PostEnsureCommentCapacity(post, post->CommentCount + 1);
	PostComment* Comment = post->Comments + post->CommentCount;

	Comment->PostID = post->ID;
	Comment->AuthorID = commentAuthorID;
	Comment->CreationTime = time(NULL);
	Comment->LastEditTime = Comment->CreationTime;
	Comment->ID = PostGetAvailableCommentID(post);
	Comment->ParentCommentID = PostGetCommentByID(post, parentCommentID) ? parentCommentID : COMMENT_NO_PARENT_COMMENT_ID;
	Comment->Contents = String_CreateCopy(commentContents);

	post->CommentCount += 1;
	return true;
}

PostComment* PostManager_GetComment(Post* post, unsigned long long commentID)
{
	return PostGetCommentByID(post, commentID);
}

bool PostManager_RemoveComment(Post* post, unsigned long long commentID)
{
	return RemovePostCommentByID(post, commentID);
}

bool PostManager_AddRequesterID(Post* post, unsigned long long requesterID)
{
	return PostAddRequesterID(post, requesterID);
}

bool PostManager_HasRequesterID(Post* post, unsigned long long id)
{
	return PostHasRequesterID(post, id);
}

bool PostManager_RemoveRequesterID(Post* post, unsigned long long requesterID)
{
	return PostRemoveRequesterID(post, requesterID);
}

void PostManager_RemoveAllRequesterIDs(Post* post)
{
	PostClearRequesterIDs(post);
}