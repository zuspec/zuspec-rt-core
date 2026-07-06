#ifndef INCLUDED_ZSP_TIMEBASE_H
#define INCLUDED_ZSP_TIMEBASE_H

#include <setjmp.h>
#include <stdint.h>
#include <stdarg.h>
#include "zsp_alloc.h"

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================
 * Time Types and Constants
 *============================================================================*/

/**
 * Time unit enumeration matching Python TimeUnit values.
 * Values represent exponents (e.g., NS = -9 means 10^-9 seconds)
 */
typedef enum {
    ZSP_TIME_S  =   1,   /* Seconds (special case: 10^0) */
    ZSP_TIME_MS =  -3,   /* Milliseconds (10^-3) */
    ZSP_TIME_US =  -6,   /* Microseconds (10^-6) */
    ZSP_TIME_NS =  -9,   /* Nanoseconds (10^-9) */
    ZSP_TIME_PS = -12,   /* Picoseconds (10^-12) */
    ZSP_TIME_FS = -15    /* Femtoseconds (10^-15) */
} zsp_time_unit_e;

/**
 * Time value with unit - can be passed by value.
 */
typedef struct zsp_time_s {
    uint64_t        amt;    /* Time amount */
    int32_t         unit;   /* Time unit (zsp_time_unit_e) */
} zsp_time_t;

/* Time construction helpers */
#define ZSP_TIME_DELTA  ((zsp_time_t){0, ZSP_TIME_S})
#define ZSP_TIME_S(v)   ((zsp_time_t){(v), ZSP_TIME_S})
#define ZSP_TIME_MS(v)  ((zsp_time_t){(v), ZSP_TIME_MS})
#define ZSP_TIME_US(v)  ((zsp_time_t){(v), ZSP_TIME_US})
#define ZSP_TIME_NS(v)  ((zsp_time_t){(v), ZSP_TIME_NS})
#define ZSP_TIME_PS(v)  ((zsp_time_t){(v), ZSP_TIME_PS})
#define ZSP_TIME_FS(v)  ((zsp_time_t){(v), ZSP_TIME_FS})

/*============================================================================
 * Thread/Coroutine Types (formerly in zsp_thread.h)
 *============================================================================*/

#define ZSP_STACK_FRAME_SZ 4096

struct zsp_thread_s;
struct zsp_timebase_s;

typedef struct zsp_frame_s *(*zsp_task_func)(
    struct zsp_timebase_s *,  /* timebase - direct parameter to avoid pointer chase */
    struct zsp_thread_s *, 
    int idx, 
    va_list *args);

typedef enum {
    ZSP_THREAD_FLAGS_NONE    = 0,
    ZSP_THREAD_FLAGS_INITIAL = (1 << 0),
    ZSP_THREAD_FLAGS_SUSPEND = (1 << 1),
    ZSP_THREAD_FLAGS_BLOCKED = (1 << 2),
    /* Cancelled: the scheduler drops the thread from the ready queue and the
     * timed-event heap without running it or advancing time (PAR-FIRST surplus).
     * rt-core extension beyond the be-sw AOT copy; reconcile at the migration
     * cutover. */
    ZSP_THREAD_FLAGS_CANCELLED = (1 << 3)
} zsp_thread_flags_e;

typedef struct zsp_frame_s {
    zsp_task_func       func;
    struct zsp_frame_s  *prev;
    int32_t             idx;
} zsp_frame_t;

typedef struct zsp_frame_wrap_s {
    zsp_frame_t         frame;
    uintptr_t           locals;
} zsp_frame_wrap_t;

#define zsp_frame_size(t) \
    sizeof(struct {zsp_frame_t __f; t __locals;})

#define zsp_frame_locals(frame, locals_t) \
    ((locals_t *)&((zsp_frame_wrap_t *)(frame))->locals)

typedef struct zsp_stack_block_s {
    struct zsp_stack_block_s    *prev;
    uintptr_t                   limit;
    uintptr_t                   base;
} zsp_stack_block_t;

typedef void (*zsp_thread_exit_f)(struct zsp_thread_s *);

typedef struct zsp_thread_s {
    zsp_thread_exit_f           exit_f;
    struct zsp_frame_s          *leaf;
    
    zsp_alloc_t                 alloc;          /* Thread-local allocator */
    zsp_stack_block_t           *block;         /* Stack blocks */
    
    struct zsp_timebase_s       *timebase;      /* Owning timebase */
    struct zsp_thread_s         *next;          /* Next in queue */
    
    uintptr_t                   rval;           /* Return value */
    zsp_thread_flags_e          flags;
    uint64_t                    local_time_ps;  /* LT mode: accumulated time not yet committed */
} zsp_thread_t;

