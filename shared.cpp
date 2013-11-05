#include "shared.h"

static inline unsigned aligned_diff(const unsigned char *sp1, int spitch1, const unsigned char *sp2, int spitch2, int hblocks, int incpitch, int height)
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

static inline unsigned unaligned_diff(const unsigned char *sp1, int spitch1, const unsigned char *sp2, int spitch2, int hblocks, int incpitch, int height)
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

unsigned gdiff(const unsigned char *sp1, int spitch1, const unsigned char *sp2, int spitch2, int hblocks, int incpitch, int height)
{
    if((((unsigned)sp1 & (SSE_INCREMENT - 1)) + ((unsigned)sp2 & (SSE_INCREMENT - 1)) ) == 0)
        aligned_diff(sp1, spitch1, sp2, spitch2, hblocks, incpitch, height);
    else unaligned_diff(sp1, spitch1, sp2, spitch2, hblocks, incpitch, height);

}

void copyChroma(VSFrameRef *dest, const VSFrameRef *source, const VSVideoInfo *vi, const VSAPI *vsapi)
{
    if (vi->format->id == pfYUV420P8) {
        int destPitch = vsapi->getStride(dest, 1);
        int srcPitch = vsapi->getStride(source, 1);
        vs_bitblt(vsapi->getWritePtr(dest, 1), destPitch, vsapi->getReadPtr(source, 1), srcPitch, vi->width / 2, vi->height / 2);
        vs_bitblt(vsapi->getWritePtr(dest, 2), destPitch, vsapi->getReadPtr(source, 2), srcPitch, vi->width / 2, vi->height / 2);
    } else {
        vs_bitblt(vsapi->getWritePtr(dest, 0) + vi->width, vsapi->getStride(dest, 0), vsapi->getReadPtr(source, 0) + vi->width, vsapi->getStride(source, 0), vi->width, vi->height);
    }
}

int RemoveDirtProcessFrame(RemoveDirt *rd, VSFrameRef *dest, const VSFrameRef *src, const VSFrameRef *previous, const VSFrameRef *next, int frame, const VSAPI *vsapi)
{
    const unsigned char *nextY = vsapi->getReadPtr(next, 0);
    int nextPitchY = vsapi->getStride(next, 0);
    markblocks(vsapi->getReadPtr(previous, 0), vsapi->getStride(previous, 0) , nextY, nextPitchY);

    unsigned char *destY = vsapi->getWritePtr(dest, 0);
    unsigned char *destU = vsapi->getWritePtr(dest, 1);
    unsigned char *destV = vsapi->getWritePtr(dest, 2);
    int destPitchY = vsapi->getStride(dest, 0);
    int destPitchUV = vsapi->getStride(dest, 1);
    const unsigned char *srcY = vsapi->getReadPtr(src, 0);
    const unsigned char *srcU = vsapi->getReadPtr(src, 1);
    const unsigned char *srcV = vsapi->getReadPtr(src, 2);
    int srcPitchY = vsapi->getStride(src, 0);
    int srcPitchUV = vsapi->getStride(src, 1);

    if(rd->grey) {
        postprocessing_grey(destY, destPitchY, srcY, srcPitchY);
    } else {
        postprocessing(destY, destPitchY, destU, destV, destPitchUV, srcY, srcPitchY, srcU, srcV, srcPitchUV);
    }

    if(rd->show) {
        show_motion(destU, destV, destPitchUV);
    }
    __asm emms

    return restored_blocks + distblocks + motionblocks;
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
                                        "gmthreshold:int:opt"\
                                        "mthreshold:int:opt"\
                                        "noise:int:opt"\
                                        "noisy:int:opt"\
                                        "dist:int:opt"\
                                        "tolerance:int:opt"\
                                        "dmode:int:opt"\
                                        "pthreshold:int:opt"\
                                        "cthreshold:int:opt"\
                                        "grey:int:opt", RestoreMotionBlocksCreate, 0, plugin);
    registerFunc("DupBlocks", "input:clip"\
                              "gmthreshold:int:opt"\
                              "mthreshold:int:opt"\
                              "noise:int:opt"\
                              "noisy:int:opt"\
                              "dist:int:opt"\
                              "tolerance:int:opt"\
                              "dmode:int:opt"\
                              "pthreshold:int:opt"\
                              "cthreshold:int:opt"\
                              "grey:int:opt", CreateDupBlocks, 0, plugin);
}
