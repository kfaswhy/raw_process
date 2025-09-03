#include "awb.h"

U8 awb_process(U16* raw, IMG_CONTEXT context, G_CONFIG cfg)
{
	if (cfg.awb_on == 0)
	{
		return OK;
	}

    U16 height = context.height;
    U16 width = context.width;

	for (U16 y = 0; y < height; y++) {
		for (U16 x = 0; x < width; x++) {
			// ²åÖµ¼ÆËã
			switch (cfg.pattern) {
			case RGGB:
				if ((y % 2 == 0) && (x % 2 == 0)) //R
				{
					raw[y * width + x] = clp_range(0, ((U32)raw[y * width + x] * cfg.r_gain) >> 10, U16MAX);
				}
				else if ((y % 2 == 1) && (x % 2 == 1)) //B
				{
					raw[y * width + x] = clp_range(0, ((U32)raw[y * width + x] * cfg.b_gain) >> 10, U16MAX);
				}
				else
				{
					raw[y * width + x] = clp_range(0, ((U32)raw[y * width + x] * cfg.g_gain) >> 10, U16MAX);
				}
				break;
			case BGGR:
				if ((y % 2 == 0) && (x % 2 == 0)) //B
				{
					raw[y * width + x] = clp_range(0, ((U32)raw[y * width + x] * cfg.b_gain) >> 10, U16MAX);
				}
				else if ((y % 2 == 1) && (x % 2 == 1)) //R
				{
					raw[y * width + x] = clp_range(0, ((U32)raw[y * width + x] * cfg.r_gain) >> 10, U16MAX);
				}
				else
				{
					raw[y * width + x] = clp_range(0, ((U32)raw[y * width + x] * cfg.g_gain) >> 10, U16MAX);
				}
				break;
			case GRBG:
				if ((y % 2 == 0) && (x % 2 == 1)) //R
				{
					raw[y * width + x] = clp_range(0, ((U32)raw[y * width + x] * cfg.r_gain) >> 10, U16MAX);
				}
				else if ((y % 2 == 1) && (x % 2 == 0)) //B
				{
					raw[y * width + x] = clp_range(0, ((U32)raw[y * width + x] * cfg.b_gain) >> 10, U16MAX);
				}
				else //G
				{
					raw[y * width + x] = clp_range(0, ((U32)raw[y * width + x] * cfg.g_gain) >> 10, U16MAX);
				}
				break;
			case GBRG:
				if ((y % 2 == 0) && (x % 2 == 1)) //B
				{
					raw[y * width + x] = clp_range(0, ((U32)raw[y * width + x] * cfg.b_gain) >> 10, U16MAX);
				}
				else if ((y % 2 == 1) && (x % 2 == 0)) //R
				{
					raw[y * width + x] = clp_range(0, ((U32)raw[y * width + x] * cfg.r_gain) >> 10, U16MAX);
				}
				else //G
				{
					raw[y * width + x] = clp_range(0, ((U32)raw[y * width + x] * cfg.g_gain) >> 10, U16MAX);
				}
				break;
			default:
				fprintf(stderr, "Unsupported Bayer Pattern.\n");
			}
		}
	}
#if DEBUG_MODE
    LOG("done.");
    RGB * rgb_data = raw2rgb(raw, context, cfg);
    save_img_with_timestamp(rgb_data, &context, "_awb");
#endif



	return OK;
}
