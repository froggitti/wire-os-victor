// SHA-256. Adapted from LibTomCrypt. This code is Public Domain
#include <string.h>
#include "sha256.h"

typedef uint32_t u32;
typedef uint64_t u64;

static const uint32_t INIT_STATE[] = {
  0x6A09E667UL,
  0xBB67AE85UL,
  0x3C6EF372UL,
  0xA54FF53AUL,
  0x510E527FUL,
  0x9B05688CUL,
  0x1F83D9ABUL,
  0x5BE0CD19UL
};

static const u32 K[64] =
{
    0x428a2f98UL, 0x71374491UL, 0xb5c0fbcfUL, 0xe9b5dba5UL, 0x3956c25bUL,
    0x59f111f1UL, 0x923f82a4UL, 0xab1c5ed5UL, 0xd807aa98UL, 0x12835b01UL,
    0x243185beUL, 0x550c7dc3UL, 0x72be5d74UL, 0x80deb1feUL, 0x9bdc06a7UL,
    0xc19bf174UL, 0xe49b69c1UL, 0xefbe4786UL, 0x0fc19dc6UL, 0x240ca1ccUL,
    0x2de92c6fUL, 0x4a7484aaUL, 0x5cb0a9dcUL, 0x76f988daUL, 0x983e5152UL,
    0xa831c66dUL, 0xb00327c8UL, 0xbf597fc7UL, 0xc6e00bf3UL, 0xd5a79147UL,
    0x06ca6351UL, 0x14292967UL, 0x27b70a85UL, 0x2e1b2138UL, 0x4d2c6dfcUL,
    0x53380d13UL, 0x650a7354UL, 0x766a0abbUL, 0x81c2c92eUL, 0x92722c85UL,
    0xa2bfe8a1UL, 0xa81a664bUL, 0xc24b8b70UL, 0xc76c51a3UL, 0xd192e819UL,
    0xd6990624UL, 0xf40e3585UL, 0x106aa070UL, 0x19a4c116UL, 0x1e376c08UL,
    0x2748774cUL, 0x34b0bcb5UL, 0x391c0cb3UL, 0x4ed8aa4aUL, 0x5b9cca4fUL,
    0x682e6ff3UL, 0x748f82eeUL, 0x78a5636fUL, 0x84c87814UL, 0x8cc70208UL,
    0x90befffaUL, 0xa4506cebUL, 0xbef9a3f7UL, 0xc67178f2UL
};

static u32 min(u32 x, u32 y)
{
    return x < y ? x : y;
}

static u32 load32(const unsigned char* y)
{
    return (u32(y[0]) << 24) | (u32(y[1]) << 16) | (u32(y[2]) << 8) | (u32(y[3]) << 0);
}

static void store64(u64 x, unsigned char* y)
{
    for(int i = 0; i != 8; ++i)
        y[i] = (x >> ((7-i) * 8)) & 255;
}

static void store32(u32 x, unsigned char* y)
{
    for(int i = 0; i != 4; ++i)
        y[i] = (x >> ((3-i) * 8)) & 255;
}

static u32 Ch(u32 x, u32 y, u32 z)  { return z ^ (x & (y ^ z)); }
static u32 Maj(u32 x, u32 y, u32 z) { return ((x | y) & z) | (x & y); }
static u32 Rot(u32 x, u32 n)        { return (x >> (n & 31)) | (x << (32 - (n & 31))); }
static u32 Sh(u32 x, u32 n)         { return x >> n; }
static u32 Sigma0(u32 x)            { return Rot(x, 2) ^ Rot(x, 13) ^ Rot(x, 22); }
static u32 Sigma1(u32 x)            { return Rot(x, 6) ^ Rot(x, 11) ^ Rot(x, 25); }
static u32 Gamma0(u32 x)            { return Rot(x, 7) ^ Rot(x, 18) ^ Sh(x, 3); }
static u32 Gamma1(u32 x)            { return Rot(x, 17) ^ Rot(x, 19) ^ Sh(x, 10); }

static void sha_compress(sha256_state& md, const unsigned char* buf)
{
    u32 S[8], W[64], t0, t1, t;

    // Copy state into S
    for(int i = 0; i < 8; i++)
        S[i] = md.state[i];

    // Copy the state into 512-bits into W[0..15]
    for(int i = 0; i < 16; i++)
        W[i] = load32(buf + (4*i));

    // Fill W[16..63]
    for(int i = 16; i < 64; i++)
        W[i] = Gamma1(W[i - 2]) + W[i - 7] + Gamma0(W[i - 15]) + W[i - 16];

    // Compress
    for(int i = 0; i < 64; ++i)
    {
        t0 = S[7] + Sigma1(S[4]) + Ch(S[4],S[5],S[6]) + K[i] + W[i];
        t1 = Sigma0(S[0]) + Maj(S[0],S[1],S[2]);
        S[3] += t0;
        S[7]  = t0 + t1;

        t = S[7]; S[7] = S[6]; S[6] = S[5]; S[5] = S[4];
        S[4] = S[3]; S[3] = S[2]; S[2] = S[1]; S[1] = S[0]; S[0] = t;
    }

    // Feedback
    for(int i = 0; i < 8; i++)
        md.state[i] = md.state[i] + S[i];
}

// Public interface
void sha_init(sha256_state& md)
{
    md.curlen = 0;
    md.length = 0;
  
    memcpy(md.state, INIT_STATE, sizeof(INIT_STATE));
}

void sha_process(sha256_state& md, const void* src, u32 inlen)
{
    const u32 block_size = sizeof( (sha256_state){0}.buf ); //sha256_state::buf);
    const unsigned char* in = (const unsigned char*) src;

    while(inlen > 0)
    {
        if(md.curlen == 0 && inlen >= block_size)
        {
            sha_compress(md, in);
            md.length += block_size * 8;
            in        += block_size;
            inlen     -= block_size;
        }
        else
        {
            u32 n = min(inlen, (block_size - md.curlen));
            memcpy(md.buf + md.curlen, in, n);
            md.curlen += n;
            in        += n;
            inlen     -= n;

            if(md.curlen == block_size)
            {
                sha_compress(md, md.buf);
                md.length += 8*block_size;
                md.curlen = 0;
            }
        }
    }
}

void sha_done(sha256_state& md, void* out)
{
    // Increase the length of the message
    md.length += md.curlen * 8;

    // Append the '1' bit
    md.buf[md.curlen++] = static_cast<unsigned char>(0x80);

    // If the length is currently above 56 bytes we append zeros then compress.
    // Then we can fall back to padding zeros and length encoding like normal.
    if(md.curlen > 56)
    {
        while(md.curlen < 64)
            md.buf[md.curlen++] = 0;
        sha_compress(md, md.buf);
        md.curlen = 0;
    }

    // Pad upto 56 bytes of zeroes
    while(md.curlen < 56)
        md.buf[md.curlen++] = 0;

    // Store length
    store64(md.length, md.buf+56);
    sha_compress(md, md.buf);

    // Copy output
    for(int i = 0; i < 8; i++)
        store32(md.state[i], static_cast<unsigned char*>(out)+(4*i));
}
