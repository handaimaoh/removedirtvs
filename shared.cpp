#include "shared.h"

__declspec(align(16)) uint32_t blockcompare_result[4];

void __stdcall SADcompareSSE2(const uint8_t *p1, const uint8_t *p2, int32_t pitch)
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
void __stdcall NSADcompareSSE2(const uint8_t *p1, const uint8_t *p2, int32_t pitch)
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

void __stdcall ExcessPixelsSSE2(const uint8_t *p1, const uint8_t *p2, int32_t pitch)
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

uint32_t __stdcall SADcompare(const uint8_t *p1, int32_t pitch1, const uint8_t *p2, int32_t pitch2)
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
uint32_t __stdcall NSADcompare(const uint8_t *p1, int32_t pitch1, const uint8_t *p2, int32_t pitch2)
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

static const __declspec(align(SSESIZE)) uint8_t excessadd[SSESIZE] = { 4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4 };

uint32_t __stdcall ExcessPixels(const uint8_t *p1, int32_t pitch1, const uint8_t *p2, int32_t pitch2)
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

static inline uint32_t aligned_diff(const uint8_t *sp1, int32_t spitch1, const uint8_t *sp2, int32_t spitch2, int32_t hblocks, int32_t incpitch, int32_t height)
{
    __asm    pxor       SSE0,   SSE0
    __asm    mov        eax,    incpitch
    __asm    mov        ebx,    spitch2
    __asm    mov        esi,    sp1
    __asm    add        ebx,    eax
    __asm    mov        edi,    sp2
    __asm    add        eax,    spitch1
    __asm    pxor       SSE1,   SSE1
    __asm    mov        edx,    height
    __asm    mov        ecx,    hblocks
    __asm    align      16
    __asm    _loop:
    __asm    SSE_RMOVE  SSE2,   [esi]
    __asm    SSE_RMOVE  SSE3,   [esi + SSE_INCREMENT]
    __asm    psadbw     SSE2,   [edi]
    __asm    add        esi,    2 * SSE_INCREMENT
    __asm    psadbw     SSE3,   [edi + SSE_INCREMENT]
    __asm    paddd      SSE0,   SSE2
    __asm    add        edi,    2 * SSE_INCREMENT
    __asm    paddd      SSE1,   SSE3
    __asm    loop       _loop
    __asm    add        esi,    eax
    __asm    add        edi,    ebx
    __asm    dec        edx
    __asm    mov        ecx,    hblocks
    __asm    jnz        _loop
    __asm    paddd      SSE0,   SSE1
    __asm    movd       eax,    SSE0
}

static inline uint32_t unaligned_diff(const uint8_t *sp1, int32_t spitch1, const uint8_t *sp2, int32_t spitch2, int32_t hblocks, int32_t incpitch, int32_t height)
{
    __asm   pxor        SSE0,   SSE0
    __asm   mov         eax,    incpitch
    __asm   mov         ebx,    spitch2
    __asm   mov         esi,    sp1
    __asm   add         ebx,    eax
    __asm   mov         edi,    sp2
    __asm   add         eax,    spitch1
    __asm   pxor        SSE1,   SSE1
    __asm   mov         edx,    height
    __asm   mov         ecx,    hblocks
    __asm   align       16
    __asm   _loop:
    __asm   SSE3_MOVE   SSE2,   [esi]
    __asm   SSE3_MOVE   SSE3,   [esi + SSE_INCREMENT]
    __asm   add         esi,    2 * SSE_INCREMENT
    __asm   SSE3_MOVE   SSE4,   [edi]
    __asm   SSE3_MOVE   SSE5,   [edi + SSE_INCREMENT]
    __asm   psadbw      SSE2,   SSE4
    __asm   add         edi,    2 * SSE_INCREMENT
    __asm   psadbw      SSE3,   SSE5
    __asm   paddd       SSE0,   SSE2
    __asm   paddd       SSE1,   SSE3
    __asm   loop        _loop
    __asm   add         esi,    eax
    __asm   add         edi,    ebx
    __asm   dec         edx
    __asm   mov         ecx,    hblocks
    __asm   jnz         _loop
    __asm   paddd       SSE0,   SSE1
    __asm   movd        eax,    SSE0
}

