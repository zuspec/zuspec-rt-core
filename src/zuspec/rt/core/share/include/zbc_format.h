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
#define ZBC_SEC_ENTRY  0x0010  /* entry points (zbc_entry[]) */
#define ZBC_SEC_LOCAL  0x0011  /* named locals (zbc_localent[]; debug lowering) */
#define ZBC_SEC_SRC    0x0012  /* PSS source texts (zbc_srcent[]; debug profile) */
#define ZBC_SEC_IDENT  0x0013  /* API hash + generation id (zbc_ident; debug profile) */
#define ZBC_SEC_META   0x0014  /* host metadata documents (zbc_metaent[]; debug profile) */
#define ZBC_SEC_COMP   0x0015  /* component tree (zbc_comp + u32 bases[]) */
#define ZBC_SEC_MSG    0x0016  /* message table (zbc_msgtab + arrays) and run strings */
#define ZBC_SEC_REG    0x0017  /* register accesses (zbc_regent[]; debug lowering) */
#define ZBC_SEC_CHAN   0x0018  /* channel operations (zbc_chanent[]; debug lowering) */
#define ZBC_SEC_COV    0x0019  /* coverage items (zbc_covent[]; debug lowering) */
#define ZBC_SEC_IMPORT 0x001a  /* user import names (zbc_impent[]) */
#define ZBC_SEC_ACT    0x001b  /* activation table (zbc_acttab + arrays) */

/* zbc_msg_kind: zbc_msgarg.kind: how a formatted value prints. */
#define ZBC_MSG_INT     0x0000  /* integer */
#define ZBC_MSG_BOOL    0x0001  /* bool */
#define ZBC_MSG_ENUM    0x0002  /* enum (items follow) */
#define ZBC_MSG_STRING  0x0003  /* string id */
#define ZBC_MSG_CHANDLE 0x0004  /* chandle */

/* zbc_hdr_flags: Header flag bits. */
#define ZBC_HDR_HAS_PROV        0x0001  /* provenance sections present */
#define ZBC_HDR_PROFILE_RUNTIME 0x0002  /* runtime profile (provenance stripped) */
#define ZBC_HDR_COMP_INIT       0x0004  /* a coroutine constructs the component tree before the entry (P1.5) */
#define ZBC_HDR_PROFILE_DEBUG   0x0008  /* debug profile (provenance kept; may carry PSS sources) */

/* zbc_secf_flags: Per-section flag bits. */
#define ZBC_SECF_COMPRESSED 0x0001  /* payload is compressed */
#define ZBC_SECF_LINE_STMT  0x0002  /* LINE: each entry starts one PSS statement (debug lowering) */

/* zbc_comp_flags: COMP flag bits. */
#define ZBC_COMP_F_NODE_INST 0x0001  /* an activation node runs in an instance other than its parent's (a static offset or one its solve chooses) */

/* zbc_local_kind: What a named local's slot holds. */
#define ZBC_LOCAL_INT     0x0000  /* integer */
#define ZBC_LOCAL_BOOL    0x0001  /* bool */
#define ZBC_LOCAL_ENUM    0x0002  /* enum (its value) */
#define ZBC_LOCAL_STRING  0x0003  /* string */
#define ZBC_LOCAL_CHANDLE 0x0004  /* chandle */
#define ZBC_LOCAL_OPAQUE  0x0005  /* opaque handle */

/* zbc_local_flags: Named-local flag bits. */
#define ZBC_LOCAL_F_SIGNED 0x0001  /* signed integer */

/* zbc_reg_flags: Register-access flag bits. */
#define ZBC_REG_F_DISCARD 0x0001  /* a read whose value the statement drops */

/* zbc_cov_flags: Coverage-item flag bits. */
#define ZBC_COV_F_ARM      0x0001  /* a branch arm (else a statement) */
#define ZBC_COV_F_IMPLICIT 0x0002  /* an arm the code did not write (an implicit else) */

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

/* zbc_entry_kind: Entry-point kinds. */
#define ZBC_ENTRY_ACTION   0x0000  /* a top-level action coroutine */
#define ZBC_ENTRY_FUNCTION 0x0001  /* a component function */

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

