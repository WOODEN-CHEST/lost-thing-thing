#pragma once
#include <stddef.h>
#include <stdbool.h>
#include "LTTErrors.h"
#include "LTTAccountManager.h"
#include "IDCodePointHashMap.h"

// Macros.
// Types.
typedef enum PostTagBitFlagsEnum
{
	PostTag_Stationery = 1,
	PostTag_Clothes = 2,
	PostTag_Devices = 4,
	PostTag_Books = 8,
	PostTag_Jewlery = 16,
	PostTag_Tanks = 32,
	PostTag_Other = 64
} PostTagBitFlags;

typedef struct PostStruct
{
	unsigned long long ID;
	unsigned long long AuthorID;
	const char* Title;
	const char* Description;

	const char* ThumbnailData;
	size_t ImageCount;

	PostTagBitFlags Tags;

	unsigned long long* RequesterIDs;
	size_t RequesterCount;
	size_t _requesterCapacity;
	unsigned long long ClaimerID;
} Post;

typedef struct PostCommentStruct
{
	unsigned long long ID;
	unsigned long long AuthorID;
	unsigned long long PostID;
	struct PostCommentStruct* ParentComment;
	const char* Contents;

} PostComment;

typedef struct DBPostContextStruct
{
	const char* PostRootPath;
	unsigned long long AvailablePostID;

	IDCodepointHashMap TitleMap;

	struct UnfinishedPostStruct* UnfinishedPosts;
	size_t UnfinishedPostCount;
	size_t _unfinishedPostCapacity;

	struct CachedPostStruct* CachedPosts;
} DBPostContext;



// Functions.
Error PostManager_Construct(ServerContext* serverContext);

void PostManager_Deconstruct(DBPostContext* context);


/* Creating posts. */
bool PostManager_BeginPostCreation(DBPostContext* context,
	UserAccount* author,
	const char* title,
	const char* description,
	PostTagBitFlags tags,
	Error* error);

Error PostManager_UploadPostImage(DBPostContext* context, unsigned long long authorID, const char* imageData, size_t imageDataSLength);

Post* PostManager_FinishPostCreation(DBPostContext* context, unsigned long long authorID, Error* error);

void PostManager_CancelPostCreation(DBPostContext* context, unsigned long long authorID);


/* Deleting posts. */
Error PostManager_DeletePost(DBPostContext context, Post* post);

Error PostManager_DeleteAllPosts(ServerContext* serverContext);


/* Retrieving data. */
Post* PostManager_GetPostByID(DBPostContext* context, unsigned long long id, Error* error);

Post** PostManager_GetPostsByTitle(DBPostContext* context, const char* title, size_t* postCount);

char* PostManager_GetImageFromPost(DBPostContext* context, Post* post, int imageIndex);