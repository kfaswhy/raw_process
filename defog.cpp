#include "defog.h"
//#include "raw_process.h"
U8 defog_process(RGB* rgb, IMG_CONTEXT context, G_CONFIG cfg)
{
    if (cfg.defog_on == 0)
    {
        return OK;
    }
	//下采样比例
	U8 smp_ratio = 4;

	//大气光计算
	float light_ratio = 1.0;//大气光缩放

	//透射系数
	float defog_str = 0.15; //去雾强度

	U16 w0 = context.width;
	U16 h0 = context.height;
	U16 w1 = w0 / smp_ratio;
	U16 h1 = h0 / smp_ratio;

	U16 y_max = (1 << cfg.rgb_bit) - 1;






	//计算暗通道
	U16* img_dark = (U16*)malloc(sizeof(U16) * w0 * h0);
	calc_dark_chanel(rgb, img_dark, w0, h0);
	save_y("defog_0_dark.bmp", img_dark, w0, h0, cfg.rgb_bit, 100);

	//暗通道中值滤波
	img_dark = mid_filter(img_dark, h0, w0, 1);
	save_y("defog_1_mid.bmp", img_dark, w0, h0, cfg.rgb_bit, 100);


	// 暗通道下采样
	U16* img_darks = (U16*)malloc(sizeof(U16) * w1 * h1);
	y_sampling(img_dark, img_darks, w0, h0, w1, h1, 1);
	save_y("defog_2_smp.bmp", img_darks, w1, h1, cfg.rgb_bit, 100);

	//最小值滤波
	img_darks = min_filter(img_darks, h1, w1, 1);
	save_y("defog_3_min.bmp", img_darks, w1, h1, cfg.rgb_bit, 100);

	//原图下采样
	RGB* img_s = (RGB*)malloc(sizeof(RGB) * w1 * h1);
	rgb_sampling(rgb, img_s, w0, h0, w1, h1, 1);
	save_img("defog_4_smp.bmp", img_s, w1, h1, cfg.rgb_bit, 100);

	//估算大气光
	RGB light = calc_atmos_light(img_s, img_darks, w1, h1, light_ratio);

	//估算透射系数
	float* trans = (float*)malloc(sizeof(float) * h1 * w1);
	calc_trans(img_s, trans, img_darks, light, defog_str, w1, h1);

	//生成trans图
#if DEBUG_MODE

	U16* trans16 = (U16*)malloc(sizeof(U16) * h1 * w1);

	float k = y_max/2;
	float b = -k;
	for (U32 i = 0; i < h1 * w1; i++)
	{
		trans16[i] = (U16)(y_max * trans[i]);
	}
	save_y("defog_5_trans.jpg", trans16, w1, h1, cfg.rgb_bit, 100);
#endif

	//trans放大
	float *trans_ori = (float*)malloc(sizeof(float) * h0 * w0);
    float_sampling(trans, trans_ori, w1, h1, w0, h0, 1);
#if DEBUG_MODE

	U16* trans_ori16 = (U16*)malloc(sizeof(U16) * h0 * w0);

	for (U32 i = 0; i < h0 * w0; i++)
	{
		trans_ori16[i] = (U16)(y_max * trans_ori[i]);
	}
	save_y("defog_6_trans.jpg", trans_ori16, w0, h0, cfg.rgb_bit, 100);
#endif


    recover_img(rgb, rgb, trans_ori, light, w0, h0);
	save_img("defog_7_end.jpg", rgb, w0, h0, cfg.rgb_bit, 100);



    //释放内存
    free(img_dark);
    free(img_darks);
    free(img_s);
    free(trans);
    free(trans_ori);

#if DEBUG_MODE
    LOG("done.");
    save_img_with_timestamp(rgb, &context, "_defog");
#endif
    

    return OK;
}


