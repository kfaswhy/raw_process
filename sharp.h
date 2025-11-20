#pragma once

#include "raw_process.h"

U8 sharp_process(YUV* yuv, IMG_CONTEXT context, G_CONFIG cfg);

S32* calc_edge_em_3x5(U16* y, int width, int height);

void edge_detect(U16* src, S32* dst, U16 height, U16 width, S8* kernel, U8 k_size);

