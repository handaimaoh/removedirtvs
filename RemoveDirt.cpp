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

__declspec(align(16)) uint32_t blockcompare_result[4];

static void __stdcall SADcompareSSE2(const uint8_t *p1, const uint8_t *p2, int32_t pitch)
{
    __asm	mov			edx,				pitch
    __asm	mov			eax,				p1
    __asm	lea			ecx,				[edx + 2*edx]
    __asm	mov			ebx,				p2
    __asm	movdqa		xmm0,				[eax]
    __asm	movdqa		xmm1,				[eax + edx]
    __asm	psadbw		xmm0,				[ebx]
    __asm	psadbw		xmm1,				[ebx + edx]
    __asm	movdqa		xmm2,				[eax + 2*edx]
    __asm	movdqa		xmm3,				[eax + ecx]
    __asm	psadbw		xmm2,				[ebx + 2*edx]
    __asm	lea			eax,				[eax + 4*edx]
    __asm	psadbw		xmm3,				[ebx + ecx]
    __asm	paddd		xmm0,				xmm2
    __asm	paddd		xmm1,				xmm3
    __asm	lea			ebx,				[ebx + 4*edx]
    __asm	movdqa		xmm2,				[eax]
    __asm	movdqa		xmm3,				[eax + edx]
    __asm	psadbw		xmm2,				[ebx]
    __asm	psadbw		xmm3,				[ebx + edx]
    __asm	paddd		xmm0,				xmm2
    __asm	paddd		xmm1,				xmm3
    __asm	movdqa		xmm2,				[eax + 2*edx]
    __asm	movdqa		xmm3,				[eax + ecx]
    __asm	psadbw		xmm2,				[ebx + 2*edx]
    __asm	psadbw		xmm3,				[ebx + ecx]
    __asm	paddd		xmm0,				xmm2
    __asm	paddd		xmm1,				xmm3
    __asm	paddd		xmm0,				xmm1
    __asm	movdqa		blockcompare_result,xmm0
}

// xmm7 contains already the noise level!
static void __stdcall NSADcompareSSE2(const uint8_t *p1, const uint8_t *p2, int32_t pitch)
{
    __asm	mov			edx,				pitch
    __asm	mov			eax,				p1
    __asm	lea			ecx,				[edx + 2*edx]
    __asm	mov			ebx,				p2
    __asm	movdqa		xmm0,				[eax]
    __asm	movdqa		xmm2,				[eax + edx]
    __asm	movdqa		xmm3,				xmm0
    __asm	movdqa		xmm4,				xmm2
    __asm	movdqa		xmm5,				[ebx]
    __asm	movdqa		xmm6,				[ebx + edx]
    __asm	psubusb		xmm0,				xmm5
    __asm	psubusb		xmm2,				xmm6
    __asm	psubusb		xmm5,				xmm3
    __asm	psubusb		xmm6,				xmm4
    __asm	psubusb		xmm0,				xmm7
    __asm	psubusb		xmm2,				xmm7
    __asm	psubusb		xmm5,				xmm7
    __asm	psubusb		xmm6,				xmm7
    __asm	psadbw		xmm0,				xmm5
    __asm	psadbw		xmm6,				xmm2
    __asm	movdqa		xmm1,				[eax + 2*edx]
    __asm	paddd		xmm0,				xmm6
    __asm	movdqa		xmm2,				[eax + ecx]

    __asm	movdqa		xmm3,				xmm1
    __asm	movdqa		xmm4,				xmm2
    __asm	movdqa		xmm5,				[ebx + 2*edx]
    __asm	movdqa		xmm6,				[ebx + ecx]
    __asm	psubusb		xmm1,				xmm5
    __asm	psubusb		xmm2,				xmm6
    __asm	lea			eax,				[eax + 4*edx]
    __asm	psubusb		xmm5,				xmm3
    __asm	psubusb		xmm6,				xmm4
    __asm	psubusb		xmm1,				xmm7
    __asm	psubusb		xmm2,				xmm7
    __asm	lea			ebx,				[ebx + 4*edx]
    __asm	psubusb		xmm5,				xmm7
    __asm	psubusb		xmm6,				xmm7
    __asm	psadbw		xmm5,				xmm1
    __asm	psadbw		xmm6,				xmm2
    __asm	paddd		xmm0,				xmm5
    __asm	movdqa		xmm1,				[eax]
    __asm	paddd		xmm0,				xmm6
    __asm	movdqa		xmm2,				[eax + edx]

    __asm	movdqa		xmm3,				xmm1
    __asm	movdqa		xmm4,				xmm2
    __asm	movdqa		xmm5,				[ebx]
    __asm	movdqa		xmm6,				[ebx + edx]
    __asm	psubusb		xmm1,				xmm5
    __asm	psubusb		xmm2,				xmm6
    __asm	psubusb		xmm5,				xmm3
    __asm	psubusb		xmm6,				xmm4
    __asm	psubusb		xmm1,				xmm7
    __asm	psubusb		xmm2,				xmm7
    __asm	psubusb		xmm5,				xmm7
    __asm	psubusb		xmm6,				xmm7
    __asm	psadbw		xmm5,				xmm1
    __asm	psadbw		xmm6,				xmm2
    __asm	paddd		xmm0,				xmm5
    __asm	movdqa		xmm1,				[eax + 2*edx]
    __asm	paddd		xmm0,				xmm6
    __asm	movdqa		xmm2,				[eax + ecx]

    __asm	movdqa		xmm3,				xmm1
    __asm	movdqa		xmm4,				xmm2
    __asm	movdqa		xmm5,				[ebx + 2*edx]
    __asm	movdqa		xmm6,				[ebx + ecx]
    __asm	psubusb		xmm1,				xmm5
    __asm	psubusb		xmm2,				xmm6
    __asm	psubusb		xmm5,				xmm3
    __asm	psubusb		xmm6,				xmm4
    __asm	psubusb		xmm1,				xmm7
    __asm	psubusb		xmm2,				xmm7
    __asm	psubusb		xmm5,				xmm7
    __asm	psubusb		xmm6,				xmm7
    __asm	psadbw		xmm5,				xmm1
    __asm	psadbw		xmm6,				xmm2
    __asm	paddd		xmm0,				xmm5
    __asm	paddd		xmm0,				xmm6

    __asm	movdqa		blockcompare_result,xmm0
}

