#include "raw_process.h"

#include "ob.h"
#include "isp_gain.h"

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
	cfg.pattern = RGGB;

	cfg.ob = 4096;
	cfg.isp_gain = 2048;
}

int main() 
{
	load_cfg();
    const int width = 1920;
    const int height = 1080;
    const char* filename = "1.raw";

    // 读取 RAW 数据到一维数组
    U16* rawData = readRawData(filename, width, height, 16, LITTLE_ENDIAN);
    if (!rawData)
    {
        fprintf(stderr, "读取 RAW 图像数据失败\n");
        return -1;
    }

	ob_process(rawData, width, height, cfg);
	isp_gain_process(rawData, width, height, cfg);


    // 为 RGB 通道分配内存
	U8* redChannel = (U8*)calloc(width * height, sizeof(U8));
	U8* greenChannel = (U8*)calloc(width * height, sizeof(U8));
	U8* blueChannel = (U8*)calloc(width * height, sizeof(U8));


    // 使用 debayer 将 RAW 数据转换为 RGB 通道
    debayer(rawData, width, height, BGGR, redChannel, greenChannel, blueChannel);

    // 使用 cv::Mat 创建一个 RGB 图像
    Mat rgbImage(height, width, CV_8UC3);  // 8-bit 3-channel image
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int index = y * width + x;
            rgbImage.at<Vec3b>(y, x) = Vec3b(blueChannel[index], greenChannel[index], redChannel[index]);
        }
    }



    // 保存图像为 JPEG 格式
    if (!imwrite("1.jpg", rgbImage)) {
        fprintf(stderr, "保存 RGB 图像失败\n");
        return -1;
    }

    //printf("图像已成功转换并保存为 1.jpg\n");

    // 释放内存
    free(rawData);
    free(redChannel);
    free(greenChannel);
    free(blueChannel);

    return 0;
}





U16* readRawData(const char* filename, int width, int height, int bitDepth, ByteOrder byteOrder) {
    int bytesPerPixel = (bitDepth + 7) / 8;  // 计算每像素的字节数
    int dataSize = width * height * bytesPerPixel;  // 数据总大小
    U16* rawData = (U16*)malloc(width * height * sizeof(U16));

    FILE* rawFile = fopen(filename, "rb");
    if (!rawFile) {
        fprintf(stderr, "无法打开文件: %s\n", filename);
        return NULL;
    }

    U8* buffer = (U8*)malloc(dataSize);
    fread(buffer, 1, dataSize, rawFile);
    fclose(rawFile);

    for (int i = 0; i < width * height; i++) {
        if (bitDepth == 16) {
            rawData[i] = byteOrder == LITTLE_ENDIAN
                ? buffer[i * 2] | (buffer[i * 2 + 1] << 8)
                : (buffer[i * 2] << 8) | buffer[i * 2 + 1];
        }
        else {
            rawData[i] = buffer[i];
        }
    }

    free(buffer);
    return rawData;
}

void debayer(const U16* rawData, int width, int height, BayerPattern pattern,
    U8* redChannel, U8* greenChannel, U8* blueChannel) {
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int index = y * width + x;
            U16 pixelValue = rawData[index];

            if ((y % 2 == 0 && x % 2 == 0 && pattern == RGGB) || (y % 2 == 1 && x % 2 == 1 && pattern == BGGR)) {
                redChannel[index] = (U8)(pixelValue >> 8);
            }
            else if ((y % 2 == 1 && x % 2 == 1 && pattern == RGGB) || (y % 2 == 0 && x % 2 == 0 && pattern == BGGR)) {
                blueChannel[index] = (U8)(pixelValue >> 8);
            }
            else {
                greenChannel[index] = (U8)(pixelValue >> 8);
            }
        }
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