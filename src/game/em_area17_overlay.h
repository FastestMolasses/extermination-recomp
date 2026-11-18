/* AREA17's own overlay owners (OVERLAY/AREA17.BIN, header id 0x0E; sub 0
 * placements and groups of the power reactor's roof, docs/AREA17_PORT.md):
 * the effect objects 0x823580, 0x8235E0, 0x8236F0, 0x823800, 0x823860, the
 * group 0x826890 points 0x8239D0, [30] 0x823BA0 (the arrival) with its
 * script's op09 callbacks 0x823F70 / 0x824280, [39] 0x8242D0, [31]
 * 0x8243E0, [33] 0x8245A0, [34] 0x824700 (the exchange), [42] 0x824CF0,
 * [40] 0x824E40, [41] 0x8250E0, the group 0x827F40 part 0x8251B0, [38]
 * 0x825470 (the way to AREA11), [5]..[8] 0x825560 (the breakables), [36]
 * 0x825EA0, the deferred group 0x826280 member 0x8260C0 and [37] 0x826170.
 * Overlay addresses are runtime addresses (the decomp's link names are 0x40
 * lower). Ground truth: the decomp's C under src/overlays/AREA17/
 * (byte-identical but 0x825560, NEARMISS: its .s is the original);
 * evidence: tools/test_area17_overlay_reference.py, which runs the ORIGINAL
 * overlay code over the guided run's AREA17 states.
 *
 * Same contract as em_area14_overlay.h: callees are explicit hooks by
 * original address, memory goes through `bytes`, a NULL or failing hook or
 * an unmapped address latches a fault (fail-stop) and the entry returns -1.
 * `sp` is the original entry stack pointer where the owner keeps a frame
 * local: 0x823860's packet (sp - 0x60, 96 bytes, written by its callee and
 * the owner), 0x8245A0's copy of the 80-byte area 0x827530 (sp - 0x50) and
 * 0x824700's copy of the 64-byte area 0x8283A0 (sp - 0x40), both written by
 * the owner itself and read by 001B1EA0. */
#ifndef EM_AREA17_OVERLAY_H
#define EM_AREA17_OVERLAY_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct { uint32_t address; int32_t code; } EmArea17OverlayFault;
enum { EM_AREA17_OVERLAY_FAULT_NONE = 0, EM_AREA17_OVERLAY_FAULT_NULL_WORKER = 1,
       EM_AREA17_OVERLAY_FAULT_WORKER_FAILED = 2, EM_AREA17_OVERLAY_FAULT_BAD_ADDRESS = 5 };
