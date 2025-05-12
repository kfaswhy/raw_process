#include "yuv_txi.h"
#include "y2r.h"


#define yuv_txi_debug 1

// ���ɸ�˹��
static void compute_gaussian_kernel(float* kernel, int mask_size, float sigma) {
    int r = mask_size / 2;
    float sum = 0.0f;
    for (int y = -r; y <= r; y++) {
        for (int x = -r; x <= r; x++) {
            float v = expf(-(x * x + y * y) / (2.0f * sigma * sigma));
            kernel[(y + r) * mask_size + (x + r)] = v;
            sum += v;
        }
    }
    // ��һ��
    for (int i = 0; i < mask_size * mask_size; i++) {
        kernel[i] /= sum;
        LOG("%f. ",kernel[i]);
    }
}

// ˫�߽��Ƹ�˹�˲�����intͼ��Ϊ���룩
void bilateral_filter(const int* src, int* dst, int w, int h, int mask_size, int dn_thd)
{
    int r = mask_size / 2;
    float sigma_spatial = mask_size / 2.0f; // �ռ�Ȩ�ر�׼��
    float inv_sigma2_spatial = 1.0f / (2.0f * sigma_spatial * sigma_spatial);

    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {

            float sum_w = 0.0f;
            float sum_val = 0.0f;
            int center_val = src[y * w + x];

            for (int dy = -r; dy <= r; dy++) {
                int yy = y + dy;
                if (yy < 0 || yy >= h) continue;

                for (int dx = -r; dx <= r; dx++) {
                    int xx = x + dx;
                    if (xx < 0 || xx >= w) continue;

                    int neighbor_val = src[yy * w + xx];
                    int diff = abs(neighbor_val - center_val);
                    if (diff > dn_thd) continue; // ���ԻҶȲ�̫��ĵ�

                    // �ռ�Ȩ�أ�ֻ����λ�þ��룬�������ز��˹Ȩ��
                    float spatial_dist2 = dx * dx + dy * dy;
                    float w_spatial = expf(-spatial_dist2 * inv_sigma2_spatial);

                    sum_w += w_spatial;
                    sum_val += neighbor_val * w_spatial;
                }
            }

            if (sum_w > 0.0f)
                dst[y * w + x] = (int)(sum_val / sum_w + 0.5f);
            else
                dst[y * w + x] = center_val;
        }
    }
}

//��˹�˲�
static void gaussian_filter(const U16* src, U16* dst, int w, int h, int radius) {
    int ksize = 2 * radius + 1;
    float sigma = radius > 0 ? radius / 2.0f : 1.0f;
    // ����һά Gaussian ��
    float* kernel = (float*)malloc(sizeof(float) * ksize);
    float sum = 0.0f;
    for (int i = 0; i < ksize; i++) {
        int x = i - radius;
        kernel[i] = expf(-(x * x) / (2.0f * sigma * sigma));
        sum += kernel[i];
    }
    // ��һ��
    for (int i = 0; i < ksize; i++) {
        kernel[i] /= sum;
    }
    // ��ʱ����
    float* temp = (float*)malloc(sizeof(float) * w * h);
    // ˮƽ���
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            float acc = 0.0f;
            for (int k = -radius; k <= radius; k++) {
                int xx = x + k;
                if (xx < 0) xx = 0;
                if (xx >= w) xx = w - 1;
                acc += kernel[k + radius] * src[y * w + xx];
            }
            temp[y * w + x] = acc;
        }
    }
    // ��ֱ���
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            float acc = 0.0f;
            for (int k = -radius; k <= radius; k++) {
                int yy = y + k;
                if (yy < 0) yy = 0;
                if (yy >= h) yy = h - 1;
                acc += kernel[k + radius] * temp[yy * w + x];
            }
            // �ضϲ�д��
            dst[y * w + x] = (U16)clp_range(0, (int)(acc + 0.5f), (1 << 16) - 1);
        }
    }
    free(kernel);
    free(temp);
}

