#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "debug.h"
#include "sfmm.h"
#include <errno.h>

/**************************/
/****HELPER FUNCTIONS******/
/**************************/

static int get_size(sf_block* block){
    return block->header & ~0x7;
}

static int invalid_ptr(void* ptr){

    //Check that param pointer is valid

    //pointer is NULL
    if(ptr == NULL){
        return 1;
    }
    //Not 8 byte aligned
    if((((uintptr_t)ptr) & 0x7) != 0){
        return 1;
    }
    sf_block* freeBlock = (sf_block*)(ptr-8);
    size_t blockSize = get_size(freeBlock);
    
    //Block size less than 32 or not multiple of 8
    if((blockSize < 32) || (blockSize % 8 != 0)){
        return 1;
    }
    //Block before prologue or after epilogue
    if(ptr < (sf_mem_start()+32) || (ptr + blockSize) > (sf_mem_end()-8)){
        return 1;
    }
    
    if(!(freeBlock->header & THIS_BLOCK_ALLOCATED)){ //allocated bit 0
        return 1;
    }
    if(freeBlock->header & IN_QUICK_LIST){ //in quick list bit set
        return 1;
    }
    if(!(freeBlock->header & PREV_BLOCK_ALLOCATED)){ //prev block supposedly free

    sf_footer *prevFooter = (sf_footer*)((void*)freeBlock-8); //get footer of previous block;
        if(*prevFooter & THIS_BLOCK_ALLOCATED){ //prev block not actually free
            return 1;
        }
    }
    return 0;
}

static void remove_free(sf_block* block){
    //This function is only to remove from free list
    //no header manipulation
    if(block->body.links.next!=NULL){
    (block->body.links.next)->body.links.prev= block->body.links.prev;}
     if(block->body.links.prev!=NULL){
    (block->body.links.prev)->body.links.next= block->body.links.next;}

}

static void insert_free(sf_block* block){
    int size = block->header >> 3; //calculate size of block
    size = size << 3;
    int reqBlockSize = 32;
    int freeIndex = 0;
        while (reqBlockSize < size) { //calculate index of smallest list to check
                reqBlockSize <<= 1; 
                freeIndex++;
        }
        if (freeIndex > NUM_FREE_LISTS - 1) { //make sure free list index does not exceed max lists
            freeIndex = NUM_FREE_LISTS - 1;
        }
    if(sf_free_list_heads[freeIndex].body.links.next == &(sf_free_list_heads[freeIndex])){
        block->body.links.next=&(sf_free_list_heads[freeIndex]);
        block->body.links.prev=&(sf_free_list_heads[freeIndex]);
        sf_free_list_heads[freeIndex].body.links.next = block;
        sf_free_list_heads[freeIndex].body.links.prev = block;
    }
    else{
    block->body.links.prev = &(sf_free_list_heads[freeIndex]); //rearrange links to insert block
    block->body.links.next = sf_free_list_heads[freeIndex].body.links.next;
    sf_free_list_heads[freeIndex].body.links.next->body.links.prev = block; 
    sf_free_list_heads[freeIndex].body.links.next = block;}

}

static sf_block* coalescePrev(sf_block* block){
    if(!(block->header & PREV_BLOCK_ALLOCATED)){
    sf_footer *prevFooter = (sf_footer*)((void*)block-8); //get footer of previous block
    size_t prevSize = *prevFooter >> 3;
    prevSize = prevSize << 3;
    
    sf_block* prevBlock = (sf_block*)((void*)block - prevSize); //get address of previous block
    remove_free(prevBlock); //remove from free list, check remove method behavior with block not in free list
    size_t totalSize = get_size(block) + prevSize; //get total size
    prevBlock->header = totalSize | PREV_BLOCK_ALLOCATED; //create new header
    sf_footer* blockFooter = (sf_footer*)((void*)prevBlock + totalSize - 8);
    *blockFooter = prevBlock->header;
    insert_free(prevBlock); //insert into free list
    return prevBlock; }

    return block;
}

