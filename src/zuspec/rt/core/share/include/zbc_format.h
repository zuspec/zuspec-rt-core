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
#define ZBC_VERSION_MAJOR 2
#define ZBC_VERSION_MINOR 0

/* zbc_sec_kind: Section kinds. Readers skip unknown kinds (forward-compat). */
#define ZBC_SEC_CODE    0x0001  /* opcode stream */
#define ZBC_SEC_CORO    0x0002  /* coroutine/func descriptors */
#define ZBC_SEC_TYPE    0x0003  /* type/field metadata */
#define ZBC_SEC_CONST   0x0004  /* >64-bit literal constant pool */
#define ZBC_SEC_SOLVE   0x0005  /* SolveProblem blobs (P4) */
#define ZBC_SEC_STRB    0x0006  /* string blob */
#define ZBC_SEC_STRO    0x0007  /* string offsets */
#define ZBC_SEC_FILE    0x0008  /* file table */
#define ZBC_SEC_PROV    0x0009  /* provenance records */
#define ZBC_SEC_CMNT    0x000a  /* comment records */
#define ZBC_SEC_LINE    0x000b  /* pc->src_ref line table */
#define ZBC_SEC_BLOCK   0x000c  /* FSM block table (zbc_block[]) */
#define ZBC_SEC_OPLIST  0x000d  /* u32 operand lists (PAR/SELECT branch ids) */
#define ZBC_SEC_SELECT  0x000e  /* weighted-SELECT descriptors (zbc_select[]) */
#define ZBC_SEC_SPROB   0x000f  /* relocatable dv-solve SolveProblem blobs */
#define ZBC_SEC_ENTRY   0x0010  /* entry points (zbc_entry[]) */
#define ZBC_SEC_LOCAL   0x0011  /* named locals (zbc_localent[]; debug lowering) */
#define ZBC_SEC_SRC     0x0012  /* PSS source texts (zbc_srcent[]; debug profile) */
#define ZBC_SEC_IDENT   0x0013  /* API hash + generation id (zbc_ident; debug profile) */
#define ZBC_SEC_META    0x0014  /* host metadata documents (zbc_metaent[]; debug profile) */
#define ZBC_SEC_COMP    0x0015  /* component tree (zbc_comp + u32 bases[]) */
#define ZBC_SEC_MSG     0x0016  /* message table (zbc_msgtab + arrays) and run strings */
#define ZBC_SEC_REG     0x0017  /* register accesses (zbc_regent[]; debug lowering) */
#define ZBC_SEC_CHAN    0x0018  /* channel operations (zbc_chanent[]; debug lowering) */
#define ZBC_SEC_COV     0x0019  /* coverage items (zbc_covent[]; debug lowering) */
#define ZBC_SEC_IMPORT  0x001a  /* user import names (zbc_impent[]) */
#define ZBC_SEC_ACT     0x001b  /* activation table (zbc_acttab + arrays) */
#define ZBC_SEC_LAYOUT  0x001c  /* component slot and instance names (zbc_layout) */
#define ZBC_SEC_REGMAP  0x001d  /* register map (zbc_regmap) */
#define ZBC_SEC_SEEDKEY 0x001e  /* each coroutine's seed key, u64[coro count] (keyed seeds, F3.9) */
#define ZBC_SEC_COVG    0x001f  /* covergroup instances (zbc_covg; F4.4) */
#define ZBC_SEC_EXEC    0x0020  /* executors and executor groups (zbc_exectab + arrays; F5.4a) */
#define ZBC_SEC_EVENT   0x0021  /* host-fed channels (zbc_evtent[]; F5.5a) */
#define ZBC_SEC_MEMMAP  0x0022  /* address spaces and regions (zbc_memmap; F7.2c) */
#define ZBC_SEC_API     0x0023  /* import-interface APIs (zbc_apient[]) */
#define ZBC_SEC_CLASS   0x0024  /* types: layouts and methods (zbc_class[]) */
#define ZBC_SEC_ROOT    0x0025  /* root candidates (zbc_rootent[]) */
#define ZBC_SEC_ROOTIMG 0x0026  /* the other root candidates' images, each 8-aligned (ROOT locates them) */

