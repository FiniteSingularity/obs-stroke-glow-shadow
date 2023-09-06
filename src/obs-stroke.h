#pragma once

#include <obs-module.h>

#define PLUGIN_INFO                                                                                 \
	"<a href=\"https://github.com/finitesingularity/obs-stroke/\">Stroke</a> (" PROJECT_VERSION \
	") by <a href=\"https://twitch.tv/finitesingularity\">FiniteSingularity</a>"

struct stroke_filter_data;
typedef struct stroke_filter_data stroke_filter_data_t;

struct stroke_filter_data {
	obs_source_t *context;

	// Effects
	gs_effect_t *effect_alpha_blur;
	gs_effect_t *effect_stroke;

	// Render pipeline
	bool input_rendered;
	gs_texrender_t *input_texrender;
	bool output_rendered;
	gs_texrender_t *output_texrender;
	// Frame Buffers
	gs_texrender_t *alpha_blur_pass_1;
	gs_texrender_t *alpha_blur_output;
	gs_texrender_t *mix_output;

	bool rendering;
	bool reload;
	bool rendered;

	uint32_t width;
	uint32_t height;

	// Parameters
	float stroke_size;
	float stroke_distance;
	struct vec4 stroke_color;

	gs_eparam_t *param_blur_radius;
	gs_eparam_t *param_blur_texel_step;
	gs_eparam_t *param_stroke_texel_step;
	gs_eparam_t *param_stroke_stroke_thickness;
	gs_eparam_t *param_stroke_color;


	// Callback Functions
	void (*video_render)(stroke_filter_data_t *filter);
	void (*load_effect)(stroke_filter_data_t *filter);
	void (*update)(stroke_filter_data_t *filter);
};
