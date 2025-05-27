#include "cnr.h"
#include "y2r.h"

U8 cnr_process(YUV* yuv, IMG_CONTEXT context, G_CONFIG cfg)
{
    if (cfg.cnr_on == 0)
    {
        return OK;
    }

    //U8 ynr_r = 2;
    U16* u = yuv->u;
    U16* v = yuv->v;


    U16* u_mid = mid_filter(u, context.height, context.width, cfg.cnr_r);
    u_mid = gauss_filter(u_mid, context.height, context.width, cfg.cnr_r);
    U16* v_mid = mid_filter(v, context.height, context.width, cfg.cnr_r);
    v_mid = gauss_filter(v_mid, context.height, context.width, cfg.cnr_r);
#if DEBUG_MODE
    save_y("cnr_0_u.jpg", u, context.width, context.height, cfg.yuv_bit, 100);
    save_y("cnr_1_u_mid.jpg", u_mid, context.width, context.height, cfg.yuv_bit, 100);
    save_y("cnr_2_v.jpg", v, context.width, context.height, cfg.yuv_bit, 100);
    save_y("cnr_3_v_mid.jpg", v_mid, context.width, context.height, cfg.yuv_bit, 100);
#endif

    memcpy(u, u_mid, context.full_size * sizeof(U16));
    memcpy(v, v_mid, context.full_size * sizeof(U16));

#if DEBUG_MODE
    LOG("done.");
    RGB* rgb_data = y2r_process(yuv, context, cfg);
    save_img_with_timestamp(rgb_data, &context, "_cnr");
#endif

    return OK;
}