/* AREA16's own overlay owners (OVERLAY/AREA16.BIN, header id 0x0D; the
 * placements of subs 0 and 1, docs/AREA16_PORT.md): the placement and group
 * behaviours, the two children they spawn (0x825260, 0x828200), their
 * script op09 callbacks (0x825630, 0x826C20) and the overlay helpers they
 * call. Overlay addresses are runtime addresses (the decomp's link names
 * are 0x40 lower). Ground truth: the decomp's C under src/overlays/AREA16/
 * (byte-identical but 0x824690 and 0x825310, NEARMISS: their .s is the
 * original); evidence: tools/test_area16_overlay_reference.py, which runs
 * the ORIGINAL overlay code from the user's AREA16.BIN over a capture's
 * boot RAM with designed states (no AREA16 capture exists).
 *
 * Same contract as em_area14_overlay.h: callees are explicit hooks by
 * original address, memory goes through `bytes`, a NULL or failing hook or
 * an unmapped address latches a fault (fail-stop) and the entry returns -1.
 * `sp` is the original entry stack pointer: an owner that keeps a frame
 * local (a packet, a matrix, a copied area) addresses it below `sp` at the
 * original's offsets, so a callee sees the same address. */
#ifndef EM_AREA16_OVERLAY_H
#define EM_AREA16_OVERLAY_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct { uint32_t address; int32_t code; } EmArea16OverlayFault;
enum { EM_AREA16_OVERLAY_FAULT_NONE = 0, EM_AREA16_OVERLAY_FAULT_NULL_WORKER = 1,
       EM_AREA16_OVERLAY_FAULT_WORKER_FAILED = 2, EM_AREA16_OVERLAY_FAULT_BAD_ADDRESS = 5,
       /* the original would read a register its caller left (a selector
        * outside the values the code assigns): not reproducible, so the
        * entry stops there (0x823580's +0x94, 0x828200's +0xD) */
       EM_AREA16_OVERLAY_FAULT_CALLER_REGISTER = 6 };
