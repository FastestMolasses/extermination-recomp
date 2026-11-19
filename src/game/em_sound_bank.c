/* The sound-bank upload 001FB370 bound over the EE sound library and the
 * IOP backend. See em_sound_bank.h and docs/IOP_STREAM.md "The sound-bank
 * transfer". */
#include "game/em_sound_bank.h"

#include <string.h>

/* The state the boot's and the title's bank uploads leave, which the port
 * does not run (001AB7E0 step 1's common bank: handles 0..2 in bucket 1;
 * the title module's bank: handle 3 in bucket 3). Every value is the title
 * capture's (the fork state slot01_title, the title before New Game; the
 * v2.6.3 slot 01 holds the same values) and holds until the New Game area
 * load: route capture 00 shows the same entries plus the area bank's. They
 * are addresses and counters, not disc data. The one value the two
 * recordings disagree on is D_002821A0, the kernel's SIF DMA id (the
 * fork's boot 0x527C1602, v2.6.3's 0x530D1702): the id comes from the
 * boot's DMA history, which the port does not model, and 001FB910 only
 * tests it against 0; the seed follows the fork, the default reference.
 * tools/test_sound_bank_reference.py checks each against the capture. */
static const struct {
    uint32_t handles[4][3];      /* D_0027C6C0[0..3]: {in use, SShd header, SPU >> 3} */
    int32_t counts[8];           /* D_00281D30 */
    int32_t bucket1[3];          /* D_00281D50 bucket 1 (words 20..22) */
    int32_t bucket3;             /* D_00281D50 bucket 3 (word 60) */
    uint8_t volume[6];           /* D_002819C0[0..5] */
    int32_t count;               /* D_0027F740 + 0x48 = D_002817C0 + 0x1C0 */
    int8_t d282150, d282151, d282159, d28215C;
    int32_t d282190, d282194, d282198;
    uint32_t d28219C;
    int32_t d2821A0;
    uint32_t d2821A4;
    int32_t d275B18, d275B1C;
} EM_SOUND_BANK_SEEDS = {
    {{1u, 0x00B2C860u, 0x2A08u}, {1u, 0x00B2F7D0u, 0xC8A0u}, {1u, 0x00B30CB0u, 0x1AA40u},
     {1u, 0x01800030u, 0x24400u}},
    {0, 3, 0, 1, 0, 0, 0, 0},
    {0, 1, 2},
    3,
    {0x64, 1, 0x64, 1, 0x64, 1},
    5,
    0, 0, 1, 1,
    3, 0xDE800, 0x12C770,
    0x0180AD00u,
    0x527C1602,
    0x01800030u,
    0x4D, 0,
};

static int bank_fail(EmSoundBank *b, uint32_t address, int32_t code)
{
    if (b->fault_code == 0) {
        b->fault_address = address;
        b->fault_code = code;
    }
    return -1;
}

static int lib_failed(EmSoundBank *b)
{
    return bank_fail(b, b->lib_fault.address, b->lib_fault.code);
}

/* ---- the library's workers ---- */

static int lw_001157F0(void *ctx, int32_t cmd, int32_t a1, int32_t a2, int32_t a3)
{
    EmSoundBank *b = ctx;
    (void)em_iop_stream_001157F0(b->iop, cmd, a1, a2, a3); /* a full queue drops it */
    if (em_iop_stream_fault(b->iop)->code)
        return -1;
    if (cmd == 0x20)
        b->uploads++;
    return 0;
}

static int lw_ack(void *ctx, int32_t *value)
{
    EmSoundBank *b = ctx;
    *value = em_iop_stream_ee_status(b->iop)[112];
    return 0;
}

static const uint8_t *file_at(const EmSoundBank *b, uint32_t address, uint32_t size)
{
    const EmSlgBankFile *f = b->file;
    if (!f || !f->bytes || address < f->address || address - f->address > f->size ||
        size > f->size - (address - f->address))
        return NULL;
    return f->bytes + (address - f->address);
}

