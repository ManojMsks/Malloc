#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/mman.h>


#define MIN_ORDER 5   
#define MAX_ORDER 20

#define MIN_SIZE (1 << MIN_ORDER) 
#define MAX_SIZE (1 << MAX_ORDER)

struct block_meta {
    int order;
    bool free;
    struct block_meta *next;
};

struct block_meta *free_lists[MAX_ORDER + 1];

void* init_allocator(){
    void *pool = mmap(NULL, MAX_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if(pool==MAP_FAILED){
        return NULL;
    }
    struct block_meta* block=(struct block_meta*)pool;
    block->order=MAX_ORDER;
    block->free=true;
    block->next=NULL;
    free_lists[MAX_ORDER]=block;
    return block;
}


void* my_malloc(size_t size) {
    size_t sz = size + sizeof(struct block_meta);
    
    int target_order = MIN_ORDER;
    while ((1 << target_order) < sz) {
        target_order++;
    }
    
    if (target_order > MAX_ORDER) {
        return NULL;
    }
    
    int current_order = target_order;
    while (current_order <= MAX_ORDER && !free_lists[current_order]) {
        current_order++;
    }
    
    if (current_order > MAX_ORDER) {
        return NULL; 
    }
    
    struct block_meta *block = free_lists[current_order];
    free_lists[current_order] = block->next;
    
    while (current_order > target_order) {
        current_order--;
        
        struct block_meta *buddy = (struct block_meta *)((char *)block + (1 << current_order));
        
        buddy->order = current_order;
        buddy->free = true;
        buddy->next = free_lists[current_order];
        free_lists[current_order] = buddy;
    }
    
    block->order = target_order;
    block->free = false;
    block->next = NULL;
    
    return (void *)(block + 1);
}