// ��ֵ�˲���mask_size ����
static void median_filter(const int* src, int* dst, int w, int h, int mask_size) {
    int r = mask_size / 2;
    int window = mask_size * mask_size;
    int* buf = (int*)malloc(window * sizeof(int));
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            int cnt = 0;
            for (int dy = -r; dy <= r; dy++) {
                int yy = y + dy;
                if (yy < 0 || yy >= h) continue;
                for (int dx = -r; dx <= r; dx++) {
                    int xx = x + dx;
                    if (xx < 0 || xx >= w) continue;
                    buf[cnt++] = src[yy * w + xx];
                }
            }
            std::nth_element(buf, buf + cnt / 2, buf + cnt);
            dst[y * w + x] = buf[cnt / 2];
        }
    }
    free(buf);
}


U8 yuv_txi_process(YUV* yuv, IMG_CONTEXT context, G_CONFIG cfg)
{
    if (cfg.yuv_txi_on == 0)
    {
        return OK;
    }

    U8  r_detail = 3;     // ƽ���˲��뾶
    float str = 7;        // ϸ����ǿ����
    U8  mask_size = 5;    // �˲����ڴ�С������Ϊ����
    int dn_thd = 50;

    int W = context.width;
    int H = context.height;
    U32 N = context.full_size;
    int maxY = (1 << cfg.yuv_bit) - 1;

    // ��̬���仺��
    U16* y_orig = (U16*)malloc(sizeof(U16) * N);
    U16* y_bg = (U16*)malloc(sizeof(U16) * N);
    int* y_detail = (int*)malloc(sizeof(int) * N);
    int* y_enh = (int*)malloc(sizeof(int) * N);
    

    // ����ԭʼ Y ͨ��
    for (U32 i = 0; i < N; i++) y_orig[i] = yuv[i].y;
#if yuv_txi_debug
    save_y("yuv_txi_1_ori.jpg", y_orig, &context, cfg, 100);
#endif 


    // ����ͼ
    gaussian_filter(y_orig, y_bg, W, H, r_detail);
#if yuv_txi_debug
    save_y("yuv_txi_2_bg.jpg", y_bg, &context, cfg, 100);
    U16* y_posi = (U16*)malloc(sizeof(int) * N);
    U16 posi_shift = pow(2, cfg.yuv_bit - 1);
#endif 


    // ϸ��ͼ
    for (U32 i = 0; i < N; i++)
    {
        y_detail[i] = (int)y_orig[i] - (int)y_bg[i];
#if yuv_txi_debug
        y_posi[i] = posi_shift + y_detail[i];
#endif 
    }
#if yuv_txi_debug
    save_y("yuv_txi_3_det.jpg", y_posi, &context, cfg, 100);
#endif 


    // ϸ����ǿ
    for (U32 i = 0; i < N; i++)
    {
        y_enh[i] = (int)(y_detail[i] * str);
#if yuv_txi_debug
        y_posi[i] = posi_shift + y_enh[i];
#endif 
    }
#if yuv_txi_debug
    save_y("yuv_txi_4_enh.jpg", y_posi, &context, cfg, 100);
#endif 


    // ϸ��ͼ�˲�ȥ��
   //  median_filter(y_enh, y_detail, W, H, mask_size);
    bilateral_filter(y_enh, y_detail, W, H, mask_size, dn_thd);
#if yuv_txi_debug
    for (U32 i = 0; i < N; i++)
    {
        y_posi[i] = posi_shift + y_detail[i];
    }
    save_y("yuv_txi_5_dn.jpg", y_posi, &context, cfg, 100);
#endif


#if yuv_txi_debug
    U16* y_new = (U16*)malloc(sizeof(int) * N);
#endif
    //�ں�Y
    for (U32 i = 0; i < N; i++) {
#if yuv_txi_debug
        y_new[i] = clp_range(0, y_bg[i] + y_detail[i], maxY);
#endif
        yuv[i].y = clp_range(0, y_bg[i] + y_detail[i], maxY);

    }

#if yuv_txi_debug
    save_y("yuv_txi_6.jpg", y_new, &context, cfg, 100);
#endif


#if DEBUG_MODE
    LOG("done.");
    RGB* rgb_data = y2r_process(yuv, context, cfg);
    save_img_with_timestamp(rgb_data, &context, "_yuv_txi");
#endif

    return OK;
}