/*
 * mm-naive.c - The least memory-efficient malloc package.
 * 
 * In this naive approach, a block is allocated by allocating a
 * new page as needed.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused.
 *
 * Name: Emile Goulard
 * uID: u1244855
 *
 * File Description: 
 * This is class encapsulates the simulation of a memory allocator.
 * This memory allocator uses an explicit free list implementation to add/delete
 * nodes in a list. These nodes act as a way to merge new pages and manage page chunks
 * (This increases the page overhead slightly (by two pointers).
 *
 * This requires having to create a "heap checker" for managing the page chunks that get allocated.
 * Therefore, I use the "extend_heap" function to create new blocks in memory and have a new node point
 * to that new space in memory.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

// Free list struct
typedef struct free_list{
  struct free_list *prev;
  struct free_list *next;
}free_list;

free_list *list_head;
static void add_node(void *bp);
static void delete_node(void *bp);

static void add_node(void *bp)
{
  free_list *curr_list = (free_list*)bp;
  curr_list->next = list_head;

  if (list_head != NULL)
    list_head->prev = curr_list;

  curr_list->prev = NULL;
  list_head = curr_list;
}

static void delete_node(void *bp)
{
  free_list *curr_list = (free_list*)(bp);
  if (curr_list->next == NULL && curr_list->prev == NULL)
  {
    list_head = NULL;
    curr_list->next = NULL;
    curr_list->prev = NULL;
  }
  else if (curr_list->next != NULL && curr_list->prev != NULL)
  {
    curr_list->next->prev = curr_list->prev;
    curr_list->prev->next = curr_list->next;
  }
  else if (curr_list->next != NULL)
  {
    list_head = curr_list->next;
    list_head->prev = NULL;
  }
  else
  {
    curr_list->prev->next = NULL;
  }
}

#define ALIGNMENT 16
#define BLOCK_OVERHEAD (2 * OVERHEAD)
#define PAGE_OVERHEAD (sizeof(block_header) * 4)
#define SIZE_DIFF (size - sizeof(size_t))
#define PAGESIZE 4096

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~(ALIGNMENT-1))

/* rounds up to the nearest multiple of mem_pagesize() */
#define PAGE_ALIGN(size) (((size) + (mem_pagesize()-1)) & ~(mem_pagesize()-1))

// This assumes you have a struct or typedef called "block_header" and "block_footer"
typedef size_t block_header;
typedef size_t block_footer;
#define OVERHEAD (sizeof(block_header)+sizeof(block_footer))

// Given a payload pointer, get the header or footer pointer
#define HDRP(bp) ((char *)(bp) - sizeof(block_header))
#define FTRP(bp) ((char *)(bp)+GET_SIZE(HDRP(bp))-OVERHEAD)

// Given a payload pointer, get the next or previous payload pointer
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)))
#define PREV_BLKP(bp) ((char *)(bp)-GET_SIZE((char *)(bp)-OVERHEAD))

// Given a pointer to a header, get or set its value
#define GET(p) (*(size_t *)(p))
#define PUT(p, val) (*(size_t *)(p) = (val))

// Combine a size and alloc bit
#define PACK(size, alloc) ((size) | (alloc))

// Given a header pointer, get the alloc or size 
#define GET_ALLOC(p) (GET(p) & 0x1)
#define GET_SIZE(p) (GET(p) & ~0xF)

/* Set a block to allocated
* Update block headers/footers as needed
* Update free list if applicable
* Split block if applicable
*/
static void *set_allocated(void *b, size_t size);

/* Request more memory by calling mem_map
* Initialize the new chunk of memory as applicable
* Update free list if applicable
*/
static void *extend_heap(size_t s);

/* Coalesce a free block if applicable
* Returns pointer to new coalesced block
*/
static void* coalesce(void *bp);

/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
  list_head = NULL;
  extend_heap(PAGESIZE * 10);
  return 0;
}

