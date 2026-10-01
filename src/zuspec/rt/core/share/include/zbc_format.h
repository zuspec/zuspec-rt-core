/*
 * zbc_format.h -- the .zbc container format, C view.
 *
 * GENERATED from zuspec.be.bc.format.spec -- DO NOT EDIT BY HAND.
 * Regenerate with:  python -m zuspec.be.bc.format.emit_c
 * (A regen no-diff test guards this file against drift.)
 */

#ifndef ZUSPEC_ZBC_FORMAT_H
#define ZUSPEC_ZBC_FORMAT_H

#include <stdint.h>

#if defined(__cplusplus)
#  define ZBC_STATIC_ASSERT(cond, msg) static_assert(cond, msg)
#else
#  define ZBC_STATIC_ASSERT(cond, msg) _Static_assert(cond, msg)
#endif

/* File identity */
#define ZBC_MAGIC0 0x5a
#define ZBC_MAGIC1 0x42
#define ZBC_MAGIC2 0x43
#define ZBC_MAGIC3 0x1a
#define ZBC_VERSION_MAJOR 1
#define ZBC_VERSION_MINOR 0

/* zbc_sec_kind: Section kinds. Readers skip unknown kinds (forward-compat). */
#define ZBC_SEC_CODE   0x0001  /* opcode stream */
#define ZBC_SEC_CORO   0x0002  /* coroutine/func descriptors */
#define ZBC_SEC_TYPE   0x0003  /* type/field metadata */
#define ZBC_SEC_CONST  0x0004  /* >64-bit literal constant pool */
#define ZBC_SEC_SOLVE  0x0005  /* SolveProblem blobs (P4) */
#define ZBC_SEC_STRB   0x0006  /* string blob */
#define ZBC_SEC_STRO   0x0007  /* string offsets */
#define ZBC_SEC_FILE   0x0008  /* file table */
#define ZBC_SEC_PROV   0x0009  /* provenance records */
#define ZBC_SEC_CMNT   0x000a  /* comment records */
#define ZBC_SEC_LINE   0x000b  /* pc->src_ref line table */
#define ZBC_SEC_BLOCK  0x000c  /* FSM block table (zbc_block[]) */
#define ZBC_SEC_OPLIST 0x000d  /* u32 operand lists (PAR/SELECT branch ids) */
#define ZBC_SEC_SELECT 0x000e  /* weighted-SELECT descriptors (zbc_select[]) */
#define ZBC_SEC_SPROB  0x000f  /* relocatable dv-solve SolveProblem blobs */

/* zbc_hdr_flags: Header flag bits. */
#define ZBC_HDR_HAS_PROV        0x0001  /* provenance sections present */
#define ZBC_HDR_PROFILE_RUNTIME 0x0002  /* runtime profile (provenance stripped) */
#define ZBC_HDR_COMP_INIT       0x0004  /* a coroutine constructs the component tree before the entry (P1.5) */

/* zbc_secf_flags: Per-section flag bits. */
#define ZBC_SECF_COMPRESSED 0x0001  /* payload is compressed */

/* zbc_prov_flags: Provenance granularity + attribute bits. */
#define ZBC_PROV_G_INSN  0x0001  /* granularity: instruction */
#define ZBC_PROV_G_BLOCK 0x0002  /* granularity: block */
#define ZBC_PROV_G_CORO  0x0004  /* granularity: coroutine */
#define ZBC_PROV_G_DECL  0x0008  /* granularity: declaration */
#define ZBC_PROV_F_SYNTH 0x0100  /* compiler-synthesized; no real source */

/* zbc_sel_flags: SELECT descriptor flag bits. */
#define ZBC_SEL_ALLOW_NONE 0x0001  /* no eligible branch -> run nothing (not an error) */

/* zbc_solve_flags: SOLVE descriptor flag bits. */
#define ZBC_SOLVE_SEED_FIXED 0x0001  /* use seed_value; else draw from the frame stream */

/* zbc_cmt_kind: Comment kinds. */
#define ZBC_CMT_LEADING  0x0001
#define ZBC_CMT_TRAILING 0x0002
#define ZBC_CMT_INLINE   0x0003
#define ZBC_CMT_DOC      0x0004

