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
    "w06-1",
    /* First member's full name */
    "Han Na",
    /* First member's email address */
    "nahaan9186@gmail.com",
    /* Second member's full name (leave blank if none) */
    "Sumin Kim",
    /* Second member's email address (leave blank if none) */
    "ksoomin25@gmail.com"
};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)

#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

/* 묵시적, 명시적 */
#define IMPLICIT
// #define EXPLICIT

/* 가용 블록 탐색 */
// #define FIRST_FIT
// #define NEXT_FIT
// #define BEST_FIT

/* 할당 */
#define LIFO
#define ADDRESS

static void *extend_heap(size_t words);
static void *coalesce(void *bp);
static void *find_fit(size_t asize);
static void place(void *bp, size_t asize);
static void remove_from_free(void *bp);
static void add_to_free(void *bp);

static char *heap_listp;
static char *free_listp;
static void *last_allocated = NULL;
static void *next_listp = NULL;


/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{   
    #ifdef IMPLICIT
        if ((heap_listp = mem_sbrk(4 * WSIZE)) == (void *)-1) return -1;

        PUT(heap_listp, 0);
        PUT(heap_listp + (1 * WSIZE), PACK(DSIZE, 1)); // prologue header
        PUT(heap_listp + (2 * WSIZE), PACK(DSIZE, 1)); // prologue footer
        PUT(heap_listp + (3 * WSIZE), PACK(0, 1)); // epilogue header

        if (extend_heap(CHUNKSIZE / WSIZE) == NULL) return -1;
        
        return 0;

    # elif defined(EXPLICIT)

        if ((free_listp = mem_sbrk(8 * WSIZE)) == (void *)-1) return -1;

        PUT(free_listp, 0); // padding            
        PUT(free_listp + (1 * WSIZE), PACK(2 * WSIZE, 1)); // prologue header
        PUT(free_listp + (2 * WSIZE), PACK(2 * WSIZE, 1)); // prologue footer
        PUT(free_listp + (3 * WSIZE), PACK(4 * WSIZE, 0)); // first free block header
        PUT(free_listp + (4 * WSIZE), NULL); // previous free block address
        PUT(free_listp + (5 * WSIZE), NULL); // next free block address
        PUT(free_listp + (6 * WSIZE), PACK(4 * WSIZE, 0)); // first free block footer
        PUT(free_listp + (7 * WSIZE), PACK(0, 1)); // epilogue header

        free_listp += (4 * WSIZE); // first free block bp

        if (extend_heap(CHUNKSIZE / WSIZE) == NULL)
            return -1;

        return 0;

    #else

    #endif
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    size_t asize;
    size_t extendsize;
    char *bp;

    if (size == 0) return NULL;

    if (size <= DSIZE) asize = 2 * DSIZE;
    else asize = DSIZE * ((size + (DSIZE) + (DSIZE - 1)) / DSIZE);

    if ((bp = find_fit(asize)) != NULL) {
        place(bp, asize);
        return bp;
    }

    extendsize = MAX(asize, CHUNKSIZE);
    // WORD 단위이므로 WORD 크기로 나누어 줌
    if ((bp = extend_heap(extendsize/WSIZE)) == NULL) return NULL;

    place(bp, asize);
    return bp;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
    size_t size = GET_SIZE(HDRP(ptr));

    PUT(HDRP(ptr), PACK(size, 0));
    PUT(FTRP(ptr), PACK(size, 0));
    coalesce(ptr);

}


/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
    if (ptr == NULL) {
        // 만약 ptr이 NULL이면, 그냥 메모리만 할당
        return mm_malloc(size);
    }
    
    if (size == 0) {
        // 만약 size가 0이면, ptr 해제
        mm_free(ptr);
        ptr = NULL;
        return NULL;
    }
    
    void *newptr;
    size_t old_size = GET_SIZE(HDRP(ptr)) - DSIZE;
    
    // 새로운 메모리 블록 할당
    newptr = mm_malloc(size);
    // 메모리 할당 실패 처리
    if (newptr == NULL)
        return NULL;

    // 기존 사이즈가 더 작으면 기존 사이즈로 복사할 사이즈 설정
    size_t copySize = old_size < size ? old_size : size;

    // 기존 데이터를 새로운 메모리로 복사
    memcpy(newptr, ptr, copySize);

    // 이전 메모리 블록 해제
    mm_free(ptr);
    ptr = NULL;

    return newptr;
}



