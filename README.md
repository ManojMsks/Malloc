malloc has implemented and in buddy malloc implementation is good but have to make its removal o(1) by using doubly linked list  
Changing it to a Doubly-Linked List. Add struct block_meta *prev; to your struct. When you want to remove a block, you don't need to search for it anymore. You just do block->prev->next = block->next;. This makes removals instant (O(1) time).
