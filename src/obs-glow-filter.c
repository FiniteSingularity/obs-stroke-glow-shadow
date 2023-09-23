#include "obs-glow-filter.h"
#include "obs-glow.h"
#include "blur/alpha-blur.h"
#include "glow.h"

#include <math.h>

struct obs_source_info obs_glow_filter = {
	.id = "obs_glow_filter",
	.type = OBS_SOURCE_TYPE_FILTER,
	.output_flags = OBS_SOURCE_VIDEO,
	.get_name = glow_filter_name,
	.create = glow_filter_create,
	.destroy = glow_filter_destroy,
	.update = glow_filter_update,
	.video_render = glow_filter_video_render,
	.video_tick = glow_filter_video_tick,
	.get_width = glow_filter_width,
	.get_height = glow_filter_height,
	.get_properties = glow_filter_properties,
	.get_defaults = glow_filter_defaults};

struct obs_source_info obs_glow_source = {
	.id = "obs_glow_source",
	.type = OBS_SOURCE_TYPE_INPUT,
	.output_flags = OBS_SOURCE_VIDEO | OBS_SOURCE_CUSTOM_DRAW,
	.get_name = glow_filter_name,
	.create = glow_filter_create,
	.destroy = glow_filter_destroy,
	.update = glow_filter_update,
	.video_render = glow_filter_video_render,
	.video_tick = glow_filter_video_tick,
	.get_width = glow_filter_width,
	.get_height = glow_filter_height,
	.get_properties = glow_filter_properties,
	.get_defaults = glow_filter_defaults,
	.icon_type = OBS_ICON_TYPE_COLOR};

struct obs_source_info obs_shadow_filter = {
	.id = "obs_shadow_filter",
	.type = OBS_SOURCE_TYPE_FILTER,
	.output_flags = OBS_SOURCE_VIDEO,
	.get_name = shadow_filter_name,
	.create = shadow_filter_create,
	.destroy = glow_filter_destroy,
	.update = glow_filter_update,
	.video_render = glow_filter_video_render,
	.video_tick = glow_filter_video_tick,
	.get_width = glow_filter_width,
	.get_height = glow_filter_height,
	.get_properties = glow_filter_properties,
	.get_defaults = shadow_filter_defaults};

struct obs_source_info obs_shadow_source = {
	.id = "obs_shadow_source",
	.type = OBS_SOURCE_TYPE_INPUT,
	.output_flags = OBS_SOURCE_VIDEO | OBS_SOURCE_CUSTOM_DRAW,
	.get_name = shadow_filter_name,
	.create = shadow_filter_create,
	.destroy = glow_filter_destroy,
	.update = glow_filter_update,
	.video_render = glow_filter_video_render,
	.video_tick = glow_filter_video_tick,
	.get_width = glow_filter_width,
	.get_height = glow_filter_height,
	.get_properties = glow_filter_properties,
	.get_defaults = shadow_filter_defaults,
	.icon_type = OBS_ICON_TYPE_COLOR};

static const char *glow_filter_name(void *unused)
{
	UNUSED_PARAMETER(unused);
	return obs_module_text("GlowFilter");
}

static const char *shadow_filter_name(void *unused)
{
	UNUSED_PARAMETER(unused);
	return obs_module_text("ShadowFilter");
}

static void *glow_filter_create(obs_data_t *settings, obs_source_t *source)
{
	glow_filter_data_t *filter = filter_create(source);
	filter->filter_type = FILTER_TYPE_GLOW;
	obs_source_update(source, settings);
	return filter;
}

static void *shadow_filter_create(obs_data_t *settings, obs_source_t *source)
{
	glow_filter_data_t *filter = filter_create(source);
	filter->filter_type = FILTER_TYPE_SHADOW;
	obs_source_update(source, settings);
	return filter;
}

