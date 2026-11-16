/* AREA14's own overlay owners (OVERLAY/AREA14.BIN, header id 0x0B; sub 0
 * placements of the constructed start k01's AREA14, docs/AREA14_PORT.md):
 * [35] / [36] 0x823580, [5] 0x823760 with its script's op09 callback
 * 0x8238C0, [0] 0x8238E0, [2] 0x823B30, [4] 0x823E30, [6] 0x825FD0 with its
 * script's op09 callback 0x825C40; and two boot functions only AREA14
 * reaches, the camera arm 00230230 and its area test 00191350. Overlay
 * addresses are runtime addresses (the
 * decomp's link names are 0x40 lower). Ground truth: the decomp's C under
 * src/overlays/AREA14/ (byte-identical but 0x823E30, NEARMISS: its .s is
 * the original); evidence: tools/test_area14_overlay_reference.py, which
 * runs the ORIGINAL overlay code over the k01 recordings.
 *
 * Same contract as em_area00s2_creature.h: callees are explicit hooks by
 * original address, memory goes through `bytes`, a NULL or failing hook or
 * an unmapped address latches a fault (fail-stop) and the entry returns -1.
 * `sp` is the original entry stack pointer where the owner keeps a frame
 * local: 0x823580's packet (sp - 0x60, 96 bytes, written only by its
 * callees) and 0x8238E0's copy of the 80-byte area 0x826F10 (sp - 0x50,
 * written by the owner itself, read by 001B1EA0). */
#ifndef EM_AREA14_OVERLAY_H
#define EM_AREA14_OVERLAY_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct { uint32_t address; int32_t code; } EmArea14OverlayFault;
enum { EM_AREA14_OVERLAY_FAULT_NONE = 0, EM_AREA14_OVERLAY_FAULT_NULL_WORKER = 1,
       EM_AREA14_OVERLAY_FAULT_WORKER_FAILED = 2, EM_AREA14_OVERLAY_FAULT_BAD_ADDRESS = 5 };
typedef struct {
    void *ctx;
    uint8_t *(*bytes)(void *ctx, uint32_t address, uint32_t size);
/* BEGIN GENERATED HOOKS */
    int (*w_001028D0)(void *ctx, uint32_t a0, uint32_t a1, uint32_t a2);
    int (*w_00102918)(void *ctx, uint32_t a0, uint32_t a1, uint32_t a2);
    int (*w_00102948)(void *ctx, uint32_t a0, uint32_t a1);
    int (*w_001029C0)(void *ctx, uint32_t a0);
    int (*w_00102C58)(void *ctx, uint32_t a0, uint32_t a1, uint32_t a2);
    int (*w_0011DE90)(void *ctx, float a0, float *result);
    int (*w_0011DF78)(void *ctx, float a0, float *result);
    int (*w_0011E2A8)(void *ctx, float a0, float *result);
    int (*w_00122BB8)(void *ctx, int32_t *result);
    int (*w_00158590)(void *ctx, uint32_t a0, int32_t a1, int32_t a2);
    int (*w_00183010)(void *ctx, uint32_t a0, uint32_t a1);
    int (*w_00183160)(void *ctx, int32_t a0, float a1);
    int (*w_0018C4B0)(void *ctx, uint32_t a0, float a1, float a2);
    int (*w_0018C6A0)(void *ctx, uint32_t a0, uint32_t a1, float a2);
    int (*w_0018D7B0)(void *ctx, uint32_t a0, int32_t a1);
    int (*w_00191120)(void *ctx, float a0, float a1, float a2, float a3, float *result);
    int (*w_00191D40)(void *ctx, uint32_t a0, float a1, float a2);
    int (*w_00192010)(void *ctx, uint32_t a0, float a1, float a2, float a3);
    int (*w_001AEDE0)(void *ctx, int32_t a0, int32_t a1);
    int (*w_001AFC10)(void *ctx, uint32_t a0);
    int (*w_001B0C60)(void *ctx, int32_t a0, int32_t a1, int32_t a2);
    int (*w_001B0FD0)(void *ctx, uint32_t a0, int32_t *result);
    int (*w_001B10B0)(void *ctx, uint32_t a0, int32_t a1, int32_t a2);
    int (*w_001B12B0)(void *ctx, float a0, float a1, float a2, float *result);
    int (*w_001B1470)(void *ctx, float a0, float *result);
    int (*w_001B17A0)(void *ctx, uint32_t a0);
    int (*w_001B1EA0)(void *ctx, int32_t a0, uint32_t a1, uint32_t a2, int32_t a3, int32_t *result);
    int (*w_001B6660)(void *ctx, uint32_t a0);
    int (*w_001BA1A0)(void *ctx, uint32_t a0, uint32_t a1);
    int (*w_001BA1C0)(void *ctx, uint32_t a0, int32_t a1, int32_t *result);
    int (*w_001BA1F0)(void *ctx, uint32_t a0, int32_t *result);
    int (*w_001BA540)(void *ctx, uint32_t a0);
    int (*w_001BA580)(void *ctx, uint32_t a0, int32_t a1);
    int (*w_001BA8E0)(void *ctx, uint32_t a0, int32_t a1);
    int (*w_001C4760)(void *ctx, int32_t a0, int32_t a1);
    int (*w_001C47A0)(void *ctx, int32_t a0, int32_t a1);
    int (*w_001C6380)(void *ctx, uint32_t a0);
    int (*w_001C63E0)(void *ctx, uint32_t a0, int32_t a1);
    int (*w_001C64F0)(void *ctx, uint32_t a0, float a1, int32_t *result);
    int (*w_001C67E0)(void *ctx, uint32_t a0, int32_t a1, float a2, float a3);
    int (*w_001C68C0)(void *ctx, uint32_t a0);
    int (*w_001CA6F0)(void *ctx, uint32_t a0, int32_t a1);
    int (*w_001CCF70)(void *ctx, uint32_t a0, int32_t *result);
    int (*w_001CFB50)(void *ctx, uint32_t a0, int32_t a1, uint32_t a2, float a3, float a4, float a5, float a6, float a7);
    int (*w_001CFBE0)(void *ctx, int32_t a0, int32_t a1, uint32_t a2, uint32_t a3, int32_t a4);
    int (*w_001F6B00)(void *ctx);
    int (*w_001FAE70)(void *ctx, int32_t a0);
    int (*w_001FB9F0)(void *ctx, int32_t a0, int32_t a1, int32_t a2, int32_t a3);
    int (*w_0021B9A0)(void *ctx, int32_t a0, float a1, float a2);
/* END GENERATED HOOKS */
    int (*w_callback)(void *ctx, uint32_t function, uint32_t self);
} EmArea14OverlayHooks;
/* Placement owners: one call of the original behaviour. Return 0, or -1 on
 * a fault. */