static sf_block* coalesceNext(sf_block* block){
    int previousStatus = 0;
    if(block->header & PREV_BLOCK_ALLOCATED){
        previousStatus = 1;
    }
    sf_block* nextPtr = (sf_block*)((void*)block + get_size(block));
    if(!(nextPtr->header & THIS_BLOCK_ALLOCATED)){
    size_t nextSize = get_size(nextPtr);
    remove_free(nextPtr); //remove from free list, check remove method behavior with block not in free list
    size_t totalSize = get_size(block) + nextSize; //get total size
    block->header = totalSize; //create new header
    if(previousStatus){block->header |= PREV_BLOCK_ALLOCATED;}
    sf_footer* blockFooter = (sf_footer*)((void*)block + totalSize - 8);
    *blockFooter = block->header;
    insert_free(block); //insert into free list
    return block; }
    
    return block;
}

static sf_block* coalesce_both(sf_block* block){
    //first coalesce with previous block if possible
    int coalescingDone = 0;
     if(!(block->header & PREV_BLOCK_ALLOCATED)){
        coalescingDone++;
         sf_footer *prevFooter = (sf_footer*)((void*)block-8); //get footer of previous block
        size_t prevSize = *prevFooter >> 3;
        prevSize = prevSize << 3;
        sf_block* prevBlock = (sf_block*)((void*)block - prevSize); //get address of previous block
        int previousStatus = 0;
         if(prevBlock->header & PREV_BLOCK_ALLOCATED){
        previousStatus = 1;
        }
        remove_free(prevBlock); //remove from free list, check remove method behavior with block not in free list
        size_t totalSize = get_size(block) + prevSize; //get total size
        prevBlock->header = totalSize;
        if(previousStatus){prevBlock->header |= PREV_BLOCK_ALLOCATED;} //create new header
        sf_footer* blockFooter = (sf_footer*)((void*)prevBlock + totalSize - 8);
        *blockFooter = prevBlock->header;
        block = prevBlock;
    }
    int previousStatus = 0;
    if(block->header & PREV_BLOCK_ALLOCATED){
        previousStatus = 1;
    }
    sf_block* nextPtr = (sf_block*)((void*)block + get_size(block));
    if(!(nextPtr->header & THIS_BLOCK_ALLOCATED)){
        coalescingDone++;

    size_t nextSize = get_size(nextPtr);
    remove_free(nextPtr); //remove from free list, check remove method behavior with block not in free list
    size_t totalSize = get_size(block) + nextSize; //get total size
    block->header = totalSize; //create new header
    if(previousStatus){block->header |= PREV_BLOCK_ALLOCATED;}
    sf_footer* blockFooter = (sf_footer*)((void*)block + totalSize - 8);
    *blockFooter = block->header;
    }
    if(!coalescingDone){
        sf_footer* blockFtr = (void*)(block)+get_size(block)-8;
        *blockFtr = block->header;
    }
    insert_free(block);
    return block;
}

static void split_block(sf_block* block, int neededSize, int leftoverSize){
    //assume leftover is >= 32
    int previousStatus = 0;
    if(block->header & PREV_BLOCK_ALLOCATED){
        previousStatus = 1;
    }
    remove_free(block); //remove lower from free list
    sf_block* upper = (sf_block*)((void*)block + neededSize); //calculate where to split block
    leftoverSize = get_size(block) - neededSize;
    upper->header = leftoverSize; //set header of upper block
    upper->header |= PREV_BLOCK_ALLOCATED; //since lower will be allocated soon
    sf_footer* upperFooter = (sf_footer*)((void*)(upper)+(get_size(upper))-8);
    *upperFooter = upper->header;
    block->header = neededSize; //set header of lower block
    block->header |= THIS_BLOCK_ALLOCATED; //lower is the block to be allocated
    if(previousStatus){
        block->header |= PREV_BLOCK_ALLOCATED;
    }
    insert_free(upper); //insert upper into a free list
}