void y_sampling(U16* src, U16* dst, int w1, int h1, int w2, int h2, bool method)
{
	for (int y = 0; y < h2; y++) {
		for (int x = 0; x < w2; x++) {
			// 计算在原图像中的位置
			float src_x = (float)x * w1 / w2;
			float src_y = (float)y * h1 / h2;

			if (method == 0) { // 邻近值采样
				int nearest_x = (int)(src_x + 0.5);
				int nearest_y = (int)(src_y + 0.5);

				// 边界检查
				if (nearest_x >= w1) nearest_x = w1 - 1;
				if (nearest_y >= h1) nearest_y = h1 - 1;

				dst[y * w2 + x] = src[nearest_y * w1 + nearest_x];
			}
			else { // 双线性插值
				int x1 = (int)src_x;
				int y1 = (int)src_y;
				int x2 = x1 + 1 < w1 ? x1 + 1 : x1;
				int y2 = y1 + 1 < h1 ? y1 + 1 : y1;

				float dx = src_x - x1;
				float dy = src_y - y1;

				U16 p1 = src[y1 * w1 + x1];
				U16 p2 = src[y1 * w1 + x2];
				U16 p3 = src[y2 * w1 + x1];
				U16 p4 = src[y2 * w1 + x2];

				dst[y * w2 + x] = (U16)(
					p1 * (1 - dx) * (1 - dy) +
					p2 * dx * (1 - dy) +
					p3 * (1 - dx) * dy +
					p4 * dx * dy
					);
			
			}
		}
		//print_prog(y, h2);
	}

	return;
}

void float_sampling(float* src, float* dst, int w1, int h1, int w2, int h2, bool method)
{
	for (int y = 0; y < h2; y++) {
		for (int x = 0; x < w2; x++) {
			// 计算在原图像中的位置
			float src_x = (float)x * w1 / w2;
			float src_y = (float)y * h1 / h2;

			if (method == 0) { // 邻近值采样
				int nearest_x = (int)(src_x + 0.5);
				int nearest_y = (int)(src_y + 0.5);

				// 边界检查
				if (nearest_x >= w1) nearest_x = w1 - 1;
				if (nearest_y >= h1) nearest_y = h1 - 1;

				dst[y * w2 + x] = src[nearest_y * w1 + nearest_x];
			}
			else { // 双线性插值
				int x1 = (int)src_x;
				int y1 = (int)src_y;
				int x2 = x1 + 1 < w1 ? x1 + 1 : x1;
				int y2 = y1 + 1 < h1 ? y1 + 1 : y1;

				float dx = src_x - x1;
				float dy = src_y - y1;

				float p1 = src[y1 * w1 + x1];
				float p2 = src[y1 * w1 + x2];
				float p3 = src[y2 * w1 + x1];
				float p4 = src[y2 * w1 + x2];

				dst[y * w2 + x] = (float)(
					p1 * (1 - dx) * (1 - dy) +
					p2 * dx * (1 - dy) +
					p3 * (1 - dx) * dy +
					p4 * dx * dy
					);

			}
		}
		//print_prog(y, h2);
	}

	return;
}


void rgb_sampling(RGB* src, RGB* dst, int w1, int h1, int w2, int h2, bool method)
{

	for (int y = 0; y < h2; y++) {
		for (int x = 0; x < w2; x++) {
			// 计算在原图像中的位置
			float src_x = (float)x * w1 / w2;
			float src_y = (float)y * h1 / h2;

			if (method == 0) { // 邻近值采样
				int nearest_x = (int)(src_x + 0.5);
				int nearest_y = (int)(src_y + 0.5);

				// 边界检查
				if (nearest_x >= w1) nearest_x = w1 - 1;
				if (nearest_y >= h1) nearest_y = h1 - 1;

				dst[y * w2 + x] = src[nearest_y * w1 + nearest_x];
			}
			else { // 双线性插值
				int x1 = (int)src_x;
				int y1 = (int)src_y;
				int x2 = x1 + 1 < w1 ? x1 + 1 : x1;
				int y2 = y1 + 1 < h1 ? y1 + 1 : y1;

				float dx = src_x - x1;
				float dy = src_y - y1;

				RGB p1 = src[y1 * w1 + x1];
				RGB p2 = src[y1 * w1 + x2];
				RGB p3 = src[y2 * w1 + x1];
				RGB p4 = src[y2 * w1 + x2];

				dst[y * w2 + x].r = (U16)(
					p1.r * (1 - dx) * (1 - dy) +
					p2.r * dx * (1 - dy) +
					p3.r * (1 - dx) * dy +
					p4.r * dx * dy
					);
				dst[y * w2 + x].g = (U16)(
					p1.g * (1 - dx) * (1 - dy) +
					p2.g * dx * (1 - dy) +
					p3.g * (1 - dx) * dy +
					p4.g * dx * dy
					);
				dst[y * w2 + x].b = (U16)(
					p1.b * (1 - dx) * (1 - dy) +
					p2.b * dx * (1 - dy) +
					p3.b * (1 - dx) * dy +
					p4.b * dx * dy
					);
			}
		}
		//print_prog(y, h2);
	}

	return;
}

