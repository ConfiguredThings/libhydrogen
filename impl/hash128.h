#define HYDRO_HASH_SIP_CROUNDS 2
#define HYDRO_HASH_SIP_DROUNDS 4
#define HYDRO_HASH_SIPROUND                \
    do {                                   \
        state->v0 += state->v1;            \
        state->v1 = ROTL64(state->v1, 13); \
        state->v1 ^= state->v0;            \
        state->v0 = ROTL64(state->v0, 32); \
        state->v2 += state->v3;            \
        state->v3 = ROTL64(state->v3, 16); \
        state->v3 ^= state->v2;            \
        state->v0 += state->v3;            \
        state->v3 = ROTL64(state->v3, 21); \
        state->v3 ^= state->v0;            \
        state->v2 += state->v1;            \
        state->v1 = ROTL64(state->v1, 17); \
        state->v1 ^= state->v2;            \
        state->v2 = ROTL64(state->v2, 32); \
    } while (0)

static void
hydro_hash128_hashblock(hydro_hash128_state *state, const uint64_t mb)
{
    int i;

    state->v3 ^= mb;
    for (i = 0; i < HYDRO_HASH_SIP_CROUNDS; i++) {
        HYDRO_HASH_SIPROUND;
    }
    state->v0 ^= mb;
}

int
hydro_hash128_init(hydro_hash128_state *state,
                   const char           ctx[hydro_hash128_CONTEXTBYTES],
                   const uint8_t        key[hydro_hash128_KEYBYTES])
{
    state->v0 = 0x736f6d6570736575ULL;
    state->v1 = 0x646f72616e646f83ULL;
    state->v2 = 0x6c7967656e657261ULL;
    state->v3 = 0x7465646279746573ULL;
    if (key != NULL) {
        const uint64_t k0 = LOAD64_LE(key);
        const uint64_t k1 = LOAD64_LE(key + 8);
        state->v0 ^= k0;
        state->v1 ^= k1;
        state->v2 ^= k0;
        state->v3 ^= k1;
    }
    state->buf_off = 0;
    state->b       = 0;

    COMPILER_ASSERT(hydro_hash128_CONTEXTBYTES == 8);
    hydro_hash128_hashblock(state, LOAD64_LE((const uint8_t *) ctx));

    return 0;
}

int
hydro_hash128_update(hydro_hash128_state *state, const void *in_, size_t in_len)
{
    const uint8_t *in = (const uint8_t *) in_;
    size_t         left;
    size_t         ps;
    size_t         i;

    state->b += (uint8_t) in_len;
    while (in_len > 0) {
        left = 8 - state->buf_off;
        if ((ps = in_len) > left) {
            ps = left;
        }
        for (i = 0; i < ps; i++) {
            state->buf[state->buf_off + i] = in[i];
        }
        state->buf_off += (uint8_t) ps;
        if (state->buf_off == 8) {
            hydro_hash128_hashblock(state, LOAD64_LE(state->buf));
            state->buf_off = 0;
        }
        in += ps;
        in_len -= ps;
    }
    return 0;
}

int
hydro_hash128_final(hydro_hash128_state *state,
                    uint8_t              out[hydro_hash128_BYTES])
{
    uint64_t mb;
    int      i;

    mb = (LOAD64_LE(state->buf) & ~(~0ULL << (state->buf_off * 8))) |
         ((uint64_t) state->b << 56);
    hydro_hash128_hashblock(state, mb);
    state->v2 ^= 0xee;
    for (i = 0; i < HYDRO_HASH_SIP_DROUNDS; i++) {
        HYDRO_HASH_SIPROUND;
    }
    mb = state->v0 ^ state->v1 ^ state->v2 ^ state->v3;
    STORE64_LE(out, mb);
    state->v1 ^= 0xdd;
    for (i = 0; i < HYDRO_HASH_SIP_DROUNDS; i++) {
        HYDRO_HASH_SIPROUND;
    }
    mb = state->v0 ^ state->v1 ^ state->v2 ^ state->v3;
    STORE64_LE(out + 8, mb);

    return 0;
}

int
hydro_hash128_hash(uint8_t out[hydro_hash128_BYTES], const void *in_,
                   size_t in_len, const char ctx[hydro_hash128_CONTEXTBYTES],
                   const uint8_t key[hydro_hash128_KEYBYTES])
{
    hydro_hash128_state st;

    hydro_hash128_init(&st, ctx, key);
    hydro_hash128_update(&st, in_, in_len);

    return hydro_hash128_final(&st, out);
}

void
hydro_hash128_keygen(uint8_t key[hydro_hash128_KEYBYTES])
{
    randombytes_buf(key, hydro_hash128_KEYBYTES);
}
