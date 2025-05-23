#pragma once

#include "raw_process.h"


#define ENV_NUM_TP 16
#define WDR_RAW_STR_Y_POINT 16
#define WDR_RAW_NUM_X 16
#define WDR_RAW_HIST_SIZE 16




typedef struct _WDR_RAW_CONFIG_TP
{
	U16 update_ratio[ENV_NUM_TP];							/* �������ʣ�0~256 */
	U16 scale[ENV_NUM_TP];									/* sigma������0~255 */
	U16 manual_detail_enahnce[ENV_NUM_TP];					/* ϸ����ǿϵ����0~255 */
	U16 global_dark_tone_enhance[ENV_NUM_TP];				/* ȫ��wdr������0~15 */
	U16 wdr_str_by_y[ENV_NUM_TP][WDR_RAW_STR_Y_POINT];		/* �������ȱ仯�Ļ�ϱ�����0~255 */
	U16 strength[ENV_NUM_TP];								/* ��ϱ�����0~255 */
	U16 dark_limit[ENV_NUM_TP];								/* �����������ƣ�0~254 */
	U16 bright_limit[ENV_NUM_TP];							/* ����ѹ�����ƣ�0~255 */
	U8 enable;												/* ģ��enable������1ʹ�ܣ�0 bypass��Ĭ��0 */
	U8 auto_mode;											/* ����/�ֶ� */
} WDR_RAW_CONFIG_TP;

typedef struct _WDR_RAW_CONTEXT
{
	WDR_RAW_CONFIG_TP wdr_raw_config_tp;					/* ���� */
	//RAW_PARAMS wdr_raw_result;								/* ��� */
	U16* wdr_coded_gamma_cruve;								/* ����gamma */
	U16* wdr_decoded_gamma_cruve;							/* ����gamma */
	U16* wdr_global_cruve[16];								/* ȫ��wdrӳ������ */
	U16* str_by_y_16bit[ENV_NUM_TP];						/* ��ֵ��ĸ������ȱ仯�Ļ�ϱ��� */
	U16 wdr_raw_env;										/* ʹ�õ�env,[0,16],0���ֶ� */
	U16(*wdr_hist)[WDR_RAW_NUM_X][WDR_RAW_HIST_SIZE];		/* ����ֱ��ͼ��������ʷ������Ϣ */
	U16(*wdr_cur_hist)[WDR_RAW_NUM_X][WDR_RAW_HIST_SIZE];	/* ����ֱ��ͼ��������ǰ֡������Ϣ */
	U8 trigger_type;										/* ������ģ���id */
	U8 dump_flag;											/* �Ƿ񱣴汾ģ����flag */
} WDR_RAW_CONTEXT;


U8 ltm_process(U16* raw, IMG_CONTEXT context, G_CONFIG cfg);

void cal_ori_Y(U16* ori_y, U16* data, U16 width, U16 height);

