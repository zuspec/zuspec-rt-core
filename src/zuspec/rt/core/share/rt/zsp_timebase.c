/**
 * zsp_timebase.c - Time-aware thread scheduler implementation
 * 
 * Replaces the previous zsp_thread scheduler with unified time management.
 * Uses a min-heap for efficient time-ordered event scheduling.
 */

#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include "zsp_timebase.h"

#define INITIAL_EVENT_CAPACITY 16

/*============================================================================
 * Min-Heap Helper Functions
 *============================================================================*/

static inline int heap_compare(zsp_timebase_event_t *a, zsp_timebase_event_t *b) {
    if (a->wake_time != b->wake_time) {
        return (a->wake_time < b->wake_time) ? -1 : 1;
    }
    /* Same time: use sequence for stable ordering */
    return (a->seq < b->seq) ? -1 : 1;
}

static void heap_sift_up(zsp_timebase_event_t *events, uint32_t idx) {
    while (idx > 0) {
        uint32_t parent = (idx - 1) / 2;
        if (heap_compare(&events[idx], &events[parent]) >= 0) {
            break;
        }
        /* Swap */
        zsp_timebase_event_t tmp = events[idx];
        events[idx] = events[parent];
        events[parent] = tmp;
        idx = parent;
    }
}

static void heap_sift_down(zsp_timebase_event_t *events, uint32_t count, uint32_t idx) {
    while (1) {
        uint32_t smallest = idx;
        uint32_t left = 2 * idx + 1;
        uint32_t right = 2 * idx + 2;
        
        if (left < count && heap_compare(&events[left], &events[smallest]) < 0) {
            smallest = left;
        }
        if (right < count && heap_compare(&events[right], &events[smallest]) < 0) {
            smallest = right;
        }
        if (smallest == idx) {
            break;
        }
        /* Swap */
        zsp_timebase_event_t tmp = events[idx];
        events[idx] = events[smallest];
        events[smallest] = tmp;
        idx = smallest;
    }
}

static void heap_push(zsp_timebase_t *tb, zsp_timebase_event_t *event) {
    /* Grow if needed */
    if (tb->event_count >= tb->event_capacity) {
        uint32_t new_cap = tb->event_capacity * 2;
        zsp_timebase_event_t *new_events = (zsp_timebase_event_t *)
            tb->alloc->alloc(tb->alloc, new_cap * sizeof(zsp_timebase_event_t));
        if (tb->events) {
            memcpy(new_events, tb->events, tb->event_count * sizeof(zsp_timebase_event_t));
            tb->alloc->free(tb->alloc, tb->events);
        }
        tb->events = new_events;
        tb->event_capacity = new_cap;
    }
    
    tb->events[tb->event_count] = *event;
    heap_sift_up(tb->events, tb->event_count);
    tb->event_count++;
}

static zsp_timebase_event_t heap_pop(zsp_timebase_t *tb) {
    zsp_timebase_event_t result = tb->events[0];
    tb->event_count--;
    if (tb->event_count > 0) {
        tb->events[0] = tb->events[tb->event_count];
        heap_sift_down(tb->events, tb->event_count, 0);
    }
    return result;
}

/*============================================================================
 * Ready Queue Functions
 *============================================================================*/

static void ready_queue_add(zsp_timebase_t *tb, zsp_thread_t *thread) {
    thread->flags &= ~ZSP_THREAD_FLAGS_BLOCKED;
    thread->next = NULL;
    if (tb->ready_tail) {
        tb->ready_tail->next = thread;
        tb->ready_tail = thread;
    } else {
        tb->ready_head = thread;
        tb->ready_tail = thread;
    }
}

static zsp_thread_t *ready_queue_pop(zsp_timebase_t *tb) {
    zsp_thread_t *thread = tb->ready_head;
    if (thread) {
        tb->ready_head = thread->next;
        if (!tb->ready_head) {
            tb->ready_tail = NULL;
        }
        thread->next = NULL;
    }
    return thread;
}

/*============================================================================
 * Time Conversion
 *============================================================================*/

static int get_exponent(zsp_time_unit_e unit) {
    return (unit == ZSP_TIME_S) ? 0 : (int)unit;
}

