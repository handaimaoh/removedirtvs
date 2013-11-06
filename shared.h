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
    __declspec(align(SSESIZE)) unsigned char noiselevel[SSESIZE];
    unsigned char *blockproperties_addr;
    unsigned threshold;
    int pline;
    int nline;
    int motionblocks;
    unsigned char *blockproperties;
    int linewidth;
    int hblocks;
    int vblocks;
} MotionDetectionData;

typedef struct {
    int distblocks;
    uint32_t blocks;
    uint32_t tolerance;
    uint32_t *isum;
    int dist;
    int dist1;
    int dist2;
    int hinterior;
    int vinterior;
    int colinc;
    int isumline;
    int isuminc1;
    int isuminc2;
    MotionDetectionData md;
} MotionDetectionDistData;

typedef struct {
    int linewidthUV;
    int chromaheight;
    int chromaheightm;
    int pthreshold;
    int cthreshold;
    int loops;
    int restored_blocks;
    MotionDetectionDistData mdd;
} PostProcessingData;

typedef struct {
    bool grey;
    bool show;
    PostProcessingData postProcessing;
} RemoveDirtData;

void VS_CC SCSelectCreate(const VSMap *in, VSMap *out, void *userData, VSCore *core, const VSAPI *vsapi);
void VS_CC RestoreMotionBlocksCreate(const VSMap *in, VSMap *out, void *userData, VSCore *core, const VSAPI *vsapi);
void VS_CC CreateDupBlocks(const VSMap *in, VSMap *out, void *userData, VSCore *core, const VSAPI *vsapi);

unsigned gdiff(const unsigned char *sp1, int spitch1, const unsigned char *sp2, int spitch2, int hblocks, int incpitch, int height);
void copyChroma(VSFrameRef *dest, const VSFrameRef *source, const VSVideoInfo *vi, const VSAPI *vsapi);
int RemoveDirtProcessFrame(RemoveDirtData *rd, VSFrameRef *dest, const VSFrameRef *src, const VSFrameRef *previous, const VSFrameRef *next, int frame, const VSAPI *vsapi);
void FillRemoveDirt(const VSMap *in, const VSAPI *vsapi, RemoveDirtData *rd, const VSVideoInfo *vi);
