#include "shared.h"

#define SSE0 xmm0
#define SSE1 xmm1
#define SSE2 xmm2
#define SSE3 xmm3
#define SSE4 xmm4
#define SSE5 xmm5
#define SSE6 xmm6
#define SSE7 xmm7

#define	ASClensePixel(daddr, saddr, paddr, naddr, reg1, reg2, reg3, reg4, reg5, reg6, reg7, reg8)	\
    __asm	movdqa	reg1,			[paddr]					\
    __asm	movdqa	reg2,			[paddr + 16]	\
    __asm	movdqa	reg3,			reg1					\
    __asm	movdqa	reg5,			[naddr]					\
    __asm	movdqa	reg4,			reg2					\
    __asm	movdqa	reg6,			[naddr + 16]	\
    __asm	pminub		reg1,			reg5					\
    __asm	pminub		reg2,			reg6					\
    __asm	pmaxub		reg3,			reg5					\
    __asm	pmaxub		reg4,			reg6					\
    __asm	movdqa	reg7,			reg3					\
    __asm	movdqa	reg8,			reg4					\
    __asm	psubusb		reg7,			reg5					\
    __asm	psubusb		reg8,			reg6					\
    __asm	psubusb		reg5,			reg1					\
    __asm	psubusb		reg6,			reg2					\
    __asm	psubusb		reg1,			reg5					\
    __asm	psubusb		reg2,			reg6					\
    __asm	pmaxub		reg1,			[saddr]					\
    __asm	paddusb		reg3,			reg7					\
    __asm	paddusb		reg4,			reg8					\
    __asm	pmaxub		reg2,			[saddr + 16]	\
    __asm	pminub		reg1,			reg3					\
    __asm	pminub		reg2,			reg4					\
    __asm	movdqa	[daddr],		reg1					\
    __asm	movdqa	[daddr + 16], reg2

#define	USClensePixel(daddr, saddr, paddr, naddr, reg1, reg2, reg3, reg4, reg5, reg6, reg7, reg8)	\
    __asm	lddqu	reg1,			[paddr]					\
    __asm	lddqu	reg2,			[paddr + 16]	\
    __asm	movdqa	reg3,			reg1					\
    __asm	lddqu	reg5,			[naddr]					\
    __asm	movdqa	reg4,			reg2					\
    __asm	lddqu	reg6,			[naddr + 16]	\
    __asm	pminub		reg1,			reg5					\
    __asm	pminub		reg2,			reg6					\
    __asm	pmaxub		reg3,			reg5					\
    __asm	pmaxub		reg4,			reg6					\
    __asm	movdqa	reg7,			reg3					\
    __asm	movdqa	reg8,			reg4					\
    __asm	psubusb		reg7,			reg5					\
    __asm	psubusb		reg8,			reg6					\
    __asm	psubusb		reg5,			reg1					\
    __asm	psubusb		reg6,			reg2					\
    __asm	psubusb		reg1,			reg5					\
    __asm	psubusb		reg2,			reg6					\
    __asm	lddqu	reg5,			[saddr]					\
    __asm	paddusb		reg3,			reg7					\
    __asm	paddusb		reg4,			reg8					\
    __asm	lddqu	reg6,			[saddr + 16]	\
    __asm	pmaxub		reg1,			reg5					\
    __asm	pmaxub		reg2,			reg6					\
    __asm	pminub		reg1,			reg3					\
    __asm	pminub		reg2,			reg4					\
    __asm	movdqu	[daddr],		reg1					\
    __asm	movdqu	[daddr + 16], reg2

