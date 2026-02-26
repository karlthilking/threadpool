/* threadpool.h */
#ifndef THREAD_POOL_H
#define THREAD_POOL_H
#include <pthread.h>
#include <stdlib.h>
#include <stdint.h>

typedef struct task_t {
    void *(*f)(void *);
    void *arg;
    struct task_t *next;
} task_t;

typedef struct {
    pthread_t           *threads;
    pthread_mutex_t     mtx;
    pthread_cond_t      cv;
    task_t              *head;
    uint16_t            nthreads;
    uint8_t             stop;
} thread_pool_t;

void *thread_pool_worker(void *arg);

void
thread_pool_init(thread_pool_t *tp, uint16_t nthreads)
{
    tp->head = NULL;
    tp->stop = 0;
    tp->nthreads = nthreads;
    tp->threads = (pthread_t *)malloc(sizeof(pthread_t) * nthreads);
    pthread_mutex_init(&tp->mtx, NULL);
    pthread_cond_init(&tp->cv, NULL);
    for (int i = 0; i < nthreads; ++i)
        pthread_create(&tp->threads[i], NULL, thread_pool_worker, (void *)tp);
}

void
thread_pool_deinit(thread_pool_t *tp)
{
    pthread_mutex_lock(&tp->mtx);
    tp->stop = 1;
    pthread_cond_broadcast(&tp->cv);
    pthread_mutex_unlock(&tp->mtx);
    for (int i = 0; i < tp->nthreads; ++i) {
        pthread_join(tp->threads[i], NULL);
    }
    free(tp->threads);
    pthread_mutex_destroy(&tp->mtx);
    pthread_cond_destroy(&tp->cv);
}

void *
thread_pool_worker(void *arg)
{
    thread_pool_t *tp = (thread_pool_t *)arg;
    for (;;) {
        task_t *task = NULL;
        pthread_mutex_lock(&tp->mtx);
        while (tp->head == NULL && !tp->stop)
            pthread_cond_wait(&tp->cv, &tp->mtx);
        if (tp->head == NULL && tp->stop) {
            pthread_mutex_unlock(&tp->mtx);
            return NULL;
        }
        task = tp->head;
        tp->head = tp->head->next;
        pthread_mutex_unlock(&tp->mtx);
        task->f(task->arg);
        free(task);
    }
}

void
thread_pool_add(thread_pool_t *tp, void *(*f)(void *arg), void *arg)
{
    task_t *task = (task_t *)malloc(sizeof(task_t));
    task->f = f;
    task->arg = arg;
    
    pthread_mutex_lock(&tp->mtx);
    task->next = tp->head;
    tp->head = task;
    pthread_cond_signal(&tp->cv);
    pthread_mutex_unlock(&tp->mtx);
}
#endif
