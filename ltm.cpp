#include "ltm.h"
#include "y2r.h"

#define U16MAX 65535



U8 ltm_process(U16* raw, IMG_CONTEXT context, G_CONFIG cfg)
{
    if (cfg.ltm_on == 0) {
        return OK;
    }

    /*U8 range_r = 10;
	float str = 1.2;
	float gain_limit_max = 2;
	float gain_limit_min = 0.0;*/


	U16 y_h = context.height >> 1;
	U16 y_w = context.width >> 1;

	U16 *y = (U16*)malloc(y_h * y_w * sizeof(U16));
	U16 *y_enh = (U16*)malloc(y_h * y_w * sizeof(U16));
	float *gain_map = (float*)malloc(y_h * y_w * sizeof(float));
	U16 *gainu16 = (U16*)malloc(y_h * y_w * sizeof(U16));

	U16 y_max = (1 << cfg.bit) - 1;


	//统计亮度并下采样
	for (U16 i = 0; i < y_h; i++)
	{
		for (U16 j = 0; j < y_w; j++)
		{
			U32 index_y = i * y_w + j;
			U32 index_raw = (i << 1) * context.width + (j << 1);
			U32 Y = (raw[index_raw] + raw[index_raw + 1] +
				raw[index_raw + context.width] + raw[index_raw + context.width + 1] + 2) >> 2;

			y[index_y] = clp_range(0, Y, y_max);
		}
	}
#if DEBUG_MODE
	save_y("ltm_0_y.jpg", y, y_w, y_h, cfg.bit, 100);
#endif

	//高斯模糊
    U16 *y_gauss = gauss_filter(y, y_h, y_w, cfg.ltm_r);
#if DEBUG_MODE
    save_y("ltm_1_gauss.jpg", y_gauss, y_w, y_h, cfg.bit, 100);
#endif

	//对比放大
	for (U32 i = 0; i < y_h * y_w; i++)
	{
		S64 Y = ((S64)y[i] - y_gauss[i]) * cfg.ltm_str + y_gauss[i];
		y_enh[i] = clp_range(0, Y, y_max);
	}

#if DEBUG_MODE
	save_y("ltm_2_y_enh.jpg", y_enh, y_w, y_h, cfg.bit, 100);
#endif

	//生成gain图
	for (U32 i = 0; i < y_h * y_w; i++)
	{
		if (y[i] == 0) {
			gain_map[i] = cfg.ltm_gain_limit_max; // 避免除以零
			continue;
		}
		float gain = (float)y_enh[i] / (float)y[i];
		gain_map[i] = clp_range(cfg.ltm_gain_limit_min, gain, cfg.ltm_gain_limit_max);
	}


	//debug:：输出gain图
#if DEBUG_MODE
	float k = (y_max - 0) / (cfg.ltm_gain_limit_max - cfg.ltm_gain_limit_min);
	float b = -cfg.ltm_gain_limit_min * k;
	for (U32 i = 0; i < y_h * y_w; i++)
	{
		gainu16[i] = (U16)(k * gain_map[i] + b);
	}
	save_y("ltm_3_gain.jpg", gainu16, y_w, y_h, cfg.bit, 100);
#endif
	//应用gain并上采样
	for (U16 i = 0; i < y_h; i++)
	{
		for (U16 j = 0; j < y_w; j++)
		{
			U32 index_y = i * y_w + j;
			U32 index_raw = (i << 1) * context.width + (j << 1);

			raw[index_raw] = clp_range(0, (S64)raw[index_raw] * gain_map[index_y], y_max);
			raw[index_raw+1] = clp_range(0, (S64)raw[index_raw+1] * gain_map[index_y], y_max);
			raw[index_raw+ context.width] = clp_range(0, (S64)raw[index_raw+ context.width] * gain_map[index_y], y_max);
			raw[index_raw+ context.width +1] = clp_range(0, (S64)raw[index_raw+ context.width +1] * gain_map[index_y], y_max);

		}
	}



#if DEBUG_MODE
    LOG("ltm done.");
    RGB* rgb_data = raw2rgb(raw, context, cfg);
    save_img_with_timestamp(rgb_data, &context, "_ltm");
    free(rgb_data);
#endif

    return OK;
}