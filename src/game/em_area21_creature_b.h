/* AREA21's creature (AREA21 code group 0x82A540 [0]): the per-frame
 * behaviour 0014EC00, its set-up 0014ED10, the alive state 0014EDD0 with its
 * moves 0014EEF0 / 0014F390 / 0014FBC0, the second alive form 00150190 with
 * 00150260 / 001504D0 / 00150600, the end 00150820, and the per-frame pieces
 * 00150850, 00150900, 001509A0, 00150DA0, 00150F40, 00151200, 001513B0.
 * Same contract as em_area17_creature.h: callees are explicit hooks, memory
 * goes through `bytes`, a NULL or failing hook latches a fault (fail-stop).
 * Evidence: tools/test_area21_creature_b_reference.py. */
#ifndef EM_AREA21_CREATURE_B_H
#define EM_AREA21_CREATURE_B_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct { uint32_t address; int32_t code; } EmArea21CreatureBFault;
enum { EM_AREA21_CREATURE_B_FAULT_NONE = 0, EM_AREA21_CREATURE_B_FAULT_NULL_WORKER = 1,
       EM_AREA21_CREATURE_B_FAULT_WORKER_FAILED = 2, EM_AREA21_CREATURE_B_FAULT_BAD_ADDRESS = 5 };
typedef struct {
    void *ctx;
    uint8_t *(*bytes)(void *ctx, uint32_t address, uint32_t size);
/* BEGIN GENERATED HOOKS */
    int (*w_001026A0)(void *ctx, uint32_t a0, uint32_t a1, uint32_t a2);
    int (*w_001028B8)(void *ctx, uint32_t a0, uint32_t a1, uint32_t a2);
    int (*w_00102948)(void *ctx, uint32_t a0, uint32_t a1);
    int (*w_00102958)(void *ctx, uint32_t a0, uint32_t a1);
    int (*w_001029C0)(void *ctx, uint32_t a0);
    int (*w_00102BB0)(void *ctx, uint32_t a0, uint32_t a1, float a2);
    int (*w_0011DE90)(void *ctx, float a0, float *result);
    int (*w_0011DF78)(void *ctx, float a0, float *result);
    int (*w_0011E2A8)(void *ctx, float a0, float *result);
    int (*w_00122BB8)(void *ctx, int32_t *result);
    int (*w_001283D0)(void *ctx, uint32_t a0, int32_t a1, uint32_t a2, int32_t *result);
    int (*w_0014D1E0)(void *ctx, uint32_t a0, uint32_t a1, int32_t a2);
    int (*w_001AFC10)(void *ctx, uint32_t a0);
    int (*w_001B10B0)(void *ctx, uint32_t a0, int32_t a1, int32_t a2, int32_t *result);
    int (*w_001B1190)(void *ctx, int32_t a0);
    int (*w_001B1240)(void *ctx, uint32_t a0, float a1, float a2, float *result);
    int (*w_001B12B0)(void *ctx, float a0, float a1, float a2, float *result);
    int (*w_001B1470)(void *ctx, float a0, float *result);
    int (*w_001B15D0)(void *ctx, uint32_t a0, uint32_t a1, float *result);
    int (*w_001B17A0)(void *ctx, uint32_t a0, int32_t *result);
    int (*w_001B1EA0)(void *ctx, int32_t a0, uint32_t a1, uint32_t a2, int32_t a3, int32_t *result);
    int (*w_001B4810)(void *ctx, uint32_t a0);
    int (*w_001B4CF0)(void *ctx, uint32_t a0);
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
} EmArea21CreatureBHooks;
int em_area21_creature_b_0014EC00(const EmArea21CreatureBHooks *h, uint32_t a0, EmArea21CreatureBFault *fault);
int em_area21_creature_b_0014ED10(const EmArea21CreatureBHooks *h, uint32_t a0, uint32_t a1, EmArea21CreatureBFault *fault);
int em_area21_creature_b_0014EDD0(const EmArea21CreatureBHooks *h, uint32_t a0, uint32_t a1, EmArea21CreatureBFault *fault);
int em_area21_creature_b_0014EEF0(const EmArea21CreatureBHooks *h, uint32_t a0, uint32_t a1, EmArea21CreatureBFault *fault);
int em_area21_creature_b_0014F390(const EmArea21CreatureBHooks *h, uint32_t a0, uint32_t a1, EmArea21CreatureBFault *fault);
int em_area21_creature_b_0014FBC0(const EmArea21CreatureBHooks *h, uint32_t a0, uint32_t a1, EmArea21CreatureBFault *fault);
int em_area21_creature_b_00150190(const EmArea21CreatureBHooks *h, uint32_t a0, uint32_t a1, EmArea21CreatureBFault *fault);
int em_area21_creature_b_00150260(const EmArea21CreatureBHooks *h, uint32_t a0, uint32_t a1, EmArea21CreatureBFault *fault);
int em_area21_creature_b_001504D0(const EmArea21CreatureBHooks *h, uint32_t a0, uint32_t a1, EmArea21CreatureBFault *fault);
int em_area21_creature_b_00150600(const EmArea21CreatureBHooks *h, uint32_t a0, uint32_t a1, EmArea21CreatureBFault *fault);
int em_area21_creature_b_00150820(const EmArea21CreatureBHooks *h, uint32_t a0, EmArea21CreatureBFault *fault);
int em_area21_creature_b_00150850(const EmArea21CreatureBHooks *h, uint32_t a0, EmArea21CreatureBFault *fault);
int em_area21_creature_b_00150900(const EmArea21CreatureBHooks *h, uint32_t a0, uint32_t a1, EmArea21CreatureBFault *fault);
int em_area21_creature_b_001509A0(const EmArea21CreatureBHooks *h, uint32_t a0, uint32_t a1, EmArea21CreatureBFault *fault);
int em_area21_creature_b_00150DA0(const EmArea21CreatureBHooks *h, uint32_t a0, uint32_t a1, EmArea21CreatureBFault *fault);
int em_area21_creature_b_00150F40(const EmArea21CreatureBHooks *h, uint32_t a0, uint32_t a1, EmArea21CreatureBFault *fault);
int em_area21_creature_b_00151200(const EmArea21CreatureBHooks *h, uint32_t a0, uint32_t a1, EmArea21CreatureBFault *fault);
int em_area21_creature_b_001513B0(const EmArea21CreatureBHooks *h, uint32_t a0, uint32_t a1, int32_t *result, EmArea21CreatureBFault *fault);
#ifdef __cplusplus
}
#endif
#endif
