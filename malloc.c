#include <assert.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>




struct block_meta {
  size_t size;
  struct block_meta *next;
  int free;
  int magic;    // For debugging only. TODO: remove this in non-debug mode.
};


#define META_SIZE sizeof(struct block_meta)

void *global_base = NULL;

// Iterate through blocks until we find one that's large enough.
// TODO: split block up if it's larger than necessary
struct block_meta *find_free_block(struct block_meta **last, size_t size) {
  struct block_meta *current = global_base;
  while (current && !(current->free && current->size >= size)) {
    *last = current;
    current = current->next;
  }
  return current;
}


struct block_meta *request_space(struct block_meta* last, size_t size) {
  struct block_meta *block;
  block=sbrk(0);
  void *pointer=sbrk(size+META_SIZE);
  if(pointer==(void*) -1){
    return NULL;
  }
  assert(block==pointer);
  if(last){
    last->next=block;
  }
  block->size=size;
  block->free=0;
  block->next=NULL;
  block->magic=0x12345678;
  return block;
}

void * malloc(size_t size){
  struct block_meta *block;
  if(size<=0){
    return NULL;
  }
  if(!global_base){
    block=request_space(NULL,size);
    if(block){
      global_base=block;
    }
    else {
      return NULL;
    }
  }
  else{
    struct block_meta *last;
    last=global_base;
    block=find_free_block(&last,size);
    if(block){
      block->free=0;
    }else {
      block=request_space(last,size);
      if(!block){
        return NULL;
      }
    }
}
  return block+1;
}


struct block_meta *get_block_ptr(void *ptr) {
  return (struct block_meta*)ptr - 1;
}

void free(void *ptr) {
  if (!ptr) {
    return;
  }

  // TODO: consider merging blocks once splitting blocks is implemented.
  struct block_meta* block_ptr = get_block_ptr(ptr);
  assert(block_ptr->free == 0);
  assert(block_ptr->magic == 0x12345678);
  block_ptr->free = 1;
  block_ptr->magic = 0x55555555;
}

void *realloc(void *ptr,size_t size) {
  if(!ptr){
    return malloc(size);
  }
  struct block_meta *block=get_block_ptr(ptr);
  if(block->size>=size){
    return ptr;
  }
  void * newptr=malloc(size);
  if(!newptr){
    return NULL;   // TODO: set errno on failure.
  }
  memcpy(newptr,ptr,block->size);
  free(ptr);
  return newptr;  
}

void *calloc(size_t nelem, size_t elsize) {
  size_t size = nelem * elsize; // TODO: check for overflow.
  void *ptr = malloc(size);
  memset(ptr, 0, size);
  return ptr;
}

