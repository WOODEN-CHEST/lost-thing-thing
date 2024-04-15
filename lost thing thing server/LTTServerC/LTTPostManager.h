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

typedef struct PostCommentStruct
{
	unsigned long long ID;
	unsigned long long AuthorID;
	unsigned long long PostID;
	time_t CreationTime;
	time_t LastEditTime;
	unsigned long long ParentCommentID;
	const char* Contents;

} PostComment;

typedef struct PostStruct
{
	unsigned long long ID;
	unsigned long long AuthorID;
	const char* Title;
	const char* Description;

	const char* ThumbnailData;
	size_t ThumbnailDataLength;
	size_t ImageCount;

	time_t CreationTime;
	PostTagBitFlags Tags;

	unsigned long long* RequesterIDs;
	size_t RequesterCount;
	size_t _requesterCapacity;
	unsigned long long ClaimerID;

	PostComment* Comments;
	size_t CommentCount;
	size_t _commentCapacity;
} Post;

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

Error PostManager_Deconstruct(DBPostContext* context);


/* Creating posts. */
bool PostManager_BeginPostCreation(DBPostContext* context,
	UserAccount* author,
	const char* title,
	const char* description,
	PostTagBitFlags tags);

Error PostManager_UploadPostImage(DBPostContext* context, unsigned long long authorID, const char* imageData, size_t imageDataSLength);

Post* PostManager_FinishPostCreation(DBPostContext* context, unsigned long long authorID, Error* error);

void PostManager_CancelPostCreation(DBPostContext* context, unsigned long long authorID);


/* Deleting posts. */
Error PostManager_DeletePost(ServerContext* context, Post* post);

Error PostManager_DeleteAllPosts(ServerContext* serverContext);


/* Retrieving data. */
Post* PostManager_GetPostByID(DBPostContext* context, unsigned long long id, Error* error);

Post** PostManager_GetPostsByTitle(DBPostContext* context, const char* title, size_t* postCount);

char* PostManager_GetImageFromPost(DBPostContext* context, Post* post, int imageIndex);


/* Managing posts. */
bool PostManager_AddComment(DBPostContext* context, 
	Post* post,
	unsigned long long commentAuthorID,
	unsigned long long parentCommentID,
	const char* commentContents);

bool PostManager_RemoveComment(DBPostContext* context, Post* post, unsigned long long commentID);

bool PostManager_AddRequesterID(Post* post, unsigned long long requesterID);

bool PostManager_RemoveRequesterID(Post* post, unsigned long long requesterID);

bool PostManager_RemoveAllRequesterIDs(Post* post, unsigned long long requesterID);