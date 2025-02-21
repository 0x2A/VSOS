#pragma once

//#include <cstdint>

// Implementation of Windows double-linked lists (see list.h)

struct ListEntry
{
	struct ListEntry *Flink;
	struct ListEntry *Blink;
};

inline void ListInitializeHead(struct ListEntry *listHead)
{
	listHead->Flink = listHead->Blink = listHead;
}

inline char ListIsEmpty(const struct ListEntry *listHead)
{
	return (listHead->Flink == listHead);
}

inline void ListRemoveEntry(struct ListEntry *entry)
{
	struct ListEntry *flink = entry->Flink;
	struct ListEntry *blink = entry->Blink;
	blink->Flink = flink;
	flink->Blink = blink;
}

inline struct ListEntry *ListRemoveHead(struct ListEntry *listHead)
{
	struct ListEntry *flink = listHead->Flink;
	ListRemoveEntry(flink);
	return flink;
}

inline void ListInsertHead(struct ListEntry *listHead, struct ListEntry *entry)
{
	struct ListEntry *flink = listHead->Flink;
	entry->Flink = flink;
	entry->Blink = listHead;
	flink->Blink = entry;
	listHead->Flink = entry;
}

inline void ListInsertTail(struct ListEntry *listHead, struct ListEntry *entry)
{
	struct ListEntry *blink = listHead->Blink;
	entry->Flink = listHead;
	entry->Blink = blink;
	blink->Flink = entry;
	listHead->Blink = entry;
}

#define LIST_CONTAINING_RECORD(address, type, field) ((type *)((uintptr_t)(address) - \
								(uintptr_t)(&((type *)0)->field)))