typedef struct {
    void *ctx;
    uint8_t *(*bytes)(void *ctx, uint32_t address, uint32_t size);
/* BEGIN GENERATED HOOKS */
    int (*w_001026A0)(void *ctx, uint32_t a0, uint32_t a1, uint32_t a2);
    int (*w_001026D0)(void *ctx, uint32_t a0, uint32_t a1, uint32_t a2);
    int (*w_00102918)(void *ctx, uint32_t a0, uint32_t a1, uint32_t a2);
    int (*w_00102948)(void *ctx, uint32_t a0, uint32_t a1);
    int (*w_00102958)(void *ctx, uint32_t a0, uint32_t a1);
    int (*w_001029C0)(void *ctx, uint32_t a0);
    int (*w_00102B08)(void *ctx, uint32_t a0, uint32_t a1, float a2);
    int (*w_00102BB0)(void *ctx, uint32_t a0, uint32_t a1, float a2);
    int (*w_00102C58)(void *ctx, uint32_t a0, uint32_t a1, uint32_t a2);
    int (*w_00103230)(void *ctx, uint32_t a0, uint32_t a1, float a2);
    int (*w_0011E2A8)(void *ctx, float a0, float *result);
    int (*w_00122BB8)(void *ctx, int32_t *result);
    int (*w_001281C0)(void *ctx, float a0, int32_t *result);
    int (*w_0019C6F0)(void *ctx, int32_t a0, int32_t a1);
    int (*w_001A2370)(void *ctx, uint32_t a0, uint32_t a1);
    int (*w_001AF800)(void *ctx, uint32_t a0);
    int (*w_001AFA90)(void *ctx, int32_t a0, uint32_t *result);
    int (*w_001AFC10)(void *ctx, uint32_t a0);
    int (*w_001B0FD0)(void *ctx, uint32_t a0, int32_t *result);
    int (*w_001B10B0)(void *ctx, uint32_t a0, int32_t a1, int32_t a2, int32_t *result);
    int (*w_001B11E0)(void *ctx, int32_t a0, int32_t *result);
    int (*w_001B17A0)(void *ctx, uint32_t a0, int32_t *result);
    int (*w_001B1B70)(void *ctx, uint32_t a0);
    int (*w_001B1EA0)(void *ctx, int32_t a0, uint32_t a1, uint32_t a2, int32_t a3, int32_t *result);
    int (*w_001B6660)(void *ctx, uint32_t a0, uint32_t *result);
    int (*w_001BA1A0)(void *ctx, uint32_t a0, uint32_t a1);
    int (*w_001BA1C0)(void *ctx, uint32_t a0, int32_t a1, int32_t *result);
    int (*w_001BA1F0)(void *ctx, uint32_t a0, int32_t *result);
    int (*w_001BA540)(void *ctx, uint32_t a0);
    int (*w_001BA580)(void *ctx, uint32_t a0, int32_t a1);
    int (*w_001BAD40)(void *ctx, uint32_t a0, uint32_t a1, int32_t *result);
    int (*w_001C4760)(void *ctx, int32_t a0, int32_t a1);
    int (*w_001C5570)(void *ctx, uint32_t a0, uint32_t a1, int32_t a2, int32_t a3, uint32_t *result);
    int (*w_001C5C90)(void *ctx, uint32_t a0);
    int (*w_001C6120)(void *ctx, int32_t a0, int32_t a1, int32_t *result);
    int (*w_001C62C0)(void *ctx, uint32_t a0);
    int (*w_001C6380)(void *ctx, uint32_t a0);
    int (*w_001C63E0)(void *ctx, uint32_t a0, int32_t a1);
    int (*w_001C64F0)(void *ctx, uint32_t a0, float a1);
    int (*w_001C68C0)(void *ctx, uint32_t a0);
    int (*w_001CA5E0)(void *ctx, uint32_t a0, int32_t a1, int32_t a2);
    int (*w_001CA5F0)(void *ctx, uint32_t a0, int32_t a1);
    int (*w_001CA6E0)(void *ctx, uint32_t a0, int32_t a1);
    int (*w_001CB5B0)(void *ctx, int32_t a0);
    int (*w_001CCF70)(void *ctx, uint32_t a0, int32_t *result);
    int (*w_001CD520)(void *ctx, int32_t a0, int32_t a1, uint32_t a2, uint64_t a3, uint32_t a4, float a5, float a6, float a7);
    int (*w_001CFAE0)(void *ctx, uint32_t a0, int32_t a1, uint32_t a2, float a3, float a4, float a5, float a6);
    int (*w_001CFBE0)(void *ctx, int32_t a0, int32_t a1, uint32_t a2, uint32_t a3, int32_t a4);
    int (*w_001D04B0)(void *ctx, uint32_t a0, int32_t a1, uint32_t a2, float a3, float a4);
    int (*w_001D7FA0)(void *ctx, uint32_t a0, uint32_t a1, int32_t a2, float a3, float a4, int32_t *result);
    int (*w_001D8060)(void *ctx, int32_t a0, uint32_t *result);
    int (*w_001D80B0)(void *ctx, int32_t a0);
    int (*w_001D8BF0)(void *ctx, uint32_t a0, int32_t a1);
    int (*w_001DAFA0)(void *ctx);
    int (*w_001DB240)(void *ctx, int32_t a0);
    int (*w_001DB250)(void *ctx);
    int (*w_001DB480)(void *ctx);
    int (*w_001EFD20)(void *ctx, int32_t a0, uint32_t a1, uint32_t *result);
    int (*w_001EFD90)(void *ctx, int32_t a0, uint32_t a1, uint32_t a2);
    int (*w_001EFEB0)(void *ctx, int32_t a0, uint32_t a1, uint32_t *result);
    int (*w_001F4BF0)(void *ctx, uint32_t a0, uint32_t a1);
    int (*w_001F4E20)(void *ctx, uint32_t a0, uint32_t a1, float a2);
    int (*w_001F5940)(void *ctx, int32_t a0, uint32_t a1, int32_t a2);
    int (*w_001FAE70)(void *ctx, int32_t a0);
    int (*w_001FBD50)(void *ctx, uint32_t a0, int32_t a1, int32_t a2, float a3);
    int (*w_001FC3C0)(void *ctx, uint32_t a0, uint32_t a1, int32_t a2, float a3, float a4);
    int (*w_001FC520)(void *ctx, uint32_t a0);
/* END GENERATED HOOKS */
    int (*w_callback)(void *ctx, uint32_t function, uint32_t self);
} EmArea16OverlayHooks;

/* Behaviours (placement, group and spawned-child owners): one call of the
 * original function with the node in a0. Return 0, or -1 on a fault. */
