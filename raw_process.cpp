#include "raw_process.h"

#include "ob.h"
#include "isp_gain.h"
#include "awb.h"
#include "demosaic.h"
#include "ccm.h"
#include "r2y.h"
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
	cfg.order = LITTLE_ENDIAN;
	cfg.pattern = BGGR;
	cfg.width = 1920;
	cfg.height = 1080;
	
	cfg.ob_on = 1;
	cfg.isp_gain_on = 1;
	cfg.awb_on = 1;
	cfg.ccm_on = 1;

	cfg.ob = 4096;
	cfg.isp_gain = 1024;

	cfg.r_gain = 1233;
	cfg.b_gain = 2010;


	float ccm_tmp[9] = {
		 1.0915,   0.0222, -0.0852,
		 -0.0336 ,  1.2101 ,-0.1708,
		 -0.0579, -0.2323 ,  1.2560

	};




	memcpy(cfg.ccm, ccm_tmp, 9 * sizeof(float));
	return;
}

int main() 
{
	load_cfg();
	IMG_CONTEXT context = { 0 };
	context.width = cfg.width;
	context.height = cfg.height;
	context.full_size = context.width * context.height;

    const char* filename = "raw.raw";
	U16* raw = NULL;
	RGB* rgb_data = NULL;
	YUV* yuv_data = NULL;

    // 读取 RAW 数据到一维数组
    raw = readraw(filename, context, 16, LITTLE_ENDIAN);
    if (!raw)
    {
        fprintf(stderr, "读取 RAW 图像数据失败\n");
		return ERROR;
    }

#if DEBUG_MODE
	rgb_data = demosaic_process(raw, context, cfg);
	save_rgb("0.jpg", rgb_data, context, cfg);
#endif

	//进入raw域
	ob_process(raw, context, cfg);
#if DEBUG_MODE
	rgb_data = demosaic_process(raw, context, cfg);
	save_rgb("1_ob.jpg", rgb_data, context, cfg);
#endif

	isp_gain_process(raw, context, cfg);
#if DEBUG_MODE
	rgb_data = demosaic_process(raw, context, cfg);
	save_rgb("2_isp_gain.jpg", rgb_data, context, cfg);
#endif

	awb_process(raw, context, cfg);
#if DEBUG_MODE
	rgb_data = demosaic_process(raw, context, cfg);
	save_rgb("3_awb.jpg", rgb_data, context, cfg);
#endif

	rgb_data = demosaic_process(raw, context, cfg);
	

	//进入RGB域

	ccm_process(rgb_data, context, cfg);
#if DEBUG_MODE
	save_rgb("10_ccm.jpg", rgb_data, context, cfg);
#endif


	yuv_data = r2y_process(rgb_data, context, cfg);
	

	//进入YUV域


	rgb_data = y2r_process(yuv_data, context, cfg);
	//save_rgb("99.jpg", rgb_data, context, cfg);

    // 释放内存
    free(raw); 
	free(rgb_data);
	free(yuv_data);

	time_print_prog_end = clock();
	LOG("time = %.2f s.", ((float)time_print_prog_end - time_print_prog_start) / 1000);


    return 0;
}





U16* readraw(const char* filename, IMG_CONTEXT context, int bitDepth, ByteOrder byteOrder) {
    int bytesPerPixel = (bitDepth + 7) / 8;  // 计算每像素的字节数
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
        if (bitDepth == 16) {
            raw[i] = byteOrder == LITTLE_ENDIAN
                ? buffer[i * 2] | (buffer[i * 2 + 1] << 8)
                : (buffer[i * 2] << 8) | buffer[i * 2 + 1];
        }
        else {
            raw[i] = buffer[i];
        }
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