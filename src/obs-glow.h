#pragma once

#include <obs-module.h>
#include "blur/alpha-blur.h"

#define DEFAULT_COLOR 4294967295

#define GLOW_FILL_TYPE_COLOR 1
#define GLOW_FILL_TYPE_COLOR_LABEL "StrokeFilter.ColorFill"
#define GLOW_FILL_TYPE_SOURCE 2
#define GLOW_FILL_TYPE_SOURCE_LABEL "StrokeFilter.SourceFill"
#define GLOW_FILL_TYPE_IMAGE 3
#define GLOW_FILL_TYPE_IMAGE_LABEL "StrokeFilter.ImageFill"

#define GLOW_POSITION_OUTER 1
#define GLOW_POSITION_OUTER_LABEL "StrokeFilter.GlowPositionOuter"
#define GLOW_POSITION_INNER 2
#define GLOW_POSITION_INNER_LABEL "StrokeFilter.GlowPositionInner"

struct glow_filter_data;
typedef struct glow_filter_data glow_filter_data_t;

struct glow_filter_data {
	obs_source_t *context;
	alpha_blur_data_t *alpha_blur_data;

	// Effects
	gs_effect_t *effect_glow;

	// Render pipeline
	bool input_rendered;
	gs_texrender_t *input_texrender;
	bool output_rendered;
	gs_texrender_t *output_texrender;
	// Frame Buffers

	bool rendering;
	bool reload;
	bool rendered;

	uint32_t width;
	uint32_t height;

	// Parameters
	float glow_size;
	float intensity;

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
};
