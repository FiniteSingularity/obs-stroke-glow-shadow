#pragma once

#include <obs-module.h>
// #include <util/base.h>
// #include <util/dstr.h>
// #include <util/darray.h>
// #include <util/platform.h>
// #include <graphics/image-file.h>

// #include <stdio.h>

#include "version.h"
#include "obs-stroke.h"
#include "obs-utils.h"

static const char *stroke_filter_name(void *unused);
static void *stroke_filter_create(obs_data_t *settings, obs_source_t *source);
static void stroke_filter_destroy(void *data);
static uint32_t stroke_filter_width(void *data);
static uint32_t stroke_filter_height(void *data);
static void stroke_filter_update(void *data, obs_data_t *settings);
static void stroke_filter_video_render(void *data, gs_effect_t *effect);
static obs_properties_t *stroke_filter_properties(void *data);
static obs_properties_t *stroke_source_properties(void *data);
static void stroke_filter_video_tick(void *data, float seconds);
static void stroke_filter_defaults(obs_data_t *settings);
static void get_input_source(stroke_filter_data_t *filter);
static void draw_output(stroke_filter_data_t *filter);
static void load_effects(stroke_filter_data_t *filter);
static bool setting_fill_type_modified(obs_properties_t *props,
				       obs_property_t *p, obs_data_t *settings);
static bool setting_stroke_position_modified(void *data,
					     obs_properties_t *props,
					     obs_property_t *p,
					     obs_data_t *settings);
