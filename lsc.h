#pragma once

#include "raw_process.h"

U8 lsc_process(U16* raw, IMG_CONTEXT context, G_CONFIG cfg);

U32 bilinear_interp(U16 left_top, U16 left_bottom, U16 right_top, U16 right_bottom, double x_weight, double y_weight);

