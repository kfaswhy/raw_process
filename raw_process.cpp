﻿#include "raw_process.h"

#include "ob.h"
#include "lsc.h"
#include "isp_gain.h"
#include "awb.h"
#include "ltm.h"
#include "demosaic.h"
#include "ccm.h"
#include "rgbgamma.h"
#include "r2y.h"
#include "ygamma.h"
#include "yuv_txi.h"
#include "sharp.h"
#include "y2r.h"

using namespace std;
using namespace cv;

namespace fs = std::filesystem;
U32 time_print_prog_start;
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

	cfg.rgb_bit = 16;
	cfg.yuv_bit = 16;
	
	cfg.ob_on = 1;
	cfg.lsc_on = 1;
	cfg.isp_gain_on = 0;
	cfg.awb_on = 1;
	cfg.ltm_on = 1;
	cfg.ccm_on = 1;
	cfg.rgbgamma_on = 1;
	//cfg.ygamma_on = 0;
	//cfg.sharp_on = 0;
	cfg.yuv_txi_on = 1;

	//换算到16bit
	cfg.ob = 64*16;

	cfg.isp_gain = 1024*2;

	cfg.r_gain = 1024 * 2.17;
	cfg.g_gain = 1024 * 1;
	cfg.b_gain = 1024 * 1.14;



	float ccm_tmp[9] = {
1.24, -0.01, -0.24,
-0.19, 1.27, -0.13,
-0.06, -0.26, 1.37


	};

	U32 gamma_xtmp[GAMMA_LENGTH] =
	{
		0,1,2,3,4,5,6,7,8,10,12,14,16,20,24,28,32,40,48,56,64,80,96,112,128,160,192,224,256,320,384,448,512,640,768,896,1024,1280,1536,1792,2048,2304,2560,2816,3072,3328,3584,3840,4095
	};

	U32 gamma_ytmp[GAMMA_LENGTH] =
	{
		0,6,11,17,22,28,33,39,44,55,66,77,88,109,130,150,170,210,248,286,323,393,460,525,586,702,809,909,1002,1172,1323,1461,1587,1810,2003,2173,2325,2589,2812,3010,3191,3355,3499,3624,3736,3836,3927,4012,4095
	};

	memcpy(cfg.ccm, ccm_tmp, 9 * sizeof(float));

	if (cfg.rgb_bit > 12)
	{
		U8 shift = cfg.rgb_bit - 12;
		for (int i = 0; i < GAMMA_LENGTH; i++)
		{
			cfg.gamma_x[i] = gamma_xtmp[i] << shift;
			cfg.gamma_y[i] = gamma_ytmp[i] << shift;
		}
	}
	else if (cfg.rgb_bit < 12)
	{
		U8 shift = 12 - cfg.rgb_bit;
		for (int i = 0; i < GAMMA_LENGTH; i++)
		{
			cfg.gamma_x[i] = gamma_xtmp[i] >> shift;
			cfg.gamma_y[i] = gamma_ytmp[i] >> shift;
		}
	}

	return;
}

int main() 
{
	clear_tmp();
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

	time_print_prog_start = clock();
	//进入raw域
	ob_process(raw, context, cfg);
	lsc_process(raw, context, cfg);
	isp_gain_process(raw, context, cfg);
	awb_process(raw, context, cfg);
	ltm_process(raw, context, cfg);

	rgb_data = demosaic_process(raw, context, cfg);

	//进入RGB域
	ccm_process(rgb_data, context, cfg);
	rgbgamma_process(rgb_data, context, cfg);
	yuv_data = r2y_process(rgb_data, context, cfg);

	//进入YUV域
	yuv_txi_process(yuv_data, context, cfg);
	//ygamma_process(yuv_data, context, cfg);
	//sharp_process(yuv_data, context, cfg);

	//结束
	rgb_data = y2r_process(yuv_data, context, cfg);
	time_print_prog_end = clock();
	LOG("time = %.2f s.", ((float)time_print_prog_end - time_print_prog_start) / 1000);

	//保存到本地
	save_img_with_timestamp(rgb_data, &context, "_end");
    free(raw); 
	free(rgb_data);
	free(yuv_data);

    return 0;
}



