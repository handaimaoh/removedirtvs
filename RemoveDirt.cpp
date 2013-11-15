// Avisynth filter for removing dirt from film clips
//
// By Rainer Wittmann <gorw@gmx.de>
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// To get a copy of the GNU General Public License write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA, or visit
// http://www.gnu.org/copyleft/gpl.html .

#include "shared.h"

#ifdef VS_TARGET_CPU_X86
#include <emmintrin.h>
#endif

#define MOTIONBLOCKWIDTH    8
#define MOTIONBLOCKHEIGHT   8

#define MOTION_FLAG     1
#define MOTION_FLAGN    2
#define MOTION_FLAGP    4
#define TO_CLEAN        8
#define BMARGIN         16
#define MOTION_FLAG1    (MOTION_FLAG | TO_CLEAN)
#define MOTION_FLAG2    (MOTION_FLAGN | TO_CLEAN)
#define MOTION_FLAG3    (MOTION_FLAGP | TO_CLEAN)
#define MOTION_FLAGS    (MOTION_FLAG | MOTION_FLAGN | MOTION_FLAGP)

#define U_N         54u // green
#define V_N         34
#define U_M         90  // red
#define V_M         240
#define U_P         240 // blue
#define V_P         110
#define u_ncolor    (U_N + (U_N << 8) + (U_N << 16) + (U_N << 24))
#define v_ncolor    (V_N + (V_N << 8) + (V_N << 16) + (V_N << 24))
#define u_mcolor    (U_M + (U_M << 8) + (U_M << 16) + (U_M << 24))
#define v_mcolor    (V_M + (V_M << 8) + (V_M << 16) + (V_M << 24))
#define u_pcolor    (U_P + (U_P << 8) + (U_P << 16) + (U_P << 24))
#define v_pcolor    (V_P + (V_P << 8) + (V_P << 16) + (V_P << 24))

#define leftdp      (-1)
#define rightdp     7
#define leftsp      leftdp
#define rightsp     rightdp
#define topdp       (-dpitch)
#define topsp       (-spitch)
#define rightbldp   8
#define rightblsp   rightbldp
#define leftbldp    (-8)
#define leftblsp    leftbldp

#define Cleftdp     (-1)
#define Crightdp    3
#define Cleftsp     Cleftdp
#define Crightsp    Crightdp
#define Crightbldp  4
#define Crightblsp  Crightbldp
#define Ctopdp      (-dpitchUV)
#define Ctopsp      (-spitchUV)

uint32_t ALIGNED_ARRAY(blockcompare_result, 16)[4];

static __forceinline void SADcompareSSE2(const uint8_t *p1, const uint8_t *p2, int32_t pitch, const uint8_t *noiselevel)
{
    int32_t pitchx2 = pitch * 2;
    int32_t pitchx3 = pitchx2 + pitch;

    __m128i xmm0 = _mm_load_si128((__m128i*)p1);
    __m128i xmm1 = _mm_load_si128((__m128i*)(p1+pitch));

    xmm0 = _mm_sad_epu8(xmm0, *((__m128i*)p2));
    xmm1 = _mm_sad_epu8(xmm1, *((__m128i*)(p2+pitch)));

    __m128i xmm2 = _mm_load_si128((__m128i*)(p1+pitchx2));
    __m128i xmm3 = _mm_load_si128((__m128i*)(p1+pitchx3));

    xmm2 = _mm_sad_epu8(xmm2, *((__m128i*)(p2+pitchx2)));
    xmm2 = _mm_sad_epu8(xmm2, *((__m128i*)(p2+pitchx3)));

    xmm0 = _mm_add_epi32(xmm0, xmm2);
    xmm1 = _mm_add_epi32(xmm1, xmm3);

    p1 += (4*pitch);
    p2 += (4*pitch);

    xmm2 = _mm_load_si128((__m128i*)p1);
    xmm3 = _mm_load_si128((__m128i*)(p1+pitch));

    xmm2 = _mm_sad_epu8(xmm2, *((__m128i*)p2));
    xmm3 = _mm_sad_epu8(xmm3, *((__m128i*)(p2+pitch)));

    xmm0 = _mm_add_epi32(xmm0, xmm2);
    xmm1 = _mm_add_epi32(xmm1, xmm3);

    xmm2 = _mm_load_si128((__m128i*)(p1+pitchx2));
    xmm3 = _mm_load_si128((__m128i*)(p1+pitchx3));

    xmm2 = _mm_sad_epu8(xmm2, *((__m128i*)(p2+pitchx2)));
    xmm3 = _mm_sad_epu8(xmm3, *((__m128i*)(p2+pitchx3)));

    xmm0 = _mm_add_epi32(xmm0, xmm2);
    xmm1 = _mm_add_epi32(xmm1, xmm3);

    xmm0 = _mm_add_epi32(xmm0, xmm1);

    _mm_store_si128((__m128i*)blockcompare_result, xmm0);
}

static __forceinline void NSADcompareSSE2(const uint8_t *p1, const uint8_t *p2, int32_t pitch, const uint8_t *noiselevel)
{
    __m128i xmm7 = _mm_loadu_si128((__m128i*)noiselevel);

    int32_t pitchx2 = pitch * 2;
    int32_t pitchx3 = pitchx2 + pitch;
    int32_t pitchx4 = pitchx3 + pitch;

    __m128i xmm0 = _mm_load_si128((__m128i*)p1);
    __m128i xmm2 = _mm_load_si128((__m128i*)(p1+pitch));
    __m128i xmm3 = xmm0;
    __m128i xmm4 = xmm2;
    __m128i xmm5 = _mm_load_si128((__m128i*)p2);
    __m128i xmm6 = _mm_load_si128((__m128i*)(p2+pitch));

    xmm0 = _mm_subs_epu8(xmm0, xmm5);
    xmm2 = _mm_subs_epu8(xmm2, xmm6);
    xmm5 = _mm_subs_epu8(xmm5, xmm3);
    xmm6 = _mm_subs_epu8(xmm6, xmm4);
    xmm0 = _mm_subs_epu8(xmm0, xmm7);
    xmm2 = _mm_subs_epu8(xmm2, xmm7);
    xmm5 = _mm_subs_epu8(xmm5, xmm7);
    xmm6 = _mm_subs_epu8(xmm6, xmm7);

    xmm0 = _mm_sad_epu8(xmm0, xmm5);
    xmm6 = _mm_sad_epu8(xmm6, xmm2);

    __m128i xmm1 = _mm_load_si128((__m128i*)(p1+pitchx2));
    xmm2 = _mm_load_si128((__m128i*)(p1+pitchx3));
    xmm3 = xmm1;
    xmm4 = xmm2;

    xmm0 = _mm_add_epi32(xmm0, xmm6);

    xmm5 = _mm_load_si128((__m128i*)(p2+pitchx2));
    xmm6 = _mm_load_si128((__m128i*)(p2+pitchx3));

    xmm1 = _mm_subs_epu8(xmm1, xmm5);
    xmm2 = _mm_subs_epu8(xmm2, xmm6);
    xmm5 = _mm_subs_epu8(xmm5, xmm3);
    xmm6 = _mm_subs_epu8(xmm6, xmm4);
    xmm1 = _mm_subs_epu8(xmm1, xmm7);
    xmm2 = _mm_subs_epu8(xmm2, xmm7);
    xmm5 = _mm_subs_epu8(xmm5, xmm7);
    xmm6 = _mm_subs_epu8(xmm6, xmm7);

    xmm5 = _mm_sad_epu8(xmm5, xmm1);
    xmm6 = _mm_sad_epu8(xmm6, xmm2);

    xmm0 = _mm_add_epi32(xmm0, xmm5);
    xmm0 = _mm_add_epi32(xmm0, xmm6);

    p1 += pitchx4;
    p2 += pitchx4;

    xmm1 = _mm_load_si128((__m128i*)p1);
    xmm2 = _mm_load_si128((__m128i*)(p1+pitch));
    xmm3 = xmm1;
    xmm4 = xmm2;
    xmm5 = _mm_load_si128((__m128i*)p2);
    xmm6 = _mm_load_si128((__m128i*)(p2+pitch));

    xmm1 = _mm_subs_epu8(xmm1, xmm5);
    xmm2 = _mm_subs_epu8(xmm2, xmm6);
    xmm5 = _mm_subs_epu8(xmm5, xmm3);
    xmm6 = _mm_subs_epu8(xmm6, xmm4);
    xmm1 = _mm_subs_epu8(xmm1, xmm7);
    xmm2 = _mm_subs_epu8(xmm2, xmm7);
    xmm5 = _mm_subs_epu8(xmm5, xmm7);
    xmm6 = _mm_subs_epu8(xmm6, xmm7);

    xmm5 = _mm_sad_epu8(xmm5, xmm1);
    xmm6 = _mm_sad_epu8(xmm6, xmm2);

    xmm0 = _mm_add_epi32(xmm0, xmm5);
    xmm0 = _mm_add_epi32(xmm0, xmm6);

    xmm1 = _mm_load_si128((__m128i*)(p1+pitchx2));
    xmm2 = _mm_load_si128((__m128i*)(p1+pitchx3));
    xmm3 = xmm1;
    xmm4 = xmm2;
    xmm5 = _mm_load_si128((__m128i*)(p2+pitchx2));
    xmm6 = _mm_load_si128((__m128i*)(p2+pitchx3));

    xmm1 = _mm_subs_epu8(xmm1, xmm5);
    xmm2 = _mm_subs_epu8(xmm2, xmm6);
    xmm5 = _mm_subs_epu8(xmm5, xmm3);
    xmm6 = _mm_subs_epu8(xmm6, xmm4);
    xmm1 = _mm_subs_epu8(xmm1, xmm7);
    xmm2 = _mm_subs_epu8(xmm2, xmm7);
    xmm5 = _mm_subs_epu8(xmm5, xmm7);
    xmm6 = _mm_subs_epu8(xmm6, xmm7);

    xmm5 = _mm_sad_epu8(xmm5, xmm1);
    xmm6 = _mm_sad_epu8(xmm6, xmm2);

    xmm0 = _mm_add_epi32(xmm0, xmm5);
    xmm0 = _mm_add_epi32(xmm0, xmm6);

    _mm_store_si128((__m128i*)blockcompare_result, xmm0);
}

