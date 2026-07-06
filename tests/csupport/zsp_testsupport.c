/*
 * zsp_testsupport.c -- ctypes test shim for the rt-core substrate.
 *
 * Compiled *into* the substrate test .so so the Python (ctypes) side can drive
 * and inspect the scheduler through opaque pointers + small accessor functions,
 * never needing to mirror the C struct layouts. This keeps the tests robust
 * against field reordering: only this file (which #includes the real headers)
 * knows the layout.
 *
 * The bare-thread factory lets us exercise the ready-queue and timed-event heap
 * WITHOUT real setjmp coroutines: a thread with leaf == NULL is popped by
 * zsp_timebase_run, counted, and "completes" immediately (firing exit_f). We
 * record run order via exit_f, reading each thread's tag back out.
 */

#include <stdlib.h>
#include <string.h>
#include "zsp_timebase.h"
#include "zsp_indexed_pool.h"

/* --- allocator ---------------------------------------------------------- */

zsp_alloc_t *zspt_malloc_alloc(void) {
    return zsp_alloc_malloc_create();
}

void *zspt_alloc_call(zsp_alloc_t *a, size_t sz) {
    return a->alloc(a, sz);
}

void zspt_alloc_free(zsp_alloc_t *a, void *p) {
    a->free(a, p);
}

/* --- bare thread factory (no coroutine) --------------------------------- */

/* A thread with leaf == NULL: run() pops it, decrements active, and takes the
 * "completed" branch (calling exit_f if set). `tag` is stashed in `rval` so the
 * exit callback can identify which thread ran. */
zsp_thread_t *zspt_thread_new(zsp_timebase_t *tb, uintptr_t tag) {
    zsp_thread_t *t = (zsp_thread_t *)malloc(sizeof(zsp_thread_t));
    memset(t, 0, sizeof(*t));
    t->timebase = tb;
    t->leaf = NULL;
    t->flags = ZSP_THREAD_FLAGS_NONE;
    t->rval = tag;
    return t;
}

void zspt_thread_free(zsp_thread_t *t) {
    free(t);
}

uintptr_t zspt_thread_tag(zsp_thread_t *t) {
    return t->rval;
}

void zspt_thread_set_exit(zsp_thread_t *t, zsp_thread_exit_f f) {
    t->exit_f = f;
}

/* --- zsp_time_t helpers (keep struct-by-value off the ctypes boundary) --- */

/* zsp_time_t is a by-value {uint64 amt; int32 unit} aggregate. Rather than rely
 * on ctypes reproducing the struct-passing ABI, the shim takes the fields as
 * plain scalars and builds the struct on the C side. */
uint64_t zspt_to_ticks(zsp_timebase_t *tb, uint64_t amt, int32_t unit) {
    zsp_time_t t;
    t.amt = amt;
    t.unit = unit;
    return zsp_timebase_to_ticks(tb, t);
}

void zspt_schedule_at(zsp_timebase_t *tb, zsp_thread_t *th,
                      uint64_t amt, int32_t unit) {
    zsp_time_t t;
    t.amt = amt;
    t.unit = unit;
    zsp_timebase_schedule_at(tb, th, t);
}

/* --- timebase accessors (opaque on the Python side) --------------------- */

uint64_t zspt_tb_current(zsp_timebase_t *tb)      { return tb->current_time; }
uint32_t zspt_tb_event_count(zsp_timebase_t *tb)  { return tb->event_count; }
uint32_t zspt_tb_event_capacity(zsp_timebase_t *tb) { return tb->event_capacity; }
int32_t  zspt_tb_active(zsp_timebase_t *tb)       { return tb->active; }
int      zspt_tb_ready_empty(zsp_timebase_t *tb)  { return tb->ready_head == NULL; }
int32_t  zspt_tb_resolution(zsp_timebase_t *tb)   { return tb->resolution; }

/* --- coroutine-driven scheduler test ------------------------------------ *
 *
 * Real zsp_task_func coroutines exercise the frame/stack machinery
 * (alloc_frame / wait / return, idx-based resume, locals across suspends) that
 * the ready-queue/heap tests bypass. Each coroutine records (tag, completion
 * time) into a shared log so the Python side can assert ordering and timing
 * without knowing any struct layout.
 */

typedef struct {
    uint32_t n;
    uint64_t tag[32];
    uint64_t time[32];
} zspt_log_t;

zspt_log_t *zspt_log_new(void)                    { return (zspt_log_t *)calloc(1, sizeof(zspt_log_t)); }
void        zspt_log_free(zspt_log_t *l)          { free(l); }
uint32_t    zspt_log_count(zspt_log_t *l)         { return l->n; }
uint64_t    zspt_log_tag(zspt_log_t *l, uint32_t i)  { return l->tag[i]; }
uint64_t    zspt_log_time(zspt_log_t *l, uint32_t i) { return l->time[i]; }

static void log_put(zspt_log_t *l, uint64_t tag, uint64_t time) {
    if (l->n < 32) {
        l->tag[l->n] = tag;
        l->time[l->n] = time;
        l->n++;
    }
}