static void aligned_sclense(uint8_t *dp, int32_t dpitch, const uint8_t *_sp, int32_t spitch, const uint8_t *pp, int32_t ppitch, const uint8_t *np, int32_t npitch, int32_t hblocks, int32_t remainder, int32_t incpitch, int32_t height)
{
    __asm	mov			eax,				incpitch
    __asm	mov			ebx,				pp
    __asm	add			dpitch,				eax
    __asm	add			spitch,				eax
    __asm	add			ppitch,				eax
    __asm	add			npitch,				eax
    __asm	mov			esi,				_sp
    __asm	mov			edi,				dp
    __asm	mov			edx,				remainder
    __asm	mov			eax,				np
    __asm	mov			ecx,				hblocks
    __asm	align		16
    __asm	_loop:
    ASClensePixel(edi, esi, ebx, eax, SSE0, SSE1, SSE2, SSE3, SSE4, SSE5, SSE6, SSE7)		 
        __asm	add			eax,				2*16
    __asm	add			esi,				2*16
    __asm	add			edi,				2*16
    __asm	add			ebx,				2*16
    __asm	loop		_loop
    // the last pixels
    USClensePixel(edi + edx, esi + edx, ebx + edx, eax + edx, SSE0, SSE1, SSE2, SSE3, SSE4, SSE5, SSE6, SSE7)
        __asm	add			esi,				spitch
    __asm	add			edi,				dpitch
    __asm	add			ebx,				ppitch
    __asm	add			eax,				npitch
    __asm	dec			height
    __asm	mov			ecx,				hblocks
    __asm	jnz			_loop
}

static void unaligned_sclense(uint8_t *dp, int32_t dpitch, const uint8_t *_sp, int32_t spitch, const uint8_t *pp, int32_t ppitch, const uint8_t *np, int32_t npitch, int32_t hblocks, int32_t remainder, int32_t incpitch, int32_t height)
{
    __asm	mov			eax,				incpitch
    __asm	mov			ebx,				pp
    __asm	add			dpitch,				eax
    __asm	add			spitch,				eax
    __asm	add			ppitch,				eax
    __asm	add			npitch,				eax
    __asm	mov			esi,				_sp
    __asm	mov			edi,				dp
    __asm	mov			edx,				remainder
    __asm	mov			eax,				np
    __asm	mov			ecx,				hblocks
    __asm	align		16
    __asm	_loop:
    USClensePixel(edi, esi, ebx, eax, SSE0, SSE1, SSE2, SSE3, SSE4, SSE5, SSE6, SSE7)		 
        __asm	add			eax,				2*16
    __asm	add			esi,				2*16
    __asm	add			edi,				2*16
    __asm	add			ebx,				2*16
    __asm	dec			ecx
    __asm	jnz			_loop
    USClensePixel(edi + edx, esi + edx, ebx + edx, eax + edx, SSE0, SSE1, SSE2, SSE3, SSE4, SSE5, SSE6, SSE7)
        __asm	add			esi,				spitch
    __asm	add			edi,				dpitch
    __asm	add			ebx,				ppitch
    __asm	add			eax,				npitch
    __asm	dec			height
    __asm	mov			ecx,				hblocks
    __asm	jnz			_loop
}

static void sclense(uint8_t *dp, int32_t dpitch, const uint8_t *_sp, int32_t spitch, const uint8_t *pp, int32_t ppitch, const uint8_t *np, int32_t npitch, int32_t hblocks, int32_t remainder, int32_t incpitch, int32_t height)
{
    if( (((unsigned)dp & (16 - 1)) + ((uint32_t)_sp & (16 - 1)) + ((uint32_t)pp & (16 - 1)) + ((uint32_t)np & (16 - 1))
        + (spitch & (16 - 1)) + (ppitch & (16 - 1)) + (npitch & (16 - 1)) 
        ) == 0 ) aligned_sclense(dp, dpitch, _sp, spitch, pp, ppitch, np, npitch, hblocks, remainder, incpitch, height);
    else unaligned_sclense(dp, dpitch, _sp, spitch, pp, ppitch, np, npitch, hblocks, remainder, incpitch, height);
}

#define	AClensePixel(daddr, saddr, paddr, naddr, reg1, reg2, reg3, reg4, reg5, reg6, reg7, reg8)	\
    __asm	align		16 \
    __asm	movdqa	reg1,			[naddr]					\
    __asm	movdqa	reg2,			[naddr + 16]	\
    __asm	movdqa	reg3,			reg1					\
    __asm	movdqa	reg5,			[paddr]					\
    __asm	movdqa	reg4,			reg2					\
    __asm	movdqa	reg6,			[paddr + 16]	\
    __asm	pminub		reg1,			reg5					\
    __asm	pminub		reg2,			reg6					\
    __asm	pmaxub		reg1,			[saddr]					\
    __asm	pmaxub		reg3,			reg5					\
    __asm	pmaxub		reg4,			reg6					\
    __asm	pmaxub		reg2,			[saddr + 16]	\
    __asm	pminub		reg1,			reg3					\
    __asm	pminub		reg2,			reg4					\
    __asm	movdqa	[daddr],		reg1					\
    __asm	movdqa	[daddr + 16], reg2

