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
  assert(requested_size >= (int)METADATA_T_ALIGNED
    && "request block size must be no smaller than header size");
  
  if (freelist == NULL) { //case when no free block exist
    return NULL;
  }

  metadata_t* block = freelist;

  while (block != NULL){ // enter this loop if cur block is not NULL (not past the last node)
    if (requested_size <= block->size){
      return block;
    } else {
      block = block->next;
    }
  }

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

  return ((char*)block + METADATA_T_ALIGNED);
}

void coalesce(metadata_t* block) {
  bool suc_is_free = false;
  metadata_t* successor;
  if (block->next != NULL){ // when block is not last in the list
    // get successor block
    successor = (metadata_t*)((char*)block + METADATA_T_ALIGNED + block->size);
    // whether successor next in freelist
    suc_is_free = (successor == block->next);
  }
  

  bool pred_is_free = false;
  if (block->prev != NULL){ // when block is not first in the list
    // get block's prev free block's successor
    metadata_t* prev_succ = (metadata_t*)((char*)block->prev + METADATA_T_ALIGNED + block->prev->size);
    // whether prev's successor is current block (aka whether predecessor is free)
    pred_is_free = (prev_succ == block);
  }

  // Case 1: both predecessor & successor allocated
  if ((!suc_is_free) && (!pred_is_free)){
    return;
  } else if (suc_is_free && (!pred_is_free)){ // Case 2: only successor free, need to be coalesced
    // New coalesced size
    size_t coalesced_size = block->size + METADATA_T_ALIGNED +successor->size;
    // Set size in coalesced block header
    block->size = coalesced_size;
    // Make coalesced block's next to point to successor's next
    block->next = successor->next;
    if (successor->next != NULL){
      // Make successor's next to point its prev to coalesced block (only when successor has next)
      successor->next->prev = block;
    }
  } else if ((!suc_is_free) && pred_is_free){ // Case 3: only predecessor free, need to be coalesced
    metadata_t* predecessor = block->prev;
    // New coalesced size
    size_t coalesced_size = block->size + METADATA_T_ALIGNED +predecessor->size;
    // Set size in coalesced block header (pred's header)
    predecessor->size = coalesced_size;
    // Make coalesced block's next to point to block's next
    predecessor->next = block->next;
    if (block->next != NULL){
      // Make block's next to point its prev to coalesced block (only when block has next)
      block->next->prev = predecessor;
    }
  } else if (suc_is_free && pred_is_free) { // Case 4: both predecessor and successor free, need to be coalesced
    metadata_t* predecessor = block->prev;
    // New coalesced_size
    size_t coalesced_size = predecessor->size + METADATA_T_ALIGNED +block->size + METADATA_T_ALIGNED +successor->size;
    // Set size in coalesced block header (pred's header)
    predecessor->size = coalesced_size;
    // Make coalesced block's next to point to successor's next
    predecessor->next = successor->next;
    if (successor->next != NULL){
      // Make successor's next to point its prev to coalesced block (only when successor has next)
      successor->next->prev = predecessor;
    }
  }
}

void dfree(void* ptr) {
   // Get the pointer to metadata of to-be-freed block
  metadata_t* header = (metadata_t*)((char*)ptr - METADATA_T_ALIGNED);

  // Inserting to-be-freed block into freelist
  if (freelist == NULL) { // case if freelist is empty
    freelist = header;
    header->prev = NULL;
    header->next = NULL;
  } else {
    metadata_t* block = freelist;
    while (block < header && block->next != NULL) {
      block = block->next;
    }

    if (block > header){ 
      // print_freelist(); 
      if (block->prev != NULL){ // insert to front of block
        // block->prev->next = header;
        metadata_t* prev_temp = block->prev;
        // fprintf(stderr, "[DEBU] %p: ",prev_temp->next);
        prev_temp->next = header; //prev has no next!
        header->prev = block->prev;
        header->next = block;
        block->prev = header;
      } else { // case where insert to front of list
        block->prev = header;
        header->next = block;
        header->prev = NULL;
        freelist = header;
      }
    } else if (block->next == NULL){ // insert to back of block, end of freelist
      block->next = header;
      header->prev = block;
      header->next = NULL;
    }
  }
  coalesce(header);
  print_freelist();
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
  print_freelist();
  return true;
}


/* for debugging; can be turned off through -NDEBUG flag*/


// This code is here for reference.  It may be useful.
// Warning: the NDEBUG flag also turns off assert protection.


// void print_freelist(); 

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