uint64_t zsp_timebase_to_ticks(zsp_timebase_t *tb, zsp_time_t time) {
    if (time.amt == 0) {
        return 0;  /* Delta time */
    }
    
    int input_exp = get_exponent((zsp_time_unit_e)time.unit);
    int res_exp = get_exponent((zsp_time_unit_e)tb->resolution);
    int diff = input_exp - res_exp;
    
    if (diff >= 0) {
        /* Input is coarser or equal - multiply */
        uint64_t mult = 1;
        for (int i = 0; i < diff; i++) mult *= 10;
        return time.amt * mult;
    } else {
        /* Input is finer - divide (with potential precision loss) */
        uint64_t div = 1;
        for (int i = 0; i < -diff; i++) div *= 10;
        return time.amt / div;
    }
}

zsp_time_t zsp_timebase_time(zsp_timebase_t *tb) {
    zsp_time_t result;
    result.amt = tb->current_time;
    result.unit = tb->resolution;
    return result;
}

uint64_t zsp_timebase_current_ticks(zsp_timebase_t *tb) {
    return tb->current_time;
}

/*============================================================================
 * Timebase Lifecycle
 *============================================================================*/

static inline uintptr_t zsp_stack_block_base0(zsp_stack_block_t *block) {
    return (uintptr_t)&block->base + sizeof(uintptr_t);
}

static zsp_stack_block_t *zsp_timebase_stack_block_alloc(
    zsp_timebase_t *tb,
    uint32_t block_sz) {

    zsp_stack_block_t **free_list = (block_sz == 4096) ? &tb->free_4k : &tb->free_8k;

    if (*free_list) {
        zsp_stack_block_t *block = *free_list;
        *free_list = block->prev;
        block->prev = NULL;
        block->base = zsp_stack_block_base0(block);
        return block;
    }

    zsp_stack_block_t *block = (zsp_stack_block_t *)
        tb->alloc->alloc(tb->alloc, sizeof(zsp_stack_block_t) + block_sz);
    block->base = zsp_stack_block_base0(block);
    block->limit = block->base + block_sz - 1;
    block->prev = NULL;

    return block;
}

static void zsp_timebase_stack_block_free(zsp_timebase_t *tb, zsp_stack_block_t *block) {
    uint32_t block_sz = (uint32_t)(block->limit - zsp_stack_block_base0(block) + 1);
    zsp_stack_block_t **free_list = (block_sz == 4096) ? &tb->free_4k : &tb->free_8k;

    block->prev = *free_list;
    *free_list = block;
}

void zsp_timebase_init(
    zsp_timebase_t      *tb,
    zsp_alloc_t         *alloc,
    zsp_time_unit_e     resolution) {
    
    tb->alloc = alloc;
    tb->current_time = 0;
    tb->resolution = resolution;
    
    tb->ready_head = NULL;
    tb->ready_tail = NULL;

    tb->free_4k = NULL;
    tb->free_8k = NULL;
    
    /* Pre-allocate event heap */
    tb->event_capacity = INITIAL_EVENT_CAPACITY;
    tb->events = (zsp_timebase_event_t *)
        alloc->alloc(alloc, tb->event_capacity * sizeof(zsp_timebase_event_t));
    tb->event_count = 0;
    tb->event_seq = 0;
    
    tb->active = 0;
    tb->running = 0;
    tb->env_p = NULL;
}

zsp_timebase_t *zsp_timebase_create(
    zsp_alloc_t         *alloc,
    zsp_time_unit_e     resolution) {
    
    zsp_timebase_t *tb = (zsp_timebase_t *)
        alloc->alloc(alloc, sizeof(zsp_timebase_t));
    zsp_timebase_init(tb, alloc, resolution);
    return tb;
}

void zsp_timebase_destroy(zsp_timebase_t *tb) {
    if (tb->events) {
        tb->alloc->free(tb->alloc, tb->events);
        tb->events = NULL;
    }

    while (tb->free_4k) {
        zsp_stack_block_t *n = tb->free_4k->prev;
        tb->alloc->free(tb->alloc, tb->free_4k);
        tb->free_4k = n;
    }

    while (tb->free_8k) {
        zsp_stack_block_t *n = tb->free_8k->prev;
        tb->alloc->free(tb->alloc, tb->free_8k);
        tb->free_8k = n;
    }
}

/*============================================================================
 * Thread Allocator (for thread-local storage)
 *============================================================================*/

static void *thread_alloc(zsp_alloc_t *alloc, size_t sz) {
    zsp_thread_t *thread = (zsp_thread_t *)(
        ((uintptr_t)alloc) - offsetof(zsp_thread_t, alloc));
    return zsp_timebase_alloca(thread, sz);
}

