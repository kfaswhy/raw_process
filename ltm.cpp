#include "ltm.h"
#include "y2r.h"

#define U16MAX 65535

WDR_RAW_CONTEXT g_wdr_raw_context = { 0 };


U8 ltm_process(U16* raw, IMG_CONTEXT context, G_CONFIG cfg)
{
    if (cfg.ltm_on == 0) {
        return OK;
    }

    float ltm_str = 0.5;
    U8 ltm_r = 1;



	U16* ori_y = (U16*)malloc(context.full_size * sizeof(U16));
	U16* dst_y_temp = (U16*)calloc(context.full_size, sizeof(U16));

	cal_ori_Y(ori_y, raw, context.width, context.height);
	save_y("wdr_ori_y.bmp", ori_y, &context, cfg, 100);

	for (int i = 0; i < context.full_size; i++)
	{
		dst_y_temp[i] = wdr_global_cruve[ori_y[i]];
	}




#if DEBUG_MODE
    LOG("done.");
    RGB* rgb_data = raw2rgb(raw, context, cfg);
    save_img_with_timestamp(rgb_data, &context, "_ltm");
#endif

    return OK;
}
   
S32 wdr_init()
{
	WDR_RAW_CONTEXT* context = &g_wdr_raw_context;
	//WDR_RAW_CONFIG_TP* config = &context->wdr_raw_config_tp;
	LOG("Enter wdr_raw_tplink_init");
	S32 i = 0, j = 0, k = 0;
	float x = 65536;
	float global_cruve_gamma[16] = { 1.0, 0.9153561, 0.84876565, 0.779089, 0.71743203, 0.65047727,
						0.58660759, 0.54341232, 0.49280807, 0.44008068, 0.39327859, 0.34194495,
						0.30352525, 0.27616544, 0.25340937, 0.23612351 };	/* 拟合mstar得到的值 */
	//GAMMA_RGB_CONFIG gamma_config;
	float map = 0;
	U16 map_floor = 0;
	float left = 0;
	float right = 0;
	float globel_gamma_str = 0.5;
	U16 lut_left = 0;
	U16 lut_right = 0;

	/* 分配内存 */
	context->wdr_coded_gamma_cruve = (U16*)malloc(BIT16_SCALE * sizeof(U16));
	context->wdr_decoded_gamma_cruve = (U16*)malloc(BIT16_SCALE * sizeof(U16));
	context->wdr_hist = (U16(*)[WDR_RAW_NUM_X][WDR_RAW_HIST_SIZE])malloc(WDR_RAW_NUM_Y *
		WDR_RAW_NUM_X * WDR_RAW_HIST_SIZE * sizeof(U16));
	context->wdr_cur_hist = (U16(*)[WDR_RAW_NUM_X][WDR_RAW_HIST_SIZE])malloc(WDR_RAW_NUM_Y *
		WDR_RAW_NUM_X * WDR_RAW_HIST_SIZE * sizeof(U16));
	if (NULL == context->wdr_coded_gamma_cruve || NULL == context->wdr_decoded_gamma_cruve ||
		NULL == context->wdr_hist || NULL == context->wdr_cur_hist)
	{
		AMS_DEBUG("Malloc fail at wdr_raw init");
		wdr_raw_deinit();
		return ERROR;
	}
	for (j = 0; j < ENV_NUM_TP; j++)
	{
		context->str_by_y_16bit[j] = (U16*)malloc(BIT16_SCALE * sizeof(U16));
		if (NULL == context->str_by_y_16bit[j])
		{
			AMS_DEBUG("Malloc fail at wdr_raw init");
			wdr_raw_deinit();
			return ERROR;
		}
	}
	for (j = 0; j < 16; j++)
	{
		context->wdr_global_cruve[j] = (U16*)malloc(BIT16_SCALE * sizeof(U16));
		if (NULL == context->wdr_global_cruve[j])
		{
			AMS_DEBUG("Malloc fail at wdr_raw init");
			wdr_raw_deinit();
			return ERROR;
		}
	}
	/* 读取gamma配置 */
	if (ERROR == linux_ds_read(GAMMA_RGB_PATH_TPLINK, (void*)(&gamma_config), sizeof(GAMMA_RGB_CONFIG)))
	{
		AMS_ERROR("read GAMMA_RGB params failed");
		return ERROR;
	}

	/* 处理读入的gamma曲线，把每条曲线插值成65536个点 */
	for (i = 0; i < BIT16MAX; ++i)
	{
		/* 用于映射，将横坐标0-255映射到0-65536 */
		map = i / (BIT16MAX / 255.0);
		map_floor = floor(map);
		left = map - map_floor;
		right = 1 - left;
		/* 将纵坐标0-1023映射到0-65536 */
		context->wdr_coded_gamma_cruve[i] = round((gamma_config.gamma_rgb_curve[map_floor] * right
			+ gamma_config.gamma_rgb_curve[map_floor + 1] * left) * 64);
	}
	/* 处理边界情况 */
	context->wdr_coded_gamma_cruve[BIT16MAX] = BIT16MAX;
	/* 计算反gamma */
	/* 为与行处理保持同步，改为使用插值后的曲线 */
	lut_right = 0;
	lut_left = 0;
	for (i = 0; i < BIT16_SCALE; i += 256)
	{
		while ((lut_right < BIT16MAX) && (interpolation_256_16bit(lut_right, gamma_config.gamma_rgb_curve) * 65535.0 / 1023.0 < (i + 256)))
		{
			lut_right++;
		}
		if (65280 == i)
		{
			/* 为了保持和行处理的结果一致 */
			lut_right = 65535;
		}
		for (j = 0; j < 256; j++)
		{
			context->wdr_decoded_gamma_cruve[i + j] = (lut_left * (256 - j) + lut_right * j) >> 8;
		}
		lut_left = lut_right;
	}

	/* 初始化全局wdr映射曲线 */
	/* 为与行处理保持同步，改为使用插值后的曲线 */
	for (j = 0; j < 16; j++)
	{
		lut_right = 0;
		lut_left = gamma_config.gamma_rgb_curve[0] * 65535.0 / 1023.0;
		for (i = 0; i < BIT16_SCALE; i += 256)
		{
			if (65280 == i)
			{
				/* 为了保持和行处理的结果一致 */
				lut_right = 65535;
			}
			else
			{
				map = (globel_gamma_str * pow((i + 256) / x, global_cruve_gamma[j]) +
					(1 - globel_gamma_str) * pow((i + 256) / x, 1.0 / global_cruve_gamma[j])) * 255.0;;
				map_floor = floor(map);
				left = map - map_floor;
				right = 1 - left;
				lut_right = round((gamma_config.gamma_rgb_curve[map_floor] * right
					+ gamma_config.gamma_rgb_curve[map_floor + 1] * left) * 65535.0 / 1023.0);
			}
			for (k = 0; k < 256; k++)
			{
				context->wdr_global_cruve[j][i + k] = (lut_left * (256 - k) + lut_right * k) >> 8;
			}
			lut_left = lut_right;
		}
	}

	/* 读取默认配置 */
	if (ERROR == linux_ds_read(WDR_RAW_PATH_TPLINK, (void*)config, sizeof(WDR_RAW_CONFIG_TP)))
	{
		AMS_ERROR("Read WDR_RAW params failed");
		return ERROR;
	}

	/* 处理读入的str_by_y曲线，把每条曲线插值成16bit */
	for (j = 0; j < ENV_NUM_TP; j++)
	{
		if (ERROR == wdr_raw_interpolation(config->wdr_str_by_y[j], context->str_by_y_16bit[j], 8, 8, 33, 65536))
		{
			AMS_ERROR("Interpolate failed!");
			return ERROR;
		}
	}

	return OK;
}



void cal_ori_Y(U16* ori_y, U16* data, U16 width, U16 height)
{
	S32 i = 0, j = 0;
	U32 Y = 0;
	U16* data_line_1 = data;
	U16* data_line_2 = data + width;
	U16* y_line_1 = ori_y;
	U16* y_line_2 = ori_y + width;

	for (i = 0; i < height; i += 2)
	{
		for (j = 0; j < width; j += 2)
		{
			Y = *data_line_1++;
			Y += *data_line_1++;
			Y += *data_line_2++;
			Y += *data_line_2++;
			/* 取平均，四舍五入取整 */
			Y = (Y + 2) >> 2;
			*y_line_1++ = Y;
			*y_line_1++ = Y;
			*y_line_2++ = Y;
			*y_line_2++ = Y;
		}
		data_line_1 += width;
		data_line_2 += width;
		y_line_1 += width;
		y_line_2 += width;
	}

	return;
}