/* zbc_header: File header at offset 0. Magic is byte-order-fixed and checked first. */
typedef struct {
    uint8_t    magic[4];  /* 'Z','B','C',0x1A */
    uint16_t   version_major;  /* incompatible bump -> engine REJECTS */
    uint16_t   version_minor;  /* additive bump -> tolerated */
    uint32_t   flags;  /* ZBC_HDR_* */
    uint32_t   header_size;  /* sizeof(zbc_header); readers skip grown tail */
    uint32_t   abi_id;  /* value-ABI version (D§11); must match engine */
    uint32_t   section_count;
    uint32_t   section_dir_off;
    uint32_t   entry_coro;  /* index of root/entry coroutine descriptor */
    uint64_t   file_size;  /* integrity: equals actual length */
    uint64_t   content_hash;  /* hash of bytes after this field (0=none) */
} zbc_header;

/* zbc_section: Section directory entry; fixed size so it is itself trivially indexable. */
typedef struct {
    uint32_t   kind;  /* ZBC_SEC_* */
    uint32_t   flags;  /* ZBC_SECF_* */
    uint64_t   offset;  /* from file start, 8-byte aligned */
    uint64_t   size;  /* byte length */
    uint32_t   count;  /* element count for record arrays (0=N/A) */
    uint32_t   elem_size;  /* per-element stride (0=N/A); forward-compat lever */
} zbc_section;

/* zbc_prov: Provenance record (32 bytes). prov[0] is the reserved all-zero 'none' entry. */
typedef struct {
    uint32_t   name;  /* StrId (0=none) */
    uint32_t   node_id;  /* stable IR node id (0=none) */
    uint32_t   line;  /* 1-based start line (0=unknown) */
    uint32_t   cmt_first;  /* CMNT index of first attached comment */
    uint16_t   file;  /* FileId */
    uint16_t   node_kind;  /* origin-node-kind enum */
    uint16_t   col;  /* start column (0=unknown) */
    uint16_t   col_end;  /* end column (0=unknown) */
    uint16_t   line_end;  /* end line (0=same as line) */
    uint16_t   flags;  /* ZBC_PROV_* */
    uint16_t   cmt_count;  /* comments in this record's run */
    uint16_t   _rsvd;
} zbc_prov;

/* zbc_comment: Comment record (8 bytes). A prov entry owns run [cmt_first, cmt_first+cmt_count). */
typedef struct {
    uint32_t   text;  /* StrId */
    uint8_t    kind;  /* ZBC_CMT_* */
    uint8_t    _pad;
    uint16_t   line;  /* source line of the comment (0=unknown) */
} zbc_comment;

/* zbc_file: File-table entry (8 bytes). FileId = u16 index into files[]. */
typedef struct {
    uint32_t   path;  /* StrId */
    uint32_t   _rsvd;
} zbc_file;

/* zbc_lineent: pc->src_ref line table entry (8 bytes). Binary-searched by pc. */
typedef struct {
    uint32_t   pc_start;  /* sorted ascending */
    uint32_t   src_ref;  /* prov index; src_at(pc)=largest pc_start<=pc */
} zbc_lineent;

/* zbc_instr: One fixed-width instruction (32 bytes). CODE section is zbc_instr[]. */
typedef struct {
    uint16_t   op;  /* ZBC opcode (see model.Op) */
    uint8_t    nargs;  /* count of valid args (0..4) */
    uint8_t    flags;  /* per-op flags (e.g. CONST from pool) */
    uint32_t   src_ref;  /* provenance index (0=none) */
    uint64_t   imm;  /* immediate / const bits (two's complement) */
    uint32_t   arg0;
    uint32_t   arg1;
    uint32_t   arg2;
    uint32_t   arg3;
} zbc_instr;

