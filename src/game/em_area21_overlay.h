/* AREA21's mounted guns (OVERLAY/AREA21.BIN, header id 0x12; docs of the
 * decomp: docs/AREA21_OVERLAY.md "[61] 0x828700 and [62] 0x8297C0" and
 * "0x828F30"): the Gatling [62] 0x8297C0 with its overlay callees 0x8260A0,
 * 0x829310, 0x8294C0, 0x829590, 0x829F10 and 0x829F40, and the cannon [61]
 * 0x828700 with 0x828410, 0x828E80 and the projectile 0x828F30 (the code
 * pointer 0x828E80 installs). Overlay addresses are runtime addresses (the
 * decomp's link names are 0x40 lower). Ground truth: the decomp's C under
 * src/overlays/AREA21/; evidence: tools/test_area21_overlay_reference.py,
 * which runs the ORIGINAL overlay code (the user's extract/OVERLAY/AREA21.BIN
 * over a recording's arena; no AREA21 recording exists).
 *
 * Same contract as em_area14_overlay.h: callees are explicit hooks by
 * original address, memory goes through `bytes`, a NULL or failing hook or
 * an unmapped address latches a fault (fail-stop) and the entry returns -1.
 * w_callback is a node's +0x4C method (function word, node). Entries that
 * take `sp` get the original's stack pointer at their entry: their frame
 * locals live at the original's stack addresses below it. */
#ifndef EM_AREA21_OVERLAY_H
#define EM_AREA21_OVERLAY_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct { uint32_t address; int32_t code; } EmArea21OverlayFault;
enum { EM_AREA21_OVERLAY_FAULT_NONE = 0, EM_AREA21_OVERLAY_FAULT_NULL_WORKER = 1,
       EM_AREA21_OVERLAY_FAULT_WORKER_FAILED = 2, EM_AREA21_OVERLAY_FAULT_BAD_ADDRESS = 5 };
typedef struct {
    void *ctx;
    uint8_t *(*bytes)(void *ctx, uint32_t address, uint32_t size);
/* BEGIN GENERATED HOOKS */
    int (*w_001026A0)(void *ctx, uint32_t a0, uint32_t a1, uint32_t a2);
    int (*w_001026D0)(void *ctx, uint32_t a0, uint32_t a1, uint32_t a2);
    int (*w_00102760)(void *ctx, uint32_t a0, uint32_t a1);
    int (*w_001028D0)(void *ctx, uint32_t a0, uint32_t a1, uint32_t a2);
    int (*w_00102918)(void *ctx, uint32_t a0, uint32_t a1, uint32_t a2);
    int (*w_00102948)(void *ctx, uint32_t a0, uint32_t a1);
    int (*w_00102958)(void *ctx, uint32_t a0, uint32_t a1);
    int (*w_001029C0)(void *ctx, uint32_t a0);
    int (*w_00102B08)(void *ctx, uint32_t a0, uint32_t a1, float a2);
    int (*w_00102BB0)(void *ctx, uint32_t a0, uint32_t a1, float a2);
    int (*w_001031E0)(void *ctx, uint32_t a0, uint32_t a1);
    int (*w_00103230)(void *ctx, uint32_t a0, uint32_t a1, float a2);
    int (*w_0011DE90)(void *ctx, float a0, float *result);
    int (*w_0011DF78)(void *ctx, float a0, float *result);
    int (*w_00122BB8)(void *ctx, int32_t *result);
    int (*w_001281C0)(void *ctx, float a0, int32_t *result);
    int (*w_00182F90)(void *ctx, uint32_t a0, uint32_t a1);
    int (*w_0019A570)(void *ctx, uint32_t a0, uint32_t a1, int32_t a2, int32_t a3, int32_t *result);
    int (*w_001A2370)(void *ctx, uint32_t a0, uint32_t a1);
    int (*w_001AFA90)(void *ctx, int32_t a0, uint32_t *result);
    int (*w_001AFC10)(void *ctx, uint32_t a0);
    int (*w_001B0FD0)(void *ctx, uint32_t a0, int32_t *result);
    int (*w_001B1470)(void *ctx, float a0, float *result);
    int (*w_001B17A0)(void *ctx, uint32_t a0);
    int (*w_001B1E20)(void *ctx, int32_t a0, int32_t a1);
    int (*w_001B41F0)(void *ctx, uint32_t a0, uint32_t a1, uint32_t a2, int32_t a3, int32_t a4, int32_t a5);
    int (*w_001B5DC0)(void *ctx, int32_t a0, int32_t *result);
    int (*w_001C5570)(void *ctx, uint32_t a0, uint32_t a1, int32_t a2, int32_t a3, uint32_t *result);
    int (*w_001C6380)(void *ctx, uint32_t a0);
    int (*w_001C94B0)(void *ctx, uint32_t a0, uint32_t a1, uint32_t a2, uint32_t a3);
    int (*w_001CCF70)(void *ctx, uint32_t a0, int32_t *result);
    int (*w_001CFB50)(void *ctx, uint32_t a0, int32_t a1, uint32_t a2, float a3, float a4, float a5, float a6, float a7);
    int (*w_001CFBE0)(void *ctx, int32_t a0, int32_t a1, uint32_t a2, uint32_t a3, int32_t a4);
    int (*w_001D1F80)(void *ctx, int32_t a0, int32_t a1, int32_t a2);
    int (*w_001EFD20)(void *ctx, uint32_t a0, uint32_t a1);
    int (*w_001EFD90)(void *ctx, int32_t a0, uint32_t a1, uint32_t a2);
    int (*w_001EFE00)(void *ctx, uint32_t a0, uint32_t a1, uint32_t *result);
    int (*w_001EFEB0)(void *ctx, uint32_t a0, uint32_t a1, uint32_t *result);
    int (*w_001F4010)(void *ctx, int32_t a0, uint32_t a1);
    int (*w_001F4F40)(void *ctx, int32_t a0, uint32_t *result);
    int (*w_001F5940)(void *ctx, int32_t a0, uint32_t a1, int32_t a2);
    int (*w_001FB9F0)(void *ctx, int32_t a0, int32_t a1, int32_t a2, int32_t a3);
    int (*w_001FBD50)(void *ctx, uint32_t a0, int32_t a1, int32_t a2, float a3);
    int (*w_00207F80)(void *ctx, int32_t a0, int32_t a1, int32_t a2, int32_t a3, int32_t a4, uint32_t a5);
/* END GENERATED HOOKS */
    int (*w_callback)(void *ctx, uint32_t function, uint32_t self);
} EmArea21OverlayHooks;
/* The Gatling [62] (sub 0): one call of the original behaviour on the node
 * a0; sp is the original's stack pointer at entry (0x829F40's frame lies
 * below it). Return 0, or -1 on a fault. */