uint32_t gdiff(const uint8_t *sp1, int32_t spitch1, const uint8_t *sp2, int32_t spitch2, int32_t hblocks, int32_t incpitch, int32_t height)
{
    if((((uint32_t)sp1 & (SSE_INCREMENT - 1)) + ((uint32_t)sp2 & (SSE_INCREMENT - 1))) == 0)
        aligned_diff(sp1, spitch1, sp2, spitch2, hblocks, incpitch, height);
    else unaligned_diff(sp1, spitch1, sp2, spitch2, hblocks, incpitch, height);

}

void copyChroma(VSFrameRef *dest, const VSFrameRef *source, const VSVideoInfo *vi, const VSAPI *vsapi)
{
    if (vi->format->id == pfYUV420P8) {
        int32_t destPitch = vsapi->getStride(dest, 1);
        int32_t srcPitch = vsapi->getStride(source, 1);
        vs_bitblt(vsapi->getWritePtr(dest, 1), destPitch, vsapi->getReadPtr(source, 1), srcPitch, vi->width / 2, vi->height / 2);
        vs_bitblt(vsapi->getWritePtr(dest, 2), destPitch, vsapi->getReadPtr(source, 2), srcPitch, vi->width / 2, vi->height / 2);
    } else {
        vs_bitblt(vsapi->getWritePtr(dest, 0) + vi->width, vsapi->getStride(dest, 0), vsapi->getReadPtr(source, 0) + vi->width, vsapi->getStride(source, 0), vi->width, vi->height);
    }
}

void __stdcall processneighbours1(MotionDetectionDistData *mdd)
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

void __stdcall  processneighbours2(MotionDetectionDistData *mdd)
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

void __stdcall  processneighbours3(MotionDetectionDistData *mdd)
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

void markneighbours(MotionDetectionDistData *mdd)
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

void markblocks(MotionDetectionDistData *mdd, const uint8_t *p1, int32_t pitch1, const uint8_t *p2, int32_t pitch2)
{
    mdd->md.motionblocks = 0;
    if(((pitch1 - pitch2) | (((uint32_t)p1) & 15) | (((uint32_t)p2) & 15)) == 0)
        markblocks2(p1, p2, pitch1);
    else	markblocks1(p1, pitch1, p2, pitch2);
    mdd->distblocks = 0;
    if(mdd->dist) {
        markneighbours(mdd);
        (mdd->processneighbours)(mdd);
    }
}

