#pragma once

#include "raw_process.h"

U8 defog_process(RGB* rgb, IMG_CONTEXT context, G_CONFIG cfg);

void y_sampling(U16* img, U16* img_smp, int w1, int h1, int w2, int h2, bool method);

void float_sampling(float* src, float* dst, int w1, int h1, int w2, int h2, bool method);

void rgb_sampling(RGB* img, RGB* img_smp, int w1, int h1, int w2, int h2, bool method);

void calc_dark_chanel(RGB* img, U16* dark, U16 w, U16 h);

RGB calc_atmos_light(RGB* img, U16* img_dark, U16 w, U16 h, float light_ratio);

void calc_trans(RGB* img, float* trans, U16* img_dark, RGB light, float str, U16 w, U16 h);

void recover_img(RGB* img, RGB* img_rec, float* trans, RGB light, U16 w, U16 h);


