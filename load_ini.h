#pragma once

#include "raw_process.h"


void load_cfg(G_CONFIG* cfg);

void load_cfg_from_ini(const char* filename, G_CONFIG* cfg);

static char* trim(char* s);

static ByteOrder parse_order(const char* s);

static BayerPattern parse_pattern(const char* s);

static double eval_simple_expr(const char* s);

static int parse_u16_array(const char* s, U16** out);

static void parse_float_array9(const char* s, float out[9]);
