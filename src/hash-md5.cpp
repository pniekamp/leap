//
// hash-md5.cpp - md5
//

// md5 written by Alexander Peslyak in 2001

#include "leap/hash.h"
#include <cstring>
#include <cstdio>

using namespace std;

struct MD5_CTX
{
  uint32_t lo, hi;
  uint32_t a, b, c, d;
  uint8_t buffer[64];
  uint32_t block[16];
  uint8_t result[16];
};

namespace leap { namespace crypto
{

  //|///////////////////// md5::Constructor /////////////////////////////////
  md5::md5()
    : state(new MD5_CTX)
  {
  }


  //|///////////////////// md5::Constructor /////////////////////////////////
  md5::md5(md5 &&that)
    : state(nullptr)
  {
    swap(state, that.state);
  }


  //|///////////////////// md5::Destructor //////////////////////////////////
  md5::~md5()
  {
    delete static_cast<MD5_CTX*>(state);
  }


  //|///////////////////// md5::hex /////////////////////////////////////////
  string md5::hex() const
  {
    char digest[size()*2];

    for(size_t i = 0; i < size(); i++)
      sprintf(digest+i*2, "%02x", data()[i]);

    return string(digest, sizeof(digest));
  }


  //|///////////////////// md5::data ////////////////////////////////////////
  uint8_t const *md5::data() const
  {
    return static_cast<MD5_CTX*>(state)->result;
  }


  /*
   * The basic MD5 functions.
   *
   * F and G are optimized compared to their RFC 1321 definitions for
   * architectures that lack an AND-NOT instruction, just like in Colin Plumb's
   * implementation.
   */
  #define F(x, y, z) ((z) ^ ((x) & ((y) ^ (z))))
  #define G(x, y, z) ((y) ^ ((z) & ((x) ^ (y))))
  #define H(x, y, z) (((x) ^ (y)) ^ (z))
  #define H2(x, y, z) ((x) ^ ((y) ^ (z)))
  #define I(x, y, z) ((y) ^ ((x) | ~(z)))

  /*
   * The MD5 transformation for all four rounds.
   */
  #define STEP(f, a, b, c, d, x, t, s) \
    (a) += f((b), (c), (d)) + (x) + (t); \
    (a) = (((a) << (s)) | (((a) & 0xffffffff) >> (32 - (s)))); \
    (a) += (b);

  /*
   * SET reads 4 input bytes in little-endian byte order and stores them
   * in a properly aligned word in host byte order.
   *
   * The check for little-endian architectures that tolerate unaligned
   * memory accesses is just an optimization.  Nothing will break if it
   * doesn't work.
   */
  #if defined(__i386__) || defined(__x86_64__) || defined(__vax__)
  #define SET(n) \
    (*(uint32_t *)&ptr[(n) * 4])
  #define GET(n) \
    SET(n)
  #else
  #define SET(n) \
    (ctx->block[(n)] = \
    (uint32_t)ptr[(n) * 4] | \
    ((uint32_t)ptr[(n) * 4 + 1] << 8) | \
    ((uint32_t)ptr[(n) * 4 + 2] << 16) | \
    ((uint32_t)ptr[(n) * 4 + 3] << 24))
  #define GET(n) \
    (ctx->block[(n)])
  #endif