/* zbc_localent: Named local (24 bytes; debug lowering). Sorted by (coro, pc_start). */
typedef struct {
    uint32_t   coro;  /* coroutine index */
    uint32_t   slot;  /* frame-local slot */
    uint32_t   name;  /* StrId: the PSS name (a struct leaf: 'cfg.src') */
    uint32_t   pc_start;  /* global CODE index where it comes into scope */
    uint32_t   pc_end;  /* global CODE index where it leaves scope (excl.) */
    uint16_t   width;  /* bits */
    uint8_t    kind;  /* zbc_local_kind */
    uint8_t    flags;  /* zbc_local_flags */
} zbc_localent;

/* zbc_regent: Register access (12 bytes; debug lowering). Sorted by pc. */
typedef struct {
    uint32_t   pc;  /* global CODE index of the access's IMPORT */
    uint32_t   inst;  /* register instance, relative to the frame's component instance; 0xFFFFFFFF: none (a discard alone) */
    uint32_t   flags;  /* zbc_reg_flags */
} zbc_regent;

/* zbc_chanent: Channel operation (24 bytes; debug lowering). Sorted by pc. */
typedef struct {
    uint32_t   pc;  /* global CODE index of the NOP that ends the operation */
    uint32_t   src_ref;  /* prov index of the operation */
    uint32_t   channel;  /* StrId: the channel's name */
    uint32_t   op;  /* StrId: the operation (put, get, try_get, ...) */
    uint32_t   val;  /* register holding the value; 0xFFFFFFFF: none */
    uint32_t   ok;  /* register holding ok (try_*); 0xFFFFFFFF: none */
} zbc_chanent;

/* zbc_covent: Coverage item (20 bytes; debug lowering). Sorted by pc. */
typedef struct {
    uint32_t   pc;  /* global CODE index where the statement starts / the arm is taken */
    uint32_t   src_ref;  /* prov index of the statement or arm */
    uint32_t   flags;  /* zbc_cov_flags */
    uint32_t   scope;  /* StrId: the function it is in (<comp type>::<fn>) */
    uint32_t   path;  /* StrId: its place in PSS structure (/-joined) */
} zbc_covent;

/* zbc_srcent: One PSS source file carried in a debug image (8 bytes; SRC section). */
typedef struct {
    uint32_t   path;  /* StrId: the source file's path (as in FILE) */
    uint32_t   text;  /* StrId: the file's full text */
} zbc_srcent;

/* zbc_ident: Image identity (16 bytes; IDENT section, one record; debug profile). */
typedef struct {
    uint64_t   api_hash;  /* FNV-1a/64 of every entry and import signature (0=unset) */
    uint64_t   generation;  /* caller-supplied build generation (0=unset) */
} zbc_ident;

/* zbc_metaent: One host metadata document carried in a debug image (8 bytes; META section). */
typedef struct {
    uint32_t   name;  /* StrId: the document's name (e.g. pssc.catalog) */
    uint32_t   text;  /* StrId: the document's text (the host's format) */
} zbc_metaent;

/* zbc_impent: One user import's name (8 bytes; IMPORT section, sorted by fn_id; codegen/debug profiles). */
typedef struct {
    uint32_t   fn_id;  /* IMPORT fn_id the code calls */
    uint32_t   name;  /* StrId: the import's PSS name */
} zbc_impent;

/* zbc_comp: Component tree (16 bytes; COMP section, one record followed by n_inst u32 instance bases in pre-order, the root 0). Present when the image has a component tree (P1.5). */
typedef struct {
    uint32_t   n_slots;  /* component object slot count */
    uint32_t   init_coro;  /* CORO constructing the tree before the entry (0xFFFFFFFF = none) */
    uint32_t   n_inst;  /* instance count; n_inst u32 bases follow */
    uint32_t   flags;  /* zbc_comp_flags */
} zbc_comp;

