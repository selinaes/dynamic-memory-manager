#include <stdio.h>  // needed for size_t etc.
#include <unistd.h> // needed for sbrk etc.
#include <sys/mman.h> // needed for mmap
#include <assert.h> // needed for asserts
#include "dmm.h"

/* 
 * The lab handout and code guide you to a solution with a single free list containing all free
 * blocks (and only the blocks that are free) sorted by starting address.  Every block (allocated
 * or free) has a header (type metadata_t) with list pointers, but no footers are defined.
 * That solution is "simple" but inefficient.  You can improve it using the concepts from the
 * reading.
 */

/* 
 *size_t is the return type of the sizeof operator.   size_t type is large enough to represent
 * the size of the largest possible object (equivalently, the maximum virtual address).
 */

typedef struct metadata {
  size_t size; // not including header size
  struct metadata* next;
  struct metadata* prev;
} metadata_t;

/*
 * Head of the freelist: pointer to the header of the first free block.
 */

static metadata_t* freelist = NULL;

//requested_size need to be aligned
void allocate_with_split(metadata_t* target_block, size_t requested_size) {
  //Only split if request size smaller than available in target block, and still leave space for a second header
  if ((target_block->size - requested_size) >= (int)METADATA_T_ALIGNED){ //If splitable to 2 blocks
    //Get a pointer to remaining 2nd block, and its size (except for header)
    metadata_t* split_remain_block = (metadata_t*)((char*)target_block + requested_size);
    size_t split_remain_size = target_block->size - requested_size - (int)METADATA_T_ALIGNED;

    //Set header of split away free block
    split_remain_block->size = split_remain_size;
    split_remain_block->prev = target_block->prev;
    split_remain_block->next = target_block->next;

    //Update freelist head if target_block is head
    if (target_block == freelist){
      freelist = split_remain_block;
    } else {
      //Set header of previous free block
      metadata_t* prev_free = target_block->prev;
      prev_free->next = split_remain_block;
    }

    //Set header of next of target_block
    if (target_block->next != NULL) {
      metadata_t* next_free = target_block->next;
      next_free->prev = split_remain_block;
    }

    //Set prev and next of allocated to NULL, size to smaller allocated size
    target_block->size = requested_size - (int)METADATA_T_ALIGNED;
    target_block->prev = NULL;
    target_block->next = NULL;
  } else { //If the exact same size, or not enough for split off a header
    // if start of freelist, set freelist to the next block (or NULL, if also the end of freelist)
    if (target_block == freelist){
      freelist = target_block->next;
    } else { //Set header of previous free block (when target_block not the start of freelist)
      target_block->prev->next = target_block->next;
    }
   
    //Set header of next free block
    if (target_block->next != NULL) {
      target_block->next->prev = target_block->prev;
    }

    //Set prev and next of allocated to NULL
    target_block->prev = NULL;
    target_block->next = NULL;
  }
}

void* search(size_t requested_size) {
  metadata_t* block = freelist;
  do{
    // if large enough, use it, otherwise search next block in freelist
    if (requested_size <= block->size){
      return block;
    } else {
      block = freelist->next;
    }
  } while (block->next != NULL);

  return NULL;
}

void* dmalloc(size_t numbytes) {

  if(freelist == NULL) {
    if(!dmalloc_init()) {
      return NULL;
    }
  }

  assert(numbytes > 0);

  // We need to allocate a space that includes payload + header.
  size_t request_block_size = ALIGN(numbytes) + METADATA_T_ALIGNED;

  //Search for suitable block size in free block list
  metadata_t* block = search(request_block_size);

  if (block == NULL){
    return NULL;
  }

  //Allocate & Split
  allocate_with_split(block, request_block_size);

  return (metadata_t*)((char*)block + METADATA_T_ALIGNED);
}

void dfree(void* ptr) {
  /* your code here */
}

/*
 * Allocate heap_region slab with a suitable syscall.
 */
bool dmalloc_init() {

  size_t max_bytes = ALIGN(MAX_HEAP_SIZE);

  /*
   * Get a slab with mmap, and put it on the freelist as one large block, starting
   * with an empty header.
   */
  freelist = (metadata_t*)
     mmap(NULL, max_bytes, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

  if (freelist == (void *)-1) {
    perror("dmalloc_init: mmap failed");
    return false;
  }
  freelist->next = NULL;
  freelist->prev = NULL;
  freelist->size = max_bytes-METADATA_T_ALIGNED;
  return true;
}


/* for debugging; can be turned off through -NDEBUG flag*/
/*

This code is here for reference.  It may be useful.
Warning: the NDEBUG flag also turns off assert protection.


void print_freelist(); 

#ifdef NDEBUG
	#define DEBUG(M, ...)
	#define PRINT_FREELIST print_freelist
#else
	#define DEBUG(M, ...) fprintf(stderr, "[DEBUG] %s:%d: " M "\n", __FILE__, __LINE__, ##__VA_ARGS__)
	#define PRINT_FREELIST
#endif


void print_freelist() {
  metadata_t *freelist_head = freelist;
  while(freelist_head != NULL) {
    DEBUG("\tFreelist Size:%zd, Head:%p, Prev:%p, Next:%p\t",
	  freelist_head->size,
	  freelist_head,
	  freelist_head->prev,
	  freelist_head->next);
    freelist_head = freelist_head->next;
  }
  DEBUG("\n");
}
*/
