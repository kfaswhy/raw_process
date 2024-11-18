#pragma once

#define _CRT_SECURE_NO_WARNINGS

#define DEBUG_MODE 1//��Ҫ����ʱ��1��������0

#include <stdio.h>
#include <iostream>
#include <windows.h> 
#include <time.h>
#include <omp.h>
#include <stdint.h>

#include <opencv2/opencv.hpp>
#include <fstream>
#include <vector>
#include <string>

typedef unsigned long long U64;
typedef long long S64;
typedef unsigned int U32;
typedef int S32;
typedef unsigned short U16;
typedef unsigned char U8;

#define U16MAX (0xFFFF)
#define U8MAX (255)
#define U8MIN (0)

#define calc_min(a,b) ((a)>(b)?(b):(a))
#define calc_max(a,b) ((a)<(b)?(b):(a))
#define calc_abs(a) ((a)>0?(a):(-a))
#define clp_range(min,x,max) calc_min(calc_max((x), (min)), (max))

#define safe_sub(a,b) ((a)>(b)?(a-b):0)

#define OK 0
#define ERROR -1

#define LOG(...) {printf("%s [%d]: ", __FUNCTION__, __LINE__); printf(__VA_ARGS__); printf("\n"); }


typedef enum { RGGB, BGGR, GBRG, GRBG } BayerPattern;  // Bayer ��ʽö������
typedef enum { LITTLE_ENDIAN, BIG_ENDIAN } ByteOrder;  // �ֽ�˳��ö������

typedef struct _G_CONFIG
{
	U8 bit;
	U8 order;
	U8 pattern;

	U16 ob;
	U16 isp_gain;
}G_CONFIG;


typedef struct _RGB
{
	U8 b;
	U8 g;
	U8 r;
}RGB;

typedef struct _IMG_CONTEXT
{
	BITMAPFILEHEADER fileHeader;
	BITMAPINFOHEADER infoHeader;

	U16 height;
	U16 width;

	int PaddingSize;
	U8* pad;
}IMG_CONTEXT;


void load_cfg();

int main();

// ������������ȡ RAW ���ݵ�һά����
U16* readRawData(const char* filename, int width, int height, int bitDepth, ByteOrder byteOrder);

// ������������ Bayer ��ʽ������ת��Ϊ R��G��B ����ͨ����ͼ��
void debayer(const U16* rawData, int width, int height, BayerPattern pattern,
	U8* redChannel, U8* greenChannel, U8* blueChannel);

void safe_free(void* p);

void print_prog(U32 cur_pos, U32 tgt);

RGB* load_bmp(const char* filename, IMG_CONTEXT* context);

void save_bmp(const char* filename, RGB* img, IMG_CONTEXT* context);