/* Single-wait coroutine: wait `delay` ps, then log (tag, now) and finish. */
typedef struct {
    zspt_log_t *log;
    uint64_t    tag;
    uint64_t    delay;
} waiter_locals_t;

static zsp_frame_t *waiter_task(
    zsp_timebase_t *tb, zsp_thread_t *thread, int idx, va_list *args) {
    zsp_frame_t *ret = thread->leaf;
    waiter_locals_t *L;
    switch (idx) {
    case 0:
        ret = zsp_timebase_alloc_frame(thread, sizeof(waiter_locals_t), &waiter_task);
        L = zsp_frame_locals(ret, waiter_locals_t);
        L->log   = va_arg(*args, zspt_log_t *);
        L->tag   = va_arg(*args, uint64_t);
        L->delay = va_arg(*args, uint64_t);
        ret->idx = 1;
        return ret;
    case 1:
        L = zsp_frame_locals(ret, waiter_locals_t);
        ret->idx = 2;
        if (zsp_timebase_wait(thread, ZSP_TIME_PS(L->delay))) {
            return ret;                 /* suspended -> resume at idx 2 */
        }
        /* fast path: time advanced inline, fall through */
        /* FALLTHROUGH */
    case 2:
        L = zsp_frame_locals(ret, waiter_locals_t);
        log_put(L->log, L->tag, zsp_timebase_current_ticks(tb));
        return zsp_timebase_return(thread, 0);
    }
    return ret;
}

zsp_thread_t *zspt_spawn_waiter(
    zsp_timebase_t *tb, zspt_log_t *log, uint64_t tag, uint64_t delay) {
    return zsp_timebase_thread_create(
        tb, &waiter_task, ZSP_THREAD_FLAGS_NONE, log, tag, delay);
}

/* Two-wait coroutine: wait d1 (log tag*10+1), wait d2 (log tag*10+2), finish.
 * Proves locals survive across two suspends and idx advances correctly. */
typedef struct {
    zspt_log_t *log;
    uint64_t    tag;
    uint64_t    d1;
    uint64_t    d2;
} double_locals_t;

static zsp_frame_t *double_task(
    zsp_timebase_t *tb, zsp_thread_t *thread, int idx, va_list *args) {
    zsp_frame_t *ret = thread->leaf;
    double_locals_t *L;
    switch (idx) {
    case 0:
        ret = zsp_timebase_alloc_frame(thread, sizeof(double_locals_t), &double_task);
        L = zsp_frame_locals(ret, double_locals_t);
        L->log = va_arg(*args, zspt_log_t *);
        L->tag = va_arg(*args, uint64_t);
        L->d1  = va_arg(*args, uint64_t);
        L->d2  = va_arg(*args, uint64_t);
        ret->idx = 1;
        return ret;
    case 1:
        L = zsp_frame_locals(ret, double_locals_t);
        ret->idx = 2;
        if (zsp_timebase_wait(thread, ZSP_TIME_PS(L->d1))) return ret;
        /* FALLTHROUGH */
    case 2:
        L = zsp_frame_locals(ret, double_locals_t);
        log_put(L->log, L->tag * 10 + 1, zsp_timebase_current_ticks(tb));
        ret->idx = 3;
        if (zsp_timebase_wait(thread, ZSP_TIME_PS(L->d2))) return ret;
        /* FALLTHROUGH */
    case 3:
        L = zsp_frame_locals(ret, double_locals_t);
        log_put(L->log, L->tag * 10 + 2, zsp_timebase_current_ticks(tb));
        return zsp_timebase_return(thread, 0);
    }
    return ret;
}

zsp_thread_t *zspt_spawn_double(
    zsp_timebase_t *tb, zspt_log_t *log, uint64_t tag, uint64_t d1, uint64_t d2) {
    return zsp_timebase_thread_create(
        tb, &double_task, ZSP_THREAD_FLAGS_NONE, log, tag, d1, d2);
}

/* Drive the sim to quiescence: run all ready threads, advance to the next timed
 * batch, repeat until neither remains. */
void zspt_run_all(zsp_timebase_t *tb) {
    for (;;) {
        while (tb->ready_head) {
            zsp_timebase_run(tb);
        }
        if (!zsp_timebase_advance(tb)) {
            break;
        }
    }
}

/* --- indexed pool (opaque handle + a used-bit accessor) ----------------- */

zsp_indexed_pool_t *zspt_pool_new(uint32_t size) {
    zsp_indexed_pool_t *p = (zsp_indexed_pool_t *)malloc(sizeof(zsp_indexed_pool_t));
    zsp_indexed_pool_init(p, size);
    return p;
}

void zspt_pool_free(zsp_indexed_pool_t *p) {
    free(p->used_mask);
    free(p);
}

/* Mirror the .c's private used-bit check so tests can assert slot state. */
int zspt_pool_is_used(zsp_indexed_pool_t *p, int idx) {
    return (p->used_mask[idx / 32] >> (idx % 32)) & 1u;
}

uint32_t zspt_pool_seed(zsp_indexed_pool_t *p)              { return p->seed; }
void     zspt_pool_set_seed(zsp_indexed_pool_t *p, uint32_t s) { p->seed = s; }