uint8_t ALIGNED_ARRAY(excessaddSSE2, 16)[16] = { 8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8 };

static __forceinline void ExcessPixelsSSE2(const uint8_t *p1, const uint8_t *p2, int32_t pitch, const uint8_t *noiselevel)
{
    int32_t pitchx2 = pitch + pitch;
    int32_t pitchx3 = pitchx2 + pitch;
    int32_t pitchx4 = pitchx3 + pitch;

    __m128i xmm7 = *((__m128i*)noiselevel);

    __m128i xmm0 = *((__m128i*)p1);
    __m128i xmm2 = *((__m128i*)(p1+pitch));
    __m128i xmm3 = xmm0;
    __m128i xmm4 = xmm2;
    __m128i xmm5 = *((__m128i*)p2);
    __m128i xmm6 = *((__m128i*)(p2+pitch));

    xmm0 = _mm_subs_epu8(xmm0, xmm5);
    xmm2 = _mm_subs_epu8(xmm2, xmm6);
    xmm5 = _mm_subs_epu8(xmm5, xmm3);
    xmm6 = _mm_subs_epu8(xmm6, xmm4);
    xmm0 = _mm_subs_epu8(xmm0, xmm7);
    xmm2 = _mm_subs_epu8(xmm2, xmm7);
    xmm5 = _mm_subs_epu8(xmm5, xmm7);
    xmm6 = _mm_subs_epu8(xmm6, xmm7);

    xmm0 = _mm_cmpeq_epi8(xmm0, xmm5);
    xmm6 = _mm_cmpeq_epi8(xmm6, xmm2);

    xmm0 = _mm_add_epi8(xmm0, xmm6);

    __m128i xmm1 = *((__m128i*)(p1+pitchx2));
    xmm2 = *((__m128i*)(p1+pitchx3));
    xmm3 = xmm1;
    xmm4 = xmm2;
    xmm5 = *((__m128i*)(p2+pitchx2));
    xmm6 = *((__m128i*)(p2+pitchx3));

    xmm1 = _mm_subs_epu8(xmm1, xmm5);
    xmm2 = _mm_subs_epu8(xmm2, xmm6);
    xmm5 = _mm_subs_epu8(xmm5, xmm3);
    xmm6 = _mm_subs_epu8(xmm6, xmm4);
    xmm1 = _mm_subs_epu8(xmm1, xmm7);
    xmm2 = _mm_subs_epu8(xmm2, xmm7);
    xmm5 = _mm_subs_epu8(xmm5, xmm7);
    xmm6 = _mm_subs_epu8(xmm6, xmm7);

    xmm5 = _mm_cmpeq_epi8(xmm5, xmm1);
    xmm6 = _mm_cmpeq_epi8(xmm6, xmm2);

    xmm0 = _mm_add_epi8(xmm0, xmm5);

    p1 += pitchx4;
    p2 += pitchx4;

    xmm0 = _mm_add_epi8(xmm0, xmm6);

    xmm1 = *((__m128i*)p1);
    xmm2 = *((__m128i*)(p1+pitch));
    xmm3 = xmm1;
    xmm4 = xmm2;
    xmm5 = *((__m128i*)p2);
    xmm6 = *((__m128i*)(p2+pitch));

    xmm1 = _mm_subs_epu8(xmm1, xmm5);
    xmm2 = _mm_subs_epu8(xmm2, xmm6);
    xmm5 = _mm_subs_epu8(xmm5, xmm3);
    xmm6 = _mm_subs_epu8(xmm6, xmm4);
    xmm1 = _mm_subs_epu8(xmm1, xmm7);
    xmm2 = _mm_subs_epu8(xmm2, xmm7);
    xmm5 = _mm_subs_epu8(xmm5, xmm7);
    xmm6 = _mm_subs_epu8(xmm6, xmm7);

    xmm5 = _mm_cmpeq_epi8(xmm5, xmm1);
    xmm6 = _mm_cmpeq_epi8(xmm6, xmm2);

    xmm0 = _mm_add_epi8(xmm0, xmm5);
    xmm0 = _mm_add_epi8(xmm0, xmm6);

    xmm1 = *((__m128i*)(p1+pitchx2));
    xmm2 = *((__m128i*)(p1+pitchx3));
    xmm3 = xmm1;
    xmm4 = xmm2;
    xmm5 = *((__m128i*)(p2+pitchx2));
    xmm6 = *((__m128i*)(p2+pitchx3));

    xmm1 = _mm_subs_epu8(xmm1, xmm5);
    xmm2 = _mm_subs_epu8(xmm2, xmm6);
    xmm5 = _mm_subs_epu8(xmm5, xmm3);
    xmm6 = _mm_subs_epu8(xmm6, xmm4);
    xmm1 = _mm_subs_epu8(xmm1, xmm7);
    xmm2 = _mm_subs_epu8(xmm2, xmm7);
    xmm5 = _mm_subs_epu8(xmm5, xmm7);
    xmm6 = _mm_subs_epu8(xmm6, xmm7);

    xmm5 = _mm_cmpeq_epi8(xmm5, xmm1);
    xmm6 = _mm_cmpeq_epi8(xmm6, xmm2);

    xmm0 = _mm_add_epi8(xmm0, xmm5);

    xmm1 = _mm_xor_si128(xmm1, xmm1);

    xmm0 = _mm_add_epi8(xmm0, xmm6);
    xmm0 = _mm_add_epi8(xmm0, *((__m128i*)excessaddSSE2));

    xmm0 = _mm_sad_epu8(xmm0, xmm1);

    _mm_store_si128((__m128i*)blockcompare_result, xmm0);
}