void clear_tmp() {
	std::string extensions[] = { ".jpg", ".png", ".bmp" };

	for (const auto& entry : fs::directory_iterator(".")) {
		if (fs::is_regular_file(entry.path())) { // Use fs::is_regular_file
			std::string file_ext = entry.path().extension().string();

			for (const auto& ext : extensions) {
				if (file_ext == ext) {
					try {
						fs::remove(entry.path());
						//std::cout << "删除文件: " << entry.path() << std::endl;
					}
					catch (const std::exception& e) {
						std::cerr << "删除文件失败: " << entry.path() << " 错误: " << e.what() << std::endl;
					}
					break;
				}
			}
		}
	}
}

RGB* raw2rgb(U16* raw, IMG_CONTEXT context, G_CONFIG cfg) {
    // 获取图像的宽高和 Bayer Pattern
    U16 width = context.width;
    U16 height = context.height;
    BayerPattern pattern = (BayerPattern)cfg.pattern;
    ByteOrder order = (ByteOrder)cfg.order;
    const U8 bit_depth = cfg.bit;
	const int bit_shift = cfg.bit - cfg.rgb_bit;
	const U16 max_rgb = (1 << cfg.rgb_bit) - 1;
	const U16 max_val = (1 << bit_depth) - 1;

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
                    pixel.r = clp_range(0, val, max_rgb);
					pixel.g = 0;
					pixel.b = 0;
                }
                else if ((y % 2 == 0) && (x % 2 == 1)) //GR
                {
                    val = raw[y * width + x] >> bit_shift;
                    pixel.g = clp_range(0, val, max_rgb);
                    pixel.r = 0;
                    pixel.b = 0;
                }
                else if ((y % 2 == 1) && (x % 2 == 0)) //GB
                {
                    val = raw[y * width + x] >> bit_shift;
                    pixel.g = clp_range(0, val, max_rgb);
					pixel.r = 0;
					pixel.b = 0;
                }
                else //B
                {
                    val = raw[y * width + x] >> bit_shift;
                    pixel.b = clp_range(0, val, max_rgb);
					pixel.r = 0;
					pixel.g = 0;
                }
				break;
            case BGGR:
                if ((y % 2 == 0) && (x % 2 == 0)) //B
                {
					val = raw[y * width + x] >> bit_shift;
					pixel.b = clp_range(0, val, max_rgb);
					pixel.r = 0;
					pixel.g = 0;
                }
                else if ((y % 2 == 0) && (x % 2 == 1)) //GB
                {
					val = raw[y * width + x] >> bit_shift;
					pixel.g = clp_range(0, val, max_rgb);
					pixel.r = 0;
					pixel.b = 0;
                }
                else if ((y % 2 == 1) && (x % 2 == 0)) //GR
                {
					val = raw[y * width + x] >> bit_shift;
					pixel.g = clp_range(0, val, max_rgb);
					pixel.r = 0;
					pixel.b = 0;
                }
                else //R
                {
					val = raw[y * width + x] >> bit_shift;
					pixel.r = clp_range(0, val, max_rgb);
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

#if DEBUG_MODE
	RGB* rgb_data = raw2rgb(raw, context, cfg);
	save_img_with_timestamp(rgb_data, &context, "_origin");
#endif

    return raw;
}

