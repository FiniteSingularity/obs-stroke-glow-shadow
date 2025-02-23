#pragma once

#include "obs-stroke.h"
#include "obs-utils.h"

void load_stroke_inner_effect(stroke_filter_data_t *filter);
void load_jump_flood_sdf_effect(stroke_filter_data_t* filter);
void load_output_effect(stroke_filter_data_t *filter);
void load_stroke_effect(stroke_filter_data_t *filter);
void load_fill_stroke_effect(stroke_filter_data_t *filter);
//void render_stroke_filter(stroke_filter_data_t *data);
void render_padded_input(stroke_filter_data_t* data);
void render_cropped_output(stroke_filter_data_t* data);
void render_jf_outer_threshold(stroke_filter_data_t* data);
void render_jf_inner_threshold(stroke_filter_data_t* data);
void render_jf_passes_outer(stroke_filter_data_t* data, float maxExtent);
void render_jf_passes_inner(stroke_filter_data_t* data, float maxExtent);
void render_jf_distance(stroke_filter_data_t* data);
void render_fill_stroke_filter(stroke_filter_data_t *data);
