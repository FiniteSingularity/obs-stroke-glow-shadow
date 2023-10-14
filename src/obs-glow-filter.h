#pragma once

#include <obs-module.h>

#include "version.h"
#include "obs-glow.h"
#include "obs-utils.h"

static const char *glow_filter_name(void *unused);
static const char *shadow_filter_name(void *unused);
static void *shadow_filter_create(obs_data_t *settings, obs_source_t *source);
static void *glow_filter_create(obs_data_t *settings, obs_source_t *source);
static glow_filter_data_t *filter_create(obs_source_t *source);

static void glow_filter_destroy(void *data);
static uint32_t glow_filter_width(void *data);
static uint32_t glow_filter_height(void *data);
static void glow_filter_update(void *data, obs_data_t *settings);
static void glow_filter_video_render(void *data, gs_effect_t *effect);
static obs_properties_t *glow_filter_properties(void *data);
static obs_properties_t *glow_source_properties(void *data);
static obs_properties_t *shadow_filter_properties(void *data);
static obs_properties_t *shadow_source_properties(void *data);
static void glow_filter_video_tick(void *data, float seconds);
static void glow_filter_defaults(obs_data_t *settings);
static void shadow_filter_defaults(obs_data_t *settings);
static void get_input_source(glow_filter_data_t *filter);
static void draw_output(glow_filter_data_t *filter);
static void load_effects(glow_filter_data_t *filter);
static bool setting_fill_type_modified(obs_properties_t *props,
				       obs_property_t *p, obs_data_t *settings);
static bool setting_glow_position_modified(void *data,
					   obs_properties_t *props,
					   obs_property_t *p,
					   obs_data_t *settings);
