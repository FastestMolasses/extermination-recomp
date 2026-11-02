/* AREA21's shared creature helpers and its first creature: the spawner
 * 001283D0, the small creature 0014C220 (its states 0014C2E0 / 0014C3A0 /
 * 0014CCC0 / 0014CD00, its moves 0014C680 / 0014C9D0 / 0014C490, its
 * motion 0014CD30 and its attack 0014CDD0) and its spawner 0014D1E0, and
 * the creature 00137830 (AREA21 code group 0x82A600 [0]'s behaviour) with
 * its states 00137920 / 00137A50 / 00137C80 / 00137E80, its damage step
 * 001381D0 and its two rig followers 00137EB0. Two other AREA21 creatures
 * (0014EC00, 00151590) reach 001283D0 and 0014D1E0 through the runtime.
 * Same contract as em_level17_boot.h: callees are explicit hooks, memory
 * goes through `bytes`, a NULL or failing hook latches a fault
 * (fail-stop). Evidence: tools/test_area21_boot_reference.py. */
#ifndef EM_AREA21_BOOT_H
#define EM_AREA21_BOOT_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct { uint32_t address; int32_t code; } EmArea21BootFault;
enum { EM_AREA21_BOOT_FAULT_NONE = 0, EM_AREA21_BOOT_FAULT_NULL_WORKER = 1,
       EM_AREA21_BOOT_FAULT_WORKER_FAILED = 2, EM_AREA21_BOOT_FAULT_BAD_ADDRESS = 5 };