static void thread_init_common(
    zsp_thread_t        *thread,
    zsp_timebase_t      *tb,
    zsp_thread_flags_e  flags) {
    
    thread->exit_f = NULL;
    thread->leaf = NULL;
    thread->block = NULL;
    thread->next = NULL;
    thread->timebase = tb;
    thread->rval = 0;
    thread->flags = flags;
    
    /* Thread-local allocator uses stack blocks */
    thread->alloc.alloc = thread_alloc;
    thread->alloc.free = NULL;
}

/*============================================================================
 * Thread Management
 *============================================================================*/

static zsp_thread_t *thread_init_va(
    zsp_timebase_t      *tb,
    zsp_thread_t        *thread,
    zsp_task_func       func,
    zsp_thread_flags_e  flags,
    va_list             *args) {
    
    thread_init_common(thread, tb, flags | ZSP_THREAD_FLAGS_INITIAL);
    
    /* Call task to initialize its frame */
    zsp_frame_t *ret = func(tb, thread, 0, args);
    
    /* Only update leaf if the task returned a frame.
     * If ret is NULL but thread->leaf is not NULL, then a sub-task completed
     * during initialization but the parent frame is still valid. */
    if (ret != NULL) {
        thread->leaf = ret;
    }
    thread->flags &= ~ZSP_THREAD_FLAGS_INITIAL;
    
    if (thread->leaf) {
        if ((thread->flags & ZSP_THREAD_FLAGS_BLOCKED) == 0) {
            /* Thread is runnable - schedule it */
            if (thread->flags & ZSP_THREAD_FLAGS_SUSPEND) {
                thread->flags &= ~ZSP_THREAD_FLAGS_SUSPEND;
            }
            zsp_timebase_schedule(tb, thread);
        } else {
            /* Thread blocked */
            thread->next = NULL;
        }
    } else {
        /* Thread completed */
        thread->flags |= ZSP_THREAD_FLAGS_BLOCKED;
        thread->next = NULL;
    }
    
    return thread;
}

zsp_thread_t *zsp_timebase_thread_create(
    zsp_timebase_t      *tb,
    zsp_task_func       func,
    zsp_thread_flags_e  flags, ...) {
    
    va_list args;
    va_start(args, flags);
    
    zsp_thread_t *thread = (zsp_thread_t *)
        tb->alloc->alloc(tb->alloc, sizeof(zsp_thread_t));
    
    thread_init_va(tb, thread, func, flags, &args);
    
    va_end(args);
    return thread;
}

zsp_thread_t *zsp_timebase_thread_init(
    zsp_timebase_t      *tb,
    zsp_thread_t        *thread,
    zsp_task_func       func,
    zsp_thread_flags_e  flags, ...) {
    
    va_list args;
    va_start(args, flags);
    
    thread_init_va(tb, thread, func, flags, &args);
    
    va_end(args);
    return thread;
}

void zsp_timebase_schedule(zsp_timebase_t *tb, zsp_thread_t *thread) {
    // printf("DEBUG: Schedule thread %p\n", (void*)thread);
    tb->active++;
    thread->flags &= ~ZSP_THREAD_FLAGS_BLOCKED;
    ready_queue_add(tb, thread);
}

void zsp_timebase_schedule_at(
    zsp_timebase_t  *tb,
    zsp_thread_t    *thread,
    zsp_time_t      delay) {
    
    uint64_t delay_ticks = zsp_timebase_to_ticks(tb, delay);
    uint64_t wake_time = tb->current_time + delay_ticks;
    
    tb->active++;
    /* Do NOT clear BLOCKED flag - thread is waiting for time */
    
    zsp_timebase_event_t event;
    event.wake_time = wake_time;
    event.seq = tb->event_seq++;
    event.thread = thread;
    
    heap_push(tb, &event);
}

void zsp_timebase_thread_free(zsp_thread_t *thread) {
    zsp_timebase_t *tb = thread->timebase;
    
    /* Free stack blocks */
    while (thread->block) {
        zsp_stack_block_t *prev = thread->block->prev;
        zsp_timebase_stack_block_free(tb, thread->block);
        thread->block = prev;
    }
    
    tb->alloc->free(tb->alloc, thread);
}

zsp_timebase_t *zsp_thread_timebase(zsp_thread_t *thread) {
    return thread->timebase;
}

/*============================================================================
 * Simulation Execution
 *============================================================================*/

