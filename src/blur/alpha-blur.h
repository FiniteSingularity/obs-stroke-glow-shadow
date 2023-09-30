#pragma once

#include <math.h>
#include <obs-module.h>
#include "../obs-utils.h"

struct alpha_blur_data;
typedef struct alpha_blur_data alpha_blur_data_t;

struct alpha_blur_data {
	// Conical/Triangular alpha blur
	gs_effect_t *effect_alpha_blur;
	gs_texrender_t *alpha_blur_pass_1;
	gs_texrender_t *alpha_blur_output;
	gs_texrender_t *alpha_blur_output_2;

	gs_eparam_t *param_blur_radius;
	gs_eparam_t *param_blur_texel_step;

	// Dual Kawase Blur
	gs_effect_t *effect_dual_kawase_downsample;
	gs_effect_t *effect_dual_kawase_upsample;
	gs_effect_t *effect_dual_kawase_mix;
	gs_texrender_t *render;
	gs_texrender_t *render2;

	gs_eparam_t *param_downsample_texel_step;
	gs_eparam_t *param_upsample_texel_step;

	gs_eparam_t *param_mix_ratio;
	gs_eparam_t *param_mix_image;
	gs_eparam_t *param_mix_image2;
};

extern void alpha_blur_init(alpha_blur_data_t *data);
extern void alpha_blur_destroy(alpha_blur_data_t *data);
extern void alpha_blur(float radius, bool include_border,
		       alpha_blur_data_t *data, gs_texrender_t *input,
		       gs_texrender_t *output);
extern void load_1d_alpha_blur_effect(alpha_blur_data_t * filter);
