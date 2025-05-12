#include "ccm.h"
//#include "raw_process.h"
U8 ccm_process(RGB* rgb, IMG_CONTEXT context, G_CONFIG cfg)
{
    if (cfg.ccm_on == 0)
    {
        return OK;
    }

    const U16 max_rgb = (1 << cfg.rgb_bit) - 1;
    
    RGB* p_rgb = &rgb[0];
    float tmp = 0.0;
    float tmpr = 0.0;
    float tmpg = 0.0;
    float tmpb = 0.0;
    for (int i = 0; i < context.full_size; i++)
    {

        if (p_rgb->g > 30000)
        {
            int uuuuuu = 0;
        }

        // 应用色彩校正矩阵
        tmpr = (float)cfg.ccm[0] * p_rgb->r + cfg.ccm[1] * p_rgb->g + cfg.ccm[2] * p_rgb->b;
        tmpg = (float)cfg.ccm[3] * p_rgb->r + cfg.ccm[4] * p_rgb->g + cfg.ccm[5] * p_rgb->b;
        tmpb = (float)cfg.ccm[6] * p_rgb->r + cfg.ccm[7] * p_rgb->g + cfg.ccm[8] * p_rgb->b;

        p_rgb->r = clp_range(0, (tmpr + 0.5), max_rgb);
        p_rgb->g = clp_range(0, (tmpg + 0.5), max_rgb);
        p_rgb->b = clp_range(0, (tmpb + 0.5), max_rgb);

        p_rgb++;
    }

#if DEBUG_MODE
    printf("\n%.2f, %.2f, %.2f,", cfg.ccm[0], cfg.ccm[1], cfg.ccm[2]);
    printf("\n%.2f, %.2f, %.2f,", cfg.ccm[3], cfg.ccm[4], cfg.ccm[5]);
    printf("\n%.2f, %.2f, %.2f\n", cfg.ccm[6], cfg.ccm[7], cfg.ccm[8]);
    LOG("done.");
    save_img_with_timestamp(rgb, &context, "_ccm");
#endif
    

    return OK;
}