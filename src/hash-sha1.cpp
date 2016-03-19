//
// hash-sha1.cpp - sha1
//

// sha1 written by Steve Reid <steve@edmweb.com>

/* #define SHA1HANDSOFF * Copies data before messing with it. */

#include "leap/hash.h"
#include <cstring>
#include <cstdio>

#ifdef _WIN32
#  include <winsock2.h>
#else
#  include <netinet/in.h>
#endif

using namespace std;

#define SHA1_SIGNATURE_SIZE 20

struct SHA1_CTX
{
  uint32_t state[5];
  uint32_t count[2];
  uint8_t buffer[64];
  uint8_t result[20];
};

namespace leap { namespace crypto
{

  //|///////////////////// sha1::Constructor ////////////////////////////////
  sha1::sha1()
    : state(new SHA1_CTX)
  {
  }


  //|///////////////////// sha1::Constructor ////////////////////////////////
  sha1::sha1(sha1 &&that)
    : state(nullptr)
  {
    swap(state, that.state);
  }


  //|///////////////////// sha1::Destructor /////////////////////////////////
  sha1::~sha1()
  {
    delete static_cast<SHA1_CTX*>(state);
  }


  //|///////////////////// sha1::hex ////////////////////////////////////////
  string sha1::hex() const
  {
    char digest[size()*2];

    for(size_t i = 0; i < size(); i++)
      sprintf(digest+i*2, "%02x", data()[i]);

    return string(digest, sizeof(digest));
  }


  //|///////////////////// sha1::data ///////////////////////////////////////
  uint8_t const *sha1::data() const
  {
    return static_cast<SHA1_CTX*>(state)->result;
  }



  #define rol(value, bits) (((value) << (bits)) | ((value) >> (32 - (bits))))

