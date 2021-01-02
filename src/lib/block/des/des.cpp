/*
* DES
* (C) 1999-2008,2018,2020 Jack Lloyd
*
* Based on a public domain implemenation by Phil Karn (who in turn
* credited Richard Outerbridge and Jim Gillogly)
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#include <botan/internal/des.h>
#include <botan/internal/loadstor.h>
#include <botan/internal/rotate.h>
#include <botan/internal/cpuid.h>

namespace Botan {

namespace {

alignas(256) const uint32_t DES_SPBOX[64*8] = {
   0x01010400, 0x00000000, 0x00010000, 0x01010404, 0x01010004, 0x00010404,
   0x00000004, 0x00010000, 0x00000400, 0x01010400, 0x01010404, 0x00000400,
   0x01000404, 0x01010004, 0x01000000, 0x00000004, 0x00000404, 0x01000400,
   0x01000400, 0x00010400, 0x00010400, 0x01010000, 0x01010000, 0x01000404,
   0x00010004, 0x01000004, 0x01000004, 0x00010004, 0x00000000, 0x00000404,
   0x00010404, 0x01000000, 0x00010000, 0x01010404, 0x00000004, 0x01010000,
   0x01010400, 0x01000000, 0x01000000, 0x00000400, 0x01010004, 0x00010000,
   0x00010400, 0x01000004, 0x00000400, 0x00000004, 0x01000404, 0x00010404,
   0x01010404, 0x00010004, 0x01010000, 0x01000404, 0x01000004, 0x00000404,
   0x00010404, 0x01010400, 0x00000404, 0x01000400, 0x01000400, 0x00000000,
   0x00010004, 0x00010400, 0x00000000, 0x01010004,

   0x80108020, 0x80008000, 0x00008000, 0x00108020, 0x00100000, 0x00000020,
   0x80100020, 0x80008020, 0x80000020, 0x80108020, 0x80108000, 0x80000000,
   0x80008000, 0x00100000, 0x00000020, 0x80100020, 0x00108000, 0x00100020,
   0x80008020, 0x00000000, 0x80000000, 0x00008000, 0x00108020, 0x80100000,
   0x00100020, 0x80000020, 0x00000000, 0x00108000, 0x00008020, 0x80108000,
   0x80100000, 0x00008020, 0x00000000, 0x00108020, 0x80100020, 0x00100000,
   0x80008020, 0x80100000, 0x80108000, 0x00008000, 0x80100000, 0x80008000,
   0x00000020, 0x80108020, 0x00108020, 0x00000020, 0x00008000, 0x80000000,
   0x00008020, 0x80108000, 0x00100000, 0x80000020, 0x00100020, 0x80008020,
   0x80000020, 0x00100020, 0x00108000, 0x00000000, 0x80008000, 0x00008020,
   0x80000000, 0x80100020, 0x80108020, 0x00108000,

   0x00000208, 0x08020200, 0x00000000, 0x08020008, 0x08000200, 0x00000000,
   0x00020208, 0x08000200, 0x00020008, 0x08000008, 0x08000008, 0x00020000,
   0x08020208, 0x00020008, 0x08020000, 0x00000208, 0x08000000, 0x00000008,
   0x08020200, 0x00000200, 0x00020200, 0x08020000, 0x08020008, 0x00020208,
   0x08000208, 0x00020200, 0x00020000, 0x08000208, 0x00000008, 0x08020208,
   0x00000200, 0x08000000, 0x08020200, 0x08000000, 0x00020008, 0x00000208,
   0x00020000, 0x08020200, 0x08000200, 0x00000000, 0x00000200, 0x00020008,
   0x08020208, 0x08000200, 0x08000008, 0x00000200, 0x00000000, 0x08020008,
   0x08000208, 0x00020000, 0x08000000, 0x08020208, 0x00000008, 0x00020208,
   0x00020200, 0x08000008, 0x08020000, 0x08000208, 0x00000208, 0x08020000,
   0x00020208, 0x00000008, 0x08020008, 0x00020200,

   0x00802001, 0x00002081, 0x00002081, 0x00000080, 0x00802080, 0x00800081,
   0x00800001, 0x00002001, 0x00000000, 0x00802000, 0x00802000, 0x00802081,
   0x00000081, 0x00000000, 0x00800080, 0x00800001, 0x00000001, 0x00002000,
   0x00800000, 0x00802001, 0x00000080, 0x00800000, 0x00002001, 0x00002080,
   0x00800081, 0x00000001, 0x00002080, 0x00800080, 0x00002000, 0x00802080,
   0x00802081, 0x00000081, 0x00800080, 0x00800001, 0x00802000, 0x00802081,
   0x00000081, 0x00000000, 0x00000000, 0x00802000, 0x00002080, 0x00800080,
   0x00800081, 0x00000001, 0x00802001, 0x00002081, 0x00002081, 0x00000080,
   0x00802081, 0x00000081, 0x00000001, 0x00002000, 0x00800001, 0x00002001,
   0x00802080, 0x00800081, 0x00002001, 0x00002080, 0x00800000, 0x00802001,
   0x00000080, 0x00800000, 0x00002000, 0x00802080,

   0x00000100, 0x02080100, 0x02080000, 0x42000100, 0x00080000, 0x00000100,
   0x40000000, 0x02080000, 0x40080100, 0x00080000, 0x02000100, 0x40080100,
   0x42000100, 0x42080000, 0x00080100, 0x40000000, 0x02000000, 0x40080000,
   0x40080000, 0x00000000, 0x40000100, 0x42080100, 0x42080100, 0x02000100,
   0x42080000, 0x40000100, 0x00000000, 0x42000000, 0x02080100, 0x02000000,
   0x42000000, 0x00080100, 0x00080000, 0x42000100, 0x00000100, 0x02000000,
   0x40000000, 0x02080000, 0x42000100, 0x40080100, 0x02000100, 0x40000000,
   0x42080000, 0x02080100, 0x40080100, 0x00000100, 0x02000000, 0x42080000,
   0x42080100, 0x00080100, 0x42000000, 0x42080100, 0x02080000, 0x00000000,
   0x40080000, 0x42000000, 0x00080100, 0x02000100, 0x40000100, 0x00080000,
   0x00000000, 0x40080000, 0x02080100, 0x40000100,

   0x20000010, 0x20400000, 0x00004000, 0x20404010, 0x20400000, 0x00000010,
   0x20404010, 0x00400000, 0x20004000, 0x00404010, 0x00400000, 0x20000010,
   0x00400010, 0x20004000, 0x20000000, 0x00004010, 0x00000000, 0x00400010,
   0x20004010, 0x00004000, 0x00404000, 0x20004010, 0x00000010, 0x20400010,
   0x20400010, 0x00000000, 0x00404010, 0x20404000, 0x00004010, 0x00404000,
   0x20404000, 0x20000000, 0x20004000, 0x00000010, 0x20400010, 0x00404000,
   0x20404010, 0x00400000, 0x00004010, 0x20000010, 0x00400000, 0x20004000,
   0x20000000, 0x00004010, 0x20000010, 0x20404010, 0x00404000, 0x20400000,
   0x00404010, 0x20404000, 0x00000000, 0x20400010, 0x00000010, 0x00004000,
   0x20400000, 0x00404010, 0x00004000, 0x00400010, 0x20004010, 0x00000000,
   0x20404000, 0x20000000, 0x00400010, 0x20004010,

   0x00200000, 0x04200002, 0x04000802, 0x00000000, 0x00000800, 0x04000802,
   0x00200802, 0x04200800, 0x04200802, 0x00200000, 0x00000000, 0x04000002,
   0x00000002, 0x04000000, 0x04200002, 0x00000802, 0x04000800, 0x00200802,
   0x00200002, 0x04000800, 0x04000002, 0x04200000, 0x04200800, 0x00200002,
   0x04200000, 0x00000800, 0x00000802, 0x04200802, 0x00200800, 0x00000002,
   0x04000000, 0x00200800, 0x04000000, 0x00200800, 0x00200000, 0x04000802,
   0x04000802, 0x04200002, 0x04200002, 0x00000002, 0x00200002, 0x04000000,
   0x04000800, 0x00200000, 0x04200800, 0x00000802, 0x00200802, 0x04200800,
   0x00000802, 0x04000002, 0x04200802, 0x04200000, 0x00200800, 0x00000000,
   0x00000002, 0x04200802, 0x00000000, 0x00200802, 0x04200000, 0x00000800,
   0x04000002, 0x04000800, 0x00000800, 0x00200002,

   0x10001040, 0x00001000, 0x00040000, 0x10041040, 0x10000000, 0x10001040,
   0x00000040, 0x10000000, 0x00040040, 0x10040000, 0x10041040, 0x00041000,
   0x10041000, 0x00041040, 0x00001000, 0x00000040, 0x10040000, 0x10000040,
   0x10001000, 0x00001040, 0x00041000, 0x00040040, 0x10040040, 0x10041000,
   0x00001040, 0x00000000, 0x00000000, 0x10040040, 0x10000040, 0x10001000,
   0x00041040, 0x00040000, 0x00041040, 0x00040000, 0x10041000, 0x00001000,
   0x00000040, 0x10040040, 0x00001000, 0x00041040, 0x10001000, 0x00000040,
   0x10000040, 0x10040000, 0x10040040, 0x10000000, 0x00040000, 0x10001040,
   0x00000000, 0x10041040, 0x00040040, 0x10000040, 0x10040000, 0x10001000,
   0x10001040, 0x00000000, 0x10041040, 0x00041000, 0x00041000, 0x00001040,
   0x00001040, 0x00040040, 0x10000000, 0x10041000 };

/*
* DES Key Schedule
*/
void des_key_schedule(uint32_t round_key[32], const uint8_t key[8])
   {
   static const uint8_t ROT[16] = { 1, 1, 2, 2, 2, 2, 2, 2,
                                 1, 2, 2, 2, 2, 2, 2, 1 };

   uint32_t C = ((key[7] & 0x80) << 20) | ((key[6] & 0x80) << 19) |
                ((key[5] & 0x80) << 18) | ((key[4] & 0x80) << 17) |
                ((key[3] & 0x80) << 16) | ((key[2] & 0x80) << 15) |
                ((key[1] & 0x80) << 14) | ((key[0] & 0x80) << 13) |
                ((key[7] & 0x40) << 13) | ((key[6] & 0x40) << 12) |
                ((key[5] & 0x40) << 11) | ((key[4] & 0x40) << 10) |
                ((key[3] & 0x40) <<  9) | ((key[2] & 0x40) <<  8) |
                ((key[1] & 0x40) <<  7) | ((key[0] & 0x40) <<  6) |
                ((key[7] & 0x20) <<  6) | ((key[6] & 0x20) <<  5) |
                ((key[5] & 0x20) <<  4) | ((key[4] & 0x20) <<  3) |
                ((key[3] & 0x20) <<  2) | ((key[2] & 0x20) <<  1) |
                ((key[1] & 0x20)      ) | ((key[0] & 0x20) >>  1) |
                ((key[7] & 0x10) >>  1) | ((key[6] & 0x10) >>  2) |
                ((key[5] & 0x10) >>  3) | ((key[4] & 0x10) >>  4);
   uint32_t D = ((key[7] & 0x02) << 26) | ((key[6] & 0x02) << 25) |
                ((key[5] & 0x02) << 24) | ((key[4] & 0x02) << 23) |
                ((key[3] & 0x02) << 22) | ((key[2] & 0x02) << 21) |
                ((key[1] & 0x02) << 20) | ((key[0] & 0x02) << 19) |
                ((key[7] & 0x04) << 17) | ((key[6] & 0x04) << 16) |
                ((key[5] & 0x04) << 15) | ((key[4] & 0x04) << 14) |
                ((key[3] & 0x04) << 13) | ((key[2] & 0x04) << 12) |
                ((key[1] & 0x04) << 11) | ((key[0] & 0x04) << 10) |
                ((key[7] & 0x08) <<  8) | ((key[6] & 0x08) <<  7) |
                ((key[5] & 0x08) <<  6) | ((key[4] & 0x08) <<  5) |
                ((key[3] & 0x08) <<  4) | ((key[2] & 0x08) <<  3) |
                ((key[1] & 0x08) <<  2) | ((key[0] & 0x08) <<  1) |
                ((key[3] & 0x10) >>  1) | ((key[2] & 0x10) >>  2) |
                ((key[1] & 0x10) >>  3) | ((key[0] & 0x10) >>  4);

   for(size_t i = 0; i != 16; ++i)
      {
      C = ((C << ROT[i]) | (C >> (28-ROT[i]))) & 0x0FFFFFFF;
      D = ((D << ROT[i]) | (D >> (28-ROT[i]))) & 0x0FFFFFFF;
      round_key[2*i  ] = ((C & 0x00000010) << 22) | ((C & 0x00000800) << 17) |
                         ((C & 0x00000020) << 16) | ((C & 0x00004004) << 15) |
                         ((C & 0x00000200) << 11) | ((C & 0x00020000) << 10) |
                         ((C & 0x01000000) >>  6) | ((C & 0x00100000) >>  4) |
                         ((C & 0x00010000) <<  3) | ((C & 0x08000000) >>  2) |
                         ((C & 0x00800000) <<  1) | ((D & 0x00000010) <<  8) |
                         ((D & 0x00000002) <<  7) | ((D & 0x00000001) <<  2) |
                         ((D & 0x00000200)      ) | ((D & 0x00008000) >>  2) |
                         ((D & 0x00000088) >>  3) | ((D & 0x00001000) >>  7) |
                         ((D & 0x00080000) >>  9) | ((D & 0x02020000) >> 14) |
                         ((D & 0x00400000) >> 21);
      round_key[2*i+1] = ((C & 0x00000001) << 28) | ((C & 0x00000082) << 18) |
                         ((C & 0x00002000) << 14) | ((C & 0x00000100) << 10) |
                         ((C & 0x00001000) <<  9) | ((C & 0x00040000) <<  6) |
                         ((C & 0x02400000) <<  4) | ((C & 0x00008000) <<  2) |
                         ((C & 0x00200000) >>  1) | ((C & 0x04000000) >> 10) |
                         ((D & 0x00000020) <<  6) | ((D & 0x00000100)      ) |
                         ((D & 0x00000800) >>  1) | ((D & 0x00000040) >>  3) |
                         ((D & 0x00010000) >>  4) | ((D & 0x00000400) >>  5) |
                         ((D & 0x00004000) >> 10) | ((D & 0x04000000) >> 13) |
                         ((D & 0x00800000) >> 14) | ((D & 0x00100000) >> 18) |
                         ((D & 0x01000000) >> 24) | ((D & 0x08000000) >> 26);
      }
   }

