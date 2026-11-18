/*
 * em_area00_arena_camera.h — the AREA00 boss arena camera.
 *
 * Translations of the boot ELF's 00194DB0 (the arena camera, byte-matched
 * C in the decomp: src/func_00194DB0.c) and its callee 0018C0C0 (the
 * target copy D_008105E0 = cam +0x20). 00195130's area arm calls 00194DB0
 * for area 0 when D_00810701 == 2 and D_00810803 == 3 (the boss fight of
 * AREA00 sub 2). docs/AREA00_OVERLAY_PORT.md "The arena camera".
 *
 * Memory goes through `bytes` (original address -> native little-endian
 * bytes laid out as the original). Every other callee is a hook; float
 * arguments and results cross as floats holding the EE bit patterns.
 *
 * Fail-stop: a NULL mapping or a hook returning < 0 stops the routine at
 * once (nothing after it runs); the call returns -1 and `*fault_address`
 * names the unmapped address or the callee's original address.
 *
 * Verified against the original instructions by
 * tools/test_area00_arena_camera_reference.py.
 */
#ifndef EM_AREA00_ARENA_CAMERA_H
#define EM_AREA00_ARENA_CAMERA_H

#include <stdint.h>

typedef struct EmA00ArenaCamHooks {
    void *ctx;
    uint8_t *(*bytes)(void *ctx, uint32_t address, uint32_t size);
    int (*w_0011DE90)(void *ctx, float x, float *result);         /* cosine */
    int (*w_0011DF78)(void *ctx, float x, float *result);         /* absolute value */
    int (*w_0011E2A8)(void *ctx, float x, float *result);         /* sine */
    int (*w_001B1470)(void *ctx, float x, float *result);         /* wrap into (-pi, pi] */
    int (*w_001B12B0)(void *ctx, float goal, float current, float rate, float *result);
    int (*w_00102948)(void *ctx, uint32_t dst, uint32_t src);     /* 0018C0C0's vector copy */
    int (*w_0018D7B0)(void *ctx, uint32_t cam, int32_t style);
    int (*w_0018C6A0)(void *ctx, uint32_t from, uint32_t to, float rate);
    int (*w_0018C4B0)(void *ctx, uint32_t vec, float y, float rate);
} EmA00ArenaCamHooks;

/* 00194DB0(cam, ent): cam the camera block (0x8101E0), ent the player
 * record (0x8102B0). Returns 0, or -1 on a fault. */
int em_a00_arena_camera_00194DB0(const EmA00ArenaCamHooks *h, uint32_t cam, uint32_t ent,
                                 uint32_t *fault_address);

/* 0018C0C0(cam): 00102948(D_008105E0, cam + 0x20). */
int em_a00_arena_camera_0018C0C0(const EmA00ArenaCamHooks *h, uint32_t cam, uint32_t *fault_address);

#endif
