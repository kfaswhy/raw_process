#include "ygamma.h"
#include "y2r.h"

U8 ygamma_process(YUV* yuv, IMG_CONTEXT context, G_CONFIG cfg)
{
    if (cfg.ygamma_on == 0)
    {
        return OK;
    }

    for (int i = 0; i < context.full_size; i++)
    {
        U32 tmp = cfg.gamma_y[yuv->y[i]] >> 2;
        yuv->y[i] = clp_range(0, tmp, U8MAX);
    }

#if DEBUG_MODE
    LOG("done.");
    RGB* rgb_data = y2r_process(yuv, context, cfg);
    save_img_with_timestamp(rgb_data, &context, "_ygamma");
#endif

    return OK;
}