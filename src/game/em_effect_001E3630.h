/* em_effect_001E3630.h - the effect node behaviour 001E3630 (with its empty
 * contact callback 001E3620) and the effect-driver handler 001ECE70.
 *
 * Who spawns it: the gun node 00188630 with sub-weapon 4 (D_00810525, the
 * player record's +0x275) and a pending transition (node +0x2E) calls
 * 001EFEB0(0x80000039, 0x700036A0); the global effect record 0x39
 * (0x257C90 + 0x39 * 0x30) names 001E3630 as its behaviour, which 001EF9D0
 * copies to the new node's +0x10. Sub-weapon 4's ammunition is the fuel
 * counter D_00810CAE (the status screen's attachment 4, fuel percent): this
 * is the flamethrower's burst. The only spawner of 0x8000001A (effect-driver
 * subtype 0x10, handler 001ECE70) is this node's retry window below.
 * Translated from the byte-matched decomp C (src/func_001E3630.c,
 * src/func_001ECE70.c, src/func_001E3620.c); every float operation and its
 * operand order are the original's. Docs: docs/EFFECT_MANAGER.md section 8.
 *
 * 001E3630(node), frame 0xB0 (a 0x60-byte block at frame +0x50).
 * w = node + 0x1F0 (the node's own work block); +0x14 is the pool's self
 * word, so +0x30 = w.
 *   state 0  00102948(+0xB0, +0x100); copy_qw4(w+0x20, +0xD0); +0x20 = 0;
 *            +0x30 = (+0x14) + 0x1F0; +0x34 = 001E3620; w+0x14 = 0;
 *            w+0xA0, w+0xA4 = rand() / 2^31 (two draws); w+0xA8 = 0;
 *            +0 = +4 = 1. With P = D_008102D0 (the player record's
 *            pointer word) + 0xA0: if 0019A570(P, +0x100, 6, 0) |
 *            0019B6C0(P, +0x100) is nonzero, w+0x1C = 0, w+0x18 = 0.0 and
 *            the call ends. Otherwise 0x700038A0 = (0, 0, 50, 1),
 *            001026A0(0x700038A0, w+0x20, 0x700038A0), w+0x1C = 0; when
 *            0019B6C0(w+0x50, 0x700038A0) hits a record (0x700031D0) whose
 *            byte +0x1A is 0x5B: 001028D0(0x700038A0, 0x700031B0, w+0x50),
 *            w+0x18 = sqrt(x*x + y*y + z*z) / 50 (the sum as ADDA then
 *            MADD), w+0x1C = 0x63. While w+0x1C is 0: w+0x1C =
 *            0019A570(w+0x50, 0x700038A0, 6, 0); on a hit the same w+0x18
 *            from 001028D0, then 0x700038A0..A8 = the record's +0x24..+0x2C,
 *            001CD390(w+0x60, 0x700038A0), 00102918(w+0x60, w+0x60,
 *            0x700031B0); on a miss w+0x18 = 1.0. On into state 1.
 *   state 1  001F0190(-145, 450); k = 001CCF70(w+0x50); D_00253BC0..CC =
 *            (1, 1, 30 * w+0x18, 1); 001CFA60(block, w+0x20, w+0x14,
 *            w+0xA4); 001CFBE0(k, 0, D_00253BC0, block, 1); k2 =
 *            001CCF70(+0x100); 001CFA60(block, +0xD0, w+0x14, w+0xA0);
 *            001CFBE0(k2, 1, D_00253B30, block, 0); 001CFBE0(k2, 1,
 *            D_00253AA0, block, 0); 001F0290(). w+0x14 += 0.04; past 1.7
 *            (not <= 1.7) +0 = 2, +4 = 3 and the call ends. Past 1.0, once
 *            (w+0xA8 0 -> 1), when w+0x18 is not 1.0 and w+0x1C is 4: for
 *            the sides (-4, -4, -2, 1) and (4, 4, -2, 1) at 0x700038A0,
 *            001026A0(0x700038A0, w+0x60, 0x700038A0) and 0019A570(w+0x50,
 *            0x700038A0, 4, 0), stopping at the first miss; when both hit,
 *            001EFD20(0x8000001A, w+0x90) and 001F0460(6, w+0x60). Then
 *            copy_qw4(0x700036A0, +0xD0), 0x700036D0..D8 = 0, 0x700036DC =
 *            1.0, 0x700038A0 = (0, 0, 50 * w+0x18 / 42.500004, 1),
 *            001026A0(0x700038A0, 0x700036A0, 0x700038A0); +0x100..+0x108
 *            += 0x700038A0..A8; 00102948(+0xB0, +0x100); w+0 = 7.0 * (+0x20
 *            / 1.7), w+4 = 7.5 * (+0x20 / 1.7), w+8 = 7.0 * (+0x20 / 1.7);
 *            +0x20 += 0.04; 001B17A0(node).
 *   2, 3     001AFC10(node).
 *   other    nothing.
 *
 * 001ECE70(a0, a1): 001CFB50(0x0081F8F0, 0, a0, D_00275C34 +0x54,
 * D_00275C34 +0x5C, 1.0, 1.0 / 10, 9.0), then 001CFBE0(a1, 1, D_00257000,
 * 0x0081F8F0, 0).
 *
 * Memory is the original's addresses through `bytes` (a NULL view faults);
 * every callee is a named worker (a NULL or failing one faults). On a fault
 * the routine stops where it is (fail-stop) and returns -1. stdint only. */
