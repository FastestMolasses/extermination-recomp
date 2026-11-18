/* AREA20's own overlay owners (OVERLAY/AREA20.BIN, header id 0x11; sub 0,
 * docs/AREA20_PORT.md): [32] 0x8239B0 (the arrival script 0x826830 and the
 * AREA21 request through the quad 0x8273B0) with its two callees 0x823B00
 * and 0x823BE0; the breakables [19]..[31] 0x823D30; and the five effect
 * objects (0x823580, 0x823600, 0x823780 in the arrival script's node
 * lists; 0x823660, 0x823840 named only by boot data). Overlay
 * addresses are runtime addresses (the decomp's link names are 0x40
 * lower). Ground truth: the decomp's byte-identical C under
 * src/overlays/AREA20/; evidence: tools/test_area20_overlay_reference.py,
 * which runs the ORIGINAL overlay code.
 *
 * Same contract as em_area14_overlay.h: callees are explicit hooks by
 * original address, memory goes through `bytes`, a NULL or failing hook or
 * an unmapped address latches a fault (fail-stop) and the entry returns -1.
 * `sp` is the original entry stack pointer: 0x823BE0 copies the 64-byte
 * quad record 0x8273B0 into its frame and passes the copy to 001B1EA0
 * (0x8239B0 calls it, so 0x8239B0 takes its own entry sp as well). */
#ifndef EM_AREA20_OVERLAY_H
#define EM_AREA20_OVERLAY_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct { uint32_t address; int32_t code; } EmArea20OverlayFault;
enum { EM_AREA20_OVERLAY_FAULT_NONE = 0, EM_AREA20_OVERLAY_FAULT_NULL_WORKER = 1,
       EM_AREA20_OVERLAY_FAULT_WORKER_FAILED = 2, EM_AREA20_OVERLAY_FAULT_BAD_ADDRESS = 5 };
typedef struct {
    void *ctx;
    uint8_t *(*bytes)(void *ctx, uint32_t address, uint32_t size);
/* BEGIN GENERATED HOOKS */
    int (*w_00102918)(void *ctx, uint32_t a0, uint32_t a1, uint32_t a2);
    int (*w_00102948)(void *ctx, uint32_t a0, uint32_t a1);
    int (*w_001029C0)(void *ctx, uint32_t a0);
    int (*w_00102B08)(void *ctx, uint32_t a0, uint32_t a1, float a2);
    int (*w_00102C58)(void *ctx, uint32_t a0, uint32_t a1, uint32_t a2);
    int (*w_0011DE90)(void *ctx, float a0, float *result);
    int (*w_0011E2A8)(void *ctx, float a0, float *result);
    int (*w_00122BB8)(void *ctx, int32_t *result);
    int (*w_0019A570)(void *ctx, uint32_t a0, uint32_t a1, int32_t a2, int32_t a3, int32_t *result);
    int (*w_0019AB20)(void *ctx, uint32_t a0, uint32_t a1, uint32_t a2, int32_t a3, int32_t *result);
    int (*w_0019AD00)(void *ctx, uint32_t a0, uint32_t a1, int32_t a2, int32_t *result);
    int (*w_001A2370)(void *ctx, uint32_t a0, uint32_t a1);
    int (*w_001AFC10)(void *ctx, uint32_t a0);
    int (*w_001B0C60)(void *ctx, int32_t a0, int32_t a1, int32_t a2);
    int (*w_001B0D80)(void *ctx, uint32_t a0);
    int (*w_001B0FD0)(void *ctx, uint32_t a0, int32_t *result);
    int (*w_001B10B0)(void *ctx, uint32_t a0, int32_t a1, int32_t a2);
    int (*w_001B1470)(void *ctx, float a0, float *result);
    int (*w_001B17A0)(void *ctx, uint32_t a0);
    int (*w_001B1B70)(void *ctx, uint32_t a0);
    int (*w_001B1EA0)(void *ctx, int32_t a0, uint32_t a1, uint32_t a2, int32_t a3, int32_t *result);
    int (*w_001BA1A0)(void *ctx, uint32_t a0, uint32_t a1);
    int (*w_001BA1C0)(void *ctx, uint32_t a0, int32_t a1, int32_t *result);
    int (*w_001BA1F0)(void *ctx, uint32_t a0, int32_t *result);
    int (*w_001BA540)(void *ctx, uint32_t a0);
    int (*w_001BA580)(void *ctx, uint32_t a0, int32_t a1);
    int (*w_001BA8E0)(void *ctx, uint32_t a0, int32_t a1);
    int (*w_001C6190)(void *ctx, uint32_t a0, int32_t *result);
    int (*w_001C6380)(void *ctx, uint32_t a0);
    int (*w_001C63E0)(void *ctx, uint32_t a0, int32_t a1);
    int (*w_001C64F0)(void *ctx, uint32_t a0, float a1, int32_t *result);
    int (*w_001C68C0)(void *ctx, uint32_t a0);
    int (*w_001CA6F0)(void *ctx, uint32_t a0, int32_t a1);
    int (*w_001D04B0)(void *ctx, uint32_t a0, int32_t a1, uint32_t a2, float a3, float a4);
    int (*w_001EFD20)(void *ctx, int32_t a0, uint32_t a1);
    int (*w_001EFEB0)(void *ctx, int32_t a0, uint32_t a1, uint32_t *result);
    int (*w_001F0460)(void *ctx, int32_t a0, uint32_t a1);
    int (*w_001F9100)(void *ctx, uint32_t a0, uint32_t a1, uint32_t a2, float a3);
    int (*w_001FABB0)(void *ctx);
    int (*w_001FB0B0)(void *ctx, int32_t a0);
    int (*w_001FC580)(void *ctx, uint32_t a0, int32_t a1);
/* END GENERATED HOOKS */
    int (*w_callback)(void *ctx, uint32_t function, uint32_t self);
} EmArea20OverlayHooks;
/* One call of the original behaviour each. Return 0, or -1 on a fault. */
int em_area20_overlay_00823580(const EmArea20OverlayHooks *h, uint32_t a0, EmArea20OverlayFault *fault);
int em_area20_overlay_00823600(const EmArea20OverlayHooks *h, uint32_t a0, EmArea20OverlayFault *fault);
int em_area20_overlay_00823660(const EmArea20OverlayHooks *h, uint32_t a0, EmArea20OverlayFault *fault);
int em_area20_overlay_00823780(const EmArea20OverlayHooks *h, uint32_t a0, EmArea20OverlayFault *fault);
int em_area20_overlay_00823840(const EmArea20OverlayHooks *h, uint32_t a0, EmArea20OverlayFault *fault);
int em_area20_overlay_008239B0(const EmArea20OverlayHooks *h, uint32_t a0, uint32_t sp, EmArea20OverlayFault *fault);
int em_area20_overlay_00823B00(const EmArea20OverlayHooks *h, uint32_t a0, EmArea20OverlayFault *fault);
int em_area20_overlay_00823BE0(const EmArea20OverlayHooks *h, uint32_t a0, uint32_t sp, EmArea20OverlayFault *fault);
int em_area20_overlay_00823D30(const EmArea20OverlayHooks *h, uint32_t a0, EmArea20OverlayFault *fault);
#ifdef __cplusplus
}
#endif
#endif
