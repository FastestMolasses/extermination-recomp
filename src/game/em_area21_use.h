/* AREA21's Use classifier (boot code, reached only in area 0x15):
 * 001AAC00 and its two tests 001AA8D0 / 001AAA90. 00160220 (the Use
 * dispatcher, em_player_use_dispatch) calls 001AAC00(player,
 * player + 0x290, player + 0x218) after the interaction scan found nothing.
 * Ground truth: the decomp's byte-identical C (src/func_001AAC00.c,
 * func_001AA8D0.c, func_001AAA90.c); evidence:
 * tools/test_area21_use_reference.py, which runs the ORIGINAL instructions.
 *
 * 001AAC00(a0, a1, a2): 0 at once while the halfword D_0028A9A0 is
 *   non-zero. Otherwise walks the published class-4 list (the halfword
 *   count D_00275B84, the word array at D_00275B7C, in order); for each
 *   entry whose byte +0 is 1, byte +3 selects the test: 0x4A -> 001AA8D0,
 *   0x4B -> 001AAA90. The first non-zero test result is returned; 0 when
 *   none.
 * 001AA8D0(a0, e, a1, a2), the ladder placement: 0 unless
 *   fabs(a0.y - e.y) <= 3.0 (+0xA4 / +0xB4). Side 2: the planar distance
 *   from a0 (+0xA0 / +0xA8) to (13 + e.x, e.z) (sqrt of a MULA then MADD
 *   sum) <= 3.5; else side 3 at (e.x - 13, e.z), else 0. Writes a1 =
 *   (e.x + 12.6 or e.x - 12.6, a0.y, e.z, 1.0) and *a2 =
 *   001B1470(pi + e.yaw) (+0xC4); returns the side.
 * 001AAA90(a0, e, a1, a2), the mount: the scratch quadword 0x70003600 =
 *   (0, 0, 8, 1) transformed by e's matrix +0xD0 (001026A0 in place);
 *   0 unless the planar distance from a0 to its x / z (sqrt of MULA then
 *   MADD) <= 8.0 and fabs(a0.y - e.y) <= 4.0. Then 0x70003600 = (0, 0, 8, 1)
 *   again, a1 = 001026A0(e + 0xD0, 0x70003600), *a2 = 001B1470(pi +
 *   e.yaw); returns 1.
 *
 * Same contract as em_area21_owners.h: callees are explicit hooks by
 * original address, memory goes through `bytes` (write 1 for a store), a
 * NULL or failing hook or an unmapped address latches a fault (fail-stop)
 * and the entry returns -1. Float arithmetic is the EE's (em_ee_float.h). */
#ifndef EM_AREA21_USE_H
#define EM_AREA21_USE_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct { uint32_t address; int32_t code; } EmArea21UseFault;
enum { EM_AREA21_USE_FAULT_NONE = 0, EM_AREA21_USE_FAULT_NULL_WORKER = 1,
       EM_AREA21_USE_FAULT_WORKER_FAILED = 2, EM_AREA21_USE_FAULT_BAD_ADDRESS = 5 };
typedef struct {
    void *ctx;
    uint8_t *(*bytes)(void *ctx, uint32_t address, uint32_t size, int write);
    /* 001026A0(out, matrix, vector): out = matrix x vector (VU0). */
    int (*w_001026A0)(void *ctx, uint32_t a0, uint32_t a1, uint32_t a2);
    int (*w_0011DF78)(void *ctx, float a0, float *result);   /* fabsf */
    int (*w_0011E748)(void *ctx, float a0, float *result);   /* sqrtf */
    int (*w_001B1470)(void *ctx, float a0, float *result);   /* the angle wrap */
} EmArea21UseHooks;
/* Each returns 0 with the original's v0 in *result, or -1 on a fault. */
int em_area21_use_001AAC00(const EmArea21UseHooks *h, uint32_t a0, uint32_t a1, uint32_t a2,
                           int32_t *result, EmArea21UseFault *fault);
int em_area21_use_001AA8D0(const EmArea21UseHooks *h, uint32_t a0, uint32_t e, uint32_t a1,
                           uint32_t a2, int32_t *result, EmArea21UseFault *fault);
int em_area21_use_001AAA90(const EmArea21UseHooks *h, uint32_t a0, uint32_t e, uint32_t a1,
                           uint32_t a2, int32_t *result, EmArea21UseFault *fault);
#ifdef __cplusplus
}
#endif
#endif