static sf_block* find_in_free(int index, int reqSize){
    size_t current_index = index; 
    while(current_index < NUM_FREE_LISTS){
        if(sf_free_list_heads[current_index].body.links.next == &sf_free_list_heads[current_index]){ //check if sentinel points to itself, TEST
            current_index++;
        }
        else{
            sf_block* blockCursor = sf_free_list_heads[current_index].body.links.next; //initialize cursor as list head
            while (blockCursor != &(sf_free_list_heads[current_index])) { //run until we reach the sentinel node again
                    size_t headerSize = blockCursor->header >> 3;
                    headerSize = headerSize << 3;
                    if(headerSize >= reqSize){
                        return blockCursor;
                    }
                    else{
                        blockCursor = blockCursor->body.links.next;
                    }
            }
            current_index++;
        }
    }
    return NULL;
}

static void split_alloc(sf_block* block, int neededSize){
    //split an allocated block into an allocated and free part
    //assume leftover is >= 32
    int previousStatus = 0;
    if(block->header & PREV_BLOCK_ALLOCATED){
        previousStatus = 1;
    }
    sf_block* upper = (sf_block*)((void*)block + neededSize); //calculate where to split block
    int leftoverSize = get_size(block) - neededSize;
    upper->header = leftoverSize; //set header of upper block
    upper->header |= PREV_BLOCK_ALLOCATED; //since lower is allocated
    sf_footer* upperFooter = (sf_footer*)((void*)(upper)+(get_size(upper))-8);
    *upperFooter = upper->header;
    block->header = neededSize; //set header of lower block
    block->header |= THIS_BLOCK_ALLOCATED; //lower is the block to be allocated
    if(previousStatus){
        block->header |= PREV_BLOCK_ALLOCATED;
    }
    coalesceNext(upper); //insert upper into a free list
}

/**************************/
/****REQUIRED FUNCTIONS****/
/**************************/

