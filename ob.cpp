#include "ob.h"
//#include "raw_process.h"

U8 ob_process(unsigned short* raw, int width, int height, G_CONFIG cfg)
{
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int index = y * width + x;
            raw[index] = safe_sub(raw[index], cfg.ob);
           // raw[index] *= 1;
            raw[index]= raw[index];
        }
    }
    return 0;
}