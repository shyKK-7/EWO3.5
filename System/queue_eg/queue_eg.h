#ifndef __QUEUE_EG_H__
#define __QUEUE_EG_H__

#include "./struct/struct.h"

// ==========鹅队列结构体==========
typedef struct goose {
    uint8_t id[LENGTH_ID];
    uint32_t egg_intime;
    uint32_t egg_outtime;
    struct goose *next;
} goose, *gooser;

typedef struct {
    gooser head;
    gooser tail;
} Goose_queue;

// ==========鹅蛋队列结构体==========
typedef struct egg_info {
    uint8_t id[LENGTH_ID];
    uint32_t egg_intime;
    uint32_t egg_outtime;
    uint8_t goose_count;
    uint32_t egg_timestamp;
    struct egg_info *next;
} egg_info, *egger;

typedef struct {
    egger head;
    egger tail;
} Egg_queue;

// ==========函数声明==========
int IsEmpty(Goose_queue *Q);
int Init_goose_queue(Goose_queue* Q);
int ADD_goose_queue(Goose_queue *Q);
int De_goose_queue(Goose_queue *Q);
void TraverseQueue(Goose_queue *Q, uint8_t *a);
void De_this_goose_queue(Goose_queue *Q, gooser node);
int IsGooseInQueue(Goose_queue *Q, uint8_t *id);

int IsEggQueueEmpty(Egg_queue *Q);
int Init_egg_queue(Egg_queue* Q);
int ADD_egg_queue(Egg_queue *Q);
int De_egg_queue(Egg_queue *Q);
void De_this_egg_queue(Egg_queue *Q, egger node);

#endif		