inline uint32_t spbox(uint32_t T0, uint32_t T1)
   {
   return
      DES_SPBOX[64*0+((T0 >> 24) & 0x3F)] ^
      DES_SPBOX[64*1+((T1 >> 24) & 0x3F)] ^
      DES_SPBOX[64*2+((T0 >> 16) & 0x3F)] ^
      DES_SPBOX[64*3+((T1 >> 16) & 0x3F)] ^
      DES_SPBOX[64*4+((T0 >>  8) & 0x3F)] ^
      DES_SPBOX[64*5+((T1 >>  8) & 0x3F)] ^
      DES_SPBOX[64*6+((T0 >>  0) & 0x3F)] ^
      DES_SPBOX[64*7+((T1 >>  0) & 0x3F)];
   }

/*
* DES Encryption
*/
inline void des_encrypt(uint32_t& Lr, uint32_t& Rr,
                        const uint32_t round_key[32])
   {
   uint32_t L = Lr;
   uint32_t R = Rr;
   for(size_t i = 0; i != 16; i += 2)
      {
      L ^= spbox(rotr<4>(R) ^ round_key[2*i  ], R ^ round_key[2*i+1]);
      R ^= spbox(rotr<4>(L) ^ round_key[2*i+2], L ^ round_key[2*i+3]);
      }

   Lr = L;
   Rr = R;
   }

