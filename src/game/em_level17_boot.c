/* Boot behaviours of the second half's placements, from the byte-matched
 * decomp C (src/func_001C2420.c, src/func_001C4A00.c). See em_level17_boot.h. */
#include "em_level17_boot_internal.h"
#define B(a) l17b_u8(o,(a))
#define SH(a) l17b_s16(o,(a))
#define C(fn,...) do { if (l17b_c_##fn(o, __VA_ARGS__)) return 0; } while (0)

/* 001C4A00(node), by the state byte +4: 0: when 001B0FD0(node) is 0,
 * 001C6380(node); 1: 001B1B70(node) and the node's +0x4C method, when the
 * scratchpad byte 0x70003B92 is 0, or when it is not and the byte
 * D_008107D8[node byte +3] differs from the signed halfword +0x56; 2, 3:
 * 001AFC10(node); other: nothing. 001C48C0 without the scratchpad test. */
static uint32_t e001C4A00(L17B *o, uint32_t self)
{
    int32_t r = 0;
    switch (B(self + 4)) {
    case 0:
        C(001B0FD0, self, &r);
        if (r == 0) C(001C6380, self);
        break;
    case 1:
        if (B(0x70003B92u) != 0) {
            const uint32_t index = B(self + 3);
            const int32_t want = SH(self + 0x56);
            if ((int32_t)B(0x8107D8u + index) == want) break;
        }
        C(001B1B70, self);
        (void)l17b_method(o, self);
        break;
    case 2:
    case 3:
        C(001AFC10, self);
        break;
    default:
        break;
    }
    return 0;
}

/* AREA21's 0x823A10 (group 0x82B500 [2]) and 0x823AB0 (group 0x82BA70 [23]):
 * AREA01's 0x8239C0 word for word but for the rate's float immediate (AREA01
 * and AREA19's 0x825030: 5.0; here 40.0 and 8.0). In state 1 (+4) the
 * scratchpad colour 0x700038B0..BC = 0x30, 0x80, 0x30, 0x80, then
 * 001F4E20(node + 0x100, 0x700038B0, rate); other states: nothing. */
static uint32_t a21_glow(L17B *o, uint32_t self, uint32_t rate)
{
    if (B(self + 4) != 1) return 0;
    l17b_w32(o, 0x700038B0u, 0x30);
    l17b_w32(o, 0x700038B4u, 0x80);
    l17b_w32(o, 0x700038B8u, 0x30);
    l17b_w32(o, 0x700038BCu, 0x80);
    C(001F4E20, self + 0x100, 0x700038B0u, rate);
    return 0;
}

int em_level17_boot_a21_00823A10(const EmLevel17BootHooks *h, uint32_t a0, EmLevel17BootFault *fault)
{
    L17B o;
    if (l17b_begin(&o, h, fault)) return -1;
    (void)a21_glow(&o, a0, 0x42200000u);   /* 40.0 */
    return l17b_end(&o);
}

int em_level17_boot_a21_00823AB0(const EmLevel17BootHooks *h, uint32_t a0, EmLevel17BootFault *fault)
{
    L17B o;
    if (l17b_begin(&o, h, fault)) return -1;
    (void)a21_glow(&o, a0, 0x41000000u);   /* 8.0 */
    return l17b_end(&o);
}

/* 001C2420(node): returns at once (an empty behaviour; the placements that
 * name it keep a node that never acts). */
int em_level17_boot_001C2420(const EmLevel17BootHooks *h, uint32_t a0, EmLevel17BootFault *fault)
{
    L17B o;
    (void)a0;
    if (l17b_begin(&o, h, fault)) return -1;
    return l17b_end(&o);
}

int em_level17_boot_001C4A00(const EmLevel17BootHooks *h, uint32_t a0, EmLevel17BootFault *fault)
{
    L17B o;
    if (l17b_begin(&o, h, fault)) return -1;
    (void)e001C4A00(&o, a0);
    return l17b_end(&o);
}
