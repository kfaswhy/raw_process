#include "yuv_txi.h"
#include "y2r.h"

using namespace std;
#define yuv_txi_debug 0


U8 yuv_txi_process(YUV* yuv, IMG_CONTEXT context, G_CONFIG cfg)
{
    if (cfg.yuv_txi_on == 0)
    {

        return OK;
    }

    U8 r_detail = 1;
    U8 r_bifilter = 1;
    float y_thd_bifilter = 16 / 255;
    float str_enh = 7;


    U16 y_max = (1 << cfg.yuv_bit) - 1;
    U16* y = (U16*)malloc(context.full_size * sizeof(U16));
    if (!y)
    {
        LOG("Memory allocation failed.");
        return ERROR;
    }
    memcpy(y, yuv->y, context.full_size * sizeof(U16));

    U16* y_tmp = (U16*)malloc(context.full_size * sizeof(U16));
    U16 tmp_shift = 1 << (cfg.yuv_bit - 1);

#if yuv_txi_debug
    
    save_y("txi_0_y.bmp", y, &context, cfg, 100);
#endif

    //获得背景图
    U16* y_bg = gauss_filter(y, context.height, context.width, r_detail);
#if yuv_txi_debug
    save_y("txi_1_bg.bmp", y_bg, &context, cfg,100);
#endif

    //提取细节图
    S32* y_detail = get_detail(y, y_bg, context.full_size, y_max);
#if yuv_txi_debug
    for (U32 i = 0; i < context.full_size; i++)
    {
        y_tmp[i] = y_detail[i] + tmp_shift;
    }
    save_y("txi_2_det.bmp", y_tmp, &context, cfg, 100);
#endif

    //细节图去噪

    for (U32 i = 0; i < context.full_size; i++)
    {
        y_tmp[i] = y_detail[i] + tmp_shift;
    }
    y_tmp = mid_filter(y_tmp, context.height, context.width, r_bifilter);
    for (U32 i = 0; i < context.full_size; i++)
    {
        y_detail[i] = y_tmp[i] - tmp_shift;
    }
#if yuv_txi_debug
    for (U32 i = 0; i < context.full_size; i++)
    {
        y_tmp[i] = y_detail[i] + tmp_shift;
    }
    save_y("txi_3_dn.bmp", y_tmp, &context, cfg, 100);
#endif
    //细节增强
    detail_enhance(y_detail, context.height, context.width, y_max, str_enh);
#if yuv_txi_debug
    for (U32 i = 0; i < context.full_size; i++)
    {
        y_tmp[i] = y_detail[i] + tmp_shift;
    }
    save_y("txi_4_enh.bmp", y_tmp, &context, cfg, 100);
#endif
    //图像融合
    merge(y, y_bg, y_detail, context.full_size, y_max);

    //恢复到yuv
    memcpy(yuv->y, y, context.full_size * sizeof(U16));
    //memset(yuv->y, y, context.full_size * sizeof(U16));

#if DEBUG_MODE
    LOG("done.");
    RGB* rgb_data = y2r_process(yuv, context, cfg);
    save_img_with_timestamp(rgb_data, &context, "_yuv_txi");
#endif

    return OK;
}


static float* gen_gauss_kernel(U8 r, float sigma)
{
    U8 size = 2 * r + 1;
    float* kernel = (float*)malloc(size * size * sizeof(float));
    float sum = 0.0f;
    float s2 = 2 * sigma * sigma;
    int index = 0;

    for (int y = -r; y <= r; y++) {
        for (int x = -r; x <= r; x++) {
            float value = expf(-(x * x + y * y) / s2);
            kernel[index++] = value;
            sum += value;
        }
    }

    for (int i = 0; i < size * size; i++) {
        kernel[i] /= sum;
    }

    return kernel;
}

// 高斯滤波
U16* gauss_filter(U16* y, U16 height, U16 width, U8 r)
{
    float sigma = r * 0.8f;
    float* kernel = gen_gauss_kernel(r, sigma);
    U16* out = (U16*)malloc(height * width * sizeof(U16));
    memset(out, 0, height * width * sizeof(U16));
    U8 size = 2 * r + 1;

    for (U16 i = 0; i < height; i++) {
        for (U16 j = 0; j < width; j++) {
            float sum = 0.0f;
            float wsum = 0.0f;

            for (int dy = -r; dy <= r; dy++) {
                for (int dx = -r; dx <= r; dx++) {
                    int y_idx = i + dy;
                    int x_idx = j + dx;
                    if (y_idx < 0 || y_idx >= height || x_idx < 0 || x_idx >= width)
                        continue;

                    float w = kernel[(dy + r) * size + (dx + r)];
                    sum += w * y[y_idx * width + x_idx];
                    wsum += w;
                }
            }

            out[i * width + j] = (U16)(sum / wsum + 0.5f);
        }
    }

    free(kernel);
    return out;
}

static int compare_u16(const void* a, const void* b) {
    U16 va = *(const U16*)a;
    U16 vb = *(const U16*)b;
    return (va > vb) - (va < vb);
}

U16* mid_filter(U16* y, U16 height, U16 width, U8 r) {
    if (!y || height == 0 || width == 0) return NULL;

    U16* output = (U16*)malloc(height * width * sizeof(U16));
    if (!output) return NULL;

    int window_size = (2 * r + 1) * (2 * r + 1);
    U16* window = (U16*)malloc(window_size * sizeof(U16));
    if (!window) {
        free(output);
        return NULL;
    }

    for (U16 row = 0; row < height; row++) {
        for (U16 col = 0; col < width; col++) {
            int count = 0;
            // 采集邻域像素
            for (int dy = -r; dy <= r; dy++) {
                int ny = row + dy;
                if (ny < 0 || ny >= height) continue;
                for (int dx = -r; dx <= r; dx++) {
                    int nx = col + dx;
                    if (nx < 0 || nx >= width) continue;
                    window[count++] = y[ny * width + nx];
                }
            }
            // 排序取中值
            qsort(window, count, sizeof(U16), compare_u16);
            output[row * width + col] = window[count / 2];
        }
    }

    free(window);
    return output;
}

S32* get_detail(U16* y, U16* y_bg, U32 full_size, U16 y_max)
{
    //提取细节图
    S32* y_detail = (S32*)malloc(full_size * sizeof(S32));
    for (U32 i = 0; i < full_size; i++)
    {
        y_detail[i] = y[i] - y_bg[i];
    }
    return y_detail;

}

U8 detail_enhance(S32* y, U16 height, U16 width, U16 y_max, float str)
{
    //细节增强
    for (U32 i = 0; i < height * width; i++)
    {
        y[i] = clp_range(-y_max, y[i] * str, y_max);
    }
    return OK;
}

U8 merge(U16* y, U16* bg, S32* detail, U32 full_size, U16 y_max)
{
    //融合Y
    for (U32 i = 0; i < full_size; i++) {
        y[i] = clp_range(0, bg[i] + detail[i], y_max);
    }
    return OK;
}
