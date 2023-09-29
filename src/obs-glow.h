#pragma once

#include <obs-module.h>
#include "blur/alpha-blur.h"

#define PLUGIN_INFO                                                                                                         \
	"<a href=\"https://github.com/finitesingularity/obs-stroke-glow-shadow/\">Stroke Glow Shadow</a> (" PROJECT_VERSION \
	") by <a href=\"https://twitch.tv/finitesingularity\">FiniteSingularity</a>"

// Default glow color: white in ABGR
#define DEFAULT_COLOR_GLOW 0xFFFFFFFF
// Default shadow color: dark gray in ABGR
#define DEFAULT_COLOR_SHADOW 0xFF111111

#define GLOW_FILL_TYPE_COLOR 1
#define GLOW_FILL_TYPE_COLOR_LABEL "StrokeFilter.ColorFill"
#define GLOW_FILL_TYPE_SOURCE 2
#define GLOW_FILL_TYPE_SOURCE_LABEL "StrokeFilter.SourceFill"
#define GLOW_FILL_TYPE_IMAGE 3
#define GLOW_FILL_TYPE_IMAGE_LABEL "StrokeFilter.ImageFill"

#define GLOW_POSITION_OUTER 1
#define GLOW_POSITION_OUTER_LABEL "GlowFilter.PositionOuter"
#define SHADOW_POSITION_OUTER_LABEL "ShadowFilter.PositionOuter"
#define GLOW_POSITION_INNER 2
#define GLOW_POSITION_INNER_LABEL "GlowFilter.PositionInner"
#define SHADOW_POSITION_INNER_LABEL "ShadowFilter.PositionInner"

#define FILTER_TYPE_GLOW 1
#define FILTER_TYPE_SHADOW 2

struct glow_filter_data;
typedef struct glow_filter_data glow_filter_data_t;

struct glow_filter_data {
	uint32_t filter_type;
	obs_source_t *context;
	alpha_blur_data_t *alpha_blur_data;

	obs_weak_source_t *source_input_source;

	bool is_filter;
	bool is_source;

	// Effects
	gs_effect_t *effect_glow;

	// Render pipeline
	bool input_rendered;
	gs_texrender_t *input_texrender;
	bool output_rendered;
	gs_texrender_t *output_texrender;
	// Frame Buffers

	bool input_texture_generated;
	bool rendering;
	bool reload;
	bool rendered;

	uint32_t width;
	uint32_t height;

	// Parameters
	float glow_size;
	float intensity;
	bool ignore_source_border;
	bool fill;
	bool use_kawase;
	struct vec2 offset_texel;

	struct vec4 glow_color;
	uint32_t fill_type;
	obs_weak_source_t *fill_source_source;

	uint32_t glow_position;

	gs_eparam_t *param_glow_texel_step;
	gs_eparam_t *param_glow_image;
	gs_eparam_t *param_glow_mask;
	gs_eparam_t *param_glow_fill_source;
	gs_eparam_t *param_glow_fill_color;
	gs_eparam_t *param_glow_intensity;
	gs_eparam_t *param_glow_fill_behind;
	gs_eparam_t *param_offset_texel;
};