int zsp_timebase_run(zsp_timebase_t *tb) {
    jmp_buf env;
    zsp_thread_t *thread = ready_queue_pop(tb);

    if (!thread) {
        return 0;  /* No ready threads */
    }

    tb->active--;

    /* A cancelled thread is dropped without running (PAR-FIRST surplus). */
    if (thread->flags & ZSP_THREAD_FLAGS_CANCELLED) {
        return (tb->ready_head != NULL) ? 1 : 0;
    }

    if (thread->leaf) {
        if (setjmp(env)) {
            /* Exception - TODO: handle */
        } else {
            tb->env_p = &env;
            thread->leaf = thread->leaf->func(
                tb,
                thread,
                thread->leaf->idx,
                NULL);
        }
    }
    
    if (thread->leaf) {
        if ((thread->flags & ZSP_THREAD_FLAGS_BLOCKED) == 0) {
            /* Thread is runnable - reschedule */
            if (thread->flags & ZSP_THREAD_FLAGS_SUSPEND) {
                thread->flags &= ~ZSP_THREAD_FLAGS_SUSPEND;
            }
            zsp_timebase_schedule(tb, thread);
        } else {
            /* Thread blocked */
            thread->next = NULL;
        }
    } else {
        /* Thread completed */
        if (thread->exit_f) {
            thread->exit_f(thread);
        }
    }
    
    return (tb->ready_head != NULL) ? 1 : 0;
}

int zsp_timebase_advance(zsp_timebase_t *tb) {
    /* Drop cancelled events sitting at the top of the heap *without* advancing
     * time -- a cancelled WAIT must not influence the clock (PAR-FIRST). */
    while (tb->event_count > 0 &&
           (tb->events[0].thread->flags & ZSP_THREAD_FLAGS_CANCELLED)) {
        heap_pop(tb);
    }

    if (tb->event_count == 0) {
        return 0;  /* No timed events */
    }

    /* Get next event time */
    uint64_t next_time = tb->events[0].wake_time;
    tb->current_time = next_time;

    /* Move all events at this time to ready queue (skip any cancelled). */
    while (tb->event_count > 0 && tb->events[0].wake_time == next_time) {
        zsp_timebase_event_t event = heap_pop(tb);
        if (event.thread->flags & ZSP_THREAD_FLAGS_CANCELLED) {
            continue;
        }
        ready_queue_add(tb, event.thread);
    }

    return 1;
}

void zsp_timebase_run_until(zsp_timebase_t *tb, zsp_time_t end_time) {
    uint64_t end_ticks = zsp_timebase_to_ticks(tb, end_time);
    tb->running = 1;
    
    while (tb->running) {
        /* Run all ready threads */
        while (tb->ready_head && tb->running) {
            zsp_timebase_run(tb);
        }
        
        /* Check for timed events */
        if (tb->event_count == 0) {
            break;  /* No more events */
        }
        
        /* Check if next event is beyond end time */
        if (tb->events[0].wake_time > end_ticks) {
            break;
        }
        
        /* Advance to next time slot */
        zsp_timebase_advance(tb);
    }
    
    /* Always advance to end time */
    if (tb->current_time < end_ticks) {
        tb->current_time = end_ticks;
    }
    tb->running = 0;
}

int zsp_timebase_has_pending(zsp_timebase_t *tb) {
    return (tb->ready_head != NULL) || (tb->event_count > 0);
}

void zsp_timebase_stop(zsp_timebase_t *tb) {
    tb->running = 0;
}

void zsp_timebase_cancel(zsp_timebase_t *tb, zsp_thread_t *thread) {
    (void)tb;
    if (thread == NULL) {
        return;
    }
    /* Mark cancelled; the ready queue / timed heap drop it lazily (see
     * zsp_timebase_run / zsp_timebase_advance). */
    thread->flags |= ZSP_THREAD_FLAGS_CANCELLED;
}

/*============================================================================
 * Thread Stack/Frame Management
 *============================================================================*/

zsp_frame_t *zsp_timebase_alloc_frame(
    zsp_thread_t    *thread,
    uint32_t        sz,
    zsp_task_func   func) {
    
    zsp_timebase_t *tb = thread->timebase;
    zsp_frame_t *ret;
    
    uint32_t total_sz = sizeof(zsp_frame_t) + sz;
    
    /* Allocate new block if needed */
    if (!thread->block || (thread->block->base + total_sz) >= thread->block->limit) {
        zsp_stack_block_t *block = zsp_timebase_stack_block_alloc(tb, 8192);
        block->prev = thread->block;
        thread->block = block;
    }
    
    ret = (zsp_frame_t *)thread->block->base;
    thread->block->base += total_sz;
    
    ret->func = func;
    ret->prev = thread->leaf;
    ret->idx = 0;
    thread->leaf = ret;
    
    return ret;
}