static __forceinline uint32_t SADcompare(const uint8_t *p1, int32_t pitch1, const uint8_t *p2, int32_t pitch2, const uint8_t *noiselevel)
{
    int32_t pitch1x2 = pitch1 + pitch1;
    int32_t pitch1x3 = pitch1x2 + pitch1;
    int32_t pitch1x4 = pitch1x3 + pitch1;
    int32_t pitch2x2 = pitch2 + pitch2;
    int32_t pitch2x3 = pitch2x2 + pitch2;
    int32_t pitch2x4 = pitch2x3 + pitch2;

    __m64 mm0 = *((__m64*)p1);
    __m64 mm1 = *((__m64*)(p1+pitch1));

    mm0 = _mm_sad_pu8(mm0, *((__m64*)p2));
    mm1 = _mm_sad_pu8(mm1, *((__m64*)(p2+pitch2)));

    __m64 mm2 = *((__m64*)(p1+pitch1x2));
    __m64 mm3 = *((__m64*)(p1+pitch1x3));

    mm2 = _mm_sad_pu8(mm2, *((__m64*)(p2+pitch2x2)));
    mm3 = _mm_sad_pu8(mm3, *((__m64*)(p2+pitch2x3)));

    mm0 = _mm_add_pi32(mm0, mm2);
    mm1 = _mm_add_pi32(mm1, mm3);

    p1 += pitch1x4;
    p2 += pitch2x4;

    mm2 = *((__m64*)p1);
    mm3 = *((__m64*)(p1+pitch1));

    mm2 = _mm_add_pi16(mm2, *((__m64*)p2));
    mm3 = _mm_add_pi16(mm3, *((__m64*)(p2+pitch2)));

    mm0 = _mm_add_pi32(mm0, mm2);
    mm1 = _mm_add_pi32(mm1, mm3);

    mm2 = *((__m64*)(p1+pitch1x2));
    mm3 = *((__m64*)(p1+pitch1x3));

    mm2 = _mm_add_pi16(mm2, *((__m64*)(p2+pitch2x2)));
    mm3 = _mm_add_pi16(mm3, *((__m64*)(p2+pitch2x3)));

    mm0 = _mm_add_pi32(mm0, mm2);
    mm1 = _mm_add_pi32(mm1, mm3);
    mm0 = _mm_add_pi32(mm0, mm1);

    return (uint32_t)_mm_cvtsi64_si32(mm0);
}

static __forceinline uint32_t NSADcompare(const uint8_t *p1, int32_t pitch1, const uint8_t *p2, int32_t pitch2, const uint8_t *noiselevel)
{
    __m128i xmm7 = _mm_loadu_si128((__m128i*)noiselevel);

    int32_t pitch1x2 = pitch1 + pitch1;
    int32_t pitch1x3 = pitch1x2 + pitch1;
    int32_t pitch1x4 = pitch1x3 + pitch1;
    int32_t pitch2x2 = pitch2 + pitch2;
    int32_t pitch2x3 = pitch2x2 + pitch2;
    int32_t pitch2x4 = pitch2x3 + pitch2;

    __m128i xmm0;
    xmm0 = _mm_castps_si128(_mm_loadh_pi(_mm_loadl_pi(_mm_castsi128_ps(xmm0), (__m64 *)(p1)), (__m64 *)(p1+pitch1x2)));
    
    __m128i xmm2;
    xmm2 = _mm_castps_si128(_mm_loadh_pi(_mm_loadl_pi(_mm_castsi128_ps(xmm2), (__m64 *)(p1+pitch1)), (__m64 *)(p1+pitch1x3)));

    __m128i xmm3 = xmm0;
    __m128i xmm4 = xmm2;

    __m128i xmm5;
    xmm5 = _mm_castps_si128(_mm_loadh_pi(_mm_loadl_pi(_mm_castsi128_ps(xmm5), (__m64 *)(p2)), (__m64 *)(p2+pitch2x2)));

    __m128i xmm6;
    xmm6 = _mm_castps_si128(_mm_loadh_pi(_mm_loadl_pi(_mm_castsi128_ps(xmm6), (__m64 *)(p2+pitch2)), (__m64 *)(p2+pitch2x3)));

    xmm0 = _mm_subs_epu8(xmm0, xmm5);
    xmm2 = _mm_subs_epu8(xmm2, xmm6);
    xmm5 = _mm_subs_epu8(xmm5, xmm3);
    xmm6 = _mm_subs_epu8(xmm6, xmm4);
    xmm0 = _mm_subs_epu8(xmm0, xmm7);
    xmm2 = _mm_subs_epu8(xmm2, xmm7);
    xmm5 = _mm_subs_epu8(xmm5, xmm7);
    xmm6 = _mm_subs_epu8(xmm6, xmm7);

    xmm0 = _mm_sad_epu8(xmm0, xmm5);
    xmm0 = _mm_sad_epu8(xmm6, xmm2);

    p1 += pitch1x4;
    p2 += pitch2x4;

    __m128i xmm1;
    xmm1 = _mm_castps_si128(_mm_loadh_pi(_mm_loadl_pi(_mm_castsi128_ps(xmm1), (__m64 *)(p1)), (__m64 *)(p1+pitch1x2)));

    xmm0 = _mm_add_epi32(xmm0, xmm6);

    xmm2 = _mm_castps_si128(_mm_loadh_pi(_mm_loadl_pi(_mm_castsi128_ps(xmm2), (__m64 *)(p1+pitch1)), (__m64 *)(p1+pitch1x3)));

    xmm3 = xmm1;
    xmm4 = xmm2;

    xmm5 = _mm_castps_si128(_mm_loadh_pi(_mm_loadl_pi(_mm_castsi128_ps(xmm5), (__m64 *)(p2)), (__m64 *)(p2+pitch2x2)));
    xmm6 = _mm_castps_si128(_mm_loadh_pi(_mm_loadl_pi(_mm_castsi128_ps(xmm6), (__m64 *)(p2+pitch2)), (__m64 *)(p2+pitch2x3)));

    xmm1 = _mm_subs_epu8(xmm1, xmm5);
    xmm2 = _mm_subs_epu8(xmm2, xmm6);
    xmm5 = _mm_subs_epu8(xmm5, xmm3);
    xmm6 = _mm_subs_epu8(xmm6, xmm4);
    xmm1 = _mm_subs_epu8(xmm1, xmm7);
    xmm2 = _mm_subs_epu8(xmm2, xmm7);
    xmm5 = _mm_subs_epu8(xmm5, xmm7);
    xmm6 = _mm_subs_epu8(xmm6, xmm7);

    xmm0 = _mm_sad_epu8(xmm1, xmm5);
    xmm0 = _mm_sad_epu8(xmm6, xmm2);

    xmm0 = _mm_add_epi32(xmm0, xmm1);
    xmm0 = _mm_add_epi32(xmm0, xmm6);

    xmm1 = _mm_castps_si128(_mm_movehl_ps(_mm_castsi128_ps(xmm1), _mm_castsi128_ps(xmm0)));

    xmm0 = _mm_add_epi32(xmm0, xmm1);

    return (uint32_t)_mm_cvtsi128_si32(xmm0);
}

uint8_t ALIGNED_ARRAY(excessadd, 16)[16] = { 4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4 };