inline void des_encrypt_x2(uint32_t& L0r, uint32_t& R0r,
                           uint32_t& L1r, uint32_t& R1r,
                           const uint32_t round_key[32])
   {
   uint32_t L0 = L0r;
   uint32_t R0 = R0r;
   uint32_t L1 = L1r;
   uint32_t R1 = R1r;

   for(size_t i = 0; i != 16; i += 2)
      {
      L0 ^= spbox(rotr<4>(R0) ^ round_key[2*i  ], R0 ^ round_key[2*i+1]);
      L1 ^= spbox(rotr<4>(R1) ^ round_key[2*i  ], R1 ^ round_key[2*i+1]);

      R0 ^= spbox(rotr<4>(L0) ^ round_key[2*i+2], L0 ^ round_key[2*i+3]);
      R1 ^= spbox(rotr<4>(L1) ^ round_key[2*i+2], L1 ^ round_key[2*i+3]);
      }

   L0r = L0;
   R0r = R0;
   L1r = L1;
   R1r = R1;
   }

/*
* DES Decryption
*/
inline void des_decrypt(uint32_t& Lr, uint32_t& Rr,
                        const uint32_t round_key[32])
   {
   uint32_t L = Lr;
   uint32_t R = Rr;
   for(size_t i = 16; i != 0; i -= 2)
      {
      L ^= spbox(rotr<4>(R) ^ round_key[2*i - 2], R  ^ round_key[2*i - 1]);
      R ^= spbox(rotr<4>(L) ^ round_key[2*i - 4], L  ^ round_key[2*i - 3]);
      }
   Lr = L;
   Rr = R;
   }