void *zsp_timebase_alloca(zsp_thread_t *thread, size_t sz) {
    zsp_timebase_t *tb = thread->timebase;
    void *ret;
    
    uint32_t total_sz = (uint32_t)sz;
    
    if (!thread->block || (thread->block->base + total_sz) >= thread->block->limit) {
        zsp_stack_block_t *block = zsp_timebase_stack_block_alloc(tb, 4096);
        block->prev = thread->block;
        thread->block = block;
    }
    
    ret = (void *)thread->block->base;
    thread->block->base += total_sz;
    
    return ret;
}

void zsp_timebase_yield(zsp_thread_t *thread) {
    thread->flags |= ZSP_THREAD_FLAGS_SUSPEND;
}

int zsp_timebase_wait(zsp_thread_t *thread, zsp_time_t delay) {
    zsp_timebase_t *tb = thread->timebase;
    uint64_t delay_ticks = zsp_timebase_to_ticks(tb, delay);
    uint64_t target_time = tb->current_time + delay_ticks;
    
    /* Optimization: If no other threads are pending in [now..target_time],
     * we can simply advance time without suspending.
     * Never use fast-path during initialization or when simulation is running. */
    int is_initializing = (thread->flags & ZSP_THREAD_FLAGS_INITIAL) != 0;
    int has_ready = (tb->ready_head != NULL);
    int has_earlier_events = (tb->event_count > 0 && tb->events[0].wake_time <= target_time);
    
    if (!is_initializing && !tb->running && !has_ready && !has_earlier_events) {
        /* Fast path: advance time directly without suspension.
         * Set SUSPEND flag to prevent recursive continuation in zsp_timebase_return */
        tb->current_time = target_time;
        thread->flags |= ZSP_THREAD_FLAGS_SUSPEND;
        return 0;  /* No suspension needed */
    }
    
    /* Slow path: other threads pending, must suspend and schedule */
    thread->flags |= ZSP_THREAD_FLAGS_BLOCKED;
    thread->flags &= ~ZSP_THREAD_FLAGS_SUSPEND;
    zsp_timebase_schedule_at(tb, thread, delay);
    return 1;  /* Thread suspended */
}

int zsp_timebase_wait_ps(zsp_thread_t *thread, uint64_t delay_ps) {
    return zsp_timebase_wait(thread, ZSP_TIME_PS(delay_ps));
}

zsp_frame_t *zsp_timebase_return(zsp_thread_t *thread, uintptr_t rval) {
    zsp_timebase_t *tb = thread->timebase;
    zsp_frame_t *ret = thread->leaf;
    uintptr_t frame_v = (uintptr_t)ret;
    thread->rval = rval;
    
    /* Free stack blocks until we find the one containing the frame */
    while (thread->block) {
        if (frame_v >= (uintptr_t)thread->block && frame_v <= thread->block->limit) {
            break;
        } else {
            zsp_stack_block_t *prev = thread->block->prev;
            zsp_timebase_stack_block_free(tb, thread->block);
            thread->block = prev;
        }
    }
    
    /* Roll back base pointer */
    if (thread->block) {
        thread->block->base = (uintptr_t)ret;
    }
    
    if (ret) {
        zsp_frame_t *prev = ret->prev;
        thread->leaf = prev;
        ret = prev;
        
        if (thread->flags & ZSP_THREAD_FLAGS_INITIAL) {
            ret = NULL;
        } else if (prev && !(thread->flags & ZSP_THREAD_FLAGS_BLOCKED)) {
            thread->flags &= ~ZSP_THREAD_FLAGS_SUSPEND;
            ret = prev->func(tb, thread, prev->idx, NULL);
            thread->leaf = ret;
        }
    }
    
    return ret;
}

zsp_frame_t *zsp_timebase_call(zsp_thread_t *thread, zsp_task_func func, ...) {
    va_list args;
    va_start(args, func);
    
    zsp_timebase_t *tb = thread->timebase;
    zsp_frame_t *ret = func(tb, thread, 0, &args);
    
    va_end(args);
    return ret;
}

uintptr_t zsp_timebase_va_arg(va_list *args, size_t sz) {
    uintptr_t ret;
    switch (sz) {
        case 1: ret = (uintptr_t)va_arg(*args, int); break;  /* promoted to int */
        case 2: ret = (uintptr_t)va_arg(*args, int); break;  /* promoted to int */
        case 4: ret = (uintptr_t)va_arg(*args, uint32_t); break;
        case 8: ret = (uintptr_t)va_arg(*args, uint64_t); break;
        default:
            fprintf(stderr, "Unsupported size for va_arg: %zu\n", sz);
            ret = 0;
            break;
    }
    return ret;
}
