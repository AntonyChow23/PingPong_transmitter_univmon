#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define LCHILD(x) 2 * x + 1
#define RCHILD(x) 2 * x + 2
#define PARENT(x) (x - 1) / 2

/* define data types */
// #define SMALL_COUNTER

#ifdef SMALL_COUNTER
typedef int16_t my_int;
#else
typedef int32_t my_int;
#endif

typedef uint32_t my_key;

typedef struct node {
    my_key key;
    my_int count;
} node;

typedef struct minHeap {
    uint32_t size;
    node *elem;
} minHeap;

void heapify(minHeap *hp, int i);
void initMinHeap(minHeap *heap, uint32_t size);
void deleteNode(minHeap *hp);
void insertNode(minHeap *hp, my_key _key, my_int _counter);
int find(minHeap *hp, uint32_t _key);
void deleteMinHeap(minHeap *hp);