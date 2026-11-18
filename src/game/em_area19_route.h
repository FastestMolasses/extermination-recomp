/* AREA19 route owners omitted by the earlier delta inventory. UNWIRED.
 * Original pointers are 32-bit; bytes returns a borrowed view consumed before
 * the next access or call. Workers succeed at >=0 and fail at <0. A latched
 * fault prevents all later observable accesses/calls. No owned game data.
 * Evidence: tools/test_area19_route_reference.py. */
#ifndef EM_AREA19_ROUTE_H
#define EM_AREA19_ROUTE_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct { uint32_t address; int32_t code; } EmArea19RouteFault;
enum { EM_AREA19_ROUTE_FAULT_NONE = 0, EM_AREA19_ROUTE_FAULT_NULL_WORKER = 1,
       EM_AREA19_ROUTE_FAULT_WORKER_FAILED = 2, EM_AREA19_ROUTE_FAULT_BAD_ADDRESS = 5 };
typedef struct {
    void *ctx;
    uint8_t *(*bytes)(void *ctx, uint32_t address, uint32_t size);
/* BEGIN GENERATED HOOKS */
    int (*w_001026A0)(void *ctx, uint32_t a0, uint32_t a1, uint32_t a2);
    int (*w_00102760)(void *ctx, uint32_t a0, uint32_t a1);
    int (*w_001028D0)(void *ctx, uint32_t a0, uint32_t a1, uint32_t a2);
    int (*w_00102918)(void *ctx, uint32_t a0, uint32_t a1, uint32_t a2);
    int (*w_00102948)(void *ctx, uint32_t a0, uint32_t a1);
    int (*w_00102958)(void *ctx, uint32_t a0, uint32_t a1);
    int (*w_001029C0)(void *ctx, uint32_t a0);
    int (*w_00102C58)(void *ctx, uint32_t a0, uint32_t a1, uint32_t a2);
    int (*w_0011DBB8)(void *ctx, float a0, float *result);
    int (*w_0011DF78)(void *ctx, float a0, float *result);
    int (*w_0011E2A8)(void *ctx, float a0, float *result);
    int (*w_0011E520)(void *ctx, float a0, float *result);
    int (*w_0011E748)(void *ctx, float a0, float *result);
    int (*w_00122BB8)(void *ctx, int32_t *result);
    int (*w_0019B6C0)(void *ctx, uint32_t a0, uint32_t a1, uint32_t a2, int32_t *result);
    int (*w_001A2370)(void *ctx, uint32_t a0, uint32_t a1);
    int (*w_001AFA90)(void *ctx, int32_t a0, uint32_t *result);
    int (*w_001AFC10)(void *ctx, uint32_t a0);
    int (*w_001B0FD0)(void *ctx, uint32_t a0, int32_t *result);
    int (*w_001B1470)(void *ctx, float a0, float *result);
    int (*w_001B17A0)(void *ctx, uint32_t a0, int32_t *result);
    int (*w_001C6380)(void *ctx, uint32_t a0);
    int (*w_001CCF70)(void *ctx, uint32_t a0, int32_t *result);
    int (*w_001CFA60)(void *ctx, uint32_t a0, uint32_t a1, float a2, float a3);
    int (*w_001CFBE0)(void *ctx, int32_t a0, int32_t a1, uint32_t a2, uint32_t a3, int32_t a4);
    int (*w_001E8B90)(void *ctx, uint32_t a0, float a1);
    int (*w_001EFD90)(void *ctx, int32_t a0, uint32_t a1, uint32_t a2, int32_t *result);
    int (*w_001FBD50)(void *ctx, uint32_t a0, int32_t a1, int32_t a2, float a3, int32_t *result);
    int (*w_001FC3C0)(void *ctx, uint32_t a0, uint32_t a1, int32_t a2, float a3, float a4);
    int (*w_001FC520)(void *ctx, uint32_t a0);
    int (*w_00829370)(void *ctx, uint32_t a0, uint32_t a1, int32_t *result);
    int (*w_00829840)(void *ctx, uint32_t a0);
    int (*w_00825530)(void *ctx, uint32_t a0, uint32_t a1, int32_t *result);
    int (*w_00825A10)(void *ctx, uint32_t a0);
    int (*w_008259B0)(void *ctx, uint32_t a0, uint32_t a1, int32_t *result);
    int (*w_00825E90)(void *ctx, uint32_t a0);
/* END GENERATED HOOKS */
    int (*w_callback)(void *ctx, uint32_t function, uint32_t self);
} EmArea19RouteHooks;
int em_area19_route_00824690(const EmArea19RouteHooks *, uint32_t self, uint32_t sp, EmArea19RouteFault *);
int em_area19_route_00827DD0(const EmArea19RouteHooks *, uint32_t self, EmArea19RouteFault *);
/* AREA14's 0x823FC0 (sub 0 group 0x826180 [13] / [15] / [17] / [19]):
 * 0x827DD0's instructions but its two overlay callees (the probe 0x825530,
 * the spawner 0x825A10) and the missed-probe effect (0x8000002C when the
 * earlier hit record's +0x1A was 5, else 0x80000003). Same oracle. */
int em_area19_route_00823FC0(const EmArea19RouteHooks *, uint32_t self, EmArea19RouteFault *);
/* AREA20's 0x824440 (lane PORT-A20): AREA14's 0x823FC0 but its callees
 * 0x8259B0 / 0x825E90 and a missed probe's effect 0x80000067 when the
 * earlier hit record's +0x1A was 8 (test_area19_route_reference). */
int em_area19_route_00824440(const EmArea19RouteHooks *, uint32_t self, EmArea19RouteFault *);
#ifdef __cplusplus
}
#endif
#endif
