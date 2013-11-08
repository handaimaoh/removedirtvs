#include "shared.h"

static inline uint32_t aligned_diff(const uint8_t *sp1, int32_t spitch1, const uint8_t *sp2, int32_t spitch2, int32_t hblocks, int32_t incpitch, int32_t height)
{
    __asm    pxor       xmm0,   xmm0
    __asm    mov        eax,    incpitch
    __asm    mov        ebx,    spitch2
    __asm    mov        esi,    sp1
    __asm    add        ebx,    eax
    __asm    mov        edi,    sp2
    __asm    add        eax,    spitch1
    __asm    pxor       xmm1,   xmm1
    __asm    mov        edx,    height
    __asm    mov        ecx,    hblocks
    __asm    align      16
    __asm    _loop:
    __asm    movdqa     xmm2,   [esi]
    __asm    movdqa     xmm3,  [esi + 16]
    __asm    psadbw     xmm2,   [edi]
    __asm    add        esi,    2 * 16
    __asm    psadbw     xmm3,   [edi + 16]
    __asm    paddd      xmm0,   xmm2
    __asm    add        edi,    2 * 16
    __asm    paddd      xmm1,   xmm3
    __asm    loop       _loop
    __asm    add        esi,    eax
    __asm    add        edi,    ebx
    __asm    dec        edx
    __asm    mov        ecx,    hblocks
    __asm    jnz        _loop
    __asm    paddd      xmm0,   xmm1
    __asm    movd       eax,    xmm0
}

static inline uint32_t unaligned_diff(const uint8_t *sp1, int32_t spitch1, const uint8_t *sp2, int32_t spitch2, int32_t hblocks, int32_t incpitch, int32_t height)
{
    __asm   pxor        xmm0,   xmm0
    __asm   mov         eax,    incpitch
    __asm   mov         ebx,    spitch2
    __asm   mov         esi,    sp1
    __asm   add         ebx,    eax
    __asm   mov         edi,    sp2
    __asm   add         eax,    spitch1
    __asm   pxor        xmm1,   xmm1
    __asm   mov         edx,    height
    __asm   mov         ecx,    hblocks
    __asm   align       16
    __asm   _loop:
    __asm   movdqu   xmm2,   [esi]
    __asm   movdqu   xmm3,   [esi + 16]
    __asm   add         esi,    2 * 16
    __asm   movdqu   xmm4,   [edi]
    __asm   movdqu   xmm5,   [edi + 16]
    __asm   psadbw      xmm2,   xmm4
    __asm   add         edi,    2 * 16
    __asm   psadbw      xmm3,   xmm5
    __asm   paddd       xmm0,   xmm2
    __asm   paddd       xmm1,   xmm3
    __asm   loop        _loop
    __asm   add         esi,    eax
    __asm   add         edi,    ebx
    __asm   dec         edx
    __asm   mov         ecx,    hblocks
    __asm   jnz         _loop
    __asm   paddd       xmm0,   xmm1
    __asm   movd        eax,    xmm0
}

uint32_t gdiff(const uint8_t *sp1, int32_t spitch1, const uint8_t *sp2, int32_t spitch2, int32_t hblocks, int32_t incpitch, int32_t height)
{
    if((((uint32_t)sp1 & (16 - 1)) + ((uint32_t)sp2 & (16 - 1))) == 0)
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
                              "grey:int32_t:opt", DupBlocksCreate, 0, plugin);
    registerFunc("Cleanse", "input:clip"\
                            "grey:int:opt"\
                            "reduceflicker:int:opt", ClenseCreate, 0);
    registerFunc("MCCleanse", "input:clip"\
                              "previous:clip"\
                              "next:clip"\
                              "grey:int:opt", MCClenseCreate, 0);
    registerFunc("BackwardCleanse", "input:clip"\
                                    "grey:int:opt", BackwardClenseCreate, 0);
    registerFunc("ForwardCleanse", "input:clip"\
                                   "grey:int:opt", ForwardClenseCreate, 0);
}