static glow_filter_data_t *filter_create(obs_source_t *source)
{
	glow_filter_data_t *filter = bzalloc(sizeof(glow_filter_data_t));

	filter->context = source;
	filter->input_texture_generated = false;
	filter->alpha_blur_data = bzalloc(sizeof(alpha_blur_data_t));

	filter->is_source = obs_source_get_type(filter->context) ==
			    OBS_SOURCE_TYPE_INPUT;

	filter->is_filter = obs_source_get_type(filter->context) ==
			    OBS_SOURCE_TYPE_FILTER;

	filter->param_glow_texel_step = NULL;
	filter->param_glow_image = NULL;
	filter->param_glow_mask = NULL;
	filter->param_glow_fill_source = NULL;
	filter->param_glow_fill_color = NULL;
	filter->param_glow_texel_step = NULL;
	filter->param_glow_intensity = NULL;
	filter->param_offset_texel = NULL;
	filter->param_glow_fill_behind = NULL;

	filter->reload = true;

	alpha_blur_init(filter->alpha_blur_data);
	return filter;
}

static void glow_filter_destroy(void *data)
{
	glow_filter_data_t *filter = data;

	obs_enter_graphics();

	if (filter->effect_glow) {
		gs_effect_destroy(filter->effect_glow);
	}
	if (filter->input_texrender) {
		gs_texrender_destroy(filter->input_texrender);
	}
	if (filter->output_texrender) {
		gs_texrender_destroy(filter->output_texrender);
	}

	alpha_blur_destroy(filter->alpha_blur_data);

	obs_leave_graphics();
	bfree(filter);
}

static uint32_t glow_filter_width(void *data)
{
	glow_filter_data_t *filter = data;
	return filter->width;
}

static uint32_t glow_filter_height(void *data)
{
	glow_filter_data_t *filter = data;
	return filter->height;
}

static void glow_filter_update(void *data, obs_data_t *settings)
{
	glow_filter_data_t *filter = data;
	filter->glow_size = (float)obs_data_get_double(settings, "glow_size");
	filter->intensity =
		(float)obs_data_get_double(settings, "glow_intensity") / 100.0f;
	filter->ignore_source_border =
		obs_data_get_bool(settings, "ignore_source_border");
	filter->fill = obs_data_get_bool(settings, "fill");

	vec4_from_rgba(&filter->glow_color,
		       (uint32_t)obs_data_get_int(settings, "glow_fill_color"));

	filter->fill_type =
		(uint32_t)obs_data_get_int(settings, "glow_fill_type");

	filter->glow_position =
		(uint32_t)obs_data_get_int(settings, "glow_position");

	if (filter->is_source) {
		const char *glow_source_name =
			obs_data_get_string(settings, "glow_source");
		obs_source_t *glow_source =
			(glow_source_name && strlen(glow_source_name))
				? obs_get_source_by_name(glow_source_name)
				: NULL;
		if (glow_source) {
			obs_weak_source_release(filter->source_input_source);
			filter->source_input_source =
				obs_source_get_weak_source(glow_source);
			filter->width =
				(uint32_t)obs_source_get_width(glow_source);
			filter->height =
				(uint32_t)obs_source_get_height(glow_source);
			obs_source_release(glow_source);
		} else {
			filter->source_input_source = NULL;
			filter->width = (uint32_t)0;
			filter->height = (uint32_t)0;
		}
	} else {
		filter->width = (uint32_t)obs_source_get_width(filter->context);
		filter->height =
			(uint32_t)obs_source_get_height(filter->context);
	}

	if (filter->filter_type == FILTER_TYPE_SHADOW) {
		double offset_distance =
			obs_data_get_double(settings, "glow_offset_distance");

		double offset_angle =
			RAD(obs_data_get_double(settings, "glow_offset_angle"));

		double x_offset = offset_distance * cos(offset_angle);
		double y_offset = offset_distance * sin(offset_angle);

		filter->offset_texel.x = (float)x_offset;
		filter->offset_texel.y = (float)y_offset;
	} else {
		filter->offset_texel.x = 0.0f;
		filter->offset_texel.y = 0.0f;
	}

	const char *fill_source_name =
		obs_data_get_string(settings, "glow_fill_source");
	obs_source_t *fill_source =
		(fill_source_name && strlen(fill_source_name))
			? obs_get_source_by_name(fill_source_name)
			: NULL;
	if (fill_source) {
		obs_weak_source_release(filter->fill_source_source);
		filter->fill_source_source =
			obs_source_get_weak_source(fill_source);
		obs_source_release(fill_source);
	} else {
		filter->fill_source_source = NULL;
	}

	if (filter->reload) {
		filter->reload = false;
		load_effects(filter);
	}
}

