#include <stdio.h>

extern int mm_init (void);
extern void *mm_malloc (size_t size);
extern void mm_free (void *ptr);
extern void *mm_realloc(void *ptr, size_t size);


/* 
 * Students work in teams of one or two.  Teams enter their team name, 
 * personal names and login IDs in a struct of this
 * type in their bits.c file.
 */
typedef struct {
    char *teamname; /* ID1+ID2 or ID1 */
    char *name1;    /* full name of first member */
    char *id1;      /* login ID of first member */
    char *name2;    /* full name of second member (if any) */
    char *id2;      /* login ID of second member */
} team_t;

extern team_t team;

/* 기본 상수 */

#define WSIZE 4 // word size

#define DSIZE 8 // double word size 

#define CHUNKSIZE (1 << 12) // 힙 확장을 위한 기본 크기 (= 초기 빈 블록의 크기)


/* 매크로 */ 

#define MAX(x, y) (x > y ? x : y) 

// size와 할당 비트를 결합, header와 footer에 저장할 값
#define PACK(size, alloc) (size | alloc) 

// p가 참조하는 워드 반환 (포인터라서 직접 역참조 불가능 -> 타입 캐스팅)
#define GET(p) (*(unsigned int *)(p)) 

// p에 val 저장 
#define PUT(p, val) (*(unsigned int *)(p) = (val)) 

// 사이즈 (~0x7: ...11111000, '&' 연산으로 뒤에 세자리 없어짐) 
#define GET_SIZE(p) (GET(p) & ~0x7) 

// 할당 상태
#define GET_ALLOC(p) (GET(p) & 0x1)

// Header 포인터
#define HDRP(bp) ((char *)(bp)-WSIZE) 

// Footer 포인터 (🚨Header의 정보를 참조해서 가져오기 때문에, Header의 정보를 변경했다면 변경된 위치의 Footer가 반환됨에 유의)
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE) 

// 다음 블록의 포인터
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp)-WSIZE))) 

// 이전 블록의 포인터 
#define PREV_BLKP(bp) ((char *)(bp)-GET_SIZE(((char *)(bp)-DSIZE)))

#define SUCCP(bp) (*(void **)((char *)(bp) + WSIZE)) // 다음 가용 블록의 주소
#define PREDP(bp) (*(void **)(bp))                   // 이전 가용 블록의 주소