#include "./struct/struct.h"
// ==========鹅队列相关函数==========
int IsEmpty(Goose_queue *Q) {
    return (Q->head == Q->tail && Q->head->next == NULL);
}

int Init_goose_queue(Goose_queue* Q) {
    Q->head=Q->tail=(gooser)malloc(sizeof(goose));
    if(!Q->head)return 0;
    Q->head->next = NULL;
    return 1;
}

int ADD_goose_queue(Goose_queue *Q) {
    gooser new_node = (gooser)malloc(sizeof(goose));
    if (!new_node) return 0;
    new_node->next = NULL;
    new_node->egg_intime = 0;
    new_node->egg_outtime = 0;
    Q->tail->next = new_node;
    Q->tail = new_node;
    return 1;
}

int De_goose_queue(Goose_queue *Q) {
    if (Q->tail->next==NULL) {
        return 0;
    }
    gooser del_node = Q->head->next;
    Q->head->next = del_node->next;
    if (del_node == Q->tail) {
        Q->tail = Q->head;
    }
    free(del_node);
    return 1;
}

void TraverseQueue(Goose_queue *Q, uint8_t *a) {
    if (IsEmpty(Q)) {
        Usart1_Printf("队列为空\r\n");
        return;
    }
    gooser p = Q->head->next;
    while (p) {
        free(p);
        p = p->next;
    }
}

void De_this_goose_queue(Goose_queue *Q, gooser node) {
    gooser NODE=Q->head;
    if (IsEmpty(Q)) {
        Usart1_Printf("队列为空\r\n");
        return;
    }
    while(NODE->next != NULL) {
        if(NODE->next==node) {
            NODE->next=node->next;
            if(node == Q->tail) {
                Q->tail = NODE ;
            }
            break;
        } else {
            NODE=NODE->next;
        }
    }
}

int IsGooseInQueue(Goose_queue *Q, uint8_t *id) {
    gooser node = Q->head->next;
    while(node != NULL) {
        if(memcmp(node->id, id, LENGTH_ID) == 0) {
            return 1;
        }
        node = node->next;
    }
    return 0;
}

// ==========鹅蛋队列相关函数==========
int IsEggQueueEmpty(Egg_queue *Q) {
    return (Q->head == Q->tail && Q->head->next == NULL);
}

int Init_egg_queue(Egg_queue* Q) {
    Q->head = Q->tail = (egger)malloc(sizeof(egg_info));
    if(!Q->head) return 0;
    Q->head->next = NULL;
    return 1;
}

int ADD_egg_queue(Egg_queue *Q) {
    egger new_node = (egger)malloc(sizeof(egg_info));
    if (!new_node) return 0;
    new_node->next = NULL;
    memset(new_node->id, 0, LENGTH_ID);
    new_node->egg_intime = 0;
    new_node->egg_outtime = 0;
    new_node->goose_count = 0;
    new_node->egg_timestamp = 0;
    Q->tail->next = new_node;
    Q->tail = new_node;
    return 1;
}

int De_egg_queue(Egg_queue *Q) {
    if (Q->tail->next == NULL) {
        return 0;
    }
    egger del_node = Q->head->next;
    Q->head->next = del_node->next;
    if (del_node == Q->tail) {
        Q->tail = Q->head;
    }
    free(del_node);
    return 1;
}

void De_this_egg_queue(Egg_queue *Q, egger node) {
    egger NODE = Q->head;
    if (IsEggQueueEmpty(Q)) {
        Usart1_Printf("鹅蛋队列为空\r\n");
        return;
    }
    while(NODE->next != NULL) {
        if(NODE->next == node) {
            NODE->next = node->next;
            if(node == Q->tail) {
                Q->tail = NODE;
            }
            free(node);
            break;
        } else {
            NODE = NODE->next;
        }
    }
}