void *sf_malloc(size_t size) {
    if (size == 0) {
        return NULL;
    }
    /*****FIRST CALL INITIALIZATIONS****/
    if(sf_mem_start()==sf_mem_end()){   
        void* initAlloc = sf_mem_grow();
        if (initAlloc == NULL) { // Check if page allocation is successful
            sf_errno = ENOMEM;
            return NULL;
        }
        struct sf_block *prologue = (sf_block*)(sf_mem_start()); 
        prologue->header = 32; //minimum block size
        prologue->header |= THIS_BLOCK_ALLOCATED; //set header of prologue to allocated

        struct sf_block *initBlock = (sf_block*)(sf_mem_start() + 32); //big free block 
        initBlock->header = (PAGE_SZ-40);   //header
        initBlock->header |= PREV_BLOCK_ALLOCATED; //set first mem grow block to prev allocated because of prologue
        
        sf_footer *initBlockFooter = (sf_footer*)((void*)initBlock + get_size(initBlock) - 8); //footer
        *initBlockFooter = initBlock->header;        

        struct sf_block *epilogue = (sf_block*)(sf_mem_end()-8); //calculate address epilogue should be at
        epilogue->header = THIS_BLOCK_ALLOCATED; //set epilogue to a block with just a header 

        for(int i = 0; i < NUM_FREE_LISTS; i++){    //initialize linking behavior of free list sentinels
            sf_free_list_heads[i].body.links.next = (&sf_free_list_heads[i]);
            sf_free_list_heads[i].body.links.prev = (&sf_free_list_heads[i]);
        }
        insert_free(initBlock);
        }

        /*****ROUNDING****/
        int roundedSize = 0;
        if (size <= 24) {
            roundedSize = 32; 
        }
        else{
            roundedSize = size + 8;  //size + header;
            int rem = roundedSize % 8;
            if(rem != 0){
                roundedSize += 8 - rem; //round up to nearest multiple of 8
            } 
            }
    
        /*****QUICKLIST SEARCHING****/
        int q_index = (roundedSize-32)/8;
        if(q_index < NUM_QUICK_LISTS){ //check if a quick list with this size exists
        if (sf_quick_lists[q_index].length > 0){ //check that there are blocks in the quick list
                if (sf_quick_lists[q_index].length == 1){ //the case that there is only one block in the quick list
                    sf_block* qk_returnPtr = sf_quick_lists[q_index].first; //obtain pointer to first block
                    sf_quick_lists[q_index].first->header &= ~IN_QUICK_LIST; //set qklst bit to 0
                    sf_quick_lists[q_index].first = NULL; //remove first block reference
                    sf_quick_lists[q_index].length--; //decrement length field.
                    return qk_returnPtr->body.payload;
                }
                else{ //more than 1 block in the quick list

                    sf_block* ret = sf_quick_lists[q_index].first;
                    sf_quick_lists[q_index].first = ret->body.links.next;
                    ret->body.links.next=NULL;
                    ret->header &= ~IN_QUICK_LIST;
                    sf_quick_lists[q_index].length--;
                    return ret->body.payload;
                }
        } 
        }
        /*****FREE LIST SEARCHING*****/
        int reqBlockSize = 32;
        int freeIndex = 0;
        while (reqBlockSize < roundedSize) { //calculate index of smallest list to check
                reqBlockSize <<= 1; 
                freeIndex++;
        }
        if (freeIndex > NUM_FREE_LISTS - 1) { //make sure free list index does not exceed max lists
            freeIndex = NUM_FREE_LISTS - 1;
        }
        sf_block* foundFree = find_in_free(freeIndex,roundedSize); //find a free list block that satisfies request
        if(foundFree != NULL){ //if a suitable block was found
            size_t leftover = roundedSize - get_size(foundFree); //calculate leftover size after fulfilling request
            if(leftover >= 32){ //if leftover size is more than 32, split can be done
                split_block(foundFree,roundedSize,leftover); //split blocks
                return foundFree->body.payload; //return block
            }
            else{ //no splitting required
            foundFree->header |= THIS_BLOCK_ALLOCATED; //set alloc status
            sf_block *nextFoundFree = (sf_block*)((void*)foundFree + get_size(foundFree));
            nextFoundFree->header |= PREV_BLOCK_ALLOCATED; //set prev alloc status of next block
            remove_free(foundFree);
            return foundFree->body.payload;
            }
        }
        /****NEED MORE MEMORY****/
        else{
            int enoughMem = 0;
            while(!enoughMem){
            
                sf_header *epilogueRef = (sf_header*)(sf_mem_end() - 8); //get reference to epilogue
                int coalesceNeeded = 1; 
                if(*epilogueRef & PREV_BLOCK_ALLOCATED){ //if previous block allocated, no coalesce needed
                    coalesceNeeded = 0;
                }
                void* extraMem = sf_mem_grow(); //get another page
                    if (extraMem == NULL) { // Check if page allocation is successful
                    sf_errno = ENOMEM;
                    return NULL;
                    }
                
                sf_block *memBlock = (sf_block*)(epilogueRef); //set new memory to a block, using epilogue as old header
                memBlock->header=PAGE_SZ; //page sz - 8 + 8 (grabbed old epilogue)
                if(coalesceNeeded){
                    memBlock = coalescePrev(memBlock);
                }
                else {
                    insert_free(memBlock); //insert block into appropriate free list
                }
                
                int newSize = get_size(memBlock); //get size after coalescing (may not have happened)
                sf_footer* memBlockFooter = (sf_footer*)((void*)memBlock + newSize - 8);
                *memBlockFooter = memBlock->header;

                //make new epilogue
                struct sf_block *epilogue = (sf_block*)(sf_mem_end()-8); //calculate address epilogue should be at
                epilogue->header = THIS_BLOCK_ALLOCATED; //set epilogue to a block with just a header 


                if(newSize >= roundedSize){ //valid block
                    if(newSize - roundedSize >= 32){ //split needed
                        split_block(memBlock,roundedSize,newSize-roundedSize); //split block if possible, removing and inserting with free lists is handled in method
                    }
                    else{
                        epilogue->header |= PREV_BLOCK_ALLOCATED; //if split did not happen, no "upper" to be free
                        remove_free(memBlock);
                    }
                    memBlock->header |= THIS_BLOCK_ALLOCATED;
                    enoughMem = 1;
                    return memBlock->body.payload;
                    }
            }
            } 
    return NULL;
}