/* zbc_coro: Coroutine/func descriptor (32 bytes). CORO section is zbc_coro[]. */
typedef struct {
    uint32_t   name;  /* StrId (0=none/runtime profile) */
    uint32_t   code_start;  /* first instr index into CODE */
    uint32_t   code_count;  /* instr count */
    uint32_t   n_blocks;  /* FSM block count */
    uint32_t   block_start;  /* first entry into BLOCK table */
    uint32_t   frame_type;  /* TYPE id of frame struct (0=none) */
    uint32_t   src_ref;  /* provenance index (0=none) */
    uint32_t   _rsvd;
} zbc_coro;

/* zbc_block: FSM block (16 bytes). BLOCK section is zbc_block[]; a coro owns a run. */
typedef struct {
    uint32_t   idx;  /* FSM block index */
    uint32_t   pc_start;  /* first instr index (relative to coro) */
    uint32_t   pc_end;  /* one past last instr index */
    uint16_t   suspend_op;  /* orchestration op ending the block (0=terminal) */
    uint16_t   _rsvd;
} zbc_block;

/* zbc_select: Weighted-SELECT descriptor (16 bytes). SELECT section is zbc_select[]; arg0 of a SELECT instr indexes it. The OPLIST pool holds, contiguously from oplist_off, three u32 runs of length n_branches: branch coro ids, positive weights, then guard registers (0xFFFFFFFF = unguarded). */
typedef struct {
    uint32_t   oplist_off;  /* start index into OPLIST u32 pool */
    uint32_t   n_branches;  /* branch count */
    uint32_t   flags;  /* ZBC_SEL_* (bit0 = allow_none) */
    uint32_t   _rsvd;
} zbc_select;

/* zbc_solve: SOLVE descriptor (32 bytes). SOLVE section is zbc_solve[]; arg0 of a SOLVE instr indexes it. The OPLIST pool holds, from oplist_off, n_writeback interleaved (field_slot, var_id) u32 pairs -- the value ABI write-back keyed by object slot. When prob_len > 0, the (prob_off, prob_len) slice of the SPROB pool is a relocatable dv-solve SolveProblem blob: the engine compiles + solves it with the drawn seed and writes solver_get_value(var_id) back to each field_slot. When prob_len == 0 the minimal M1 randomizer applies: slot = seed + var_id (FixedSolveBackend, base 0). Seed is seed_value if SEED_FIXED else the frame's next draw. */
typedef struct {
    uint64_t   seed_value;  /* fixed-seed value (when flags & SEED_FIXED) */
    uint32_t   oplist_off;  /* start of (field_slot, var_id) pairs in OPLIST */
    uint32_t   n_writeback;  /* writeback pair count */
    uint32_t   flags;  /* ZBC_SOLVE_* (bit0 = fixed seed) */
    uint32_t   prob_off;  /* byte offset of the problem blob in the SPROB pool */
    uint32_t   prob_len;  /* problem blob length in bytes (0 = minimal randomizer) */
    uint32_t   _rsvd;
} zbc_solve;

/* Layout guards: sizes must match the spec (natural alignment). */
ZBC_STATIC_ASSERT(sizeof(zbc_header) == 48, "zbc_header size mismatch");
ZBC_STATIC_ASSERT(sizeof(zbc_section) == 32, "zbc_section size mismatch");
ZBC_STATIC_ASSERT(sizeof(zbc_prov) == 32, "zbc_prov size mismatch");
ZBC_STATIC_ASSERT(sizeof(zbc_comment) == 8, "zbc_comment size mismatch");
ZBC_STATIC_ASSERT(sizeof(zbc_file) == 8, "zbc_file size mismatch");
ZBC_STATIC_ASSERT(sizeof(zbc_lineent) == 8, "zbc_lineent size mismatch");
ZBC_STATIC_ASSERT(sizeof(zbc_instr) == 32, "zbc_instr size mismatch");
ZBC_STATIC_ASSERT(sizeof(zbc_coro) == 32, "zbc_coro size mismatch");
ZBC_STATIC_ASSERT(sizeof(zbc_block) == 16, "zbc_block size mismatch");
ZBC_STATIC_ASSERT(sizeof(zbc_select) == 16, "zbc_select size mismatch");
ZBC_STATIC_ASSERT(sizeof(zbc_solve) == 32, "zbc_solve size mismatch");

#endif /* ZUSPEC_ZBC_FORMAT_H */
