#include "shared.h"

typedef struct {
    VSNodeRef *input;
    VSNodeRef *sceneBegin;
    VSNodeRef *sceneEnd;
    VSNodeRef *globalMotion;
    const VSVideoInfo *vi;
    int32_t dfactor;
    int32_t hblocks;
    int32_t incpitch;
    uint32_t lastdiff;
    uint32_t lnr;
    double dirmult;
} SCSelectData;

static void VS_CC SCSelectFree(void *instanceData, VSCore *core, const VSAPI *vsapi)
{
    SCSelectData *d = (SCSelectData *)instanceData;
    vsapi->freeNode(d->input);
    vsapi->freeNode(d->sceneBegin);
    vsapi->freeNode(d->sceneEnd);
    vsapi->freeNode(d->globalMotion);
    free(d);
}

static const VSFrameRef *VS_CC SCSelectGetFrame(int32_t n, int32_t activationReason, void **instanceData, void **frameData, VSFrameContext *frameCtx, VSCore *core, const VSAPI *vsapi)
{
    SCSelectData *d = (SCSelectData *) *instanceData;

    if (activationReason == arInitial) {
        vsapi->requestFrameFilter(n, d->input, frameCtx);
        vsapi->requestFrameFilter(n, d->sceneBegin, frameCtx);
        vsapi->requestFrameFilter(n, d->sceneEnd, frameCtx);
        vsapi->requestFrameFilter(n, d->globalMotion, frameCtx);
    } else if (activationReason == arAllFramesReady) {
        const VSFrameRef *input_frame = vsapi->getFrameFilter(n, d->input, frameCtx);
        const VSFrameRef *sceneBegin_frame = vsapi->getFrameFilter(n, d->sceneBegin, frameCtx);
        const VSFrameRef *sceneEnd_frame = vsapi->getFrameFilter(n, d->sceneEnd, frameCtx);
        const VSFrameRef *globalMotion_frame = vsapi->getFrameFilter(n, d->globalMotion, frameCtx);

        VSNodeRef *selected;

        if (n == 0) {
set_begin:
            selected = d->sceneBegin;
        } else if (n >= d->vi->numFrames) {
set_end:
            selected = d->sceneEnd;
        } else {
            const VSFrameRef *sf = vsapi->getFrameFilter(n, d->input, frameCtx);

            if (d->lnr != n - 1) {
                const VSFrameRef *pf = vsapi->getFrameFilter(n - 1, d->input, frameCtx);
                d->lastdiff = gdiff(vsapi->getReadPtr(sf, 0), vsapi->getStride(sf, 0),
                    vsapi->getReadPtr(pf, 0), vsapi->getStride(pf, 0),
                    d->hblocks, d->incpitch, d->vi->height);
                vsapi->freeFrame(pf);
            }
            int32_t olddiff = d->lastdiff;
            const VSFrameRef *nf = vsapi->getFrameFilter(n + 1, d->input, frameCtx);
            d->lastdiff  = gdiff(vsapi->getReadPtr(sf, 0), vsapi->getStride(sf, 0),
                vsapi->getReadPtr(nf, 0), vsapi->getStride(nf, 0),
                d->hblocks, d->incpitch, d->vi->height);
            d->lnr = n;

            vsapi->freeFrame(sf);
            vsapi->freeFrame(nf);

            SSE_EMMS
                if(d->dirmult * olddiff < d->lastdiff ) {
                    goto set_end;
                }
                if(d->dirmult * d->lastdiff < olddiff ) {
                    goto set_begin;
                }
                selected = d->globalMotion;
        }
        vsapi->freeFrame(input_frame);
        vsapi->freeFrame(sceneBegin_frame);
        vsapi->freeFrame(sceneEnd_frame);
        vsapi->freeFrame(globalMotion_frame);
        return vsapi->getFrameFilter(n, selected, frameCtx);
    }

    return 0;
}

static void VS_CC SCSelectInit(VSMap *in, VSMap *out, void **instanceData, VSNode *node, VSCore *core, const VSAPI *vsapi)
{
    SCSelectData *d = (SCSelectData *) *instanceData;
    vsapi->setVideoInfo(d->vi, 1, node);
}

void VS_CC SCSelectCreate(const VSMap *in, VSMap *out, void *userData, VSCore *core, const VSAPI *vsapi)
{
    SCSelectData d;

    d.input = vsapi->propGetNode(in, "input", 0, 0);
    d.vi = vsapi->getVideoInfo(d.input);

    if (!isConstantFormat(d.vi)) {
        vsapi->freeNode(d.input);
        vsapi->setError(out, "SCSelect: Only constant format input supported");
        return;
    }

    if (d.vi->format->id != pfYUV420P8 || d.vi->format->id != pfYUV422P8) {
        vsapi->freeNode(d.input);
        vsapi->setError(out, "SCSelect: Only planar YV12 and YUY2 colorspaces are supported");
        return;
    }

    d.sceneBegin = vsapi->propGetNode(in, "sceneBegin", 0, 0);
    d.sceneEnd = vsapi->propGetNode(in, "sceneEnd", 0, 0);
    d.globalMotion = vsapi->propGetNode(in, "globalMotion", 0, 0);

    if (!isSameFormat(d.vi, vsapi->getVideoInfo(d.sceneBegin)) ||
        !isSameFormat(d.vi, vsapi->getVideoInfo(d.sceneEnd)) ||
        !isSameFormat(d.vi, vsapi->getVideoInfo(d.globalMotion))) {
            vsapi->freeNode(d.input);
            vsapi->freeNode(d.sceneBegin);
            vsapi->freeNode(d.sceneEnd);
            vsapi->freeNode(d.globalMotion);
            vsapi->setError(out, "SCSelect: Clips are not of equal type");
            return;
    }

    int32_t err;
    double dFactor = vsapi->propGetFloat(in, "dfactor", 0, &err);
    if (err) {
        dFactor = 4.0;
    }

    d.hblocks = d.vi->width / (2 * SSE_INCREMENT);
    d.incpitch = d.hblocks * (-2 * SSE_INCREMENT);

    SCSelectData *data = (SCSelectData *)malloc(sizeof(d));
    *data = d;

    vsapi->createFilter(in, out, "SCSelect", SCSelectInit, SCSelectGetFrame, SCSelectFree, fmParallel, 0, data, core);
};