#define	UClensePixel(daddr, saddr, paddr, naddr, reg1, reg2, reg3, reg4, reg5, reg6, reg7, reg8)	\
    __asm	align		16 \
    __asm	movdqu	reg1,			[naddr]					\
    __asm	movdqu	reg2,			[naddr + 16]	\
    __asm	movdqa	reg3,			reg1					\
    __asm	movdqu	reg5,			[paddr]					\
    __asm	movdqa	reg4,			reg2					\
    __asm	movdqu	reg6,			[paddr + 16]	\
    __asm	pminub		reg1,			reg5					\
    __asm	pminub		reg2,			reg6					\
    __asm	movdqu	reg7,			[saddr]					\
    __asm	pmaxub		reg3,			reg5					\
    __asm	pmaxub		reg4,			reg6					\
    __asm	movdqu	reg8,			[saddr + 16]	\
    __asm	pmaxub		reg1,			reg7					\
    __asm	pmaxub		reg2,			reg8					\
    __asm	pminub		reg1,			reg3					\
    __asm	pminub		reg2,			reg4					\
    __asm	movdqu	[daddr],		reg1					\
    __asm	movdqu	[daddr + 16], reg2

void inline ACleansePixel(uint8_t *dp, const uint8_t *_sp, const uint8_t *pp, const uint8_t *np)
{
    __m128i reg1 = _mm_load_si128((__m128i*)np);
    __m128i reg2 = _mm_load_si128((__m128i*)(np+16));
    __m128i reg3 = reg1;

    __m128i reg5 = _mm_load_si128((__m128i*)pp);
    __m128i reg4 = reg2;

    __m128i reg6 = _mm_load_si128((__m128i*)(pp+16));

    reg1 = _mm_min_epu8(reg1, reg5);
    reg2 = _mm_min_epu8(reg2, reg6);

    __m128i reg7 = _mm_load_si128((__m128i*)_sp);

    reg3 = _mm_max_epu8(reg3, reg5);
    reg4 = _mm_max_epu8(reg4, reg6);

    __m128i reg8 = _mm_load_si128((__m128i*)(_sp+16));

    reg1 = _mm_min_epu8(reg1, reg3);
    reg2 = _mm_min_epu8(reg2, reg4);

    _mm_store_si128((__m128i*)dp, reg1);
    _mm_store_si128((__m128i*)(dp+16), reg2);
}

void inline UCleansePixel(uint8_t *dp, const uint8_t *_sp, const uint8_t *pp, const uint8_t *np)
{
    __m128i xmm0 = _mm_loadu_si128((__m128i*)np);
    __m128i xmm1 = _mm_loadu_si128((__m128i*)(np+16));
    __m128i xmm2 = xmm0;

    __m128i xmm4 = _mm_loadu_si128((__m128i*)pp);
    __m128i xmm3 = xmm1;

    __m128i xmm5 = _mm_loadu_si128((__m128i*)(pp+16));

    xmm0 = _mm_min_epu8(xmm0, xmm4);
    xmm1 = _mm_min_epu8(xmm1, xmm5);

    __m128i xmm6 = _mm_loadu_si128((__m128i*)_sp);

    xmm2 = _mm_max_epu8(xmm2, xmm4);
    xmm3 = _mm_max_epu8(xmm3, xmm5);

    __m128i xmm7 = _mm_loadu_si128((__m128i*)(_sp+16));

    xmm0 = _mm_max_epu8(xmm0, xmm6);
    xmm1 = _mm_max_epu8(xmm1, xmm7);
    xmm0 = _mm_min_epu8(xmm0, xmm2);
    xmm1 = _mm_min_epu8(xmm1, xmm3);

    _mm_storeu_si128((__m128i*)dp, xmm0);
    _mm_storeu_si128((__m128i*)(dp+16), xmm1);
}

