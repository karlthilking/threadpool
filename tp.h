#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <pthread.h>
#include <stdint.h>
#include <stdatomic.h>
#include <stdlib.h>

#define TP_NONE_FLAG    0x0
#define TP_STOP_FLAG    0x1
#define TP_EXIT_FLAG    0x2

#define TP_STOP(f)      ((f) & TP_STOP_FLAG)
#define TP_EXIT(f)      ((f) & TP_EXIT_FLAG)

typedef int8_t          i8;
typedef uint8_t         u8;
typedef int16_t         i16;
typedef uint16_t        u16;
typedef int32_t         i32;
typedef uint32_t        u32;
typedef int64_t         i64;
typedef uint64_t        u64;

typedef struct __task_t         task_t;
typedef struct __task_list_t    task_list_t;
typedef struct __task_queue_t   task_queue_t;
typedef struct __thpool_t       thpool_t;

typedef struct __task_t {
        void    *(*f)(void *);
        void    *arg;
        void    *result;
        union {
                task_t *next;
                task_t *prev;
        };
        u8      complete;
} task_t;

typedef struct __task_list_t {
        task_t *head;
} task_list_t;

typedef struct __task_queue_t {
        task_t          *front;
        task_t          *back;
        u8              empty;
} task_queue_t;

typedef struct __thpool_t {
        pthread_mutex_t q_mtx;
        pthread_mutex_t lst_mtx;
        pthread_cond_t  cond;
        pthread_t       *thrds;
        task_queue_t    *pending;
        task_list_t     *done;
        u16             nr_thrds;
        u8              flag;
} thpool_t;

thpool_t        *thpool_create(u16);
void            thpool_destroy(thpool_t *);
void            *thpool_insert(thpool_t *, void *(*)(void *), void *);
void            thpool_stop(thpool_t *);
void            thpool_cont(thpool_t *);
void            *thpool_worker(void *);

#endif
