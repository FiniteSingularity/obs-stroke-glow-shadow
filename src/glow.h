#pragma once

#include "obs-glow.h"
#include "obs-utils.h"

void load_glow_effect(glow_filter_data_t *filter);
void load_glow_output_effect(glow_filter_data_t *filter);
void render_glow_filter(glow_filter_data_t *data);