int em_area14_overlay_00823580(const EmArea14OverlayHooks *h, uint32_t a0, uint32_t sp, EmArea14OverlayFault *fault);
int em_area14_overlay_00823760(const EmArea14OverlayHooks *h, uint32_t a0, EmArea14OverlayFault *fault);
int em_area14_overlay_008238E0(const EmArea14OverlayHooks *h, uint32_t a0, uint32_t sp, EmArea14OverlayFault *fault);
int em_area14_overlay_00823B30(const EmArea14OverlayHooks *h, uint32_t a0, EmArea14OverlayFault *fault);
int em_area14_overlay_00823E30(const EmArea14OverlayHooks *h, uint32_t a0, EmArea14OverlayFault *fault);
int em_area14_overlay_00825FD0(const EmArea14OverlayHooks *h, uint32_t a0, EmArea14OverlayFault *fault);
/* Script op09 callbacks. 0x8238C0 (script 0x826800's) reads no argument:
 * 001FAE70(0), *result = 1. 0x825C40 (script 0x828120's) takes (self, the
 * op09 record, the script's work block); *result is its return value (1 =
 * done). *result is left unwritten when the call returns -1. */
int em_area14_overlay_008238C0(const EmArea14OverlayHooks *h, int32_t *result, EmArea14OverlayFault *fault);
int em_area14_overlay_00825C40(const EmArea14OverlayHooks *h, uint32_t a0, uint32_t a1, uint32_t a2, int32_t *result,
                               EmArea14OverlayFault *fault);
/* AREA14's boot pieces: the camera arm 00230230(camera, player) (case 0xE
 * of the area switch em_camera_area11_specials runs; *result 1 when a state
 * was handled) and its area test 00191350(point record) (*result 1 when
 * the record's +0xA0 lies in the area D_0024A4F0). Byte-matched decomp C. */
int em_area14_overlay_00230230(const EmArea14OverlayHooks *h, uint32_t a0, uint32_t a1, int32_t *result,
                               EmArea14OverlayFault *fault);
int em_area14_overlay_00191350(const EmArea14OverlayHooks *h, uint32_t a0, int32_t *result,
                               EmArea14OverlayFault *fault);
#ifdef __cplusplus
}
#endif
#endif