/* --- interpreter-as-coroutine feasibility spike (P3 core mechanic) ------- *
 *
 * Proves an *interpreted* coroutine plugs into the migrated scheduler exactly
 * like a compiled one:
 *   - the coroutine entrypoint is an interp-defined task func (zbc_interp_task);
 *   - its bytecode is conveyed through the thread-create args (read at idx 0);
 *   - its operand/variable stack lives IN the coroutine frame (the zsp_timebase
 *     stack-block arena), so interpreted and compiled coroutines share one stack
 *     discipline and interleave under the same scheduler.
 *
 * Suspend/resume uses the same stackless protocol as a compiled task func: save
 * pc (it is a frame local), call zsp_timebase_wait, return the leaf; resume
 * re-enters the dispatch loop at the saved pc. A toy ISA keeps it minimal. */

#define TOY_PUSH   1   /* arg -> push immediate onto the in-frame stack */
#define TOY_ADD    2   /* pop b, pop a, push a+b */
#define TOY_WAIT   3   /* arg -> wait arg ps (a suspend point) */
#define TOY_LOGACC 4   /* log (top-of-stack, now) */
#define TOY_HALT   5

typedef struct { uint32_t op; uint64_t arg; } toy_instr_t;
typedef struct { toy_instr_t ins[64]; uint32_t n; } toy_prog_t;

toy_prog_t *zspt_prog_new(void)      { return (toy_prog_t *)calloc(1, sizeof(toy_prog_t)); }
void        zspt_prog_free(toy_prog_t *p) { free(p); }

static void prog_add(toy_prog_t *p, uint32_t op, uint64_t arg) {
    if (p->n < 64) { p->ins[p->n].op = op; p->ins[p->n].arg = arg; p->n++; }
}
void zspt_prog_push(toy_prog_t *p, uint64_t v) { prog_add(p, TOY_PUSH, v); }
void zspt_prog_add(toy_prog_t *p)              { prog_add(p, TOY_ADD, 0); }
void zspt_prog_wait(toy_prog_t *p, uint64_t d) { prog_add(p, TOY_WAIT, d); }
void zspt_prog_logacc(toy_prog_t *p)           { prog_add(p, TOY_LOGACC, 0); }
void zspt_prog_halt(toy_prog_t *p)             { prog_add(p, TOY_HALT, 0); }

/* The interpreter frame: pc + operand stack, all in the coroutine frame. */
#define TOY_STACK_DEPTH 16
typedef struct {
    toy_prog_t *prog;
    zspt_log_t *log;
    uint64_t    tag;
    uint32_t    pc;
    uint32_t    sp;                         /* next-free index */
    uint64_t    stack[TOY_STACK_DEPTH];     /* interp variable stack, in-frame */
} interp_locals_t;

static zsp_frame_t *zbc_interp_task(
    zsp_timebase_t *tb, zsp_thread_t *thread, int idx, va_list *args) {
    zsp_frame_t *ret = thread->leaf;
    interp_locals_t *L;

    if (idx == 0) {
        /* Frame + variable stack are allocated on the coroutine's stack arena. */
        ret = zsp_timebase_alloc_frame(thread, sizeof(interp_locals_t), &zbc_interp_task);
        L = zsp_frame_locals(ret, interp_locals_t);
        L->prog = va_arg(*args, toy_prog_t *);
        L->log  = va_arg(*args, zspt_log_t *);
        L->tag  = va_arg(*args, uint64_t);
        L->pc = 0;
        L->sp = 0;
        ret->idx = 1;
        return ret;
    }

    L = zsp_frame_locals(ret, interp_locals_t);
    for (;;) {
        toy_instr_t *in = &L->prog->ins[L->pc];
        switch (in->op) {
        case TOY_PUSH:
            L->stack[L->sp++] = in->arg;
            L->pc++;
            break;
        case TOY_ADD: {
            uint64_t b = L->stack[--L->sp];
            uint64_t a = L->stack[--L->sp];
            L->stack[L->sp++] = a + b;
            L->pc++;
            break;
        }
        case TOY_WAIT:
            L->pc++;                        /* advance past WAIT before suspending */
            if (zsp_timebase_wait(thread, ZSP_TIME_PS(in->arg))) {
                return ret;                 /* suspend; resume re-enters at new pc */
            }
            break;                          /* fast path: time advanced inline */
        case TOY_LOGACC:
            log_put(L->log, (L->sp ? L->stack[L->sp - 1] : 0),
                    zsp_timebase_current_ticks(tb));
            L->pc++;
            break;
        case TOY_HALT:
        default:
            return zsp_timebase_return(thread, (L->sp ? L->stack[L->sp - 1] : 0));
        }
    }
}

zsp_thread_t *zspt_spawn_interp(
    zsp_timebase_t *tb, toy_prog_t *prog, zspt_log_t *log, uint64_t tag) {
    return zsp_timebase_thread_create(
        tb, &zbc_interp_task, ZSP_THREAD_FLAGS_NONE, prog, log, tag);
}