void sf_free(void *pp) {
    
    if(invalid_ptr(pp)){
        abort();
    }

    sf_block* freeBlock = (sf_block*)(pp-8);
    size_t blockSize = get_size(freeBlock);

    //Check if block can be inserted into qklst

    int q_index = ((blockSize-32)/8);
    if(q_index < NUM_QUICK_LISTS){

        if(sf_quick_lists[q_index].length < QUICK_LIST_MAX){
        
            if(sf_quick_lists[q_index].length == 0){
                sf_quick_lists[q_index].first = freeBlock;
                freeBlock->header |= IN_QUICK_LIST;
                sf_quick_lists[q_index].length++;
                freeBlock->body.links.next = NULL;
                return;
            }
            else{
                sf_block* qkPtr = sf_quick_lists[q_index].first;
                freeBlock->body.links.next = qkPtr;
                sf_quick_lists[q_index].first = freeBlock;
                sf_quick_lists[q_index].length++;
                
                }
        }
        else{
        //flush quicklist
            sf_block* qkPtr = sf_quick_lists[q_index].first;
            for(int i = 0; i < sf_quick_lists[q_index].length; i++){
                qkPtr->header &= ~THIS_BLOCK_ALLOCATED;
                qkPtr->header &= ~IN_QUICK_LIST;
                sf_block* nextPtr = qkPtr->body.links.next;
                sf_block* nextRef = (sf_block*)((void*)qkPtr+get_size(qkPtr));
                nextRef->header &= ~PREV_BLOCK_ALLOCATED;
                qkPtr = coalesce_both(qkPtr);
                qkPtr = nextPtr;
            }
            sf_quick_lists[q_index].first = freeBlock;
            freeBlock->body.links.next= NULL;
            sf_quick_lists[q_index].length = 1;
            freeBlock->header |= IN_QUICK_LIST;
            return;
        }

    }
    else{
        //try to coalesce and add to free list
        sf_block* nextRef = (sf_block*)((void*)freeBlock+get_size(freeBlock));
        int noCoalescing = 1;
        if(!(freeBlock->header & PREV_BLOCK_ALLOCATED)){
            noCoalescing = 1;
            sf_footer *prevFooter = (sf_footer*)((void*)freeBlock-8); //get footer of previous block
            size_t prevSize = *prevFooter >> 3;
            prevSize = prevSize << 3;
            sf_block* prevBlock = (sf_block*)((void*)freeBlock - prevSize); //get address of previous block
            remove_free(prevBlock); //remove from free list, check remove method behavior with block not in free list
            size_t totalSize = get_size(freeBlock) + prevSize; //get total size
            prevBlock->header = totalSize | PREV_BLOCK_ALLOCATED; //create new header
            sf_footer* blockFooter = (sf_footer*)((void*)prevBlock + totalSize - 8);
            *blockFooter = prevBlock->header;
            freeBlock = prevBlock;
        }
        if(!(nextRef->header & THIS_BLOCK_ALLOCATED)){
            freeBlock = coalesceNext(freeBlock);
            noCoalescing = 0;
        }

        freeBlock->header &= ~THIS_BLOCK_ALLOCATED;
        if(noCoalescing){
        insert_free(freeBlock);
        sf_footer* freeFooter = (sf_footer*)((void*)freeBlock+get_size(freeBlock)-8);
        *freeFooter = freeBlock->header;}
        nextRef->header &= ~PREV_BLOCK_ALLOCATED;
        return;
    }
}


