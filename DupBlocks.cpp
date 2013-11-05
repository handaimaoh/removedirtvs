#include "shared.h"

typedef struct {
    VSNodeRef *input;
    VSFrameRef *lf;
    VSVideoInfo *vi;
    int mthreshold;
    int lfnr;
    RemoveDirt rd;
} DupBlocks;

static void VS_CC DupBlocksFree(void *instanceData, VSCore *core, const VSAPI *vsapi)
{
    DupBlocks *d = (DupBlocks *)instanceData;
    vsapi->freeNode(d->input);
    vsapi->freeFrame(d->lf);
    free(d);
}

static const VSFrameRef *VS_CC DupBlocksGetFrame(int n, int activationReason, void **instanceData, void **frameData, VSFrameContext *frameCtx, VSCore *core, const VSAPI *vsapi)
{
    DupBlocks *d = (DupBlocks *) *instanceData;

    if (activationReason == arInitial) {
        vsapi->requestFrameFilter(n, d->input, frameCtx);
    } else if (activationReason == arAllFramesReady) {
        const VSFrameRef *rf = vsapi->getFrameFilter(n, d->input, frameCtx);
        if(n - 1 != d->lfnr) {
            if( n == 0 ) {
                return rf;
            }

            d->lf = vsapi->copyFrame(rf, core);
        }

        if(d->rd.show) {
            copyChroma(d->lf, rf, d->vi, vsapi);
        }

        if(RemoveDirtProcessFrame(&d->rd, d->lf, rf, d->lf, rf, n, vsapi) > d->mthreshold) {
            return rf;
        }

        d->lfnr = n;
        return vsapi->copyFrame(d->lf, core);
    }

    return 0;
}

static void VS_CC DupBlocksInit(VSMap *in, VSMap *out, void **instanceData, VSNode *node, VSCore *core, const VSAPI *vsapi)
{
    DupBlocks *d = (DupBlocks *) *instanceData;
    vsapi->setVideoInfo(d->vi, 1, node);
}

void VS_CC DupBlocksCreate(const VSMap *in, VSMap *out, void *userData, VSCore *core, const VSAPI *vsapi)
{
    DupBlocks d;

    d.input = vsapi->propGetNode(in, "input", 0, 0);
    d.lfnr = -2;

    int err;
    d.mthreshold = vsapi->propGetInt(in, "gmthreshold", 0, &err);
    if (err) {
        d.mthreshold = 80;
    }
    d.mthreshold = (d.mthreshold * d.rd.hblocks * d.rd.vblocks) / 100;

    if (d.vi->format->id != pfYUV420P8 || d.vi->format->id != pfYUV422P8) {
        vsapi->freeNode(d.input);
        vsapi->setError(out, "SCSelect: Only planar YV12 and YUY2 colorspaces are supported");
        return;
    }

    DupBlocks *data = (DupBlocks *)malloc(sizeof(d));
    *data = d;

    vsapi->createFilter(in, out, "DupBlocks", DupBlocksInit, DupBlocksGetFrame, DupBlocksFree, fmParallel, 0, data, core);
}

class	DupBlocks : public GenericVideoFilterCopy
{
protected:
    RemoveDirt rd;
    int	mthreshold;

    int		lfnr;
    PVideoFrame	lf;

    PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env)
    {
        PVideoFrame rf = child->GetFrame(n, env);
        if( n - 1 != lfnr )
        {
            if( n == 0 ) return rf;
            lf = GetWritableChildFrame(n - 1, env);
        }

        if( rd.show ) CopyChroma(lf, rf, env);

        if( rd.ProcessFrame(lf, rf, lf, rf, n) > mthreshold ) return rf;
        lfnr = n;
        return CopyFrame(lf, env);
    }

    enum	creatargs { SRC, PLANAR, SHOW, DEBUG, GMTHRES, MTHRES, NOISE, NOISY, DIST, TOLERANCE, DMODE, PTHRES, CTHRES, GREY, REDUCEF};
public:	
    DupBlocks(AVSValue args, IScriptEnvironment *env)
        : GenericVideoFilterCopy(args[SRC].AsClip(), args[GREY].AsBool(false)), lfnr(-2)
        , rd(vi.width, vi.height, args[DIST].AsInt(DEFAULT_DIST), args[TOLERANCE].AsInt(DEFAULT_TOLERANCE), args[DMODE].AsInt(0)
        , args[MTHRES].AsInt(100), args[NOISE].AsInt(0), args[NOISY].AsInt(-1), vi.IsYUY2()
        , args[PTHRES].AsInt(DEFAULT_PTHRESHOLD), args[CTHRES].AsInt(DEFAULT_PTHRESHOLD)
        , args[GREY].AsBool(false), args[SHOW].AsBool(false), args[DEBUG].AsBool(false), env)
    {
        mthreshold = (args[GMTHRES].AsInt(80) * rd.hblocks * rd.vblocks) / 100;
        if( vi.IsRGB() || (vi.IsYV12() + args[PLANAR].AsBool(false) == 0) )
            env->ThrowError("RemoveDirt: only YV12 and planar YUY2 clips are supported");
        //child->SetCacheHints(CACHE_NOTHING, 0);
    }
};