  /* blk0() and blk() perform the initial expand. */
  /* I got the idea of expanding during the round function from SSLeay */
  #define blk0(i) (block->l[i] = htonl(block->l[i]))
  #define blk(i) (block->l[i&15] = rol(block->l[(i+13)&15]^block->l[(i+8)&15] \
      ^block->l[(i+2)&15]^block->l[i&15],1))

  /* (R0+R1), R2, R3, R4 are the different operations used in SHA1 */
  #define R0(v,w,x,y,z,i) z+=((w&(x^y))^y)+blk0(i)+0x5A827999+rol(v,5);w=rol(w,30);
  #define R1(v,w,x,y,z,i) z+=((w&(x^y))^y)+blk(i)+0x5A827999+rol(v,5);w=rol(w,30);
  #define R2(v,w,x,y,z,i) z+=(w^x^y)+blk(i)+0x6ED9EBA1+rol(v,5);w=rol(w,30);
  #define R3(v,w,x,y,z,i) z+=(((w|x)&y)|(w&x))+blk(i)+0x8F1BBCDC+rol(v,5);w=rol(w,30);
  #define R4(v,w,x,y,z,i) z+=(w^x^y)+blk(i)+0xCA62C1D6+rol(v,5);w=rol(w,30);


  /* Hash a single 512-bit block. This is the core of the algorithm. */

  static void sha1_transform(uint32_t state[5], const uint8_t buffer[64])
  {
      uint32_t a, b, c, d, e;
      typedef union {
          uint8_t c[64];
          uint32_t l[16];
      } CHAR64LONG16;
      CHAR64LONG16 *block;

  #ifdef SHA1HANDSOFF
      static uint8_t workspace[64];
      block = (CHAR64LONG16 *) workspace;
      memcpy(block, buffer, 64);
  #else
      block = (CHAR64LONG16 *) buffer;
  #endif
      /* Copy context->state[] to working vars */
      a = state[0];
      b = state[1];
      c = state[2];
      d = state[3];
      e = state[4];
      /* 4 rounds of 20 operations each. Loop unrolled. */
      R0(a,b,c,d,e, 0); R0(e,a,b,c,d, 1); R0(d,e,a,b,c, 2); R0(c,d,e,a,b, 3);
      R0(b,c,d,e,a, 4); R0(a,b,c,d,e, 5); R0(e,a,b,c,d, 6); R0(d,e,a,b,c, 7);
      R0(c,d,e,a,b, 8); R0(b,c,d,e,a, 9); R0(a,b,c,d,e,10); R0(e,a,b,c,d,11);
      R0(d,e,a,b,c,12); R0(c,d,e,a,b,13); R0(b,c,d,e,a,14); R0(a,b,c,d,e,15);
      R1(e,a,b,c,d,16); R1(d,e,a,b,c,17); R1(c,d,e,a,b,18); R1(b,c,d,e,a,19);
      R2(a,b,c,d,e,20); R2(e,a,b,c,d,21); R2(d,e,a,b,c,22); R2(c,d,e,a,b,23);
      R2(b,c,d,e,a,24); R2(a,b,c,d,e,25); R2(e,a,b,c,d,26); R2(d,e,a,b,c,27);
      R2(c,d,e,a,b,28); R2(b,c,d,e,a,29); R2(a,b,c,d,e,30); R2(e,a,b,c,d,31);
      R2(d,e,a,b,c,32); R2(c,d,e,a,b,33); R2(b,c,d,e,a,34); R2(a,b,c,d,e,35);
      R2(e,a,b,c,d,36); R2(d,e,a,b,c,37); R2(c,d,e,a,b,38); R2(b,c,d,e,a,39);
      R3(a,b,c,d,e,40); R3(e,a,b,c,d,41); R3(d,e,a,b,c,42); R3(c,d,e,a,b,43);
      R3(b,c,d,e,a,44); R3(a,b,c,d,e,45); R3(e,a,b,c,d,46); R3(d,e,a,b,c,47);
      R3(c,d,e,a,b,48); R3(b,c,d,e,a,49); R3(a,b,c,d,e,50); R3(e,a,b,c,d,51);
      R3(d,e,a,b,c,52); R3(c,d,e,a,b,53); R3(b,c,d,e,a,54); R3(a,b,c,d,e,55);
      R3(e,a,b,c,d,56); R3(d,e,a,b,c,57); R3(c,d,e,a,b,58); R3(b,c,d,e,a,59);
      R4(a,b,c,d,e,60); R4(e,a,b,c,d,61); R4(d,e,a,b,c,62); R4(c,d,e,a,b,63);
      R4(b,c,d,e,a,64); R4(a,b,c,d,e,65); R4(e,a,b,c,d,66); R4(d,e,a,b,c,67);
      R4(c,d,e,a,b,68); R4(b,c,d,e,a,69); R4(a,b,c,d,e,70); R4(e,a,b,c,d,71);
      R4(d,e,a,b,c,72); R4(c,d,e,a,b,73); R4(b,c,d,e,a,74); R4(a,b,c,d,e,75);
      R4(e,a,b,c,d,76); R4(d,e,a,b,c,77); R4(c,d,e,a,b,78); R4(b,c,d,e,a,79);
      /* Add the working vars back into context.state[] */
      state[0] += a;
      state[1] += b;
      state[2] += c;
      state[3] += d;
      state[4] += e;
      /* Wipe variables */
      a = b = c = d = e = 0;
  }


  //////////////////////// sha1_init ////////////////////////////////////////
  void sha1_init(sha1 *context)
  {
      SHA1_CTX *ctx = static_cast<SHA1_CTX*>(context->state);

      /* SHA1 initialization constants */
      ctx->state[0] = 0x67452301;
      ctx->state[1] = 0xEFCDAB89;
      ctx->state[2] = 0x98BADCFE;
      ctx->state[3] = 0x10325476;
      ctx->state[4] = 0xC3D2E1F0;
      ctx->count[0] = ctx->count[1] = 0;
  }


  //////////////////////// sha1_update //////////////////////////////////////
  void sha1_update(sha1 *context, const void *data, size_t len)
  {
      SHA1_CTX *ctx = static_cast<SHA1_CTX*>(context->state);

      unsigned int i, j;

      j = (ctx->count[0] >> 3) & 63;
      if ((ctx->count[0] += len << 3) < (len << 3)) ctx->count[1]++;
      ctx->count[1] += (len >> 29);
      i = 64 - j;
      while (len >= i) {
          memcpy(&ctx->buffer[j], data, i);
          sha1_transform(ctx->state, ctx->buffer);
          data = (const uint8_t *)data + i;
          len -= i;
          i = 64;
          j = 0;
      }

      memcpy(&ctx->buffer[j], data, len);
  }


  //////////////////////// sha1_finalise ////////////////////////////////////
  void sha1_finalise(sha1 *context)
  {
      SHA1_CTX *ctx = static_cast<SHA1_CTX*>(context->state);

      uint32_t i, j;
      uint8_t finalcount[8];

      for (i = 0; i < 8; i++) {
          finalcount[i] = (uint8_t)((ctx->count[(i >= 4 ? 0 : 1)]
           >> ((3-(i & 3)) * 8) ) & 255);  /* Endian independent */
      }
      sha1_update(context, (uint8_t *) "\200", 1);
      while ((ctx->count[0] & 504) != 448) {
          sha1_update(context, (uint8_t *) "\0", 1);
      }
      sha1_update(context, finalcount, 8);  /* Should cause a SHA1Transform() */
      for (i = 0; i < 20; i++) {
          ctx->result[i] = (uint8_t)((ctx->state[i>>2] >> ((3-(i & 3)) * 8) ) & 255);
      }
      /* Wipe variables */
      i = j = 0;
      memset(ctx->buffer, 0, 64);
      memset(ctx->state, 0, 20);
      memset(ctx->count, 0, 8);
      memset(&finalcount, 0, 8);
  #ifdef SHA1HANDSOFF  /* make SHA1Transform overwrite it's own static vars */
      SHA1Transform(context->state, context->buffer);
  #endif
  }


  //////////////////////// sha1digest ///////////////////////////////////////
  sha1 sha1digest(const void *data, size_t size)
  {
    sha1 context;

    sha1_init(&context);
    sha1_update(&context, data, size);
    sha1_finalise(&context);

    return context;
  }


} } // namespace crypto
