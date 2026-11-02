/* The AREA17 creature: AREA17 code group 0x8263C0 [0]'s behaviour 00133E30,
 * its state handlers and move steps, the spit 001370C0 / 001373B0 and the
 * haze grid 001E04E0 / 001E0240 / 001E00F0. Same contract as
 * em_level17_boot.h: callees are explicit hooks, memory goes through
 * `bytes`, a NULL or failing hook latches a fault (fail-stop). Entries that
 * take `sp` get the original stack pointer (their frame locals live below
 * it). Evidence: tools/test_area17_creature_reference.py. */
#ifndef EM_AREA17_CREATURE_H
#define EM_AREA17_CREATURE_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct { uint32_t address; int32_t code; } EmArea17CreatureFault;
enum { EM_AREA17_CREATURE_FAULT_NONE = 0, EM_AREA17_CREATURE_FAULT_NULL_WORKER = 1,
       EM_AREA17_CREATURE_FAULT_WORKER_FAILED = 2, EM_AREA17_CREATURE_FAULT_BAD_ADDRESS = 5 };
typedef struct {
    void *ctx;
    uint8_t *(*bytes)(void *ctx, uint32_t address, uint32_t size);
/* BEGIN GENERATED HOOKS */
    int (*w_001026A0)(void *ctx, uint32_t a0, uint32_t a1, uint32_t a2);
    int (*w_00102760)(void *ctx, uint32_t a0, uint32_t a1);
    int (*w_001028B8)(void *ctx, uint32_t a0, uint32_t a1, uint32_t a2);
    int (*w_00102948)(void *ctx, uint32_t a0, uint32_t a1);
    int (*w_00102958)(void *ctx, uint32_t a0, uint32_t a1);
    int (*w_001029C0)(void *ctx, uint32_t a0);
    int (*w_00102BB0)(void *ctx, uint32_t a0, uint32_t a1, float a2);
    int (*w_001031E0)(void *ctx, uint32_t a0, uint32_t a1);
    int (*w_00103230)(void *ctx, uint32_t a0, uint32_t a1, float a2);
    int (*w_0011DE90)(void *ctx, float a0, float *result);
    int (*w_0011DF78)(void *ctx, float a0, float *result);
    int (*w_0011E2A8)(void *ctx, float a0, float *result);
    int (*w_0011E620)(void *ctx, float a0, float a1, float *result);
    int (*w_00122BB8)(void *ctx, int32_t *result);
    int (*w_001281C0)(void *ctx, float a0, int32_t *result);
    int (*w_00131ED0)(void *ctx, uint32_t a0);
    int (*w_0019A570)(void *ctx, uint32_t a0, uint32_t a1, int32_t a2, int32_t a3, int32_t *result);
    int (*w_001A7B80)(void *ctx, uint32_t a0, int32_t *result);
    int (*w_001AFA90)(void *ctx, int32_t a0, uint32_t *result);
    int (*w_001AFC10)(void *ctx, uint32_t a0);
    int (*w_001B0D80)(void *ctx, uint32_t a0);
    int (*w_001B10B0)(void *ctx, uint32_t a0, int32_t a1, int32_t a2, int32_t *result);
    int (*w_001B1190)(void *ctx, int32_t a0);
    int (*w_001B1240)(void *ctx, uint32_t a0, float a1, float a2, float *result);
    int (*w_001B12B0)(void *ctx, float a0, float a1, float a2, float *result);
    int (*w_001B1380)(void *ctx, uint32_t a0, uint32_t a1, float a2, int32_t *result);
    int (*w_001B1470)(void *ctx, float a0, float *result);
    int (*w_001B15D0)(void *ctx, uint32_t a0, uint32_t a1, float *result);
    int (*w_001B17A0)(void *ctx, uint32_t a0, int32_t *result);
    int (*w_001B3440)(void *ctx, uint32_t a0, uint32_t a1, uint32_t a2, float a3, int32_t *result);
    int (*w_001B3C30)(void *ctx, uint32_t a0, uint32_t a1, uint32_t a2, float a3, float a4);
    int (*w_001B5360)(void *ctx, uint32_t a0);
    int (*w_001B55E0)(void *ctx, uint32_t a0, int32_t a1);
    int (*w_001C63E0)(void *ctx, uint32_t a0, int32_t a1);
    int (*w_001C64F0)(void *ctx, uint32_t a0, float a1, int32_t *result);
    int (*w_001C67E0)(void *ctx, uint32_t a0, int32_t a1, float a2, float a3);
    int (*w_001C68C0)(void *ctx, uint32_t a0);
    int (*w_001C94B0)(void *ctx, uint32_t a0, uint32_t a1, uint32_t a2, uint32_t a3);
    int (*w_001CAAC0)(void *ctx, uint32_t a0, uint32_t a1);
    int (*w_001CD370)(void *ctx, int32_t a0, uint32_t *result);
    int (*w_001D1F20)(void *ctx, int32_t a0);
    int (*w_001D1F80)(void *ctx, int32_t a0, int32_t a1, int32_t a2);
    int (*w_001D1FF0)(void *ctx, int32_t a0, int32_t a1);
    int (*w_001D6B10)(void *ctx, int32_t a0, int32_t a1, int32_t a2, int32_t a3);
    int (*w_001D6BA0)(void *ctx, int32_t a0, int32_t a1, int32_t a2, int32_t a3, int32_t a4, int32_t a5);
    int (*w_001DA6A0)(void *ctx, uint32_t a0);
    int (*w_001EFD90)(void *ctx, int32_t a0, uint32_t a1, uint32_t a2);
    int (*w_001EFE00)(void *ctx, int32_t a0, uint32_t a1, int32_t *result);
    int (*w_001F4E20)(void *ctx, uint32_t a0, uint32_t a1, float a2);
    int (*w_001F91C0)(void *ctx, uint32_t a0);
    int (*w_001FBD50)(void *ctx, uint32_t a0, int32_t a1, int32_t a2, float a3, int32_t *result);
    int (*w_0021BE40)(void *ctx, uint32_t a0, uint32_t a1, int32_t *result);
/* END GENERATED HOOKS */
    int (*w_callback)(void *ctx, uint32_t function, uint32_t self);
} EmArea17CreatureHooks;
int em_area17_creature_00133E30(const EmArea17CreatureHooks *h, uint32_t a0, uint32_t sp, EmArea17CreatureFault *fault);
int em_area17_creature_00133FD0(const EmArea17CreatureHooks *h, uint32_t a0, uint32_t a1, EmArea17CreatureFault *fault);
int em_area17_creature_00134090(const EmArea17CreatureHooks *h, uint32_t a0, uint32_t a1, uint32_t sp, EmArea17CreatureFault *fault);
int em_area17_creature_00134300(const EmArea17CreatureHooks *h, uint32_t a0, uint32_t a1, uint32_t sp, EmArea17CreatureFault *fault);
int em_area17_creature_00134500(const EmArea17CreatureHooks *h, uint32_t a0, uint32_t a1, EmArea17CreatureFault *fault);
int em_area17_creature_001348E0(const EmArea17CreatureHooks *h, uint32_t a0, uint32_t a1, EmArea17CreatureFault *fault);
int em_area17_creature_00134DC0(const EmArea17CreatureHooks *h, uint32_t a0, uint32_t a1, EmArea17CreatureFault *fault);
int em_area17_creature_00134FF0(const EmArea17CreatureHooks *h, uint32_t a0, uint32_t a1, EmArea17CreatureFault *fault);
int em_area17_creature_001352E0(const EmArea17CreatureHooks *h, uint32_t a0, uint32_t a1, EmArea17CreatureFault *fault);
int em_area17_creature_00135870(const EmArea17CreatureHooks *h, uint32_t a0, uint32_t a1, EmArea17CreatureFault *fault);
int em_area17_creature_00135D00(const EmArea17CreatureHooks *h, uint32_t a0, uint32_t a1, EmArea17CreatureFault *fault);
int em_area17_creature_00136140(const EmArea17CreatureHooks *h, uint32_t a0, uint32_t a1, EmArea17CreatureFault *fault);
int em_area17_creature_00136240(const EmArea17CreatureHooks *h, uint32_t a0, uint32_t a1, EmArea17CreatureFault *fault);
int em_area17_creature_00136490(const EmArea17CreatureHooks *h, uint32_t a0, uint32_t a1, EmArea17CreatureFault *fault);
int em_area17_creature_00136600(const EmArea17CreatureHooks *h, uint32_t a0, EmArea17CreatureFault *fault);
int em_area17_creature_00136630(const EmArea17CreatureHooks *h, uint32_t a0, uint32_t a1, EmArea17CreatureFault *fault);
int em_area17_creature_001366E0(const EmArea17CreatureHooks *h, uint32_t a0, uint32_t a1, EmArea17CreatureFault *fault);
int em_area17_creature_001368D0(const EmArea17CreatureHooks *h, uint32_t a0, uint32_t a1, int32_t *result, EmArea17CreatureFault *fault);
int em_area17_creature_00136C40(const EmArea17CreatureHooks *h, int32_t a0, uint32_t a1, uint32_t a2, float a3, EmArea17CreatureFault *fault);
int em_area17_creature_00136CB0(const EmArea17CreatureHooks *h, uint32_t a0, uint32_t a1, EmArea17CreatureFault *fault);
int em_area17_creature_00136F20(const EmArea17CreatureHooks *h, uint32_t a0, int32_t a1, EmArea17CreatureFault *fault);
int em_area17_creature_001370C0(const EmArea17CreatureHooks *h, int32_t a0, uint32_t a1, EmArea17CreatureFault *fault);
int em_area17_creature_001373B0(const EmArea17CreatureHooks *h, uint32_t a0, EmArea17CreatureFault *fault);
int em_area17_creature_001E00F0(const EmArea17CreatureHooks *h, uint32_t a0, uint32_t a1, int32_t a2, EmArea17CreatureFault *fault);
int em_area17_creature_001E0240(const EmArea17CreatureHooks *h, uint32_t a0, uint32_t a1, int32_t a2, int32_t a3, uint32_t sp, int32_t *result, EmArea17CreatureFault *fault);
int em_area17_creature_001E04E0(const EmArea17CreatureHooks *h, uint32_t a0, int32_t a1, float a2, uint32_t sp, EmArea17CreatureFault *fault);
#ifdef __cplusplus
}
#endif
#endif