void calc_dark_chanel(RGB* img, U16* dark, U16 w, U16 h)
{
	for (int i = 0; i < h; i++) {
		for (int j = 0; j < w; j++) {
			int index = i * w + j;
			
			U16 rgb_min = U16MAX;
			rgb_min = calc_min(rgb_min, img[index].r);
			rgb_min = calc_min(rgb_min, img[index].g);
			rgb_min = calc_min(rgb_min, img[index].b);
			rgb_min = calc_max(rgb_min, U16MIN);
			if (img[index].b > 15360 && img[index].b < 17920)
			{
				//LOG("r g b min = %u,%u,%u,%u", img[index].r, img[index].g, img[index].b, rgb_min);
			}
			dark[index] = rgb_min;
		}
	}

	return;
}


RGB calc_atmos_light(RGB* img, U16* img_dark, U16 w, U16 h, float light_ratio)
{
	U32 max_dark = 0;
	int max_i = 0;
	int max_j = 0;
	U8 max_rgb = 0;
	float ratio = 0;
	RGB light = { 0 };
	for (int i = h * 2 / 3; i < h; i++)
	{
		for (int j = 0; j < w; j++)
		{
			int index = i * w + j;
			if (img_dark[index] > max_dark)
			{
				max_i = i;
				max_j = j;
				max_dark = img_dark[index];
			}
		}
	}

	int index = max_i * w + max_j;

	max_rgb = calc_max(max_rgb, img[index].r);
	max_rgb = calc_max(max_rgb, img[index].g);
	max_rgb = calc_max(max_rgb, img[index].b);
	max_rgb = calc_min(max_rgb, U8MAX);

	ratio = (float)U8MAX / max_rgb;
	ratio = calc_min(ratio, light_ratio);

	light.r = img[index].r * ratio;
	light.g = img[index].g * ratio;
	light.b = img[index].b * ratio;

	LOG("defog_light pos=[%d,%d], max_dark=%u, light=[%u,%u,%u]",
		h - max_i - 1, max_j, max_dark, light.r, light.g, light.b);

	return light;
}

void calc_trans(RGB* img, float* trans, U16* img_dark, RGB light, float str, U16 w, U16 h)
{
	float tmp = 0.0;

	//计算img图的透射率
	for (int y = 0; y < h; y++)
	{
		for (int x = 0; x < w; x++)
		{
			U32 index = y * w + x;
			float trans_tmp = U8MAX;
			RGB* cur = &img[index];
			tmp = (float)cur->r / calc_max(light.r, 1);
			trans_tmp = calc_min(trans_tmp, tmp);
			tmp = (float)cur->g / calc_max(light.g, 1);
			trans_tmp = calc_min(trans_tmp, tmp);
			tmp = (float)cur->b / calc_max(light.b, 1);
			trans_tmp = calc_min(trans_tmp, tmp);
			
			trans_tmp = 1.0 - str * trans_tmp;
			//trans_tmp *= 255;

			trans[index] = trans_tmp;
		}
	}
	
	return;
}

void recover_img(RGB* img, RGB* img_rec, float* trans, RGB light, U16 w, U16 h)
{
	for (int y = 0; y < h; y++)
	{
		for (int x = 0; x < w; x++)
		{
			U32 index = y * w + x;
			float defog_str = 0.0;

			//defog_str = (float)calc_interpolation_array(wgt_dark, wgt_str, sizeof(wgt_dark) / sizeof(S32), img_dark[index].r) / 100;

			U32 tmp = 0;

			float t = calc_min(calc_max(trans[index], 0.001), 1);
			tmp = calc_max(img[index].r - light.r * (1 - t), 0);
			img_rec[index].r = (U16)clp_range(0, (float)tmp / t, U16MAX);

			tmp = calc_max(img[index].g - light.g * (1 - t), 0);
			img_rec[index].g = (U16)clp_range(0, (float)tmp / t, U16MAX);

			tmp = calc_max(img[index].b - light.b * (1 - t), 0);
			img_rec[index].b = (U16)clp_range(0, (float)tmp / t, U16MAX);

			/*if (x == 0)
			{	
				LOG("pos=%u,%u, t=%f, img=%u,%u,%u, rec=%u,%u,%u.", x, y,t,
					img[index].r, img[index].g, img[index].b,
					img_rec[index].r, img_rec[index].g, img_rec[index].b);
			}*/
		}
	}
}
