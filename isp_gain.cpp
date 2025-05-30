#include "isp_gain.h"
//#include "raw_process.h"
U8 isp_gain_process(U16* raw, IMG_CONTEXT context, G_CONFIG cfg)
{
    if (cfg.isp_gain_on == 0)
    {
        return OK;
    }
    

    for (int i = 0; i < context.full_size; i++)
    {
        raw[i] = clp_range(0, ((U32)raw[i] * cfg.isp_gain) >> 10, U16MAX);
    }
#if DEBUG_MODE
    LOG("done.");
    RGB* rgb_data = raw2rgb(raw, context, cfg);
    save_img_with_timestamp(rgb_data, &context, "_igain");
#endif
    
    return OK;
}