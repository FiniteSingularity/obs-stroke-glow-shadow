#pragma once

#include <obs-module.h>
#include "blur/alpha-blur.h"
#include "defines.h"

#define DEFAULT_COLOR 4294967295

#define STROKE_FILL_TYPE_COLOR_LABEL "StrokeFilter.ColorFill"
#define STROKE_FILL_TYPE_SOURCE_LABEL "StrokeFilter.SourceFill"
#define STROKE_FILL_TYPE_IMAGE_LABEL "StrokeFilter.ImageFill"

enum stroke_fill_type {
	STROKE_FILL_TYPE_COLOR = 1,
	STROKE_FILL_TYPE_SOURCE,
	STROKE_FILL_TYPE_IMAGE,
};

#define OFFSET_QUALITY_NORMAL_LABEL "StrokeFilter.OffsetQualityNormal"
#define OFFSET_QUALITY_HIGH_LABEL "StrokeFilter.OffsetQualityHigh"

enum offset_quality {
	OFFSET_QUALITY_NORMAL = 1,
	OFFSET_QUALITY_HIGH,
};

#define STROKE_POSITION_OUTER_LABEL "StrokeFilter.StrokePositionOuter"
#define STROKE_POSITION_INNER_LABEL "StrokeFilter.StrokePositionInner"
#define STROKE_POSITION_OUTER_CONTOUR_LABEL "StrokeFilter.StrokePositionOuterContour"
#define STROKE_POSITION_INNER_CONTOUR_LABEL "StrokeFilter.StrokePositionInnerContour"

enum stroke_position {
	STROKE_POSITION_OUTER = 1,
	STROKE_POSITION_INNER,
	STROKE_POSITION_OUTER_CONTOUR,
	STROKE_POSITION_INNER_CONTOUR
};

struct stroke_filter_data;
typedef struct stroke_filter_data stroke_filter_data_t;

struct stroke_filter_data {
	obs_source_t *context;
	alpha_blur_data_t *alpha_blur_data;

	obs_weak_source_t *source_input_source;

	bool is_filter;
	bool is_source;

	// Effects
	gs_effect_t *effect_stroke;
	gs_effect_t *effect_jump_flood_sdf;
	gs_effect_t *effect_stroke_inner;
	gs_effect_t *effect_anti_alias;
	gs_effect_t *effect_fill_stroke;
	gs_effect_t *effect_output;

	// Render pipeline
	bool input_rendered;
	gs_texrender_t *input_texrender;
	bool output_rendered;
	gs_texrender_t* output_texrender;
	// Frame Buffers
	gs_texrender_t* stroke_mask;

	gs_texrender_t* buffer_a;
	gs_texrender_t* buffer_b;
	gs_texrender_t* buffer_outer_threshold;
	gs_texrender_t* buffer_inner_threshold;
	gs_texrender_t* buffer_outer_distance_field;
	gs_texrender_t* buffer_inner_distance_field;

	bool input_texture_generated;
	bool rendering;
	bool reload;
	bool rendered;

	uint32_t width;
	uint32_t height;

	// Parameters
	float stroke_size;
	float stroke_offset;
	float stroke_distance;
	float contour_spacing;
	float contour_offset;
	float contour_falloff_start;
	float contour_falloff_end;
	float contour_spacing_power;

	uint32_t pad_t;
	uint32_t pad_b;
	uint32_t pad_l;
	uint32_t pad_r;

	uint32_t padding_amount;

	uint32_t source_width;
	uint32_t source_height;

	uint32_t output_width;
	uint32_t output_height;

	bool ignore_source_border;
	bool fill;

	float jump_flood_threshold;

	struct vec4 stroke_color;
	struct vec4 stroke_color_srgb;

	enum stroke_fill_type fill_type;
	obs_weak_source_t *fill_source_source;

	enum stroke_position stroke_position;

	struct vec2 mul_val;
	struct vec2 add_val;

	gs_eparam_t* param_stroke_texel_step;
	gs_eparam_t* param_stroke_stroke_thickness;
	gs_eparam_t* param_stroke_offset;

	gs_eparam_t* param_stroke_inner_texel_step;
	gs_eparam_t* param_stroke_inner_stroke_thickness;
	gs_eparam_t* param_stroke_inner_offset;

	gs_eparam_t* param_fill_stroke_image;
	gs_eparam_t* param_fill_stroke_stroke_mask;
	gs_eparam_t* param_fill_stroke_fill_source;
	gs_eparam_t* param_fill_stroke_fill_color;
	gs_eparam_t* param_fill_stroke_fill_behind;

	gs_eparam_t* param_aa_texel_step;
	gs_eparam_t* param_aa_size;
	gs_eparam_t* param_aa_image;

	gs_eparam_t* param_jump_flood_threshold;
	gs_eparam_t* param_stroke_uv_size;
	gs_eparam_t* param_jump_flood_offset;
	gs_eparam_t* param_jump_flood_stroke_offset;
	gs_eparam_t* param_jump_flood_stroke_extent;
	gs_eparam_t* param_jump_flood_overlay;
	gs_eparam_t* param_jump_flood_stroke_color;
	gs_eparam_t* param_jump_flood_fill_source;
	gs_eparam_t* param_jump_flood_contour_offset;
	gs_eparam_t* param_jump_flood_contour_spacing;
	gs_eparam_t* param_jump_flood_contour_falloff_start;
	gs_eparam_t* param_jump_flood_contour_falloff_end;
	gs_eparam_t* param_jump_flood_contour_spacing_power;
	gs_eparam_t* param_inner_distance_field;
	gs_eparam_t* param_outer_distance_field;
	gs_eparam_t* param_infill;

	gs_eparam_t* param_mul_val;
	gs_eparam_t* param_add_val;

	gs_eparam_t* param_output_image;

	// Callback Functions
	void (*video_render)(stroke_filter_data_t *filter);
	void (*load_effect)(stroke_filter_data_t *filter);
	void (*update)(stroke_filter_data_t *filter);
};