inline void des_decrypt_x2(uint32_t& L0r, uint32_t& R0r,
                           uint32_t& L1r, uint32_t& R1r,
                           const uint32_t round_key[32])
   {
   uint32_t L0 = L0r;
   uint32_t R0 = R0r;
   uint32_t L1 = L1r;
   uint32_t R1 = R1r;

   for(size_t i = 16; i != 0; i -= 2)
      {
      L0 ^= spbox(rotr<4>(R0) ^ round_key[2*i - 2], R0  ^ round_key[2*i - 1]);
      L1 ^= spbox(rotr<4>(R1) ^ round_key[2*i - 2], R1  ^ round_key[2*i - 1]);

      R0 ^= spbox(rotr<4>(L0) ^ round_key[2*i - 4], L0  ^ round_key[2*i - 3]);
      R1 ^= spbox(rotr<4>(L1) ^ round_key[2*i - 4], L1  ^ round_key[2*i - 3]);
      }

   L0r = L0;
   R0r = R0;
   L1r = L1;
   R1r = R1;
   }

inline void des_IP(uint32_t& L, uint32_t& R)
   {
   // IP sequence by Wei Dai, taken from public domain Crypto++
   uint32_t T;
   R = rotl<4>(R);
   T = (L ^ R) & 0xF0F0F0F0;
   L ^= T;
   R = rotr<20>(R ^ T);
   T = (L ^ R) & 0xFFFF0000;
   L ^= T;
   R = rotr<18>(R ^ T);
   T = (L ^ R) & 0x33333333;
   L ^= T;
   R = rotr<6>(R ^ T);
   T = (L ^ R) & 0x00FF00FF;
   L ^= T;
   R = rotl<9>(R ^ T);
   T = (L ^ R) & 0xAAAAAAAA;
   L = rotl<1>(L ^ T);
   R ^= T;
   }