static void glow_filter_video_render(void *data, gs_effect_t *effect)
{
	UNUSED_PARAMETER(effect);
	glow_filter_data_t *filter = data;

	if (filter->rendered) {
		draw_output(filter);
		return;
	}

	if (filter->rendering && filter->is_filter) {
		obs_source_skip_video_filter(filter->context);
		return;
	} else if(filter->rendering) {
		return;
	}

	filter->rendering = true;

	// 1. Get the input source as a texture renderer
	//    accessed as filter->input_texrender after call
	get_input_source(filter);

	if (!filter->input_texture_generated) {
		if (filter->is_filter) {
			obs_source_skip_video_filter(filter->context);
		}
		filter->rendering = false;
		return;
	}

	// 2. Apply effect to texture, and render texture to video
	alpha_blur(filter->glow_size, filter->ignore_source_border,
		   filter->alpha_blur_data, filter->input_texrender,
		   filter->alpha_blur_data->alpha_blur_output);

	// 3. Render glow effect to output
	render_glow_filter(filter);

	//filter->output_texrender =
	//	create_or_reset_texrender(filter->output_texrender);
	//gs_texrender_t *tmp = filter->output_texrender;
	//filter->output_texrender = filter->alpha_blur_data->alpha_blur_output;
	//filter->alpha_blur_data->alpha_blur_output = tmp;

	// 4. Draw result (filter->output_texrender) to source
	draw_output(filter);
	filter->rendered = true;
	filter->rendering = false;
}

static obs_properties_t *glow_filter_properties(void *data)
{
	glow_filter_data_t *filter = data;

	obs_properties_t *props = obs_properties_create();
	obs_properties_set_param(props, filter, NULL);

	if (filter->is_source) {
		obs_property_t *stroke_source = obs_properties_add_list(
			props, "glow_source",
			obs_module_text("StrokeSource.Source"),
			OBS_COMBO_TYPE_EDITABLE, OBS_COMBO_FORMAT_STRING);
		obs_property_list_add_string(
			stroke_source, obs_module_text("StrokeCommon.None"),
			"");
		obs_enum_sources(add_source_to_list, stroke_source);
		obs_enum_scenes(add_source_to_list, stroke_source);
	}

	obs_property_t *glow_position_list = obs_properties_add_list(
		props, "glow_position",
		obs_module_text("GlowShadowFilter.Position"),
		OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);

	if (filter->filter_type == FILTER_TYPE_GLOW) {
		obs_property_list_add_int(
			glow_position_list,
			obs_module_text(GLOW_POSITION_OUTER_LABEL),
			GLOW_POSITION_OUTER);
		obs_property_list_add_int(
			glow_position_list,
			obs_module_text(GLOW_POSITION_INNER_LABEL),
			GLOW_POSITION_INNER);
	} else {
		obs_property_list_add_int(
			glow_position_list,
			obs_module_text(SHADOW_POSITION_OUTER_LABEL),
			GLOW_POSITION_OUTER);
		obs_property_list_add_int(
			glow_position_list,
			obs_module_text(SHADOW_POSITION_INNER_LABEL),
			GLOW_POSITION_INNER);
	}

	obs_property_set_modified_callback2(
		glow_position_list, setting_glow_position_modified, data);

	obs_properties_add_bool(
		props, "ignore_source_border",
		obs_module_text("StrokeCommon.IgnoreSourceBorder"));

	obs_properties_add_bool(props, "fill",
				obs_module_text("GlowShadowFilter.FillSource"));

	obs_property_t *prop = obs_properties_add_float_slider(
		props, "glow_size", obs_module_text("GlowShadowFilter.Size"),
		0.0, 100.0, 1.0);
	obs_property_float_set_suffix(prop, "px");

	prop = obs_properties_add_float_slider(
		props, "glow_intensity",
		obs_module_text("GlowShadowFilter.Intensity"), 0.0, 200.0, 0.1);
	obs_property_float_set_suffix(prop, "%");

	if (filter->filter_type == FILTER_TYPE_SHADOW) {
		prop = obs_properties_add_float_slider(
			props, "glow_offset_angle",
			obs_module_text("ShadowFilter.OffsetAngle"), -180.0,
			180.0, 0.1);
		obs_property_float_set_suffix(prop, "°");

		prop = obs_properties_add_float_slider(
			props, "glow_offset_distance",
			obs_module_text("ShadowFilter.OffsetDistance"), 0, 50,
			1.0);
		obs_property_float_set_suffix(prop, "px");
	}

	obs_property_t *glow_fill_method_list = obs_properties_add_list(
		props, "glow_fill_type",
		obs_module_text("StrokeFilter.FillType"), OBS_COMBO_TYPE_LIST,
		OBS_COMBO_FORMAT_INT);

	obs_property_list_add_int(glow_fill_method_list,
				  obs_module_text(GLOW_FILL_TYPE_COLOR_LABEL),
				  GLOW_FILL_TYPE_COLOR);
	obs_property_list_add_int(glow_fill_method_list,
				  obs_module_text(GLOW_FILL_TYPE_SOURCE_LABEL),
				  GLOW_FILL_TYPE_SOURCE);
	//obs_property_list_add_int(
	//	glow_fill_method_list,
	//	obs_module_text(GLOW_FILL_TYPE_IMAGE_LABEL),
	//	GLOW_FILL_TYPE_IMAGE);

	obs_property_set_modified_callback(glow_fill_method_list,
					   setting_fill_type_modified);

	obs_properties_add_color_alpha(
		props, "glow_fill_color",
		obs_module_text("StrokeFilter.ColorFill"));

	obs_properties_add_path(
		props, "glow_fill_image",
		obs_module_text("StrokeFilter.ImageFill"), OBS_PATH_FILE,
		"Textures (*.bmp *.tga *.png *.jpeg *.jpg *.gif);;", NULL);

	obs_property_t *glow_fill_source_source = obs_properties_add_list(
		props, "glow_fill_source",
		obs_module_text("StrokeFilter.SourceFill"),
		OBS_COMBO_TYPE_EDITABLE, OBS_COMBO_FORMAT_STRING);
	obs_property_list_add_string(glow_fill_source_source, "None", "");
	obs_enum_sources(add_source_to_list, glow_fill_source_source);
	obs_enum_scenes(add_source_to_list, glow_fill_source_source);

	obs_properties_add_text(props, "plugin_info", PLUGIN_INFO,
				OBS_TEXT_INFO);

	return props;
}

