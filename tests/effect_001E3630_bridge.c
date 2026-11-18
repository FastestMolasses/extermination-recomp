/* The test bridge of tools/test_effect_001E3630_reference.py: the module's
 * memory is one RAM image and one scratchpad image the test owns, and every
 * worker forwards to one Python callback (fn, integer arguments, float
 * argument bits) that answers v0 / f0. b_null names a worker to leave NULL
 * (the fail-stop check). Test code only; holds no original data. */
#include <stdint.h>
#include <string.h>

#include "game/em_effect_001E3630.h"

typedef int (*PyCall)(uint32_t fn, uint32_t na, const uint64_t *a, uint32_t nf, const uint32_t *f, uint64_t *v0,
                      uint32_t *f0);

static uint8_t *RAM, *SPAD;
static PyCall CB;
static uint32_t NULLED;
#define RAM_SIZE UINT32_C(0x02000000)

static uint8_t *bytes(void *c, uint32_t a, uint32_t n, int w)
{
    (void)c;
    (void)w;
    if (a < RAM_SIZE && n <= RAM_SIZE - a) return RAM + a;
    if (a >= 0x70000000u && a < 0x70004000u && n <= 0x70004000u - a) return SPAD + (a - 0x70000000u);
    return NULL;
}

static int go(uint32_t fn, uint32_t na, const uint64_t *a, uint32_t nf, const uint32_t *f, uint64_t *v0,
              uint32_t *f0)
{
    uint64_t v = 0;
    uint32_t r = 0;
    int rc = CB(fn, na, a, nf, f, &v, &r);
    if (v0) *v0 = v;
    if (f0) *f0 = r;
    return rc;
}

#define A(x) ((uint64_t)(int64_t)(int32_t)(x))
static int w2(uint32_t fn, uint32_t a0, uint32_t a1)
{
    const uint64_t a[2] = {A(a0), A(a1)};
    return go(fn, 2, a, 0, NULL, NULL, NULL);
}
static int w3(uint32_t fn, uint32_t a0, uint32_t a1, uint32_t a2)
{
    const uint64_t a[3] = {A(a0), A(a1), A(a2)};
    return go(fn, 3, a, 0, NULL, NULL, NULL);
}
static int w_00102948(void *c, uint32_t a0, uint32_t a1) { (void)c; return w2(0x102948u, a0, a1); }
static int w_00102958(void *c, uint32_t a0, uint32_t a1) { (void)c; return w2(0x102958u, a0, a1); }
static int w_001026A0(void *c, uint32_t a0, uint32_t a1, uint32_t a2) { (void)c; return w3(0x1026A0u, a0, a1, a2); }
static int w_001028D0(void *c, uint32_t a0, uint32_t a1, uint32_t a2) { (void)c; return w3(0x1028D0u, a0, a1, a2); }
static int w_00102918(void *c, uint32_t a0, uint32_t a1, uint32_t a2) { (void)c; return w3(0x102918u, a0, a1, a2); }
static int w_0011E748(void *c, uint32_t f12, uint32_t *f0)
{
    (void)c;
    return go(0x11E748u, 0, NULL, 1, &f12, NULL, f0);
}
static int w_00122BB8(void *c, int32_t *v0)
{
    uint64_t v = 0;
    (void)c;
    int rc = go(0x122BB8u, 0, NULL, 0, NULL, &v, NULL);
    *v0 = (int32_t)v;
    return rc;
}
static int w_0019A570(void *c, uint32_t a0, uint32_t a1, int32_t a2, int32_t a3, int32_t *v0)
{
    const uint64_t a[4] = {A(a0), A(a1), A(a2), A(a3)};
    uint64_t v = 0;
    (void)c;
    int rc = go(0x19A570u, 4, a, 0, NULL, &v, NULL);
    *v0 = (int32_t)v;
    return rc;
}
static int w_0019B6C0(void *c, uint32_t a0, uint32_t a1, int32_t *v0)
{
    const uint64_t a[2] = {A(a0), A(a1)};
    uint64_t v = 0;
    (void)c;
    int rc = go(0x19B6C0u, 2, a, 0, NULL, &v, NULL);
    *v0 = (int32_t)v;
    return rc;
}
static int w1(uint32_t fn, uint32_t a0)
{
    const uint64_t a[1] = {A(a0)};
    return go(fn, 1, a, 0, NULL, NULL, NULL);
}
static int w_001AFC10(void *c, uint32_t a0) { (void)c; return w1(0x1AFC10u, a0); }
static int w_001B17A0(void *c, uint32_t a0) { (void)c; return w1(0x1B17A0u, a0); }
static int w_001CCF70(void *c, uint32_t a0, int32_t *v0)
{
    const uint64_t a[1] = {A(a0)};
    uint64_t v = 0;
    (void)c;
    int rc = go(0x1CCF70u, 1, a, 0, NULL, &v, NULL);
    *v0 = (int32_t)v;
    return rc;
}
static int w_001CD390(void *c, uint32_t a0, uint32_t a1) { (void)c; return w2(0x1CD390u, a0, a1); }
static int w_001CFA60(void *c, uint32_t a0, uint32_t a1, uint32_t f12, uint32_t f13)
{
    const uint64_t a[2] = {A(a0), A(a1)};
    const uint32_t f[2] = {f12, f13};
    (void)c;
    return go(0x1CFA60u, 2, a, 2, f, NULL, NULL);
}
static int w_001CFB50(void *c, uint32_t a0, int32_t a1, uint32_t a2, const uint32_t f[5])
{
    const uint64_t a[3] = {A(a0), A(a1), A(a2)};
    (void)c;
    return go(0x1CFB50u, 3, a, 5, f, NULL, NULL);
}
static int w_001CFBE0(void *c, int32_t a0, int32_t a1, uint32_t a2, uint32_t a3, int32_t t0)
{
    const uint64_t a[5] = {A(a0), A(a1), A(a2), A(a3), A(t0)};
    (void)c;
    return go(0x1CFBE0u, 5, a, 0, NULL, NULL, NULL);
}
static int w_001EFD20(void *c, uint32_t a0, uint32_t a1) { (void)c; return w2(0x1EFD20u, a0, a1); }
static int w_001F0190(void *c, uint32_t f12, uint32_t f13)
{
    const uint32_t f[2] = {f12, f13};
    (void)c;
    return go(0x1F0190u, 0, NULL, 2, f, NULL, NULL);
}
static int w_001F0290(void *c) { (void)c; return go(0x1F0290u, 0, NULL, 0, NULL, NULL, NULL); }
static int w_001F0460(void *c, int32_t a0, uint32_t a1) { (void)c; return w2(0x1F0460u, (uint32_t)a0, a1); }

