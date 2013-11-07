#include "shared.h"

typedef struct {
    VSNodeRef *input;
    VSNodeRef *restore;
    VSNodeRef *before;
    VSNodeRef *after;
    VSNodeRef *alternative;
    const VSVideoInfo *vi;
    int32_t mthreshold;
    int32_t lastframe;
    int32_t before_offset;
    int32_t after_offset;
    RemoveDirtData rd;
} RestoreMotionBlocksData;

static void VS_CC RestoreMotionBlocksFree(void *instanceData, VSCore *core, const VSAPI *vsapi)
{
    RestoreMotionBlocksData *d = (RestoreMotionBlocksData *)instanceData;
    vsapi->freeNode(d->input);
    vsapi->freeNode(d->restore);
    vsapi->freeNode(d->before);
    vsapi->freeNode(d->after);
    vsapi->freeNode(d->alternative);
    free(d);
}

static const VSFrameRef *VS_CC RestoreMotionBlocksGetFrame(int32_t n, int32_t activationReason, void **instanceData, void **frameData, VSFrameContext *frameCtx, VSCore *core, const VSAPI *vsapi)
{
    RestoreMotionBlocksData *d = (RestoreMotionBlocksData *) *instanceData;

    if (activationReason == arInitial) {
        vsapi->requestFrameFilter(n, d->input, frameCtx);
        vsapi->requestFrameFilter(n, d->restore, frameCtx);
        vsapi->requestFrameFilter(n, d->before, frameCtx);
        vsapi->requestFrameFilter(n, d->after, frameCtx);
        vsapi->requestFrameFilter(n, d->alternative, frameCtx);
    } else if (activationReason == arAllFramesReady) {        
        if ((n + d->before_offset < 0) || (n + d->after_offset > d->lastframe)) {
            return vsapi->getFrameFilter(n, d->alternative, frameCtx);
        }

        const VSFrameRef *pf = vsapi->getFrameFilter(n + d->before_offset, d->before, frameCtx);
        const VSFrameRef *df = vsapi->getFrameFilter(n, d->input, frameCtx);
        VSFrameRef *df_copy = vsapi->copyFrame(df, core);
        const VSFrameRef *rf = vsapi->getFrameFilter(n, d->restore, frameCtx);
        const VSFrameRef *nf = vsapi->getFrameFilter(n + d->after_offset, d->after, frameCtx);

        if(RemoveDirtProcessFrame(&d->rd, df_copy, rf, pf, nf, n, vsapi) > d->mthreshold) {
            vsapi->freeFrame(pf);
            vsapi->freeFrame(df);
            vsapi->freeFrame(rf);
            vsapi->freeFrame(nf);
            vsapi->freeFrame(df_copy);
            return vsapi->getFrameFilter(n, d->alternative, frameCtx);
        } else {
            vsapi->freeFrame(pf);
            vsapi->freeFrame(df);
            vsapi->freeFrame(rf);
            vsapi->freeFrame(nf);
            return df_copy;
        }
    }

    return 0;
}

static void VS_CC RestoreMotionBlocksInit(VSMap *in, VSMap *out, void **instanceData, VSNode *node, VSCore *core, const VSAPI *vsapi)
{
    RestoreMotionBlocksData *d = (RestoreMotionBlocksData *) *instanceData;
    vsapi->setVideoInfo(d->vi, 1, node);
}

