#include "sharp.h"
#include "y2r.h"
U8 sharp_process(YUV* yuv, IMG_CONTEXT context, G_CONFIG cfg) {
	if (cfg.sharp_on == 0) {
		return OK;
	}

	U16 y_max = (1 << cfg.yuv_bit) - 1;
	U16* y = (U16*)malloc(context.full_size * sizeof(U16));
	S32* y_em = (S32*)malloc(context.full_size * sizeof(S32));
	memcpy(y, yuv->y, context.full_size * sizeof(U16));
#if DEBUG_MODE
	save_y("sharp_0_y.jpg", y, context.width, context.height, cfg.yuv_bit, 100);
#endif

	//1. 边缘响应EM提取
	y_em = calc_edge_em_3x5(y, context.width, context.height);
#if DEBUG_MODE
	U16* em_tmp = (U16*)malloc(context.full_size * sizeof(U16));
	for (U32 i = 0; i < context.full_size; i++) 
	{
		em_tmp[i] = calc_abs(y_em[i]);
	}
	save_y("sharp_1_em.jpg", em_tmp, context.width, context.height, cfg.yuv_bit, 100);
#endif
	//2. 边缘响应映射EMLut修正
	//3. Avg 平滑替代
	//4. 边缘增强叠加
	//




#if DEBUG_MODE
	LOG("done.");
	RGB* rgb_data = y2r_process(yuv, context, cfg);
	save_img_with_timestamp(rgb_data, &context, "_sharp");
#endif
	return OK;
}

S32* calc_edge_em_3x5(U16* y, int width, int height)
{
	const int pad_w = 2;
	const int pad_h = 1;
	const int pad_width = width + pad_w * 2;
	const int pad_height = height + pad_h * 2;

	// 分配带边界填充的缓冲区
	U16* y_pad = (U16*)malloc(pad_width * pad_height * sizeof(U16));
	if (!y_pad)
		return NULL;

	// 边界反射填充
	for (int y_idx = 0; y_idx < pad_height; y_idx++) {
		int src_y = y_idx - pad_h;
		if (src_y < 0)
			src_y = -src_y - 1;
		else if (src_y >= height)
			src_y = 2 * height - src_y - 1;

		for (int x_idx = 0; x_idx < pad_width; x_idx++) {
			int src_x = x_idx - pad_w;
			if (src_x < 0)
				src_x = -src_x - 1;
			else if (src_x >= width)
				src_x = 2 * width - src_x - 1;

			y_pad[y_idx * pad_width + x_idx] = y[src_y * width + src_x];
		}
	}

	// 分配输出 EM 图
	S32* em = (S32*)malloc(width * height * sizeof(S32));
	if (!em) {
		free(y_pad);
		return NULL;
	}

	// 3x5 边缘检测核
	const int kernel[3][5] = {
		{-1, 0, -1, 0, -1},
		{-1, 0,  8, 0, -1},
		{-1, 0, -1, 0, -1}
	};

	// 卷积计算 EM
	for (int y_idx = 0; y_idx < height; y_idx++) {
		for (int x_idx = 0; x_idx < width; x_idx++) {
			S32 acc = 0;
			for (int ky = 0; ky < 3; ky++) {
				for (int kx = 0; kx < 5; kx++) {
					int yy = y_idx + ky;
					int xx = x_idx + kx;
					acc += (S32)y_pad[yy * pad_width + xx] * kernel[ky][kx];
				}
			}
			em[y_idx * width + x_idx] = acc >> 3; // 除以 8
		}
	}

	free(y_pad);
	return em;
}

