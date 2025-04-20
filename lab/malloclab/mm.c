/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 * 
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "zhumi&miaomi",
    /* First member's full name */
    "Zhumimi",
    /* First member's email address */
    "zhumizhijia",
    /* Second member's full name (leave blank if none) */
    "Miaomimi",
    /* Second member's email address (leave blank if none) */
    "zhumizhijia"
};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)


#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

/* $begin mallocmacros */
/* Basic constants and macros */
#define WSIZE       4       /* Word and header/footer size (bytes) */ //line:vm:mm:beginconst
#define DSIZE       8       /* Double word size (bytes) */
#define CHUNKSIZE  (1<<12)  /* Extend heap by this amount (bytes) */  //line:vm:mm:endconst 

#define MAX(x, y) ((x) > (y)? (x) : (y))  

/* Pack a size and allocated bit into a word */
#define PACK(size, alloc)  ((size) | (alloc)) //line:vm:mm:pack

/* Read and write a word at address p */
#define GET(p)       (*(unsigned int *)(p))            //line:vm:mm:get
#define PUT(p, val)  (*(unsigned int *)(p) = (val))    //line:vm:mm:put

#define GET_PRE(bp)     ((unsigned int *)GET((bp)))
#define GET_SUCC(bp)    ((unsigned int *)(GET((unsigned int *)bp + 1)))
#define GET_HEAD(index) ((unsigned int *)(GET(heap_listp + index * WSIZE)))

/* Read the size and allocated fields from address p */
#define GET_SIZE(p)  (GET(p) & ~0x7)                   //line:vm:mm:getsize
#define GET_ALLOC(p) (GET(p) & 0x1)                    //line:vm:mm:getalloc

/* Given block ptr bp, compute address of its header and footer */
#define HDRP(bp)       ((char *)(bp) - WSIZE)                      //line:vm:mm:hdrp
#define FTRP(bp)       ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE) //line:vm:mm:ftrp

/* Given block ptr bp, compute address of next and previous blocks */
#define NEXT_BLKP(bp)  ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE))) //line:vm:mm:nextblkp
#define PREV_BLKP(bp)  ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE))) //line:vm:mm:prevblkp
/* $end mallocmacros */

#define SIZE_CLASS_NUM (20)
// {16}, {17 ~ 32}, {33 ~ 64}, ... , {1025 ~ 2048}, ...,  {2^22 + 1 ~ infinity}

static char* prologue_ptr = 0;  /* pointer to Prologue block */

static char *heap_listp = 0;  /* Pointer to first block */ 

int debug = 0;


/* Function prototypes for internal helper routines */
static void *extend_heap(size_t words);
static void place(void *bp, size_t asize);
static void *find_fit(size_t asize);
static int get_index(size_t asize);
static void insert(void* bp);
static void delete(void *bp);
static void *coalesce(void *bp);
static void printblock(void *bp); 
static void checkheap(int verbose);
static void checkblock(void *bp);

/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
#ifdef DEBUG_INFO
    debug = 0;
