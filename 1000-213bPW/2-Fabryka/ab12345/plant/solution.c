#include "../common/err.h"
#include "../common/plant.h"

#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>

/* -------------------------------------------------------------------------- */

/**
 * Extended station type.
 * 
 * @capacity: Maximum number of workers that may work at this station at once.
 * @num_workers: Current number of workers that are working at this station.
 */
typedef struct {
    size_t capacity;
    size_t num_workers;
} station_ex_t;

station_ex_t* stations_ex;
size_t num_stations, max_station_capacity;

/* -------------------------------------------------------------------------- */

enum task_status {
    PENDING,
    IN_PROGRESS,
    FINISHED
};

/**
 * Extended task type.
 * 
 * @task: Pointer to the base task object.
 * @id: A unique task identifier, equal to the base task identifier.
 * @status: Current task status (`PENDING`/`IN_PROGRESS`/`FINISHED`).
 * @is_skipped: `True` if task was skipped due to not having enough workers.
 * @task_finished: Condition for threads waiting until this task is finished.
 * @num_waiting: Number of threads waiting on `task_finished` condition.
 * @none_waiting: Condition for threads waiting until `num_waiting = 0`.
 */
typedef struct {
    task_t* task;
    int id;
    enum task_status status;
    bool is_skipped;
    pthread_cond_t task_finished;
    size_t num_waiting;
    pthread_cond_t none_waiting;
} task_ex_t;

task_ex_t** task_ex_ptrs;
size_t num_tasks, size_tasks, num_pending_tasks, num_in_progress_tasks;

/**
 * Finds the extended task entry for a given task.
 * 
 * @param t Pointer to the base task object.
 * @return Pointer to the matching `task_ex_t` entry, or `NULL` if not found.
 */
task_ex_t* task_ex_ptrs_find(task_t* t) {
    for (size_t i = 0; i < num_tasks; ++i) {
        if (task_ex_ptrs[i]->id == t->id) {
            return task_ex_ptrs[i];
        }
    }

    return NULL;
}

/**
 * Appends a new extended task entry with status set to `PENDING`.
 * 
 * @param t Pointer to the base task object.
 * @return Pointer to the newly added `task_ex_t` entry, or `NULL` if failed.
 */
task_ex_t* task_ex_ptrs_append(task_t* t) {
    // Allocate memory for a new entry.
    task_ex_t* task_ex_ptr = (task_ex_t*)malloc(sizeof(task_ex_t));

    if (task_ex_ptr == NULL) {
        return NULL;
    }

    // Check if we need to resize the array.
    if (num_tasks == size_tasks) {
        task_ex_t** tmp = (task_ex_t**)realloc(
            task_ex_ptrs,
            2 * size_tasks * sizeof(task_ex_t*)
        );

        if (tmp == NULL) {
            free(task_ex_ptr); // Free the previously allocated memory.
            return NULL;
        }

        task_ex_ptrs = tmp;
        size_tasks *= 2;
    }

    // Initialize the entry.
    task_ex_ptr->task = t;
    task_ex_ptr->id = t->id;
    task_ex_ptr->status = PENDING;
    task_ex_ptr->is_skipped = false;
    ASSERT_ZERO(pthread_cond_init(&task_ex_ptr->task_finished, NULL));
    task_ex_ptr->num_waiting = 0;
    ASSERT_ZERO(pthread_cond_init(&task_ex_ptr->none_waiting, NULL));

    num_pending_tasks++;

    return task_ex_ptrs[num_tasks++] = task_ex_ptr;
};

/* -------------------------------------------------------------------------- */

enum worker_status {
    IDLE,
    WORKING
};

/**
 * Extended worker type.
 * 
 * @worker: Pointer to the base worker object.
 * @id: A unique worker identifier, equal to the base worker identifier.
 * @status: Current worker status (`IDLE`/`WORKING`).
 * @work_args: Placeholder for the `start_routine` argument (see `work_args_t`).
 */
struct worker_ex_t;

// Function `pthread_create()` requires its `start_routine`
// argument to have one argument of type `void*`.
typedef struct {
    struct worker_ex_t* w_ex_ptr; 
    task_ex_t* t_ex_ptr;
    int station_index;
    int i;
} work_args_t;

typedef struct worker_ex_t {
    worker_t* worker;
    int id;
    enum worker_status status;
    work_args_t work_args;
} worker_ex_t;

worker_ex_t** worker_ex_ptrs;
size_t num_active_workers, num_total_workers, num_workers_to_come;

/**
 * Finds the extended worker entry for a given worker.
 * 
 * @param w Pointer to the base worker object.
 * @return Pointer to the matching `worker_ex_t` entry, or `NULL` if not found.
 */