/* 
 * mm_malloc - Allocate a block by using bytes from current_avail,
 *     grabbing a new page if necessary.
 */
void *mm_malloc(size_t size)
{
  free_list* curr_list = list_head;
  while (curr_list != NULL)
  {
    if (GET_SIZE(HDRP(curr_list)) >= ALIGN(OVERHEAD + size))
      return set_allocated(curr_list, ALIGN(OVERHEAD + size));

    curr_list = curr_list->next;
  }
  void* new_heap = extend_heap(PAGE_ALIGN(ALIGN(OVERHEAD + size)) * (ALIGNMENT * 3 + 3));
  return set_allocated(new_heap, ALIGN(OVERHEAD + size));
}

/*
 * Free the block associated with the supplied address, coalescing and releasing pages if necessary.
 */
void mm_free(void *bp)
{
  long free_block_diff = (long)(bp - BLOCK_OVERHEAD);
  void* free_block = (void*)(free_block_diff);
  size_t overhead_block = GET_SIZE(HDRP(bp)) + PAGE_OVERHEAD;

  PUT(HDRP(bp), PACK(GET_SIZE(HDRP(bp)), 0));
  PUT(FTRP(bp), PACK(GET_SIZE(HDRP(bp)), 0));
  
  void* new_block = coalesce(bp);
  if (GET_SIZE(HDRP(NEXT_BLKP(new_block))) == 0 && GET_SIZE(HDRP(PREV_BLKP(new_block))) == BLOCK_OVERHEAD)
    mem_unmap(free_block, overhead_block);
}

static void *extend_heap(size_t size)
{
  char* s_map = mem_map(size);
  void* heap = (void*)(s_map + BLOCK_OVERHEAD);
  add_node(s_map + BLOCK_OVERHEAD);

  PUT(s_map, 0);
  PUT(s_map + 8, PACK(OVERHEAD, 1));
  PUT(FTRP(s_map + BLOCK_OVERHEAD), PACK(size - BLOCK_OVERHEAD, 0));
  PUT(s_map + SIZE_DIFF, PACK(0, 1));
  PUT(s_map + OVERHEAD, PACK(OVERHEAD, 1));
  PUT(s_map + OVERHEAD + 8, PACK(size - BLOCK_OVERHEAD, 0));

  return heap;
}

static void *set_allocated(void *b, size_t size) 
{
  size_t unallocated_space = GET_SIZE(HDRP(b));
  size_t space_diff = unallocated_space - size;
  delete_node(b);
   
  if (space_diff > PAGE_OVERHEAD)
  {
    PUT(HDRP(b), PACK(size, 1));
    PUT(FTRP(b), PACK(size, 1));
    PUT(HDRP(NEXT_BLKP(b)), PACK(space_diff, 1));
    PUT(FTRP(NEXT_BLKP(b)), PACK(space_diff, 1));
    add_node(NEXT_BLKP(b));
  }
  else
  {   
    PUT(HDRP(b), PACK(unallocated_space, 1));
    PUT(FTRP(b), PACK(unallocated_space, 1));
  }

  return b;
}

static void* coalesce(void *bp)
{
  size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
  size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
  size_t size = GET_SIZE(HDRP(bp));

  if (prev_alloc && next_alloc)
  {
    add_node(bp);
    return bp;
  }
  else if (prev_alloc && !next_alloc)
  {
    size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
    delete_node(NEXT_BLKP(bp));
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    add_node(bp);
  }
  else if (!prev_alloc && next_alloc)
  {
    size += GET_SIZE(HDRP(PREV_BLKP(bp)));
    PUT(FTRP(bp), PACK(size, 0));
    PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
    bp = PREV_BLKP(bp);
  }
  else
  {
    size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(HDRP(NEXT_BLKP(bp)));
    delete_node(NEXT_BLKP(bp));
    PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
    PUT(FTRP(PREV_BLKP(bp)), PACK(size, 0));
    bp = PREV_BLKP(bp);
  }

  return bp;
}