/* zbc_evt_kind: zbc_evtent.kind: equal to the notify kind (N_IRQ 1 has no rows). */
#define ZBC_EVT_CHAN 0x0002  /* a channel_c; fed by notify N_CHAN */

/* zbc_msg_kind: zbc_msgarg.kind: how a formatted value prints. */
#define ZBC_MSG_INT     0x0000  /* integer */
#define ZBC_MSG_BOOL    0x0001  /* bool */
#define ZBC_MSG_ENUM    0x0002  /* enum (items follow) */
#define ZBC_MSG_STRING  0x0003  /* string id */
#define ZBC_MSG_CHANDLE 0x0004  /* chandle */
#define ZBC_MSG_FLOAT   0x0005  /* float (binary64 bits) */

/* zbc_hdr_flags: Header flag bits. Bits 0x0100-0x8000 (ZBC_HDR_EXT_MASK) each declare an extension profile the image uses; a loader refuses an image declaring one it does not implement, and an opcode in an extension's range the image does not declare. */
#define ZBC_HDR_HAS_PROV        0x0001  /* provenance sections present */
#define ZBC_HDR_PROFILE_RUNTIME 0x0002  /* runtime profile (provenance stripped) */
#define ZBC_HDR_COMP_INIT       0x0004  /* a coroutine constructs the component tree before the entry (P1.5) */
#define ZBC_HDR_PROFILE_DEBUG   0x0008  /* debug profile (provenance kept; may carry PSS sources) */
#define ZBC_HDR_EXT_PSS         0x0100  /* the pss extension: opcodes 0x80-0xBF and the ACT, COMP, EXEC, EVENT, MEMMAP and COVG sections */

/* zbc_profile: Profiles: the core opcode and section set, and extensions an image declares in its header flags, each with its own opcode range (0xC0-0xFF: the next front end). */
#define ZBC_HDR_EXT_MASK  0xff00  /* header flag bits that declare extensions */
#define ZBC_OP_CORE_FIRST 0x0000  /* core opcodes: every implementation */
#define ZBC_OP_CORE_LAST  0x007f
#define ZBC_OP_PSS_FIRST  0x0080  /* the pss extension's opcodes (ZBC_HDR_EXT_PSS) */
#define ZBC_OP_PSS_LAST   0x00bf

/* zbc_secf_flags: Per-section flag bits. */
#define ZBC_SECF_COMPRESSED 0x0001  /* payload is compressed */
#define ZBC_SECF_LINE_STMT  0x0002  /* LINE: each entry starts one PSS statement (debug lowering) */

/* zbc_comp_flags: COMP flag bits. */
#define ZBC_COMP_F_NODE_INST 0x0001  /* an activation node runs in an instance other than its parent's (a static offset or one its solve chooses) */
#define ZBC_COMP_F_RUN_HOOKS 0x0002  /* coroutines init_coro+1 ($run_start: exec run_start, top-down) and init_coro+2 ($run_end: exec run_end, bottom-up) run before and after a root action */

/* zbc_local_kind: What a named local's slot holds. */
#define ZBC_LOCAL_INT     0x0000  /* integer */
#define ZBC_LOCAL_BOOL    0x0001  /* bool */
#define ZBC_LOCAL_ENUM    0x0002  /* enum (its value) */
#define ZBC_LOCAL_STRING  0x0003  /* string */
#define ZBC_LOCAL_CHANDLE 0x0004  /* chandle */
#define ZBC_LOCAL_OPAQUE  0x0005  /* opaque handle */
#define ZBC_LOCAL_FLOAT   0x0006  /* float (binary64 bits) */

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
#define ZBC_SOLVE_PROCEDURAL 0x0002  /* a procedural randomize: each (k, var_id) pair keeps var_id's value in the frame's solved slot k for SOLVE_GET; nothing is written back */
#define ZBC_SOLVE_ARRAYS     0x0004  /* the problem reads lists (dv-solve arrays): each input pair whose var_id >= the blob's n_vars is (key, n_vars + array_id), key = the field slot holding the list's heap handle (procedural: the SOLVE_PIN id it is pinned at); the blob's ArraySpec gives each array's element width, signedness and kind (RAND lists are solved and written back). An engine without heap lists must refuse it */