static __forceinline uint32_t ExcessPixels(const uint8_t *p1, int32_t pitch1, const uint8_t *p2, int32_t pitch2, const uint8_t *noiselevel)
{
    __m128i xmm7 = _mm_loadu_si128((__m128i*)noiselevel);
    
    int32_t pitch1x2 = pitch1 + pitch1;
    int32_t pitch1x3 = pitch1x2 + pitch1;
    int32_t pitch1x4 = pitch1x3 + pitch1;
    int32_t pitch2x2 = pitch2 + pitch2;
    int32_t pitch2x3 = pitch2x2 + pitch2;
    int32_t pitch2x4 = pitch2x3 + pitch2;

    __m128i xmm0;
    xmm0 = _mm_castps_si128(_mm_loadh_pi(_mm_loadl_pi(_mm_castsi128_ps(xmm0), (__m64 *)(p1)), (__m64 *)(p1+pitch1x2)));

    __m128i xmm2;
    xmm2 = _mm_castps_si128(_mm_loadh_pi(_mm_loadl_pi(_mm_castsi128_ps(xmm2), (__m64 *)(p1+pitch1)), (__m64 *)(p1+pitch1x3)));
    
    __m128i xmm3 = xmm0;
    __m128i xmm4 = xmm2;

    __m128i xmm5;
    xmm5 = _mm_castps_si128(_mm_loadh_pi(_mm_loadl_pi(_mm_castsi128_ps(xmm5), (__m64 *)(p2)), (__m64 *)(p2+pitch2x2)));

    __m128i xmm6;
    xmm6 = _mm_castps_si128(_mm_loadh_pi(_mm_loadl_pi(_mm_castsi128_ps(xmm6), (__m64 *)(p2+pitch2)), (__m64 *)(p2+pitch2x3)));

    xmm0 = _mm_subs_epu8(xmm0, xmm5);
    xmm2 = _mm_subs_epu8(xmm2, xmm6);
    xmm5 = _mm_subs_epu8(xmm5, xmm3);
    xmm6 = _mm_subs_epu8(xmm6, xmm4);
    xmm0 = _mm_subs_epu8(xmm0, xmm7);
    xmm2 = _mm_subs_epu8(xmm2, xmm7);
    xmm5 = _mm_subs_epu8(xmm5, xmm7);
    xmm6 = _mm_subs_epu8(xmm6, xmm7);

    xmm0 = _mm_cmpeq_epi8(xmm0, xmm5);
    xmm6 = _mm_cmpeq_epi8(xmm6, xmm2);

    p1 += pitch1x4;
    p2 += pitch2x4;

    __m128i xmm1;
    xmm1 = _mm_castps_si128(_mm_loadh_pi(_mm_loadl_pi(_mm_castsi128_ps(xmm1), (__m64 *)(p1)), (__m64 *)(p1+pitch1x2)));

    xmm0 = _mm_add_epi8(xmm0, xmm6);

    xmm2 = _mm_castps_si128(_mm_loadh_pi(_mm_loadl_pi(_mm_castsi128_ps(xmm2), (__m64 *)(p1+pitch1)), (__m64 *)(p1+pitch1x3)));

    xmm3 = xmm1;
    xmm4 = xmm2;

    xmm5 = _mm_castps_si128(_mm_loadh_pi(_mm_loadl_pi(_mm_castsi128_ps(xmm5), (__m64 *)(p2)), (__m64 *)(p2+pitch2x2)));

    xmm6 = _mm_castps_si128(_mm_loadh_pi(_mm_loadl_pi(_mm_castsi128_ps(xmm6), (__m64 *)(p2+pitch2)), (__m64 *)(p2+pitch2x3)));

    xmm1 = _mm_subs_epu8(xmm1, xmm5);
    xmm2 = _mm_subs_epu8(xmm2, xmm6);
    xmm5 = _mm_subs_epu8(xmm5, xmm3);
    xmm6 = _mm_subs_epu8(xmm6, xmm4);
    xmm1 = _mm_subs_epu8(xmm1, xmm7);
    xmm2 = _mm_subs_epu8(xmm2, xmm7);
    xmm5 = _mm_subs_epu8(xmm5, xmm7);
    xmm6 = _mm_subs_epu8(xmm6, xmm7);

    xmm1 = _mm_cmpeq_epi8(xmm1, xmm5);
    xmm6 = _mm_cmpeq_epi8(xmm6, xmm2);

    _mm_xor_si128(xmm5, xmm5);

    xmm0 = _mm_add_epi8(xmm0, xmm1);
    xmm6 = _mm_add_epi8(xmm6, *((__m128i*)excessadd));
    xmm0 = _mm_add_epi8(xmm0, xmm6);

    xmm0 = _mm_sad_epu8(xmm0, xmm5);

    xmm1 = _mm_castps_si128(_mm_movehl_ps(_mm_castsi128_ps(xmm1), _mm_castsi128_ps(xmm0)));
    
    xmm0 = _mm_add_epi8(xmm0, xmm1);

    return (uint32_t)_mm_cvtsi128_si32(xmm0);
}

static void processneighbours1(MotionDetectionDistData *mdd)
{
    uint8_t *properties = mdd->md.blockproperties;
    int32_t j = mdd->md.vblocks;

    do {
        int32_t i = mdd->md.hblocks;

        do {
            if(properties[0] == MOTION_FLAG2) {
                ++mdd->distblocks;
            }
            ++properties;
        } while(--i);

        properties++;
    } while(--j);
}

static void processneighbours2(MotionDetectionDistData *mdd)
{
    uint8_t *properties = mdd->md.blockproperties;
    int32_t j = mdd->md.vblocks;

    do {
        int32_t i = mdd->md.hblocks;

        do {
            if(properties[0] == MOTION_FLAG1) {
                --mdd->distblocks;
                properties[0] = 0;
            } else if(properties[0] == MOTION_FLAG2) { 
                ++mdd->distblocks;
            }
            ++properties;
        } while(--i);

        properties++;
    } while(--j);
}

static void processneighbours3(MotionDetectionDistData *mdd)
{
    uint8_t *properties = mdd->md.blockproperties;
    int32_t j = mdd->md.vblocks;

    do {
        int32_t i = mdd->md.hblocks;

        do {
            if(properties[0] != (MOTION_FLAG1 | MOTION_FLAG2)) {
                if(properties[0] == MOTION_FLAG1) {
                    --mdd->distblocks;
                }
                properties[0] = 0;
            }
            ++properties;
        } while(--i);

        properties++;
    } while(--j);
}

static void markneighbours(MotionDetectionDistData *mdd)
{
    uint8_t *begin = mdd->md.blockproperties;
    uint8_t *end = begin;
    uint32_t *isum2 = mdd->isum;

    int32_t j = mdd->md.vblocks;
    do {
        uint32_t sum = 0;
        int32_t i = mdd->dist;
        do {
            sum += *end++;
        } while(--i);

        i = mdd->dist1;
        do {
            *isum2++ = (sum += *end++);
        } while(--i);

        i = mdd->hint32_terior; 
        do {
            *isum2++ = (sum += *end++ - *begin++);
        } while(--i);

        i = mdd->dist;
        do {
            *isum2++ = (sum -= *begin++);
        } while(--i);

        begin += mdd->dist2;
        end++;
    } while(--j);

    uint32_t *isum1 = isum2 = mdd->isum;
    begin = mdd->md.blockproperties;
    j = mdd->md.hblocks;
    do {
        uint32_t sum = 0;
        int32_t	i = mdd->dist;
        do {
            sum += *isum2;
            isum2 = (uint32_t*)((char*)isum2 + mdd->isumline);
        } while(--i);

        i = mdd->dist1;
        do {
            sum += *isum2;
            isum2 = (uint32_t*)((char*)isum2 + mdd->isumline);
            if (sum > mdd->tolerance) {
                *begin |= MOTION_FLAG2;
            }
            begin += mdd->md.nline;
        } while(--i);

        i = mdd->vint32_terior;
        do {
            sum += *isum2 - *isum1;
            isum2 = (uint32_t*)((char*)isum2 + mdd->isumline);
            isum1 = (uint32_t*)((char*)isum1 + mdd->isumline);
            if(sum > mdd->tolerance) {
                *begin |= MOTION_FLAG2;
            }
            begin += mdd->md.nline;
        } while(--i);

        i = mdd->dist;
        do {
            sum -= *isum1;
            isum1 = (uint32_t*)((char*)isum1 + mdd->isumline);
            if(sum > mdd->tolerance) {
                *begin |= MOTION_FLAG2;
            }
            begin += mdd->md.nline;
        } while(--i);	

        begin += mdd->colinc;
        isum1 = (uint32_t*)((char*)isum1 + mdd->isuminc1);
        isum2 = (uint32_t*)((char*)isum2 + mdd->isuminc2);
    } while(--j);
}

