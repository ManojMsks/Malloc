#include <assert.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>


void *malloc(size_t size) {
  void *p = sbrk(0);
  void *request = sbrk(size);
  if (request == (void*) -1) {
    return NULL; // sbrk failed.
  } else {
    assert(p == request); // Not thread safe.
    return p;
  }
}


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