inline void des_FP(uint32_t& L, uint32_t& R)
   {
   // FP sequence by Wei Dai, taken from public domain Crypto++
   uint32_t T;

   R = rotr<1>(R);
   T = (L ^ R) & 0xAAAAAAAA;
   R ^= T;
   L = rotr<9>(L ^ T);
   T = (L ^ R) & 0x00FF00FF;
   R ^= T;
   L = rotl<6>(L ^ T);
   T = (L ^ R) & 0x33333333;
   R ^= T;
   L = rotl<18>(L ^ T);
   T = (L ^ R) & 0xFFFF0000;
   R ^= T;
   L = rotl<20>(L ^ T);
   T = (L ^ R) & 0xF0F0F0F0;
   R ^= T;
   L = rotr<4>(L ^ T);
   }

}

/*
* DES Encryption
*/
void DES::encrypt_n(const uint8_t in[], uint8_t out[], size_t blocks) const
   {
   verify_key_set(m_round_key.empty() == false);

   while(blocks >= 2)
      {
      uint32_t L0 = load_be<uint32_t>(in, 0);
      uint32_t R0 = load_be<uint32_t>(in, 1);
      uint32_t L1 = load_be<uint32_t>(in, 2);
      uint32_t R1 = load_be<uint32_t>(in, 3);

      des_IP(L0, R0);
      des_IP(L1, R1);

      des_encrypt_x2(L0, R0, L1, R1, m_round_key.data());

      des_FP(L0, R0);
      des_FP(L1, R1);

      store_be(out, R0, L0, R1, L1);

      in += 2*BLOCK_SIZE;
      out += 2*BLOCK_SIZE;
      blocks -= 2;
      }

   while(blocks > 0)
      {
      uint32_t L0 = load_be<uint32_t>(in, 0);
      uint32_t R0 = load_be<uint32_t>(in, 1);
      des_IP(L0, R0);
      des_encrypt(L0, R0, m_round_key.data());
      des_FP(L0, R0);
      store_be(out, R0, L0);

      in += BLOCK_SIZE;
      out += BLOCK_SIZE;
      blocks -= 1;
      }
   }

