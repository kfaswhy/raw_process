#include "r2y.h"

YUV* r2y_process(RGB* rgb, IMG_CONTEXT context, G_CONFIG cfg)
{
    YUV *yuv = (YUV*)calloc(context.height * context.width, sizeof(YUV));

    if (!yuv) return NULL;

    //const U16 max_rgb = (1 << cfg.rgb_bit) - 1;
    const U16 max_yuv = (1 << cfg.yuv_bit) - 1;

    // 遍历每个像素
    for (U32 i = 0; i < context.full_size; i++)
    {
        S64 r = rgb[i].r;
        S64 g = rgb[i].g;
        S64 b = rgb[i].b;

        // 转换为 YUV，使用整数计算

        //Y = (0.299*R + 0.587*G + 0.114*B)*2^(ybit-rbit)
        //U = (-0.1687*R − -0.3313*G + 0.5*B)*2^(ybit-rbit) + 2^(ybit-1)
        //V = (0.5*R − 0-0.4187*G − -0.0813*B)*2^(ybit-rbit) + 2^(ybit-1)

        float y_val = ((float)0.299 * r + 0.587 * g + 0.114 * b) * pow(2.0, (cfg.yuv_bit - cfg.rgb_bit));
        float u_val = ((float)-0.1687 * r + -0.3313 * g + 0.5 * b) * pow(2.0, (cfg.yuv_bit - cfg.rgb_bit)) + pow(2.0, cfg.yuv_bit - 1);
        float v_val = ((float)0.5 * r + -0.4187 * g + -0.0813* b) * pow(2.0, (cfg.yuv_bit - cfg.rgb_bit)) + pow(2.0, cfg.yuv_bit - 1);

        // 限制值在 [0, max_yuv] 范围
        yuv[i].y = clp_range(0, y_val + 0.5, max_yuv);
        yuv[i].u = clp_range(0, u_val + 0.5, max_yuv);
        yuv[i].v = clp_range(0, v_val + 0.5, max_yuv);

    }
#if DEBUG_MODE
    LOG("done.");
#endif
    return yuv;
}