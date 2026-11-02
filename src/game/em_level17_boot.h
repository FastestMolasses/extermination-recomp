/* Boot behaviours the second half's placements name (AREA16, AREA17, AREA20,
 * AREA21) that no earlier module translates. Same contract as
 * em_level16_boot.h: callees are explicit hooks, memory goes through
 * `bytes`, a NULL or failing hook latches a fault (fail-stop). Evidence:
 * tools/test_level17_boot_reference.py. */
#ifndef EM_LEVEL17_BOOT_H
#define EM_LEVEL17_BOOT_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct { uint32_t address; int32_t code; } EmLevel17BootFault;
enum { EM_LEVEL17_BOOT_FAULT_NONE = 0, EM_LEVEL17_BOOT_FAULT_NULL_WORKER = 1,
       EM_LEVEL17_BOOT_FAULT_WORKER_FAILED = 2, EM_LEVEL17_BOOT_FAULT_BAD_ADDRESS = 5 };
typedef struct {
    void *ctx;
    uint8_t *(*bytes)(void *ctx, uint32_t address, uint32_t size);
/* BEGIN GENERATED HOOKS */
    int (*w_001281C0)(void *ctx, float a0, int32_t *result);
    int (*w_001AFC10)(void *ctx, uint32_t a0);
    int (*w_001B0FD0)(void *ctx, uint32_t a0, int32_t *result);
    int (*w_001B1B70)(void *ctx, uint32_t a0);
    int (*w_001C6160)(void *ctx, uint32_t a0, int32_t *result);
    int (*w_001C6380)(void *ctx, uint32_t a0);
    int (*w_001F4E20)(void *ctx, uint32_t a0, uint32_t a1, float a2);
/* END GENERATED HOOKS */
    int (*w_callback)(void *ctx, uint32_t function, uint32_t self);
} EmLevel17BootHooks;
int em_level17_boot_001C2420(const EmLevel17BootHooks *h, uint32_t a0, EmLevel17BootFault *fault);
int em_level17_boot_001C4A00(const EmLevel17BootHooks *h, uint32_t a0, EmLevel17BootFault *fault);
int em_level17_boot_001C6190(const EmLevel17BootHooks *h, uint32_t a0, int32_t *result, EmLevel17BootFault *fault);
/* AREA21's overlay twins of AREA01's 0x8239C0 (only the rate differs). */
int em_level17_boot_a21_00823A10(const EmLevel17BootHooks *h, uint32_t a0, EmLevel17BootFault *fault);
int em_level17_boot_a21_00823AB0(const EmLevel17BootHooks *h, uint32_t a0, EmLevel17BootFault *fault);
#ifdef __cplusplus
}
#endif
#endif