static const __declspec(align(16)) uint8_t excessaddSSE2[16] = { 8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8 };

static void __stdcall ExcessPixelsSSE2(const uint8_t *p1, const uint8_t *p2, int32_t pitch)
{
    __asm	mov			edx,				pitch
    __asm	mov			eax,				p1
    __asm	lea			ecx,				[edx + 2*edx]
    __asm	mov			ebx,				p2
    __asm	movdqa		xmm0,				[eax]
    __asm	movdqa		xmm2,				[eax + edx]
    __asm	movdqa		xmm3,				xmm0
    __asm	movdqa		xmm4,				xmm2
    __asm	movdqa		xmm5,				[ebx]
    __asm	movdqa		xmm6,				[ebx + edx]
    __asm	psubusb		xmm0,				xmm5
    __asm	psubusb		xmm2,				xmm6
    __asm	psubusb		xmm5,				xmm3
    __asm	psubusb		xmm6,				xmm4
    __asm	psubusb		xmm0,				xmm7
    __asm	psubusb		xmm2,				xmm7
    __asm	psubusb		xmm5,				xmm7
    __asm	psubusb		xmm6,				xmm7
    __asm	pcmpeqb		xmm0,				xmm5
    __asm	pcmpeqb		xmm6,				xmm2
    __asm	movdqa		xmm1,				[eax + 2*edx]
    __asm	paddb		xmm0,				xmm6
    __asm	movdqa		xmm2,				[eax + ecx]

    __asm	movdqa		xmm3,				xmm1
    __asm	movdqa		xmm4,				xmm2
    __asm	movdqa		xmm5,				[ebx + 2*edx]
    __asm	movdqa		xmm6,				[ebx + ecx]
    __asm	psubusb		xmm1,				xmm5
    __asm	psubusb		xmm2,				xmm6
    __asm	lea			eax,				[eax + 4*edx]
    __asm	psubusb		xmm5,				xmm3
    __asm	psubusb		xmm6,				xmm4
    __asm	psubusb		xmm1,				xmm7
    __asm	psubusb		xmm2,				xmm7
    __asm	lea			ebx,				[ebx + 4*edx]
    __asm	psubusb		xmm5,				xmm7
    __asm	psubusb		xmm6,				xmm7
    __asm	pcmpeqb		xmm5,				xmm1
    __asm	pcmpeqb		xmm6,				xmm2
    __asm	paddb		xmm0,				xmm5
    __asm	movdqa		xmm1,				[eax]
    __asm	paddb		xmm0,				xmm6
    __asm	movdqa		xmm2,				[eax + edx]

    __asm	movdqa		xmm3,				xmm1
    __asm	movdqa		xmm4,				xmm2
    __asm	movdqa		xmm5,				[ebx]
    __asm	movdqa		xmm6,				[ebx + edx]
    __asm	psubusb		xmm1,				xmm5
    __asm	psubusb		xmm2,				xmm6
    __asm	psubusb		xmm5,				xmm3
    __asm	psubusb		xmm6,				xmm4
    __asm	psubusb		xmm1,				xmm7
    __asm	psubusb		xmm2,				xmm7
    __asm	psubusb		xmm5,				xmm7
    __asm	psubusb		xmm6,				xmm7
    __asm	pcmpeqb		xmm5,				xmm1
    __asm	pcmpeqb		xmm6,				xmm2
    __asm	paddb		xmm0,				xmm5
    __asm	movdqa		xmm1,				[eax + 2*edx]
    __asm	paddb		xmm0,				xmm6
    __asm	movdqa		xmm2,				[eax + ecx]

    __asm	movdqa		xmm3,				xmm1
    __asm	movdqa		xmm4,				xmm2
    __asm	movdqa		xmm5,				[ebx + 2*edx]
    __asm	movdqa		xmm6,				[ebx + ecx]
    __asm	psubusb		xmm1,				xmm5
    __asm	psubusb		xmm2,				xmm6
    __asm	psubusb		xmm5,				xmm3
    __asm	psubusb		xmm6,				xmm4
    __asm	psubusb		xmm1,				xmm7
    __asm	psubusb		xmm2,				xmm7
    __asm	psubusb		xmm5,				xmm7
    __asm	psubusb		xmm6,				xmm7
    __asm	pcmpeqb		xmm5,				xmm1
    __asm	pcmpeqb		xmm6,				xmm2
    __asm	paddb		xmm0,				xmm5
    __asm	pxor		xmm1,				xmm1
    __asm	paddb		xmm0,				xmm6
    __asm	paddb		xmm0,				excessaddSSE2
    __asm	psadbw		xmm0,				xmm1
    __asm	movdqa		blockcompare_result,xmm0
}

