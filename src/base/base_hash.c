// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ Integer Hashes (for hash tables)

// MurmurHash3 64-bit finalizer: bijective, good avalanche, no multiplications by
// small constants, so sequential keys (0,1,2,...) spread evenly across buckets.
internal U64
u64_hash_from_u64(U64 x)
{
  x ^= x >> 33;
  x *= 0xff51afd7ed558ccdULL;
  x ^= x >> 33;
  x *= 0xc4ceb9fe1a85ec53ULL;
  x ^= x >> 33;
  return x;
}

internal U64
u64_hash_from_s32(S32 x)
{
  // Cast through U32 so negative values hash by their bit pattern (no sign-extension surprises).
  return u64_hash_from_u64((U64)(U32)x);
}

////////////////////////////////
//~ rjf: MD5

#include "../third_party/martins_hash/md5.h"

internal MD5
md5_from_data(String8 data)
{
  md5_ctx ctx = {0};
  md5_init(&ctx);
  md5_update(&ctx, (void*)data.str, data.size);
  MD5 result = {0};
  md5_finish(&ctx, result.u8);
  return result;
}

////////////////////////////////
//~ rjf: SHA

#include "../third_party/martins_hash/sha1.h"
#include "../third_party/martins_hash/sha256.h"

internal SHA1
sha1_from_data(String8 data)
{
  SHA1 result = {0};
  {
    sha1_ctx ctx = {0};
    sha1_init(&ctx);
    sha1_update(&ctx, data.str, data.size);
    sha1_finish(&ctx, result.u8);
  }
  return result;
}

internal SHA256
sha256_from_data(String8 data)
{
  SHA256 result = {0};
  {
    sha256_ctx ctx = {0};
    sha256_init(&ctx);
    sha256_update(&ctx, data.str, data.size);
    sha256_finish(&ctx, result.u8);
  }
  return result;
}