#ifndef EM_EFFECT_001E3630_H
#define EM_EFFECT_001E3630_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define EM_EFFECT_001E3630_CALLBACK 0x001E3630u
#define EM_EFFECT_001E3620_CONTACT 0x001E3620u
#define EM_EFFECT_001E3630_FRAME 0xB0u
#define EM_EFFECT_001E3630_BLOCK 0x50u /* the 0x60-byte block at frame + 0x50 */

enum {
    EM_EFFECT_001E3630_FAULT_NONE = 0,
    EM_EFFECT_001E3630_FAULT_UNMAPPED = 1,   /* address: the memory address */
    EM_EFFECT_001E3630_FAULT_NULL_WORKER = 2,
    EM_EFFECT_001E3630_FAULT_WORKER = 3      /* address: the callee */
};

typedef struct {
    int32_t code;
    uint32_t address;
} EmEffect001E3630Fault;

/* Floats are bit patterns. Pointer arguments are original addresses. */
typedef struct {
    void *ctx;
    uint8_t *(*bytes)(void *ctx, uint32_t address, uint32_t size, int write);
    int (*w_00102948)(void *ctx, uint32_t a0, uint32_t a1);             /* vector copy */
    int (*w_00102958)(void *ctx, uint32_t a0, uint32_t a1);             /* copy_qw4 */
    int (*w_001026A0)(void *ctx, uint32_t a0, uint32_t a1, uint32_t a2);
    int (*w_001028D0)(void *ctx, uint32_t a0, uint32_t a1, uint32_t a2);
    int (*w_00102918)(void *ctx, uint32_t a0, uint32_t a1, uint32_t a2);
    int (*w_0011E748)(void *ctx, uint32_t f12, uint32_t *f0);           /* sqrtf */
    int (*w_00122BB8)(void *ctx, int32_t *v0);                          /* rand() */
    int (*w_0019A570)(void *ctx, uint32_t a0, uint32_t a1, int32_t a2, int32_t a3, int32_t *v0);
    int (*w_0019B6C0)(void *ctx, uint32_t a0, uint32_t a1, int32_t *v0);
    int (*w_001AFC10)(void *ctx, uint32_t a0);                          /* the pool's free */
    int (*w_001B17A0)(void *ctx, uint32_t a0);
    int (*w_001CCF70)(void *ctx, uint32_t a0, int32_t *v0);
    int (*w_001CD390)(void *ctx, uint32_t a0, uint32_t a1);
    int (*w_001CFA60)(void *ctx, uint32_t a0, uint32_t a1, uint32_t f12, uint32_t f13);
    int (*w_001CFB50)(void *ctx, uint32_t a0, int32_t a1, uint32_t a2, const uint32_t f[5]);
    int (*w_001CFBE0)(void *ctx, int32_t a0, int32_t a1, uint32_t a2, uint32_t a3, int32_t t0);
    int (*w_001EFD20)(void *ctx, uint32_t a0, uint32_t a1);
    int (*w_001F0190)(void *ctx, uint32_t f12, uint32_t f13);
    int (*w_001F0290)(void *ctx);
    int (*w_001F0460)(void *ctx, int32_t a0, uint32_t a1);
} EmEffect001E3630Hooks;

/* 001E3630(node) with the caller's stack pointer sp (the frame is sp -
 * 0xB0). 0, or -1 with *fault set. */
int em_effect_001E3630(const EmEffect001E3630Hooks *h, uint32_t node, uint32_t sp, EmEffect001E3630Fault *fault);

/* 001E3620: the node's +0x34 contact callback, an empty function. */
int em_effect_001E3620(void);

/* 001ECE70(a0, a1): uses bytes (D_00275C34 and its +0x54 / +0x5C),
 * w_001CFB50 and w_001CFBE0 only. */
int em_effect_001ECE70(const EmEffect001E3630Hooks *h, uint32_t a0, int32_t a1, EmEffect001E3630Fault *fault);

#ifdef __cplusplus
}
#endif

#endif