static uint32_t __stdcall SADcompare(const uint8_t *p1, int32_t pitch1, const uint8_t *p2, int32_t pitch2)
{
    __asm	mov			edx,				pitch1
    __asm	mov			esi,				pitch2
    __asm	mov			eax,				p1
    __asm	lea			ecx,				[edx + 2*edx]
    __asm	lea			edi,				[esi + 2*esi]
    __asm	mov			ebx,				p2
    __asm	movq		mm0,				[eax]
    __asm	movq		mm1,				[eax + edx]
    __asm	psadbw		mm0,				[ebx]
    __asm	psadbw		mm1,				[ebx + esi]
    __asm	movq		mm2,				[eax + 2*edx]
    __asm	movq		mm3,				[eax + ecx]
    __asm	psadbw		mm2,				[ebx + 2*esi]
    __asm	lea			eax,				[eax + 4*edx]
    __asm	psadbw		mm3,				[ebx + edi]
    __asm	paddd		mm0,				mm2
    __asm	paddd		mm1,				mm3
    __asm	lea			ebx,				[ebx + 4*esi]
    __asm	movq		mm2,				[eax]
    __asm	movq		mm3,				[eax + edx]
    __asm	psadbw		mm2,				[ebx]
    __asm	psadbw		mm3,				[ebx + esi]
    __asm	paddd		mm0,				mm2
    __asm	paddd		mm1,				mm3
    __asm	movq		mm2,				[eax + 2*edx]
    __asm	movq		mm3,				[eax + ecx]
    __asm	psadbw		mm2,				[ebx + 2*esi]
    __asm	psadbw		mm3,				[ebx + edi]
    __asm	paddd		mm0,				mm2
    __asm	paddd		mm1,				mm3
    __asm	paddd		mm0,				mm1
    __asm	movd		eax,				mm0
}

// mm7 contains already the noise level!
static uint32_t __stdcall NSADcompare(const uint8_t *p1, int32_t pitch1, const uint8_t *p2, int32_t pitch2)
{
    __asm	mov			edx,				pitch1
    __asm	mov			esi,				pitch2
    __asm	mov			eax,				p1
    __asm	lea			ecx,				[edx + 2*edx]
    __asm	lea			edi,				[esi + 2*esi]
    __asm	mov			ebx,				p2
    __asm	movq		xmm0,				QWORD PTR[eax]
    __asm	movq		xmm2,				QWORD PTR[eax + edx]
    __asm	movhps		xmm0,				[eax + 2*edx]
    __asm	movhps		xmm2,				[eax + ecx]
    __asm	movdqa		xmm3,				xmm0
    __asm	movdqa		xmm4,				xmm2
    __asm	movq		xmm5,				QWORD PTR[ebx]
    __asm	movq		xmm6,				QWORD PTR[ebx + esi]
    __asm	lea			eax,				[eax + 4*edx]
    __asm	movhps		xmm5,				[ebx + 2*esi]
    __asm	movhps		xmm6,				[ebx + edi]
    __asm	psubusb		xmm0,				xmm5
    __asm	psubusb		xmm2,				xmm6
    __asm	psubusb		xmm5,				xmm3
    __asm	psubusb		xmm6,				xmm4
    __asm	psubusb		xmm0,				xmm7
    __asm	psubusb		xmm2,				xmm7
    __asm	lea			ebx,				[ebx + 4*esi]
    __asm	psubusb		xmm5,				xmm7
    __asm	psubusb		xmm6,				xmm7
    __asm	psadbw		xmm0,				xmm5
    __asm	psadbw		xmm6,				xmm2
    __asm	movq		xmm1,				QWORD PTR[eax]
    __asm	paddd		xmm0,				xmm6
    __asm	movq		xmm2,				QWORD PTR[eax + edx]
    __asm	movhps		xmm1,				[eax + 2*edx]
    __asm	movhps		xmm2,				[eax + ecx]
    __asm	movdqa		xmm3,				xmm1
    __asm	movdqa		xmm4,				xmm2
    __asm	movq		xmm5,				QWORD PTR[ebx]
    __asm	movq		xmm6,				QWORD PTR[ebx + esi]
    __asm	movhps		xmm5,				[ebx + 2*esi]
    __asm	movhps		xmm6,				[ebx + edi]
    __asm	psubusb		xmm1,				xmm5
    __asm	psubusb		xmm2,				xmm6
    __asm	psubusb		xmm5,				xmm3
    __asm	psubusb		xmm6,				xmm4
    __asm	psubusb		xmm1,				xmm7
    __asm	psubusb		xmm2,				xmm7
    __asm	psubusb		xmm5,				xmm7
    __asm	psubusb		xmm6,				xmm7
    __asm	psadbw		xmm1,				xmm5
    __asm	psadbw		xmm6,				xmm2
    __asm	paddd		xmm0,				xmm1
    __asm	paddd		xmm0,				xmm6
    __asm	movhlps		xmm1,				xmm0
    __asm	paddd		xmm0,				xmm1
    __asm	movd		eax,				xmm0
}

