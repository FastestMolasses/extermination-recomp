/* The AREA21 creature (AREA21 code group 0x82A5A0 [0]): its behaviour
 * 00151590, the states 001516B0 / 00151770 / 00152CC0 / 00152E80, the moves
 * 00151940 .. 00152B60, the per-frame pieces 00152EB0 / 00152F20 / 00153180
 * / 00153770, the spawn 00153290 and the spawned pieces' behaviour 00153540.
 * Same contract as em_area17_creature.h: callees are explicit hooks, memory
 * goes through `bytes`, a NULL or failing hook latches a fault (fail-stop).
 * Entries that take `sp` get the original stack pointer (their frame locals
 * live below it). Evidence: tools/test_area21_creature_c_reference.py. */
#ifndef EM_AREA21_CREATURE_C_H
#define EM_AREA21_CREATURE_C_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct { uint32_t address; int32_t code; } EmArea21CreatureCFault;
enum { EM_AREA21_CREATURE_C_FAULT_NONE = 0, EM_AREA21_CREATURE_C_FAULT_NULL_WORKER = 1,
       EM_AREA21_CREATURE_C_FAULT_WORKER_FAILED = 2, EM_AREA21_CREATURE_C_FAULT_BAD_ADDRESS = 5 };
typedef struct {
    void *ctx;
    uint8_t *(*bytes)(void *ctx, uint32_t address, uint32_t size);
/* BEGIN GENERATED HOOKS */
    int (*w_001026A0)(void *ctx, uint32_t a0, uint32_t a1, uint32_t a2);
    int (*w_001028B8)(void *ctx, uint32_t a0, uint32_t a1, uint32_t a2);
    int (*w_00102948)(void *ctx, uint32_t a0, uint32_t a1);
    int (*w_00102958)(void *ctx, uint32_t a0, uint32_t a1);
    int (*w_001029C0)(void *ctx, uint32_t a0);
    int (*w_00102B08)(void *ctx, uint32_t a0, uint32_t a1, float a2);
    int (*w_00102BB0)(void *ctx, uint32_t a0, uint32_t a1, float a2);
    int (*w_001031E0)(void *ctx, uint32_t a0, uint32_t a1);
    int (*w_00103230)(void *ctx, uint32_t a0, uint32_t a1, float a2);
    int (*w_0011DE90)(void *ctx, float a0, float *result);
    int (*w_0011DF78)(void *ctx, float a0, float *result);
    int (*w_0011E0A8)(void *ctx, uint32_t a0, float a1, float *result);
    int (*w_0011E2A8)(void *ctx, float a0, float *result);
    int (*w_00122BB8)(void *ctx, int32_t *result);
    int (*w_001281C0)(void *ctx, float a0, int32_t *result);
    int (*w_001283D0)(void *ctx, uint32_t a0, int32_t a1, uint32_t a2, int32_t *result);
    int (*w_0014D1E0)(void *ctx, uint32_t a0, uint32_t a1, int32_t a2);
    int (*w_0019A570)(void *ctx, uint32_t a0, uint32_t a1, int32_t a2, int32_t a3, int32_t *result);
    int (*w_001A7B80)(void *ctx, uint32_t a0, int32_t *result);
    int (*w_001AFA90)(void *ctx, int32_t a0, uint32_t *result);
    int (*w_001AFC10)(void *ctx, uint32_t a0);
    int (*w_001B10B0)(void *ctx, uint32_t a0, int32_t a1, int32_t a2, int32_t *result);
    int (*w_001B1190)(void *ctx, int32_t a0);
    int (*w_001B1240)(void *ctx, uint32_t a0, float a1, float a2, float *result);
    int (*w_001B12B0)(void *ctx, float a0, float a1, float a2, float *result);
    int (*w_001B1380)(void *ctx, uint32_t a0, uint32_t a1, float a2, int32_t *result);
    int (*w_001B1470)(void *ctx, float a0, float *result);
    int (*w_001B1560)(void *ctx, uint32_t a0, uint32_t a1, float a2, int32_t *result);
    int (*w_001B15D0)(void *ctx, uint32_t a0, uint32_t a1, float *result);
    int (*w_001B17A0)(void *ctx, uint32_t a0, int32_t *result);
    int (*w_001B2B10)(void *ctx, uint32_t a0, uint32_t a1, uint32_t a2);
    int (*w_001B4810)(void *ctx, uint32_t a0);
    int (*w_001B4CF0)(void *ctx, uint32_t a0);
    int (*w_001B55E0)(void *ctx, uint32_t a0, int32_t a1);
    int (*w_001C6160)(void *ctx, uint32_t a0, int32_t *result);
    int (*w_001C63E0)(void *ctx, uint32_t a0, int32_t a1);
    int (*w_001C64F0)(void *ctx, uint32_t a0, float a1, int32_t *result);
    int (*w_001C67E0)(void *ctx, uint32_t a0, int32_t a1, float a2, float a3);
    int (*w_001C68C0)(void *ctx, uint32_t a0);
    int (*w_001C94B0)(void *ctx, uint32_t a0, uint32_t a1, uint32_t a2, uint32_t a3);
    int (*w_001EFD20)(void *ctx, int32_t a0, uint32_t a1);
    int (*w_001EFD90)(void *ctx, int32_t a0, uint32_t a1, uint32_t a2);
    int (*w_001EFE00)(void *ctx, int32_t a0, uint32_t a1, int32_t *result);
    int (*w_001F91C0)(void *ctx, uint32_t a0);
    int (*w_001FBD50)(void *ctx, uint32_t a0, int32_t a1, int32_t a2, float a3, int32_t *result);
    int (*w_0021BE40)(void *ctx, uint32_t a0, uint32_t a1, int32_t *result);
/* END GENERATED HOOKS */
    int (*w_callback)(void *ctx, uint32_t function, uint32_t self);
} EmArea21CreatureCHooks;
int em_area21_creature_c_00151590(const EmArea21CreatureCHooks *h, uint32_t a0, uint32_t sp, EmArea21CreatureCFault *fault);
int em_area21_creature_c_001516B0(const EmArea21CreatureCHooks *h, uint32_t a0, uint32_t a1, EmArea21CreatureCFault *fault);
int em_area21_creature_c_00151770(const EmArea21CreatureCHooks *h, uint32_t a0, uint32_t a1, uint32_t sp, EmArea21CreatureCFault *fault);
int em_area21_creature_c_00151940(const EmArea21CreatureCHooks *h, uint32_t a0, uint32_t a1, EmArea21CreatureCFault *fault);
int em_area21_creature_c_00151E50(const EmArea21CreatureCHooks *h, uint32_t a0, uint32_t a1, EmArea21CreatureCFault *fault);
int em_area21_creature_c_00152040(const EmArea21CreatureCHooks *h, uint32_t a0, uint32_t a1, EmArea21CreatureCFault *fault);
int em_area21_creature_c_00152330(const EmArea21CreatureCHooks *h, uint32_t a0, uint32_t a1, EmArea21CreatureCFault *fault);
int em_area21_creature_c_00152660(const EmArea21CreatureCHooks *h, uint32_t a0, uint32_t a1, EmArea21CreatureCFault *fault);
int em_area21_creature_c_00152930(const EmArea21CreatureCHooks *h, uint32_t a0, uint32_t a1, EmArea21CreatureCFault *fault);
int em_area21_creature_c_00152B60(const EmArea21CreatureCHooks *h, uint32_t a0, uint32_t a1, EmArea21CreatureCFault *fault);
int em_area21_creature_c_00152CC0(const EmArea21CreatureCHooks *h, uint32_t a0, uint32_t a1, EmArea21CreatureCFault *fault);
int em_area21_creature_c_00152E80(const EmArea21CreatureCHooks *h, uint32_t a0, EmArea21CreatureCFault *fault);
int em_area21_creature_c_00152EB0(const EmArea21CreatureCHooks *h, uint32_t a0, EmArea21CreatureCFault *fault);
int em_area21_creature_c_00152F20(const EmArea21CreatureCHooks *h, uint32_t a0, uint32_t a1, uint32_t sp, EmArea21CreatureCFault *fault);
int em_area21_creature_c_00153180(const EmArea21CreatureCHooks *h, uint32_t a0, uint32_t a1, EmArea21CreatureCFault *fault);
int em_area21_creature_c_00153290(const EmArea21CreatureCHooks *h, uint32_t a0, EmArea21CreatureCFault *fault);
int em_area21_creature_c_00153540(const EmArea21CreatureCHooks *h, uint32_t a0, EmArea21CreatureCFault *fault);
int em_area21_creature_c_00153770(const EmArea21CreatureCHooks *h, uint32_t a0, uint32_t a1, int32_t *result, EmArea21CreatureCFault *fault);
#ifdef __cplusplus
}
#endif
#endif