static void markblocks1(MotionDetectionData *md, const uint8_t *p1, int32_t pitch1, const uint8_t *p2, int32_t pitch2)
{
    int32_t inc1 = MOTIONBLOCKHEIGHT * pitch1 - md->linewidth;
    int32_t inc2 = MOTIONBLOCKHEIGHT * pitch2 - md->linewidth;
    uint8_t *properties = md->blockproperties;

    int32_t j = md->vblocks;
    do {
        int32_t i = md->hblocks;

        do {
            properties[0] = 0;

            if(md->blockcompare(p1, pitch1, p2, pitch2, md->noiselevel) >= md->threshold) {
                properties[0] = MOTION_FLAG1;
                ++md->motionblocks;
            }

            p1 += MOTIONBLOCKWIDTH;
            p2 += MOTIONBLOCKWIDTH;
            ++properties;
        } while(--i);

        p1 += inc1;
        p2 += inc2;
        ++properties;
    } while(--j);
}

static void markblocks2(MotionDetectionData *md, const uint8_t *p1, const uint8_t *p2, int32_t pitch)
{
    int32_t inc = MOTIONBLOCKHEIGHT*pitch - md->linewidthSSE2;
    uint8_t *properties = md->blockproperties;

    int32_t j = md->vblocks;
    do {
        int32_t i = md->hblocksSSE2;

        do {
            md->blockcompareSSE2(p1, p2, pitch, md->noiselevel); 
            properties[0] = properties[1] = 0;

            if(blockcompare_result[0] >= md->threshold) {
                properties[0] = MOTION_FLAG1;
                ++md->motionblocks;
            }

            if(blockcompare_result[2] >= md->threshold) {
                properties[1] = MOTION_FLAG1;
                ++md->motionblocks;
            }

            p1 += 2*MOTIONBLOCKWIDTH;
            p2 += 2*MOTIONBLOCKWIDTH;
            properties += 2;
        } while(--i);

        if(md->remainderSSE2) {
            properties[0] = 0;

            if(md->blockcompare(p1, pitch, p2, pitch, md->noiselevel) >= md->threshold) {
                properties[0] = MOTION_FLAG1;
                ++md->motionblocks;
            }
            ++properties;
        }

        p1 += inc;
        p2 += inc;
        ++properties;
    } while(--j);
}

static void markblocks(MotionDetectionDistData *mdd, const uint8_t *p1, int32_t pitch1, const uint8_t *p2, int32_t pitch2)
{
    mdd->md.motionblocks = 0;
    if(((pitch1 - pitch2) | (((uint32_t)p1) & 15) | (((uint32_t)p2) & 15)) == 0) {
        markblocks2(&mdd->md, p1, p2, pitch2);
    } else {
        markblocks1(&mdd->md, p1, pitch1, p2, pitch2);
    }
    mdd->distblocks = 0;
    if(mdd->dist) {
        markneighbours(mdd);
        (mdd->processneighbours)(mdd);
    }
}

static __forceinline void copy8x8(uint8_t *dest, int32_t dpitch, const uint8_t *src, int32_t spitch)
{
    int32_t spitchx2 = spitch + spitch;
    int32_t spitchx3 = spitchx2 + spitch;
    int32_t spitchx4 = spitchx3 + spitch;

    int32_t dpitchx2 = dpitch + dpitch;
    int32_t dpitchx3 = dpitchx2 + dpitch;
    int32_t dpitchx4 = dpitchx3 + dpitch;

    __m64 mm0 = *((__m64*)src);
    __m64 mm1 = *((__m64*)(src+spitch));

    *((uint32_t*)dest) = (uint32_t)_mm_cvtsi64_si32(mm0);
    *((uint32_t*)(dest+dpitch)) = (uint32_t)_mm_cvtsi64_si32(mm1);

    mm0 = *((__m64*)(src+spitchx2));
    mm1 = *((__m64*)(src+spitchx3));

    *((uint32_t*)(dest+dpitchx2)) = (uint32_t)_mm_cvtsi64_si32(mm0);
    *((uint32_t*)(dest+dpitchx3)) = (uint32_t)_mm_cvtsi64_si32(mm1);

    src += spitchx4;
    dest += dpitchx4;

    mm0 = *((__m64*)(src));
    mm1 = *((__m64*)(src+spitch));

    *((uint32_t*)dest) = (uint32_t)_mm_cvtsi64_si32(mm0);
    *((uint32_t*)(dest+dpitch)) = (uint32_t)_mm_cvtsi64_si32(mm1);

    mm0 = *((__m64*)(src+spitchx2));
    mm1 = *((__m64*)(src+spitchx3));

    *((uint32_t*)(dest+dpitchx2)) = (uint32_t)_mm_cvtsi64_si32(mm0);
    *((uint32_t*)(dest+dpitchx3)) = (uint32_t)_mm_cvtsi64_si32(mm1);
}

static __forceinline int32_t vertical_diff(const uint8_t *p, int32_t pitch, const uint8_t *noiselevel)
{
    int32_t pitchx2 = pitch + pitch;
    int32_t pitchx3 = pitchx2 + pitch;
    int32_t pitchx4 = pitchx3 + pitch;
    __m64 mm7 = *((__m64*)noiselevel);

    __m64 mm0, mm1;
    mm0 = _mm_insert_pi16(mm0, *((int32_t*)p), 0);
    mm0 = _mm_insert_pi16(mm0, *((int32_t*)(p+pitchx2)), 1);
    mm1 = _mm_insert_pi16(mm1, *((int32_t*)(p+pitch)), 0);
    mm1 = _mm_insert_pi16(mm1, *((int32_t*)(p+pitchx3)), 1);

    mm7 = _mm_cmpeq_pi8(mm7, mm7);
    
    p += pitchx4;

    mm0 = _mm_insert_pi16(mm0, *((int32_t*)p), 2);
    mm0 = _mm_insert_pi16(mm0, *((int32_t*)(p+pitchx2)), 3);
    mm1 = _mm_insert_pi16(mm1, *((int32_t*)(p+pitch)), 2);
    mm1 = _mm_insert_pi16(mm1, *((int32_t*)(p+pitchx3)), 3);

    __m64 mm2 = mm0;
    __m64 mm3 = mm1;

    mm7 = _mm_srli_pi16(mm7, 8);

    mm0 = _mm_and_si64(mm0, mm7);

    mm1 = _mm_slli_pi16(mm1, 8);
    mm2 = _mm_srli_pi16(mm2, 8);

    mm3 = _mm_subs_pu8(mm3, mm7);

    mm0 = _mm_or_si64(mm0, mm1);
    mm2 = _mm_or_si64(mm2, mm3);

    mm0 = _mm_sad_pu8(mm0, mm2);

    return _mm_cvtsi64_si32(mm0);
}

