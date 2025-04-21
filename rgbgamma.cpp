#include "rgbgamma.h"

U8 rgbgamma_process(RGB* rgb, IMG_CONTEXT context, G_CONFIG cfg)
{
    if (cfg.rgbgamma_on == 0)
    {
        return OK;
    }
    U32 tmp_x = 0;
    U32 tmp_y = 0;
    RGB* p_rgb = &rgb[0];


    for (int i = 0; i < context.full_size; i++)
    {
        tmp_x = (U32)p_rgb->r << 4;
        tmp_y = calc_inter(tmp_x, cfg.gamma_x, cfg.gamma_y, 49);
        p_rgb->r = clp_range(0, (tmp_y + 8) >> 4, U8MAX);

        tmp_x = (U32)p_rgb->g << 4;
        tmp_y = calc_inter(tmp_x, cfg.gamma_x, cfg.gamma_y, 49);
        p_rgb->g = clp_range(0, (tmp_y + 8) >> 4, U8MAX);

        tmp_x = (U32)p_rgb->b << 4;
        tmp_y = calc_inter(tmp_x, cfg.gamma_x, cfg.gamma_y, 49);
        p_rgb->b = clp_range(0, (tmp_y + 8) >> 4, U8MAX);

        p_rgb++;
    }
   
#if DEBUG_MODE
    LOG("done.");
    save_img_with_timestamp(rgb, &context, "_rgbgamma");
#endif
    return OK;
}