int em_area21_overlay_008297C0(const EmArea21OverlayHooks *h, uint32_t a0, uint32_t sp,
                               EmArea21OverlayFault *fault);
/* 0x8260A0: the Gatling's heat gauge for t = +0x22C / 900 (a0 is t). */
int em_area21_overlay_008260A0(const EmArea21OverlayHooks *h, float a0, EmArea21OverlayFault *fault);
/* 0x829310: the rate-limited step of the float at a0 on the pad key a1. */
int em_area21_overlay_00829310(const EmArea21OverlayHooks *h, uint32_t a0, int32_t a1,
                               EmArea21OverlayFault *fault);
/* 0x8294C0: places the object a1 behind the Gatling a0. */
int em_area21_overlay_008294C0(const EmArea21OverlayHooks *h, uint32_t a0, uint32_t a1,
                               EmArea21OverlayFault *fault);
/* 0x829590: the gun's model points of the node a0 into the work a1. */
int em_area21_overlay_00829590(const EmArea21OverlayHooks *h, uint32_t a0, uint32_t a1,
                               EmArea21OverlayFault *fault);
/* 0x829F10: the cool-down phase's start on the node a0. */
int em_area21_overlay_00829F10(const EmArea21OverlayHooks *h, uint32_t a0, EmArea21OverlayFault *fault);
/* 0x829F40: one cool-down frame of the node a0; sp is the original's stack
 * pointer at entry (its matrix and packet live below it). */
int em_area21_overlay_00829F40(const EmArea21OverlayHooks *h, uint32_t a0, uint32_t sp,
                               EmArea21OverlayFault *fault);
/* The cannon [61] (sub 0) on the node a0. */
int em_area21_overlay_00828700(const EmArea21OverlayHooks *h, uint32_t a0, EmArea21OverlayFault *fault);
/* 0x828410: the cannon's model points of the node a0 into the work a1. */
int em_area21_overlay_00828410(const EmArea21OverlayHooks *h, uint32_t a0, uint32_t a1,
                               EmArea21OverlayFault *fault);
/* 0x828E80: spawns the projectile from the node a0 at the work a1's muzzle
 * a2 (0: +0x10, else +0x20); *result is 1, or 0 when no object was free. */
int em_area21_overlay_00828E80(const EmArea21OverlayHooks *h, uint32_t a0, uint32_t a1, int32_t a2,
                               int32_t *result, EmArea21OverlayFault *fault);
/* 0x828F30: the projectile (the behaviour 0x828E80 installs) on the node a0. */
int em_area21_overlay_00828F30(const EmArea21OverlayHooks *h, uint32_t a0, EmArea21OverlayFault *fault);
#ifdef __cplusplus
}
#endif
#endif
