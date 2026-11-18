/* AREA21's group members (OVERLAY/AREA21.BIN, header id 0x12; docs of the
 * decomp: docs/AREA21_OVERLAY.md): the members of the groups 0x82B500 /
 * 0x82BA70 that the scripts 0x82B5E0 / 0x82C5C0 place, 0x823580, 0x8237B0,
 * 0x823860, 0x8238E0 and the empty 0x823AA0 (their 0x823A10 / 0x823AB0 are
 * in em_level17_boot.c), and 0x824CF0, which no table reaches (unreached
 * in the original: no static reference). Overlay addresses are runtime
 * addresses (the decomp's link names are 0x40 lower). Ground truth: the
 * decomp's byte-identical C under src/overlays/AREA21/; evidence:
 * tools/test_area21_groups_reference.py, which runs the ORIGINAL overlay
 * code (the user's extract/OVERLAY/AREA21.BIN over a recording's arena).
 *
 * Same contract as em_area14_overlay.h: callees are explicit hooks by
 * original address, memory goes through `bytes`, a NULL or failing hook or
 * an unmapped address latches a fault (fail-stop) and the entry returns -1.
 * No entry calls a node's +0x4C method; w_callback is there for the hook
 * table shape the AREA21 modules share and is never called. Entries with a
 * frame local take the original stack pointer `sp` at entry: the local
 * lives at its original address below it. */
#ifndef EM_AREA21_GROUPS_H
#define EM_AREA21_GROUPS_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct { uint32_t address; int32_t code; } EmArea21GroupsFault;
enum { EM_AREA21_GROUPS_FAULT_NONE = 0, EM_AREA21_GROUPS_FAULT_NULL_WORKER = 1,
       EM_AREA21_GROUPS_FAULT_WORKER_FAILED = 2, EM_AREA21_GROUPS_FAULT_BAD_ADDRESS = 5 };
typedef struct {
    void *ctx;
    uint8_t *(*bytes)(void *ctx, uint32_t address, uint32_t size);
/* BEGIN GENERATED HOOKS */
    int (*w_00102918)(void *ctx, uint32_t a0, uint32_t a1, uint32_t a2);
    int (*w_00102948)(void *ctx, uint32_t a0, uint32_t a1);
    int (*w_001029C0)(void *ctx, uint32_t a0);
    int (*w_00102C58)(void *ctx, uint32_t a0, uint32_t a1, uint32_t a2);
    int (*w_0011DE90)(void *ctx, float a0, float *result);
    int (*w_00122BB8)(void *ctx, int32_t *result);
    int (*w_001AFC10)(void *ctx, uint32_t a0);
    int (*w_001C6190)(void *ctx, uint32_t a0, int32_t *result);
    int (*w_001CCF70)(void *ctx, uint32_t a0, int32_t *result);
    int (*w_001CD520)(void *ctx, int32_t a0, int32_t a1, uint32_t a2, uint64_t a3, uint32_t a4, float a5, float a6, float a7);
    int (*w_001CFB50)(void *ctx, uint32_t a0, int32_t a1, uint32_t a2, float a3, float a4, float a5, float a6, float a7);
    int (*w_001CFBE0)(void *ctx, int32_t a0, int32_t a1, uint32_t a2, uint32_t a3, int32_t a4);
    int (*w_001EFD20)(void *ctx, uint32_t a0, uint32_t a1);
/* END GENERATED HOOKS */
    int (*w_callback)(void *ctx, uint32_t function, uint32_t self);
} EmArea21GroupsHooks;
/* Group members: one call of the original behaviour on node a0. Return 0,
 * or -1 on a fault. 0x823580 keeps its effect position (three floats) at
 * sp - 0x10; 0x824CF0 its packet at sp - 0x60 (0x60 bytes). */
int em_area21_groups_00823580(const EmArea21GroupsHooks *h, uint32_t a0, uint32_t sp, EmArea21GroupsFault *fault);
int em_area21_groups_008237B0(const EmArea21GroupsHooks *h, uint32_t a0, EmArea21GroupsFault *fault);
int em_area21_groups_00823860(const EmArea21GroupsHooks *h, uint32_t a0, EmArea21GroupsFault *fault);
int em_area21_groups_008238E0(const EmArea21GroupsHooks *h, uint32_t a0, EmArea21GroupsFault *fault);
/* 0x823AA0: an empty function (it reads and writes nothing; a0 is
 * ignored). */
int em_area21_groups_00823AA0(const EmArea21GroupsHooks *h, uint32_t a0, EmArea21GroupsFault *fault);
int em_area21_groups_00824CF0(const EmArea21GroupsHooks *h, uint32_t a0, uint32_t sp, EmArea21GroupsFault *fault);
#ifdef __cplusplus
}
#endif
#endif