/* zbc_entry_kind: Entry-point kinds. */
#define ZBC_ENTRY_ACTION   0x0000  /* a top-level action coroutine */
#define ZBC_ENTRY_FUNCTION 0x0001  /* a component function */

/* zbc_entry_flags: Entry-point flag bits. */
#define ZBC_ENTRY_F_EXPORTED 0x0001  /* an exported action: a host may run it by name and a typed API gives it a method (PSS export statements, --export-action) */

/* zbc_imp_flags: Import flag bits. */
#define ZBC_IMP_F_BLOCKING 0x0001  /* the call may consume host time */
#define ZBC_IMP_F_TARGET   0x0002  /* declared import target */
#define ZBC_IMP_F_SOLVE    0x0004  /* declared import solve */

/* zbc_api_kind: API kinds. */
#define ZBC_API_BUILTIN 0x0000  /* an API the engine defines (mem, config, seq) */
#define ZBC_API_USER    0x0001  /* the PSS imports of one package or component */

/* zbc_api_id: API ids (host-abi.md, the import interface). */
#define ZBC_API_MEM        0x0000  /* memory and register access (read, write) */
#define ZBC_API_CONFIG     0x0001  /* solve-time configuration (reserved) */
#define ZBC_API_SEQ        0x0002  /* host sequences (reserved) */
#define ZBC_API_USER_FIRST 0x0010  /* the first user API id */

/* zbc_class_kind: CLASS row kinds. */
#define ZBC_CLASS_COMPONENT 0x0000
#define ZBC_CLASS_ACTION    0x0001
#define ZBC_CLASS_STRUCT    0x0002

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

/* zbc_ident: Image identity (24 bytes; IDENT section, one record; debug profile). */
typedef struct {
    uint64_t   api_hash;  /* FNV-1a/64 of every entry and import signature (0=unset) */
    uint64_t   generation;  /* caller-supplied build generation (0=unset) */
    uint32_t   time_exp;  /* int32: one WAIT TIME tick is 10^time_exp s (-12 = ps; no IDENT: -12) */
    uint32_t   _pad;
} zbc_ident;

/* zbc_metaent: One host metadata document carried in a debug image (8 bytes; META section). */
typedef struct {
    uint32_t   name;  /* StrId: the document's name (e.g. pssc.catalog) */
    uint32_t   text;  /* StrId: the document's text (the host's format) */
} zbc_metaent;

/* zbc_impent: One user import (24 bytes; IMPORT section, sorted by fn_id; codegen/debug profiles): its name, API, kind and signature, which a host links against. */
typedef struct {
    uint32_t   fn_id;  /* IMPORT fn_id the code calls */
    uint32_t   name;  /* StrId: the import's PSS name */
    uint32_t   api;  /* API id (an API row): the package or component declaring it */
    uint32_t   flags;  /* zbc_imp_flags */
    uint32_t   sig;  /* StrId: JSON {arg_types, ret_type, string_at[, ret_types]} (0=none) */
    uint32_t   qname;  /* StrId: the import's qualified name (0=none) */
} zbc_impent;

/* zbc_apient: One API of the import interface (16 bytes; API section, sorted by id; codegen/debug profiles). Ids 0-15 are built in; user APIs are 16 up, one per package or component declaring imports, in name order. */
typedef struct {
    uint32_t   id;  /* the API id requests carry (zbc_api_id) */
    uint32_t   name;  /* StrId: mem, config, seq, or the declaring scope */
    uint32_t   kind;  /* zbc_api_kind */
    uint32_t   n_fns;  /* the IMPORT rows naming this API */
} zbc_apient;