/* zbc_msgtab: Message table (32 bytes; MSG section). Followed, in order, by zbc_msg[n_msgs], zbc_msgarg[n_args], zbc_msgitem[n_items], u32 slots[n_slots], u32 str_off[n_strings] and the string text. Self-contained (not the STRB string table), so a runtime image can format message()/error()/fatal() text (P4.2g). */
typedef struct {
    uint32_t   n_msgs;  /* zbc_msg records */
    uint32_t   n_args;  /* zbc_msgarg records */
    uint32_t   n_items;  /* zbc_msgitem records */
    uint32_t   n_slots;  /* u32 frame-local slot numbers */
    uint32_t   n_strings;  /* u32 string offsets */
    uint32_t   n_model_strings;  /* the first n_model_strings strings are the run's string ids */
    uint32_t   strb_size;  /* bytes of NUL-terminated string text */
    uint32_t   _pad;
} zbc_msgtab;

/* zbc_msg: One message()/error()/fatal() call site (16 bytes; MSG section). */
typedef struct {
    uint32_t   fmt;  /* string index of the format */
    uint32_t   n_args;  /* formatted values (slots: n_args + 1) */
    uint32_t   arg0;  /* index of its first zbc_msgarg */
    uint32_t   slot0;  /* index of its first slot; slot0 = verbosity/status, then each value */
} zbc_msg;

/* zbc_msgarg: The static type of one formatted value (16 bytes; MSG section). */
typedef struct {
    uint8_t    kind;  /* zbc_msg_kind */
    uint8_t    is_signed;
    uint16_t   _pad;
    uint32_t   width;  /* value width in bits */
    uint32_t   item0;  /* enum: index of its first zbc_msgitem */
    uint32_t   n_items;  /* enum: item count */
} zbc_msgarg;

/* zbc_msgitem: One enum item, for %n (16 bytes; MSG section). */
typedef struct {
    uint32_t   name;  /* string index of the item name */
    uint32_t   _pad;
    uint64_t   value;  /* item value (two's complement) */
} zbc_msgitem;

/* zbc_acttab: Activation table (64 bytes; ACT section). Followed, in order, by zbc_actinit[n_init], zbc_act[n_acts], zbc_actnode[n_nodes], zbc_actscope[n_scopes], zbc_actsite[n_sites], zbc_actcone[n_cones], zbc_actvar[n_vars], zbc_actcon[n_cons], zbc_actpool[n_pools], zbc_actclaim[n_claims], zbc_actwrite[n_writes], zbc_actpick[n_picks], u32 lists[n_u32], u32 str_off[n_strings], the string text and the blob area (blob offsets are relative to its start; each blob 8-aligned). Record indices (*0) are section-global; ids inside an activation (node, scope, site, cone, pool) are local to it. Absent when no coroutine has an activity. */
typedef struct {
    uint32_t   n_acts;
    uint32_t   n_nodes;
    uint32_t   n_scopes;
    uint32_t   n_sites;
    uint32_t   n_cones;
    uint32_t   n_vars;
    uint32_t   n_cons;
    uint32_t   n_pools;
    uint32_t   n_claims;
    uint32_t   n_writes;
    uint32_t   n_picks;
    uint32_t   n_init;  /* zbc_actinit records */
    uint32_t   n_u32;  /* u32 list entries */
    uint32_t   n_strings;  /* u32 string offsets */
    uint32_t   strb_size;  /* bytes of NUL-terminated string text, padded to 8 */
    uint32_t   blob_size;  /* bytes of dv-solve blobs after the string text */
} zbc_acttab;

/* zbc_actinit: A non-rand attribute's initial value (16 bytes; ACT section). */
typedef struct {
    uint32_t   slot;
    uint32_t   unknown;  /* 1: computed at traversal (value unused) */
    uint64_t   value;  /* constant initial value (two's complement) */
} zbc_actinit;

/* zbc_act: One activation (80 bytes; ACT section). */
typedef struct {
    uint32_t   coro;  /* coroutine index the activation is rooted at */
    uint32_t   size;  /* object slots */
    uint32_t   root;  /* string */
    uint32_t   type;  /* string: root action type */
    uint32_t   flags;  /* bit 0: lookahead */
    uint32_t   node0;
    uint32_t   n_nodes;
    uint32_t   scope0;
    uint32_t   n_scopes;
    uint32_t   site0;
    uint32_t   n_sites;
    uint32_t   cone0;
    uint32_t   n_cones;
    uint32_t   pool0;
    uint32_t   n_pools;
    uint32_t   names0;  /* u32 list: string id per slot */
    uint32_t   n_names;
    uint32_t   init0;
    uint32_t   n_init;
    uint32_t   _pad;
} zbc_act;

