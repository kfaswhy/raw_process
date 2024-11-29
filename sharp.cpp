#include "sharp.h"
// Simple sharpen kernel (example: a 3x3 filter)
int sharp_kernel[3][3] = {
    { 0, -1,  0 },
    {-1,  4, -1 },
    { 0, -1,  0 }
};

U8 sharp_process(YUV* yuv, IMG_CONTEXT context, G_CONFIG cfg)
{
    if (cfg.sharp_on == 0)
    {
        return OK;
    }

    // Apply sharpen filter
    for (U16 y = 0; y < context.height; ++y) {
        for (U16 x = 0; x < context.width; ++x) {
            int new_y = 0, new_u = 0, new_v = 0;

            // Apply sharpening filter to Y, U, V channels separately
            for (int ky = -1; ky <= 1; ++ky) {
                for (int kx = -1; kx <= 1; ++kx) {
                    // Calculate the pixel index considering boundary conditions
                    int ny = y + ky;
                    int nx = x + kx;

                    // Boundary
                    // check: if out of bounds, use the edge pixel
                    if (ny < 0) ny = 0;
                    if (ny >= context.height) ny = context.height - 1;
                    if (nx < 0) nx = 0;
                    if (nx >= context.width) nx = context.width - 1;


                    int pixel_idx = ny * context.width + nx;

                    new_y += sharp_kernel[ky + 1][kx + 1] * yuv[pixel_idx].y;
                    //new_u += sharp_kernel[ky + 1][kx + 1] * yuv[pixel_idx].u;
                    //new_v += sharp_kernel[ky + 1][kx + 1] * yuv[pixel_idx].v;
                }
            }

            // Ensure the values are within the 8-bit range [0, 255]
            yuv[y * context.width + x].y = (U8)(new_y > 255 ? 255 : (new_y < 0 ? 0 : new_y));
            //yuv[y * context.width + x].u = (U8)(new_u > 255 ? 255 : (new_u < 0 ? 0 : new_u));
            //yuv[y * context.width + x].v = (U8)(new_v > 255 ? 255 : (new_v < 0 ? 0 : new_v));
        }
    }

    LOG("done.");
    return OK;
}