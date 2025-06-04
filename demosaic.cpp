#include "demosaic.h"
#define CLAMP(val, min, max) ((val) < (min) ? (min) : ((val) > (max) ? (max) : (val)))

RGB* demosaic_process(U16* raw, IMG_CONTEXT context, G_CONFIG cfg) {
	// 获取图像的宽高和 Bayer Pattern
	U16 width = context.width;
	U16 height = context.height;
	BayerPattern pattern = (BayerPattern)cfg.pattern;
	ByteOrder order = (ByteOrder)cfg.order;
	const U8 raw_bit = cfg.bit;
	const int bit_shift = cfg.bit - cfg.rgb_bit;

	const U16 rgb_max = (1 << cfg.rgb_bit) - 1;
	const U16 raw_max = (1 << raw_bit) - 1;

	// 确保原始数据按大小端顺序处理
	for (U32 i = 0; i < width * height; i++) {
		if (order == LITTLE_ENDIAN) {
			raw[i] = raw[i] & raw_max; // 提取低 raw_bit 位
		}
		else if (order == BIG_ENDIAN) {
			raw[i] = ((raw[i] & 0xFF) << 8 | (raw[i] >> 8)) & raw_max;
		}
	}

	// 分配 RGB 数据的内存
	RGB* rgb_data = (RGB*)malloc(width * height * sizeof(RGB));
	if (!rgb_data) {
		fprintf(stderr, "Memory allocation for RGB data failed.\n");
		return NULL;
	}

	// 根据 Bayer Pattern 进行插值处理
	for (U16 y = 0; y < height; y++) {
		for (U16 x = 0; x < width; x++) {
			RGB pixel = { 0, 0, 0 };
			U32 val = 0;

			// 边界检测逻辑
			int left = (x > 0) ? x - 1 : x;
			int right = (x < width - 1) ? x + 1 : x;
			int top = (y > 0) ? y - 1 : y;
			int bottom = (y < height - 1) ? y + 1 : y;

			// 插值计算
			switch (pattern) {
			case RGGB:
				if ((y % 2 == 0) && (x % 2 == 0)) //R
				{
					val = raw[y * width + x] >> bit_shift;
					pixel.r = clp_range(0, val, rgb_max);

					val = ((U32)raw[y * width + left] + raw[y * width + right] +
						raw[top * width + x] + raw[bottom * width + x]) >> (2 + bit_shift);
					pixel.g = clp_range(0, val, rgb_max);

					val = ((U32)raw[top * width + left] + raw[top * width + right] +
						raw[bottom * width + left] + raw[bottom * width + right]) >> (2 + bit_shift);
					pixel.b = clp_range(0, val, rgb_max);
				}
				else if ((y % 2 == 0) && (x % 2 == 1)) //GR
				{
					val = raw[y * width + x] >> bit_shift;
					pixel.g = clp_range(0, val, rgb_max);

					val = ((U32)raw[y * width + left] + raw[y * width + right]) >> (1 + bit_shift);
					pixel.r = clp_range(0, val, rgb_max);

					val = ((U32)raw[top * width + x] + raw[bottom * width + x]) >> (1 + bit_shift);
					pixel.b = clp_range(0, val, rgb_max);
				}
				else if ((y % 2 == 1) && (x % 2 == 0)) //GB
				{
					val = raw[y * width + x] >> bit_shift;
					pixel.g = clp_range(0, val, rgb_max);

					val = ((U32)raw[top * width + x] + raw[bottom * width + x]) >> (1 + bit_shift);
					pixel.r = clp_range(0, val, rgb_max);

					val = ((U32)raw[y * width + left] + raw[y * width + right]) >> (1 + bit_shift);
					pixel.b = clp_range(0, val, rgb_max);
				}
				else //B
				{
					val = raw[y * width + x] >> bit_shift;
					pixel.b = clp_range(0, val, rgb_max);

					val = ((U32)raw[y * width + left] + raw[y * width + right] +
						raw[top * width + x] + raw[bottom * width + x]) >> (2 + bit_shift);
					pixel.g = clp_range(0, val, rgb_max);

					val = ((U32)raw[top * width + left] + raw[top * width + right] +
						raw[bottom * width + left] + raw[bottom * width + right]) >> (2 + bit_shift);
					pixel.r = clp_range(0, val, rgb_max);
				}
				break;
			case BGGR:
				if ((y % 2 == 0) && (x % 2 == 0)) //B
				{
					val = raw[y * width + x] >> bit_shift;
					pixel.b = clp_range(0, val, rgb_max);

					val = ((U32)raw[y * width + left] + raw[y * width + right] +
						raw[top * width + x] + raw[bottom * width + x]) >> (2 + bit_shift);
					pixel.g = clp_range(0, val, rgb_max);

					val = ((U32)raw[top * width + left] + raw[top * width + right] +
						raw[bottom * width + left] + raw[bottom * width + right]) >> (2 + bit_shift);
					pixel.r = clp_range(0, val, rgb_max);
				}
				else if ((y % 2 == 0) && (x % 2 == 1)) //GB
				{
					val = raw[y * width + x] >> bit_shift;
					pixel.g = clp_range(0, val, rgb_max);

					val = ((U32)raw[top * width + x] + raw[bottom * width + x]) >> (1 + bit_shift);
					pixel.r = clp_range(0, val, rgb_max);

					val = ((U32)raw[y * width + left] + raw[y * width + right]) >> (1 + bit_shift);
					pixel.b = clp_range(0, val, rgb_max);
				}
				else if ((y % 2 == 1) && (x % 2 == 0)) //GR
				{
					val = raw[y * width + x] >> bit_shift;
					pixel.g = clp_range(0, val, rgb_max);

					val = ((U32)raw[y * width + left] + raw[y * width + right]) >> (1 + bit_shift);
					pixel.r = clp_range(0, val, rgb_max);

					val = ((U32)raw[top * width + x] + raw[bottom * width + x]) >> (1 + bit_shift);
					pixel.b = clp_range(0, val, rgb_max);
				}
				else //R
				{
					val = raw[y * width + x] >> bit_shift;
					pixel.r = clp_range(0, val, rgb_max);

					val = ((U32)raw[y * width + left] + raw[y * width + right] +
						raw[top * width + x] + raw[bottom * width + x]) >> (2 + bit_shift);
					pixel.g = clp_range(0, val, rgb_max);

					val = ((U32)raw[top * width + left] + raw[top * width + right] +
						raw[bottom * width + left] + raw[bottom * width + right]) >> (2 + bit_shift);
					pixel.b = clp_range(0, val, rgb_max);
				}
				break;
				// Other patterns (GRBG, GBRG, BGGR) can be implemented similarly
			default:
				fprintf(stderr, "Unsupported Bayer Pattern.\n");
				free(rgb_data);
				return NULL;
			}

			// 保存到 RGB 数据
			rgb_data[y * width + x] = pixel;
		}
	}


#if DEBUG_MODE
	LOG("done.");
	save_img_with_timestamp(rgb_data, &context, "_demosaic");
#endif
	return rgb_data;
}