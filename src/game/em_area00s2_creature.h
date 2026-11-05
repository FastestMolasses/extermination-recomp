/* AREA00 sub 2's creature (the record the owner 0x825D70 spawns, group
 * 0x827EF0; constructed start k01): the per-frame behaviour 0014D260, its
 * set-up 0014D3B0 (state 0), the alive state 0014D470 (state 1) with its
 * moves 0014D5F0 / 0014D7C0 / 0014DC30 / 0014E050 / 0014E2C0, the scripted
 * state 0014E370 (state 2) with 0014E420 / 0014E4F0, the end 0014E640
 * (state 3), and the per-frame pieces 0014E670, 0014E7A0, 0014E890 and
 * 0014E9C0. Same contract as em_area21_creature_b.h: callees are explicit
 * hooks, memory goes through `bytes`, a NULL or failing hook latches a
 * fault (fail-stop). `sp` is the original entry stack pointer (0014E890
 * keeps a frame local below it). Evidence:
 * tools/test_area00s2_creature_reference.py. */
#ifndef EM_AREA00S2_CREATURE_H
#define EM_AREA00S2_CREATURE_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct { uint32_t address; int32_t code; } EmArea00s2CreatureFault;
enum { EM_AREA00S2_CREATURE_FAULT_NONE = 0, EM_AREA00S2_CREATURE_FAULT_NULL_WORKER = 1,
       EM_AREA00S2_CREATURE_FAULT_WORKER_FAILED = 2, EM_AREA00S2_CREATURE_FAULT_BAD_ADDRESS = 5 };
typedef struct {
    void *ctx;
    uint8_t *(*bytes)(void *ctx, uint32_t address, uint32_t size);
/* BEGIN GENERATED HOOKS */
    int (*w_001026A0)(void *ctx, uint32_t a0, uint32_t a1, uint32_t a2);
    int (*w_00102760)(void *ctx, uint32_t a0, uint32_t a1);
    int (*w_00102948)(void *ctx, uint32_t a0, uint32_t a1);
    int (*w_00102958)(void *ctx, uint32_t a0, uint32_t a1);
    int (*w_0011E0A8)(void *ctx, uint32_t a0, float a1, float *result);
    int (*w_0011E620)(void *ctx, float a0, float a1, float *result);
    int (*w_00122BB8)(void *ctx, int32_t *result);
    int (*w_001281C0)(void *ctx, float a0, int32_t *result);
    int (*w_001A7B80)(void *ctx, uint32_t a0, int32_t *result);
    int (*w_001AFC10)(void *ctx, uint32_t a0);
    int (*w_001B10B0)(void *ctx, uint32_t a0, int32_t a1, int32_t a2, int32_t *result);
    int (*w_001B1190)(void *ctx, int32_t a0);
    int (*w_001B12B0)(void *ctx, float a0, float a1, float a2, float *result);
    int (*w_001B1470)(void *ctx, float a0, float *result);
    int (*w_001B15D0)(void *ctx, uint32_t a0, uint32_t a1, float *result);
    int (*w_001B17A0)(void *ctx, uint32_t a0, int32_t *result);
    int (*w_001B4810)(void *ctx, uint32_t a0);
    int (*w_001B4CF0)(void *ctx, uint32_t a0);
    int (*w_001B55E0)(void *ctx, uint32_t a0, int32_t a1);
    int (*w_001C63E0)(void *ctx, uint32_t a0, int32_t a1);
    int (*w_001C64F0)(void *ctx, uint32_t a0, float a1, int32_t *result);
    int (*w_001C67E0)(void *ctx, uint32_t a0, int32_t a1, float a2, float a3);
    int (*w_001C68C0)(void *ctx, uint32_t a0);
    int (*w_001EFD90)(void *ctx, int32_t a0, uint32_t a1, uint32_t a2);
    int (*w_001EFE00)(void *ctx, int32_t a0, uint32_t a1, int32_t *result);
    int (*w_001F91C0)(void *ctx, uint32_t a0);
    int (*w_001FBD50)(void *ctx, uint32_t a0, int32_t a1, int32_t a2, float a3, int32_t *result);
    int (*w_0021BE40)(void *ctx, uint32_t a0, uint32_t a1, int32_t *result);
/* END GENERATED HOOKS */
    int (*w_callback)(void *ctx, uint32_t function, uint32_t self);
} EmArea00s2CreatureHooks;
int em_area00s2_creature_0014D260(const EmArea00s2CreatureHooks *h, uint32_t a0, uint32_t sp, EmArea00s2CreatureFault *fault);
int em_area00s2_creature_0014D3B0(const EmArea00s2CreatureHooks *h, uint32_t a0, uint32_t a1, EmArea00s2CreatureFault *fault);
int em_area00s2_creature_0014D470(const EmArea00s2CreatureHooks *h, uint32_t a0, uint32_t a1, uint32_t sp, EmArea00s2CreatureFault *fault);
int em_area00s2_creature_0014D5F0(const EmArea00s2CreatureHooks *h, uint32_t a0, uint32_t a1, EmArea00s2CreatureFault *fault);
int em_area00s2_creature_0014D7C0(const EmArea00s2CreatureHooks *h, uint32_t a0, uint32_t a1, EmArea00s2CreatureFault *fault);
int em_area00s2_creature_0014DC30(const EmArea00s2CreatureHooks *h, uint32_t a0, uint32_t a1, EmArea00s2CreatureFault *fault);
int em_area00s2_creature_0014E050(const EmArea00s2CreatureHooks *h, uint32_t a0, uint32_t a1, EmArea00s2CreatureFault *fault);
int em_area00s2_creature_0014E2C0(const EmArea00s2CreatureHooks *h, uint32_t a0, uint32_t a1, EmArea00s2CreatureFault *fault);
int em_area00s2_creature_0014E370(const EmArea00s2CreatureHooks *h, uint32_t a0, uint32_t a1, uint32_t sp, EmArea00s2CreatureFault *fault);
int em_area00s2_creature_0014E420(const EmArea00s2CreatureHooks *h, uint32_t a0, uint32_t a1, EmArea00s2CreatureFault *fault);
int em_area00s2_creature_0014E4F0(const EmArea00s2CreatureHooks *h, uint32_t a0, uint32_t a1, EmArea00s2CreatureFault *fault);
int em_area00s2_creature_0014E640(const EmArea00s2CreatureHooks *h, uint32_t a0, EmArea00s2CreatureFault *fault);
int em_area00s2_creature_0014E670(const EmArea00s2CreatureHooks *h, uint32_t a0, uint32_t a1, EmArea00s2CreatureFault *fault);
int em_area00s2_creature_0014E7A0(const EmArea00s2CreatureHooks *h, uint32_t a0, uint32_t a1, EmArea00s2CreatureFault *fault);
int em_area00s2_creature_0014E890(const EmArea00s2CreatureHooks *h, uint32_t a0, uint32_t a1, uint32_t sp, EmArea00s2CreatureFault *fault);
int em_area00s2_creature_0014E9C0(const EmArea00s2CreatureHooks *h, uint32_t a0, uint32_t a1, int32_t *result, EmArea00s2CreatureFault *fault);
#ifdef __cplusplus
}
#endif
#endif
