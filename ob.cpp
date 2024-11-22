#include "ob.h"

U8 ob_process(U16* raw, IMG_CONTEXT context, G_CONFIG cfg)
{
    if (cfg.ob_on == 0)
    {
        return OK;
    }



    float ob_gain = (float)U16MAX / (U16MAX - cfg.ob);
    for (int i = 0; i < context.full_size; i++)
    {
        raw[i] = safe_sub(raw[i], cfg.ob);
        raw[i] = clp_range(0, (U32)raw[i] * ob_gain, U16MAX);
    }
    LOG("done.");

    return OK;
}