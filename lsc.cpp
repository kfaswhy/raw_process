#include "lsc.h"

U8 lsc_process(U16* raw, IMG_CONTEXT context, G_CONFIG cfg)
{
    if (cfg.lsc_on == 0)
    {
        return OK;
    }


    //参数区
    U8 lsc_type = cfg.lsc_type; 	/* 使用flat shading还是interpolation shading，1为flat，0为插值 */
    U8 wblock = cfg.lsc_wblock;
    U8 hblock = cfg.lsc_hblock;

    U16* rgain = cfg.lsc_rgain;
    U16* grgain = cfg.lsc_grgain;
    U16* gbgain = cfg.lsc_gbgain;
    U16* bgain = cfg.lsc_bgain;


    S32 i = 0;
    S32 j = 0;
    U16 w_pos = 0;		/* 某坐标位于x方向的哪一块 */
    U16 h_pos = 0;		/* 某坐标位于y方向的哪一块 */
    U16 width = context.width;
    U16 height = context.height;

    float tmp = 0;
    double w_cell = 0;		/* 分块的宽度，若使用int有可能导致越界 */
    double h_cell = 0;		/* 分块的长度，若使用int有可能导致越界 */
    double x_weight = 0;	/* 线性插值中，x方向的权重 */
    double y_weight = 0;	/* 线性插值中，y方向的权重 */
    double gain = 0;		/* gain值 */


    if (cfg.pattern == BGGR)
    {
        if (lsc_type == 1)/* flat无插值 */
        {
            /* 此处使用double型，下方h_pos = i * H_CELL_NUM / height */
            /* 由于i的范围为[0, height - 1]，所以 i / height < 1，所以h_pos < H_CELL_NUM，不会溢出 */
            /* 若w_cell、h_cell采用int型，由于C语言会直接去掉小数点（而不是四舍五入） */
            /* 会导致得到的数(int)(w_cell) <= (double)(w_cell)，所以 j / (int)(w_cell) >= j / (double)(w_cell) */
            /* 有可能导致溢出，故保留采用double，下同 */

            w_cell = (double)(width) / (double)(wblock);
            h_cell = (double)(height) / (double)(hblock);

            for (i = 0; i < height; ++i)
            {
                h_pos = i / h_cell; /* 所属的y网格 */
                for (j = 0; j < width; ++j)
                {
                    w_pos = j / w_cell; /* 所属的x网格 */
                    /* 利用与操作代替取余，提高运行速度，此处注释与下方interp的相同*/
                    if ((i & 1) && (j & 1)) /* i、j均为奇数 */
                    {
                        gain = rgain[h_pos * wblock + w_pos] / (double)(GAIN_FACTOR);
                    }
                    else if (!(i & 1) && !(j & 1)) /* i、j均为偶数 */
                    {
                        gain = bgain[h_pos * wblock + w_pos] / (double)(GAIN_FACTOR);
                    }
                    else if (!(i & 1) && (j & 1))
                    {
                        gain = gbgain[h_pos * wblock + w_pos] / (double)(GAIN_FACTOR);
                    }
                    else if ((i & 1) && !(j & 1))
                    {
                        gain = grgain[h_pos * wblock + w_pos] / (double)(GAIN_FACTOR);
                    }
                    raw[width * i + j] = clp_range(0, raw[width * i + j] * gain, U16MAX);
                }
            }
        }
        else if (lsc_type == 0)/* 插值 */
        {
            w_cell = (double)(width) / (double)(wblock - 1);
            h_cell = (double)(height) / (double)(hblock - 1);

            for (i = 0; i < height; ++i)
            {
                h_pos = i / h_cell;
                y_weight = (i - h_pos * h_cell) / (double)(h_cell); /* 双线性插值时的权值 */

                for (j = 0; j < width; ++j)
                {
                    w_pos = j / w_cell;
                    x_weight = (j - w_pos * w_cell) / (double)(w_cell); /* 双线性插值时的权值 */

                    if ((i & 1) && (j & 1)) /* i、j均为奇数 */
                    {
                        gain = bilinear_interp(rgain[h_pos * wblock + w_pos],
                            rgain[(h_pos + 1) * wblock + w_pos],
                            rgain[h_pos * wblock + w_pos + 1],
                            rgain[(h_pos + 1) * wblock + w_pos + 1], x_weight, y_weight) / (double)(GAIN_FACTOR);

                    }
                    else if (!(i & 1) && !(j & 1)) /* i、j均为偶数 */
                    {
                        gain = bilinear_interp(bgain[h_pos * wblock + w_pos],
                            bgain[(h_pos + 1) * wblock + w_pos],
                            bgain[h_pos * wblock + w_pos + 1],
                            bgain[(h_pos + 1) * wblock + w_pos + 1], x_weight, y_weight) / (double)(GAIN_FACTOR);
                    }
                    else if (!(i & 1) && (j & 1))
                    {
                        gain = bilinear_interp(gbgain[h_pos * wblock + w_pos],
                            gbgain[(h_pos + 1) * wblock + w_pos],
                            gbgain[h_pos * wblock + w_pos + 1],
                            gbgain[(h_pos + 1) * wblock + w_pos + 1], x_weight, y_weight) / (double)(GAIN_FACTOR);
                    }
                    else if ((i & 1) && !(j & 1))
                    {
                        gain = bilinear_interp(grgain[h_pos * wblock + w_pos],
                            grgain[(h_pos + 1) * wblock + w_pos],
                            grgain[h_pos * wblock + w_pos + 1],
                            grgain[(h_pos + 1) * wblock + w_pos + 1], x_weight, y_weight) / (double)(GAIN_FACTOR);
                    }

                    raw[width * i + j] = clp_range(0, raw[width * i + j] * gain, U16MAX);
                    /* 查看紫色区域 */
                    /* if ((i > 1420) && (i < 1435) && (j > 1820) && (j < 1840))
                    {
                    printf("[shading]i = %d, j = %d, ori_data = %d, gain = %lf, result = %f, final = %lf\n",
                    i, j, data[i * width + j], gain, tmp, SUPPRESS(tmp));
                    } */
                }
            }
        }
    }

    if (cfg.pattern == RGGB)
    {
        if (lsc_type == 1)/* flat无插值 */
        {
            /* 此处使用double型，下方h_pos = i * H_CELL_NUM / height */
            /* 由于i的范围为[0, height - 1]，所以 i / height < 1，所以h_pos < H_CELL_NUM，不会溢出 */
            /* 若w_cell、h_cell采用int型，由于C语言会直接去掉小数点（而不是四舍五入） */
            /* 会导致得到的数(int)(w_cell) <= (double)(w_cell)，所以 j / (int)(w_cell) >= j / (double)(w_cell) */
            /* 有可能导致溢出，故保留采用double，下同 */

            w_cell = (double)(width) / (double)(wblock);
            h_cell = (double)(height) / (double)(hblock);

            for (i = 0; i < height; ++i)
            {
                h_pos = i / h_cell; /* 所属的y网格 */
                for (j = 0; j < width; ++j)
                {
                    w_pos = j / w_cell; /* 所属的x网格 */
                    /* 利用与操作代替取余，提高运行速度，此处注释与下方interp的相同*/
                    if ((i & 1) && (j & 1)) /* i、j均为奇数 */
                    {
                        gain = bgain[h_pos * wblock + w_pos] / (double)(GAIN_FACTOR);
                    }
                    else if (!(i & 1) && !(j & 1)) /* i、j均为偶数 */
                    {
                        gain = rgain[h_pos * wblock + w_pos] / (double)(GAIN_FACTOR);
                    }
                    else if (!(i & 1) && (j & 1))
                    {
                        gain = grgain[h_pos * wblock + w_pos] / (double)(GAIN_FACTOR);
                    }
                    else if ((i & 1) && !(j & 1))
                    {
                        gain = gbgain[h_pos * wblock + w_pos] / (double)(GAIN_FACTOR);
                    }
                    raw[width * i + j] = clp_range(0, raw[width * i + j] * gain, U16MAX);
                }
            }
        }
        else if (lsc_type == 0)/* 插值 */
        {
            w_cell = (double)(width) / (double)(wblock - 1);
            h_cell = (double)(height) / (double)(hblock - 1);

            for (i = 0; i < height; ++i)
            {
                h_pos = i / h_cell;
                y_weight = (i - h_pos * h_cell) / (double)(h_cell); /* 双线性插值时的权值 */

                for (j = 0; j < width; ++j)
                {
                    w_pos = j / w_cell;
                    x_weight = (j - w_pos * w_cell) / (double)(w_cell); /* 双线性插值时的权值 */

                    if ((i & 1) && (j & 1)) /* i、j均为奇数 */
                    {
                        gain = bilinear_interp(bgain[h_pos * wblock + w_pos],
                            bgain[(h_pos + 1) * wblock + w_pos],
                            bgain[h_pos * wblock + w_pos + 1],
                            bgain[(h_pos + 1) * wblock + w_pos + 1], x_weight, y_weight) / (double)(GAIN_FACTOR);

                    }
                    else if (!(i & 1) && !(j & 1)) /* i、j均为偶数 */
                    {
                        gain = bilinear_interp(rgain[h_pos * wblock + w_pos],
                            rgain[(h_pos + 1) * wblock + w_pos],
                            rgain[h_pos * wblock + w_pos + 1],
                            rgain[(h_pos + 1) * wblock + w_pos + 1], x_weight, y_weight) / (double)(GAIN_FACTOR);
                    }
                    else if (!(i & 1) && (j & 1))
                    {
                        gain = bilinear_interp(grgain[h_pos * wblock + w_pos],
                            grgain[(h_pos + 1) * wblock + w_pos],
                            grgain[h_pos * wblock + w_pos + 1],
                            grgain[(h_pos + 1) * wblock + w_pos + 1], x_weight, y_weight) / (double)(GAIN_FACTOR);
                    }
                    else if ((i & 1) && !(j & 1))
                    {
                        gain = bilinear_interp(gbgain[h_pos * wblock + w_pos],
                            gbgain[(h_pos + 1) * wblock + w_pos],
                            gbgain[h_pos * wblock + w_pos + 1],
                            gbgain[(h_pos + 1) * wblock + w_pos + 1], x_weight, y_weight) / (double)(GAIN_FACTOR);
                    }

                    raw[width * i + j] = clp_range(0, raw[width * i + j] * gain, U16MAX);
                    /* 查看紫色区域 */
                    /* if ((i > 1420) && (i < 1435) && (j > 1820) && (j < 1840))
                    {
                    printf("[shading]i = %d, j = %d, ori_data = %d, gain = %lf, result = %f, final = %lf\n",
                    i, j, data[i * width + j], gain, tmp, SUPPRESS(tmp));
                    } */
                }
            }
        }
    }

    if (cfg.pattern == GRBG)
    {
        if (lsc_type == 1)/* flat无插值 */
        {
            /* 此处使用double型，下方h_pos = i * H_CELL_NUM / height */
            /* 由于i的范围为[0, height - 1]，所以 i / height < 1，所以h_pos < H_CELL_NUM，不会溢出 */
            /* 若w_cell、h_cell采用int型，由于C语言会直接去掉小数点（而不是四舍五入） */
            /* 会导致得到的数(int)(w_cell) <= (double)(w_cell)，所以 j / (int)(w_cell) >= j / (double)(w_cell) */
            /* 有可能导致溢出，故保留采用double，下同 */

            w_cell = (double)(width) / (double)(wblock);
            h_cell = (double)(height) / (double)(hblock);

            for (i = 0; i < height; ++i)
            {
                h_pos = i / h_cell; /* 所属的y网格 */
                for (j = 0; j < width; ++j)
                {
                    w_pos = j / w_cell; /* 所属的x网格 */
                    /* 利用与操作代替取余，提高运行速度，此处注释与下方interp的相同*/
                    if ((i & 1) && (j & 1)) /* i、j均为奇数 *///4
                    {
                        gain = gbgain[h_pos * wblock + w_pos] / (double)(GAIN_FACTOR);
                    }
                    else if (!(i & 1) && !(j & 1)) /* i、j均为偶数 *///1
                    {
                        gain = grgain[h_pos * wblock + w_pos] / (double)(GAIN_FACTOR);
                    }
                    else if (!(i & 1) && (j & 1))//2
                    {
                        gain = rgain[h_pos * wblock + w_pos] / (double)(GAIN_FACTOR);
                    }
                    else if ((i & 1) && !(j & 1))//3
                    {
                        gain = bgain[h_pos * wblock + w_pos] / (double)(GAIN_FACTOR);
                    }
                    raw[width * i + j] = clp_range(0, raw[width * i + j] * gain, U16MAX);
                }
            }
        }
        else if (lsc_type == 0)/* 插值 */
        {
            w_cell = (double)(width) / (double)(wblock - 1);
            h_cell = (double)(height) / (double)(hblock - 1);

            for (i = 0; i < height; ++i)
            {
                h_pos = i / h_cell;
                y_weight = (i - h_pos * h_cell) / (double)(h_cell); /* 双线性插值时的权值 */

                for (j = 0; j < width; ++j)
                {
                    w_pos = j / w_cell;
                    x_weight = (j - w_pos * w_cell) / (double)(w_cell); /* 双线性插值时的权值 */

                    if ((i & 1) && (j & 1)) /* i、j均为奇数 */
                    {
                        gain = bilinear_interp(gbgain[h_pos * wblock + w_pos],
                            gbgain[(h_pos + 1) * wblock + w_pos],
                            gbgain[h_pos * wblock + w_pos + 1],
                            gbgain[(h_pos + 1) * wblock + w_pos + 1], x_weight, y_weight) / (double)(GAIN_FACTOR);

                    }
                    else if (!(i & 1) && !(j & 1)) /* i、j均为偶数 */
                    {
                        gain = bilinear_interp(grgain[h_pos * wblock + w_pos],
                            grgain[(h_pos + 1) * wblock + w_pos],
                            grgain[h_pos * wblock + w_pos + 1],
                            grgain[(h_pos + 1) * wblock + w_pos + 1], x_weight, y_weight) / (double)(GAIN_FACTOR);
                    }
                    else if (!(i & 1) && (j & 1))
                    {
                        gain = bilinear_interp(rgain[h_pos * wblock + w_pos],
                            rgain[(h_pos + 1) * wblock + w_pos],
                            rgain[h_pos * wblock + w_pos + 1],
                            rgain[(h_pos + 1) * wblock + w_pos + 1], x_weight, y_weight) / (double)(GAIN_FACTOR);
                    }
                    else if ((i & 1) && !(j & 1))
                    {
                        gain = bilinear_interp(bgain[h_pos * wblock + w_pos],
                            bgain[(h_pos + 1) * wblock + w_pos],
                            bgain[h_pos * wblock + w_pos + 1],
                            bgain[(h_pos + 1) * wblock + w_pos + 1], x_weight, y_weight) / (double)(GAIN_FACTOR);
                    }

                    raw[width * i + j] = clp_range(0, raw[width * i + j] * gain, U16MAX);
                    /* 查看紫色区域 */
                    /* if ((i > 1420) && (i < 1435) && (j > 1820) && (j < 1840))
                    {
                    printf("[shading]i = %d, j = %d, ori_data = %d, gain = %lf, result = %f, final = %lf\n",
                    i, j, data[i * width + j], gain, tmp, SUPPRESS(tmp));
                    } */
                }
            }
        }
    }

    if (cfg.pattern == GBRG)
    {
        if (lsc_type == 1)/* flat无插值 */
        {
            /* 此处使用double型，下方h_pos = i * H_CELL_NUM / height */
            /* 由于i的范围为[0, height - 1]，所以 i / height < 1，所以h_pos < H_CELL_NUM，不会溢出 */
            /* 若w_cell、h_cell采用int型，由于C语言会直接去掉小数点（而不是四舍五入） */
            /* 会导致得到的数(int)(w_cell) <= (double)(w_cell)，所以 j / (int)(w_cell) >= j / (double)(w_cell) */
            /* 有可能导致溢出，故保留采用double，下同 */

            w_cell = (double)(width) / (double)(wblock);
            h_cell = (double)(height) / (double)(hblock);

            for (i = 0; i < height; ++i)
            {
                h_pos = i / h_cell; /* 所属的y网格 */
                for (j = 0; j < width; ++j)
                {
                    w_pos = j / w_cell; /* 所属的x网格 */
                    /* 利用与操作代替取余，提高运行速度，此处注释与下方interp的相同*/
                    if ((i & 1) && (j & 1)) /* i、j均为奇数 *///4
                    {
                        gain = grgain[h_pos * wblock + w_pos] / (double)(GAIN_FACTOR);
                    }
                    else if (!(i & 1) && !(j & 1)) /* i、j均为偶数 *///1
                    {
                        gain = gbgain[h_pos * wblock + w_pos] / (double)(GAIN_FACTOR);
                    }
                    else if (!(i & 1) && (j & 1))//2
                    {
                        gain = bgain[h_pos * wblock + w_pos] / (double)(GAIN_FACTOR);
                    }
                    else if ((i & 1) && !(j & 1))//3
                    {
                        gain = rgain[h_pos * wblock + w_pos] / (double)(GAIN_FACTOR);
                    }
                    raw[width * i + j] = clp_range(0, raw[width * i + j] * gain, U16MAX);
                }
            }
        }
        else if (lsc_type == 0)/* 插值 */
        {
            w_cell = (double)(width) / (double)(wblock - 1);
            h_cell = (double)(height) / (double)(hblock - 1);

            for (i = 0; i < height; ++i)
            {
                h_pos = i / h_cell;
                y_weight = (i - h_pos * h_cell) / (double)(h_cell); /* 双线性插值时的权值 */

                for (j = 0; j < width; ++j)
                {
                    w_pos = j / w_cell;
                    x_weight = (j - w_pos * w_cell) / (double)(w_cell); /* 双线性插值时的权值 */

                    if ((i & 1) && (j & 1)) /* i、j均为奇数 */
                    {
                        gain = bilinear_interp(grgain[h_pos * wblock + w_pos],
                            grgain[(h_pos + 1) * wblock + w_pos],
                            grgain[h_pos * wblock + w_pos + 1],
                            grgain[(h_pos + 1) * wblock + w_pos + 1], x_weight, y_weight) / (double)(GAIN_FACTOR);

                    }
                    else if (!(i & 1) && !(j & 1)) /* i、j均为偶数 */
                    {
                        gain = bilinear_interp(gbgain[h_pos * wblock + w_pos],
                            gbgain[(h_pos + 1) * wblock + w_pos],
                            gbgain[h_pos * wblock + w_pos + 1],
                            gbgain[(h_pos + 1) * wblock + w_pos + 1], x_weight, y_weight) / (double)(GAIN_FACTOR);
                    }
                    else if (!(i & 1) && (j & 1))
                    {
                        gain = bilinear_interp(bgain[h_pos * wblock + w_pos],
                            bgain[(h_pos + 1) * wblock + w_pos],
                            bgain[h_pos * wblock + w_pos + 1],
                            bgain[(h_pos + 1) * wblock + w_pos + 1], x_weight, y_weight) / (double)(GAIN_FACTOR);
                    }
                    else if ((i & 1) && !(j & 1))
                    {
                        gain = bilinear_interp(rgain[h_pos * wblock + w_pos],
                            rgain[(h_pos + 1) * wblock + w_pos],
                            rgain[h_pos * wblock + w_pos + 1],
                            rgain[(h_pos + 1) * wblock + w_pos + 1], x_weight, y_weight) / (double)(GAIN_FACTOR);
                    }

                    raw[width * i + j] = clp_range(0, raw[width * i + j] * gain, U16MAX);
                    /* 查看紫色区域 */
                    /* if ((i > 1420) && (i < 1435) && (j > 1820) && (j < 1840))
                    {
                    printf("[shading]i = %d, j = %d, ori_data = %d, gain = %lf, result = %f, final = %lf\n",
                    i, j, data[i * width + j], gain, tmp, SUPPRESS(tmp));
                    } */
                }
            }
        }
    }


#if DEBUG_MODE
    LOG("done.");
    RGB *rgb_data = raw2rgb(raw, context, cfg);
    save_img_with_timestamp(rgb_data, &context, "_lsc");
#endif



    return OK;
}


U32 bilinear_interp(U16 left_top, U16 left_bottom, U16 right_top, U16 right_bottom, double x_weight, double y_weight)
{
    S32 x_up = 0;
    S32 x_down = 0;
    S32 res = 0;

    /* x方向插值 */
    x_up = (1 - x_weight) * left_top + x_weight * right_top;
    x_down = (1 - x_weight) * left_bottom + x_weight * right_bottom;
    /* y方向插值 */
    res = (1 - y_weight) * x_up + y_weight * x_down;
    return res;
}
