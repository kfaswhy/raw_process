#include "ynr.h"
#include "y2r.h"

U8 ynr_process(YUV* yuv, IMG_CONTEXT context, G_CONFIG cfg)
{
    if (cfg.ynr_on == 0)
    {
        return OK;
    }

    //U8 ynr_r = 2;
    U16* y = yuv->y;


    U16* y_mid = mid_filter(y, context.height, context.width, cfg.ynr_r);
#if DEBUG_MODE
    save_y("ynr_0_y.jpg", y, context.width, context.height, cfg.yuv_bit, 100);
    save_y("ynr_1_y_mid.jpg", y_mid, context.width, context.height, cfg.yuv_bit, 100);
#endif

    memcpy(y, y_mid, context.full_size * sizeof(U16));

#if DEBUG_MODE
    LOG("done.");
    RGB* rgb_data = y2r_process(yuv, context, cfg);
    save_img_with_timestamp(rgb_data, &context, "_ynr");
#endif

    return OK;
}