void FillMotionDetection(const VSMap *in, VSMap *out, const VSAPI *vsapi, MotionDetectionData *md, const VSVideoInfo *vi)
{
    md = (MotionDetectionData *)malloc(sizeof(MotionDetectionData));
    
    int32_t err;    
    int32_t noise = vsapi->propGetint32_t(in, "noise", 0, &err);
    if (err) {
        noise = 0;
    }

    int32_t noisy = vsapi->propGetint32_t(in, "noisy", 0, &err);
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
        memset(md->noiselevel, noise, SSESIZE);
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

void FillMotionDetectionDist(const VSMap *in, VSMap *out, const VSAPI *vsapi, MotionDetectionDistData *mdd, const VSVideoInfo *vi)
{
    mdd = (MotionDetectionDistData *)malloc(sizeof(MotionDetectionDistData));

    FillMotionDetection(in, out, vsapi, &mdd->md, vi);

    int32_t err;
    uint32_t dmode = vsapi->propGetint32_t(in, "dmode", 0, &err);
    if (err) {
        dmode = 0;
    }

    uint32_t tolerance = vsapi->propGetint32_t(in, "", 0, &err);
    if (err) {
        tolerance = 12;
    }

    int32_t dist = vsapi->propGetint32_t(in, "dist", 0, &err);
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

void FillPostProcessing(const VSMap *in, VSMap *out, const VSAPI *vsapi, PostProcessingData *pp, const VSVideoInfo *vi)
{
    pp = (PostProcessingData *)malloc(sizeof(PostProcessingData));

    int32_t err;
    pp->pthreshold = vsapi->propGetint32_t(in, "pthreshold", 0, &err);
    if (err) {
        pp->pthreshold = 10;
    }

    pp->cthreshold = vsapi->propGetint32_t(in, "cthreshold", 0, &err);
    if (err) {
        pp->cthreshold = 10;
    }

    FillMotionDetectionDist(in, out, vsapi, &pp->mdd, vi);

    pp->linewidthUV = pp->mdd.md.linewidth / 2;
    pp->chromaheight = MOTIONBLOCKHEIGHT / 2;

    if(vi->format->id == pfYUV422P8)
    {
        pp->chromaheight *= 2;
    }
    pp->chromaheightm = pp->chromaheight - 1;
}

void FillRemoveDirt(const VSMap *in, VSMap *out, const VSAPI *vsapi, RemoveDirtData *rd, const VSVideoInfo *vi)
{
    rd = (RemoveDirtData *)malloc(sizeof(RemoveDirtData));

    int32_t err;
    rd->grey = vsapi->propGetint32_t(in, "grey", 0, &err);
    if (err) {
        rd->grey = false;
    }

    rd->show = vsapi->propGetint32_t(in, "show", 0, &err);
    if (err) {
        rd->show = false;
    }

    FillPostProcessing(in, out, vsapi, &rd->pp, vi);
}

int32_t RemoveDirtProcessFrame(RemoveDirtData *rd, VSFrameRef *dest, const VSFrameRef *src, const VSFrameRef *previous, const VSFrameRef *next, int32_t frame, const VSAPI *vsapi)
{
    const uint8_t *nextY = vsapi->getReadPtr(next, 0);
    int32_t nextPitchY = vsapi->getStride(next, 0);
    markblocks(vsapi->getReadPtr(previous, 0), vsapi->getStride(previous, 0) , nextY, nextPitchY);

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
        postprocessing_grey(destY, destPitchY, srcY, srcPitchY);
    } else {
        postprocessing(destY, destPitchY, destU, destV, destPitchUV, srcY, srcPitchY, srcU, srcV, srcPitchUV);
    }

    if(rd->show) {
        show_motion(destU, destV, destPitchUV);
    }
    __asm emms

    return rd->pp.restored_blocks + rd->pp.mdd.distblocks + rd->pp.mdd.md.motionblocks;
}

VS_EXTERNAL_API(void) VapourSynthPluginInit(VSConfigPlugin configFunc, VSRegisterFunction registerFunc, VSPlugin *plugin)
{
    configFunc("com.fakeurl.vsremovedirt", "vsrd", "RemoveDirt VapourSynth Port", VAPOURSYNTH_API_VERSION, 1, plugin);
    registerFunc("SCSelect", "input:clip;"\
                             "sceneBegin:clip"\
                             "sceneEnd:clip"\
                             "globalMotion:clip"\
                             "dfactor:float:opt", SCSelectCreate, 0, plugin);
    registerFunc("RestoreMotionBlocks", "input:clip"\
                                        "restore:clip"\
                                        "neighbour:clip:opt"\
                                        "neighbour2:clip:opt"\
                                        "alternative:clip:opt"\
                                        "gmthreshold:int32_t:opt"\
                                        "mthreshold:int32_t:opt"\
                                        "noise:int32_t:opt"\
                                        "noisy:int32_t:opt"\
                                        "dist:int32_t:opt"\
                                        "tolerance:int32_t:opt"\
                                        "dmode:int32_t:opt"\
                                        "pthreshold:int32_t:opt"\
                                        "cthreshold:int32_t:opt"\
                                        "grey:int32_t:opt", RestoreMotionBlocksCreate, 0, plugin);
    registerFunc("DupBlocks", "input:clip"\
                              "gmthreshold:int32_t:opt"\
                              "mthreshold:int32_t:opt"\
                              "noise:int32_t:opt"\
                              "noisy:int32_t:opt"\
                              "dist:int32_t:opt"\
                              "tolerance:int32_t:opt"\
                              "dmode:int32_t:opt"\
                              "pthreshold:int32_t:opt"\
                              "cthreshold:int32_t:opt"\
                              "grey:int32_t:opt", CreateDupBlocks, 0, plugin);
}