static void glow_filter_video_tick(void *data, float seconds)
{
	UNUSED_PARAMETER(seconds);
	glow_filter_data_t *filter = data;
	if (filter->is_filter) {
		obs_source_t *target = obs_filter_get_target(filter->context);
		if (!target) {
			return;
		}
		filter->width = (uint32_t)obs_source_get_base_width(target);
		filter->height = (uint32_t)obs_source_get_base_height(target);
	}
	filter->rendered = false;
}

static void glow_filter_defaults(obs_data_t *settings)
{
	obs_data_set_default_double(settings, "glow_size", 4.0);
	obs_data_set_default_double(settings, "glow_intensity", 100.0);
	obs_data_set_default_int(settings, "glow_fill_type",
				 GLOW_FILL_TYPE_COLOR);
	obs_data_set_default_int(settings, "glow_fill_color",
				 DEFAULT_COLOR_GLOW);
	obs_data_set_default_int(settings, "glow_position",
				 GLOW_POSITION_OUTER);
	obs_data_set_default_double(settings, "glow_offset_angle", 0.0);
	obs_data_set_default_double(settings, "glow_offset_distance", 0.0);
}

static void shadow_filter_defaults(obs_data_t *settings)
{
	obs_data_set_default_double(settings, "glow_size", 4.0);
	obs_data_set_default_double(settings, "glow_intensity", 100.0);
	obs_data_set_default_int(settings, "glow_fill_type",
				 GLOW_FILL_TYPE_COLOR);
	obs_data_set_default_int(settings, "glow_fill_color",
				 DEFAULT_COLOR_SHADOW);
	obs_data_set_default_int(settings, "glow_position",
				 GLOW_POSITION_OUTER);
	obs_data_set_default_double(settings, "glow_offset_angle", 45.0);
	obs_data_set_default_double(settings, "glow_offset_distance", 10.0);
	obs_data_set_default_bool(settings, "ignore_source_border", true);
}