static void *coalesce(void *bp)
{
    #ifdef LIFO
        size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
        size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
        size_t size = GET_SIZE(HDRP(bp));

        if (prev_alloc && next_alloc) {

            add_to_free(bp);
            return bp;

        } else if (prev_alloc && !next_alloc) {

            remove_from_free(NEXT_BLKP(bp));
            size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
            PUT(HDRP(bp), PACK(size, 0));
            PUT(FTRP(bp), PACK(size, 0));

        } else if (!prev_alloc && next_alloc) {

            remove_from_free(PREV_BLKP(bp));
            size += GET_SIZE(HDRP(PREV_BLKP(bp)));
            PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
            PUT(FTRP(bp), PACK(size, 0));
            bp = PREV_BLKP(bp);

        } else {

            remove_from_free(PREV_BLKP(bp));
            remove_from_free(NEXT_BLKP(bp));
            size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(FTRP(NEXT_BLKP(bp)));
            PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
            PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
            bp = PREV_BLKP(bp);

        }

        add_to_free(bp);
        return bp;
    
    #elif defined(ADDRESS)
        size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp))); 
        size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp))); 
        size_t size = GET_SIZE(HDRP(bp));                   

        if (prev_alloc && next_alloc) 
        {
            add_to_free(bp); 
            return bp;          
        }
        else if (prev_alloc && !next_alloc) 
        {
            remove_from_free(NEXT_BLKP(bp)); 
            size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
            PUT(HDRP(bp), PACK(size, 0)); 
            PUT(FTRP(bp), PACK(size, 0)); 
            add_to_free(bp); // 추가
        }
        else if (!prev_alloc && next_alloc) 
        {
            size += GET_SIZE(HDRP(PREV_BLKP(bp)));
            PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0)); 
            PUT(FTRP(bp), PACK(size, 0));            
            bp = PREV_BLKP(bp);                      
        }
        else 
        {
            remove_from_free(NEXT_BLKP(bp)); 
            size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(FTRP(NEXT_BLKP(bp)));
            PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0)); 
            PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0)); 
            bp = PREV_BLKP(bp);                      
        }
        return bp; 

    #else
        size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
        size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
        size_t size = GET_SIZE(HDRP(bp));

        if (prev_alloc && next_alloc) return bp; // CASE 1
        
        else if (prev_alloc && !next_alloc) // CASE 2
        {
            size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
            PUT(HDRP(bp), PACK(size, 0));
            PUT(FTRP(bp), PACK(size, 0));
        }

        else if (!prev_alloc && next_alloc) // CASE 3
        {
            size += GET_SIZE(HDRP(PREV_BLKP(bp))); // FTRP는 X?
            PUT(FTRP(bp), PACK(size, 0));
            PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
            bp = PREV_BLKP(bp);
        }

        else // CASE 4 
        {
            size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(FTRP(NEXT_BLKP(bp)));
            PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
            PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
            bp = PREV_BLKP(bp);
        }

        last_allocated = bp;
        return bp;
    #endif
}

static void *extend_heap(size_t words)
{
    char *bp;
    size_t size;

    size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE;
    if ((long)(bp = mem_sbrk(size)) == -1) return NULL;

    PUT(HDRP(bp), PACK(size, 0)); // new free block header
    PUT(FTRP(bp), PACK(size, 0)); // new free block footer
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1)); // mew epilogue header

    return coalesce(bp);
}

static void *find_fit(size_t asize)
{
    

#ifdef FIRST_FIT

    void *bp;

    for (bp = heap_listp; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp)) {
        if (!GET_ALLOC(HDRP(bp)) && GET_SIZE(HDRP(bp)) >= asize) return bp;
    }

    return NULL;

#elif defined(NEXT_FIT)

    void *bp;

    if (last_allocated == NULL || last_allocated < heap_listp || last_allocated > mem_heap_hi()) {
        last_allocated = heap_listp;
    }

    for (bp = last_allocated; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp)) {

        if (!GET_ALLOC(HDRP(bp)) && GET_SIZE(HDRP(bp)) >= asize) {
            last_allocated = NEXT_BLKP(bp);
            return bp;
        }
    }

    for (bp = heap_listp; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp)) {
        if (!GET_ALLOC(HDRP(bp)) && GET_SIZE(HDRP(bp)) >= asize) {
            last_allocated = NEXT_BLKP(bp);
            return bp;
        }
    }

    return NULL; 

