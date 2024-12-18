﻿#include "raw_process.h"

#include "ob.h"
#include "isp_gain.h"
#include "awb.h"
#include "ltm.h"
#include "demosaic.h"
#include "ccm.h"
#include "rgbgamma.h"
#include "r2y.h"
#include "ygamma.h"
#include "sharp.h"
#include "y2r.h"

using namespace std;
using namespace cv;


U32 time_print_prog_start = clock();
U32 time_print_prog_end;
U32 g_time_start;
U32 g_time_end;

G_CONFIG cfg = { 0 };

void load_cfg()
{
	cfg.bit = 16;
	cfg.used_bit = 10;
	cfg.order = LITTLE_ENDIAN;
	cfg.pattern = BGGR;
	cfg.width = 1440;
	cfg.height = 1048;
	
	cfg.ob_on = 1;
	cfg.isp_gain_on = 1;
	cfg.awb_on = 1;
	cfg.ltm_on = 0;
	cfg.ccm_on = 1;
	cfg.rgbgamma_on = 1;
	cfg.ygamma_on = 0;
	cfg.sharp_on = 1;

	cfg.ob = 1024 ;
	cfg.isp_gain = 1024;

	cfg.r_gain = 1400;
	cfg.b_gain = 2800;

	cfg.ltm_strength = 0.2;
	cfg.ltm_vblk = 4;
	cfg.ltm_hblk = 2;
	cfg.ltm_cst_thdr = 1;

	float ccm_tmp[9] = {
		0.9923,   0.0977, -0.1112,
		-0.1590,   1.3558, -0.2169,
		0.1667, -0.4535 ,  1.3281

	};

	U16 gamma_tmp[256] =
	{
		0,14,27,40,52,65,77,89,101,113,124,135,146,157,168,178,189,199,209,219,228,238,247,257,266,275,283,292,301,309,318,326,334,342,350,357,365,373,380,387,395,402,409,416,423,429,436,443,449,456,462,468,474,481,487,493,498,504,510,516,521,527,532,538,543,548,554,559,564,569,574,579,584,589,593,598,603,607,612,616,621,625,630,634,638,643,647,651,655,659,663,667,671,675,679,683,687,690,694,698,701,705,709,712,716,719,723,726,729,733,736,739,743,746,749,752,755,758,762,765,768,771,774,777,780,782,785,788,791,794,797,799,802,805,808,810,813,816,818,821,823,826,828,831,833,836,838,841,843,845,848,850,853,855,857,859,862,864,866,868,871,873,875,877,879,881,883,885,888,890,892,894,896,898,900,902,904,905,907,909,911,913,915,917,919,921,922,924,926,928,930,931,933,935,937,938,940,942,943,945,947,948,950,952,953,955,956,958,960,961,963,964,966,967,969,970,972,973,975,976,978,979,981,982,983,985,986,988,989,991,992,993,995,996,997,999,1000,1001,1003,1004,1005,1007,1008,1009,1010,1012,1013,1014,1015,1017,1018,1019,1020,1021,1023,1023
	};

	
	memcpy(cfg.ccm, ccm_tmp, 9 * sizeof(float));
	memcpy(cfg.gamma, gamma_tmp, 256 * sizeof(U16));
	return;
}

int main() 
{
	load_cfg();
	IMG_CONTEXT context = { 0 };
	context.width = cfg.width;
	context.height = cfg.height;
	context.full_size = context.width * context.height;

    const char* filename = "data/raw.raw";
	U16* raw = NULL;
	RGB* rgb_data = NULL;
	YUV* yuv_data = NULL;

    // 读取 RAW 数据到一维数组
    raw = readraw(filename, context, cfg);
    if (!raw)
    {
        fprintf(stderr, "读取 RAW 图像数据失败\n");
		return ERROR;
    }

#if DEBUG_MODE
	rgb_data = raw2rgb(raw, context, cfg);
	//rgb_data = demosaic_process(raw, context, cfg);
	save_rgb("0.bmp", rgb_data, context, cfg);
#endif

	//进入raw域
	ob_process(raw, context, cfg);
#if DEBUG_MODE
	rgb_data = raw2rgb(raw, context, cfg);
	save_rgb("1_ob.bmp", rgb_data, context, cfg);
#endif

	isp_gain_process(raw, context, cfg);
#if DEBUG_MODE
	rgb_data = raw2rgb(raw, context, cfg);
	save_rgb("2_isp_gain.bmp", rgb_data, context, cfg);
#endif

#if DEBUG_MODE
	rgb_data = raw2rgb(raw, context, cfg);
	save_rgb("3.0_pre_awb.bmp", rgb_data, context, cfg);
#endif
	awb_process(raw, context, cfg);
#if DEBUG_MODE
	rgb_data = raw2rgb(raw, context, cfg);
	save_rgb("3.1_awb.bmp", rgb_data, context, cfg);
#endif

	ltm_process(raw, context, cfg);
#if DEBUG_MODE
	rgb_data = raw2rgb(raw, context, cfg);
	save_rgb("4_ltm.bmp", rgb_data, context, cfg);
#endif

	rgb_data = demosaic_process(raw, context, cfg);
#if DEBUG_MODE
	save_rgb("5_demosaic.bmp", rgb_data, context, cfg);
#endif
	//进入RGB域

	rgbgamma_process(rgb_data, context, cfg);
#if DEBUG_MODE
	save_rgb("10_rgbgamma.bmp", rgb_data, context, cfg);
#endif


	ccm_process(rgb_data, context, cfg);
#if DEBUG_MODE
	save_rgb("11_ccm.bmp", rgb_data, context, cfg);
#endif


	yuv_data = r2y_process(rgb_data, context, cfg);
	//进入YUV域

	ygamma_process(yuv_data, context, cfg);
#if DEBUG_MODE
	rgb_data = y2r_process(yuv_data, context, cfg); 
	save_rgb("15_ygamma.bmp", rgb_data, context, cfg);
#endif

	sharp_process(yuv_data, context, cfg);
#if DEBUG_MODE
	rgb_data = y2r_process(yuv_data, context, cfg);
	save_rgb("20_sharp.bmp", rgb_data, context, cfg);
#endif


	rgb_data = y2r_process(yuv_data, context, cfg);
	save_rgb("99_final.bmp", rgb_data, context, cfg);

    // 释放内存
    free(raw); 
	free(rgb_data);
	free(yuv_data);

	time_print_prog_end = clock();
	LOG("time = %.2f s.", ((float)time_print_prog_end - time_print_prog_start) / 1000);


    return 0;
}

