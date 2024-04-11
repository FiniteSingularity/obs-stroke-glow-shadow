#pragma once

#include "obs-stroke.h"
#include "obs-utils.h"

void load_stroke_inner_effect(stroke_filter_data_t *filter);
void load_output_effect(stroke_filter_data_t *filter);
void load_stroke_effect(stroke_filter_data_t *filter);
void load_fill_stroke_effect(stroke_filter_data_t *filter);
void render_stroke_filter(stroke_filter_data_t *data);
void render_fill_stroke_filter(stroke_filter_data_t *data);
