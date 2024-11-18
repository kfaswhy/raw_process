#include "isp_gain.h"
//#include "raw_process.h"
U8 isp_gain_process(U16* raw, int width, int height, G_CONFIG cfg)
{
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int index = y * width + x;
            raw[index] = clp_range(0, ((U32)raw[index] * cfg.isp_gain)>>10, U16MAX);
        }
    }

    return 0;
}