static void cleanse(uint8_t *dp, int32_t dpitch, const uint8_t *_sp, int32_t spitch, const uint8_t *pp, int32_t ppitch, const uint8_t *np, int32_t npitch, int32_t hblocks, int32_t remainder, int32_t incpitch, int32_t height)
{
    bool isAligned = ((((uint32_t)dp & (16 - 1)) + 
                       ((uint32_t)_sp & (16 - 1)) +
                       ((uint32_t)pp & (16 - 1)) +
                       ((uint32_t)np & (16 - 1)) +
                       (spitch & (16 - 1)) +
                       (ppitch & (16 - 1)) +
                       (npitch & (16 - 1))) == 0);

    //__asm	mov			eax,				incpitch
    //__asm	mov			ebx,				pp
    //__asm	add			dpitch,				eax
    dpitch += incpitch;
    //__asm	add			spitch,				eax
    spitch += incpitch;
    //__asm	add			ppitch,				eax
    ppitch += incpitch;
    //__asm	add			npitch,				eax
    npitch += incpitch;
    //__asm	mov			esi,				_sp
    //__asm	mov			edi,				dp
    //__asm	mov			edx,				remainder
    //__asm	mov			eax,				np
    //__asm	mov			ecx,				hblocks
    //__asm	_loop:
    int counter = hblocks;
    do {
        if (isAligned) {
            ACleansePixel(dp, _sp, pp, np);
        } else {
            UCleansePixel(dp, _sp, pp, np);
        }
        //__asm	add			eax,				2*16
        np += 32;
        //__asm	add			esi,				2*16
        _sp += 32;
        //__asm	add			edi,				2*16
        dp += 32;
        //__asm	add			ebx,				2*16
        pp += 32;
        //__asm	loop		_loop
        if (--counter > 0) {
            continue;
        }
        // the last pixels
        UCleansePixel(dp, _sp, pp, np);
        //__asm	add			esi,				spitch
        _sp += spitch;
        //__asm	add			edi,				dpitch
        dp += dpitch;
        //__asm	add			ebx,				ppitch
        pp += ppitch;
        //__asm	add			eax,				npitch
        np += npitch;
        //__asm	dec			height
        --height;
        //__asm	mov			ecx,				hblocks
        counter = hblocks;
        //__asm	jnz			_loop
    } while (height > 0);
}

static void FillGenericCleanse(GenericClenseData *gc, const VSMap *in, VSMap *out, const VSAPI *vsapi)
{
    gc->input = vsapi->propGetNode(in, "input", 0, 0);
    gc->vi = vsapi->getVideoInfo(gc->input);

    int32_t err;
    gc->grey = vsapi->propGetInt(in, "grey", 0, &err) == 1;
    if (err) {
        gc->grey = false;
    }

    if (gc->grey) {
        return;
    }

    const VSFrameRef *frame = vsapi->getFrame(0, gc->input, 0, 0);
    for (int32_t i = 0; i < gc->vi->format->numPlanes; ++i)
    {
        int32_t width = vsapi->getFrameWidth(frame, i);

        gc->hblocks[i] = --width / (2 * 16);
        gc->remainder[i] = (width & (2 * 16 - 1)) - (2 * 16 - 1);
        gc->incpitch[i] = 2 * 16 - vsapi->getFrameWidth(frame, i) + gc->remainder[i];
    }
    vsapi->freeFrame(frame);
}

static void VS_CC CleanseFree(void *instanceData, VSCore *core, const VSAPI *vsapi)
{
    CleanseData *d = (CleanseData *)instanceData;
    vsapi->freeNode(d->gc.input);
    vsapi->freeFrame(d->last_frame);
    free(d);
}

