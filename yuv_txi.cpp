#include "yuv_txi.h"
#include "y2r.h"

using namespace std;


U8 yuv_txi_process(YUV* yuv, IMG_CONTEXT context, G_CONFIG cfg)
{
	if (cfg.yuv_txi_on == 0)
	{

		return OK;
	}

	U8 r_detail = cfg.txi_r_detail;
	U8 r_bifilter = cfg.txi_r_bifilter;
	//float y_thd_bifilter = 16 / 255;
	float str_enh = cfg.txi_str;


	U16 y_max = (1 << cfg.yuv_bit) - 1;
	U16* y = (U16*)malloc(context.full_size * sizeof(U16));
	if (!y)
	{
		LOG("Memory allocation failed.");
		return ERROR;
	}
	memcpy(y, yuv->y, context.full_size * sizeof(U16));

	U16* y_tmp = (U16*)malloc(context.full_size * sizeof(U16));
	U16 tmp_shift = 1 << (cfg.yuv_bit - 1);


#if DEBUG_MODE
	save_y("txi_0_y.bmp", y, context.width, context.height, cfg.yuv_bit, 100);
#endif

	//获得背景图
	U16* y_bg = gauss_filter(y, context.height, context.width, r_detail);
#if DEBUG_MODE
	save_y("txi_1_bg.bmp", y_bg, context.width, context.height, cfg.yuv_bit, 100);
#endif

	//提取细节图
	S32* y_detail = get_detail(y, y_bg, context.full_size, y_max);
#if  DEBUG_MODE
	for (U32 i = 0; i < context.full_size; i++)
	{
		y_tmp[i] = y_detail[i] + tmp_shift;
	}
	save_y("txi_2_det.bmp", y_tmp, context.width, context.height, cfg.yuv_bit, 100);
#endif

	//细节图去噪

	for (U32 i = 0; i < context.full_size; i++)
	{
		y_tmp[i] = y_detail[i] + tmp_shift;
	}
	y_tmp = mid_filter(y_tmp, context.height, context.width, r_bifilter);

	for (U32 i = 0; i < context.full_size; i++)
	{
		y_detail[i] = y_tmp[i] - tmp_shift;
	}

#if  DEBUG_MODE
	for (U32 i = 0; i < context.full_size; i++)
	{
		y_tmp[i] = y_detail[i] + tmp_shift;
	}
	save_y("txi_3_dn.bmp", y_tmp, context.width, context.height, cfg.yuv_bit, 100);
#endif

	//细节增强
	detail_enhance(y_detail, context.height, context.width, y_max, str_enh);

#if  DEBUG_MODE
	for (U32 i = 0; i < context.full_size; i++)
	{
		y_tmp[i] = y_detail[i] + tmp_shift;
	}
	save_y("txi_4_enh.bmp", y_tmp, context.width, context.height, cfg.yuv_bit, 100);
#endif


	//图像融合
	merge(y, y_bg, y_detail, context.full_size, y_max);

	//恢复到yuv
	memcpy(yuv->y, y, context.full_size * sizeof(U16));
	//memset(yuv->y, y, context.full_size * sizeof(U16));

#if DEBUG_MODE
	LOG("done.");
	RGB* rgb_data = y2r_process(yuv, context, cfg);
	save_img_with_timestamp(rgb_data, &context, "_yuv_txi");
#endif

	return OK;
}



S32* get_detail(U16* y, U16* y_bg, U32 full_size, U16 y_max)
{
	//提取细节图
	S32* y_detail = (S32*)malloc(full_size * sizeof(S32));
	for (U32 i = 0; i < full_size; i++)
	{
		y_detail[i] = y[i] - y_bg[i];
	}
	return y_detail;

}

U8 detail_enhance(S32* y, U16 height, U16 width, U16 y_max, float str)
{
	//细节增强
	for (U32 i = 0; i < height * width; i++)
	{
		y[i] = clp_range(-y_max, y[i] * str, y_max);
	}
	return OK;
}

U8 merge(U16* y, U16* bg, S32* detail, U32 full_size, U16 y_max)
{
	//融合Y
	for (U32 i = 0; i < full_size; i++) {
		y[i] = clp_range(0, bg[i] + detail[i], y_max);
	}
	return OK;
}
