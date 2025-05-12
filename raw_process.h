#pragma once

#define _CRT_SECURE_NO_WARNINGS

//��Ҫ����ʱ��1��������0
#define DEBUG_MODE 1

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

#include <filesystem>
//#include <experimental/filesystem>


#define _SILENCE_EXPERIMENTAL_FILESYSTEM_DEPRECATION_WARNING

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


typedef enum { RGGB = 0, GRBG = 1, GBRG = 2, BGGR = 3 } BayerPattern;  // Bayer ��ʽö������
typedef enum { LITTLE_ENDIAN, BIG_ENDIAN } ByteOrder;  // �ֽ�˳��ö������

#define GAMMA_LENGTH 49


typedef struct _G_CONFIG
{
	U8 bit;
	U8 used_bit;
	U8 order;
	U8 pattern;
	U16 width;
	U16 height;

	U8 rgb_bit;
	U8 yuv_bit;

	U8 ob_on;
	U8 isp_gain_on;
	U8 awb_on;
	U8 ltm_on;
	U8 ccm_on;
	U8 rgbgamma_on;
	U8 ygamma_on;
	U8 yuv_txi_on;
	//U8 sharp_on;

	U16 ob;
	U16 isp_gain;
	U16 r_gain;
	U16 g_gain;
	U16 b_gain;
	
	float ltm_strength; // ����ǿ�� (0~1, 1 ��ʾ��ȫӦ�� LTM)
	U8 ltm_vblk;    //����ֿ���
	U8 ltm_hblk;    //����ֿ���
	float ltm_cst_thdr;

	float ccm[9];

	U32 gamma_x[GAMMA_LENGTH];
	U32 gamma_y[GAMMA_LENGTH];

	int sharp_on;             // �񻯿���
	// ȫ�ֿ���
	float global_strength = 1.0;  // ����ǿ������

	// �������
	float flat_strength = 0.2;     // ƽ̹������
	float texture_strength = 0.7;  // �ݵ�ά��ԭǿ��
	float edge_strength = 1.8;     // ���ɱ�Եǿ��
	float grad_flat_th = 3.0;      // ����ƽ̹����ֵ
	float grad_edge_th = 20.0;     // ��߱�Ե����ֵ

	// ��������
	float dir_horizontal_strength = 0.9;  // ˮƽ����
	float dir_vertical_strength = 1.5;   // ��ֱ��ǿ�����ɣ�
	float dir_diag1_strength = 0.9;      // �Խ���1
	float dir_diag2_strength = 0.9;       // �Խ���2

	// ��ɫ����
	float Rgain = 1.3;   
	float Ggain = 1.0;   
	float Bgain = 1.3;   

	// ���ȷֶ�
	U8 brightness_low_thresh = 30;
	U8 brightness_high_thresh = 200;
	float brightness_low_strength = 0.3;  // ��������
	float brightness_mid_strength = 1.0;  // ����������
	float brightness_high_strength = 1.0;  // ����������
}G_CONFIG;


typedef struct _RGB
{
	U16 b;
	U16 g;
	U16 r;
}RGB;

typedef struct _YUV
{
	U16 y;
	U16 u;
	U16 v;
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

void clear_tmp();

RGB* raw2rgb(U16* raw, IMG_CONTEXT context, G_CONFIG cfg);

// ������������ȡ RAW ���ݵ�һά����

U16* readraw(const char* filename, IMG_CONTEXT context, G_CONFIG cfg);

RGB* yyy2rgb_process(YUV* yuv, IMG_CONTEXT context, G_CONFIG cfg);

void safe_free(void* p);

void print_prog(U32 cur_pos, U32 tgt);

RGB* load_bmp(const char* filename, IMG_CONTEXT* context);

void save_bmp(const char* filename, RGB* img, IMG_CONTEXT* context);

U32 calc_inter(U32 x0, U32* x, U32* y, U32 len);

void save_y(const char* filename, U16* y, IMG_CONTEXT* context, G_CONFIG cfg, int compression_quality);

void save_img(const char* filename, RGB* img, IMG_CONTEXT* context, G_CONFIG cfg, int compression_quality);


void save_img_with_timestamp(RGB* rgb_data, IMG_CONTEXT* context, const char* suffix);

