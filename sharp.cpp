#include "sharp.h"
#include "y2r.h"
U8 sharp_process(YUV* yuv, IMG_CONTEXT context, G_CONFIG cfg) {
    if (cfg.sharp_on == 0) {
        return OK;
    }

    for (int i = 0; i < context.height; ++i) {
        for (int j = 0; j < context.width; ++j) {
            YUV* pixel = &yuv[i * context.width + j];
            U8 y_val = pixel->y;

            // �߽紦��
            int row_prev = (i > 0) ? i - 1 : 0;
            int row_next = (i < context.height - 1) ? i + 1 : context.height - 1;
            int col_prev = (j > 0) ? j - 1 : 0;
            int col_next = (j < context.width - 1) ? j + 1 : context.width - 1;

            // ����Sobel�ݶ�
            int gx = (-1) * yuv[row_prev * context.width + col_prev].y
                + 1 * yuv[row_prev * context.width + col_next].y
                + (-2) * yuv[i * context.width + col_prev].y
                + 2 * yuv[i * context.width + col_next].y
                + (-1) * yuv[row_next * context.width + col_prev].y
                + 1 * yuv[row_next * context.width + col_next].y;

            int gy = (-1) * yuv[row_prev * context.width + col_prev].y
                - 2 * yuv[row_prev * context.width + j].y
                - 1 * yuv[row_prev * context.width + col_next].y
                + 1 * yuv[row_next * context.width + col_prev].y
                + 2 * yuv[row_next * context.width + j].y
                + 1 * yuv[row_next * context.width + col_next].y;

            float grad_mag = sqrtf(gx * gx + gy * gy);

            // �����ȵ���ǿ��
            float brightness_strength;
            if (y_val < cfg.brightness_low_thresh) {
                brightness_strength = cfg.brightness_low_strength;
            }
            else if (y_val < cfg.brightness_high_thresh) {
                brightness_strength = cfg.brightness_mid_strength;
            }
            else {
                brightness_strength = cfg.brightness_high_strength;
            }

            // ���������͵���
            float region_strength;
            if (grad_mag < cfg.grad_flat_th) {
                region_strength = cfg.flat_strength;
            }
            else if (grad_mag >= cfg.grad_edge_th) {
                region_strength = cfg.edge_strength;
            }
            else {
                region_strength = cfg.texture_strength;
            }

            // ���������
            float dir_strength = 1.0f;
            if (grad_mag >= cfg.grad_flat_th) { // ��ƽ̹���Ŵ�����
                float angle = atan2f(gy, gx) * 180.0f / M_PI;
                if (angle < 0) angle += 360.0f;

                if ((angle >= 22.5 && angle < 67.5) || (angle >= 202.5 && angle < 247.5)) {
                    dir_strength = cfg.dir_diag1_strength;
                }
                else if ((angle >= 67.5 && angle < 112.5) || (angle >= 247.5 && angle < 292.5)) {
                    dir_strength = cfg.dir_vertical_strength;
                }
                else if ((angle >= 112.5 && angle < 157.5) || (angle >= 292.5 && angle < 337.5)) {
                    dir_strength = cfg.dir_diag2_strength;
                }
                else {
                    dir_strength = cfg.dir_horizontal_strength;
                }
            }

            // �ۺ�ǿ�ȼ���
            float total_strength = cfg.global_strength * brightness_strength * region_strength * dir_strength;

            // ����������˹������ʹ��3x3������˹ģ�壩
            int lap = 8 * y_val
                - yuv[row_prev * context.width + col_prev].y
                - yuv[row_prev * context.width + j].y
                - yuv[row_prev * context.width + col_next].y
                - yuv[i * context.width + col_prev].y
                - yuv[i * context.width + col_next].y
                - yuv[row_next * context.width + col_prev].y
                - yuv[row_next * context.width + j].y
                - yuv[row_next * context.width + col_next].y;

            // ��һ����Ӧ����
            float sharpened_y = y_val + (lap / 8.0f) * total_strength;
            sharpened_y = sharpened_y < 0 ? 0 : (sharpened_y > 255 ? 255 : sharpened_y);
            pixel->y = (U8)sharpened_y;
        }
    }

#if DEBUG_MODE
    LOG("done.");
    RGB* rgb_data = y2r_process(yuv, context, cfg);
    save_img_with_timestamp(rgb_data, &context, "_sharp");
#endif
    return OK;
}