  /*
   * This processes one or more 64-byte data blocks, but does NOT update
   * the bit counters.  There are no alignment requirements.
   */
  static const void *body(MD5_CTX *ctx, const void *data, unsigned long size)
  {
    const uint8_t *ptr;
    uint32_t a, b, c, d;
    uint32_t saved_a, saved_b, saved_c, saved_d;

    ptr = (const uint8_t *)data;

    a = ctx->a;
    b = ctx->b;
    c = ctx->c;
    d = ctx->d;

    do {
      saved_a = a;
      saved_b = b;
      saved_c = c;
      saved_d = d;

  /* Round 1 */
      STEP(F, a, b, c, d, SET(0), 0xd76aa478, 7)
      STEP(F, d, a, b, c, SET(1), 0xe8c7b756, 12)
      STEP(F, c, d, a, b, SET(2), 0x242070db, 17)
      STEP(F, b, c, d, a, SET(3), 0xc1bdceee, 22)
      STEP(F, a, b, c, d, SET(4), 0xf57c0faf, 7)
      STEP(F, d, a, b, c, SET(5), 0x4787c62a, 12)
      STEP(F, c, d, a, b, SET(6), 0xa8304613, 17)
      STEP(F, b, c, d, a, SET(7), 0xfd469501, 22)
      STEP(F, a, b, c, d, SET(8), 0x698098d8, 7)
      STEP(F, d, a, b, c, SET(9), 0x8b44f7af, 12)
      STEP(F, c, d, a, b, SET(10), 0xffff5bb1, 17)
      STEP(F, b, c, d, a, SET(11), 0x895cd7be, 22)
      STEP(F, a, b, c, d, SET(12), 0x6b901122, 7)
      STEP(F, d, a, b, c, SET(13), 0xfd987193, 12)
      STEP(F, c, d, a, b, SET(14), 0xa679438e, 17)
      STEP(F, b, c, d, a, SET(15), 0x49b40821, 22)

  /* Round 2 */
      STEP(G, a, b, c, d, GET(1), 0xf61e2562, 5)
      STEP(G, d, a, b, c, GET(6), 0xc040b340, 9)
      STEP(G, c, d, a, b, GET(11), 0x265e5a51, 14)
      STEP(G, b, c, d, a, GET(0), 0xe9b6c7aa, 20)
      STEP(G, a, b, c, d, GET(5), 0xd62f105d, 5)
      STEP(G, d, a, b, c, GET(10), 0x02441453, 9)
      STEP(G, c, d, a, b, GET(15), 0xd8a1e681, 14)
      STEP(G, b, c, d, a, GET(4), 0xe7d3fbc8, 20)
      STEP(G, a, b, c, d, GET(9), 0x21e1cde6, 5)
      STEP(G, d, a, b, c, GET(14), 0xc33707d6, 9)
      STEP(G, c, d, a, b, GET(3), 0xf4d50d87, 14)
      STEP(G, b, c, d, a, GET(8), 0x455a14ed, 20)
      STEP(G, a, b, c, d, GET(13), 0xa9e3e905, 5)
      STEP(G, d, a, b, c, GET(2), 0xfcefa3f8, 9)
      STEP(G, c, d, a, b, GET(7), 0x676f02d9, 14)
      STEP(G, b, c, d, a, GET(12), 0x8d2a4c8a, 20)

  /* Round 3 */
      STEP(H, a, b, c, d, GET(5), 0xfffa3942, 4)
      STEP(H2, d, a, b, c, GET(8), 0x8771f681, 11)
      STEP(H, c, d, a, b, GET(11), 0x6d9d6122, 16)
      STEP(H2, b, c, d, a, GET(14), 0xfde5380c, 23)
      STEP(H, a, b, c, d, GET(1), 0xa4beea44, 4)
      STEP(H2, d, a, b, c, GET(4), 0x4bdecfa9, 11)
      STEP(H, c, d, a, b, GET(7), 0xf6bb4b60, 16)
      STEP(H2, b, c, d, a, GET(10), 0xbebfbc70, 23)
      STEP(H, a, b, c, d, GET(13), 0x289b7ec6, 4)
      STEP(H2, d, a, b, c, GET(0), 0xeaa127fa, 11)
      STEP(H, c, d, a, b, GET(3), 0xd4ef3085, 16)
      STEP(H2, b, c, d, a, GET(6), 0x04881d05, 23)
      STEP(H, a, b, c, d, GET(9), 0xd9d4d039, 4)
      STEP(H2, d, a, b, c, GET(12), 0xe6db99e5, 11)
      STEP(H, c, d, a, b, GET(15), 0x1fa27cf8, 16)
      STEP(H2, b, c, d, a, GET(2), 0xc4ac5665, 23)

  /* Round 4 */
      STEP(I, a, b, c, d, GET(0), 0xf4292244, 6)
      STEP(I, d, a, b, c, GET(7), 0x432aff97, 10)
      STEP(I, c, d, a, b, GET(14), 0xab9423a7, 15)
      STEP(I, b, c, d, a, GET(5), 0xfc93a039, 21)
      STEP(I, a, b, c, d, GET(12), 0x655b59c3, 6)
      STEP(I, d, a, b, c, GET(3), 0x8f0ccc92, 10)
      STEP(I, c, d, a, b, GET(10), 0xffeff47d, 15)
      STEP(I, b, c, d, a, GET(1), 0x85845dd1, 21)
      STEP(I, a, b, c, d, GET(8), 0x6fa87e4f, 6)
      STEP(I, d, a, b, c, GET(15), 0xfe2ce6e0, 10)
      STEP(I, c, d, a, b, GET(6), 0xa3014314, 15)
      STEP(I, b, c, d, a, GET(13), 0x4e0811a1, 21)
      STEP(I, a, b, c, d, GET(4), 0xf7537e82, 6)
      STEP(I, d, a, b, c, GET(11), 0xbd3af235, 10)
      STEP(I, c, d, a, b, GET(2), 0x2ad7d2bb, 15)
      STEP(I, b, c, d, a, GET(9), 0xeb86d391, 21)

      a += saved_a;
      b += saved_b;
      c += saved_c;
      d += saved_d;

      ptr += 64;
    } while (size -= 64);

    ctx->a = a;
    ctx->b = b;
    ctx->c = c;
    ctx->d = d;

    return ptr;
  }

