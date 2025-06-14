#pragma once

#include "raw_process.h"

U8 lsc_process(U16* raw, IMG_CONTEXT context, G_CONFIG cfg);

void write_csv(const char* filename, U16* chn1, U16* chn2, U16* chn3, U16* chn4, int wblock, int hblock);

U8 lsc_process2(U16* raw, IMG_CONTEXT context, G_CONFIG cfg);

U16 calc_min4(U16 a, U16 b, U16 c, U16 d);

U32 bilinear_interp(U16 left_top, U16 left_bottom, U16 right_top, U16 right_bottom, double x_weight, double y_weight);