#endif
    /* $begin mminit */

    /* Create the initial empty free list array */
    // free_list_arrayp = (char**)calloc(SIZE_CLASS_NUM, sizeof(char*));
    
    /* Create the initial empty heap */
    if ((heap_listp = mem_sbrk((4 + SIZE_CLASS_NUM) * WSIZE)) == (void *)-1) //line:vm:mm:begininit
        return -1;

    for (int i = 0; i < SIZE_CLASS_NUM; ++i) {
        PUT(heap_listp + i * WSIZE, 0);
    }
    PUT(heap_listp + SIZE_CLASS_NUM * WSIZE, 0); /* Alignment padding */
    PUT(heap_listp + (SIZE_CLASS_NUM + 1) * WSIZE, PACK(DSIZE, 1));  /* Prologue header */
    PUT(heap_listp + (SIZE_CLASS_NUM + 2) * WSIZE, PACK(DSIZE, 1));  /* Prologue footer */ 
    PUT(heap_listp + (SIZE_CLASS_NUM + 3) * WSIZE, PACK(0, 1));  /* Epilogue header */

    prologue_ptr = heap_listp + (SIZE_CLASS_NUM + 2) * WSIZE;   /* prologue_ptr pos */

    /* $end mminit */

    /* Extend the empty heap with a free block of CHUNKSIZE bytes */
    if (extend_heap(CHUNKSIZE/WSIZE) == NULL) 
        return -1;
    checkheap(debug);
    return 0;
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    if (debug) {
        printf("call malloc..., size = %d\n", size);
        for (int i = 0; i < SIZE_CLASS_NUM; ++i) {
            printf("size class %d - %d free list ptr: %x\n", i, (1 << (i + 4)), GET(heap_listp + i * WSIZE));
        }
    }

    // printf("free list status:\n");
    // for (int i = 0; i < SIZE_CLASS_NUM; ++i) {
    //     printf("%i(%d)\t", i, 1 << (i + 4));
    // }
    // printf("\n");
    // for (int i = 0; i < SIZE_CLASS_NUM; ++i) {
    //     printf("%x\t", GET(heap_listp + i * WSIZE));
    // }
    // printf("\n");

    size_t asize;      /* Adjusted block size */
    size_t extendsize; /* Amount to extend heap if no fit */
    char *bp;      

    /* $end mmmalloc */
    if (heap_listp == 0){
        mm_init();
    }
    /* $begin mmmalloc */
    /* Ignore spurious requests */
    if (size == 0)
        return NULL;

    /* Adjust block size to include overhead and alignment reqs. */
    if (size <= DSIZE)                                          //line:vm:mm:sizeadjust1
        asize = 2*DSIZE;                                        //line:vm:mm:sizeadjust2
    else
        asize = DSIZE * ((size + (DSIZE) + (DSIZE-1)) / DSIZE); //line:vm:mm:sizeadjust3

    /* Search the free list for a fit */
    if ((bp = find_fit(asize)) != NULL) {  //line:vm:mm:findfitcall
        place(bp, asize);                  //line:vm:mm:findfitplace
        return bp;
    }

    /* No fit found. Get more memory and place the block */
    extendsize = MAX(asize,CHUNKSIZE);                 //line:vm:mm:growheap1
    if ((bp = extend_heap(extendsize/WSIZE)) == NULL)  
        return NULL;                                  //line:vm:mm:growheap2
    place(bp, asize);                                 //line:vm:mm:growheap3
    if (debug) {
        checkheap(debug);
    }
    return bp;
}

/*
 * get_index - Get the index in free list array by asize
 */
static int get_index(size_t asize) {
    //  1 2 4 8 16 32      10001
    // 111000
    int i = 4;
    while (asize > (1 << i)) {
        // asize >>= 1;
        i++;
        if (i >= 22) break; // max block list
    }
    if (debug) {
        printf("get_index... asize = %d, index = %d\n", asize, i - 4);
    }
    return i - 4;
}

/*
 * insert - Insert the bp into free list
 */
static void insert(void* bp) {
    size_t bsize = GET_SIZE(HDRP(bp));
    int index = get_index(bsize);
    if (debug) {
        printf("insert bp: %x into free list index: %d, bsize = %d\n", bp, index, bsize);
    }

    // insert bp in free list
    unsigned int* head = GET_HEAD(index);
    if (debug) {
        printf("heap_listp = %x, head = %x\n", heap_listp, head);
    }
    if (head == 0) {
        if (debug) {
            printf("insert---free list %d is empty...\n", index);
        }
        // head = bp;  
        PUT(heap_listp + index * WSIZE, bp);
        // set pre as 0
        PUT(bp, 0);
        unsigned int* succ = GET_SUCC(bp);
        // succ = 0; 
        // set succ as 0
        PUT((unsigned int*)bp + 1, 0);
    } else {
        if (debug) {
            printf("insert---free list %d is not empty...\n", index);
        }
        // unsigned int* ptr = head;
        unsigned int* ptr = heap_listp + index * WSIZE;
        unsigned int* ori_pre = GET_PRE(head);
        // PUT(head, bp);
        PUT(heap_listp + index * WSIZE, bp);
        PUT(bp, 0);
        if (debug) {
            printf("bp: %x, *bp = %x\n", bp, GET(bp));
        }
        // unsigned succ = GET_SUCC(bp);
        // PUT(succ, ptr);
        PUT((unsigned int*)bp + 1, head);
        if (debug) {
            printf("bp: %x, *bp = %x\n", bp, GET(bp));
        }
        // ori_pre = bp;
        // PUT(ori_pre, bp);
        PUT(head, bp);
    }
}

