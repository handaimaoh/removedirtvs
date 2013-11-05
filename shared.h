#include "VapourSynth.h"
#include "VSHelper.h"

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
    int blocks;
    bool grey;
    bool show;
} RemoveDirt;

void VS_CC SCSelectCreate(const VSMap *in, VSMap *out, void *userData, VSCore *core, const VSAPI *vsapi);
void VS_CC RestoreMotionBlocksCreate(const VSMap *in, VSMap *out, void *userData, VSCore *core, const VSAPI *vsapi);
void VS_CC CreateDupBlocks(const VSMap *in, VSMap *out, void *userData, VSCore *core, const VSAPI *vsapi);

unsigned gdiff(const unsigned char *sp1, int spitch1, const unsigned char *sp2, int spitch2, int hblocks, int incpitch, int height);
void copyChroma(VSFrameRef *dest, const VSFrameRef *source, const VSVideoInfo *vi, const VSAPI *vsapi);
int RemoveDirtProcessFrame(RemoveDirt *rd, VSFrameRef *dest, const VSFrameRef *src, const VSFrameRef *previous, const VSFrameRef *next, int frame, const VSAPI *vsapi);
