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
	gs_effect_t *effect;
	gs_effect_t *effect_2;
	gs_effect_t *composite_effect;
	gs_effect_t *mix_effect;
	gs_effect_t *effect_mask_effect;

	// Render pipeline
	bool input_rendered;
	gs_texrender_t *input_texrender;
	bool output_rendered;
	gs_texrender_t *output_texrender;
	// Frame Buffers
	gs_texrender_t *render;
	gs_texrender_t *render2;
	// Renderer for composite render step
	gs_texrender_t *composite_render;

	bool rendering;
	bool reload;
	bool rendered;

	uint32_t width;
	uint32_t height;

	// Callback Functions
	// void (*video_render)(stroke_filter_data_t *filter);
	// void (*load_effect)(stroke_filter_data_t *filter);
	// void (*update)(stroke_filter_data_t *filter);
};