  //////////////////////// md5_init /////////////////////////////////////////
  void md5_init(md5 *context)
  {
    MD5_CTX *ctx = static_cast<MD5_CTX*>(context->state);

    ctx->a = 0x67452301;
    ctx->b = 0xefcdab89;
    ctx->c = 0x98badcfe;
    ctx->d = 0x10325476;

    ctx->lo = 0;
    ctx->hi = 0;
  }

  //////////////////////// md5_update ///////////////////////////////////////
  void md5_update(md5 *context, const void *data, size_t size)
  {
    MD5_CTX *ctx = static_cast<MD5_CTX*>(context->state);

    uint32_t saved_lo;
    unsigned long used, available;

    saved_lo = ctx->lo;
    if ((ctx->lo = (saved_lo + size) & 0x1fffffff) < saved_lo)
      ctx->hi++;
    ctx->hi += size >> 29;

    used = saved_lo & 0x3f;

    if (used) {
      available = 64 - used;

      if (size < available) {
        memcpy(&ctx->buffer[used], data, size);
        return;
      }

      memcpy(&ctx->buffer[used], data, available);
      data = (const uint8_t *)data + available;
      size -= available;
      body(ctx, ctx->buffer, 64);
    }

    if (size >= 64) {
      data = body(ctx, data, size & ~(unsigned long)0x3f);
      size &= 0x3f;
    }

    memcpy(ctx->buffer, data, size);
  }

  //////////////////////// md5_finalise /////////////////////////////////////
  void md5_finalise(md5 *context)
  {
    MD5_CTX *ctx = static_cast<MD5_CTX*>(context->state);

    unsigned long used, available;

    used = ctx->lo & 0x3f;

    ctx->buffer[used++] = 0x80;

    available = 64 - used;

    if (available < 8) {
      memset(&ctx->buffer[used], 0, available);
      body(ctx, ctx->buffer, 64);
      used = 0;
      available = 64;
    }

    memset(&ctx->buffer[used], 0, available - 8);

    ctx->lo <<= 3;
    ctx->buffer[56] = ctx->lo;
    ctx->buffer[57] = ctx->lo >> 8;
    ctx->buffer[58] = ctx->lo >> 16;
    ctx->buffer[59] = ctx->lo >> 24;
    ctx->buffer[60] = ctx->hi;
    ctx->buffer[61] = ctx->hi >> 8;
    ctx->buffer[62] = ctx->hi >> 16;
    ctx->buffer[63] = ctx->hi >> 24;

    body(ctx, ctx->buffer, 64);

    ctx->result[0] = ctx->a;
    ctx->result[1] = ctx->a >> 8;
    ctx->result[2] = ctx->a >> 16;
    ctx->result[3] = ctx->a >> 24;
    ctx->result[4] = ctx->b;
    ctx->result[5] = ctx->b >> 8;
    ctx->result[6] = ctx->b >> 16;
    ctx->result[7] = ctx->b >> 24;
    ctx->result[8] = ctx->c;
    ctx->result[9] = ctx->c >> 8;
    ctx->result[10] = ctx->c >> 16;
    ctx->result[11] = ctx->c >> 24;
    ctx->result[12] = ctx->d;
    ctx->result[13] = ctx->d >> 8;
    ctx->result[14] = ctx->d >> 16;
    ctx->result[15] = ctx->d >> 24;

    ctx->lo = ctx->hi = 0;
    ctx->a = ctx->b = ctx->c = ctx->d = 0;
    memset(ctx->buffer, 0, 64);
    memset(ctx->block, 0, 64);
  }

  //////////////////////// md5digest ////////////////////////////////////////
  md5 md5digest(const void *data, size_t size)
  {
    md5 context;

    md5_init(&context);
    md5_update(&context, data, size);
    md5_finalise(&context);

    return context;
  }


} } // namespace crypto
