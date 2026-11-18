/* The effect handler 001EB440 (D_00255434's handler of the AREA21 effect
 * first reached in the guided run's g77, phase 1 of the boss): two passes of
 * one transformed sprite between fog / depth-range settings.
 *
 * 001EB440(a0, a1): 0021B9A0(2, 0, 0) and 0021B9A0(3, 0, 2000); then twice
 * (sources D_00255C50, then D_00255CE0): with w the work block D_00275C34,
 * r = w[+4], f = (float)((r >> 16) & 0xFFFF) / 65535, w[+4] = r * 0x25 + 0xB,
 * 001CFB50(D_0081F8F0, 0, a0, w[+0x54], f + 0.0001, 1.0, 1e-6, 15.0) and
 * 001CFBE0(a1, 5, source, D_0081F8F0, 0); finally 0021B9A0(1, 0, 0).
 * 001EB250(a0, a1), its twin (first reached in g77 too): first, unless
 * w[+0x54] <= 0.8, the word a0 + 0x38 (the node matrix's translation z)
 * -= 2.0; then the same with the sources D_00255B30 / D_00255BC0.
 * Ground truth: the decomp's byte-identical C (src/func_001EB440.c,
 * func_001EB250.c);
 * evidence: tools/test_effect_001EB440_reference.py (the ORIGINAL
 * instructions). Float arithmetic is the EE's (em_ee_float.h).
 *
 * Contract: memory through `bytes` (write 1 for a store); callees are hooks
 * by original address with float arguments as bits. A NULL or failing hook
 * or an unmapped address latches a fault and the entry returns -1. */
#ifndef EM_EFFECT_001EB440_H
#define EM_EFFECT_001EB440_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct { uint32_t address; int32_t code; } EmEffect001EB440Fault;
enum { EM_EFFECT_001EB440_FAULT_NULL_WORKER = 1, EM_EFFECT_001EB440_FAULT_WORKER_FAILED = 2,
       EM_EFFECT_001EB440_FAULT_BAD_ADDRESS = 5 };
typedef struct {
    void *ctx;
    uint8_t *(*bytes)(void *ctx, uint32_t address, uint32_t size, int write);
    int (*w_0021B9A0)(void *ctx, int32_t mode, uint32_t f12, uint32_t f13);
    int (*w_001CFB50)(void *ctx, uint32_t a0, int32_t a1, uint32_t a2, uint32_t f12, uint32_t f13,
                      uint32_t f14, uint32_t f15, uint32_t f16);
    int (*w_001CFBE0)(void *ctx, int32_t a0, int32_t a1, uint32_t a2, uint32_t a3, int32_t a4);
} EmEffect001EB440Hooks;
int em_effect_001EB440(const EmEffect001EB440Hooks *h, uint32_t a0, int32_t a1,
                       EmEffect001EB440Fault *fault);
int em_effect_001EB250(const EmEffect001EB440Hooks *h, uint32_t a0, int32_t a1,
                       EmEffect001EB440Fault *fault);
#ifdef __cplusplus
}
#endif
#endif
