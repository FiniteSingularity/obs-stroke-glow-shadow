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
	blog(LOG_INFO, "======  GLOW FILTER CREATE =======");
	glow_filter_data_t *filter = filter_create(source);
	filter->filter_type = FILTER_TYPE_GLOW;
	obs_source_update(source, settings);
	return filter;
}

static void *shadow_filter_create(obs_data_t *settings, obs_source_t *source)
{
	blog(LOG_INFO, "======  SHADOW FILTER CREATE =======");
	glow_filter_data_t *filter = filter_create(source);
	filter->filter_type = FILTER_TYPE_SHADOW;
	obs_source_update(source, settings);
	return filter;
}

static glow_filter_data_t *filter_create(obs_source_t *source)
{
	glow_filter_data_t *filter = bzalloc(sizeof(glow_filter_data_t));

	filter->alpha_blur_data = bzalloc(sizeof(alpha_blur_data_t));

	filter->context = source;
	filter->param_glow_texel_step = NULL;
	filter->param_glow_image = NULL;
	filter->param_glow_mask = NULL;
	filter->param_glow_fill_source = NULL;
	filter->param_glow_fill_color = NULL;
	filter->param_glow_texel_step = NULL;
	filter->param_glow_intensity = NULL;
	filter->param_offset_texel = NULL;

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
	blog(LOG_INFO, "============  GLOW FILTER UPDATE, %i",
	     filter->filter_type);
	filter->glow_size = (float)obs_data_get_double(settings, "glow_size");
	filter->intensity =
		(float)obs_data_get_double(settings, "glow_intensity") / 100.0f;
	vec4_from_rgba(&filter->glow_color,
		       (uint32_t)obs_data_get_int(settings, "glow_fill_color"));
	blog(LOG_INFO, "     %f", filter->intensity);
	filter->fill_type =
		(uint32_t)obs_data_get_int(settings, "glow_fill_type");

	filter->glow_position =
		(uint32_t)obs_data_get_int(settings, "glow_position");

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
		blog(LOG_INFO, "    SKIPPING RENDER");
		draw_output_to_source(filter);
		return;
	}

	if (filter->rendering) {
		blog(LOG_INFO, "    SKIPPING RENDER- STILL RENDERING");
		obs_source_skip_video_filter(filter->context);
		return;
	}
	//blog(LOG_INFO, "======= START RENDER PASS ===========");

	filter->rendering = true;

	// 1. Get the input source as a texture renderer
	//    accessed as filter->input_texrender after call
	get_input_source(filter);

	// 2. Apply effect to texture, and render texture to video
	alpha_blur(filter->glow_size, filter->alpha_blur_data,
		   filter->input_texrender,
		   filter->alpha_blur_data->alpha_blur_output);

	// 3. Render glow effect to output
	render_glow_filter(filter);

	//filter->output_texrender =
	//	create_or_reset_texrender(filter->output_texrender);
	//gs_texrender_t *tmp = filter->output_texrender;
	//filter->output_texrender = filter->alpha_blur_data->alpha_blur_output;
	//filter->alpha_blur_data->alpha_blur_output = tmp;

	// 4. Draw result (filter->output_texrender) to source
	draw_output_to_source(filter);
	filter->rendered = true;
	filter->rendering = false;
}

static obs_properties_t *glow_filter_properties(void *data)
{
	glow_filter_data_t *filter = data;

	obs_properties_t *props = obs_properties_create();
	obs_properties_set_param(props, filter, NULL);

	obs_property_t *stroke_position_list = obs_properties_add_list(
		props, "glow_position",
		obs_module_text("GlowShadowFilter.Position"),
		OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);

	if (filter->filter_type == FILTER_TYPE_GLOW) {
		obs_property_list_add_int(
			stroke_position_list,
			obs_module_text(GLOW_POSITION_OUTER_LABEL),
			GLOW_POSITION_OUTER);
		obs_property_list_add_int(
			stroke_position_list,
			obs_module_text(GLOW_POSITION_INNER_LABEL),
			GLOW_POSITION_INNER);
	} else {
		obs_property_list_add_int(
			stroke_position_list,
			obs_module_text(SHADOW_POSITION_OUTER_LABEL),
			GLOW_POSITION_OUTER);
		obs_property_list_add_int(
			stroke_position_list,
			obs_module_text(SHADOW_POSITION_INNER_LABEL),
			GLOW_POSITION_INNER);
	}

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
	obs_source_t *target = obs_filter_get_target(filter->context);
	if (!target) {
		return;
	}
	filter->width = (uint32_t)obs_source_get_base_width(target);
	filter->height = (uint32_t)obs_source_get_base_height(target);
	//filter->uv_size.x = (float)filter->width;
	//filter->uv_size.y = (float)filter->height;
	//blog(LOG_INFO, "VIDEO TICK!!!!");
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
}

static void get_input_source(glow_filter_data_t *filter)
{
	gs_effect_t *pass_through = obs_get_base_effect(OBS_EFFECT_DEFAULT);

	filter->input_texrender =
		create_or_reset_texrender(filter->input_texrender);
	if (obs_source_process_filter_begin(filter->context, GS_RGBA,
					    OBS_ALLOW_DIRECT_RENDERING) &&
	    gs_texrender_begin(filter->input_texrender, filter->width,
			       filter->height)) {

		set_blending_parameters();

		gs_ortho(0.0f, (float)filter->width, 0.0f,
			 (float)filter->height, -100.0f, 100.0f);

		obs_source_process_filter_end(filter->context, pass_through,
					      filter->width, filter->height);
		gs_texrender_end(filter->input_texrender);
		gs_blend_state_pop();
	}
}

static void draw_output_to_source(glow_filter_data_t *filter)
{
	gs_texture_t *texture =
		gs_texrender_get_texture(filter->output_texrender);

	gs_effect_t *pass_through = obs_get_base_effect(OBS_EFFECT_DEFAULT);
	gs_eparam_t *param = gs_effect_get_param_by_name(pass_through, "image");
	gs_effect_set_texture(param, texture);

	while (gs_effect_loop(pass_through, "Draw")) {
		gs_draw_sprite(texture, 0, filter->width, filter->height);
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