#elif defined(BEST_FIT)

    void *best_fit = NULL;
    void *bp;

    for (bp = mem_heap_lo() + 2 * WSIZE; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp)) {
        if (!GET_ALLOC(HDRP(bp)) && (GET_SIZE(HDRP(bp)) >= asize)) {
            if (!best_fit || GET_SIZE(HDRP(bp)) < GET_SIZE(HDRP(best_fit))) {
                best_fit = bp;
                // printf("BEST FIT\n"); // 이거 왜 무한?
            }
        }
    }

    return best_fit;

#else

    void *bp = free_listp;
    while (bp != NULL) {
        if (asize <= GET_SIZE(HDRP(bp))) return bp;
        bp = SUCCP(bp);
    }
    return NULL;

#endif
}
    
static void place(void *bp, size_t asize)
{
    #ifdef LIFO

        remove_from_free(bp);
        
        size_t csize = GET_SIZE(HDRP(bp));

        if ((csize - asize) >= (2 * DSIZE)) {
            PUT(HDRP(bp), PACK(asize, 1));
            PUT(FTRP(bp), PACK(asize, 1));

            PUT(HDRP(NEXT_BLKP(bp)), PACK(csize - asize, 0));
            PUT(FTRP(NEXT_BLKP(bp)), PACK(csize - asize, 0));
            add_to_free(NEXT_BLKP(bp)); 
        } else {
            PUT(HDRP(bp), PACK(csize, 1));
            PUT(FTRP(bp), PACK(csize, 1));
        }

    #elif defined(ADDRESS)
    
        remove_from_free(bp);

        size_t csize = GET_SIZE(HDRP(bp));

        if ((csize - asize) >= (2 * DSIZE)) {

            // 현재 블록을 나누어서 할당
            PUT(HDRP(bp), PACK(asize, 1)); // 할당된 블록 설정
            PUT(FTRP(bp), PACK(asize, 1)); // 할당된 블록 설정
            bp = NEXT_BLKP(bp);
            PUT(HDRP(bp), PACK((csize - asize), 0)); // 나머지 블록 설정
            PUT(FTRP(bp), PACK((csize - asize), 0)); // 나머지 블록 설정
            add_to_free(bp); // 나머지 블록 가용 리스트에 추가

        } else {

            // 나눌 수 없을 경우
            PUT(HDRP(bp), PACK(csize, 1)); // 전체 블록을 할당
            PUT(FTRP(bp), PACK(csize, 1)); // 전체 블록을 할당

        }
    #else

        size_t csize = GET_SIZE(HDRP(bp));

        if ((csize - asize) >= (2 * DSIZE)) {

            PUT(HDRP(bp), PACK(asize, 1));
            PUT(FTRP(bp), PACK(asize, 1));
            bp = NEXT_BLKP(bp);
            PUT(HDRP(bp), PACK((csize-asize), 0));
            PUT(FTRP(bp), PACK((csize-asize), 0));
            
        } else {

            PUT(HDRP(bp), PACK(csize, 1));
            PUT(FTRP(bp), PACK(csize, 1));

        }

    #endif
}

static void remove_from_free(void *bp) 
{
    // bp의 이전 블록과 다음 블록을 찾습니다.
    void *prevp = PREDP(bp);
    void *nextp = SUCCP(bp);

    // 가용 리스트에서 bp를 제거
    if (prevp != NULL) {
        SUCCP(prevp) = nextp;
    } else {
        free_listp = nextp; // bp가 리스트의 맨 앞에 있을 경우
    }
    if (nextp != NULL) {
        PREDP(nextp) = prevp;
    }
}


// 가용 리스트의 맨 앞에 현재 블록 추가
static void add_to_free(void *bp)
{
    #ifdef LIFO
        SUCCP(bp) = free_listp;
        PREDP(bp) = NULL; //
        if (free_listp != NULL) {
            PREDP(free_listp) = bp;
        }
        free_listp = bp;
    #else
        // 삽입할 위치 : prev 뒤, current 앞
        void *currentp = free_listp;
        void *prevp = NULL;
        while (currentp != NULL && currentp < bp) {
            prevp = currentp;
            currentp = SUCCP(currentp);
        }

        // 가용 리스트에 삽입합니다.
        SUCCP(bp) = currentp;
        PREDP(bp) = prevp;
        if (currentp != NULL) {
            PREDP(currentp) = bp;
        }
        if (prevp != NULL) {
            SUCCP(prevp) = bp;
        } else {
            free_listp = bp; // 맨 앞에 삽입됩니다.
        }
    #endif
}