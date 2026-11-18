/* AREA16's water grid (lane PORT-WATER): the boot functions AREA16 sub 0's
 * [10] 0x825800 calls for the water barrier. 001DAFA0 (the grid's init),
 * 001DB240 (its stage byte 0x81C011), 001DB250 (the per-frame stage step,
 * which runs the wave step 001DB040), 001DB040 (NEARMISS in the decomp)
 * and 001DB480 (the grid's GS display list, with its vertex emit 001DAE10
 * and vertex record 001DAC90 as internal translations). The grid is the
 * 32 x 32 cells of 0x10 bytes at 0x818000 and its parameter block at
 * 0x81C000.
 *
 * Same contract as em_area17_creature.h: callees are explicit hooks,
 * memory goes through `bytes` by original address, a NULL or failing hook
 * latches a fault (fail-stop). 001DB480 takes `sp`, the original stack
 * pointer at its entry (its frame locals live below it). Evidence:
 * tools/test_area16_water_reference.py. */
#ifndef EM_AREA16_WATER_H
#define EM_AREA16_WATER_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct { uint32_t address; int32_t code; } EmArea16WaterFault;
/* UNMEASURED: a VU0 form outside the measured float model, or a clip lane
 * with exponent 255 (001DAE10's clip test; not modelled). */
enum { EM_AREA16_WATER_FAULT_NONE = 0, EM_AREA16_WATER_FAULT_NULL_WORKER = 1,
       EM_AREA16_WATER_FAULT_WORKER_FAILED = 2, EM_AREA16_WATER_FAULT_BAD_ADDRESS = 5,
       EM_AREA16_WATER_FAULT_UNMEASURED = 6 };
typedef struct {
    void *ctx;
    uint8_t *(*bytes)(void *ctx, uint32_t address, uint32_t size);
/* BEGIN GENERATED HOOKS */
    int (*w_001026A0)(void *ctx, uint32_t a0, uint32_t a1, uint32_t a2);
    int (*w_00102948)(void *ctx, uint32_t a0, uint32_t a1);
    int (*w_00122BB8)(void *ctx, int32_t *result);
    int (*w_001281C0)(void *ctx, float a0, int32_t *result);
    int (*w_001CAAC0)(void *ctx, uint32_t a0, uint32_t a1);
    int (*w_001CD370)(void *ctx, int32_t a0, uint32_t *result);
    int (*w_001D1F20)(void *ctx, int32_t a0);
    int (*w_001D1FF0)(void *ctx, int32_t a0, int32_t a1);
    int (*w_001D6B10)(void *ctx, int32_t a0, int32_t a1, int32_t a2, int32_t a3);
    int (*w_001D6BA0)(void *ctx, int32_t a0, int32_t a1, int32_t a2, int32_t a3, int32_t a4, int32_t a5);
    int (*w_001D6C90)(void *ctx, int32_t a0, int32_t a1, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7);
/* END GENERATED HOOKS */
} EmArea16WaterHooks;
int em_area16_water_001DAFA0(const EmArea16WaterHooks *h, EmArea16WaterFault *fault);
int em_area16_water_001DB240(const EmArea16WaterHooks *h, int32_t a0, EmArea16WaterFault *fault);
int em_area16_water_001DB040(const EmArea16WaterHooks *h, uint32_t a0, EmArea16WaterFault *fault);
int em_area16_water_001DB250(const EmArea16WaterHooks *h, EmArea16WaterFault *fault);
int em_area16_water_001DB480(const EmArea16WaterHooks *h, uint32_t sp, EmArea16WaterFault *fault);
#ifdef __cplusplus
}
#endif
#endif