static const __declspec(align(16)) uint8_t excessadd[16] = { 4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4 };

static uint32_t __stdcall ExcessPixels(const uint8_t *p1, int32_t pitch1, const uint8_t *p2, int32_t pitch2)
{
    __asm	mov			edx,				pitch1
    __asm	mov			esi,				pitch2
    __asm	mov			eax,				p1
    __asm	lea			ecx,				[edx + 2*edx]
    __asm	lea			edi,				[esi + 2*esi]
    __asm	mov			ebx,				p2
    __asm	movq		xmm0,				QWORD PTR[eax]
    __asm	movq		xmm2,				QWORD PTR[eax + edx]
    __asm	movhps		xmm0,				[eax + 2*edx]
    __asm	movhps		xmm2,				[eax + ecx]
    __asm	movdqa		xmm3,				xmm0
    __asm	movdqa		xmm4,				xmm2
    __asm	movq		xmm5,				QWORD PTR[ebx]
    __asm	movq		xmm6,				QWORD PTR[ebx + esi]
    __asm	lea			eax,				[eax + 4*edx]
    __asm	movhps		xmm5,				[ebx + 2*esi]
    __asm	movhps		xmm6,				[ebx + edi]
    __asm	psubusb		xmm0,				xmm5
    __asm	psubusb		xmm2,				xmm6
    __asm	psubusb		xmm5,				xmm3
    __asm	psubusb		xmm6,				xmm4
    __asm	psubusb		xmm0,				xmm7
    __asm	psubusb		xmm2,				xmm7
    __asm	lea			ebx,				[ebx + 4*esi]
    __asm	psubusb		xmm5,				xmm7
    __asm	psubusb		xmm6,				xmm7
    __asm	pcmpeqb		xmm0,				xmm5
    __asm	pcmpeqb		xmm6,				xmm2
    __asm	movq		xmm1,				QWORD PTR[eax]
    __asm	paddb		xmm0,				xmm6
    __asm	movq		xmm2,				QWORD PTR[eax + edx]
    __asm	movhps		xmm1,				[eax + 2*edx]
    __asm	movhps		xmm2,				[eax + ecx]
    __asm	movdqa		xmm3,				xmm1
    __asm	movdqa		xmm4,				xmm2
    __asm	movq		xmm5,				QWORD PTR[ebx]
    __asm	movq		xmm6,				QWORD PTR[ebx + esi]
    __asm	movhps		xmm5,				[ebx + 2*esi]
    __asm	movhps		xmm6,				[ebx + edi]
    __asm	psubusb		xmm1,				xmm5
    __asm	psubusb		xmm2,				xmm6
    __asm	psubusb		xmm5,				xmm3
    __asm	psubusb		xmm6,				xmm4
    __asm	psubusb		xmm1,				xmm7
    __asm	psubusb		xmm2,				xmm7
    __asm	psubusb		xmm5,				xmm7
    __asm	psubusb		xmm6,				xmm7
    __asm	pcmpeqb		xmm1,				xmm5
    __asm	pcmpeqb		xmm6,				xmm2
    __asm	pxor		xmm5,				xmm5
    __asm	paddb		xmm0,				xmm1
    __asm	paddb		xmm6,				excessadd
    __asm	paddb		xmm0,				xmm6
    __asm	psadbw		xmm0,				xmm5
    __asm	movhlps		xmm1,				xmm0
    __asm	paddd		xmm0,				xmm1
    __asm	movd		eax,				xmm0
}

static void __stdcall processneighbours1(MotionDetectionDistData *mdd)
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

static void __stdcall  processneighbours2(MotionDetectionDistData *mdd)
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

