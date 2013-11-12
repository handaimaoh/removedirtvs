#include "VapourSynth.h"
#include "VSHelper.h"

#ifdef VS_TARGET_CPU_X86
#include <emmintrin.h>
#endif

#if defined(_MSC_VER)
#define _ALLOW_KEYWORD_MACROS
#define alignas(x) __declspec(align(x))
#define ALIGNED_ARRAY(decl, alignment) alignas(alignment) decl
#else
#define __forceinline inline
#define ALIGNED_ARRAY(decl, alignment) __attribute__((aligned(16))) decl
#endif

typedef struct {
    __declspec(align(16)) uint8_t noiselevel[16];
    uint8_t *blockproperties_addr;
    uint32_t threshold;
    int32_t pline;
    int32_t nline;
    int32_t motionblocks;
    uint8_t *blockproperties;
    int32_t linewidth;
    int32_t hblocks;
    int32_t vblocks;
    int32_t hblocksSSE2;
    bool remainderSSE2;
    int32_t linewidthSSE2;
    uint32_t (*blockcompare)(const uint8_t *p1, int32_t pitch1, const uint8_t *p2, int32_t pitch2, const uint8_t *noiselevel);
    void (*blockcompareSSE2)(const uint8_t *p1, const uint8_t *p2, int32_t pitch, const uint8_t *noiselevel);
} MotionDetectionData;

typedef struct MotionDetectionDistData MotionDetectionDistData;

struct MotionDetectionDistData{
    int32_t distblocks;
    uint32_t blocks;
    uint32_t tolerance;
    uint32_t *isum;
    int32_t dist;
    int32_t dist1;
    int32_t dist2;
    int32_t hint32_terior;
    int32_t vint32_terior;
    int32_t colinc;
    int32_t isumline;
    int32_t isuminc1;
    int32_t isuminc2;
    void (*processneighbours)(MotionDetectionDistData*);
    MotionDetectionData md;
};

#define Cleftdp     (-1)
#define Crightdp    3
#define Cleftsp     Cleftdp
#define Crightsp    Crightdp
#define Crightbldp  4
#define Crightblsp  Crightbldp
#define Ctopdp      (-dpitchUV)
#define Ctopsp      (-spitchUV)

typedef struct {
    int32_t linewidthUV;
    int32_t chromaheight;
    int32_t chromaheightm;
    int32_t pthreshold;
    int32_t cthreshold;
    int32_t loops;
    int32_t restored_blocks;
    int32_t (*vertical_diff_chroma)(const uint8_t *u, const uint8_t *v, int32_t pitch, const uint8_t *noiselevel);
    void (*copy_chroma)(uint8_t *destu, uint8_t *destv, int32_t dpitch, const uint8_t *srcu, const uint8_t *srcv, int32_t spitch);
    MotionDetectionDistData mdd;
} PostProcessingData;

typedef struct {
    bool grey;
    bool show;
    PostProcessingData pp;
} RemoveDirtData;

void VS_CC SCSelectCreate(const VSMap *in, VSMap *out, void *userData, VSCore *core, const VSAPI *vsapi);
void VS_CC RestoreMotionBlocksCreate(const VSMap *in, VSMap *out, void *userData, VSCore *core, const VSAPI *vsapi);
void VS_CC DupBlocksCreate(const VSMap *in, VSMap *out, void *userData, VSCore *core, const VSAPI *vsapi);

uint32_t gdiff(const uint8_t *sp1, int32_t spitch1, const uint8_t *sp2, int32_t spitch2, int32_t hblocks, int32_t incpitch, int32_t height);
void copyChroma(VSFrameRef *dest, const VSFrameRef *source, const VSVideoInfo *vi, const VSAPI *vsapi);
int32_t RemoveDirtProcessFrame(RemoveDirtData *rd, VSFrameRef *dest, const VSFrameRef *src, const VSFrameRef *previous, const VSFrameRef *next, int32_t frame, const VSAPI *vsapi);
void FillRemoveDirt(RemoveDirtData *rd, const VSMap *in, VSMap *out, const VSAPI *vsapi, const VSVideoInfo *vi);