typedef struct {
    void *ctx;
    uint8_t *(*bytes)(void *ctx, uint32_t address, uint32_t size);
/* BEGIN GENERATED HOOKS */
    int (*w_001026A0)(void *ctx, uint32_t a0, uint32_t a1, uint32_t a2);
    int (*w_001028B8)(void *ctx, uint32_t a0, uint32_t a1, uint32_t a2);
    int (*w_00102948)(void *ctx, uint32_t a0, uint32_t a1);
    int (*w_00102958)(void *ctx, uint32_t a0, uint32_t a1);
    int (*w_001029C0)(void *ctx, uint32_t a0);
    int (*w_00102B08)(void *ctx, uint32_t a0, uint32_t a1, float a2);
    int (*w_0011DE90)(void *ctx, float a0, float *result);
    int (*w_0011DF78)(void *ctx, float a0, float *result);
    int (*w_0011E2A8)(void *ctx, float a0, float *result);
    int (*w_0011E620)(void *ctx, float a0, float a1, float *result);
    int (*w_00122BB8)(void *ctx, int32_t *result);
    int (*w_00131ED0)(void *ctx, uint32_t a0);
    int (*w_0019AB20)(void *ctx, uint32_t a0, uint32_t a1, uint32_t a2, int32_t a3, int32_t *result);
    int (*w_0019AD00)(void *ctx, uint32_t a0, uint32_t a1, int32_t a2, int32_t *result);
    int (*w_001A7B80)(void *ctx, uint32_t a0, int32_t *result);
    int (*w_001AF780)(void *ctx, uint32_t *result);
    int (*w_001AFA90)(void *ctx, int32_t a0, uint32_t *result);
    int (*w_001AFC10)(void *ctx, uint32_t a0);
    int (*w_001B10B0)(void *ctx, uint32_t a0, int32_t a1, int32_t a2, int32_t *result);
    int (*w_001B1190)(void *ctx, int32_t a0);
    int (*w_001B1240)(void *ctx, uint32_t a0, float a1, float a2, float *result);
    int (*w_001B12B0)(void *ctx, float a0, float a1, float a2, float *result);
    int (*w_001B1470)(void *ctx, float a0, float *result);
    int (*w_001B1560)(void *ctx, uint32_t a0, uint32_t a1, float a2, int32_t *result);
    int (*w_001B15D0)(void *ctx, uint32_t a0, uint32_t a1, float *result);
    int (*w_001B17A0)(void *ctx, uint32_t a0, int32_t *result);
    int (*w_001B2B10)(void *ctx, uint32_t a0, uint32_t a1, uint32_t a2);
    int (*w_001B3C30)(void *ctx, uint32_t a0, uint32_t a1, uint32_t a2, float a3, float a4);
    int (*w_001B4810)(void *ctx, uint32_t a0);
    int (*w_001B4CF0)(void *ctx, uint32_t a0);
    int (*w_001B5360)(void *ctx, uint32_t a0);
    int (*w_001C6150)(void *ctx, uint32_t a0, int32_t *result);
    int (*w_001C62C0)(void *ctx, uint32_t a0);
    int (*w_001C63E0)(void *ctx, uint32_t a0, int32_t a1);
    int (*w_001C64F0)(void *ctx, uint32_t a0, float a1, int32_t *result);
    int (*w_001C67E0)(void *ctx, uint32_t a0, int32_t a1, float a2, float a3);
    int (*w_001C68C0)(void *ctx, uint32_t a0);
    int (*w_001CA6E0)(void *ctx, uint32_t a0, uint32_t a1);
    int (*w_001CB5B0)(void *ctx, int32_t a0);
    int (*w_001EFD20)(void *ctx, int32_t a0, uint32_t a1);
    int (*w_001EFD90)(void *ctx, int32_t a0, uint32_t a1, uint32_t a2);
    int (*w_001EFE00)(void *ctx, int32_t a0, uint32_t a1, int32_t *result);
    int (*w_001FBD50)(void *ctx, uint32_t a0, int32_t a1, int32_t a2, float a3, int32_t *result);
    int (*w_0021BE40)(void *ctx, uint32_t a0, uint32_t a1, int32_t *result);
/* END GENERATED HOOKS */
    int (*w_callback)(void *ctx, uint32_t function, uint32_t self);
} EmArea21BootHooks;
int em_area21_boot_001283D0(const EmArea21BootHooks *h, uint32_t a0, int32_t a1, uint32_t a2, int32_t *result, EmArea21BootFault *fault);
int em_area21_boot_0014C220(const EmArea21BootHooks *h, uint32_t a0, EmArea21BootFault *fault);
int em_area21_boot_0014C2E0(const EmArea21BootHooks *h, uint32_t a0, uint32_t a1, EmArea21BootFault *fault);
int em_area21_boot_0014C3A0(const EmArea21BootHooks *h, uint32_t a0, uint32_t a1, EmArea21BootFault *fault);
int em_area21_boot_0014C490(const EmArea21BootHooks *h, uint32_t a0, uint32_t a1, EmArea21BootFault *fault);
int em_area21_boot_0014C680(const EmArea21BootHooks *h, uint32_t a0, uint32_t a1, EmArea21BootFault *fault);
int em_area21_boot_0014C9D0(const EmArea21BootHooks *h, uint32_t a0, uint32_t a1, EmArea21BootFault *fault);
int em_area21_boot_0014CCC0(const EmArea21BootHooks *h, uint32_t a0, EmArea21BootFault *fault);
int em_area21_boot_0014CD00(const EmArea21BootHooks *h, uint32_t a0, EmArea21BootFault *fault);
int em_area21_boot_0014CD30(const EmArea21BootHooks *h, uint32_t a0, uint32_t a1, EmArea21BootFault *fault);
int em_area21_boot_0014CDD0(const EmArea21BootHooks *h, uint32_t a0, uint32_t a1, EmArea21BootFault *fault);
int em_area21_boot_0014D1E0(const EmArea21BootHooks *h, uint32_t a0, uint32_t a1, int32_t a2, EmArea21BootFault *fault);
int em_area21_boot_00137830(const EmArea21BootHooks *h, uint32_t a0, EmArea21BootFault *fault);
int em_area21_boot_00137920(const EmArea21BootHooks *h, uint32_t a0, uint32_t a1, EmArea21BootFault *fault);
int em_area21_boot_00137A50(const EmArea21BootHooks *h, uint32_t a0, uint32_t a1, EmArea21BootFault *fault);
int em_area21_boot_00137C80(const EmArea21BootHooks *h, uint32_t a0, uint32_t a1, EmArea21BootFault *fault);
int em_area21_boot_00137E80(const EmArea21BootHooks *h, uint32_t a0, EmArea21BootFault *fault);
int em_area21_boot_00137EB0(const EmArea21BootHooks *h, uint32_t a0, EmArea21BootFault *fault);
int em_area21_boot_001381D0(const EmArea21BootHooks *h, uint32_t a0, uint32_t a1, int32_t *result, EmArea21BootFault *fault);
#ifdef __cplusplus
}
#endif
#endif