static void __stdcall  processneighbours3(MotionDetectionDistData *mdd)
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
    do
    {
        uint32_t sum = 0;
        int32_t i = mdd->dist;
        do
        {
            sum += *end++;
        } while(--i);

        i = mdd->dist1;
        do
        {
            *isum2++ = (sum += *end++);
        } while(--i);

        i = mdd->hint32_terior; 
        do
        {
            *isum2++ = (sum += *end++ - *begin++);
        } while(--i);

        i = mdd->dist;
        do
        {
            *isum2++ = (sum -= *begin++);
        } while(--i);

        begin += mdd->dist2;
        end++;
    } while(--j);

    uint32_t *isum1 = isum2 = mdd->isum;
    begin = mdd->md.blockproperties;
    j = mdd->md.hblocks;
    do
    {
        uint32_t sum = 0;
        int32_t	i = mdd->dist;
        do
        {
            sum += *isum2;
            isum2 = (uint32_t*)((char*)isum2 + mdd->isumline);
        } while(--i);

        i = mdd->dist1;
        do
        {
            sum += *isum2;
            isum2 = (uint32_t*)((char*)isum2 + mdd->isumline);
            if(sum > mdd->tolerance) *begin |= MOTION_FLAG2;
            begin += mdd->md.nline;
        } while(--i);

        i = mdd->vint32_terior;
        do
        {
            sum += *isum2 - *isum1;
            isum2 = (uint32_t*)((char*)isum2 + mdd->isumline);
            isum1 = (uint32_t*)((char*)isum1 + mdd->isumline);
            if(sum > mdd->tolerance) *begin |= MOTION_FLAG2;
            begin += mdd->md.nline;
        } while(--i);

        i = mdd->dist;
        do
        {
            sum -= *isum1;
            isum1 = (uint32_t*)((char*)isum1 + mdd->isumline);
            if(sum > mdd->tolerance) *begin |= MOTION_FLAG2;
            begin += mdd->md.nline;
        } while(--i);	

        begin += mdd->colinc;
        isum1 = (uint32_t*)((char*)isum1 + mdd->isuminc1);
        isum2 = (uint32_t*)((char*)isum2 + mdd->isuminc2);
    } while(--j);
}

#define SSE2init() \
    __asm movdqu xmm7, [ecx].noiselevel

#define mminit() \
    __asm movq mm7, [ecx].noiselevel

