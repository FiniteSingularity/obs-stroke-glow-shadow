#pragma once

#include <obs-module.h>
#include "blur/alpha-blur.h"

#define PLUGIN_INFO                                                                                                         \
	"<a href=\"https://github.com/finitesingularity/obs-stroke-glow-shadow/\">Stroke Glow Shadow</a> (" PROJECT_VERSION \
	") by <a href=\"https://twitch.tv/finitesingularity\">FiniteSingularity</a>"

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

enum stroke_position {
	STROKE_POSITION_OUTER = 1,
	STROKE_POSITION_INNER,
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
	gs_effect_t *effect_stroke_inner;
	gs_effect_t *effect_anti_alias;
	gs_effect_t *effect_fill_stroke;

	// Render pipeline
	bool input_rendered;
	gs_texrender_t *input_texrender;
	bool output_rendered;
	gs_texrender_t *output_texrender;
	// Frame Buffers
	gs_texrender_t *stroke_mask;

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
	bool anti_alias;
	bool ignore_source_border;
	bool fill;

	struct vec4 stroke_color;
	enum stroke_fill_type fill_type;
	obs_weak_source_t *fill_source_source;

	enum offset_quality offset_quality;
	enum stroke_position stroke_position;

	gs_eparam_t *param_stroke_texel_step;
	gs_eparam_t *param_stroke_stroke_thickness;
	gs_eparam_t *param_stroke_offset;

	gs_eparam_t *param_stroke_inner_texel_step;
	gs_eparam_t *param_stroke_inner_stroke_thickness;
	gs_eparam_t *param_stroke_inner_offset;

	gs_eparam_t *param_fill_stroke_image;
	gs_eparam_t *param_fill_stroke_stroke_mask;
	gs_eparam_t *param_fill_stroke_fill_source;
	gs_eparam_t *param_fill_stroke_fill_color;
	gs_eparam_t *param_fill_stroke_fill_behind;

	gs_eparam_t *param_aa_texel_step;
	gs_eparam_t *param_aa_size;
	gs_eparam_t *param_aa_image;

	// Callback Functions
	void (*video_render)(stroke_filter_data_t *filter);
	void (*load_effect)(stroke_filter_data_t *filter);
	void (*update)(stroke_filter_data_t *filter);
};
