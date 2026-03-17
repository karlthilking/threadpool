#include "tp.h"

thpool_t *thpool_create(u16 nr_thrds)
{
        thpool_t *tp = malloc(sizeof(thpool_t));
        
        tp->nr_thrds = nr_thrds;
        tp->thrds = malloc(sizeof(pthread_t) * nr_thrds);

        tp->pending = malloc(sizeof(task_queue_t));
        tp->pending->empty = 1;
        tp->pending->front = NULL;
        tp->pending->back = NULL;

        tp->done = malloc(sizeof(task_list_t));
        tp->done->head = NULL;
        
        pthread_cond_init(&tp->cond, NULL);
        pthread_mutex_init(&tp->q_mtx, NULL);
        pthread_mutex_init(&tp->lst_mtx, NULL);

        tp->flag = TP_NONE_FLAG;
        for (int i = 0; i < nr_thrds; i++)
                pthread_create(tp->thrds + i, NULL,
                               thpool_worker, (void *)tp); 
}

void thpool_destroy(thpool_t *tp)
{
        pthread_mutex_lock(&tp->q_mtx);
        tp->flag = TP_EXIT_FLAG;
        pthread_cond_broadcast(&tp->cond);
        pthread_mutex_unlock(&tp->q_mtx);

        for (int i = 0; i < tp->nr_thrds; i++)
                pthread_join(tp->thrds[i], NULL);
        
        pthread_cond_destroy(&tp->cond);
        pthread_mutex_destroy(&tp->q_mtx);
        pthread_mutex_destroy(&tp->lst_mtx);

        free(tp->thrds);
        free(tp->pending);
        free(tp->done);
        free(tp);
}

void *thpool_insert(thpool_t *tp, void *(*f)(void *), void *arg)
{
        task_t *task = malloc(sizeof(task_t));
        task->f = f;
        task->arg = arg;

        pthread_mutex_lock(&tp->q_mtx);
        
        if (tp->pending->empty) {
                tp->pending->front = task;
                tp->pending->back = task;
                tp->pending->empty = 0;
        } else {
                tp->pending->back->next = task;
                tp->pending->back = task;
        }
        
        pthread_cond_signal(&tp->cond);
        pthread_mutex_unlock(&tp->q_mtx);
}

void thpool_stop(thpool_t *tp)
{
        pthread_mutex_lock(&tp->q_mtx);
        tp->flag = TP_STOP_FLAG;
        pthread_mutex_unlock(&tp->q_mtx);
}

void thpool_cont(thpool_t *tp)
{
        pthread_mutex_lock(&tp->q_mtx);
        tp->flag = TP_NONE_FLAG;
        pthread_cond_broadcast(&tp->cond);
        pthread_mutex_unlock(&tp->q_mtx);
}

void *thpool_worker(void *arg)
{
        thpool_t *tp = (thpool_t *)arg;

        for (;;) {
                task_t *t = NULL;
                
                pthread_mutex_lock(&tp->q_mtx);
                
                /**
                 * Wait while task queue is empty and it is not
                 * necessary to exit
                 */
                while ((tp->pending->empty && !TP_EXIT(tp->flag)) ||
                       TP_STOP(tp->flag))
                        pthread_cond_wait(&tp->cond, &tp->q_mtx);
                
                /**
                 * Exit if no tasks remain and the flag indicates
                 * that exiting should begin
                 */
                if (tp->pending->empty && TP_EXIT(tp->flag)) {
                        pthread_mutex_unlock(&tp->q_mtx);
                        return NULL;
                }
                
                /* Pop task front task queue */
                t = tp->pending->front;
                task_t *next = tp->pending->front->next;
                if (!next)
                        tp->pending->empty = 1;
                else 
                        tp->pending->front = next;
                pthread_mutex_unlock(&tp->q_mtx);
                
                /* Invoke function and store result in task */
                t->result = t->f(t->arg);
                t->complete = 1;

                /* Insert task into list of completed tasks */
                pthread_mutex_lock(&tp->lst_mtx);
                t->next = tp->done->head;
                tp->done->head = t;
                pthread_mutex_unlock(&tp->lst_mtx);
        }
}