static void markblocks1(MotionDetectionData *md, const uint8_t *p1, int32_t pitch1, const uint8_t *p2, int32_t pitch2)
{
    SSE2init()
        mminit()

        int32_t inc1 = MOTIONBLOCKHEIGHT * pitch1 - md->linewidth;
    int32_t inc2 = MOTIONBLOCKHEIGHT * pitch2 - md->linewidth;
    uint8_t *properties = md->blockproperties;

    int32_t j = md->vblocks;
    do {
        int32_t i = md->hblocks;

        do {
            properties[0] = 0;

            if(md->blockcompare(p1, pitch1, p2, pitch2) >= md->threshold) {
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
    SSE2init();

    int32_t inc = MOTIONBLOCKHEIGHT*pitch - md->linewidthSSE2;
    uint8_t *properties = md->blockproperties;

    int32_t j = md->vblocks;
    do {
        int32_t i = md->hblocksSSE2;

        do {
            md->blockcompareSSE2(p1, p2, pitch); 
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

            if(md->blockcompare(p1, pitch, p2, pitch) >= md->threshold) {
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
        markblocks2(&mdd->md, p1, p2, pitch1);
    } else {
        markblocks1(&mdd->md, p1, pitch1, p2, pitch2);
    }
    mdd->distblocks = 0;
    if(mdd->dist) {
        markneighbours(mdd);
        (mdd->processneighbours)(mdd);
    }
}

static void __stdcall copy8x8(uint8_t *dest, int32_t dpitch, const uint8_t *src, int32_t spitch)
{
    __asm	mov			esi,			src
    __asm	mov			eax,			spitch
    __asm	mov			edi,			dest
    __asm	mov			ebx,			dpitch
    __asm	movq		mm0,			[esi]
    __asm	lea			ecx,			[eax + 2*eax]
    __asm	movq		mm1,			[esi + eax]
    __asm	movq		[edi],			mm0
    __asm	lea			edx,			[ebx + 2*ebx]
    __asm	movq		[edi + ebx],	mm1

    __asm	movq		mm0,			[esi + 2*eax]
    __asm	movq		mm1,			[esi + ecx]
    __asm	movq		[edi + 2*ebx],	mm0
    __asm	lea			esi,			[esi + 4*eax]
    __asm	movq		[edi + edx],	mm1

    __asm	movq		mm0,			[esi]
    __asm	lea			edi,			[edi + 4*ebx]
    __asm	movq		mm1,			[esi + eax]
    __asm	movq		[edi],			mm0
    __asm	movq		[edi + ebx],	mm1

    __asm	movq		mm0,			[esi + 2*eax]
    __asm	movq		mm1,			[esi + ecx]
    __asm	movq		[edi + 2*ebx],	mm0
    __asm	movq		[edi + edx],	mm1
}

static int32_t __stdcall vertical_diff(const uint8_t *p, int32_t pitch)
{
    __asm	mov			eax,			p
    __asm	mov			edx,			pitch
    __asm	pinsrw		mm0,			[eax], 0
    __asm	lea			ecx,			[2*edx + edx]
    __asm	pinsrw		mm1,			[eax + edx], 0
    __asm	pinsrw		mm0,			[eax + 2*edx], 1
    __asm	pinsrw		mm1,			[eax + ecx], 1
    __asm	lea			eax,			[eax + 4*edx]
    __asm	pcmpeqb		mm7,			mm7
    __asm	pinsrw		mm0,			[eax], 2
    __asm	pinsrw		mm1,			[eax + edx], 2
    __asm	psrlw		mm7,			8
    __asm	pinsrw		mm0,			[eax + 2*edx], 3
    __asm	pinsrw		mm1,			[eax + ecx], 3
    __asm	movq		mm2,			mm0
    __asm	movq		mm3,			mm1
    __asm	pand		mm0,			mm7
    __asm	psllw		mm1,			8
    __asm	psrlw		mm2,			8
    __asm	psubusb		mm3,			mm7
    __asm	por			mm0,			mm1
    __asm	por			mm2,			mm3
    __asm	psadbw		mm0,			mm2
    __asm	movd		eax,			mm0
}

static int32_t __stdcall horizontal_diff(const uint8_t *p, int32_t pitch)
{
    __asm	mov			edx,			p
    __asm	mov			eax,			pitch
    __asm	movq		mm0,			[edx]
    __asm	psadbw		mm0,			[edx + eax]
    __asm	movd		eax,			mm0
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
                        if(vertical_diff(dp2 + leftdp, dpitch) > vertical_diff(sp2 + leftsp, spitch) + pp->pthreshold) {
                            ++to_restore;
                            cl[-1] = MOTION_FLAG3;
                        }
                    }

                    if(cl[1] == 0) {
                        if(vertical_diff(dp2 + rightdp, dpitch) > vertical_diff(sp2 + rightsp, spitch) + pp->pthreshold) {
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

static void __stdcall copy_yv12_chroma(uint8_t *destu, uint8_t *destv, int32_t dpitch, const uint8_t *srcu, const uint8_t *srcv, int32_t spitch)
{
    __asm	mov			esi,			srcu
    __asm	mov			eax,			spitch
    __asm	mov			edi,			destu
    __asm	mov			ebx,			dpitch
    __asm	movq		mm0,			[esi]
    __asm	lea			ecx,			[eax + 2*eax]
    __asm	movq		mm1,			[esi + eax]
    __asm	movq		[edi],			mm0
    __asm	lea			edx,			[ebx + 2*ebx]
    __asm	movq		[edi + ebx],	mm1

    __asm	movq		mm0,			[esi + 2*eax]
    __asm	movq		mm1,			[esi + ecx]
    __asm	movq		[edi + 2*ebx],	mm0
    __asm	movq		[edi + edx],	mm1

    __asm	mov			esi,			srcv
    __asm	mov			edi,			destv
    __asm	movq		mm0,			[esi]
    __asm	movq		mm1,			[esi + eax]
    __asm	movq		[edi],			mm0
    __asm	movq		[edi + ebx],	mm1

    __asm	movq		mm0,			[esi + 2*eax]
    __asm	movq		mm1,			[esi + ecx]
    __asm	movq		[edi + 2*ebx],	mm0
    __asm	movq		[edi + edx],	mm1
}

static void __stdcall copy_yuy2_chroma(uint8_t *destu, uint8_t *destv, int32_t dpitch, const uint8_t *srcu, const uint8_t *srcv, int32_t spitch)
{
    __asm	mov			esi,			srcu
    __asm	mov			eax,			spitch
    __asm	mov			edi,			destu
    __asm	mov			ebx,			dpitch
    __asm	movq		mm0,			[esi]
    __asm	lea			ecx,			[eax + 2*eax]
    __asm	movq		mm1,			[esi + eax]
    __asm	movq		[edi],			mm0
    __asm	lea			edx,			[ebx + 2*ebx]
    __asm	movq		[edi + ebx],	mm1

    __asm	movq		mm0,			[esi + 2*eax]
    __asm	movq		mm1,			[esi + ecx]
    __asm	movq		[edi + 2*ebx],	mm0
    __asm	lea			esi,			[esi + 4*eax]
    __asm	movq		[edi + edx],	mm1

    __asm	movq		mm0,			[esi]
    __asm	lea			edi,			[edi + 4*ebx]
    __asm	movq		mm1,			[esi + eax]
    __asm	movq		[edi],			mm0
    __asm	movq		[edi + ebx],	mm1

    __asm	movq		mm0,			[esi + 2*eax]
    __asm	movq		mm1,			[esi + ecx]
    __asm	movq		[edi + 2*ebx],	mm0
    __asm	movq		[edi + edx],	mm1

    __asm	mov			esi,			srcv
    __asm	mov			edi,			destv
    __asm	movq		mm0,			[esi]
    __asm	movq		mm1,			[esi + eax]
    __asm	movq		[edi],			mm0
    __asm	movq		[edi + ebx],	mm1

    __asm	movq		mm0,			[esi + 2*eax]
    __asm	movq		mm1,			[esi + ecx]
    __asm	movq		[edi + 2*ebx],	mm0
    __asm	lea			esi,			[esi + 4*eax]
    __asm	movq		[edi + edx],	mm1

    __asm	movq		mm0,			[esi]
    __asm	lea			edi,			[edi + 4*ebx]
    __asm	movq		mm1,			[esi + eax]
    __asm	movq		[edi],			mm0
    __asm	movq		[edi + ebx],	mm1

    __asm	movq		mm0,			[esi + 2*eax]
    __asm	movq		mm1,			[esi + ecx]
    __asm	movq		[edi + 2*ebx],	mm0
    __asm	movq		[edi + edx],	mm1
}

static int32_t __stdcall horizontal_diff_chroma(const uint8_t *u, const uint8_t *v, int32_t pitch)
{
    __asm	mov			edx,			u
    __asm	mov			eax,			pitch
    __asm	movd		mm0,			[edx]
    __asm	mov			ecx,			v
    __asm	movd		mm1,			[edx + eax]
    __asm	punpckldq	mm0,			[ecx]
    __asm	punpckldq	mm1,			[ecx + eax]
    __asm	psadbw		mm0,			mm1
    __asm	movd		eax,			mm0
}

static int32_t __stdcall vertical_diff_yv12_chroma(const uint8_t *u, const uint8_t *v, int32_t pitch)
{
    __asm	mov			eax,			u
    __asm	mov			edx,			pitch
    __asm	pinsrw		mm0,			[eax], 0
    __asm	lea			ecx,			[2*edx + edx]
    __asm	pinsrw		mm1,			[eax + edx], 0
    __asm	pinsrw		mm0,			[eax + 2*edx], 1
    __asm	pinsrw		mm1,			[eax + ecx], 1
    __asm	mov			eax,			v
    __asm	pcmpeqb		mm7,			mm7
    __asm	pinsrw		mm0,			[eax], 2
    __asm	pinsrw		mm1,			[eax + edx], 2
    __asm	psrlw		mm7,			8
    __asm	pinsrw		mm0,			[eax + 2*edx], 3
    __asm	pinsrw		mm1,			[eax + ecx], 3
    __asm	movq		mm2,			mm0
    __asm	movq		mm3,			mm1
    __asm	pand		mm0,			mm7
    __asm	psllw		mm1,			8
    __asm	psrlw		mm2,			8
    __asm	psubusb		mm3,			mm7
    __asm	por			mm0,			mm1
    __asm	por			mm2,			mm3
    __asm	psadbw		mm0,			mm2
    __asm	movd		eax,			mm0
}

static int32_t __stdcall vertical_diff_yuy2_chroma(const uint8_t *u, const uint8_t *v, int32_t pitch)
{
    __asm	mov			eax,			u
    __asm	mov			edx,			pitch
    __asm	pinsrw		mm0,			[eax], 0
    __asm	lea			ecx,			[2*edx + edx]
    __asm	pinsrw		mm1,			[eax + edx], 0
    __asm	pinsrw		mm0,			[eax + 2*edx], 1
    __asm	pinsrw		mm1,			[eax + ecx], 1
    __asm	lea			eax,			[eax + 4*edx]
    __asm	pinsrw		mm0,			[eax], 2
    __asm	pinsrw		mm1,			[eax + edx], 2
    __asm	pinsrw		mm0,			[eax + 2*edx], 3
    __asm	pinsrw		mm1,			[eax + ecx], 3
    __asm	movq		mm2,			mm0
    __asm	movq		mm3,			mm1
    __asm	pand		mm0,			mm7
    __asm	psllw		mm1,			8
    __asm	psrlw		mm2,			8
    __asm	psubusb		mm3,			mm7
    __asm	mov			eax,			v
    __asm	por			mm0,			mm1
    __asm	por			mm2,			mm3
    __asm	pinsrw		mm4,			[eax], 0
    __asm	pinsrw		mm1,			[eax + edx], 0
    __asm	pinsrw		mm4,			[eax + 2*edx], 1
    __asm	pinsrw		mm1,			[eax + ecx], 1
    __asm	lea			eax,			[eax + 4*edx]
    __asm	pinsrw		mm4,			[eax], 2
    __asm	pinsrw		mm1,			[eax + edx], 2
    __asm	pinsrw		mm4,			[eax + 2*edx], 3
    __asm	pinsrw		mm1,			[eax + ecx], 3
    __asm	movq		mm6,			mm4
    __asm	movq		mm3,			mm1
    __asm	pand		mm4,			mm7
    __asm	psllw		mm1,			8
    __asm	psrlw		mm6,			8
    __asm	psubusb		mm3,			mm7
    __asm	por			mm4,			mm1
    __asm	por			mm6,			mm3
    __asm	psadbw		mm0,			mm2
    __asm	psadbw		mm4,			mm6
    __asm	pavgw		mm0,			mm4
    __asm	movd		eax,			mm0
}

static void inline colorise(uint8_t *u, uint8_t *v, int32_t pitch, int32_t height, uint32_t ucolor, uint32_t vcolor)
{
    int32_t i = height;

    do {
        *(uint32_t*)u = ucolor;
        *(uint32_t*)v = vcolor;
        u += pitch;
        v += pitch;
    } while(--i);
}

static void postprocessing(PostProcessingData *pp, uint8_t *dp, int32_t dpitch, uint8_t *dpU, uint8_t *dpV, int32_t dpitchUV, const uint8_t *sp, int32_t spitch, const uint8_t *spU, const uint8_t *spV, int32_t spitchUV)
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
                    pp->copy_chroma(dpU2, dpV2, dpitchUV, spU2, spV2, spitchUV);
                    cl[0] &= ~TO_CLEAN;

                    if(cl[-1] == 0) {
                        if((vertical_diff(dp2 + leftdp, dpitch) > vertical_diff(sp2 + leftsp, spitch) + pp->pthreshold)
                            || (pp->vertical_diff_chroma(dpU2 + Cleftdp, dpV2 + Cleftdp, dpitchUV) > pp->vertical_diff_chroma(spU2 + Cleftsp, spV2 + Cleftsp, spitchUV) + pp->cthreshold)) {
                                ++to_restore;
                                cl[-1] = MOTION_FLAG3;
                        }
                    }

                    if(cl[1] == 0) {
                        if((vertical_diff(dp2 + rightdp, dpitch) > vertical_diff(sp2 + rightsp, spitch) + pp->pthreshold)
                            || (pp->vertical_diff_chroma(dpU2 + Crightdp, dpV2 + Crightdp, dpitchUV) > pp->vertical_diff_chroma(spU2 + Crightsp, spV2 + Crightsp, spitchUV) + pp->cthreshold)) {
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
                if((properties[0] & MOTION_FLAG) != 0)
                {
                    u_color = u_mcolor; v_color = v_mcolor;
                }
                if((properties[0] & MOTION_FLAGP) != 0)
                {
                    u_color = u_pcolor; v_color = v_pcolor;
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

static void FillMotionDetection(const VSMap *in, VSMap *out, const VSAPI *vsapi, MotionDetectionData *md, const VSVideoInfo *vi)
{
    md = (MotionDetectionData *)malloc(sizeof(MotionDetectionData));

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

    if(noise > 0)
    {
        md->blockcompareSSE2 = NSADcompareSSE2;
        md->blockcompare = NSADcompare;
        memset(md->noiselevel, noise, 16);
        if(noisy >= 0)
        {
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

static void FillMotionDetectionDist(const VSMap *in, VSMap *out, const VSAPI *vsapi, MotionDetectionDistData *mdd, const VSVideoInfo *vi)
{
    mdd = (MotionDetectionDistData *)malloc(sizeof(MotionDetectionDistData));

    FillMotionDetection(in, out, vsapi, &mdd->md, vi);

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

    static void (__stdcall *neighbourproc[3])(MotionDetectionDistData*) = { processneighbours1,
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

static void FillPostProcessing(const VSMap *in, VSMap *out, const VSAPI *vsapi, PostProcessingData *pp, const VSVideoInfo *vi)
{
    pp = (PostProcessingData *)malloc(sizeof(PostProcessingData));

    int32_t err;
    pp->pthreshold = (int32_t) vsapi->propGetInt(in, "pthreshold", 0, &err);
    if (err) {
        pp->pthreshold = 10;
    }

    pp->cthreshold = (int32_t) vsapi->propGetInt(in, "cthreshold", 0, &err);
    if (err) {
        pp->cthreshold = 10;
    }

    FillMotionDetectionDist(in, out, vsapi, &pp->mdd, vi);

    pp->vertical_diff_chroma = vertical_diff_yv12_chroma;
    pp->copy_chroma = copy_yv12_chroma;
    pp->linewidthUV = pp->mdd.md.linewidth / 2;
    pp->chromaheight = MOTIONBLOCKHEIGHT / 2;

    if(vi->format->id == pfYUV422P8)
    {
        pp->chromaheight *= 2;
        pp->vertical_diff_chroma = vertical_diff_yuy2_chroma;
        pp->copy_chroma = copy_yuy2_chroma;
    }
    pp->chromaheightm = pp->chromaheight - 1;
}

void FillRemoveDirt(const VSMap *in, VSMap *out, const VSAPI *vsapi, RemoveDirtData *rd, const VSVideoInfo *vi)
{
    rd = (RemoveDirtData *)malloc(sizeof(RemoveDirtData));

    int32_t err;
    rd->grey = vsapi->propGetInt(in, "grey", 0, &err) == 1;
    if (err) {
        rd->grey = false;
    }

    rd->show = vsapi->propGetInt(in, "show", 0, &err) == 1;
    if (err) {
        rd->show = false;
    }

    FillPostProcessing(in, out, vsapi, &rd->pp, vi);
}

int32_t RemoveDirtProcessFrame(RemoveDirtData *rd, VSFrameRef *dest, const VSFrameRef *src, const VSFrameRef *previous, const VSFrameRef *next, int32_t frame, const VSAPI *vsapi)
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

    if(rd->grey) {
        postprocessing_grey(&rd->pp, destY, destPitchY, srcY, srcPitchY);
    } else {
        postprocessing(&rd->pp, destY, destPitchY, destU, destV, destPitchUV, srcY, srcPitchY, srcU, srcV, srcPitchUV);
    }

    if(rd->show) {
        show_motion(&rd->pp, destU, destV, destPitchUV);
    }
    __asm emms

    return rd->pp.restored_blocks + rd->pp.mdd.distblocks + rd->pp.mdd.md.motionblocks;
}