/* zbc_actnode: One action node of an activation (80 bytes; ACT section). */
typedef struct {
    uint32_t   path;  /* string */
    uint32_t   type;  /* string */
    uint32_t   base;
    uint32_t   size;
    uint32_t   parent;  /* 0xFFFFFFFF = none */
    uint32_t   decl_scope;  /* 0xFFFFFFFF = none */
    uint32_t   comp0;  /* u32 list: candidate instances */
    uint32_t   n_comp;
    uint32_t   comp_slot;  /* 0xFFFFFFFF = none */
    uint32_t   claim0;  /* zbc_actclaim */
    uint32_t   n_claims;
    uint32_t   swrite0;  /* zbc_actwrite: state writes */
    uint32_t   n_swrites;
    uint32_t   bwrite0;  /* zbc_actwrite: buffer writes */
    uint32_t   n_bwrites;
    uint32_t   pick0;  /* zbc_actpick */
    uint32_t   n_picks;
    uint32_t   child0;  /* u32 list: child node ids */
    uint32_t   n_children;
    uint32_t   _pad;
} zbc_actnode;

/* zbc_actscope: One activity block (16 bytes; ACT section). */
typedef struct {
    uint32_t   kind;  /* index in ScopeKind order */
    uint32_t   parent;  /* 0xFFFFFFFF = none */
    uint32_t   node;
    uint32_t   _pad;
} zbc_actscope;

/* zbc_actsite: One traversal site (16 bytes; ACT section). */
typedef struct {
    uint32_t   owner;
    uint32_t   target;
    uint32_t   scope;
    uint32_t   _pad;
} zbc_actsite;

/* zbc_actcone: One solve cone (40 bytes; ACT section). A solve compiles the base blob, then adds the blob of each constraint in force (dv-solve add_constraint). */
typedef struct {
    uint32_t   id;
    uint32_t   node0;  /* u32 list */
    uint32_t   n_nodes;
    uint32_t   var0;  /* zbc_actvar */
    uint32_t   n_vars;
    uint32_t   con0;  /* zbc_actcon */
    uint32_t   n_cons;
    uint32_t   base_off;  /* blob area: dv-solve blob of the cone's variables */
    uint32_t   base_size;
    uint32_t   _pad;
} zbc_actcone;

/* zbc_actvar: One cone variable (48 bytes; ACT section). */
typedef struct {
    uint32_t   name;  /* string */
    uint32_t   node;
    uint32_t   slot;
    uint32_t   width;
    uint32_t   flags;  /* 1 signed, 2 rand, 4 busy_lock */
    uint32_t   comp_read;  /* 0xFFFFFFFF = none */
    uint32_t   loop_local;  /* string; 0xFFFFFFFF = none */
    uint32_t   loop_node;  /* 0xFFFFFFFF = none */
    uint32_t   live_from;  /* 0xFFFFFFFF = none */
    uint32_t   busy_pool;  /* 0xFFFFFFFF = none */
    uint32_t   pick;  /* 0xFFFFFFFF = none */
    uint32_t   loop0;  /* u32 list: n, then n (coro, frame slot) pairs of the coros holding loop_local; 0xFFFFFFFF = none */
} zbc_actvar;

/* zbc_actcon: One cone constraint (40 bytes; ACT section). */
typedef struct {
    uint32_t   kind;  /* 0 type, 1 activity, 2 with, 3 comp */
    uint32_t   owner;
    uint32_t   scope;  /* 0xFFFFFFFF = none */
    uint32_t   site;  /* 0xFFFFFFFF = none */
    uint32_t   node0;  /* u32 list */
    uint32_t   n_nodes;
    uint32_t   blob_off;  /* blob area: the cone's variables and this constraint */
    uint32_t   blob_size;
    uint32_t   text;  /* string: the constraint as written, for unsat messages */
    uint32_t   _pad;
} zbc_actcon;