worker_ex_t* worker_ex_ptrs_find(worker_t* w) {
    for (size_t i = 0; i < num_total_workers; ++i) {
        if (worker_ex_ptrs[i]->id == w->id) {
            return worker_ex_ptrs[i];
        }
    }
    
    return NULL;
}

/**
 * Appends a new extended worker entry with status set to `IDLE`.
 * 
 * @param w Pointer to the base worker object.
 * @return Pointer to the newly added `worker_ex_t` entry, or `NULL` if failed.
 */
worker_ex_t* worker_ex_ptrs_append(worker_t* w) {
    // Allocate memory for a new entry.
    worker_ex_t* worker_ex_ptr = (worker_ex_t*)malloc(sizeof(worker_ex_t));

    if (worker_ex_ptr == NULL) {
        return NULL;
    }

    // Initialize the entry.
    worker_ex_ptr->worker = w;
    worker_ex_ptr->id = w->id;
    worker_ex_ptr->status = IDLE;
    worker_ex_ptr->work_args.w_ex_ptr = worker_ex_ptr;

    // Move the first inactive worker to the end.
    worker_ex_ptrs[num_total_workers++] = worker_ex_ptrs[num_active_workers];

    num_workers_to_come--;
    
    return worker_ex_ptrs[num_active_workers++] = worker_ex_ptr;    
}

/* -------------------------------------------------------------------------- */

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

bool is_plant_initialized = false, is_plant_being_destroyed = false;
pthread_cond_t plant_idle;

/**
 * Updates the plant on any interesting events in the future:
 * - worker starts or ends working,
 * - task can now be started.
 */
pthread_t alarm_thread;

time_t next_alarm_time;
pthread_cond_t alarm_cond;

/* -------------------------------------------------------------------------- */

/**
 * Updates the plant state.
 * 
 * @note Requires an active `mutex` ownership.
 */
void tick_plant();

void do_work(worker_ex_t* w_ex, task_ex_t* t_ex, int station_index, int i) {
    // Work on the assigned task (non-blocking).
    t_ex->task->results[i] = w_ex->worker->work(w_ex->worker, t_ex->task, i);

    ASSERT_ZERO(pthread_mutex_lock(&mutex));
    
    w_ex->status = IDLE;
    
    // Check if the last worker finishes working and the task is finished.
    if (--stations_ex[station_index].num_workers == 0) {
        t_ex->status = FINISHED;

        // Broadcast other threads waiting until this task is finished.
        ASSERT_ZERO(pthread_cond_broadcast(&t_ex->task_finished));
        
        // [!] ORDER DEPENDENCY [!]
        // If there are no tasks (present or future) to wait for,
        // signal the thread waiting until they are all finished.
        if (--num_in_progress_tasks == 0 && num_pending_tasks == 0) {
            ASSERT_ZERO(pthread_cond_signal(&plant_idle));
        }
    }
    
    // Update the plant state.
    tick_plant();

    ASSERT_ZERO(pthread_mutex_unlock(&mutex));
}

// Function `pthread_create()` requires its `start_routine`
// argument to be of type `void*`.
void* do_work_pthread_wrap(void* arg) {
    work_args_t* args = (work_args_t*)arg;
    do_work(args->w_ex_ptr, args->t_ex_ptr, args->station_index, args->i);
    return NULL;
}

