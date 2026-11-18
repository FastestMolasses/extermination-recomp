/* AREA21's own placement owners and script callbacks (OVERLAY/AREA21.BIN,
 * header id 0x12): [42] 0x8261A0; [43] / [44] 0x8262D0 with its two
 * halves 0x826310 (+3 == 0) and 0x8264D0 (+3 != 0); [45] 0x8266F0 with its
 * two steps 0x826840 and 0x8268F0; the op09 callbacks 0x8269A0 and
 * 0x8269F0 of script 0x82BEC0; [53] 0x827040 (the ending request) with its
 * script 0x82CBD0's op09 callback 0x827250; [58] 0x827CD0. Overlay
 * addresses are runtime addresses (the decomp's link names are 0x40
 * lower). Ground truth: the decomp's byte-identical C under
 * src/overlays/AREA21/; evidence: tools/test_area21_owners_reference.py,
 * which runs the ORIGINAL overlay code (the user's extract/OVERLAY/
 * AREA21.BIN loaded over a recording's arena).
 *
 * Same contract as em_area14_overlay.h: callees are explicit hooks by
 * original address, memory goes through `bytes`, a NULL or failing hook or
 * an unmapped address latches a fault (fail-stop) and the entry returns -1.
 * The node's +0x4C method goes through w_callback(function word, node). No
 * entry keeps a frame local, so none takes the stack pointer. */
#ifndef EM_AREA21_OWNERS_H
#define EM_AREA21_OWNERS_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct { uint32_t address; int32_t code; } EmArea21OwnersFault;
enum { EM_AREA21_OWNERS_FAULT_NONE = 0, EM_AREA21_OWNERS_FAULT_NULL_WORKER = 1,
       EM_AREA21_OWNERS_FAULT_WORKER_FAILED = 2, EM_AREA21_OWNERS_FAULT_BAD_ADDRESS = 5 };
typedef struct {
    void *ctx;
    uint8_t *(*bytes)(void *ctx, uint32_t address, uint32_t size);
/* BEGIN GENERATED HOOKS */
    int (*w_001026A0)(void *ctx, uint32_t a0, uint32_t a1, uint32_t a2);
    int (*w_001028B8)(void *ctx, uint32_t a0, uint32_t a1, uint32_t a2);
    int (*w_00102948)(void *ctx, uint32_t a0, uint32_t a1);
    int (*w_001281C0)(void *ctx, float a0, int32_t *result);
    int (*w_0015C700)(void *ctx, uint32_t a0);
    int (*w_0015C750)(void *ctx, uint32_t a0);
    int (*w_0019C6F0)(void *ctx, int32_t a0, int32_t a1);
    int (*w_001AEDE0)(void *ctx, int32_t a0, int32_t a1);
    int (*w_001AFC10)(void *ctx, uint32_t a0);
    int (*w_001B0FD0)(void *ctx, uint32_t a0, int32_t *result);
    int (*w_001B10B0)(void *ctx, uint32_t a0, int32_t a1, int32_t a2);
    int (*w_001B17A0)(void *ctx, uint32_t a0);
    int (*w_001B1B70)(void *ctx, uint32_t a0);
    int (*w_001B6660)(void *ctx, uint32_t a0, int32_t *result);
    int (*w_001B81D0)(void *ctx, uint32_t a0);
    int (*w_001BA1A0)(void *ctx, uint32_t a0, uint32_t a1);
    int (*w_001BA1C0)(void *ctx, uint32_t a0, int32_t a1, int32_t *result);
    int (*w_001BA1F0)(void *ctx, uint32_t a0, int32_t *result);
    int (*w_001BA540)(void *ctx, uint32_t a0);
    int (*w_001BA580)(void *ctx, uint32_t a0, int32_t a1);
    int (*w_001BA8E0)(void *ctx, uint32_t a0, int32_t a1);
    int (*w_001C6380)(void *ctx, uint32_t a0);
    int (*w_001C63E0)(void *ctx, uint32_t a0, int32_t a1);
    int (*w_001C64F0)(void *ctx, uint32_t a0, float a1);
    int (*w_001C67E0)(void *ctx, uint32_t a0, int32_t a1, float a2, float a3);
    int (*w_001C68C0)(void *ctx, uint32_t a0);
    int (*w_001CA6F0)(void *ctx, uint32_t a0, int32_t a1);
    int (*w_001D2830)(void *ctx, int32_t a0, int32_t a1);
    int (*w_001FA790)(void *ctx, int32_t a0, int32_t a1);
    int (*w_001FABB0)(void *ctx);
    int (*w_001FAE70)(void *ctx, int32_t a0);
    int (*w_001FB0B0)(void *ctx, int32_t a0);
    int (*w_001FBC50)(void *ctx);
/* END GENERATED HOOKS */
    int (*w_callback)(void *ctx, uint32_t function, uint32_t self);
} EmArea21OwnersHooks;
/* Placement owners and their called halves: one call of the original
 * behaviour on node a0. Return 0, or -1 on a fault. */
int em_area21_owners_008261A0(const EmArea21OwnersHooks *h, uint32_t a0, EmArea21OwnersFault *fault);
int em_area21_owners_008262D0(const EmArea21OwnersHooks *h, uint32_t a0, EmArea21OwnersFault *fault);
int em_area21_owners_00826310(const EmArea21OwnersHooks *h, uint32_t a0, EmArea21OwnersFault *fault);
int em_area21_owners_008264D0(const EmArea21OwnersHooks *h, uint32_t a0, EmArea21OwnersFault *fault);
int em_area21_owners_008266F0(const EmArea21OwnersHooks *h, uint32_t a0, EmArea21OwnersFault *fault);
int em_area21_owners_00826840(const EmArea21OwnersHooks *h, uint32_t a0, EmArea21OwnersFault *fault);
int em_area21_owners_008268F0(const EmArea21OwnersHooks *h, uint32_t a0, EmArea21OwnersFault *fault);
int em_area21_owners_00827040(const EmArea21OwnersHooks *h, uint32_t a0, EmArea21OwnersFault *fault);
int em_area21_owners_00827CD0(const EmArea21OwnersHooks *h, uint32_t a0, EmArea21OwnersFault *fault);
/* Script op09 callbacks. 0x8269A0 and 0x827250 read no argument; 0x8269F0
 * takes (self, the op09 record, the script's work block). *result is the
 * return value (1 = done); it is left unwritten when the call returns -1. */
int em_area21_owners_008269A0(const EmArea21OwnersHooks *h, int32_t *result, EmArea21OwnersFault *fault);
int em_area21_owners_008269F0(const EmArea21OwnersHooks *h, uint32_t a0, uint32_t a1, uint32_t a2, int32_t *result,
                              EmArea21OwnersFault *fault);
int em_area21_owners_00827250(const EmArea21OwnersHooks *h, int32_t *result, EmArea21OwnersFault *fault);
#ifdef __cplusplus
}
#endif
#endif