static void delete(void *bp) {
    if (debug) {
        printf("delete bp: %x\n", bp);
    }
    size_t bsize = GET_SIZE(HDRP(bp));
    int index = get_index(bsize);
    unsigned int* pre = GET_PRE(bp);
    unsigned int* succ = GET_SUCC(bp);
    if (debug) {
        printf("*pre = %x, *succ = %x\n", GET(bp), GET_SUCC(bp));
    }

    if (GET_PRE(bp) == 0 && GET_SUCC(bp) == 0) {  // the free list in index has only one block
        if (debug) {
            printf("delete----the free list %d has only one block\n", index);
        }
        PUT(heap_listp + index * WSIZE, 0);
    } else if (GET_PRE(bp) == 0) {  // bp is the first node in free list
        if (debug) {
            printf("delete----bp is the first node in free list %d\n", index);
        }
        PUT(heap_listp + index * WSIZE, GET_SUCC(bp));
        PUT(GET_SUCC(bp), 0);
    } else if (GET_SUCC(bp) == 0) {  // bp is the last block in free list
        if (debug) {
            printf("delete----bp is the last node in free list %d\n", index);
        }
        PUT(GET_PRE(bp) + 1, 0);
    } else {
        if (debug) {
            printf("delete----common case\n");
        }
        PUT(GET_PRE(bp) + 1, GET_SUCC(bp));
        PUT(GET_SUCC(bp), GET_PRE(bp));
    }

}

/* $begin mmplace */
/* $begin mmplace-proto */
static void place(void *bp, size_t asize)
/* $end mmplace-proto */
{
    size_t csize = GET_SIZE(HDRP(bp));
    if (debug) {
        printf("place bp: %x, asize = %d, csize = %d\n", bp, asize, csize);   
    }

    delete(bp);
    if ((csize - asize) >= (2*DSIZE)) { 
        PUT(HDRP(bp), PACK(asize, 1));
        PUT(FTRP(bp), PACK(asize, 1));
        bp = NEXT_BLKP(bp);
        PUT(HDRP(bp), PACK(csize-asize, 0));
        PUT(FTRP(bp), PACK(csize-asize, 0));
        insert(bp);
    }
    else { 
        PUT(HDRP(bp), PACK(csize, 1));
        PUT(FTRP(bp), PACK(csize, 1));
    }
}
/* $end mmplace */

/* 
 * find_fit - Find a fit for a block with asize bytes 
 */
/* $begin mmfirstfit */
/* $begin mmfirstfit-proto */
static void *find_fit(size_t asize)
/* $end mmfirstfit-proto */
{
    /* $end mmfirstfit */

    /* $begin mmfirstfit */

    int index = get_index(asize);
    if (debug) {
        printf("find_fit... asize = %d, index = %d\n", asize, index);
    }
    while (index < SIZE_CLASS_NUM) {
        unsigned int* bp = GET_HEAD(index);
        while (bp) {
            if (GET_SIZE(HDRP(bp)) >= asize) {
                if (debug) {
                    printf("find_fit return index: %d for asize: %d\n", index, asize);
                }
                return bp;
            }
            bp = GET_SUCC(bp);
            if (debug) {
                printf("find_fix bp = %x\n", bp);
            }
        }
        index++;
    }
    if (debug) {
        printf("find_fit return null\n");
    }
    return NULL;
}
/* $end mmfirstfit */

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
    if (debug) {
        printf("call free... ptr = %x \n", ptr);
    }
    if (ptr == NULL) return;
    if (heap_listp == 0){
        mm_init();
    }
    size_t size = GET_SIZE(HDRP(ptr));
    PUT(HDRP(ptr), PACK(size, 0));
    PUT(FTRP(ptr), PACK(size, 0));
    coalesce(ptr);
    if (debug) {
        for (int i = 0; i < SIZE_CLASS_NUM; ++i) {
            printf("size class %d - %d free list ptr: %x\n", i, (1 << (i + 4)), GET(heap_listp + i * WSIZE));
        }
    }
    if (debug) {
        checkheap(debug);
    }
}

/*
 * coalesce - Boundary tag coalescing. Return ptr to coalesced block
 */