void tick_plant() {
    time_t now = time(NULL);

    // Filter all active workers (i.e. those with `.end < now`)
    // such that each worker from the `[0, num_active_workers)` range is active.
    for (size_t i = 0; i < num_active_workers; ++i) {
        while (i < num_active_workers &&
               now >= worker_ex_ptrs[i]->worker->end
        ) {
            // Swap `worker_ex_ptrs[i]` and
            // `worker_ex_ptrs[num_active_workers - 1]`, while decrementing
            // `num_active_workers` by 1.
            worker_ex_t* tmp = worker_ex_ptrs[i];
            worker_ex_ptrs[i] = worker_ex_ptrs[num_active_workers - 1];
            worker_ex_ptrs[--num_active_workers] = tmp;
        }
    }
    
    // Skip all tasks that we now know can never be done, because:
    // - there will never be enough workers, or
    // - no station has sufficient capacity.
    for (size_t i = 0; i < num_tasks; ++i) {
        if (task_ex_ptrs[i]->status == PENDING) {
            size_t max_num_workers = num_active_workers + num_workers_to_come;
            
            if (max_num_workers < task_ex_ptrs[i]->task->capacity ||
                max_station_capacity < task_ex_ptrs[i]->task->capacity
            ) {
                task_ex_ptrs[i]->status = FINISHED;
                task_ex_ptrs[i]->is_skipped = true;

                num_pending_tasks--;
                
                // Broadcast other threads waiting until this task is finished.
                ASSERT_ZERO(
                    pthread_cond_broadcast(&task_ex_ptrs[i]->task_finished)
                );
            }
        }
    }

    // If there are no tasks (present or future) to wait for,
    // signal the thread waiting until they are all finished.
    if (num_pending_tasks == 0 && num_in_progress_tasks == 0) {
        ASSERT_ZERO(pthread_cond_signal(&plant_idle));
        return;
    }
        
    while(true) {
        size_t num_workers = 0;
        
        // Count the number of workers who are able to work
        // (i.e. those with `.start <= now` and `IDLE` status).
        for (size_t i = 0; i < num_active_workers; ++i) {
            if (now >= worker_ex_ptrs[i]->worker->start &&
                worker_ex_ptrs[i]->status == IDLE
            ) {
                num_workers++;
            }
        }
        
        station_ex_t* station_ex_ptr = NULL;
        size_t station_index;
        
        // Out of all free stations, pick one with
        // the biggest capacity and keep its index.
        for (size_t i = 0; i < num_stations; ++i) {
            if (stations_ex[i].num_workers == 0) {
                if (station_ex_ptr == NULL ||
                    station_ex_ptr->capacity < stations_ex[i].capacity
                ) {
                    station_ex_ptr = &stations_ex[i];
                    station_index = i;
                }
            }
        }
        
        // Break if not found.
        if (station_ex_ptr == NULL) {
            break;
        }
        
        task_ex_t* task_ex_ptr = NULL;
        
        // Out of all tasks with `PENDING` state that we can now work on,
        // pick the first one that we do have the resources to take.
        for (size_t i = 0; i < num_tasks; ++i) {
            if (task_ex_ptrs[i]->status == PENDING &&
                now >= task_ex_ptrs[i]->task->start
            ) {
                if (num_workers >= task_ex_ptrs[i]->task->capacity &&
                    station_ex_ptr->capacity >= task_ex_ptrs[i]->task->capacity
                ) {
                    task_ex_ptr = task_ex_ptrs[i];
                    break;
                }
            }
        }
        
        // Break if not found.
        if (task_ex_ptr == NULL) {
            break;
        }
    
        // Change the task status from `PENDING` to `IN_PROGRESS`.
        station_ex_ptr->num_workers = task_ex_ptr->task->capacity;
        task_ex_ptr->status = IN_PROGRESS;
        num_pending_tasks--;
        num_in_progress_tasks++;
        
        size_t capacity_cpy = task_ex_ptr->task->capacity;
    
        // Assign workers to this task (with descending indices).
        for (size_t i = 0; i < num_active_workers && capacity_cpy > 0; ++i) {
            if (now >= worker_ex_ptrs[i]->worker->start &&
                worker_ex_ptrs[i]->status == IDLE
            ) {
                worker_ex_ptrs[i]->status = WORKING;
    
                work_args_t* args = &worker_ex_ptrs[i]->work_args;

                // Initialize the worker arguments.
                args->t_ex_ptr = task_ex_ptr;
                args->station_index = station_index;
                args->i = --capacity_cpy;
    
                pthread_t thread_id;
    
                // Run each worker in a separate thread.
                ASSERT_ZERO(
                    pthread_create(&thread_id, NULL, do_work_pthread_wrap, args)
                );
    
                // Do not wait for worker threads to finish.
                ASSERT_ZERO(pthread_detach(thread_id));
            }
        }
    }

    time_t min_future_action_time = 0;

    // Find the earliest future worker starting or ending time.
    for (size_t i = 0; i < num_active_workers; ++i) {
        if (now < worker_ex_ptrs[i]->worker->start) {
            if (min_future_action_time == 0 ||
                min_future_action_time > worker_ex_ptrs[i]->worker->start
            ) {
                min_future_action_time= worker_ex_ptrs[i]->worker->start;
            }
        }

        if (now < worker_ex_ptrs[i]->worker->end) {
            if (min_future_action_time == 0 ||
                min_future_action_time > worker_ex_ptrs[i]->worker->end
            ) {
                min_future_action_time = worker_ex_ptrs[i]->worker->end;
            }
        }
    }

    // Find the earliest future task starting time.
    for (size_t i = 0; i < num_tasks; ++i) {
        if (now < task_ex_ptrs[i]->task->start) {
            if (min_future_action_time == 0 ||
                min_future_action_time > task_ex_ptrs[i]->task->start
            ) {
                min_future_action_time = task_ex_ptrs[i]->task->start;
            }
        }
    }

    // Update the alarm thread if we found an earlier alarm time.
    if (min_future_action_time != 0 &&
        (next_alarm_time == 0 || min_future_action_time < next_alarm_time)
    ) {
        next_alarm_time = min_future_action_time;
        ASSERT_ZERO(pthread_cond_signal(&alarm_cond));
    }
}