typedef struct {
    void *ctx;
    uint8_t *(*bytes)(void *ctx, uint32_t address, uint32_t size);
/* BEGIN GENERATED HOOKS */
    int (*w_001026A0)(void *ctx, uint32_t a0, uint32_t a1, uint32_t a2);
    int (*w_00102738)(void *ctx, uint32_t a0, uint32_t a1, float *result);
    int (*w_001028D0)(void *ctx, uint32_t a0, uint32_t a1, uint32_t a2);
    int (*w_00102918)(void *ctx, uint32_t a0, uint32_t a1, uint32_t a2);
    int (*w_00102948)(void *ctx, uint32_t a0, uint32_t a1);
    int (*w_001029C0)(void *ctx, uint32_t a0);
    int (*w_00102B08)(void *ctx, uint32_t a0, uint32_t a1, float a2);
    int (*w_0011DE90)(void *ctx, float a0, float *result);
    int (*w_0011E2A8)(void *ctx, float a0, float *result);
    int (*w_0011E748)(void *ctx, float a0, float *result);
    int (*w_00122BB8)(void *ctx, int32_t *result);
    int (*w_0019A570)(void *ctx, uint32_t a0, uint32_t a1, int32_t a2, int32_t a3, int32_t *result);
    int (*w_0019AB20)(void *ctx, uint32_t a0, uint32_t a1, uint32_t a2, int32_t a3, int32_t *result);
    int (*w_0019AD00)(void *ctx, uint32_t a0, uint32_t a1, int32_t a2, int32_t *result);
    int (*w_0019C6F0)(void *ctx, int32_t a0, int32_t a1);
    int (*w_001A2370)(void *ctx, uint32_t a0, uint32_t a1);
    int (*w_001AEDE0)(void *ctx, int32_t a0, int32_t a1);
    int (*w_001AEE10)(void *ctx, int32_t a0, int32_t a1);
    int (*w_001AFC10)(void *ctx, uint32_t a0);
    int (*w_001B0C60)(void *ctx, int32_t a0, int32_t a1, int32_t a2);
    int (*w_001B0D80)(void *ctx, uint32_t a0, int32_t *result);
    int (*w_001B0FD0)(void *ctx, uint32_t a0, int32_t *result);
    int (*w_001B1020)(void *ctx, uint32_t a0, int32_t a1, int32_t a2, int32_t a3, int32_t *result);
    int (*w_001B10B0)(void *ctx, uint32_t a0, int32_t a1, int32_t a2);
    int (*w_001B1470)(void *ctx, float a0, float *result);
    int (*w_001B17A0)(void *ctx, uint32_t a0, int32_t *result);
    int (*w_001B1B70)(void *ctx, uint32_t a0);
    int (*w_001B1D20)(void *ctx, uint32_t a0);
    int (*w_001B1EA0)(void *ctx, int32_t a0, uint32_t a1, uint32_t a2, int32_t a3, int32_t *result);
    int (*w_001B6660)(void *ctx, uint32_t a0, uint32_t *result);
    int (*w_001BA1A0)(void *ctx, uint32_t a0, uint32_t a1);
    int (*w_001BA1C0)(void *ctx, uint32_t a0, int32_t a1, int32_t *result);
    int (*w_001BA1F0)(void *ctx, uint32_t a0, int32_t *result);
    int (*w_001BA540)(void *ctx, uint32_t a0);
    int (*w_001BA580)(void *ctx, uint32_t a0, int32_t a1);
    int (*w_001BA8E0)(void *ctx, uint32_t a0, int32_t a1);
    int (*w_001BAD40)(void *ctx, uint32_t a0, uint32_t a1, int32_t *result);
    int (*w_001C4760)(void *ctx, int32_t a0, int32_t a1);
    int (*w_001C47A0)(void *ctx, int32_t a0, int32_t a1);
    int (*w_001C47E0)(void *ctx, int32_t a0, int32_t a1);
    int (*w_001C5570)(void *ctx, uint32_t a0, uint32_t a1, int32_t a2, int32_t a3, uint32_t *result);
    int (*w_001C5C90)(void *ctx, uint32_t a0);
    int (*w_001C6120)(void *ctx, int32_t a0, int32_t a1, int32_t *result);
    int (*w_001C62C0)(void *ctx, uint32_t a0);
    int (*w_001C6380)(void *ctx, uint32_t a0);
    int (*w_001C63E0)(void *ctx, uint32_t a0, int32_t a1);
    int (*w_001C64F0)(void *ctx, uint32_t a0, float a1, int32_t *result);
    int (*w_001C67E0)(void *ctx, uint32_t a0, int32_t a1, float a2, float a3);
    int (*w_001C68C0)(void *ctx, uint32_t a0);
    int (*w_001CA6E0)(void *ctx, uint32_t a0, int32_t a1);
    int (*w_001CA6F0)(void *ctx, uint32_t a0, int32_t a1);
    int (*w_001CCF70)(void *ctx, uint32_t a0, int32_t *result);
    int (*w_001CFB50)(void *ctx, uint32_t a0, int32_t a1, uint32_t a2, float a3, float a4, float a5, float a6, float a7);
    int (*w_001CFBE0)(void *ctx, int32_t a0, int32_t a1, uint32_t a2, uint32_t a3, int32_t a4);
    int (*w_001D04B0)(void *ctx, uint32_t a0, int32_t a1, uint32_t a2, float a3, float a4);
    int (*w_001EFD20)(void *ctx, int32_t a0, uint32_t a1);
    int (*w_001F0460)(void *ctx, int32_t a0, uint32_t a1);
    int (*w_001F4E20)(void *ctx, uint32_t a0, uint32_t a1, float a2);
    int (*w_001F5940)(void *ctx, int32_t a0, uint32_t a1, int32_t a2);
    int (*w_001F6B60)(void *ctx);
    int (*w_001F9100)(void *ctx, uint32_t a0, uint32_t a1, uint32_t a2, float a3);
    int (*w_001FAE70)(void *ctx, int32_t a0);
    int (*w_001FB0B0)(void *ctx, int32_t a0);
    int (*w_001FBD50)(void *ctx, uint32_t a0, int32_t a1, int32_t a2, float a3);
    int (*w_001FC580)(void *ctx, uint32_t a0, int32_t a1);
/* END GENERATED HOOKS */
    int (*w_callback)(void *ctx, uint32_t function, uint32_t self);
} EmArea17OverlayHooks;
/* Owners: one call of the original behaviour. Return 0, or -1 on a fault. */
int em_area17_overlay_00823580(const EmArea17OverlayHooks *h, uint32_t a0, EmArea17OverlayFault *fault);
int em_area17_overlay_008235E0(const EmArea17OverlayHooks *h, uint32_t a0, EmArea17OverlayFault *fault);
int em_area17_overlay_008236F0(const EmArea17OverlayHooks *h, uint32_t a0, EmArea17OverlayFault *fault);
int em_area17_overlay_00823800(const EmArea17OverlayHooks *h, uint32_t a0, EmArea17OverlayFault *fault);
int em_area17_overlay_00823860(const EmArea17OverlayHooks *h, uint32_t a0, uint32_t sp, EmArea17OverlayFault *fault);
int em_area17_overlay_008239D0(const EmArea17OverlayHooks *h, uint32_t a0, EmArea17OverlayFault *fault);
int em_area17_overlay_00823BA0(const EmArea17OverlayHooks *h, uint32_t a0, EmArea17OverlayFault *fault);
int em_area17_overlay_008242D0(const EmArea17OverlayHooks *h, uint32_t a0, EmArea17OverlayFault *fault);
int em_area17_overlay_008243E0(const EmArea17OverlayHooks *h, uint32_t a0, EmArea17OverlayFault *fault);
int em_area17_overlay_008245A0(const EmArea17OverlayHooks *h, uint32_t a0, uint32_t sp, EmArea17OverlayFault *fault);
int em_area17_overlay_00824700(const EmArea17OverlayHooks *h, uint32_t a0, uint32_t sp, EmArea17OverlayFault *fault);
int em_area17_overlay_00824CF0(const EmArea17OverlayHooks *h, uint32_t a0, EmArea17OverlayFault *fault);
int em_area17_overlay_00824E40(const EmArea17OverlayHooks *h, uint32_t a0, EmArea17OverlayFault *fault);
int em_area17_overlay_008250E0(const EmArea17OverlayHooks *h, uint32_t a0, EmArea17OverlayFault *fault);
int em_area17_overlay_008251B0(const EmArea17OverlayHooks *h, uint32_t a0, EmArea17OverlayFault *fault);
int em_area17_overlay_00825470(const EmArea17OverlayHooks *h, uint32_t a0, EmArea17OverlayFault *fault);
int em_area17_overlay_00825560(const EmArea17OverlayHooks *h, uint32_t a0, EmArea17OverlayFault *fault);
int em_area17_overlay_00825EA0(const EmArea17OverlayHooks *h, uint32_t a0, EmArea17OverlayFault *fault);
int em_area17_overlay_008260C0(const EmArea17OverlayHooks *h, uint32_t a0, EmArea17OverlayFault *fault);
int em_area17_overlay_00826170(const EmArea17OverlayHooks *h, uint32_t a0, EmArea17OverlayFault *fault);
/* Script 0x8270A0's op09 callbacks. 0x823F70 takes (self, the op09
 * record, the script's work block); *result is its return value (1 =
 * done). 0x824280 reads no argument: two effects and a group, *result = 1.
 * *result is left unwritten when the call returns -1. */
int em_area17_overlay_00823F70(const EmArea17OverlayHooks *h, uint32_t a0, uint32_t a1, uint32_t a2, int32_t *result,
                               EmArea17OverlayFault *fault);
int em_area17_overlay_00824280(const EmArea17OverlayHooks *h, int32_t *result, EmArea17OverlayFault *fault);
#ifdef __cplusplus
}
#endif
#endif