/*
* DES Decryption
*/
void DES::decrypt_n(const uint8_t in[], uint8_t out[], size_t blocks) const
   {
   verify_key_set(m_round_key.empty() == false);

   while(blocks >= 2)
      {
      uint32_t L0 = load_be<uint32_t>(in, 0);
      uint32_t R0 = load_be<uint32_t>(in, 1);
      uint32_t L1 = load_be<uint32_t>(in, 2);
      uint32_t R1 = load_be<uint32_t>(in, 3);

      des_IP(L0, R0);
      des_IP(L1, R1);

      des_decrypt_x2(L0, R0, L1, R1, m_round_key.data());

      des_FP(L0, R0);
      des_FP(L1, R1);

      store_be(out, R0, L0, R1, L1);

      in += 2*BLOCK_SIZE;
      out += 2*BLOCK_SIZE;
      blocks -= 2;
      }

   while(blocks > 0)
      {
      uint32_t L0 = load_be<uint32_t>(in, 0);
      uint32_t R0 = load_be<uint32_t>(in, 1);
      des_IP(L0, R0);
      des_decrypt(L0, R0, m_round_key.data());
      des_FP(L0, R0);
      store_be(out, R0, L0);

      in += BLOCK_SIZE;
      out += BLOCK_SIZE;
      blocks -= 1;
      }
   }

/*
* DES Key Schedule
*/
void DES::key_schedule(const uint8_t key[], size_t)
   {
   m_round_key.resize(32);
   des_key_schedule(m_round_key.data(), key);
   }

void DES::clear()
   {
   zap(m_round_key);
   }