/* zbc_actpool: One pool instance (48 bytes; ACT section). */
typedef struct {
    uint32_t   id;
    uint32_t   inst;
    uint32_t   name;  /* string */
    uint32_t   kind;  /* string: buffer/stream/state/resource */
    uint32_t   size;  /* 0xFFFFFFFF = none */
    uint32_t   slot0;  /* u32 list */
    uint32_t   n_slots;
    uint32_t   leaf0;  /* u32 list of strings */
    uint32_t   n_leaves;
    uint32_t   init0;  /* u32 list */
    uint32_t   n_init;
    uint32_t   _pad;
} zbc_actpool;

/* zbc_actclaim: One resource claim (24 bytes; ACT section). */
typedef struct {
    uint32_t   ref;  /* string */
    uint32_t   lock;  /* 1 lock, 0 share */
    uint32_t   iid_slot;
    uint32_t   pool0;  /* u32 list of (inst, pool) pairs */
    uint32_t   n_pools;  /* pairs */
    uint32_t   _pad;
} zbc_actclaim;

/* zbc_actwrite: One state or buffer output (24 bytes; ACT section). */
typedef struct {
    uint32_t   ref;  /* string */
    uint32_t   src0;  /* u32 list */
    uint32_t   n_src;
    uint32_t   pool0;  /* u32 list of (inst, pool) pairs */
    uint32_t   n_pools;  /* pairs */
    uint32_t   _pad;
} zbc_actwrite;

/* zbc_actpick: One unbound buffer input (24 bytes; ACT section). */
typedef struct {
    uint32_t   ref;  /* string */
    uint32_t   slot0;  /* u32 list */
    uint32_t   n_slots;
    uint32_t   sel_slot;
    uint32_t   pool0;  /* u32 list of (inst, pool) pairs */
    uint32_t   n_pools;  /* pairs */
} zbc_actpick;

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

/* zbc_solve: SOLVE descriptor (32 bytes). SOLVE section is zbc_solve[]; arg0 of a SOLVE instr indexes it. The OPLIST pool holds, from oplist_off, n_writeback interleaved (field_slot, var_id) u32 pairs -- the value ABI write-back keyed by object slot. When prob_len > 0, the (prob_off, prob_len) slice of the SPROB pool is a relocatable dv-solve SolveProblem blob: the engine compiles + solves it with the drawn seed and writes dvs_solver_get_value(var_id) back to each field_slot. When prob_len == 0 the minimal M1 randomizer applies: slot = seed + var_id (FixedSolveBackend, base 0). Seed is seed_value if SEED_FIXED else the frame's next draw. */
typedef struct {
    uint64_t   seed_value;  /* fixed-seed value (when flags & SEED_FIXED) */
    uint32_t   oplist_off;  /* start of (field_slot, var_id) pairs in OPLIST */
    uint32_t   n_writeback;  /* writeback pair count */
    uint32_t   flags;  /* ZBC_SOLVE_* (bit0 = fixed seed) */
    uint32_t   prob_off;  /* byte offset of the problem blob in the SPROB pool */
    uint32_t   prob_len;  /* problem blob length in bytes (0 = minimal randomizer) */
    uint32_t   _rsvd;
} zbc_solve;

/* zbc_entry: Entry point (32 bytes). ENTRY section is zbc_entry[]: where a host may enter the image. A function's arguments go in the root frame's args (LD_ARG), one per leaf of each parameter, in the order sig.params lists them; sig is JSON {"params": [[name, type descriptor], ...], "returns": descriptor|null}. */
typedef struct {
    uint32_t   name;  /* StrId: action coroutine name, or <comp type>::<fn> */
    uint32_t   coro;  /* CORO index the host starts */
    uint32_t   kind;  /* ZBC_ENTRY_* */
    uint32_t   comp_type;  /* StrId of a function's component type (0=none) */
    uint32_t   sig;  /* StrId of the JSON signature {params, returns} (0=none) */
    uint32_t   _rsvd0;
    uint32_t   _rsvd1;
    uint32_t   _rsvd2;
} zbc_entry;

