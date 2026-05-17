#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/mman.h>
#include <stdint.h>
#include <stdio.h>


#define MIN_ORDER 5   
#define MAX_ORDER 20

void *pool_start = NULL;

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
    pool_start = pool;
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
        void *pool= mmap(NULL, 1<<target_order , PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if(pool==MAP_FAILED){
        return NULL;
    }
    struct block_meta* block=(struct block_meta*)pool;
    block->order=target_order;
    block->free=false;
    block->next=NULL;
    return (void *)(block + 1);   
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


void remove_block(int order, struct block_meta *block) {
    if (free_lists[order] == block) {
        free_lists[order] = block->next;
        return;
    }
    struct block_meta *curr = free_lists[order];
    while (curr && curr->next != block) {
        curr = curr->next;
    }
    if (curr) {
        curr->next = block->next;
    }
}

void my_free(void* ptr){
    if(ptr==NULL) return ;

    struct block_meta *block=(struct block_meta *)ptr-1;
    if(block->order>MAX_ORDER){
        munmap((void*)block,(1<<block->order));
        return ;
    }

    int order=block->order;
    while(order<MAX_ORDER){
        uintptr_t block_offset = (char *)block - (char *)pool_start;
        uintptr_t buddy_offset = block_offset ^ (1 << order);
        struct block_meta *buddy = (struct block_meta *)((char *)pool_start + buddy_offset);
        if(buddy->free&& buddy->order==order){
            order++;
            if(buddy<block){
                block=buddy;
            }
            remove_block(order-1,buddy);
        }
        else{
            break;
        }
    }
    block->order=order;
    block->free=true;
    block->next=free_lists[order];
    free_lists[order]=block;
}

void* my_realloc(void *ptr, size_t size){
    if(ptr==NULL) return my_malloc(size);
    if(size==0){
        my_free(ptr);
        return NULL;
    }
    struct block_meta *block=(struct block_meta *)ptr-1;
    size_t sz=size+sizeof(struct block_meta);
    int current_order=block->order;
    int target_order=5;

    while(sz>(1<<target_order)){
        target_order++;
    }
    if(target_order==current_order){
        return ptr;
    }
    void* new_ptr=my_malloc(size);
    if(new_ptr==NULL){
        return NULL;
    }   


    size_t old_usable_size = (1 << current_order) - sizeof(struct block_meta);
    size_t copy_size = (old_usable_size < size) ? old_usable_size : size;

    memcpy(new_ptr,ptr,copy_size);
    my_free(ptr);
    return new_ptr;
}





int main() {
    void *pool = init_allocator();
    assert(pool != NULL);
    printf("Allocator initialized successfully.\n");

    char *ptr1 = (char *)my_malloc(10);
    assert(ptr1 != NULL);
    strcpy(ptr1, "OS_Shell");
    printf("Small allocation successful: %s\n", ptr1);

    void *ptr2 = my_malloc(500);
    assert(ptr2 != NULL);
    printf("Medium allocation (requires splitting) successful.\n");

    void *ptr3 = my_malloc(2 * 1024 * 1024);
    assert(ptr3 != NULL);
    printf("Oversized allocation (bypassing pool via direct mmap) successful.\n");

    void *ptr4 = my_realloc(ptr1, 12);
    assert(ptr4 == ptr1);
    printf("Realloc shortcut (same power-of-two order) successful.\n");

    char *ptr5 = (char *)my_realloc(ptr1, 200);
    assert(ptr5 != ptr1);
    assert(strcmp(ptr5, "OS_Shell") == 0);
    printf("Realloc data copy and move successful: %s\n", ptr5);

    my_free(ptr2);
    my_free(ptr3);
    my_free(ptr5);
    printf("All allocations freed and coalesced successfully.\n");

    printf("\nCongratulations! All tests passed flawlessly.\n");
    return 0;
}