RGB* raw2rgb(U16* raw, IMG_CONTEXT context, G_CONFIG cfg) {
    // 获取图像的宽高和 Bayer Pattern
    U16 width = context.width;
    U16 height = context.height;
    BayerPattern pattern = (BayerPattern)cfg.pattern;
    ByteOrder order = (ByteOrder)cfg.order;
    const U8 bit_depth = cfg.bit;
    const int bit_shift = cfg.bit - 8;

    // 确定最大有效值
    U16 max_val = (1 << bit_depth) - 1;

    // 确保原始数据按大小端顺序处理
    for (U32 i = 0; i < width * height; i++) {
        if (order == LITTLE_ENDIAN) {
            raw[i] = raw[i] & max_val; // 提取低 bit_depth 位
        }
        else if (order == BIG_ENDIAN) {
            raw[i] = ((raw[i] & 0xFF) << 8 | (raw[i] >> 8)) & max_val;
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

            // 插值计算
            switch (pattern) {
            case RGGB:
                if ((y % 2 == 0) && (x % 2 == 0)) //R
                {
                    val = raw[y * width + x] >> bit_shift;
                    pixel.r = clp_range(0, val, U8MAX);
					pixel.g = 0;
					pixel.b = 0;
                }
                else if ((y % 2 == 0) && (x % 2 == 1)) //GR
                {
                    val = raw[y * width + x] >> bit_shift;
                    pixel.g = clp_range(0, val, U8MAX);
                    pixel.r = 0;
                    pixel.b = 0;
                }
                else if ((y % 2 == 1) && (x % 2 == 0)) //GB
                {
                    val = raw[y * width + x] >> bit_shift;
                    pixel.g = clp_range(0, val, U8MAX);
					pixel.r = 0;
					pixel.b = 0;
                }
                else //B
                {
                    val = raw[y * width + x] >> bit_shift;
                    pixel.b = clp_range(0, val, U8MAX);
					pixel.r = 0;
					pixel.g = 0;
                }
				break;
            case BGGR:
                if ((y % 2 == 0) && (x % 2 == 0)) //B
                {
					val = raw[y * width + x] >> bit_shift;
					pixel.b = clp_range(0, val, U8MAX);
					pixel.r = 0;
					pixel.g = 0;
                }
                else if ((y % 2 == 0) && (x % 2 == 1)) //GB
                {
					val = raw[y * width + x] >> bit_shift;
					pixel.g = clp_range(0, val, U8MAX);
					pixel.r = 0;
					pixel.b = 0;
                }
                else if ((y % 2 == 1) && (x % 2 == 0)) //GR
                {
					val = raw[y * width + x] >> bit_shift;
					pixel.g = clp_range(0, val, U8MAX);
					pixel.r = 0;
					pixel.b = 0;
                }
                else //R
                {
					val = raw[y * width + x] >> bit_shift;
					pixel.r = clp_range(0, val, U8MAX);
					pixel.g = 0;
					pixel.b = 0;
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

    return rgb_data;
}



U16* readraw(const char* filename, IMG_CONTEXT context, G_CONFIG cfg)
{
    int bytesPerPixel = (cfg.bit + 7) / 8;  // 计算每像素的字节数
    int dataSize = context.width * context.height * bytesPerPixel;  // 数据总大小
    U16* raw = (U16*)malloc(context.width * context.height * sizeof(U16));

    FILE* rawFile = fopen(filename, "rb");
    if (!rawFile) {
        fprintf(stderr, "无法打开文件: %s\n", filename);
        return NULL;
    }

    U8* buffer = (U8*)malloc(dataSize);
    fread(buffer, 1, dataSize, rawFile);
    fclose(rawFile);

    for (int i = 0; i < context.width * context.height; i++) {
		if (cfg.bit == 16) {
			raw[i] = cfg.order == LITTLE_ENDIAN
                ? buffer[i * 2] | (buffer[i * 2 + 1] << 8)
                : (buffer[i * 2] << 8) | buffer[i * 2 + 1];
        }
        else {
			raw[i] = buffer[i];
        }

		raw[i] = raw[i] << (cfg.bit - cfg.used_bit);
    }

    free(buffer);
    return raw;
}

U8 save_rgb(const char* filename, RGB* rgb_data, IMG_CONTEXT context, G_CONFIG cfg) {
	// 创建 OpenCV Mat 对象
	int width = context.width;
	int height = context.height;
	cv::Mat img(height, width, CV_8UC3);

	// 填充 Mat 数据
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			RGB pixel = rgb_data[y * width + x];
			img.at<cv::Vec3b>(y, x) = cv::Vec3b(pixel.b, pixel.g, pixel.r);
		}
	}

	// 保存到文件
	if (cv::imwrite(filename, img)) {
		return 1; // 保存成功
	}
	else {
		return 0; // 保存失败
	}
}

void safe_free(void* p)
{
	if (NULL != p)
	{
		free(p);
		p = NULL;
	}
	return;
}

void print_prog(U32 cur_pos, U32 tgt)
{
	time_print_prog_end = clock();
	if ((time_print_prog_end - time_print_prog_start) >= 1000)
	{
		LOG("Processing: %d%%.", cur_pos * 100 / tgt);
		time_print_prog_start = clock();
	}
	return;
}

RGB* load_bmp(const char* filename, IMG_CONTEXT* context)
{
	FILE* f_in = fopen(filename, "rb");
	if (f_in == NULL)
	{
		LOG("Cannot find %s", filename);
		return NULL;
	}

	fread(&context->fileHeader, sizeof(BITMAPFILEHEADER), 1, f_in);
	fread(&context->infoHeader, sizeof(BITMAPINFOHEADER), 1, f_in);

	context->height = context->infoHeader.biHeight;
	context->width = context->infoHeader.biWidth;

	//context->w_samp = context->width / cfg.sample_ratio;
	//context->h_samp = context->height / cfg.sample_ratio;


	int LineByteCnt = (((context->width * context->infoHeader.biBitCount) + 31) >> 5) << 2;
	//int ImageDataSize = LineByteCnt * height;
	context->PaddingSize = 4 - ((context->width * context->infoHeader.biBitCount) >> 3) & 3;
	context->pad = (BYTE*)malloc(sizeof(BYTE) * context->PaddingSize);
	RGB* img = (RGB*)malloc(sizeof(RGB) * context->height * context->width);

	if (context->infoHeader.biBitCount == 24) {
		for (int i = 0; i < context->height; i++) {
			for (int j = 0; j < context->width; j++) {
				int index = i * context->width + j;
				fread(&img[index], sizeof(RGB), 1, f_in);
			}
			if (context->PaddingSize != 0)
			{
				fread(context->pad, 1, context->PaddingSize, f_in);
			}
		}
	}
	else
	{
		LOG("Only support BMP in 24-bit.");
		return NULL;
	}

	fclose(f_in);
	return img;
}

void save_bmp(const char* filename, RGB* img, IMG_CONTEXT* context)
{
	FILE* f_out = fopen(filename, "wb");

	context->fileHeader.bfType = 0x4D42; // 'BM'
	context->fileHeader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
	context->fileHeader.bfSize = context->fileHeader.bfOffBits + context->width * context->height * sizeof(RGB);
	context->fileHeader.bfReserved1 = 0;
	context->fileHeader.bfReserved2 = 0;

	context->infoHeader.biSize = sizeof(BITMAPINFOHEADER);
	context->infoHeader.biWidth = context->width;
	context->infoHeader.biHeight = context->height;
	context->infoHeader.biPlanes = 1;
	context->infoHeader.biBitCount = 24;
	context->infoHeader.biCompression = 0;
	context->infoHeader.biSizeImage = context->width * context->height * sizeof(RGB);
	context->infoHeader.biXPelsPerMeter = 0;
	context->infoHeader.biYPelsPerMeter = 0;
	context->infoHeader.biClrUsed = 0;
	context->infoHeader.biClrImportant = 0;

	fwrite(&context->fileHeader, sizeof(context->fileHeader), 1, f_out);
	fwrite(&context->infoHeader, sizeof(context->infoHeader), 1, f_out);

	for (int i = 0; i < context->height; i++)
	{
		for (int j = 0; j < context->width; j++)
		{
			fwrite(&img[i * context->width + j], sizeof(RGB), 1, f_out);
		}
		if (context->PaddingSize != 0)
		{
			fwrite(context->pad, 1, context->PaddingSize, f_out);
		}
	}
	fclose(f_out);
	return;
}