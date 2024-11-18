#pragma once

#include "raw_process.h"

U8 yuv_txi_process(YUV* yuv, IMG_CONTEXT context, G_CONFIG cfg);

S32* get_detail(U16* y, U16* y_bg, U32 full_size, U16 y_max);

U8 detail_enhance(S32* y, U16 height, U16 width, U16 y_max, float str);

U8 merge(U16* y, U16* bg, S32* detail, U32 full_size, U16 y_max);

