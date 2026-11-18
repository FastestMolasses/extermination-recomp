/* AREA21's doors and small sub-0 owners (OVERLAY/AREA21.BIN, header id
 * 0x12; docs of the decomp: docs/AREA21_OVERLAY.md): the doors [30]
 * 0x826BB0 and [27] / [33] 0x826E60, [54] / [55] 0x82A1D0, the rounds
 * 0x825C70 with its code pointer 0x825C60, and further sub-0 owners listed
 * below. Overlay addresses are runtime addresses (the decomp's link names
 * are 0x40 lower). Ground truth: the decomp's C under src/overlays/AREA21/;
 * evidence: tools/test_area21_doors_reference.py, which runs the ORIGINAL
 * overlay code (the user's extract/OVERLAY/AREA21.BIN over a recording's
 * arena; no AREA21 recording exists).
 *
 * Same contract as em_area14_overlay.h: callees are explicit hooks by
 * original address, memory goes through `bytes`, a NULL or failing hook or
 * an unmapped address latches a fault (fail-stop) and the entry returns -1.
 * w_callback is a node's +0x4C method (function word, node). */
#ifndef EM_AREA21_DOORS_H
#define EM_AREA21_DOORS_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct { uint32_t address; int32_t code; } EmArea21DoorsFault;
enum { EM_AREA21_DOORS_FAULT_NONE = 0, EM_AREA21_DOORS_FAULT_NULL_WORKER = 1,
       EM_AREA21_DOORS_FAULT_WORKER_FAILED = 2, EM_AREA21_DOORS_FAULT_BAD_ADDRESS = 5 };
typedef struct {
    void *ctx;
    uint8_t *(*bytes)(void *ctx, uint32_t address, uint32_t size);
/* BEGIN GENERATED HOOKS */
    int (*w_00102760)(void *ctx, uint32_t a0, uint32_t a1);
    int (*w_001028B8)(void *ctx, uint32_t a0, uint32_t a1, uint32_t a2);
    int (*w_00102948)(void *ctx, uint32_t a0, uint32_t a1);
    int (*w_00103230)(void *ctx, uint32_t a0, uint32_t a1, float a2);
    int (*w_0011DF78)(void *ctx, float a0, float *result);
    int (*w_00122BB8)(void *ctx, int32_t *result);
    int (*w_0015AC00)(void *ctx, uint32_t a0, uint32_t a1);
    int (*w_0015AE20)(void *ctx, uint32_t a0, uint32_t a1);
    int (*w_0019A570)(void *ctx, uint32_t a0, uint32_t a1, int32_t a2, int32_t a3, int32_t *result);
    int (*w_001A2370)(void *ctx, uint32_t a0, uint32_t a1);
    int (*w_001AFC10)(void *ctx, uint32_t a0);
    int (*w_001B0FD0)(void *ctx, uint32_t a0, int32_t *result);
    int (*w_001B1190)(void *ctx, int32_t a0);
    int (*w_001B1470)(void *ctx, float a0, float *result);
    int (*w_001B17A0)(void *ctx, uint32_t a0, int32_t *result);
    int (*w_001B1B70)(void *ctx, uint32_t a0);
    int (*w_001B41F0)(void *ctx, uint32_t a0, uint32_t a1, uint32_t a2, int32_t a3, int32_t a4, int32_t a5);
    int (*w_001BA1A0)(void *ctx, uint32_t a0, uint32_t a1);
    int (*w_001BBDA0)(void *ctx, uint32_t a0);
    int (*w_001BBE40)(void *ctx, uint32_t a0, uint32_t a1, int32_t a2, int32_t *result);
    int (*w_001BC0E0)(void *ctx, uint32_t a0, uint32_t a1, int32_t *result);
    int (*w_001BC240)(void *ctx, uint32_t a0, uint32_t a1);
    int (*w_001BC290)(void *ctx, uint32_t a0, uint32_t a1, int32_t *result);
    int (*w_001BC300)(void *ctx, uint32_t a0);
    int (*w_001C6120)(void *ctx, int32_t a0, int32_t a1, int32_t *result);
    int (*w_001C62C0)(void *ctx, uint32_t a0);
    int (*w_001C6380)(void *ctx, uint32_t a0);
    int (*w_001CA6E0)(void *ctx, uint32_t a0, int32_t a1);
    int (*w_001CD390)(void *ctx, uint32_t a0, uint32_t a1);
    int (*w_001CEEE0)(void *ctx, uint32_t a0, uint32_t a1, uint32_t a2, int32_t a3, float a4, float a5, float a6);
    int (*w_001EFD20)(void *ctx, uint32_t a0, uint32_t a1);
    int (*w_001EFD90)(void *ctx, uint32_t a0, uint32_t a1, uint32_t a2);
    int (*w_001F02C0)(void *ctx, uint32_t a0, int32_t a1, float a2);
    int (*w_001FC3C0)(void *ctx, uint32_t a0, uint32_t a1, int32_t a2, float a3, float a4);
/* END GENERATED HOOKS */
    int (*w_callback)(void *ctx, uint32_t function, uint32_t self);
} EmArea21DoorsHooks;
/* Placement owners: one call of the original behaviour on the node a0.
 * Return 0, or -1 on a fault. */
int em_area21_doors_00826BB0(const EmArea21DoorsHooks *h, uint32_t a0, EmArea21DoorsFault *fault);
int em_area21_doors_00826E60(const EmArea21DoorsHooks *h, uint32_t a0, EmArea21DoorsFault *fault);
int em_area21_doors_0082A1D0(const EmArea21DoorsHooks *h, uint32_t a0, EmArea21DoorsFault *fault);
int em_area21_doors_00825C70(const EmArea21DoorsHooks *h, uint32_t a0, EmArea21DoorsFault *fault);
/* 0x825C60: the rounds' +0x34 code pointer, an empty function (it reads
 * and writes nothing; a0 is ignored). */
int em_area21_doors_00825C60(const EmArea21DoorsHooks *h, uint32_t a0, EmArea21DoorsFault *fault);
/* [47] 0x8272C0, [49] / [50] 0x8273C0, [51] 0x827640, [56] 0x827880,
 * [57] 0x827A30, [59] / [60] 0x827E70 and the deferred group's 0x828350
 * (sub 0). */
int em_area21_doors_008272C0(const EmArea21DoorsHooks *h, uint32_t a0, EmArea21DoorsFault *fault);
int em_area21_doors_008273C0(const EmArea21DoorsHooks *h, uint32_t a0, EmArea21DoorsFault *fault);
int em_area21_doors_00827640(const EmArea21DoorsHooks *h, uint32_t a0, EmArea21DoorsFault *fault);
int em_area21_doors_00827880(const EmArea21DoorsHooks *h, uint32_t a0, EmArea21DoorsFault *fault);
int em_area21_doors_00827A30(const EmArea21DoorsHooks *h, uint32_t a0, EmArea21DoorsFault *fault);
int em_area21_doors_00827E70(const EmArea21DoorsHooks *h, uint32_t a0, EmArea21DoorsFault *fault);
int em_area21_doors_00828350(const EmArea21DoorsHooks *h, uint32_t a0, EmArea21DoorsFault *fault);
#ifdef __cplusplus
}
#endif
#endif