/* $begin mmfree */
static void *coalesce(void *bp) 
{
    if (debug) {
        printf("call coalesce... bp: %x\n", bp);
    }
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));

    if (prev_alloc && next_alloc) {            /* Case 1 */
        if (debug) {
            printf("insert case1  bp: %x\n", bp);
        }
        insert(bp);
        return bp;
    }

    else if (prev_alloc && !next_alloc) {      /* Case 2 */
        if (debug) {
            printf("insert case2\n");
        }
        delete(NEXT_BLKP(bp));
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size,0));
    }

    else if (!prev_alloc && next_alloc) {      /* Case 3 */
        if (debug) {
            printf("insert case3\n");
        }
        delete(PREV_BLKP(bp));
        if (debug) {
            printf("insert case3 debug...\n");
        }
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        PUT(FTRP(bp), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }

    else {  
        if (debug) {                                   /* Case 4 */
            printf("insert case4\n");
        }
        delete(PREV_BLKP(bp));
        delete(NEXT_BLKP(bp));
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) + 
            GET_SIZE(FTRP(NEXT_BLKP(bp)));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }
    if (debug) {
        printf("insert bp: %x\n", bp);
    }
    insert(bp);

    /* $end mmfree */
    /* $begin mmfree */
    return bp;
}
/* $end mmfree */

/* 
 * extend_heap - Extend heap with free block and return its block pointer
 */
/* $begin mmextendheap */
static void *extend_heap(size_t words) 
{
    char *bp;
    size_t size;

    /* Allocate an even number of words to maintain alignment */
    size = (words % 2) ? (words+1) * WSIZE : words * WSIZE; //line:vm:mm:beginextend
    if ((long)(bp = mem_sbrk(size)) == -1)  
        return NULL;                                        //line:vm:mm:endextend

    /* Initialize free block header/footer and the epilogue header */
    PUT(HDRP(bp), PACK(size, 0));         /* Free block header */   //line:vm:mm:freeblockhdr
    PUT(FTRP(bp), PACK(size, 0));         /* Free block footer */   //line:vm:mm:freeblockftr
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1)); /* New epilogue header */ //line:vm:mm:newepihdr

    if (debug) {
        printf("coalesce bp: %x, size: %d\n", bp, size);
    }

    /* Coalesce if the previous block was free */
    return coalesce(bp);                                          //line:vm:mm:returnblock
}
/* $end mmextendheap */

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
    size_t oldsize;
    void *newptr;

    /* If size == 0 then this is just free, and we return NULL. */
    if(size == 0) {
        mm_free(ptr);
        return 0;
    }

    /* If oldptr is NULL, then this is just malloc. */
    if(ptr == NULL) {
        return mm_malloc(size);
    }

    newptr = mm_malloc(size);

    /* If realloc() fails the original block is left untouched  */
    if(!newptr) {
        return 0;
    }

    /* Copy the old data. */
    oldsize = GET_SIZE(HDRP(ptr));
    if(size < oldsize) oldsize = size;
    memcpy(newptr, ptr, oldsize);

    /* Free the old block. */
    mm_free(ptr);

    return newptr;
}

static void printblock(void *bp) 
{
    size_t hsize, halloc, fsize, falloc;

    checkheap(0);
    hsize = GET_SIZE(HDRP(bp));
    halloc = GET_ALLOC(HDRP(bp));  
    fsize = GET_SIZE(FTRP(bp));
    falloc = GET_ALLOC(FTRP(bp));  

    if (hsize == 0) {
        printf("%p: EOL\n", bp);
        return;
    }

    printf("%p: header: [%ld:%c] footer: [%ld:%c]\n", bp, 
           hsize, (halloc ? 'a' : 'f'), 
           fsize, (falloc ? 'a' : 'f')); 
}

static void checkblock(void *bp) 
{
    if ((size_t)bp % 8)
        printf("Error: %p is not doubleword aligned\n", bp);
    if (GET(HDRP(bp)) != GET(FTRP(bp)))
        printf("Error: header does not match footer\n");
}

/* 
 * checkheap - Minimal check of the heap for consistency 
 */
void checkheap(int verbose) 
{
    char *bp = prologue_ptr;

    if (verbose)
        printf("Heap (%p):\n", prologue_ptr);

    if ((GET_SIZE(HDRP(prologue_ptr)) != DSIZE) || !GET_ALLOC(HDRP(prologue_ptr)))
        printf("Bad prologue header\n");
    checkblock(prologue_ptr);

    for (bp = prologue_ptr; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp)) {
        if (verbose) 
            printblock(bp);
        checkblock(bp);
    }

    if (verbose)
        printblock(bp);
    if ((GET_SIZE(HDRP(bp)) != 0) || !(GET_ALLOC(HDRP(bp))))
        printf("Bad epilogue header\n");
}