/* zbc_class: One type (40 bytes; CLASS section; codegen/debug profiles): every component type in the tree and its bases, every action with an entry and its bases, and every plain-data struct their fields use, by qname. ``fields`` is the type's normative native layout (value-abi.md 2.1); slot k of an object is row k. */
typedef struct {
    uint32_t   qname;  /* StrId: the type's qualified name */
    uint32_t   kind;  /* zbc_class_kind */
    uint32_t   base;  /* StrId: its base type (0=none) */
    uint32_t   coro;  /* an action's coroutine (its traverse method); 0xFFFFFFFF = none */
    uint32_t   n_fields;  /* leaves (slots) of an object of the type */
    uint32_t   size;  /* native size in bytes (value-abi.md 2.1) */
    uint32_t   align;  /* native alignment in bytes */
    uint32_t   fields;  /* StrId: JSON [[name, descriptor, offset, size], ...] in slot order */
    uint32_t   methods;  /* StrId: JSON [[name, entry symbol], ...]: its functions' entries (0=none) */
    uint32_t   _pad;
} zbc_class;

/* zbc_rootent: One root candidate (32 bytes; ROOT section; codegen/debug profiles). Row 0 is the image's own root, whose tables (COMP, ACT, ENTRY, ...) are this image's. Every other candidate's tables are a complete image of its own, nested in ROOTIMG at (img_off, img_size). */
typedef struct {
    uint32_t   type;  /* StrId: the root candidate's component type */
    uint32_t   ctor;  /* StrId: JSON {name, params: [{name, type}]}: its constructor (0=none) */
    uint64_t   api_hash;  /* the API hash of the image built for this root */
    uint64_t   img_off;  /* offset of its image in the ROOTIMG section (img_size 0: this image) */
    uint64_t   img_size;  /* byte length of its image (0: this image) */
} zbc_rootent;

/* zbc_comp: Component tree (16 bytes; COMP section, one record followed by n_inst u32 instance bases in pre-order, the root 0). Present when the image has a component tree (P1.5). */
typedef struct {
    uint32_t   n_slots;  /* component object slot count */
    uint32_t   init_coro;  /* CORO constructing the tree before the entry (0xFFFFFFFF = none) */
    uint32_t   n_inst;  /* instance count; n_inst u32 bases follow */
    uint32_t   flags;  /* zbc_comp_flags */
} zbc_comp;

/* zbc_layout: Component layout (16 bytes; LAYOUT section, one record; codegen/debug profiles): the component object's slots, each with its name, type and offset in the normative native layout (value-abi.md 2.1), and its instances' paths and types. */
typedef struct {
    uint32_t   root;  /* StrId: the root component type */
    uint32_t   slots;  /* StrId: JSON [[name, descriptor, offset], ...], one per component slot in slot order */
    uint32_t   insts;  /* StrId: JSON [[path, type], ...], one per instance in pre-order (root path "") */
    uint32_t   size;  /* native size of the component object in bytes */
} zbc_layout;

/* zbc_covg: Covergroup instances (4 bytes; COVG section, one record): each one's coverpoints, bins, crosses and iff flags as plain data (F4.4; codegen/debug). */
typedef struct {
    uint32_t   doc;  /* StrId: JSON [covergroup instance], indexed by COV_SAMPLE's covg id */
} zbc_covg;

/* zbc_exectab: Executor table (16 bytes; EXEC section). Followed by zbc_execent[n_exec] (row index = the executor id requests carry), zbc_execgrp[n_groups] and u32 members[n_members] (F5.4a; codegen/debug profiles). */
typedef struct {
    uint32_t   n_exec;  /* zbc_execent records (executor instances) */
    uint32_t   n_groups;  /* zbc_execgrp records (executor groups) */
    uint32_t   n_members;  /* u32 group member executor ids */
    uint32_t   default_exec;  /* the default executor's row + 1; 0: none (pssvm 8e) */
} zbc_exectab;