#define zsp_thread_clear_flags(thread, f) \
    ((zsp_thread_t *)(thread))->flags &= ~(f)

/*============================================================================
 * Timebase Event Queue
 *============================================================================*/

/**
 * Event entry in the timebase queue.
 * Uses a min-heap for O(log n) insert and O(1) peek.
 */
typedef struct zsp_timebase_event_s {
    uint64_t            wake_time;  /* Absolute wake time in resolution units */
    uint32_t            seq;        /* Sequence number for stable ordering */
    zsp_thread_t        *thread;    /* Thread to wake */
} zsp_timebase_event_t;

/*============================================================================
 * Timebase Structure (replaces scheduler)
 *============================================================================*/

/**
 * Timebase structure - unified time-aware thread scheduler.
 * 
 * Replaces the previous zsp_scheduler_t, adding time management.
 * Time is stored as a single integer in the resolution unit for efficiency.
 * Uses a min-heap event queue for time-ordered thread wake-up.
 */
typedef struct zsp_timebase_s {
    zsp_alloc_t             *alloc;         /* Memory allocator */
    
    /* Current time state */
    uint64_t                current_time;   /* Current time in resolution units */
    int32_t                 resolution;     /* Resolution unit (zsp_time_unit_e) */
    
    /* Ready queue - threads ready to run at current time */
    zsp_thread_t            *ready_head;
    zsp_thread_t            *ready_tail;
    
    /* Timed event queue - min-heap of threads waiting for future times */
    zsp_timebase_event_t    *events;        /* Event array (heap) */
    uint32_t                event_count;    /* Number of events in queue */
    uint32_t                event_capacity; /* Allocated capacity */
    uint32_t                event_seq;      /* Sequence counter for stable sort */

    /* Stack-block caches to reduce alloc/free churn (4096/8192 payload blocks) */
    zsp_stack_block_t       *free_4k;
    zsp_stack_block_t       *free_8k;
    
    int32_t                 active;         /* Number of active threads */
    int                     running;        /* Non-zero if simulation running */
    jmp_buf                 *env_p;         /* For exception handling */
} zsp_timebase_t;

/*============================================================================
 * Timebase Functions
 *============================================================================*/

/**
 * Initialize a timebase with specified resolution.
 * 
 * @param tb         Timebase to initialize
 * @param alloc      Memory allocator
 * @param resolution Time resolution unit (e.g., ZSP_TIME_PS for picoseconds)
 */
void zsp_timebase_init(
    zsp_timebase_t      *tb,
    zsp_alloc_t         *alloc,
    zsp_time_unit_e     resolution);

/**
 * Create and initialize a timebase.
 */
zsp_timebase_t *zsp_timebase_create(
    zsp_alloc_t         *alloc,
    zsp_time_unit_e     resolution);

/**
 * Free timebase resources.
 */
void zsp_timebase_destroy(zsp_timebase_t *tb);

/**
 * Convert time value to internal resolution units (ticks).
 */
uint64_t zsp_timebase_to_ticks(zsp_timebase_t *tb, zsp_time_t time);

/**
 * Get current simulation time as zsp_time_t.
 */
zsp_time_t zsp_timebase_time(zsp_timebase_t *tb);

/**
 * Get current time in resolution units (ticks).
 */
uint64_t zsp_timebase_current_ticks(zsp_timebase_t *tb);

/*============================================================================
 * Thread Management Functions (replacing zsp_scheduler/zsp_thread functions)
 *============================================================================*/

/**
 * Create a new thread and schedule it.
 */
zsp_thread_t *zsp_timebase_thread_create(
    zsp_timebase_t      *tb,
    zsp_task_func       func,
    zsp_thread_flags_e  flags, ...);

/**
 * Initialize a thread (caller provides storage).
 */
zsp_thread_t *zsp_timebase_thread_init(
    zsp_timebase_t      *tb,
    zsp_thread_t        *thread,
    zsp_task_func       func,
    zsp_thread_flags_e  flags, ...);

/**
 * Schedule a thread to run at current time.
 */
void zsp_timebase_schedule(zsp_timebase_t *tb, zsp_thread_t *thread);

/**
 * Schedule a thread to wake after a delay.
 */
void zsp_timebase_schedule_at(
    zsp_timebase_t  *tb,
    zsp_thread_t    *thread,
    zsp_time_t      delay);

/**
 * Run one ready thread. Returns 1 if more threads pending, 0 otherwise.
 */
int zsp_timebase_run(zsp_timebase_t *tb);

/**
 * Run simulation until the specified time or no more events.
 */
