# Dynamic memory manager

For this project you will implement a basic heap manager. The standard C runtime library provides a standard heap manager through the malloc and free library calls. For this project you will implement your own version of this interface.

# Recommended reading 

* [basic from OSTEP](http://pages.cs.wisc.edu/~remzi/OSTEP/vm-api.pdf)
* [free space management (ch 17) from OSTEP](http://pages.cs.wisc.edu/~remzi/OSTEP/vm-freespace.pdf)
* [advanced from CS:APP](https://www2.cs.duke.edu/courses/fall20/compsci310/internal/dynamicmem.pdf)


# Introduction

First you should understand the interface, purpose, and function of a heap manager. On any properly installed Unix system typing "man malloc" in a shell or terminal window will output documentation for the interface.

You will implement a the heap API operators dmalloc and dfree. From the perspective of the user program (the program using your heap manager), the behavior should be equivalent to the standard malloc and free. We have supplied some code to get you started, including a header file called dmm.h. Please use that header file and API in your solution, and do not change it.

The lab is designed to build your understanding of how data is laid out in memory, and how operating systems manage memory. Your heap manager should be designed to use memory efficiently: you should understand the issues related to memory fragmentation, and some techniques for minimizing fragmentation and achieving good memory utilization.

As a side benefit, the lab will you get better at system programming in the C language, and manipulating data structures "underneath" the type system. We strongly encourage you to start early and familiarize yourself with C and its pitfalls and with the C/Unix development environment. You will need to know some basic Unix command line tools: man, cd, ls, cat, more/less, pwd, cp, mv, rm, diff, make, and an editor/IDE of some kind. Also, debugging will go much more easily if you use a debugger such as gdb, at least a little.

# Dynamic memory allocation

At any given time, the heap consists of a sequence of blocks. Each heap block is a contiguous sequence of bytes. Every byte in the heap is part of exactly one heap block. Each block is either allocated or free.

Heap blocks are variably sized. The borders between the blocks shift as blocks are allocated (with dmalloc) and freed (with dfree). In particular, the heap manager splits and coalesces heap blocks to satisfy the mix of arriving heap requests. The heap manager must be careful to track the borders and the status of each block. The following subsections discuss the implementation in more detail.

## Block metadata: headers

The heap manager places a header at the start of each block of the heap space. A block's header represents some information about the block, including the block's size. In general "metadata" is data about data: the header describes the block, but it is not user data. The rest of the block is available for use by the user program. The user program does not see the metadata headers, which are only for the internal use of the heap manager.

The code we provide defines metadata_t as a data structure template (a struct type) for the block headers. The intent is that each heap block will have a metadata_t structure at the top of it, whether the block is allocated or free. The metadata_t structure is defined in dmm.c as:

    typedef struct metadata {
    
      size_t size;
      struct metadata* next;
      struct metadata* prev;
    
    } metadata_t;


The block header is useful for two reasons. First, a block's header indicates whether the block is allocated or free. A heap manager must track that information so that it does not allocate the same region or overlapping regions of memory for two different dmalloc() calls by accident.

Second, the block headers help track and locate the borders between heap blocks. This makes it possible to coalesce free heap blocks to form larger blocks, which may be needed for later large dmalloc requests. The supplied metadata structure makes it easy to link the headers of the free blocks into a list (the free list).

There are many ways to implement a heap manager. The most efficient schemes also place a footer at the end of each block. You may use footers if you wish, but they are not required.

## Initialization: sbrk

When a heap manager is initialized it obtains a large slab of virtual memory to carve up into blocks. It does this by requesting virtual memory from the operating system kernel using a system call, e.g., mmap or sbrk.

The supplied code uses sbrk to extend the data segment of the virtual address space. The sbrk system call causes a region of the virtual memory that was previously unused (and invalid) to be registered for use: the kernel sets up page tables for the region and zero-fills each page of the region as it is referenced. sbrk returns a pointer to the new region, i.e., the "slab".

Initially the heap consists of a single free heap block that contains the entire slab. The supplied code casts the slab address to a metadata_t pointer and places it in a global pointer variable called heap_region:

    metadata_t* heap_region = (metadata_t*) sbrk(MAX_HEAP_SIZE);


The heap_region pointer references the header for the first and only block in the heap, which initially is a free block. A "real" heap manager may obtain additional slabs as needed. For project 0 we limit the number of sbrk calls to HEAP_SYSCALL_LIMIT for evaluation purposes. The default value is 1.

## API: dmalloc and dfree

All of the heap blocks you allocate with dmalloc() should come from the one initial slab. Whenever dmalloc allocates a block, it returns a pointer to the block for use by the client application. The returned pointer should skip past the block's header, so that the program does not overwrite it. The returned pointer should be aligned on a long-word boundary. Be sure that you understand what this means and why it is important by reading [this](https://www2.cs.duke.edu/courses/fall20/compsci310/internal/dynamicmem.pdf).

The supplied code includes some macros to assist you in dmm.h. It also makes it easy to keep track of the available space using a doubly linked list of headers of the free heap blocks, called a freelist. At the start of the program, the freelist is initialized to contain a single large block, consisting of the entire slab pointed to by heap region.

## Splitting a free heap block

It is often useful to split a free heap block on a call to dmalloc. The split produces two contiguous free heap blocks of variable size, within the address range of the original block before the split. You must implement a split operation: without splitting, the heap could never contain more than one block.

For a split, we first need to check whether the requested size is less than space available in the target block. If so, the most memory-efficient approach is to allocate a block of the requested size by splitting it off of the target block, leaving the rest of the target block free. The first block is returned to the caller and the second block remains in the freelist. The metadata headers in both blocks must be updated accordingly to reflect their sizes.

## Freeing space: coalescing

A client program frees an allocated heap block by calling dfree(), passing a pointer to the block to free. The heap manager must reclaim this space so that it becomes available for use by a future dmalloc. One solution is to just insert the block back into the freelist.

As blocks are allocated and freed, you may end up with adjacent blocks that are both free. In that case, it is necessary to combine the adjacent blocks into a single contiguous heap block. This is called coalescing. Coalescing makes it possible to reuse freed space as part of a future block of a larger size. Without coalescing, a program that fills its heap with small blocks could never allocate a large block, even if it frees all of the heap memory. We say that a heap is fragmented if its space is available only in small blocks.

You have a couple of options to perform coalescing. First, you can coalesce as you exit the dfree() call. Second, you can periodically or explicitly coalesce when you are no longer able to find a sufficiently large block to satisfy a call to dmalloc.

One optimization we can perform is to keep the freelist in sorted order with respect to addresses so that you can do the coalescing in one pass of the list. For example, if your coalescing function were to start at the beginning of the freelist and iterate through the end, at any block it could look up its adjacent blocks on the left and the right ("above" and "below"). If free blocks are contiguous/adjacent, the blocks can be coalesced.

If we keep the freelist in sorted order, coalescing two blocks is simple. You add the space of the second block and its metadata to the space in the first block. In addition, you need to unlink the second block from the freelist since it has been absorbed by the first block.

# Logistics

We recommend that you first implement dmalloc with splitting. Test it. Then implement dfree by inserting freed blocks into a sorted freelist. Test it, and be sure you can recycle heap blocks through the free list. Then add support for coalescing to reduce fragmentation. Make sure you pass all local tests first.

    make test 