/* zbc_execent: One executor instance (12 bytes; EXEC section), in instance order. */
typedef struct {
    uint32_t   path;  /* StrId: the executor instance's path */
    uint32_t   type;  /* StrId: its component type */
    uint32_t   trait;  /* StrId: its trait struct type ("" = none) */
} zbc_execent;

/* zbc_execgrp: One executor group (16 bytes; EXEC section); members are executor ids. */
typedef struct {
    uint32_t   path;  /* StrId: the group instance's path */
    uint32_t   trait;  /* StrId: its trait struct type */
    uint32_t   first;  /* its first member in members[] */
    uint32_t   count;  /* its member count */
} zbc_execgrp;

/* zbc_evtent: One host-fed source (32 bytes; EVENT section): every scalar channel_c instance, in slot order; the row index is the id N_CHAN notifies (F5.5a; codegen/debug profiles). */
typedef struct {
    uint32_t   path;  /* StrId: the channel's instance path (``d0.c``) */
    uint32_t   kind;  /* zbc_evt_kind: the notify kind that feeds it */
    uint32_t   width;  /* element width in bits */
    uint32_t   flags;  /* bit 0: the element is signed */
    uint32_t   count;  /* component slot of its element count */
    uint32_t   head;  /* component slot of its ring head */
    uint32_t   buf;  /* component slot of its first ring element */
    uint32_t   depth;  /* ring elements (consecutive slots from buf) */
} zbc_evtent;

/* zbc_regmap: Register map (4 bytes; REGMAP section, one record): each register instance's path, offset, width, access and fields, as the host built it (debug profile). */
typedef struct {
    uint32_t   doc;  /* StrId: JSON {reg-id: register}, reg-id an instance index (decimal string) */
} zbc_regmap;

/* zbc_memmap: Memory map (4 bytes; MEMMAP section, one record): each address space instance and the regions added to it, as the host built it (F7.2c; codegen/debug profiles). */
typedef struct {
    uint32_t   doc;  /* StrId: JSON {"spaces": [...], "regions": [...]} */
} zbc_memmap;

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
    uint32_t   unknown;  /* 1: computed at traversal (value unused); 2: the node's uid, numbered per run */
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

/* zbc_actnode: One action node of an activation (88 bytes; ACT section). ``addr0`` indexes the u32 lists at ``n`` then ``n`` quads (space string, addr slot, size slot, permanent slot or 0xFFFFFFFF) of the node's address claims, which the run's ledger keeps apart; list index 0 never holds them. A contiguous claim has addr slot 0xFFFFFFFF; after the quads come ``m`` then, per contiguous claim, its quad index, alignment slot (or 0xFFFFFFFF), ``$addr`` slot the run writes the placed address to (or 0xFFFFFFFF; F7.4b), ``k`` and ``k`` (trait field string, slot) pairs (F7.3e). ``sinfer0`` indexes ``n`` then, per state input, its ref string, ``m`` and ``m`` (instance, ``k``, ``k`` hidden node ids) groups (M9a5c); after them, in the same form, the inferred stream partners per unpaired stream reference (M9a6b). */
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
    uint32_t   addr0;  /* u32 list: address claims, 0 = none (F7.3d) */
    uint32_t   infer0;  /* u32 list: inferred for (consumer, ref string, candidate); 0xFFFFFFFF = traversed (M9a3) */
    uint32_t   sinfer0;  /* u32 list: inferred state writers, then stream partners; 0 = none (M9a5c, M9a6b) */
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
    uint32_t   flags;  /* 1 signed, 2 rand, 4 busy_lock, 8 array (a list the constraints read), 16 attr (a resource instance's rand attribute: pool in pick); bits 16-31: busy_word (busy mask word; 63 instances each: 63*w..), or with 16 the attribute's instance */
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