/* Layout guards: sizes must match the spec (natural alignment). */
ZBC_STATIC_ASSERT(sizeof(zbc_header) == 48, "zbc_header size mismatch");
ZBC_STATIC_ASSERT(sizeof(zbc_section) == 32, "zbc_section size mismatch");
ZBC_STATIC_ASSERT(sizeof(zbc_prov) == 32, "zbc_prov size mismatch");
ZBC_STATIC_ASSERT(sizeof(zbc_comment) == 8, "zbc_comment size mismatch");
ZBC_STATIC_ASSERT(sizeof(zbc_file) == 8, "zbc_file size mismatch");
ZBC_STATIC_ASSERT(sizeof(zbc_lineent) == 8, "zbc_lineent size mismatch");
ZBC_STATIC_ASSERT(sizeof(zbc_localent) == 24, "zbc_localent size mismatch");
ZBC_STATIC_ASSERT(sizeof(zbc_regent) == 12, "zbc_regent size mismatch");
ZBC_STATIC_ASSERT(sizeof(zbc_chanent) == 24, "zbc_chanent size mismatch");
ZBC_STATIC_ASSERT(sizeof(zbc_covent) == 20, "zbc_covent size mismatch");
ZBC_STATIC_ASSERT(sizeof(zbc_srcent) == 8, "zbc_srcent size mismatch");
ZBC_STATIC_ASSERT(sizeof(zbc_ident) == 16, "zbc_ident size mismatch");
ZBC_STATIC_ASSERT(sizeof(zbc_metaent) == 8, "zbc_metaent size mismatch");
ZBC_STATIC_ASSERT(sizeof(zbc_impent) == 8, "zbc_impent size mismatch");
ZBC_STATIC_ASSERT(sizeof(zbc_comp) == 16, "zbc_comp size mismatch");
ZBC_STATIC_ASSERT(sizeof(zbc_msgtab) == 32, "zbc_msgtab size mismatch");
ZBC_STATIC_ASSERT(sizeof(zbc_msg) == 16, "zbc_msg size mismatch");
ZBC_STATIC_ASSERT(sizeof(zbc_msgarg) == 16, "zbc_msgarg size mismatch");
ZBC_STATIC_ASSERT(sizeof(zbc_msgitem) == 16, "zbc_msgitem size mismatch");
ZBC_STATIC_ASSERT(sizeof(zbc_acttab) == 64, "zbc_acttab size mismatch");
ZBC_STATIC_ASSERT(sizeof(zbc_actinit) == 16, "zbc_actinit size mismatch");
ZBC_STATIC_ASSERT(sizeof(zbc_act) == 80, "zbc_act size mismatch");
ZBC_STATIC_ASSERT(sizeof(zbc_actnode) == 80, "zbc_actnode size mismatch");
ZBC_STATIC_ASSERT(sizeof(zbc_actscope) == 16, "zbc_actscope size mismatch");
ZBC_STATIC_ASSERT(sizeof(zbc_actsite) == 16, "zbc_actsite size mismatch");
ZBC_STATIC_ASSERT(sizeof(zbc_actcone) == 40, "zbc_actcone size mismatch");
ZBC_STATIC_ASSERT(sizeof(zbc_actvar) == 48, "zbc_actvar size mismatch");
ZBC_STATIC_ASSERT(sizeof(zbc_actcon) == 40, "zbc_actcon size mismatch");
ZBC_STATIC_ASSERT(sizeof(zbc_actpool) == 48, "zbc_actpool size mismatch");
ZBC_STATIC_ASSERT(sizeof(zbc_actclaim) == 24, "zbc_actclaim size mismatch");
ZBC_STATIC_ASSERT(sizeof(zbc_actwrite) == 24, "zbc_actwrite size mismatch");
ZBC_STATIC_ASSERT(sizeof(zbc_actpick) == 24, "zbc_actpick size mismatch");
ZBC_STATIC_ASSERT(sizeof(zbc_instr) == 32, "zbc_instr size mismatch");
ZBC_STATIC_ASSERT(sizeof(zbc_coro) == 32, "zbc_coro size mismatch");
ZBC_STATIC_ASSERT(sizeof(zbc_block) == 16, "zbc_block size mismatch");
ZBC_STATIC_ASSERT(sizeof(zbc_select) == 16, "zbc_select size mismatch");
ZBC_STATIC_ASSERT(sizeof(zbc_solve) == 32, "zbc_solve size mismatch");
ZBC_STATIC_ASSERT(sizeof(zbc_entry) == 32, "zbc_entry size mismatch");

#endif /* ZUSPEC_ZBC_FORMAT_H */
