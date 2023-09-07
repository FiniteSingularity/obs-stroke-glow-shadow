#include "obs-stroke-filter.h"
#include "obs-stroke.h"
#include "blur/alpha-blur.h"
#include "stroke.h"

struct obs_source_info obs_stroke_filter = {
	.id = "obs_stroke_filter",
	.type = OBS_SOURCE_TYPE_FILTER,
	.output_flags = OBS_SOURCE_VIDEO,
	.get_name = stroke_filter_name,
	.create = stroke_filter_create,
	.destroy = stroke_filter_destroy,
	.update = stroke_filter_update,
	.video_render = stroke_filter_video_render,
	.video_tick = stroke_filter_video_tick,
	.get_width = stroke_filter_width,
	.get_height = stroke_filter_height,
	.get_properties = stroke_filter_properties,
	.get_defaults = stroke_filter_defaults};

static const char *stroke_filter_name(void *unused)
{
	UNUSED_PARAMETER(unused);
	return obs_module_text("StrokeFilter");
}

static void *stroke_filter_create(obs_data_t *settings, obs_source_t *source)
{
	blog(LOG_INFO, "======  STROKE FILTER CREATE =======");
	stroke_filter_data_t *filter = bzalloc(sizeof(stroke_filter_data_t));

	filter->context = source;
	filter->param_blur_radius = NULL;
	filter->param_blur_texel_step = NULL;

	filter->reload = true;

	obs_source_update(source, settings);
	return filter;
}

static void stroke_filter_destroy(void *data)
{
	stroke_filter_data_t *filter = data;

	obs_enter_graphics();
	if (filter->effect_alpha_blur) {
		gs_effect_destroy(filter->effect_alpha_blur);
	}
	if (filter->effect_stroke) {
		gs_effect_destroy(filter->effect_stroke);
	}
	if (filter->alpha_blur_pass_1) {
		gs_texrender_destroy(filter->alpha_blur_pass_1);
	}
	if (filter->alpha_blur_output) {
		gs_texrender_destroy(filter->alpha_blur_output);
	}

	if (filter->input_texrender) {
		gs_texrender_destroy(filter->input_texrender);
	}
	if (filter->output_texrender) {
		gs_texrender_destroy(filter->output_texrender);
	}

	obs_leave_graphics();
	bfree(filter);
}

static uint32_t stroke_filter_width(void *data)
{
	stroke_filter_data_t *filter = data;
	return filter->width;
}

static uint32_t stroke_filter_height(void *data)
{
	stroke_filter_data_t *filter = data;
	return filter->height;
}

static void stroke_filter_update(void *data, obs_data_t *settings)
{
	stroke_filter_data_t *filter = data;
	filter->stroke_size =
		(float)obs_data_get_double(settings, "stroke_size");

	vec4_from_rgba(&filter->stroke_color,
		       (uint32_t)obs_data_get_int(settings, "stroke_color"));

	if (filter->reload) {
		filter->reload = false;
		load_effects(filter);
	}
}

static void stroke_filter_video_render(void *data, gs_effect_t *effect)
{
	UNUSED_PARAMETER(effect);
	stroke_filter_data_t *filter = data;

	if (filter->rendered) {
		//draw_output_to_source(filter);
		return;
	}

	if (filter->rendering) {
		obs_source_skip_video_filter(filter->context);
		return;
	}

	filter->rendering = true;

	// 1. Get the input source as a texture renderer
	//    accessed as filter->input_texrender after call
	get_input_source(filter);

	// 2. Apply effect to texture, and render texture to video
	alpha_blur(filter);

	// 3. Apply stroke to texture
	render_stroke_filter(filter);

	// 3. Draw result (filter->output_texrender) to source
	draw_output_to_source(filter);
	filter->rendered = true;

	filter->rendering = false;
}

static obs_properties_t *stroke_filter_properties(void *data)
{
	stroke_filter_data_t *filter = data;

	obs_properties_t *props = obs_properties_create();
	obs_properties_set_param(props, filter, NULL);

	obs_properties_add_float_slider(
		props, "stroke_size",
		obs_module_text("StrokeFilter.StrokeSize"), 0.0, 100.0, 1.0);

	obs_properties_add_color_alpha(
		props, "stroke_color",
		obs_module_text("StrokeFilter.StrokeColor"));

	obs_property_t *stroke_fill_method_list = obs_properties_add_list(
		props, "stroke_fill_method",
		obs_module_text("StrokeFilter.StrokeFill"),
		OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);

	obs_property_list_add_int(stroke_fill_method_list,
				  obs_module_text(STROKE_FILL_TYPE_COLOR_LABEL),
				  STROKE_FILL_TYPE_COLOR);
	obs_property_list_add_int(stroke_fill_method_list,
				  obs_module_text(STROKE_FILL_TYPE_SOURCE_LABEL),
				  STROKE_FILL_TYPE_SOURCE);

	return props;
}

static void stroke_filter_video_tick(void *data, float seconds)
{
	UNUSED_PARAMETER(seconds);
	stroke_filter_data_t *filter = data;
	obs_source_t *target = obs_filter_get_target(filter->context);
	if (!target) {
		return;
	}
	filter->width = (uint32_t)obs_source_get_base_width(target);
	filter->height = (uint32_t)obs_source_get_base_height(target);
	//filter->uv_size.x = (float)filter->width;
	//filter->uv_size.y = (float)filter->height;
	filter->rendered = false;
}

static void stroke_filter_defaults(obs_data_t *settings)
{
	// Example default set.
	obs_data_set_default_double(settings, "stroke_size", 4.0);
	obs_data_set_default_int(settings, "stroke_fill_method", STROKE_FILL_TYPE_COLOR);
}

static void get_input_source(stroke_filter_data_t *filter)
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

static void draw_output_to_source(stroke_filter_data_t *filter)
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

static void load_effects(stroke_filter_data_t *filter)
{
	load_1d_alpha_blur_effect(filter);
	load_stroke_effect(filter);
}