RGB* yyy2rgb_process(YUV* yuv, IMG_CONTEXT context, G_CONFIG cfg)
{
	if (!yuv) {
		std::cerr << "Invalid input parameters!" << std::endl;
		return NULL; // 错误代码
	}

	RGB* rgb = (RGB*)calloc(context.height * context.width, sizeof(RGB));

	RGB* tmp = rgb;
	for (U32 i = 0; i < context.full_size; ++i) 
	{
		yuv->y[i] = clp_range(0, yuv->y[i], 255);
		tmp->r = yuv->y[i];
		tmp->g = yuv->y[i];
		tmp->b = yuv->y[i];
		tmp++;
	}
	LOG("done.");

	return rgb;
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

U32 calc_inter(U32 x0, U32* x, U32* y, U32 len)
{
	U32 y0 = 0;

	if (len < 2) {
		// 长度小于2无法插值
		return y0;
	}

	// 判断递增还是递减
	int increasing = (x[len-1] > x[0]) ? 1 : 0;

	// 寻找x0所在的区间
	for (U32 i = 0; i < len - 1; i++) {
		if ((increasing && x0 >= x[i] && x0 <= x[i + 1]) ||
			(!increasing && x0 <= x[i] && x0 >= x[i + 1])) {
			// 线性插值计算y0

			if(x[i] == x[i + 1]) {
				// 避免除以零
				y0 = y[i];
				return y0;
			}

			U32 x1 = x[i], x2 = x[i + 1];
			U32 y1 = y[i], y2 = y[i + 1];

			y0 = y1 + (y2 - y1) * (x0 - x1) / (x2 - x1);
			return y0;
		}
	}

	// 如果x0不在范围内，返回边界值
	if (increasing) {
		y0 = (x0 < x[0]) ? y[0] : y[len - 1];
	}
	else {
		y0 = (x0 > x[0]) ? y[0] : y[len - 1];
	}

	return y0;
}

void save_y(const char* filename, U16* y, U16 width, U16 height, U8 bit, int compression_quality = 100)
{
	int total = width * height;

	// 创建 OpenCV 灰度图像
	cv::Mat gray_img(height, width, CV_8UC1);
	U8* dst = gray_img.data;

	// 判断是否需要归一化
	if (bit == 8) {
		for (int i = 0; i < total; ++i) {
			dst[i] = static_cast<U8>(y[i]);
		}
	}
	else {
		int max_val = (1 << bit) - 1;
		for (int i = 0; i < total; ++i) {
			// 四舍五入归一化到 0~255
			dst[i] = static_cast<U8>((y[i] * 255 + max_val / 2) / max_val);
		}
	}

	// 设置保存参数
	std::vector<int> compression_params;
	if (strstr(filename, ".jpg") || strstr(filename, ".jpeg")) {
		compression_params.push_back(cv::IMWRITE_JPEG_QUALITY);
		compression_params.push_back(compression_quality); // 1 ~ 100
	}

	// 保存图像
	if (!cv::imwrite(filename, gray_img, compression_params)) {
		fprintf(stderr, "Failed to write image: %s\n", filename);
	}
}


void save_img(const char* filename, RGB* img, IMG_CONTEXT* context, G_CONFIG cfg, int compression_quality = 100)
{
	// 创建一个空的 OpenCV Mat 对象
	cv::Mat mat_img(context->height, context->width, CV_8UC3);

	// 填充 Mat 对象
	for (int y = 0; y < context->height; y++) {
		for (int x = 0; x < context->width; x++) {
			RGB pixel = img[y * context->width + x];

			if (cfg.rgb_bit >= 8)
			{
				mat_img.at<cv::Vec3b>(y, x) =
					cv::Vec3b(pixel.b >> (cfg.rgb_bit - 8), pixel.g >> (cfg.rgb_bit - 8), pixel.r >> (cfg.rgb_bit - 8));
			}
			else
			{
				mat_img.at<cv::Vec3b>(y, x) =
					cv::Vec3b(pixel.b << (8 - cfg.rgb_bit), pixel.g << (8 - cfg.rgb_bit), pixel.r << (8 - cfg.rgb_bit));
			}

		}
	}

	// 设置图像保存参数，例如压缩质量
	std::vector<int> params;
	params.push_back(cv::IMWRITE_JPEG_QUALITY);  // 对 JPEG 格式指定压缩质量
	params.push_back(compression_quality);

	// 使用 OpenCV 保存图像，传递参数来设置压缩质量
	bool success = cv::imwrite(filename, mat_img, params);

	if (success) {
		LOG("%s saved (Q=%d)", filename, compression_quality);
	}
	else {
		LOG("Failed to save %s.", filename);
	}
}

void save_img_with_timestamp(RGB* rgb_data, IMG_CONTEXT* context, const char* suffix) {
	// 获取当前时间戳
	SYSTEMTIME st;
	GetSystemTime(&st);

	// 格式化时间戳
	char buffer[80];
	snprintf(buffer, sizeof(buffer), "%02d%02d%02d%03d", st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);

	// 生成文件名
	char filename[100];
	snprintf(filename, sizeof(filename), "%s%s.jpg", buffer, suffix);

	// 保存 BMP 文件
	save_img(filename, rgb_data, context, cfg);
}


static float* gen_gauss_kernel(U8 r, float sigma)
{
	U8 size = 2 * r + 1;
	float* kernel = (float*)malloc(size * size * sizeof(float));
	float sum = 0.0f;
	float s2 = 2 * sigma * sigma;
	int index = 0;

	for (int y = -r; y <= r; y++) {
		for (int x = -r; x <= r; x++) {
			float value = expf(-(x * x + y * y) / s2);
			kernel[index++] = value;
			sum += value;
		}
	}

	for (int i = 0; i < size * size; i++) {
		kernel[i] /= sum;
	}

	return kernel;
}

// 高斯滤波
U16* gauss_filter(U16* y, U16 height, U16 width, U8 r)
{
	float sigma = r * 0.8f;
	float* kernel = gen_gauss_kernel(r, sigma);
	U16* out = (U16*)malloc(height * width * sizeof(U16));
	memset(out, 0, height * width * sizeof(U16));
	U8 size = 2 * r + 1;

	for (U16 i = 0; i < height; i++) {
		for (U16 j = 0; j < width; j++) {
			float sum = 0.0f;
			float wsum = 0.0f;

			for (int dy = -r; dy <= r; dy++) {
				for (int dx = -r; dx <= r; dx++) {
					int y_idx = i + dy;
					int x_idx = j + dx;
					if (y_idx < 0 || y_idx >= height || x_idx < 0 || x_idx >= width)
						continue;

					float w = kernel[(dy + r) * size + (dx + r)];
					sum += w * y[y_idx * width + x_idx];
					wsum += w;
				}
			}

			out[i * width + j] = (U16)(sum / wsum + 0.5f);
		}
	}

	free(kernel);
	return out;
}



static int compare_u16(const void* a, const void* b) {
	U16 va = *(const U16*)a;
	U16 vb = *(const U16*)b;
	return (va > vb) - (va < vb);
}

U16* mid_filter(U16* y, U16 height, U16 width, U8 r) {
	if (!y || height == 0 || width == 0) return NULL;

	U16* output = (U16*)malloc(height * width * sizeof(U16));
	if (!output) return NULL;

	int window_size = (2 * r + 1) * (2 * r + 1);
	U16* window = (U16*)malloc(window_size * sizeof(U16));
	if (!window) {
		free(output);
		return NULL;
	}

	for (U16 row = 0; row < height; row++) {
		for (U16 col = 0; col < width; col++) {
			int count = 0;
			// 采集邻域像素
			for (int dy = -r; dy <= r; dy++) {
				int ny = row + dy;
				if (ny < 0 || ny >= height) continue;
				for (int dx = -r; dx <= r; dx++) {
					int nx = col + dx;
					if (nx < 0 || nx >= width) continue;
					window[count++] = y[ny * width + nx];
				}
			}
			// 排序取中值
			qsort(window, count, sizeof(U16), compare_u16);
			output[row * width + col] = window[count / 2];
		}
	}

	free(window);
	return output;
}