U8 sharp_process_bak(YUV* yuv, IMG_CONTEXT context, G_CONFIG cfg) {
	if (cfg.sharp_on == 0) {
		return OK;
	}

	S8 k3o[9] = {
	-1,-1,-1,
	-1,8,-1,
	-1,-1,-1
	};

	S8 k3v[9] = {
	   -1,2,-1,
	   -1,2,-1,
	   -1,2,-1
	};

	S8 k3h[9] = {
	-1,-1,-1,
	2,2,2,
	-1,-1,-1
	};

	S8 k3l[9] = {
	-1,-1,2,
	-1,2,-1,
	2,-1,-1
	};

	S8 k3r[9] = {
	2,-1,-1,
	-1,2,-1,
	-1,-1,2
	};

	S8 k5o[25] = {
	-1, -1, -1, -1, -1,
	-1,	2,	2,	2, -1,
	-1,	2,	0,	2, -1,
	-1,	2,	2,	2, -1,
	-1, -1, -1, -1, -1
	};

	S8 k5v[25] = {
	-1,1,0,1,-1,
	-1,1,0,1,-1,
	-1,1,0,1,-1,
	-1,1,0,1,-1,
	-1,1,0,1,-1
	};

	S8 k5h[25] = {
	-1,-1,-1,-1,-1,
	1,1,1,1,1,
	0,0,0,0,0,
	1,1,1,1,1,
	-1,-1,-1,-1,-1
	};

	S8 k5r[25] = {
	0,	1, -2,	0,	0,
	1,	0,	2, -2,	0,
	-2,	2,	0,	2, -2,
	0, -2,	2,	0,	1,
	0,	0, -2,	1,	0

	};
	S8 k5l[25] = {
	0,	0, -2,	1,	0,
	0, -2,	2,	0,	1,
	-2,	2,	0,	2, -2,
	1,	0,	2, -2,	0,
	0,	1, -2,	0,	0

	};


	U16 y_max = (1 << cfg.yuv_bit) - 1;
	U16* y = (U16*)malloc(context.full_size * sizeof(U16));
	U16* tmp = (U16*)malloc(context.full_size * sizeof(U16));
	memcpy(y, yuv->y, context.full_size * sizeof(U16));
#if DEBUG_MODE
	save_y("sharp_0_y.bmp", y, context.width, context.height, cfg.yuv_bit, 100);
#endif


	//全向边缘小
	S32* edge_3o = (S32*)malloc(context.full_size * sizeof(S32));
	edge_detect(y, edge_3o, context.height, context.width, k3o, 3);
#if DEBUG_MODE
	for (U32 i = 0; i < context.full_size; i++) {
		//tmp[i] = edge_3o[i] / 2 + U16MAX / 2;
		tmp[i] = calc_abs(edge_3o[i]);
	}
	save_y("sharp_1_3o.jpg", tmp, context.width, context.height, cfg.yuv_bit, 100);
#endif

	//全向边缘大
	S32* edge_5o = (S32*)malloc(context.full_size * sizeof(S32));
	edge_detect(y, edge_5o, context.height, context.width, k5o, 5);
#if DEBUG_MODE
	for (U32 i = 0; i < context.full_size; i++) {
		//tmp[i] = edge_5o[i] / 2 + U16MAX / 2;
		tmp[i] = calc_abs(edge_5o[i]);
	}
	save_y("sharp_1_5o.jpg", tmp, context.width, context.height, cfg.yuv_bit, 100);
#endif

	//全向边缘
	S32* edge_o = (S32*)malloc(context.full_size * sizeof(S32));
#if DEBUG_MODE
	for (U32 i = 0; i < context.full_size; i++) {
		tmp[i] = clp_range(0, calc_abs(edge_5o[i]) + calc_abs(edge_3o[i]), U16MAX);
	}
	save_y("sharp_1_o.jpg", tmp, context.width, context.height, cfg.yuv_bit, 100);
#endif

	//竖直边缘小
	S32* edge_3v = (S32*)malloc(context.full_size * sizeof(S32));
	edge_detect(y, edge_3v, context.height, context.width, k3v, 3);
#if DEBUG_MODE
	for (U32 i = 0; i < context.full_size; i++) {
		//tmp[i] = edge_3v[i] / 2 + U16MAX / 2;
		tmp[i] = calc_abs(edge_3v[i]);
	}
	save_y("sharp_2_3v.jpg", tmp, context.width, context.height, cfg.yuv_bit, 100);
#endif

	//竖直边缘大
	S32* edge_5v = (S32*)malloc(context.full_size * sizeof(S32));
	edge_detect(y, edge_5v, context.height, context.width, k5v, 5);
#if DEBUG_MODE
	for (U32 i = 0; i < context.full_size; i++) {
		//tmp[i] = edge_5v[i] / 2 + U16MAX / 2;
		tmp[i] = calc_abs(edge_5v[i]);
	}
	save_y("sharp_2_5v.jpg", tmp, context.width, context.height, cfg.yuv_bit, 100);
#endif

	//水平边缘小
	S32* edge_3h = (S32*)malloc(context.full_size * sizeof(S32));
	edge_detect(y, edge_3h, context.height, context.width, k3h, 3);
#if DEBUG_MODE
	for (U32 i = 0; i < context.full_size; i++) {
		//tmp[i] = edge_3h[i] / 2 + U16MAX / 2;
		tmp[i] = calc_abs(edge_3h[i]);
	}
	save_y("sharp_3_3h.jpg", tmp, context.width, context.height, cfg.yuv_bit, 100);
#endif
	//水平边缘大
	S32* edge_5h = (S32*)malloc(context.full_size * sizeof(S32));
	edge_detect(y, edge_5h, context.height, context.width, k5h, 5);
#if DEBUG_MODE
	for (U32 i = 0; i < context.full_size; i++) {
		//tmp[i] = edge_5h[i] / 2 + U16MAX / 2;
		tmp[i] = calc_abs(edge_5h[i]);
	}
	save_y("sharp_3_5h.jpg", tmp, context.width, context.height, cfg.yuv_bit, 100);
#endif

	//左斜边缘小
	S32* edge_3l = (S32*)malloc(context.full_size * sizeof(S32));
	edge_detect(y, edge_3l, context.height, context.width, k3l, 3);
#if DEBUG_MODE
	for (U32 i = 0; i < context.full_size; i++) {
		//tmp[i] = edge_3l[i] / 2 + U16MAX / 2;
		tmp[i] = calc_abs(edge_3l[i]);
	}
	save_y("sharp_4_3l.jpg", tmp, context.width, context.height, cfg.yuv_bit, 100);
#endif


	//左斜边缘大
	S32* edge_5l = (S32*)malloc(context.full_size * sizeof(S32));
	edge_detect(y, edge_5l, context.height, context.width, k5l, 5);
#if DEBUG_MODE
	for (U32 i = 0; i < context.full_size; i++) {
		//tmp[i] = edge_5l[i] / 2 + U16MAX / 2;
		tmp[i] = calc_abs(edge_5l[i]);
	}
	save_y("sharp_4_5l.jpg", tmp, context.width, context.height, cfg.yuv_bit, 100);
#endif



	//右斜边缘小
	S32* edge_3r = (S32*)malloc(context.full_size * sizeof(S32));
	edge_detect(y, edge_3r, context.height, context.width, k3r, 3);
#if DEBUG_MODE
	for (U32 i = 0; i < context.full_size; i++) {
		//tmp[i] = edge_3r[i] / 2 + U16MAX / 2;
		tmp[i] = calc_abs(edge_3r[i]);
	}
	save_y("sharp_5_3r.jpg", tmp, context.width, context.height, cfg.yuv_bit, 100);
#endif

	//右斜边缘大
	S32* edge_5r = (S32*)malloc(context.full_size * sizeof(S32));
	edge_detect(y, edge_5r, context.height, context.width, k5r, 5);
#if DEBUG_MODE
	for (U32 i = 0; i < context.full_size; i++) {
		//tmp[i] = edge_5r[i] / 2 + U16MAX / 2;
		tmp[i] = calc_abs(edge_5r[i]);
	}
	save_y("sharp_5_5r.jpg", tmp, context.width, context.height, cfg.yuv_bit, 100);
#endif

#if DEBUG_MODE
	LOG("done.");
	RGB* rgb_data = y2r_process(yuv, context, cfg);
	save_img_with_timestamp(rgb_data, &context, "_sharp");
#endif
	return OK;
}

void edge_detect(U16* src, S32* dst, U16 height, U16 width, S8* kernel, U8 k_size)
{
	if (k_size % 2 == 0 || k_size < 3) {
		LOG("Invalid kernel size.");
		return;
	}

	S8 k_half = k_size / 2;

	for (U16 y = 0; y < height; y++) {
		for (U16 x = 0; x < width; x++) {
			S32 sum = 0;

			for (S8 ky = -k_half; ky <= k_half; ky++) {
				for (S8 kx = -k_half; kx <= k_half; kx++) {

					// 计算原始坐标（可能越界）
					S16 iy = y + ky;
					S16 ix = x + kx;

					// 镜像处理
					if (iy < 0) iy = -iy;
					if (iy >= height) iy = 2 * height - iy - 2;
					if (ix < 0) ix = -ix;
					if (ix >= width) ix = 2 * width - ix - 2;

					U16 pixel = src[iy * width + ix];
					S8 k_val = kernel[(ky + k_half) * k_size + (kx + k_half)];
					sum += pixel * k_val;
				}
			}

			dst[y * width + x] = sum;
		}
	}
}