void *sf_realloc(void *pp, size_t rsize) {
    if(invalid_ptr(pp)){
        sf_errno = EINVAL;
        abort();
    }
    
    if(rsize == 0){
        sf_free(pp);
        return NULL;
    }

    int roundedSize = 0;
        if (rsize <= 24) {
            roundedSize = 32; 
        }
        else{
            roundedSize = rsize + 8;  //size + header;
            int rem = roundedSize % 8;
            if(rem != 0){
                roundedSize += 8 - rem; //round up to nearest multiple of 8
            } 
            }

    sf_block* origBlock = (sf_block*)(pp-8);
    if(roundedSize > get_size(origBlock)){
        void* newMem = sf_malloc(rsize);
        if(newMem == NULL){
            sf_errno=ENOMEM;
            return NULL;
        }
        newMem = memcpy(newMem, (void*)pp, (get_size(origBlock)-8));
        sf_free(pp);
        return newMem;
    }
    else if(roundedSize < get_size(origBlock)){
            size_t leftover = get_size(origBlock) - roundedSize; //calculate leftover size 
            if(leftover >= 32){ //if leftover size is more than 32, split can be done
            
            int previousStatus = 0;
            if(origBlock->header & PREV_BLOCK_ALLOCATED){
                previousStatus = 1;
            }
            sf_block* upper = (sf_block*)((void*)origBlock + roundedSize); //calculate where to split block
            upper->header = leftover; //set header of upper block
            upper->header |= PREV_BLOCK_ALLOCATED; 
            upper->header &= ~THIS_BLOCK_ALLOCATED;
            sf_footer* upperFooter = (sf_footer*)((void*)(upper)+(get_size(upper))-8);
            *upperFooter = upper->header;
            origBlock->header = roundedSize; //set header of lower block
            origBlock->header |= THIS_BLOCK_ALLOCATED; //lower is the block to be allocated
            if(previousStatus){
                origBlock->header |= PREV_BLOCK_ALLOCATED;
            }
            coalesce_both(upper);
            return origBlock->body.payload;
            }
            else{
                return origBlock->body.payload;
            }
  }
  return pp;
}
void *sf_memalign(size_t size, size_t align) {
    
    if(align < 8){
        sf_errno = EINVAL;
        return NULL;
    }
    if(((align & (align - 1)) != 0)){
        sf_errno = EINVAL;
        return NULL;
    }
     int roundedSize = 0;
        if (size <= 24) {
            roundedSize = 32; 
        }
        else{
            roundedSize = size + 8;  //size + header;
            int rem = roundedSize % 8;
            if(rem != 0){
                roundedSize += 8 - rem; //round up to nearest multiple of 8
            } 
            }
    size_t totalSize = size + align + 32;
    void* payL = sf_malloc(totalSize);
    sf_block* block = (sf_block*)((void*)(payL)-8); 
    if ((uintptr_t)payL % align == 0){ //starting address of payload is aligned
        if(get_size(block)-roundedSize){
        split_alloc(block,roundedSize);}
        return block->body.payload;
    }
    else{
        sf_block* middle = block;
        int keepLooking = 1;
        int i = 32;
        do{
            if(((uintptr_t)(payL + i)) % align == 0){
                int sizeBefore = get_size(block);
                int setPrev = 0;
                if(block->header & PREV_BLOCK_ALLOCATED){
                    setPrev = 1;
                }
                //Lower portion
                block->header= i;
                if(setPrev){block->header |= PREV_BLOCK_ALLOCATED;}
                sf_footer* lowerFooter = (sf_footer*)((void*)block + get_size(block) -8); //i + 8 -8
                *lowerFooter = block->header;
                sf_block* oldBlock = block;
                block = coalescePrev(block);
                if(oldBlock == block){
                    insert_free(block);
                }
                //Middle portion (that starts at aligned address)
                middle = (sf_block*)(payL + i - 8);
                middle->header = sizeBefore - get_size(block);
                middle->header |= THIS_BLOCK_ALLOCATED;
                int leftover = get_size(middle) - roundedSize;
                if(leftover >= 32){
                    split_alloc(middle,roundedSize);
                }
                keepLooking = 0;
                break;
            }
           else{
            if(i == get_size(block)){
                break;
            }
            i++;}
        }while(keepLooking);
    return middle->body.payload;
    }
}
