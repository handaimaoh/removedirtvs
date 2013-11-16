#include "shared.h"

#ifdef VS_TARGET_CPU_X86
#include <emmintrin.h>
#endif

static __forceinline uint32_t aligned_diff(const uint8_t *sp1, int32_t spitch1, const uint8_t *sp2, int32_t spitch2, int32_t hblocks, int32_t incpitch, int32_t height)
{
    __m128i xmm0 = _mm_setzero_si128();
    __m128i xmm1 = _mm_setzero_si128();

    spitch2 += incpitch;
    incpitch += spitch1;

    int32_t counter = hblocks;
    do {
        __m128i xmm2 = _mm_load_si128((__m128i*)sp1);
        __m128i xmm3 = _mm_load_si128((__m128i*)(sp1+16));
        __m128i xmm4 = _mm_load_si128((__m128i*)sp2);
        __m128i xmm5 = _mm_load_si128((__m128i*)(sp2+16));

        xmm2 = _mm_sad_epu8(xmm2, xmm4);
        xmm3 = _mm_sad_epu8(xmm3, xmm5);
        xmm0 = _mm_add_epi32(xmm0, xmm2);
        xmm1 = _mm_add_epi32(xmm1, xmm3);

        sp1 += 32;
        sp2 += 32;
        if (--counter > 0) {
            continue;
        }
        sp1 += incpitch;
        sp2 += spitch2;
        counter = hblocks;
    } while (--height > 0);
    
    xmm0 = _mm_add_epi32(xmm0, xmm1);
    return (uint32_t)_mm_cvtsi128_si32(xmm0);
}

static __forceinline uint32_t unaligned_diff(const uint8_t *sp1, int32_t spitch1, const uint8_t *sp2, int32_t spitch2, int32_t hblocks, int32_t incpitch, int32_t height)
{
    __m128i xmm0 = _mm_setzero_si128();
    __m128i xmm1 = _mm_setzero_si128();

    spitch2 += incpitch;
    incpitch += spitch1;

    int32_t counter = hblocks;
    do {
        __m128i xmm2 = _mm_loadu_si128((__m128i*)sp1);
        __m128i xmm3 = _mm_loadu_si128((__m128i*)(sp1+16));
        __m128i xmm4 = _mm_loadu_si128((__m128i*)sp2);
        __m128i xmm5 = _mm_loadu_si128((__m128i*)(sp2+16));

        xmm2 = _mm_sad_epu8(xmm2, xmm4);
        xmm3 = _mm_sad_epu8(xmm3, xmm5);
        xmm0 = _mm_add_epi32(xmm0, xmm2);
        xmm1 = _mm_add_epi32(xmm1, xmm3);

        sp1 += 32;
        sp2 += 32;
        if (--counter > 0) {
            continue;
        }
    } while (--height > 0);

    xmm0 = _mm_add_epi32(xmm0, xmm1);
    return (uint32_t)_mm_cvtsi128_si32(xmm0);
}

uint32_t gdiff(const uint8_t *sp1, int32_t spitch1, const uint8_t *sp2, int32_t spitch2, int32_t hblocks, int32_t incpitch, int32_t height)
{
    if((((uint32_t)sp1 & (16 - 1)) + ((uint32_t)sp2 & (16 - 1))) == 0) {
        return aligned_diff(sp1, spitch1, sp2, spitch2, hblocks, incpitch, height);
    } else {
        return unaligned_diff(sp1, spitch1, sp2, spitch2, hblocks, incpitch, height);
    }
}

void copyChroma(VSFrameRef *dest, const VSFrameRef *source, const VSVideoInfo *vi, const VSAPI *vsapi)
{
    int32_t destPitch = vsapi->getStride(dest, 1);
    int32_t srcPitch = vsapi->getStride(source, 1);
    vs_bitblt(vsapi->getWritePtr(dest, 1), destPitch, vsapi->getReadPtr(source, 1), srcPitch, vi->width >> vi->format->subSamplingW, vi->height >> vi->format->subSamplingH);
    vs_bitblt(vsapi->getWritePtr(dest, 2), destPitch, vsapi->getReadPtr(source, 2), srcPitch, vi->width >> vi->format->subSamplingW, vi->height >> vi->format->subSamplingH);
}

VS_EXTERNAL_API(void) VapourSynthPluginInit(VSConfigPlugin configFunc, VSRegisterFunction registerFunc, VSPlugin *plugin)
{
    configFunc("com.fakeurl.vsremovedirt", "vsrd", "RemoveDirt VapourSynth Port", VAPOURSYNTH_API_VERSION, 1, plugin);
    registerFunc("SCSelect", "input:clip;"\
                             "sceneBegin:clip;"\
                             "sceneEnd:clip;"\
                             "globalMotion:clip;"\
                             "dfactor:float:opt;", SCSelectCreate, 0, plugin);
    registerFunc("RestoreMotionBlocks", "input:clip;"\
                                        "restore:clip;"\
                                        "neighbour:clip:opt;"\
                                        "neighbour2:clip:opt;"\
                                        "alternative:clip:opt;"\
                                        "gmthreshold:int:opt;"\
                                        "mthreshold:int:opt;"\
                                        "noise:int:opt;"\
                                        "noisy:int:opt;"\
                                        "dist:int:opt;"\
                                        "tolerance:int:opt;"\
                                        "dmode:int:opt;"\
                                        "pthreshold:int:opt;"\
                                        "cthreshold:int:opt;"\
                                        "grey:int:opt;", RestoreMotionBlocksCreate, 0, plugin);
    registerFunc("DupBlocks", "input:clip;"\
                              "gmthreshold:int:opt;"\
                              "mthreshold:int:opt;"\
                              "noise:int:opt;"\
                              "noisy:int:opt;"\
                              "dist:int:opt;"\
                              "tolerance:int:opt;"\
                              "dmode:int:opt;"\
                              "pthreshold:int:opt;"\
                              "cthreshold:int:opt;"\
                              "grey:int:opt;", DupBlocksCreate, 0, plugin);
}
