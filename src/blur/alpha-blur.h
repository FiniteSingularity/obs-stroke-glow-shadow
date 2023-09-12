#pragma once

#include <math.h>
#include <obs-module.h>
#include "../obs-utils.h"
#include "../obs-stroke.h"

extern void alpha_blur(stroke_filter_data_t *data, float radius,
		       gs_texrender_t *output);
extern void load_1d_alpha_blur_effect(stroke_filter_data_t *filter);
