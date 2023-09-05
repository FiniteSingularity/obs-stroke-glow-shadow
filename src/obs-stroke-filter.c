#include "obs-stroke-filter.h"
#include "obs-stroke.h"


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
	stroke_filter_data_t *filter = bzalloc(sizeof(stroke_filter_data_t));

	return filter;
}

static void stroke_filter_destroy(void *data)
{
	stroke_filter_data_t *filter = data;

	obs_enter_graphics();
	if (filter->effect) {
		gs_effect_destroy(filter->effect);
	}
	if (filter->effect_2) {
		gs_effect_destroy(filter->effect_2);
	}
	if (filter->composite_effect) {
		gs_effect_destroy(filter->composite_effect);
	}
	if (filter->mix_effect) {
		gs_effect_destroy(filter->mix_effect);
	}
	if (filter->effect_mask_effect) {
		gs_effect_destroy(filter->effect_mask_effect);
	}
	if (filter->render) {
		gs_texrender_destroy(filter->render);
	}
	if (filter->render2) {
		gs_texrender_destroy(filter->render2);
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

	if (true) {
		filter->rendered = true;
	}

	filter->rendering = false;
}

static obs_properties_t *stroke_filter_properties(void *data)
{
	stroke_filter_data_t *filter = data;

	obs_properties_t *props = obs_properties_create();
	obs_properties_set_param(props, filter, NULL);

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
	// obs_data_set_default_double(settings, "radius", 10.0);
}
