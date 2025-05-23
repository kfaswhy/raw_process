#pragma once

#include "raw_process.h"


#define ENV_NUM_TP 16
#define WDR_RAW_STR_Y_POINT 16
#define WDR_RAW_NUM_X 16
#define WDR_RAW_HIST_SIZE 16




typedef struct _WDR_RAW_CONFIG_TP
{
	U16 update_ratio[ENV_NUM_TP];							/* 更新速率，0~256 */
	U16 scale[ENV_NUM_TP];									/* sigma参数，0~255 */
	U16 manual_detail_enahnce[ENV_NUM_TP];					/* 细节增强系数，0~255 */
	U16 global_dark_tone_enhance[ENV_NUM_TP];				/* 全局wdr档数，0~15 */
	U16 wdr_str_by_y[ENV_NUM_TP][WDR_RAW_STR_Y_POINT];		/* 根据亮度变化的混合比例，0~255 */
	U16 strength[ENV_NUM_TP];								/* 混合比例，0~255 */
	U16 dark_limit[ENV_NUM_TP];								/* 暗区增亮限制，0~254 */
	U16 bright_limit[ENV_NUM_TP];							/* 亮区压暗限制，0~255 */
	U8 enable;												/* 模块enable参数，1使能，0 bypass，默认0 */
	U8 auto_mode;											/* 联动/手动 */
} WDR_RAW_CONFIG_TP;

typedef struct _WDR_RAW_CONTEXT
{
	WDR_RAW_CONFIG_TP wdr_raw_config_tp;					/* 配置 */
	//RAW_PARAMS wdr_raw_result;								/* 结果 */
	U16* wdr_coded_gamma_cruve;								/* 编码gamma */
	U16* wdr_decoded_gamma_cruve;							/* 解码gamma */
	U16* wdr_global_cruve[16];								/* 全局wdr映射曲线 */
	U16* str_by_y_16bit[ENV_NUM_TP];						/* 插值后的根据亮度变化的混合比例 */
	U16 wdr_raw_env;										/* 使用的env,[0,16],0是手动 */
	U16(*wdr_hist)[WDR_RAW_NUM_X][WDR_RAW_HIST_SIZE];		/* 亮度直方图，包含历史亮度信息 */
	U16(*wdr_cur_hist)[WDR_RAW_NUM_X][WDR_RAW_HIST_SIZE];	/* 亮度直方图，包含当前帧亮度信息 */
	U8 trigger_type;										/* 触发本模块的id */
	U8 dump_flag;											/* 是否保存本模块结果flag */
} WDR_RAW_CONTEXT;


U8 ltm_process(U16* raw, IMG_CONTEXT context, G_CONFIG cfg);

void cal_ori_Y(U16* ori_y, U16* data, U16 width, U16 height);