void VS_CC RestoreMotionBlocksCreate(const VSMap *in, VSMap *out, void *userData, VSCore *core, const VSAPI *vsapi)
{
    RestoreMotionBlocksData d;

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

    d.restore = vsapi->propGetNode(in, "restore", 0, 0);

    int32_t err;
    d.after = vsapi->propGetNode(in, "neighbour", 0, &err);
    if (err) {
        d.after = 0;
    }

    d.before = vsapi->propGetNode(in, "neighbour2", 0, &err);
    if (err) {
        d.before = 0;
    }

    d.alternative = vsapi->propGetNode(in, "alternative", 0, &err);
    if (err) {
        d.alternative = 0;
    }

    d.mthreshold = vsapi->propGetInt(in, "gmthreshold", 0, &err);
    if (err) {
        d.mthreshold = 80;
    }
    d.mthreshold = (d.mthreshold * d.rd.pp.mdd.md.hblocks * d.rd.pp.mdd.md.vblocks) / 100;

    d.lastframe = d.vi->numFrames - 1;
    d.before_offset = d.after_offset = 0; 
    if(d.after == 0) {
        d.after = d.restore;
        goto set_before;
    }
    if (d.before == 0) {
set_before:
        d.before_offset = -1;
        d.after_offset = 1;
        d.before = d.after;
    }
    if(d.alternative == 0) {
        d.alternative = d.restore;
    }

    if (!isSameFormat(d.vi, vsapi->getVideoInfo(d.restore)) ||
        !isSameFormat(d.vi, vsapi->getVideoInfo(d.before)) ||
        !isSameFormat(d.vi, vsapi->getVideoInfo(d.after))) {
            vsapi->freeNode(d.input);
            vsapi->freeNode(d.restore);
            vsapi->freeNode(d.before);
            vsapi->freeNode(d.after);
            vsapi->freeNode(d.alternative);
            vsapi->setError(out, "SCSelect: Clips are not of equal type");
            return;
    }

    FillRemoveDirt(in, out, vsapi, &d.rd, vsapi->getVideoInfo(d.input));

    RestoreMotionBlocksData *data = (RestoreMotionBlocksData *)malloc(sizeof(d));
    *data = d;

    vsapi->createFilter(in, out, "RestoreMotionBlocks", RestoreMotionBlocksInit, RestoreMotionBlocksGetFrame, RestoreMotionBlocksFree, fmParallel, 0, data, core);
}

/*RestoreMotionBlocks(AVSValue args, IScriptEnvironment *env)
                : GenericVideoFilterCopy(args[SRC].AsClip(), args[GREY].AsBool(false) && !args[SHOW].AsBool(false)), restore(args[RESTORE].AsClip())
                        , after(args[AFTER].Defined() ? args[AFTER].AsClip() : NULL)
                        , before(args[BEFORE].Defined() ? args[BEFORE].AsClip() : NULL)
                        , alternative(args[ALTERNATIVE].Defined() ? args[ALTERNATIVE].AsClip() : NULL)
                        , rd(vi.width, vi.height, args[DIST].AsInt(DEFAULT_DIST), args[TOLERANCE].AsInt(DEFAULT_TOLERANCE), args[DMODE].AsInt(0)
                                        , args[MTHRES].AsInt(DEFAULT_MTHRESHOLD), args[NOISE].AsInt(0), args[NOISY].AsInt(-1), vi.IsYUY2()
                                        , args[PTHRES].AsInt(DEFAULT_PTHRESHOLD), args[CTHRES].AsInt(DEFAULT_PTHRESHOLD)
                                        , args[GREY].AsBool(false), args[SHOW].AsBool(false), args[DEBUG].AsBool(false), env)
        {
                mthreshold = (args[GMTHRES].AsInt(DEFAULT_GMTHRESHOLD) * rd.hblocks * rd.vblocks) / 100;
                if( vi.IsRGB() || (vi.IsYV12() + args[PLANAR].AsBool(false) == 0) )
                        env->ThrowError("RemoveDirt: only YV12 and planar YUY2 clips are supported");
                child->SetCacheHints(CACHE_NOTHING, 0);
                restore->SetCacheHints(CACHE_RANGE, 0);
                lastframe = vi.num_frames - 1;
                before_offset = after_offset = 0; 
                if( after == NULL )
                {
                        after = restore;
                        goto set_before;
                }
                if( before != NULL )
                {
                        after->SetCacheHints(CACHE_RANGE, 0);
                        before->SetCacheHints(CACHE_RANGE, 0);
                }
                else
                {
                        set_before:
                        before_offset = -1;
                        after_offset = 1;
                        before = after;
                        after->SetCacheHints(CACHE_RANGE, 2);
                }
                if( alternative == NULL ) alternative = restore;
                else alternative->SetCacheHints(CACHE_RANGE, 0);
                CompareVideoInfo(vi, restore->GetVideoInfo(), "RemoveDirt", env);
                CompareVideoInfo(vi, before->GetVideoInfo(), "RemoveDirt", env);
                CompareVideoInfo(vi, after->GetVideoInfo(), "RemoveDirt", env);
        }/*