static const VSFrameRef *VS_CC CleanseGetFrame(int32_t n, int32_t activationReason, void **instanceData, void **frameData, VSFrameContext *frameCtx, VSCore *core, const VSAPI *vsapi)
{
    CleanseData *d = (CleanseData *) *instanceData;

    if (activationReason == arInitial) {
        vsapi->requestFrameFilter(n, d->gc.input, frameCtx);
        if (n > 0) {
            vsapi->requestFrameFilter(n - 1, d->gc.input, frameCtx);
            if (n < d->gc.vi->numFrames) {
                vsapi->requestFrameFilter(n + 1, d->gc.input, frameCtx);
            }
        }
    } else if (activationReason == arAllFramesReady) {
        const VSFrameRef *src_frame = vsapi->getFrameFilter(n, d->gc.input, frameCtx);

        if (!d->reduceflicker || (d->lnr != n - 1)) {
            if (n == 0) {
                return src_frame;
            }
            if (d->last_frame) {
                vsapi->freeFrame(d->last_frame);
            }
            d->last_frame = vsapi->getFrameFilter(n - 1, d->gc.input, frameCtx);
        }

        if(n >= d->gc.vi->numFrames) {
            return src_frame;
        }

        const VSFrameRef *next_frame = vsapi->getFrameFilter(n + 1, d->gc.input, frameCtx);

        int planes[3] = {0, 1, 2};
        const VSFrameRef * cp_planes[3] = { src_frame, src_frame, src_frame };
        VSFrameRef *dest = vsapi->newVideoFrame2(vsapi->getFrameFormat(src_frame), vsapi->getFrameWidth(src_frame, 0),
                                                 vsapi->getFrameHeight(src_frame, 0), cp_planes, planes, src_frame, core);

        for (int i = 0; i < d->gc.vi->format->numPlanes; ++i) {
            cleanse(vsapi->getWritePtr(dest, i), vsapi->getStride(dest, i),
                    vsapi->getReadPtr(src_frame, i), vsapi->getStride(src_frame, i),
                    vsapi->getReadPtr(d->last_frame, i), vsapi->getStride(d->last_frame, i),
                    vsapi->getReadPtr(next_frame, i), vsapi->getStride(next_frame, i),
                    d->gc.hblocks[i], d->gc.remainder[i], d->gc.incpitch[i], vsapi->getFrameHeight(src_frame, i));
        }

        vsapi->freeFrame(src_frame);
        vsapi->freeFrame(d->last_frame);
        vsapi->freeFrame(next_frame);

        d->last_frame = vsapi->copyFrame(dest, core);
        d->lnr = n;
        return dest;
    }

    return 0;
}

static void VS_CC CleanseInit(VSMap *in, VSMap *out, void **instanceData, VSNode *node, VSCore *core, const VSAPI *vsapi)
{
    CleanseData *d = (CleanseData *) *instanceData;
    vsapi->setVideoInfo(d->gc.vi, 1, node);
}

void VS_CC CleanseCreate(const VSMap *in, VSMap *out, void *userData, VSCore *core, const VSAPI *vsapi)
{
    CleanseData d = { 0 };

    FillGenericCleanse(&d.gc, in, out, vsapi);

    int err;
    d.reduceflicker = vsapi->propGetInt(in, "reduceflicker", 0, &err) == 1;
    if (err) {
        d.reduceflicker = true;
    }

    d.last_frame = 0;
    d.lnr = -2;

    CleanseData *data = (CleanseData *)malloc(sizeof(d));
    *data = d;

    vsapi->createFilter(in, out, "Cleanse", CleanseInit, CleanseGetFrame, CleanseFree, fmSerial, 0, data, core);
}

static void VS_CC MCCleanseFree(void *instanceData, VSCore *core, const VSAPI *vsapi)
{
    BMCCleanse *d = (BMCCleanse *)instanceData;
    vsapi->freeNode(d->gc.input);
    vsapi->freeNode(d->prev_clip);
    vsapi->freeNode(d->next_clip);
    free(d);
}

static const VSFrameRef *VS_CC MCCleanseGetFrame(int32_t n, int32_t activationReason, void **instanceData, void **frameData, VSFrameContext *frameCtx, VSCore *core, const VSAPI *vsapi)
{
    BMCCleanse *d = (BMCCleanse *) *instanceData;

    if (activationReason == arInitial) {
        vsapi->requestFrameFilter(n, d->gc.input, frameCtx);
        vsapi->requestFrameFilter(n, d->prev_clip, frameCtx);
        vsapi->requestFrameFilter(n, d->next_clip, frameCtx);
    } else if (activationReason == arAllFramesReady) {
        const VSFrameRef *src_frame = vsapi->getFrameFilter(n, d->gc.input, frameCtx);
        const VSFrameRef *prev_frame = vsapi->getFrameFilter(n, d->prev_clip, frameCtx);
        const VSFrameRef *next_frame = vsapi->getFrameFilter(n, d->next_clip, frameCtx);

        VSFrameRef *dest= vsapi->newVideoFrame(d->vi->format, d->vi->width, d->vi->height, src_frame, core);

        for (int i = 0; i < d->vi->format->numPlanes; ++i) {
            cleanse(vsapi->getWritePtr(dest, i), vsapi->getStride(dest, i),
                    vsapi->getReadPtr(src_frame, i), vsapi->getStride(src_frame, i),
                    vsapi->getReadPtr(prev_frame, i), vsapi->getStride(prev_frame, i),
                    vsapi->getReadPtr(next_frame, i), vsapi->getStride(next_frame, i),
                    d->gc.hblocks[i], d->gc.remainder[i], d->gc.incpitch[i], vsapi->getFrameHeight(src_frame, i));
        }
        vsapi->freeFrame(src_frame);
        vsapi->freeFrame(prev_frame);
        vsapi->freeFrame(next_frame);
        return dest;
    }

    return 0;
}

