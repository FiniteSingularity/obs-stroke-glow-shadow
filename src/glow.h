#pragma once

#include "obs-glow.h"
#include "obs-utils.h"

void load_glow_effect(glow_filter_data_t *filter);
void load_glow_output_effect(glow_filter_data_t *filter);
void render_glow_filter(glow_filter_data_t *data);
void glow_render_padded_input(glow_filter_data_t* data);
void glow_render_cropped_output(glow_filter_data_t* data);
void render_glow_alpha_mask(glow_filter_data_t* data);