static void get_input_source(glow_filter_data_t *filter)
{
	obs_source_t *input_source = filter->context;
	if (filter->is_source) {
		input_source = filter->source_input_source
				       ? obs_weak_source_get_source(
						 filter->source_input_source)
				       : NULL;
		if (!input_source) {
			filter->input_texture_generated = false;
			return;
		}
	}

	const enum gs_color_space preferred_spaces[] = {
		GS_CS_SRGB,
		GS_CS_SRGB_16F,
		GS_CS_709_EXTENDED,
	};
	const enum gs_color_space space = obs_source_get_color_space(
		input_source, OBS_COUNTOF(preferred_spaces), preferred_spaces);

	// Set up a tex renderer for source
	filter->input_texrender =
		create_or_reset_texrender(filter->input_texrender);
	uint32_t base_width = obs_source_get_width(input_source);
	uint32_t base_height = obs_source_get_height(input_source);
	filter->width = base_width;
	filter->height = base_height;
	gs_blend_state_push();
	gs_blend_function(GS_BLEND_ONE, GS_BLEND_ZERO);
	if (gs_texrender_begin_with_color_space(
		    filter->input_texrender, base_width, base_height, space)) {
		const float w = (float)base_width;
		const float h = (float)base_height;
		struct vec4 clear_color;

		vec4_zero(&clear_color);
		gs_clear(GS_CLEAR_COLOR, &clear_color, 0.0f, 0);
		gs_ortho(0.0f, w, 0.0f, h, -100.0f, 100.0f);

		obs_source_video_render(input_source);
		gs_texrender_end(filter->input_texrender);
		filter->input_texture_generated = w > 0 && h > 0;
	} else {
		filter->input_texture_generated = false;
	}
	gs_blend_state_pop();
	if (filter->is_source) {
		obs_source_release(input_source);
	}
}

static void draw_output(glow_filter_data_t *filter)
{
	gs_texture_t *texture =
		gs_texrender_get_texture(filter->output_texrender);
	gs_effect_t *pass_through = obs_get_base_effect(OBS_EFFECT_DEFAULT);
	gs_eparam_t *param = gs_effect_get_param_by_name(pass_through, "image");
	gs_effect_set_texture(param, texture);
	uint32_t width = gs_texture_get_width(texture);
	uint32_t height = gs_texture_get_height(texture);
	while (gs_effect_loop(pass_through, "Draw")) {
		gs_draw_sprite(texture, 0, width, height);
	}
}

static void load_effects(glow_filter_data_t *filter)
{
	load_1d_alpha_blur_effect(filter->alpha_blur_data);
	load_glow_effect(filter);
}

static bool setting_fill_type_modified(obs_properties_t *props,
				       obs_property_t *p, obs_data_t *settings)
{
	UNUSED_PARAMETER(p);
	int fill_type = (int)obs_data_get_int(settings, "glow_fill_type");
	switch (fill_type) {
	case GLOW_FILL_TYPE_COLOR:
		setting_visibility("glow_fill_color", true, props);
		setting_visibility("glow_fill_source", false, props);
		setting_visibility("glow_fill_image", false, props);
		break;
	case GLOW_FILL_TYPE_SOURCE:
		setting_visibility("glow_fill_color", false, props);
		setting_visibility("glow_fill_source", true, props);
		setting_visibility("glow_fill_image", false, props);
		break;
	case GLOW_FILL_TYPE_IMAGE:
		setting_visibility("glow_fill_color", false, props);
		setting_visibility("glow_fill_source", false, props);
		setting_visibility("glow_fill_image", true, props);
		break;
	}
	return true;
}

static bool setting_glow_position_modified(void *data, obs_properties_t *props,
					   obs_property_t *p,
					   obs_data_t *settings)
{
	UNUSED_PARAMETER(p);
	glow_filter_data_t *filter = data;

	int position = (int)obs_data_get_int(settings, "glow_position");
	switch (position) {
	case GLOW_POSITION_INNER:
		setting_visibility("ignore_source_border", true, props);
		setting_visibility("fill", false, props);
		break;
	case GLOW_POSITION_OUTER:
		setting_visibility("ignore_source_border", false, props);
		setting_visibility("fill", filter->is_source, props);
		break;
	default:
		break;
	}
	return true;
}