static void VS_CC MCCleanseInit(VSMap *in, VSMap *out, void **instanceData, VSNode *node, VSCore *core, const VSAPI *vsapi)
{
    BMCCleanse *d = (BMCCleanse *) *instanceData;
    vsapi->setVideoInfo(d->vi, 1, node);
}

void VS_CC MCCleanseCreate(const VSMap *in, VSMap *out, void *userData, VSCore *core, const VSAPI *vsapi)
{
    BMCCleanse d = { 0 };

    FillGenericCleanse(&d.gc, in, out, vsapi);

    d.prev_clip = vsapi->propGetNode(in, "previous", 0, 0);
    d.next_clip = vsapi->propGetNode(in, "next", 0, 0);

    if (!isSameFormat(d.vi, vsapi->getVideoInfo(d.prev_clip)) ||
        !isSameFormat(d.vi, vsapi->getVideoInfo(d.next_clip))) {
            vsapi->freeNode(d.gc.input);
            vsapi->freeNode(d.prev_clip);
            vsapi->freeNode(d.next_clip);
            vsapi->setError(out, "MCCleanse: Clips are not of equal type");
            return;
    }

    BMCCleanse *data = (BMCCleanse *)malloc(sizeof(d));
    *data = d;

    vsapi->createFilter(in, out, "MCCleanse", MCCleanseInit, MCCleanseGetFrame, MCCleanseFree, fmSerial, 0, data, core);
}

static void VS_CC BackwardCleanseFree(void *instanceData, VSCore *core, const VSAPI *vsapi)
{
    BackwardCleanseData *d = (BackwardCleanseData *)instanceData;
    vsapi->freeNode(d->gc.input);
    free(d);
}

static const VSFrameRef *VS_CC BackwardCleanseGetFrame(int32_t n, int32_t activationReason, void **instanceData, void **frameData, VSFrameContext *frameCtx, VSCore *core, const VSAPI *vsapi)
{
    BackwardCleanseData *d = (BackwardCleanseData *) *instanceData;

    if (activationReason == arInitial) {
        vsapi->requestFrameFilter(n, d->gc.input, frameCtx);
        if (n - 1 >= 0) {
            vsapi->requestFrameFilter(n - 1, d->gc.input, frameCtx);
        }
        if (n - 2 >= 0) {
            vsapi->requestFrameFilter(n - 2, d->gc.input, frameCtx);
        }
    } else if (activationReason == arAllFramesReady) {
        const VSFrameRef *src_frame = vsapi->getFrameFilter(n, d->gc.input, frameCtx);

        if (n < 2) {
            return src_frame;
        }

        const VSFrameRef *next_frame = vsapi->getFrameFilter(n - 1, d->gc.input, frameCtx);
        const VSFrameRef *next_frame2 = vsapi->getFrameFilter(n - 2, d->gc.input, frameCtx);

        VSFrameRef *dest= vsapi->newVideoFrame(d->gc.vi->format, d->gc.vi->width, d->gc.vi->height, src_frame, core);

        for (int i = 0; i < d->gc.vi->format->numPlanes; ++i) {
            sclense(vsapi->getWritePtr(dest, i), vsapi->getStride(dest, i),
                    vsapi->getReadPtr(src_frame, i), vsapi->getStride(src_frame, i),
                    vsapi->getReadPtr(next_frame, i), vsapi->getStride(next_frame, i),
                    vsapi->getReadPtr(next_frame2, i), vsapi->getStride(next_frame2, i),
                    d->gc.hblocks[i], d->gc.remainder[i], d->gc.incpitch[i], vsapi->getFrameHeight(src_frame, i));
        }

        vsapi->freeFrame(src_frame);
        vsapi->freeFrame(next_frame);
        vsapi->freeFrame(next_frame2);
        return dest;
    }

    return 0;
}