static __forceinline int32_t horizontal_diff(const uint8_t *p, int32_t pitch)
{
    __m64 mm0 = *((__m64*)p);
    mm0 = _mm_sad_pu8(mm0, *((__m64*)(p+pitch)));
    return _mm_cvtsi64_si32(mm0);
}

static void postprocessing_grey(PostProcessingData *pp, uint8_t *dp, int32_t dpitch, const uint8_t *sp, int32_t spitch)
{
    int32_t bottomdp = 7 * dpitch;
    int32_t bottomsp = 7 * spitch;
    int32_t dinc = MOTIONBLOCKHEIGHT * dpitch - pp->mdd.md.linewidth;
    int32_t sinc = MOTIONBLOCKHEIGHT * spitch - pp->mdd.md.linewidth;

    pp->loops = pp->restored_blocks = 0;

    int32_t to_restore;

    do {
        uint8_t *dp2 = dp;
        const uint8_t *sp2 = sp;
        uint8_t *cl = pp->mdd.md.blockproperties;
        ++pp->loops;
        to_restore = 0;

        int32_t i = pp->mdd.md.vblocks;
        do
        {
            int32_t j = pp->mdd.md.hblocks;

            do {
                if((cl[0] & TO_CLEAN) != 0) {
                    copy8x8(dp2, dpitch, sp2, spitch);
                    cl[0] &= ~TO_CLEAN;

                    if(cl[-1] == 0) {
                        if(vertical_diff(dp2 + leftdp, dpitch, pp->mdd.md.noiselevel) > vertical_diff(sp2 + leftsp, spitch, pp->mdd.md.noiselevel) + pp->pthreshold) {
                            ++to_restore;
                            cl[-1] = MOTION_FLAG3;
                        }
                    }

                    if(cl[1] == 0) {
                        if(vertical_diff(dp2 + rightdp, dpitch, pp->mdd.md.noiselevel) > vertical_diff(sp2 + rightsp, spitch, pp->mdd.md.noiselevel) + pp->pthreshold) {
                            ++to_restore;
                            cl[1] = MOTION_FLAG3;
                        }
                    }

                    if(cl[pp->mdd.md.pline] == 0) {
                        if(horizontal_diff(dp2 + topdp, dpitch) > horizontal_diff(sp2 + topsp, spitch) + pp->pthreshold) {
                            ++to_restore;
                            cl[pp->mdd.md.pline] = MOTION_FLAG3;
                        }
                    }

                    if(cl[pp->mdd.md.nline] == 0) {
                        if(horizontal_diff(dp2 + bottomdp, dpitch) > horizontal_diff(sp2 + bottomsp, spitch) + pp->pthreshold) {
                            ++to_restore;
                            cl[pp->mdd.md.nline] = MOTION_FLAG3;
                        }
                    }
                }

                ++cl;
                dp2 += rightbldp;
                sp2 += rightblsp;
            } while(--j);

            cl++;
            dp2 += dinc;
            sp2 += sinc;
        } while(--i);

        pp->restored_blocks += to_restore;
    } while(to_restore != 0);
}

static __forceinline int32_t horizontal_diff_chroma(const uint8_t *u, const uint8_t *v, int32_t pitch)
{
    __m64 mm0 = *((__m64*)u);
    __m64 mm1 = _mm_cvtsi32_si64(*((int32_t*)(u+pitch)));

    mm0 = _mm_unpacklo_pi32(mm0, *((__m64*)v));
    mm1 = _mm_unpacklo_pi32(mm1, *((__m64*)(v+pitch)));

    mm0 = _mm_sad_pu8(mm0, mm1);

    return _mm_cvtsi64_si32(mm0);
}

static __forceinline int32_t vertical_diff_yv12_chroma(const uint8_t *u, const uint8_t *v, int32_t pitch, const uint8_t *noiselevel)
{
    __m64 mm7 = *((__m64*)noiselevel);
    int32_t pitchx2 = pitch + pitch;
    int32_t pitchx3 = pitchx2 + pitch;

    __m64 mm0, mm1;
    mm0 = _mm_insert_pi16(mm0, *((int32_t*)u), 0);
    mm0 = _mm_insert_pi16(mm0, *((int32_t*)(u+pitchx2)), 1);
    mm1 = _mm_insert_pi16(mm1, *((int32_t*)(u+pitch)), 0);
    mm1 = _mm_insert_pi16(mm1, *((int32_t*)(u+pitchx3)), 1);
    mm7 = _mm_cmpeq_pi8(mm7, mm7);

    mm0 = _mm_insert_pi16(mm0, *((int32_t*)v), 2);
    mm0 = _mm_insert_pi16(mm0, *((int32_t*)(v+pitchx2)), 3);
    mm1 = _mm_insert_pi16(mm1, *((int32_t*)v+pitch), 2);
    mm1 = _mm_insert_pi16(mm1, *((int32_t*)(v+pitchx3)), 3);
    __m64 mm2 = mm0;
    __m64 mm3 = mm1;

    mm7 = _mm_srli_pi16(mm7, 8);

    mm0 = _mm_and_si64(mm0, mm7);

    mm1 = _mm_slli_pi16(mm1, 8);
    mm2 = _mm_srli_pi16(mm2, 8);

    mm3 = _mm_subs_pu8(mm3, mm7);

    mm0 = _mm_or_si64(mm0, mm1);
    mm2 = _mm_or_si64(mm2, mm3);

    mm0 = _mm_sad_pu8(mm0, mm2);

    return (uint32_t)_mm_cvtsi64_si32(mm0);
}

static __forceinline int32_t vertical_diff_yuy2_chroma(const uint8_t *u, const uint8_t *v, int32_t pitch, const uint8_t *noiselevel)
{
    __m64 mm7 = *((__m64*)noiselevel);
    int32_t pitchx2 = pitch + pitch;
    int32_t pitchx3 = pitchx2 + pitch;
    int32_t pitchx4 = pitchx3 + pitch;

    __m64 mm0, mm1;
    mm0 = _mm_insert_pi16(mm0, *((int32_t*)u), 0);
    mm0 = _mm_insert_pi16(mm0, *((int32_t*)(u+pitchx2)), 1);
    mm1 = _mm_insert_pi16(mm1, *((int32_t*)(u+pitch)), 0);
    mm1 = _mm_insert_pi16(mm1, *((int32_t*)(u+pitchx3)), 1);

    u += pitchx4;

    mm0 = _mm_insert_pi16(mm0, *((int32_t*)u), 2);
    mm0 = _mm_insert_pi16(mm0, *((int32_t*)(u+pitchx2)), 3);
    mm1 = _mm_insert_pi16(mm1, *((int32_t*)(u+pitch)), 2);
    mm1 = _mm_insert_pi16(mm1, *((int32_t*)(u+pitchx3)), 3);
    __m64 mm2 = mm0;
    __m64 mm3 = mm1;

    mm0 = _mm_and_si64(mm0, mm7);

    mm1 = _mm_slli_pi16(mm1, 8);
    mm2 = _mm_srli_pi16(mm2, 8);

    mm3 = _mm_subs_pu8(mm3, mm7);

    mm0 = _mm_or_si64(mm0, mm1);
    mm2 = _mm_or_si64(mm2, mm3);

    __m64 mm4;
    mm4 = _mm_insert_pi16(mm4, *((int32_t*)v), 0);
    mm0 = _mm_insert_pi16(mm0, *((int32_t*)(v+pitchx2)), 1);
    mm1 = _mm_insert_pi16(mm1, *((int32_t*)(v+pitch)), 0);
    mm1 = _mm_insert_pi16(mm1, *((int32_t*)(v+pitchx3)), 1);

    v += pitchx4;

    mm4 = _mm_insert_pi16(mm4, *((int32_t*)v), 2);
    mm4 = _mm_insert_pi16(mm4, *((int32_t*)(v+pitchx2)), 3);
    mm1 = _mm_insert_pi16(mm1, *((int32_t*)(v+pitch)), 2);
    mm1 = _mm_insert_pi16(mm1, *((int32_t*)(v+pitchx3)), 3);

    __m64 mm6 = mm4;
    mm3 = mm1;

    mm4 = _mm_and_si64(mm4, mm7);

    mm1 = _mm_slli_pi16(mm1, 8);
    mm6 = _mm_srli_pi16(mm6, 8);

    mm3 = _mm_subs_pu8(mm3, mm7);

    mm4 = _mm_or_si64(mm4, mm1);
    mm6 = _mm_or_si64(mm6, mm3);

    mm0 = _mm_sad_pu8(mm0, mm2);
    mm4 = _mm_sad_pu8(mm4, mm6);

    mm0 = _mm_avg_pu16(mm0, mm4);

    return _mm_cvtsi64_si32(mm0);
}

