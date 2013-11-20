#include "shared.h"

typedef struct {
    VSNodeRef *input;
    VSFrameRef *lf;
    const VSVideoInfo *vi;
    int32_t mthreshold;
    int32_t lfnr;
    RemoveDirtData rd;
} DupBlocksData;

static void VS_CC DupBlocksFree(void *instanceData, VSCore *core, const VSAPI *vsapi)
{
    DupBlocksData *d = (DupBlocksData *)instanceData;
    vsapi->freeNode(d->input);
    vsapi->freeFrame(d->lf);
    free(d);
}

static const VSFrameRef *VS_CC DupBlocksGetFrame(int32_t n, int32_t activationReason, void **instanceData, void **frameData, VSFrameContext *frameCtx, VSCore *core, const VSAPI *vsapi)
{
    DupBlocksData *d = (DupBlocksData *) *instanceData;

    if (activationReason == arInitial) {
        vsapi->requestFrameFilter(n, d->input, frameCtx);
    } else if (activationReason == arAllFramesReady) {
        const VSFrameRef *restore_frame = vsapi->getFrameFilter(n, d->input, frameCtx);
        
        if(n - 1 != d->lfnr) {
            if( n == 0 ) {
                return restore_frame;
            }

            d->lf = vsapi->copyFrame(restore_frame, core);
        }

        if(d->rd.show) {
            copyChroma(d->lf, restore_frame, d->vi, vsapi);
        }

        if(RemoveDirtProcessFrame(&d->rd, d->lf, restore_frame, d->lf, restore_frame, vsapi, d->vi) > d->mthreshold) {
            return restore_frame;
        }

        vsapi->freeFrame(restore_frame);
        d->lfnr = n;
        return vsapi->copyFrame(d->lf, core);
    }

    return 0;
}

static void VS_CC DupBlocksInit(VSMap *in, VSMap *out, void **instanceData, VSNode *node, VSCore *core, const VSAPI *vsapi)
{
    DupBlocksData *d = (DupBlocksData *) *instanceData;
    vsapi->setVideoInfo(d->vi, 1, node);
}

void VS_CC DupBlocksCreate(const VSMap *in, VSMap *out, void *userData, VSCore *core, const VSAPI *vsapi)
{
    DupBlocksData d = { 0 };

    d.input = vsapi->propGetNode(in, "input", 0, 0);
    d.vi = vsapi->getVideoInfo(d.input);

    FillRemoveDirt(&d.rd, in, out, vsapi, vsapi->getVideoInfo(d.input));

    if (d.vi->format->id != pfYUV420P8 && d.vi->format->id != pfYUV422P8) {
        vsapi->freeNode(d.input);
        vsapi->setError(out, "SCSelect: Only planar YV12 and YUY2 colorspaces are supported");
        return;
    }

    d.lfnr = -2;

    int32_t err;
    d.mthreshold = int64ToIntS(vsapi->propGetInt(in, "gmthreshold", 0, &err));
    if (err) {
        d.mthreshold = 80;
    }
    d.mthreshold = (d.mthreshold * d.rd.pp.mdd.md.hblocks * d.rd.pp.mdd.md.vblocks) / 100;

    DupBlocksData *data = (DupBlocksData *)malloc(sizeof(d));
    if (!data) {
        vsapi->setError(out, "Could not allocate DupBlocksData");
        return;
    }
    *data = d;

    vsapi->createFilter(in, out, "DupBlocks", DupBlocksInit, DupBlocksGetFrame, DupBlocksFree, fmParallel, 0, data, core);
}