static void hooks(EmEffect001E3630Hooks *h)
{
    memset(h, 0, sizeof *h);
    h->bytes = bytes;
#define SET(name, fn) h->name = NULLED == (fn) ? NULL : name
    SET(w_00102948, 0x102948u); SET(w_00102958, 0x102958u); SET(w_001026A0, 0x1026A0u);
    SET(w_001028D0, 0x1028D0u); SET(w_00102918, 0x102918u); SET(w_0011E748, 0x11E748u);
    SET(w_00122BB8, 0x122BB8u); SET(w_0019A570, 0x19A570u); SET(w_0019B6C0, 0x19B6C0u);
    SET(w_001AFC10, 0x1AFC10u); SET(w_001B17A0, 0x1B17A0u); SET(w_001CCF70, 0x1CCF70u);
    SET(w_001CD390, 0x1CD390u); SET(w_001CFA60, 0x1CFA60u); SET(w_001CFB50, 0x1CFB50u);
    SET(w_001CFBE0, 0x1CFBE0u); SET(w_001EFD20, 0x1EFD20u); SET(w_001F0190, 0x1F0190u);
    SET(w_001F0290, 0x1F0290u); SET(w_001F0460, 0x1F0460u);
#undef SET
}

void b_null(uint32_t fn) { NULLED = fn; }

int b_run(uint8_t *ram, uint8_t *spad, PyCall cb, uint32_t node, uint32_t sp, uint32_t *fault)
{
    EmEffect001E3630Hooks h;
    EmEffect001E3630Fault f = {0, 0};
    RAM = ram;
    SPAD = spad;
    CB = cb;
    hooks(&h);
    int rc = em_effect_001E3630(&h, node, sp, &f);
    fault[0] = (uint32_t)f.code;
    fault[1] = f.address;
    return rc;
}

int b_ece70(uint8_t *ram, uint8_t *spad, PyCall cb, uint32_t a0, int32_t a1, uint32_t *fault)
{
    EmEffect001E3630Hooks h;
    EmEffect001E3630Fault f = {0, 0};
    RAM = ram;
    SPAD = spad;
    CB = cb;
    hooks(&h);
    int rc = em_effect_001ECE70(&h, a0, a1, &f);
    fault[0] = (uint32_t)f.code;
    fault[1] = f.address;
    return rc;
}

int b_contact(void) { return em_effect_001E3620(); }