/*
* TripleDES Encryption
*/
void TripleDES::encrypt_n(const uint8_t in[], uint8_t out[], size_t blocks) const
   {
   verify_key_set(m_round_key.empty() == false);

#if defined(BOTAN_HAS_DES_BMI2)
   if(CPUID::has_bmi2() && CPUID::has_fast_pdep())
      {
      return bmi2_encrypt_n(in, out, blocks, &m_round_key[0]);
      }
#endif

   while(blocks >= 2)
      {
      uint32_t L0 = load_be<uint32_t>(in, 0);
      uint32_t R0 = load_be<uint32_t>(in, 1);
      uint32_t L1 = load_be<uint32_t>(in, 2);
      uint32_t R1 = load_be<uint32_t>(in, 3);

      des_IP(L0, R0);
      des_IP(L1, R1);

      des_encrypt_x2(L0, R0, L1, R1, &m_round_key[0]);
      des_decrypt_x2(R0, L0, R1, L1, &m_round_key[32]);
      des_encrypt_x2(L0, R0, L1, R1, &m_round_key[64]);

      des_FP(L0, R0);
      des_FP(L1, R1);

      store_be(out, R0, L0, R1, L1);

      in += 2*BLOCK_SIZE;
      out += 2*BLOCK_SIZE;
      blocks -= 2;
      }

   while(blocks > 0)
      {
      uint32_t L0 = load_be<uint32_t>(in, 0);
      uint32_t R0 = load_be<uint32_t>(in, 1);

      des_IP(L0, R0);
      des_encrypt(L0, R0, &m_round_key[0]);
      des_decrypt(R0, L0, &m_round_key[32]);
      des_encrypt(L0, R0, &m_round_key[64]);
      des_FP(L0, R0);

      store_be(out, R0, L0);

      in += BLOCK_SIZE;
      out += BLOCK_SIZE;
      blocks -= 1;
      }
   }

/*
* TripleDES Decryption
*/
void TripleDES::decrypt_n(const uint8_t in[], uint8_t out[], size_t blocks) const
   {
   verify_key_set(m_round_key.empty() == false);

#if defined(BOTAN_HAS_DES_BMI2)
   if(CPUID::has_bmi2() && CPUID::has_fast_pdep())
      {
      return bmi2_decrypt_n(in, out, blocks, &m_round_key[0]);
      }
#endif

   while(blocks >= 2)
      {
      uint32_t L0 = load_be<uint32_t>(in, 0);
      uint32_t R0 = load_be<uint32_t>(in, 1);
      uint32_t L1 = load_be<uint32_t>(in, 2);
      uint32_t R1 = load_be<uint32_t>(in, 3);

      des_IP(L0, R0);
      des_IP(L1, R1);

      des_decrypt_x2(L0, R0, L1, R1, &m_round_key[64]);
      des_encrypt_x2(R0, L0, R1, L1, &m_round_key[32]);
      des_decrypt_x2(L0, R0, L1, R1, &m_round_key[0]);

      des_FP(L0, R0);
      des_FP(L1, R1);

      store_be(out, R0, L0, R1, L1);

      in += 2*BLOCK_SIZE;
      out += 2*BLOCK_SIZE;
      blocks -= 2;
      }

   while(blocks > 0)
      {
      uint32_t L0 = load_be<uint32_t>(in, 0);
      uint32_t R0 = load_be<uint32_t>(in, 1);

      des_IP(L0, R0);
      des_decrypt(L0, R0, &m_round_key[64]);
      des_encrypt(R0, L0, &m_round_key[32]);
      des_decrypt(L0, R0, &m_round_key[0]);
      des_FP(L0, R0);

      store_be(out, R0, L0);

      in += BLOCK_SIZE;
      out += BLOCK_SIZE;
      blocks -= 1;
      }
   }

/*
* TripleDES Key Schedule
*/
void TripleDES::key_schedule(const uint8_t key[], size_t length)
   {
   m_round_key.resize(3*32);
   des_key_schedule(&m_round_key[0], key);
   des_key_schedule(&m_round_key[32], key + 8);

   if(length == 24)
      des_key_schedule(&m_round_key[64], key + 16);
   else
      copy_mem(&m_round_key[64], &m_round_key[0], 32);
   }

void TripleDES::clear()
   {
   zap(m_round_key);
   }

}
