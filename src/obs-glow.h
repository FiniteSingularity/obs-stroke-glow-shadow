#pragma once

#include <obs-module.h>
#include "blur/alpha-blur.h"
#include "defines.h"

// Default glow color: white in ABGR
#define DEFAULT_COLOR_GLOW 0xFFFFFFFF
// Default shadow color: dark gray in ABGR
#define DEFAULT_COLOR_SHADOW 0xFF111111

#define GLOW_FILL_TYPE_COLOR_LABEL "StrokeFilter.ColorFill"
#define GLOW_FILL_TYPE_SOURCE_LABEL "StrokeFilter.SourceFill"
#define GLOW_FILL_TYPE_IMAGE_LABEL "StrokeFilter.ImageFill"

enum glow_fill_type {
	GLOW_FILL_TYPE_COLOR = 1,
	GLOW_FILL_TYPE_SOURCE,
	GLOW_FILL_TYPE_IMAGE,
};

#define GLOW_POSITION_OUTER_LABEL "GlowFilter.PositionOuter"
#define SHADOW_POSITION_OUTER_LABEL "ShadowFilter.PositionOuter"
#define GLOW_POSITION_INNER_LABEL "GlowFilter.PositionInner"
#define SHADOW_POSITION_INNER_LABEL "ShadowFilter.PositionInner"

enum glow_position {
	GLOW_POSITION_OUTER = 1,
	GLOW_POSITION_INNER,
};

#define BLUR_TYPE_TRIANGULAR_LABEL "GlowShadowFilter.BlurType.Triangular"
#define BLUR_TYPE_DUAL_KAWASE_LABEL "GlowShadowFilter.BlurType.DualKawase"

enum blur_type {
	BLUR_TYPE_TRIANGULAR = 1,
	BLUR_TYPE_DUAL_KAWASE,
};

enum filter_type {
	FILTER_TYPE_GLOW = 1,
	FILTER_TYPE_SHADOW,
};

struct glow_filter_data;
typedef struct glow_filter_data glow_filter_data_t;

struct glow_filter_data {
	enum filter_type filter_type;
	obs_source_t *context;
	alpha_blur_data_t *alpha_blur_data;

	obs_weak_source_t *source_input_source;

	bool is_filter;
	bool is_source;

	// Effects
	gs_effect_t *effect_glow;
	gs_effect_t *effect_output;

	// Render pipeline
	bool input_rendered;
	gs_texrender_t *input_texrender;
	bool output_rendered;
	gs_texrender_t *output_texrender;
	gs_texrender_t* alpha_mask_texrender;
	// Frame Buffers

	bool input_texture_generated;
	bool rendering;
	bool reload;
	bool rendered;

	uint32_t width;
	uint32_t height;
	uint32_t source_width;
	uint32_t source_height;

	// Parameters
	float glow_size;
	float intensity;
	bool ignore_source_border;
	bool fill;
	enum blur_type blur_type;
	struct vec2 offset_texel;
	float threshold;

	uint32_t pad_t;
	uint32_t pad_b;
	uint32_t pad_l;
	uint32_t pad_r;

	uint32_t padding_amount;

	struct vec2 mul_val;
	struct vec2 add_val;

	struct vec4 glow_color;
	struct vec4 glow_color_srgb;
	enum glow_fill_type fill_type;
	obs_weak_source_t *fill_source_source;

	enum glow_position glow_position;

	gs_eparam_t *param_glow_texel_step;
	gs_eparam_t *param_glow_image;
	gs_eparam_t *param_glow_mask;
	gs_eparam_t *param_glow_fill_source;
	gs_eparam_t *param_glow_fill_color;
	gs_eparam_t *param_glow_intensity;
	gs_eparam_t *param_glow_fill_behind;
	gs_eparam_t *param_offset_texel;

	gs_eparam_t* param_mul_val;
	gs_eparam_t* param_add_val;

	gs_eparam_t* param_threshold;

	gs_eparam_t *param_output_image;
};
