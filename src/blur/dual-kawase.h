#pragma once

#include <math.h>
#include <obs-module.h>
#include "../obs-utils.h"
#include "alpha-blur.h"


extern void load_effect_dual_kawase(alpha_blur_data_t *filter);

extern void dual_kawase_blur(int radius, bool include_border,
			     alpha_blur_data_t *data,
			     gs_texrender_t *input);
static void
load_dual_kawase_down_sample_effect(alpha_blur_data_t *filter);
static void load_dual_kawase_up_sample_effect(alpha_blur_data_t *filter);
static void load_dual_kawase_mix_effect(alpha_blur_data_t *filter);

static gs_texture_t *mix_textures(alpha_blur_data_t *data,
				  gs_texture_t *base, gs_texture_t *residual,
				  float ratio);
static gs_texture_t *down_sample(alpha_blur_data_t *data,
				 gs_texture_t *input_texture, int divisor,
				 float ratio, uint32_t width, uint32_t height);
static gs_texture_t *up_sample(alpha_blur_data_t *data,
			       gs_texture_t *input_texture, int divisor,
			       float ratio, uint32_t width, uint32_t height);