static void VS_CC BackwardCleanseInit(VSMap *in, VSMap *out, void **instanceData, VSNode *node, VSCore *core, const VSAPI *vsapi)
{
    BackwardCleanseData *d = (BackwardCleanseData *) *instanceData;
    vsapi->setVideoInfo(d->gc.vi, 1, node);
}

void VS_CC BackwardCleanseCreate(const VSMap *in, VSMap *out, void *userData, VSCore *core, const VSAPI *vsapi)
{
    BackwardCleanseData d = { 0 };

    FillGenericCleanse(&d.gc, in, out, vsapi);

    BackwardCleanseData *data = (BackwardCleanseData *)malloc(sizeof(d));
    *data = d;

    vsapi->createFilter(in, out, "BackwardCleanse", BackwardCleanseInit, BackwardCleanseGetFrame, BackwardCleanseFree, fmSerial, 0, data, core);
}

static void VS_CC ForwardCleanseFree(void *instanceData, VSCore *core, const VSAPI *vsapi)
{
    FowardCleanseData *d = (FowardCleanseData *)instanceData;
    vsapi->freeNode(d->bc.gc.input);
    free(d);
}

static const VSFrameRef *VS_CC ForwardCleanseGetFrame(int32_t n, int32_t activationReason, void **instanceData, void **frameData, VSFrameContext *frameCtx, VSCore *core, const VSAPI *vsapi)
{
    FowardCleanseData *d = (FowardCleanseData *) *instanceData;

    if (activationReason == arInitial) {
        vsapi->requestFrameFilter(n, d->bc.gc.input, frameCtx);
        if (n + 1 <= d->bc.gc.vi->numFrames) {
            vsapi->requestFrameFilter(n + 1, d->bc.gc.input, frameCtx);
        }
        if (n + 2 <= d->bc.gc.vi->numFrames) {
            vsapi->requestFrameFilter(n + 2, d->bc.gc.input, frameCtx);
        }
    } else if (activationReason == arAllFramesReady) {
        const VSFrameRef *src_frame = vsapi->getFrameFilter(n, d->bc.gc.input, frameCtx);

        if (n >= d->lastnr) {
            return src_frame;
        }

        const VSFrameRef *next_frame = vsapi->getFrameFilter(n + 1, d->bc.gc.input, frameCtx);
        const VSFrameRef *next_frame2 = vsapi->getFrameFilter(n + 2, d->bc.gc.input, frameCtx);

        VSFrameRef *dest= vsapi->newVideoFrame(d->bc.gc.vi->format, d->bc.gc.vi->width, d->bc.gc.vi->height, src_frame, core);

        for (int i = 0; i < d->bc.gc.vi->format->numPlanes; ++i) {
            sclense(vsapi->getWritePtr(dest, i), vsapi->getStride(dest, i),
                    vsapi->getReadPtr(src_frame, i), vsapi->getStride(src_frame, i),
                    vsapi->getReadPtr(next_frame, i), vsapi->getStride(next_frame, i),
                    vsapi->getReadPtr(next_frame2, i), vsapi->getStride(next_frame2, i),
                    d->bc.gc.hblocks[i], d->bc.gc.remainder[i], d->bc.gc.incpitch[i], vsapi->getFrameHeight(dest, i));
        }

        vsapi->freeFrame(src_frame);
        vsapi->freeFrame(next_frame);
        vsapi->freeFrame(next_frame2);
        return dest;
    }

    return 0;
}

static void VS_CC ForwardCleanseInit(VSMap *in, VSMap *out, void **instanceData, VSNode *node, VSCore *core, const VSAPI *vsapi)
{
    FowardCleanseData *d = (FowardCleanseData *) *instanceData;
    vsapi->setVideoInfo(d->bc.gc.vi, 1, node);
}

void VS_CC ForwardCleanseCreate(const VSMap *in, VSMap *out, void *userData, VSCore *core, const VSAPI *vsapi)
{
    FowardCleanseData d = { 0 };

    FillGenericCleanse(&d.bc.gc, in, out, vsapi);

    d.lastnr = d.bc.gc.vi->numFrames - 2;

    FowardCleanseData *data = (FowardCleanseData *)malloc(sizeof(d));
    *data = d;

    vsapi->createFilter(in, out, "FowardCleanseData", ForwardCleanseInit, ForwardCleanseGetFrame, ForwardCleanseFree, fmSerial, 0, data, core);
}
