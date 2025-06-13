#include "gic.h"

U8 gic_process(U16* raw, IMG_CONTEXT context, G_CONFIG cfg)
{
    if (cfg.gic_on == 0)
    {
        return OK;
    }

    U16 height = context.height;
    U16 width = context.width;

    //x1 = (1-k/2)x1+(k/2)x2
    U16 g1 = 0;
    U16 g2 = 0;
    U32 index1 = 0;
    U32 index2 = 0;


    for (U16 y = 0; y < height; y+=2) 
    {
        for (U16 x = 0; x < width; x+=2) 
        {
            if (x + 1 >= width || y + 1 >= height) continue;
            // ²åÖµ¼ÆËã
            switch (cfg.pattern) {
            case RGGB:
            case BGGR:
                index1 = y * width + x + 1;
                index2 = (y + 1) * width + x;
                break;
            case GRBG:
            case GBRG:
                index1 = y * width + x;
                index2 = (y + 1) * width + x + 1;
                break;
            default:
                fprintf(stderr, "Unsupported Bayer Pattern.\n");
                return ERROR;
            }
            g1 = raw[index1];
            g2 = raw[index2];
            if ((g1 > g2 && g1 < (U16)(g2 * cfg.gic_thd)) || (g2 > g1 && g2 < (U16)(g1 * cfg.gic_thd)))
            {
                g1 = (1 - cfg.gic_str / 2) * g1 + (cfg.gic_str / 2) * g2 + 0.5;
                g2 = (1 - cfg.gic_str / 2) * g2 + (cfg.gic_str / 2) * g1 + 0.5;
                raw[index1] = g1;
                raw[index2] = g2;
            }

        }
    } 
#if DEBUG_MODE
    LOG("done.");
    RGB* rgb_data = raw2rgb(raw, context, cfg);
    save_img_with_timestamp(rgb_data, &context, "_gic");
#endif

    return OK;
}
