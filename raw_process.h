#pragma once

#define _CRT_SECURE_NO_WARNINGS

#define DEBUG_MODE 1//需要调试时置1，否则置0

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
typedef short S16;
typedef unsigned char U8;

#define U16MAX (0xFFFF)
#define U8MAX (255)
#define U8MIN (0)
#define M_PI 3.14159265358979323846

#define calc_min(a,b) ((a)>(b)?(b):(a))
#define calc_max(a,b) ((a)<(b)?(b):(a))
#define calc_abs(a) ((a)>0?(a):(-a))
#define clp_range(min,x,max) calc_min(calc_max((x), (min)), (max))

#define safe_sub(a,b) ((a)>(b)?(a-b):0)

#define OK 0
#define ERROR -1

#define LOG(...) {printf("%s [%d]: ", __FUNCTION__, __LINE__); printf(__VA_ARGS__); printf("\n"); }


typedef enum { RGGB = 0, GRBG = 1, GBRG = 2, BGGR = 3 } BayerPattern;  // Bayer 格式枚举类型
typedef enum { LITTLE_ENDIAN, BIG_ENDIAN } ByteOrder;  // 字节顺序枚举类型

typedef struct _G_CONFIG
{
	U8 bit;
	U8 used_bit;
	U8 order;
	U8 pattern;
	U16 width;
	U16 height;


	U8 ob_on;
	U8 isp_gain_on;
	U8 awb_on;
	U8 ltm_on;
	U8 ccm_on;
	U8 rgbgamma_on;
	U8 ygamma_on;
	//U8 sharp_on;

	U16 ob;
	U16 isp_gain;
	U16 r_gain;
	U16 g_gain;
	U16 b_gain;
	
	float ltm_strength; // 整体强度 (0~1, 1 表示完全应用 LTM)
	U8 ltm_vblk;    //纵向分块数
	U8 ltm_hblk;    //纵向分块数
	float ltm_cst_thdr;

	float ccm[9];

	U32 gamma_x[49];
	U32 gamma_y[49];

	int sharp_on;             // 锐化开关
	// 全局控制
	float global_strength = 1.0;  // 整体强度适中

	// 区域分类
	float flat_strength = 0.2;     // 平坦区弱锐化
	float texture_strength = 0.7;  // 草地维持原强度
	float edge_strength = 1.8;     // 树干边缘强化
	float grad_flat_th = 3.0;      // 降低平坦区阈值
	float grad_edge_th = 20.0;     // 提高边缘区阈值

	// 方向增益
	float dir_horizontal_strength = 0.9;  // 水平抑制
	float dir_vertical_strength = 1.5;   // 垂直增强（树干）
	float dir_diag1_strength = 0.9;      // 对角线1
	float dir_diag2_strength = 0.9;       // 对角线2

	// 颜色保护
	float Rgain = 1.3;   // 红色区域增强（树皮）
	float Ggain = 1.0;   // 绿色保持（草地）
	float Bgain = 1.3;   // 蓝色增强（阴影树干）

	// 亮度分段
	U8 brightness_low_thresh = 30;
	U8 brightness_high_thresh = 200;
	float brightness_low_strength = 0.3;  // 暗部降噪
	float brightness_mid_strength = 1.0;  // 中亮度正常
	float brightness_high_strength = 1.0;  // 高亮度正常
}G_CONFIG;


typedef struct _RGB
{
	U8 b;
	U8 g;
	U8 r;
}RGB;

typedef struct _YUV
{
	U8 y;
	U8 u;
	U8 v;
} YUV;

typedef struct _IMG_CONTEXT
{
	BITMAPFILEHEADER fileHeader;
	BITMAPINFOHEADER infoHeader;

	U16 height;
	U16 width;
	U32 full_size;

	int PaddingSize;
	U8* pad;
}IMG_CONTEXT;


void load_cfg();

int main();

RGB* raw2rgb(U16* raw, IMG_CONTEXT context, G_CONFIG cfg);

// 函数声明：读取 RAW 数据到一维数组

U16* readraw(const char* filename, IMG_CONTEXT context, G_CONFIG cfg);

RGB* yyy2rgb_process(YUV* yuv, IMG_CONTEXT context, G_CONFIG cfg);

U8 save_rgb(const char* filename, RGB* rgb, IMG_CONTEXT context, G_CONFIG cfg);

void safe_free(void* p);

void print_prog(U32 cur_pos, U32 tgt);

RGB* load_bmp(const char* filename, IMG_CONTEXT* context);

void save_bmp(const char* filename, RGB* img, IMG_CONTEXT* context);

U32 calc_inter(U32 x0, U32* x, U32* y, U32 len);