void zsp_timebase_run_until(zsp_timebase_t *tb, zsp_time_t end_time);

/**
 * Advance to next time slot (if no ready threads).
 * Moves threads from timed queue to ready queue.
 * Returns 1 if advanced, 0 if no events.
 */
int zsp_timebase_advance(zsp_timebase_t *tb);

/**
 * Check if there are pending events or ready threads.
 */
int zsp_timebase_has_pending(zsp_timebase_t *tb);

/**
 * Stop the simulation loop.
 */
void zsp_timebase_stop(zsp_timebase_t *tb);

/**
 * Cancel a thread: it is dropped from the ready queue / timed-event heap without
 * running or advancing time. A no-op if already done or cancelled.
 */
void zsp_timebase_cancel(zsp_timebase_t *tb, zsp_thread_t *thread);

/*============================================================================
 * Thread Utility Functions
 *============================================================================*/

/**
 * Allocate a stack frame for a thread.
 */
zsp_frame_t *zsp_timebase_alloc_frame(
    zsp_thread_t    *thread,
    uint32_t        sz,
    zsp_task_func   func);

/**
 * Allocate thread-local storage.
 */
void *zsp_timebase_alloca(zsp_thread_t *thread, size_t sz);

/**
 * Yield execution (reschedule at current time).
 */
void zsp_timebase_yield(zsp_thread_t *thread);

/**
 * Wait for specified delay (reschedule at future time).
 * Returns 1 if thread was suspended (needs yield), 0 if time was advanced inline.
 */
int zsp_timebase_wait(zsp_thread_t *thread, zsp_time_t delay);

/**
 * Wait for a raw picosecond delay (hot-path helper, avoids zsp_time_t construction).
 * In precise mode: equivalent to zsp_timebase_wait(thread, ZSP_TIME_PS(delay_ps)).
 * In LT mode: generated code uses ZSP_WAIT_PS instead of calling this directly.
 */
int zsp_timebase_wait_ps(zsp_thread_t *thread, uint64_t delay_ps);

/**
 * Return from current frame.
 */
zsp_frame_t *zsp_timebase_return(zsp_thread_t *thread, uintptr_t ret);

/**
 * Call a subtask.
 */
zsp_frame_t *zsp_timebase_call(zsp_thread_t *thread, zsp_task_func func, ...);

/**
 * Free thread resources.
 */
void zsp_timebase_thread_free(zsp_thread_t *thread);

/**
 * Get thread's timebase.
 */
zsp_timebase_t *zsp_thread_timebase(zsp_thread_t *thread);

/**
 * Extract va_arg of given size.
 */
uintptr_t zsp_timebase_va_arg(va_list *args, size_t sz);

/*============================================================================
 * Task Macros
 *============================================================================*/

#define zsp_task_head_begin(name) \
    zsp_frame_t *ret = thread->leaf; \
    zsp_task_func func = &name; \
    struct __locals_s {

#define zsp_task_head_end \
    } *__locals = zsp_frame_locals(ret, struct __locals_s);

#define zsp_task_yield zsp_timebase_yield(thread); break

/*============================================================================
 * LT-mode Wait Macro
 *
 * ZSP_WAIT_PS(thread, delay_ps) — used by generated coroutine code.
 *
 * In LT mode (-DZSP_LT_MODE): accumulate into local_time_ps; yield only
 * when the quantum is exceeded.  ZSP_LT_QUANTUM_PS must be defined before
 * including this header in LT builds.
 *
 * In precise mode (default): call zsp_timebase_wait_ps() directly, which
 * suspends the thread for exactly delay_ps picoseconds.
 *============================================================================*/
#ifdef ZSP_LT_MODE
#  ifndef ZSP_LT_QUANTUM_PS
#    error "ZSP_LT_MODE requires ZSP_LT_QUANTUM_PS to be defined"
#  endif
#  define ZSP_WAIT_PS(thread, delay_ps)                                     \
     do {                                                                    \
         (thread)->local_time_ps += (uint64_t)(delay_ps);                   \
         if ((thread)->local_time_ps >= (uint64_t)ZSP_LT_QUANTUM_PS) {      \
             zsp_timebase_wait_ps((thread), (thread)->local_time_ps);        \
             (thread)->local_time_ps = 0;                                    \
         }                                                                   \
     } while (0)
#else
#  define ZSP_WAIT_PS(thread, delay_ps) \
     zsp_timebase_wait_ps((thread), (uint64_t)(delay_ps))
#endif

#ifdef __cplusplus
}
#endif

#endif /* INCLUDED_ZSP_TIMEBASE_H */