static __forceinline void colorise(uint8_t *u, uint8_t *v, int32_t pitch, int32_t height, uint32_t ucolor, uint32_t vcolor)
{
    int32_t i = height;

    do {
        *(uint32_t*)u = ucolor;
        *(uint32_t*)v = vcolor;
        u += pitch;
        v += pitch;
    } while(--i);
}

static void postprocessing(PostProcessingData *pp, uint8_t *dp, int32_t dpitch, uint8_t *dpU, uint8_t *dpV, int32_t dpitchUV, const uint8_t *sp, int32_t spitch, const uint8_t *spU, const uint8_t *spV, int32_t spitchUV, int32_t rowsize, int32_t height)
{
    int32_t bottomdp = 7 * dpitch;
    int32_t bottomsp = 7 * spitch;
    int32_t Cbottomdp = pp->chromaheightm * dpitchUV;
    int32_t Cbottomsp = pp->chromaheightm * spitchUV;
    int32_t dinc = MOTIONBLOCKHEIGHT * dpitch - pp->mdd.md.linewidth;
    int32_t sinc = MOTIONBLOCKHEIGHT * spitch - pp->mdd.md.linewidth;
    int32_t dincUV = pp->chromaheight * dpitchUV - pp->linewidthUV;
    int32_t sincUV = pp->chromaheight * spitchUV - pp->linewidthUV;

    pp->loops = pp->restored_blocks = 0;

    int32_t to_restore;

    do {
        uint8_t *dp2 = dp;
        uint8_t *dpU2 = dpU;
        uint8_t *dpV2 = dpV;
        const uint8_t *sp2 = sp;
        const uint8_t *spU2 = spU;
        const uint8_t *spV2 = spV;
        uint8_t *cl = pp->mdd.md.blockproperties;
        ++pp->loops;
        to_restore = 0; 

        int32_t i = pp->mdd.md.vblocks;

        do {
            int32_t j = pp->mdd.md.hblocks;

            do {
                if((cl[0] & TO_CLEAN) != 0) {
                    copy8x8(dp2, dpitch, sp2, spitch);
                    // copy chroma planes
                    vs_bitblt(dpU2, dpitchUV, spU2, spitchUV, rowsize, height);
                    vs_bitblt(dpV2, dpitchUV, spV2, spitchUV, rowsize, height);
                    cl[0] &= ~TO_CLEAN;

                    if(cl[-1] == 0) {
                        if((vertical_diff(dp2 + leftdp, dpitch, pp->mdd.md.noiselevel) > vertical_diff(sp2 + leftsp, spitch, pp->mdd.md.noiselevel) + pp->pthreshold)
                            || (pp->vertical_diff_chroma(dpU2 + Cleftdp, dpV2 + Cleftdp, dpitchUV, pp->mdd.md.noiselevel) > pp->vertical_diff_chroma(spU2 + Cleftsp, spV2 + Cleftsp, spitchUV, pp->mdd.md.noiselevel) + pp->cthreshold)) {
                                ++to_restore;
                                cl[-1] = MOTION_FLAG3;
                        }
                    }

                    if(cl[1] == 0) {
                        if((vertical_diff(dp2 + rightdp, dpitch, pp->mdd.md.noiselevel) > vertical_diff(sp2 + rightsp, spitch, pp->mdd.md.noiselevel) + pp->pthreshold)
                            || (pp->vertical_diff_chroma(dpU2 + Crightdp, dpV2 + Crightdp, dpitchUV, pp->mdd.md.noiselevel) > pp->vertical_diff_chroma(spU2 + Crightsp, spV2 + Crightsp, spitchUV, pp->mdd.md.noiselevel) + pp->cthreshold)) {
                                ++to_restore;
                                cl[1] = MOTION_FLAG3;
                        }
                    }

                    if(cl[pp->mdd.md.pline] == 0) {
                        if((horizontal_diff(dp2 + topdp, dpitch) > horizontal_diff(sp2 + topsp, spitch) + pp->pthreshold)
                            || (horizontal_diff_chroma(dpU2 + Ctopdp, dpV2 + Ctopdp, dpitchUV) > horizontal_diff_chroma(spU2 + Ctopsp, spV2 + Ctopsp, spitchUV) + pp->cthreshold)) {
                                ++to_restore;
                                cl[pp->mdd.md.pline] = MOTION_FLAG3;
                        }
                    }

                    if(cl[pp->mdd.md.nline] == 0) {
                        if((horizontal_diff(dp2 + bottomdp, dpitch) > horizontal_diff(sp2 + bottomsp, spitch) + pp->pthreshold)
                            || (horizontal_diff_chroma(dpU2 + Cbottomdp, dpV2 + Cbottomdp, dpitchUV) > horizontal_diff_chroma(spU2 + Cbottomsp, spV2 + Cbottomsp, spitchUV) + pp->cthreshold)) {
                                ++to_restore;
                                cl[pp->mdd.md.nline] = MOTION_FLAG3;
                        }
                    }
                }

                ++cl;
                dp2 += rightbldp;
                sp2 += rightblsp;
                dpU2 += Crightbldp;
                spU2 += Crightbldp;
                dpV2 += Crightbldp;
                spV2 += Crightbldp;
            } while(--j);

            cl++;
            dp2 += dinc;
            sp2 += sinc;
            dpU2 += dincUV;
            spU2 += sincUV;
            dpV2 += dincUV;
            spV2 += sincUV;
        } while(--i);

        pp->restored_blocks += to_restore;
    } while(to_restore != 0);
}

static void show_motion(PostProcessingData *pp, uint8_t *u, uint8_t *v, int32_t pitchUV)
{
    int32_t inc = pp->chromaheight * pitchUV - pp->linewidthUV;

    uint8_t *properties = pp->mdd.md.blockproperties;

    int32_t j = pp->mdd.md.vblocks;

    do {
        int32_t i = pp->mdd.md.hblocks;

        do {
            if(properties[0]) {
                uint32_t u_color = u_ncolor; 
                uint32_t v_color = v_ncolor;
                if((properties[0] & MOTION_FLAG) != 0) {
                    u_color = u_mcolor;
                    v_color = v_mcolor;
                }
                if((properties[0] & MOTION_FLAGP) != 0) {
                    u_color = u_pcolor;
                    v_color = v_pcolor;
                }
                colorise(u, v, pitchUV, pp->chromaheight, u_color, v_color);
            }

            u += MOTIONBLOCKWIDTH / 2;
            v += MOTIONBLOCKWIDTH / 2;
            ++properties;
        } while(--i);

        u += inc;
        v += inc;
        ++properties;
    } while(--j);
}

