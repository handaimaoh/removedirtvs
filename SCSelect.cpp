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
        if (n == 0) {
            vsapi->requestFrameFilter(n, d->sceneBegin, frameCtx);
        } else if (n >= d->vi->numFrames) {
            vsapi->requestFrameFilter(n, d->sceneEnd, frameCtx);
        } else if (n > 0) {
            vsapi->requestFrameFilter(n, d->input, frameCtx);
            vsapi->requestFrameFilter(n - 1, d->input, frameCtx);
            if (n < d->vi->numFrames) {
                vsapi->requestFrameFilter(n + 1, d->input, frameCtx);
            }
            vsapi->requestFrameFilter(n, d->sceneBegin, frameCtx);
            vsapi->requestFrameFilter(n, d->sceneEnd, frameCtx);
            vsapi->requestFrameFilter(n, d->globalMotion, frameCtx);
        }
    } else if (activationReason == arAllFramesReady) {
        VSNodeRef *selected;

        if (n == 0) {
set_begin:
            selected = d->sceneBegin;
        } else if (n >= d->vi->numFrames) {
set_end:
            selected = d->sceneEnd;
        } else {
            const VSFrameRef *src_frame = vsapi->getFrameFilter(n, d->input, frameCtx);

            if (d->lnr != n - 1) {
                const VSFrameRef *prev_frame = vsapi->getFrameFilter(n - 1, d->input, frameCtx);
                d->lastdiff = gdiff(vsapi->getReadPtr(src_frame, 0), vsapi->getStride(src_frame, 0),
                    vsapi->getReadPtr(prev_frame, 0), vsapi->getStride(prev_frame, 0),
                    d->hblocks, d->incpitch, d->vi->height);
                vsapi->freeFrame(prev_frame);
            }

            int32_t olddiff = d->lastdiff;
            const VSFrameRef *next_frame = vsapi->getFrameFilter(n + 1, d->input, frameCtx);
            d->lastdiff = gdiff(vsapi->getReadPtr(src_frame, 0), vsapi->getStride(src_frame, 0),
                vsapi->getReadPtr(next_frame, 0), vsapi->getStride(next_frame, 0),
                d->hblocks, d->incpitch, d->vi->height);
            d->lnr = n;

            vsapi->freeFrame(src_frame);
            vsapi->freeFrame(next_frame);

            if(d->dirmult * olddiff < d->lastdiff ) {
                goto set_end;
            }
            if(d->dirmult * d->lastdiff < olddiff ) {
                goto set_begin;
            }
            selected = d->globalMotion;
        }
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
    SCSelectData d = { 0 };

    d.input = vsapi->propGetNode(in, "input", 0, 0);
    d.vi = vsapi->getVideoInfo(d.input);

    if (!isConstantFormat(d.vi)) {
        vsapi->freeNode(d.input);
        vsapi->setError(out, "SCSelect: Only constant format input supported");
        return;
    }

    if (d.vi->format->id != pfYUV420P8 && d.vi->format->id != pfYUV422P8) {
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

    d.hblocks = d.vi->width / (2 * 16);
    d.incpitch = d.hblocks * (-2 * 16);

    SCSelectData *data = (SCSelectData *)malloc(sizeof(d));
    if (!data) {
        vsapi->setError(out, "Could not allocate SCSelectData");
        return;
    }
    *data = d;

    vsapi->createFilter(in, out, "SCSelect", SCSelectInit, SCSelectGetFrame, SCSelectFree, fmParallel, 0, data, core);
};
