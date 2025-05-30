#pragma once

#include "raw_process.h"

U8 sharp_process(YUV* yuv, IMG_CONTEXT context, G_CONFIG cfg);

void edge_detect(U16* src, S32* dst, U16 height, U16 width, S8* kernel, U8 k_size);

