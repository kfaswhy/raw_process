#include "y2r.h"

RGB* y2r_process(YUV* yuv, IMG_CONTEXT context, G_CONFIG cfg)
{
    if (!yuv) {
        std::cerr << "Invalid input parameters!" << std::endl;
        return NULL; // 错误代码
    }
    
    RGB* rgb = (RGB*)calloc(context.height * context.width, sizeof(RGB));
    const U16 max_rgb = (1 << cfg.rgb_bit) - 1;

    for (U32 i = 0; i < context.full_size; ++i) {
        // 根据 G_CONFIG::order 决定 U 和 V 的读取顺序
        float y = (float)yuv[i].y;
        float u = (float)yuv[i].u - pow(2.0, (cfg.yuv_bit - 1));
        float v = (float)yuv[i].v - pow(2.0, (cfg.yuv_bit - 1));

        float r = (y + 1.402 * v) * pow(2.0, (cfg.rgb_bit - cfg.yuv_bit));
        float g = (y - 0.3441 * u - 0.7141 * v) * pow(2.0, (cfg.rgb_bit - cfg.yuv_bit));
        float b = (y + 1.7720 * u + 0.0001 * v) * pow(2.0, (cfg.rgb_bit - cfg.yuv_bit));

        // 限制 RGB 值在 [0, 255] 范围
        rgb[i].r = clp_range(0, r + 0.5, max_rgb);
        rgb[i].g = clp_range(0, g + 0.5, max_rgb);
        rgb[i].b = clp_range(0, b + 0.5, max_rgb);
    }
    //LOG("done.");

    return rgb; 
}