/* zbc_actpick: One unbound buffer input (32 bytes; ACT section). */
typedef struct {
    uint32_t   ref;  /* string */
    uint32_t   slot0;  /* u32 list */
    uint32_t   n_slots;
    uint32_t   sel_slot;
    uint32_t   pool0;  /* u32 list of (inst, pool) pairs */
    uint32_t   n_pools;  /* pairs */
    uint32_t   infer0;  /* u32 list: per instance, (inst, n, n inferred nodes) (M9a3) */
    uint32_t   n_infer;  /* instances */
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

/* zbc_solve: SOLVE descriptor (32 bytes). SOLVE section is zbc_solve[]; arg0 of a SOLVE instr indexes it. The OPLIST pool holds, from oplist_off, n_writeback interleaved (field_slot, var_id) u32 pairs -- the value ABI write-back keyed by object slot -- then n_input more: non-rand fields a constraint reads, each var pinned to its field's value before the solve and not written back (with ZBC_SOLVE_ARRAYS, some are lists: see that flag). When prob_len > 0, the (prob_off, prob_len) slice of the SPROB pool is a relocatable dv-solve SolveProblem blob: the engine compiles + solves it with the drawn seed and writes dvs_solver_get_value(var_id) back to each field_slot. When prob_len == 0 the minimal M1 randomizer applies: slot = seed + var_id (FixedSolveBackend, base 0). Seed is seed_value if SEED_FIXED else the frame's next draw. */
typedef struct {
    uint64_t   seed_value;  /* fixed-seed value (when flags & SEED_FIXED) */
    uint32_t   oplist_off;  /* start of (field_slot, var_id) pairs in OPLIST */
    uint32_t   n_writeback;  /* writeback pair count */
    uint32_t   flags;  /* ZBC_SOLVE_* (bit0 = fixed seed) */
    uint32_t   prob_off;  /* byte offset of the problem blob in the SPROB pool */
    uint32_t   prob_len;  /* problem blob length in bytes (0 = minimal randomizer) */
    uint32_t   n_input;  /* input pair count (after the writeback pairs) */
} zbc_solve;

/* zbc_entry: Entry point (40 bytes). ENTRY section is zbc_entry[]: where a host may enter the image. A function's arguments go in the root frame's args (LD_ARG), one per leaf of each parameter, in the order sig.params lists them; sig is JSON {"params": [[name, type descriptor], ...], "returns": descriptor|null}. An action's object layout (obj) is JSON [[name, type descriptor], ...] in slot order; a rand field's descriptor has "rand": true. */
typedef struct {
    uint32_t   name;  /* StrId: action coroutine name, or <comp type>::<fn> */
    uint32_t   coro;  /* CORO index the host starts */
    uint32_t   kind;  /* ZBC_ENTRY_* */
    uint32_t   comp_type;  /* StrId of a function's component type (0=none) */
    uint32_t   sig;  /* StrId of the JSON signature {params, returns} (0=none) */
    uint32_t   qname;  /* StrId of an action entry's qualified action type (0=none) */
    uint32_t   obj;  /* StrId of the JSON action-object layout (0=none) */
    uint32_t   flags;  /* zbc_entry_flags */
    uint32_t   fixed;  /* StrId of the JSON list of fields an exported action fixes (0=none) */
    uint32_t   _pad;
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
ZBC_STATIC_ASSERT(sizeof(zbc_ident) == 24, "zbc_ident size mismatch");
ZBC_STATIC_ASSERT(sizeof(zbc_metaent) == 8, "zbc_metaent size mismatch");
ZBC_STATIC_ASSERT(sizeof(zbc_impent) == 24, "zbc_impent size mismatch");
ZBC_STATIC_ASSERT(sizeof(zbc_apient) == 16, "zbc_apient size mismatch");
ZBC_STATIC_ASSERT(sizeof(zbc_class) == 40, "zbc_class size mismatch");
ZBC_STATIC_ASSERT(sizeof(zbc_rootent) == 32, "zbc_rootent size mismatch");
ZBC_STATIC_ASSERT(sizeof(zbc_comp) == 16, "zbc_comp size mismatch");
ZBC_STATIC_ASSERT(sizeof(zbc_layout) == 16, "zbc_layout size mismatch");
ZBC_STATIC_ASSERT(sizeof(zbc_covg) == 4, "zbc_covg size mismatch");
ZBC_STATIC_ASSERT(sizeof(zbc_exectab) == 16, "zbc_exectab size mismatch");
ZBC_STATIC_ASSERT(sizeof(zbc_execent) == 12, "zbc_execent size mismatch");
ZBC_STATIC_ASSERT(sizeof(zbc_execgrp) == 16, "zbc_execgrp size mismatch");
ZBC_STATIC_ASSERT(sizeof(zbc_evtent) == 32, "zbc_evtent size mismatch");
ZBC_STATIC_ASSERT(sizeof(zbc_regmap) == 4, "zbc_regmap size mismatch");
ZBC_STATIC_ASSERT(sizeof(zbc_memmap) == 4, "zbc_memmap size mismatch");
ZBC_STATIC_ASSERT(sizeof(zbc_msgtab) == 32, "zbc_msgtab size mismatch");
ZBC_STATIC_ASSERT(sizeof(zbc_msg) == 16, "zbc_msg size mismatch");
ZBC_STATIC_ASSERT(sizeof(zbc_msgarg) == 16, "zbc_msgarg size mismatch");
ZBC_STATIC_ASSERT(sizeof(zbc_msgitem) == 16, "zbc_msgitem size mismatch");
ZBC_STATIC_ASSERT(sizeof(zbc_acttab) == 64, "zbc_acttab size mismatch");
ZBC_STATIC_ASSERT(sizeof(zbc_actinit) == 16, "zbc_actinit size mismatch");
ZBC_STATIC_ASSERT(sizeof(zbc_act) == 80, "zbc_act size mismatch");
ZBC_STATIC_ASSERT(sizeof(zbc_actnode) == 88, "zbc_actnode size mismatch");
ZBC_STATIC_ASSERT(sizeof(zbc_actscope) == 16, "zbc_actscope size mismatch");
ZBC_STATIC_ASSERT(sizeof(zbc_actsite) == 16, "zbc_actsite size mismatch");
ZBC_STATIC_ASSERT(sizeof(zbc_actcone) == 40, "zbc_actcone size mismatch");
ZBC_STATIC_ASSERT(sizeof(zbc_actvar) == 48, "zbc_actvar size mismatch");
ZBC_STATIC_ASSERT(sizeof(zbc_actcon) == 40, "zbc_actcon size mismatch");
ZBC_STATIC_ASSERT(sizeof(zbc_actpool) == 48, "zbc_actpool size mismatch");
ZBC_STATIC_ASSERT(sizeof(zbc_actclaim) == 24, "zbc_actclaim size mismatch");
ZBC_STATIC_ASSERT(sizeof(zbc_actwrite) == 24, "zbc_actwrite size mismatch");
ZBC_STATIC_ASSERT(sizeof(zbc_actpick) == 32, "zbc_actpick size mismatch");
ZBC_STATIC_ASSERT(sizeof(zbc_instr) == 32, "zbc_instr size mismatch");
ZBC_STATIC_ASSERT(sizeof(zbc_coro) == 32, "zbc_coro size mismatch");
ZBC_STATIC_ASSERT(sizeof(zbc_block) == 16, "zbc_block size mismatch");
ZBC_STATIC_ASSERT(sizeof(zbc_select) == 16, "zbc_select size mismatch");
ZBC_STATIC_ASSERT(sizeof(zbc_solve) == 32, "zbc_solve size mismatch");
ZBC_STATIC_ASSERT(sizeof(zbc_entry) == 40, "zbc_entry size mismatch");

#endif /* ZUSPEC_ZBC_FORMAT_H */