static void FillMotionDetection(MotionDetectionData *md, const VSMap *in, VSMap *out, const VSAPI *vsapi, const VSVideoInfo *vi)
{
    int32_t err;    
    int32_t noise = (int32_t) vsapi->propGetInt(in, "noise", 0, &err);
    if (err) {
        noise = 0;
    }

    int32_t noisy = (int32_t) vsapi->propGetInt(in, "noisy", 0, &err);
    if (err) {
        noisy = -1;
    }

    int32_t width = vi->width;
    int32_t height = vi->height;

    md->hblocks = (md->linewidth = width) / MOTIONBLOCKWIDTH;
    md->vblocks = height / MOTIONBLOCKHEIGHT;

    md->linewidthSSE2 = md->linewidth;
    md->hblocksSSE2 = md->hblocks / 2;
    if((md->remainderSSE2 = (md->hblocks & 1)) != 0) {
        md->linewidthSSE2 -= MOTIONBLOCKWIDTH;
    }
    if((md->hblocksSSE2 == 0) || (md->vblocks == 0)) {
        vsapi->setError(out, "RemoveDirt: width or height of the clip too small");
    }

    md->blockcompareSSE2 = SADcompareSSE2;
    md->blockcompare = SADcompare;

    if(noise > 0) {
        md->blockcompareSSE2 = NSADcompareSSE2;
        md->blockcompare = NSADcompare;
        memset(md->noiselevel, noise, 16);

        if(noisy >= 0) {
            md->blockcompareSSE2 = ExcessPixelsSSE2;
            md->blockcompare = ExcessPixels;
            md->threshold = noisy;
        }
    }
    int32_t size;
    md->blockproperties_addr = new uint8_t[size = (md->nline = md->hblocks + 1) * (md->vblocks + 2)];
    md->blockproperties = md->blockproperties_addr  + md->nline;
    md->pline = -md->nline;
    memset(md->blockproperties_addr, BMARGIN, size);
}

static void FillMotionDetectionDist(MotionDetectionDistData *mdd, const VSMap *in, VSMap *out, const VSAPI *vsapi, const VSVideoInfo *vi)
{
    FillMotionDetection(&mdd->md, in, out, vsapi, vi);

    int32_t err;
    uint32_t dmode = (int32_t) vsapi->propGetInt(in, "dmode", 0, &err);
    if (err) {
        dmode = 0;
    }

    uint32_t tolerance = (int32_t) vsapi->propGetInt(in, "", 0, &err);
    if (err) {
        tolerance = 12;
    }

    int32_t dist = (int32_t) vsapi->propGetInt(in, "dist", 0, &err);
    if (err) {
        dist = 1;
    }

    static void (*neighbourproc[3])(MotionDetectionDistData*) = { processneighbours1,
                                                                  processneighbours2,
                                                                  processneighbours3 };

    mdd->blocks = mdd->md.hblocks * mdd->md.vblocks;
    mdd->isum = new uint32_t[mdd->blocks];

    if(dmode >= 3) {
        dmode = 0;
    }

    if(tolerance > 100) {
        tolerance = 100;
    } else if(tolerance == 0) {
        if(dmode == 2) {
            dist = 0;
        }
        dmode = 0;
    }

    mdd->processneighbours = neighbourproc[dmode];

    mdd->dist = dist;
    mdd->dist1 = mdd->dist + 1;
    mdd->dist2 = mdd->dist1 + 1;

    mdd->isumline = mdd->md.hblocks * sizeof(uint32_t);
    uint32_t d = mdd->dist1 + mdd->dist;
    tolerance = (d * d * tolerance * MOTION_FLAG1) / 100;
    mdd->hint32_terior = mdd->md.hblocks - d;
    mdd->vint32_terior = mdd->md.vblocks - d;
    mdd->colinc = 1 - (mdd->md.vblocks * mdd->md.nline);
    mdd->isuminc1 = (1 - (mdd->vint32_terior + dist) * mdd->md.hblocks) * sizeof(uint32_t);
    mdd->isuminc2 = (1 - mdd->md.vblocks * mdd->md.hblocks) * sizeof(uint32_t);
}

static void FillPostProcessing(PostProcessingData *pp, const VSMap *in, VSMap *out, const VSAPI *vsapi, const VSVideoInfo *vi)
{
    int32_t err;
    pp->pthreshold = (int32_t) vsapi->propGetInt(in, "pthreshold", 0, &err);
    if (err) {
        pp->pthreshold = 10;
    }

    pp->cthreshold = (int32_t) vsapi->propGetInt(in, "cthreshold", 0, &err);
    if (err) {
        pp->cthreshold = 10;
    }

    FillMotionDetectionDist(&pp->mdd, in, out, vsapi, vi);

    pp->vertical_diff_chroma = vertical_diff_yv12_chroma;
    pp->linewidthUV = pp->mdd.md.linewidth / 2;
    pp->chromaheight = MOTIONBLOCKHEIGHT / 2;

    if(vi->format->id == pfYUV422P8)
    {
        pp->chromaheight *= 2;
        pp->vertical_diff_chroma = vertical_diff_yuy2_chroma;
    }
    pp->chromaheightm = pp->chromaheight - 1;
}

void FillRemoveDirt(RemoveDirtData *rd, const VSMap *in, VSMap *out, const VSAPI *vsapi, const VSVideoInfo *vi)
{
    int32_t err;
    rd->grey = vsapi->propGetInt(in, "grey", 0, &err) == 1;
    if (err) {
        rd->grey = false;
    }

    rd->show = vsapi->propGetInt(in, "show", 0, &err) == 1;
    if (err) {
        rd->show = false;
    }

    FillPostProcessing(&rd->pp, in, out, vsapi, vi);
}

int32_t RemoveDirtProcessFrame(RemoveDirtData *rd, VSFrameRef *dest, const VSFrameRef *src, const VSFrameRef *previous, const VSFrameRef *next, int32_t frame, const VSAPI *vsapi, const VSVideoInfo *vi)
{
    const uint8_t *nextY = vsapi->getReadPtr(next, 0);
    int32_t nextPitchY = vsapi->getStride(next, 0);
    markblocks(&rd->pp.mdd, vsapi->getReadPtr(previous, 0), vsapi->getStride(previous, 0) , nextY, nextPitchY);

    uint8_t *destY = vsapi->getWritePtr(dest, 0);
    uint8_t *destU = vsapi->getWritePtr(dest, 1);
    uint8_t *destV = vsapi->getWritePtr(dest, 2);
    int32_t destPitchY = vsapi->getStride(dest, 0);
    int32_t destPitchUV = vsapi->getStride(dest, 1);
    const uint8_t *srcY = vsapi->getReadPtr(src, 0);
    const uint8_t *srcU = vsapi->getReadPtr(src, 1);
    const uint8_t *srcV = vsapi->getReadPtr(src, 2);
    int32_t srcPitchY = vsapi->getStride(src, 0);
    int32_t srcPitchUV = vsapi->getStride(src, 1);
    int32_t chroma_rowsize = vi->width >> vi->format->subSamplingW;
    int32_t chroma_height = vi->height >> vi->format->subSamplingH;

    if(rd->grey) {
        postprocessing_grey(&rd->pp, destY, destPitchY, srcY, srcPitchY);
    } else {
        postprocessing(&rd->pp, destY, destPitchY, destU, destV, destPitchUV, srcY, srcPitchY, srcU, srcV, srcPitchUV, chroma_rowsize, chroma_height);
    }

    if(rd->show) {
        show_motion(&rd->pp, destU, destV, destPitchUV);
    }

    return rd->pp.restored_blocks + rd->pp.mdd.distblocks + rd->pp.mdd.md.motionblocks;
}