int em_area16_overlay_00823580(const EmArea16OverlayHooks *h, uint32_t a0, uint32_t sp, EmArea16OverlayFault *fault);
int em_area16_overlay_00824440(const EmArea16OverlayHooks *h, uint32_t a0, uint32_t sp, EmArea16OverlayFault *fault);
int em_area16_overlay_00824690(const EmArea16OverlayHooks *h, uint32_t a0, uint32_t sp, EmArea16OverlayFault *fault);
int em_area16_overlay_00824E10(const EmArea16OverlayHooks *h, uint32_t a0, uint32_t sp, EmArea16OverlayFault *fault);
int em_area16_overlay_00825260(const EmArea16OverlayHooks *h, uint32_t a0, uint32_t sp, EmArea16OverlayFault *fault);
int em_area16_overlay_00825310(const EmArea16OverlayHooks *h, uint32_t a0, uint32_t sp, EmArea16OverlayFault *fault);
int em_area16_overlay_00825800(const EmArea16OverlayHooks *h, uint32_t a0, uint32_t sp, EmArea16OverlayFault *fault);
int em_area16_overlay_008261A0(const EmArea16OverlayHooks *h, uint32_t a0, uint32_t sp, EmArea16OverlayFault *fault);
int em_area16_overlay_008263D0(const EmArea16OverlayHooks *h, uint32_t a0, uint32_t sp, EmArea16OverlayFault *fault);
int em_area16_overlay_00826C50(const EmArea16OverlayHooks *h, uint32_t a0, uint32_t sp, EmArea16OverlayFault *fault);
int em_area16_overlay_00827030(const EmArea16OverlayHooks *h, uint32_t a0, uint32_t sp, EmArea16OverlayFault *fault);
int em_area16_overlay_008273F0(const EmArea16OverlayHooks *h, uint32_t a0, uint32_t sp, EmArea16OverlayFault *fault);
int em_area16_overlay_00827B70(const EmArea16OverlayHooks *h, uint32_t a0, uint32_t sp, EmArea16OverlayFault *fault);
int em_area16_overlay_00827CE0(const EmArea16OverlayHooks *h, uint32_t a0, uint32_t sp, EmArea16OverlayFault *fault);
int em_area16_overlay_00827FA0(const EmArea16OverlayHooks *h, uint32_t a0, uint32_t sp, EmArea16OverlayFault *fault);
int em_area16_overlay_00828200(const EmArea16OverlayHooks *h, uint32_t a0, uint32_t sp, EmArea16OverlayFault *fault);

/* The effect behaviours (no AREA16 word names them; effect ids 0..4 of
 * AREA16's effect table select them through 001EF9D0): one call with the
 * effect node in a0. */
int em_area16_overlay_008237A0(const EmArea16OverlayHooks *h, uint32_t a0, uint32_t sp, EmArea16OverlayFault *fault);
int em_area16_overlay_008239B0(const EmArea16OverlayHooks *h, uint32_t a0, uint32_t sp, EmArea16OverlayFault *fault);
int em_area16_overlay_00823D20(const EmArea16OverlayHooks *h, uint32_t a0, uint32_t sp, EmArea16OverlayFault *fault);
int em_area16_overlay_00823F70(const EmArea16OverlayHooks *h, uint32_t a0, uint32_t sp, EmArea16OverlayFault *fault);
int em_area16_overlay_00824110(const EmArea16OverlayHooks *h, uint32_t a0, uint32_t sp, EmArea16OverlayFault *fault);

/* Script op09 callbacks: (self, the op09 record, the script's work block);
 * *result is the return value (1 = done), left unwritten on -1.
 * 0x825630 is script 0x829970's, 0x826C20 scripts 0x82A1C0 / 0x82A3C0's. */
int em_area16_overlay_00825630(const EmArea16OverlayHooks *h, uint32_t a0, uint32_t a1, uint32_t a2, int32_t *result,
                               EmArea16OverlayFault *fault);
int em_area16_overlay_00826C20(const EmArea16OverlayHooks *h, uint32_t a0, uint32_t a1, uint32_t a2, int32_t *result,
                               EmArea16OverlayFault *fault);

/* The overlay helpers the owners call (each also an oracle entry):
 * 0x824540(self) from 0x824440, 0x825660(self) from 0x824440,
 * 0x825670(self, out, idx), 0x8256E0(self, idx), 0x825730(self),
 * 0x825780(self, idx) from 0x824690 / 0x824E10, and 0x826A50(self, a1,
 * f12) from 0x825800 (its float argument as its bits). */
int em_area16_overlay_00824540(const EmArea16OverlayHooks *h, uint32_t a0, uint32_t sp, EmArea16OverlayFault *fault);
int em_area16_overlay_00825660(const EmArea16OverlayHooks *h, uint32_t a0, uint32_t sp, EmArea16OverlayFault *fault);
int em_area16_overlay_00825670(const EmArea16OverlayHooks *h, uint32_t a0, uint32_t a1, int32_t a2, uint32_t sp,
                               EmArea16OverlayFault *fault);
int em_area16_overlay_008256E0(const EmArea16OverlayHooks *h, uint32_t a0, int32_t a1, uint32_t sp,
                               EmArea16OverlayFault *fault);
int em_area16_overlay_00825730(const EmArea16OverlayHooks *h, uint32_t a0, uint32_t sp, EmArea16OverlayFault *fault);
int em_area16_overlay_00825780(const EmArea16OverlayHooks *h, uint32_t a0, int32_t a1, uint32_t sp,
                               EmArea16OverlayFault *fault);
int em_area16_overlay_00826A50(const EmArea16OverlayHooks *h, uint32_t a0, uint32_t a1, float f12, uint32_t sp,
                               EmArea16OverlayFault *fault);
#ifdef __cplusplus
}
#endif
#endif
