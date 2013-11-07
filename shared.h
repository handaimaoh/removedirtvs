#include "VapourSynth.h"
#include "VSHelper.h"

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

#define SSESIZE         16
#define SSE_INCREMENT   16
#define SSE_MOVE        movdqu
#define SSE3_MOVE       movdqu
#define SSE_RMOVE       movdqa
#define SSE0            xmm0
#define SSE1            xmm1
#define SSE2            xmm2
#define SSE3            xmm3
#define SSE4            xmm4
#define SSE5            xmm5
#define SSE6            xmm6
#define SSE7            xmm7
#define SSE_EMMS

typedef struct {
    __declspec(align(SSESIZE)) uint8_t noiselevel[SSESIZE];
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
    uint32_t (__stdcall *blockcompare)(const uint8_t *p1, int32_t pitch1, const uint8_t *p2, int32_t pitch2);
    void (__stdcall *blockcompareSSE2)(const uint8_t *p1, const uint8_t *p2, int32_t pitch);
} MotionDetectionData;

typedef struct {
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
    void (__stdcall *processneighbours)(MotionDetectionDistData*);
    MotionDetectionData md;
} MotionDetectionDistData;

typedef struct {
    int32_t linewidthUV;
    int32_t chromaheight;
    int32_t chromaheightm;
    int32_t pthreshold;
    int32_t cthreshold;
    int32_t loops;
    int32_t restored_blocks;
    MotionDetectionDistData mdd;
} PostProcessingData;

typedef struct {
    bool grey;
    bool show;
    PostProcessingData pp;
} RemoveDirtData;

void VS_CC SCSelectCreate(const VSMap *in, VSMap *out, void *userData, VSCore *core, const VSAPI *vsapi);
void VS_CC RestoreMotionBlocksCreate(const VSMap *in, VSMap *out, void *userData, VSCore *core, const VSAPI *vsapi);
void VS_CC CreateDupBlocks(const VSMap *in, VSMap *out, void *userData, VSCore *core, const VSAPI *vsapi);

uint32_t gdiff(const uint8_t *sp1, int32_t spitch1, const uint8_t *sp2, int32_t spitch2, int32_t hblocks, int32_t incpitch, int32_t height);
void copyChroma(VSFrameRef *dest, const VSFrameRef *source, const VSVideoInfo *vi, const VSAPI *vsapi);
int32_t RemoveDirtProcessFrame(RemoveDirtData *rd, VSFrameRef *dest, const VSFrameRef *src, const VSFrameRef *previous, const VSFrameRef *next, int32_t frame, const VSAPI *vsapi);
void FillRemoveDirt(const VSMap *in, VSMap *out, const VSAPI *vsapi, RemoveDirtData *rd, const VSVideoInfo *vi);