/* -------------------------------------------------------------------------- */

// Needs to be defined manually, as importing `<errno.h>`
// causes errors with the `ASSERT_ZERO()` function.
#ifndef ETIMEDOUT
#define ETIMEDOUT 110
#endif

// Function `pthread_create()` requires its `start_routine` argument
// to be of type `void*` and have one argument of type `void*`.
void* alarm_thread_func(void* arg) {
    ASSERT_ZERO(pthread_mutex_lock(&mutex));

    while (is_plant_initialized) {
        if (next_alarm_time == 0) {
            // Nothing to wait for, just sleep until signaled.
            ASSERT_ZERO(pthread_cond_wait(&alarm_cond, &mutex));
        }
        else {
            struct timespec ts;

            // Initialize the `timespec` struct.
            ts.tv_sec = next_alarm_time;
            ts.tv_nsec = 0;

            int ret = pthread_cond_timedwait(&alarm_cond, &mutex, &ts);

            // Ensure the plant did not get destroyed while waiting.
            if (is_plant_initialized && ret == ETIMEDOUT) {
                next_alarm_time = 0;
                tick_plant(); // Update the plant state.
            }
        }
    }

    ASSERT_ZERO(pthread_mutex_unlock(&mutex));

    return NULL;
}

/* -------------------------------------------------------------------------- */

int init_plant(int* stations, int n_stations, int n_workers)
{
    ASSERT_ZERO(pthread_mutex_lock(&mutex));

    // Ensure the plant is not initialized yet.
    if (is_plant_initialized) {
        ASSERT_ZERO(pthread_mutex_unlock(&mutex));
        return ERROR;
    }

    // Allocate memory for the plant.
    stations_ex = (station_ex_t*)malloc(n_stations * sizeof(station_ex_t));
    worker_ex_ptrs = (worker_ex_t**)malloc(n_workers * sizeof(worker_ex_t*));
    task_ex_ptrs = (task_ex_t**)malloc(sizeof(task_ex_t*));

    if (stations_ex == NULL || worker_ex_ptrs == NULL || task_ex_ptrs == NULL) {
        // Free the previously allocated memory.
        free(stations_ex);
        free(worker_ex_ptrs);
        free(task_ex_ptrs);

        ASSERT_ZERO(pthread_mutex_unlock(&mutex));
        return ERROR;
    }

    // Initialize variables (stations).
    num_stations = (size_t)n_stations;
    max_station_capacity = 0;

    // Initialize extended station entries.
    for (size_t i = 0; i < num_stations; ++i) {
        stations_ex[i].capacity = stations[i];
        stations_ex[i].num_workers = 0;

        if (stations_ex[i].capacity > max_station_capacity) {
            max_station_capacity = stations_ex[i].capacity;
        }
    }
    
    // Initialize variables (tasks).
    num_tasks = 0;
    size_tasks = 1; // There are no tasks yet, but their initial capacity is 1.
    num_pending_tasks = 0;
    num_in_progress_tasks = 0;

    // Initialize variables (workers).
    num_active_workers = 0;
    num_total_workers = 0;
    num_workers_to_come = (size_t)n_workers;
    
    // Initialize the plant variables.
    is_plant_initialized = true;
    is_plant_being_destroyed = false;
    ASSERT_ZERO(pthread_cond_init(&plant_idle, NULL));

    // Initialize the alarm thread variables.
    next_alarm_time = 0;
    ASSERT_ZERO(pthread_cond_init(&alarm_cond, NULL));

    // Start the alarm thread.
    ASSERT_ZERO(pthread_create(&alarm_thread, NULL, alarm_thread_func, NULL));

    ASSERT_ZERO(pthread_mutex_unlock(&mutex));
    
    return PLANTOK;
}