static int lw_word(void *ctx, uint32_t address, uint32_t *value)
{
    const uint8_t *p = file_at(ctx, address, 4);
    if (!p)
        return -1;
    *value = (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
    return 0;
}

static int lw_voice(void *ctx, int32_t voice, uint16_t *state, uint16_t *handle)
{
    EmSoundBank *b = ctx;
    if (!b->voice)
        return -1;
    return b->voice(b->voice_ctx, voice, state, handle);
}

static EmEeSoundLibWorkers lib_workers(EmSoundBank *b)
{
    return (EmEeSoundLibWorkers){b, lw_001157F0, lw_ack, lw_word, lw_voice};
}

/* ---- 001FB3E0's workers ---- */

static int w_001195A8(void *ctx, int32_t handle)
{
    EmSoundBank *b = ctx;
    EmEeSoundLibWorkers w = lib_workers(b);
    int32_t ret;
    return em_ee_sound_lib_001195A8(&b->lib, &w, (uint32_t)handle, &ret, &b->lib_fault) < 0
               ? lib_failed(b) : 0;
}

static int w_0010F8F8(void *ctx, int32_t size, int32_t *ret)
{
    EmSoundBank *b = ctx;
    uint32_t block;
    if (em_iop_stream_0010F8F8(b->iop, size, &block) < 0)
        return bank_fail(b, 0x0010F8F8u, 2);
    *ret = (int32_t)block;
    return 0;
}

/* The running load's next recorded answer for a query of `kind` (0 the
 * status query, 1 the acknowledgement), or 0 when none is served (no entry,
 * or the entry broke here: a query of another kind or no answer left). */
static char replay_answer(EmSoundBank *b, int kind)
{
    if (b->replay_active < 0)
        return 0;
    const char *a = b->replay[b->replay_active].answers;
    char c = a[b->replay_cursor];
    int is_stat = c == 'p' || c == 'd';
    if (!c || is_stat != (kind == 0)) {
        b->replay_active = -1;
        b->replay_stats.broken += 1;
        return 0;
    }
    b->replay_cursor += 1;
    b->replay_stats.answers += 1;
    return c;
}

static int w_00119450(void *ctx, int32_t a0, int32_t *ret)
{
    EmSoundBank *b = ctx;
    EmEeSoundLibWorkers w = lib_workers(b);
    if (em_ee_sound_lib_00119450(&b->lib, &w, a0, ret, &b->lib_fault) < 0)
        return lib_failed(b);
    char c = a0 == 0 ? replay_answer(b, 1) : 0;
    if (c) {
        int32_t r = c == '1' ? 1 : c == '0' ? 0 : -1;
        b->replay_stats.differed += r != *ret;
        *ret = r;
    }
    return 0;
}

static int w_001194B8(void *ctx, int32_t a0, uint32_t a1, int32_t a2, int32_t *ret)
{
    EmSoundBank *b = ctx;
    EmEeSoundLibWorkers w = lib_workers(b);
    return em_ee_sound_lib_001194B8(&b->lib, &w, a0, a1, a2, ret, &b->lib_fault) < 0
               ? lib_failed(b) : 0;
}

static int w_001199F0(void *ctx, int32_t handle, int32_t a1)
{
    EmSoundBank *b = ctx;
    int32_t ret;
    return em_ee_sound_lib_001199F0(&b->lib, handle, a1, &ret, &b->lib_fault) < 0 ? lib_failed(b)
                                                                                   : 0;
}

static int w_0010F968(void *ctx, int32_t block)
{
    EmSoundBank *b = ctx;
    return em_iop_stream_0010F968(b->iop, (uint32_t)block) < 0 ? bank_fail(b, 0x0010F968u, 2) : 0;
}

/* sceSifSetDChain (syscall 0x78) and FlushCache(0) (syscall 0x64): the SIF0
 * receive chain and the EE data cache are not host state. */
static int w_0010BC00(void *ctx)
{
    (void)ctx;
    return 0;
}

static int w_0010BAA0(void *ctx, int32_t a0)
{
    (void)ctx;
    (void)a0;
    return 0;
}

static int w_0010BBE0(void *ctx, const uint32_t desc[4], int32_t count, int32_t *ret)
{
    EmSoundBank *b = ctx;
    const uint8_t *src;
    if (count != 1 || desc[3] != 0 || !(src = file_at(b, desc[0], desc[2])))
        return bank_fail(b, 0x0010BBE0u, 4);
    return em_iop_stream_sif_set_dma(b->iop, src, desc[1], desc[2], ret) < 0
               ? bank_fail(b, 0x0010BBE0u, 2) : 0;
}

static int w_0010BBC0(void *ctx, int32_t id, int32_t *ret)
{
    EmSoundBank *b = ctx;
    if (em_iop_stream_sif_dma_stat(b->iop, id, ret) < 0)
        return bank_fail(b, 0x0010BBC0u, 2);
    char c = replay_answer(b, 0);
    if (c) {
        int32_t r = c == 'p' ? 0 : -1;   /* the fork's kernel: 0 pending, negative done */
        b->replay_stats.differed += (r >= 0) != (*ret >= 0);
        *ret = r;
    }
    return 0;
}

/* ---- the binding ---- */

int em_sound_bank_init(EmSoundBank *b, EmIopStream *iop, const int32_t base[5])
{
    if (!b || !iop || !base)
        return -1;
    memset(b, 0, sizeof *b);
    b->iop = iop;
    EmSlgBankLoad *s = &b->load;
    s->d282150 = EM_SOUND_BANK_SEEDS.d282150;
    s->d282151 = EM_SOUND_BANK_SEEDS.d282151;
    s->d282159 = EM_SOUND_BANK_SEEDS.d282159;
    s->d28215C = EM_SOUND_BANK_SEEDS.d28215C;
    s->d282190 = EM_SOUND_BANK_SEEDS.d282190;
    s->d282194 = EM_SOUND_BANK_SEEDS.d282194;
    s->d282198 = EM_SOUND_BANK_SEEDS.d282198;
    s->d28219C = EM_SOUND_BANK_SEEDS.d28219C;
    s->d2821A0 = EM_SOUND_BANK_SEEDS.d2821A0;
    s->d2821A4 = EM_SOUND_BANK_SEEDS.d2821A4;
    s->d275B18 = EM_SOUND_BANK_SEEDS.d275B18;
    s->d275B1C = EM_SOUND_BANK_SEEDS.d275B1C;
    memcpy(s->d281D30, EM_SOUND_BANK_SEEDS.counts, sizeof s->d281D30);
    memcpy(&s->d281D50[20], EM_SOUND_BANK_SEEDS.bucket1, sizeof EM_SOUND_BANK_SEEDS.bucket1);
    s->d281D50[60] = EM_SOUND_BANK_SEEDS.bucket3;
    memcpy(s->base, base, sizeof s->base);
    b->replay_active = -1;
    memcpy(b->lib.d27C6C0, EM_SOUND_BANK_SEEDS.handles, sizeof EM_SOUND_BANK_SEEDS.handles);
    memcpy(b->lib.d2819C0, EM_SOUND_BANK_SEEDS.volume, sizeof EM_SOUND_BANK_SEEDS.volume);
    b->lib.d27F788 = EM_SOUND_BANK_SEEDS.count;
    em_iop_stream_seed_transfer_count(iop, (uint32_t)EM_SOUND_BANK_SEEDS.count);
    return 0;
}

void em_sound_bank_set_voice(EmSoundBank *b, EmSoundBankVoice voice, void *ctx)
{
    if (!b)
        return;
    b->voice = voice;
    b->voice_ctx = ctx;
}

/* A load begins (001FB370 enters with D_00282150 0): the nearest unserved
 * entry with this bank's record count (the file's word +0x0C, which
 * 001FB8A4 counts D_00282159 to; the entry's 'd' answers, one per record's
 * finished transfer) within the window of the recording's counter now. The
 * bank's EE address (and so its end) is the port's own, not the original's. */
static void replay_begin(EmSoundBank *b, const EmSlgBankFile *f)
{
    uint32_t c;
    b->replay_active = -1;
    b->replay_cursor = 0;
    b->replay_calls = 0;
    if (!b->replay_counter(b->replay_ctx, &c))
        return;
    if (f->size < 0x14)
        return;
    const uint8_t *p = f->bytes + 0x0C;
    const uint32_t records = (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 |
                             (uint32_t)p[3] << 24;
    int32_t best = -1;
    uint32_t best_d = 0;
    for (uint32_t i = 0; i < b->replay_n; ++i) {
        const EmSoundBankReplayLoad *e = &b->replay[i];
        uint32_t d = e->counter > c ? e->counter - c : c - e->counter;
        uint32_t done = 0;
        for (const char *a = e->answers; *a; ++a)
            done += *a == 'd';
        if (e->served || done != records || d > EM_SOUND_BANK_REPLAY_WINDOW)
            continue;
        if (best < 0 || d < best_d) {
            best = (int32_t)i;
            best_d = d;
        }
    }
    if (best < 0) {
        if (b->replay_stats.missed < 4) {
            b->replay_stats.first_missed[b->replay_stats.missed][0] = c;
            b->replay_stats.first_missed[b->replay_stats.missed][1] = records;
        }
        b->replay_stats.missed += 1;
        return;
    }
    b->replay[best].served = 1;
    b->replay_active = best;
    b->replay_stats.loads += 1;
    b->replay_stats.near += best_d != 0;
}

int em_sound_bank_001FB370(EmSoundBank *b, uint32_t address, const uint8_t *bytes, uint32_t size,
                           uint32_t *result)
{
    if (!b || !result)
        return -1;
    if (b->fault_code)
        return -1;
    if (!b->iop || !bytes)
        return bank_fail(b, 0x001FB370u, 1);
    const EmSlgBankFile file = {address, bytes, size};
    const EmSlgBankWorkers w = {b,          w_001195A8, w_0010F8F8, w_00119450,
                                w_001194B8, w_001199F0, w_0010F968, w_0010BC00,
                                w_0010BAA0, w_0010BBE0, w_0010BBC0};
    if (b->replay_n && b->load.d282150 == 0)
        replay_begin(b, &file);
    b->file = &file;
    b->calls++;
    b->replay_calls++;
    int rc = em_slg_001FB370(&w, &b->load, &file, result);
    b->file = NULL;
    if (rc < 0)
        return bank_fail(b, 0x001FB370u, 2);
    if (*result && b->replay_active >= 0) {   /* the load's last call */
        const EmSoundBankReplayLoad *e = &b->replay[b->replay_active];
        if (e->answers[b->replay_cursor])
            b->replay_stats.broken += 1;          /* answers left over */
        b->replay_stats.calls_equal += b->replay_calls == e->calls;
        b->replay_active = -1;
    }
    return 0;
}

int em_sound_bank_set_replay(EmSoundBank *b, EmSoundBankReplayLoad *loads, uint32_t n,
                             EmSoundBankReplayCounter counter, void *ctx)
{
    if (!b || (n && (!loads || !counter)))
        return -1;
    for (uint32_t i = 0; i < n; ++i) {
        if (!loads[i].answers || (i && loads[i].counter < loads[i - 1].counter))
            return -1;
        for (const char *c = loads[i].answers; *c; ++c)
            if (!strchr("pd01n", *c))
                return -1;
        loads[i].served = 0;
    }
    b->replay = loads;
    b->replay_n = n;
    b->replay_counter = counter;
    b->replay_ctx = ctx;
    b->replay_active = -1;
    memset(&b->replay_stats, 0, sizeof b->replay_stats);
    return 0;
}

void em_sound_bank_replay_stats(const EmSoundBank *b, EmSoundBankReplayStats *stats)
{
    if (!stats)
        return;
    memset(stats, 0, sizeof *stats);
    if (b)
        *stats = b->replay_stats;
}

int em_sound_bank_failed(const EmSoundBank *b, uint32_t *address, int32_t *code)
{
    if (!b || !b->fault_code)
        return 0;
    if (address)
        *address = b->fault_address;
    if (code)
        *code = b->fault_code;
    return 1;
}
