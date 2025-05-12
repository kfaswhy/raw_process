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

    const U16 max_rgb = (1 << cfg.rgb_bit) - 1;


    for (int i = 0; i < context.full_size; i++)
    {


        tmp_y = calc_inter((U32)p_rgb->r, cfg.gamma_x, cfg.gamma_y, GAMMA_LENGTH);
        p_rgb->r = clp_range(0, tmp_y, max_rgb);

        tmp_y = calc_inter((U32)p_rgb->g, cfg.gamma_x, cfg.gamma_y, GAMMA_LENGTH);
        p_rgb->g = clp_range(0, tmp_y, max_rgb);

        tmp_y = calc_inter((U32)p_rgb->b, cfg.gamma_x, cfg.gamma_y, GAMMA_LENGTH);
        p_rgb->b = clp_range(0, tmp_y, max_rgb);

        p_rgb++;
    }
   
#if DEBUG_MODE
    LOG("done.");
    save_img_with_timestamp(rgb, &context, "_rgbgamma");
#endif
    return OK;
}