int destroy_plant()
{
    ASSERT_ZERO(pthread_mutex_lock(&mutex));

    // Ensure we are in between `init_plant()` and `destroy_plant()`.
    if (!is_plant_initialized || is_plant_being_destroyed) {
        ASSERT_ZERO(pthread_mutex_unlock(&mutex));
        return ERROR;
    }

    is_plant_being_destroyed = true;

    // Wait until all tasks (present and future) are finished.
    while (num_pending_tasks > 0 || num_in_progress_tasks > 0) {
        ASSERT_ZERO(pthread_cond_wait(&plant_idle, &mutex));
    }

    for (size_t i = 0; i < num_tasks; ++i) {
        // Wait for everyone to have collected this task.
        while (task_ex_ptrs[i]->num_waiting > 0) {
            ASSERT_ZERO(
                pthread_cond_wait(&task_ex_ptrs[i]->none_waiting, &mutex)
            );
        }
    }
    
    // Free the previously allocated memory (extended worker entries).
    for (size_t i = 0; i < num_total_workers; ++i) {
        free(worker_ex_ptrs[i]);
    }
    
    // Free the previously allocated memory (extended task entries).
    for (size_t i = 0; i < num_tasks; ++i) {
        free(task_ex_ptrs[i]);
    }
    
    // Free the previously allocated memory.
    free(stations_ex);
    free(worker_ex_ptrs);
    free(task_ex_ptrs);
    
    // Leave the plant variables in a valid state for reinitialization.
    is_plant_initialized = false;
    is_plant_being_destroyed = false;

    // Signal the alarm thread.
    // For this to work, `is_plant_initialized` must be equal `false`.
    ASSERT_ZERO(pthread_cond_signal(&alarm_cond));

    ASSERT_ZERO(pthread_mutex_unlock(&mutex));
    
    // (optional, the alarm thread will break the while loop and exit anyway)
    ASSERT_ZERO(pthread_join(alarm_thread, NULL));

    return PLANTOK;
}

int add_worker(worker_t* w)
{
    ASSERT_ZERO(pthread_mutex_lock(&mutex));

    // Ensure we are in between `init_plant()` and `destroy_plant()`.
    if (!is_plant_initialized || is_plant_being_destroyed) {
        ASSERT_ZERO(pthread_mutex_unlock(&mutex));
        return ERROR;
    }
    
    // Ignore multiple additions of the same worker.
    if (worker_ex_ptrs_find(w) == NULL) {
        if (worker_ex_ptrs_append(w) == NULL) {
            ASSERT_ZERO(pthread_mutex_unlock(&mutex));
            return ERROR;
        }

        // Update the plant state.
        tick_plant();
    }

    ASSERT_ZERO(pthread_mutex_unlock(&mutex));

    return PLANTOK;
}

int add_task(task_t* t)
{
    ASSERT_ZERO(pthread_mutex_lock(&mutex));

    // Ensure we are in between `init_plant()` and `destroy_plant()`.
    if (!is_plant_initialized || is_plant_being_destroyed) {
        ASSERT_ZERO(pthread_mutex_unlock(&mutex));
        return ERROR;
    }

    // Ignore multiple additions of the same task.
    if (task_ex_ptrs_find(t) == NULL) {
        if (task_ex_ptrs_append(t) == NULL) {
            ASSERT_ZERO(pthread_mutex_unlock(&mutex));
            return ERROR;
        }
        
        // Update the plant state.
        tick_plant();
    }

    ASSERT_ZERO(pthread_mutex_unlock(&mutex));

    return PLANTOK;
}

int collect_task(task_t* t)
{
    ASSERT_ZERO(pthread_mutex_lock(&mutex));

    // Ensure we are in between `init_plant()` and `destroy_plant()`.
    if (!is_plant_initialized || is_plant_being_destroyed) {
        ASSERT_ZERO(pthread_mutex_unlock(&mutex));
        return ERROR;
    }

    task_ex_t* task_ex_ptr = task_ex_ptrs_find(t);

    // We cannot wait for this task, as it was never added in the first place.
    if (task_ex_ptr == NULL) {
        ASSERT_ZERO(pthread_mutex_unlock(&mutex));
        return ERROR;
    }

    // Wait until this task is finished.
    while (task_ex_ptr->status != FINISHED) {
        task_ex_ptr->num_waiting++;
        ASSERT_ZERO(pthread_cond_wait(&task_ex_ptr->task_finished, &mutex));
        task_ex_ptr->num_waiting--;
    }

    if (task_ex_ptr->num_waiting == 0) {
        // Signal the thread waiting for everyone to have collected this task.
        ASSERT_ZERO(pthread_cond_signal(&task_ex_ptr->none_waiting));
    }

    // Save the return value,
    // as the pointer may be freed during `destroy_plant()`.
    int ret = task_ex_ptr->is_skipped ? ERROR : PLANTOK;

    ASSERT_ZERO(pthread_mutex_unlock(&mutex));

    return ret;
}
