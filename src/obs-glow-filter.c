#include "obs-glow-filter.h"
#include "obs-glow.h"
#include "blur/alpha-blur.h"
#include "blur/dual-kawase.h"
#include "glow.h"

#include <math.h>

struct obs_source_info obs_glow_filter = {
	.id = "obs_glow_filter",
	.type = OBS_SOURCE_TYPE_FILTER,
	.output_flags = OBS_SOURCE_VIDEO | OBS_SOURCE_SRGB,
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
	.output_flags = OBS_SOURCE_VIDEO | OBS_SOURCE_CUSTOM_DRAW | OBS_SOURCE_SRGB,
	.get_name = glow_filter_name,
	.create = glow_filter_create,
	.destroy = glow_filter_destroy,
	.update = glow_filter_update,
	.video_render = glow_filter_video_render,
	.video_tick = glow_filter_video_tick,
	.get_width = glow_filter_width,
	.get_height = glow_filter_height,
	.get_properties = glow_source_properties,
	.get_defaults = glow_filter_defaults,
	.icon_type = OBS_ICON_TYPE_COLOR};

struct obs_source_info obs_shadow_filter = {
	.id = "obs_shadow_filter",
	.type = OBS_SOURCE_TYPE_FILTER,
	.output_flags = OBS_SOURCE_VIDEO | OBS_SOURCE_SRGB,
	.get_name = shadow_filter_name,
	.create = shadow_filter_create,
	.destroy = glow_filter_destroy,
	.update = glow_filter_update,
	.video_render = glow_filter_video_render,
	.video_tick = glow_filter_video_tick,
	.get_width = glow_filter_width,
	.get_height = glow_filter_height,
	.get_properties = shadow_filter_properties,
	.get_defaults = shadow_filter_defaults};

struct obs_source_info obs_shadow_source = {
	.id = "obs_shadow_source",
	.type = OBS_SOURCE_TYPE_INPUT,
	.output_flags = OBS_SOURCE_VIDEO | OBS_SOURCE_CUSTOM_DRAW | OBS_SOURCE_SRGB,
	.get_name = shadow_filter_name,
	.create = shadow_filter_create,
	.destroy = glow_filter_destroy,
	.update = glow_filter_update,
	.video_render = glow_filter_video_render,
	.video_tick = glow_filter_video_tick,
	.get_width = glow_filter_width,
	.get_height = glow_filter_height,
	.get_properties = shadow_source_properties,
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
	filter->param_threshold = NULL;

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
	if (filter->effect_output) {
		gs_effect_destroy(filter->effect_output);
	}
	if (filter->input_texrender) {
		gs_texrender_destroy(filter->input_texrender);
	}
	if (filter->output_texrender) {
		gs_texrender_destroy(filter->output_texrender);
	}
	if (filter->alpha_mask_texrender) {
		gs_texrender_destroy(filter->alpha_mask_texrender);
	}

	if (filter->source_input_source) {
		obs_weak_source_release(filter->source_input_source);
	}

	if (filter->fill_source_source) {
		obs_weak_source_release(filter->fill_source_source);
	}

	alpha_blur_destroy(filter->alpha_blur_data);

	obs_leave_graphics();
	bfree(filter->alpha_blur_data);
	bfree(filter);
}

static uint32_t glow_filter_width(void *data)
{
	glow_filter_data_t *filter = data;
	if (filter->is_source) {
		return filter->width;
	}
	return filter->source_width + filter->pad_l + filter->pad_r;
	//return filter->width;
}

static uint32_t glow_filter_height(void *data)
{
	glow_filter_data_t *filter = data;
	if (filter->is_source) {
		return filter->height;
	}
	return filter->source_height + filter->pad_t + filter->pad_b;
	//return filter->height;
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

	filter->blur_type = (enum blur_type)obs_data_get_int(settings, "blur_type");

	vec4_from_rgba(&filter->glow_color,
		(uint32_t)obs_data_get_int(settings, "glow_fill_color"));

	vec4_from_rgba_srgb(&filter->glow_color_srgb,
		       (uint32_t)obs_data_get_int(settings, "glow_fill_color"));

	filter->fill_type =
		(enum glow_fill_type)obs_data_get_int(settings, "glow_fill_type");

	filter->glow_position =
		(enum glow_position)obs_data_get_int(settings, "glow_position");

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

	filter->padding_amount = obs_data_get_int(settings, "glow_padding") == PADDING_MANUAL ? (uint32_t)obs_data_get_int(settings, "padding_amount") : 0;

	if (obs_data_get_int(settings, "glow_padding") == PADDING_AUTO && filter->glow_position == GLOW_POSITION_OUTER) {
		double offset = filter->filter_type == FILTER_TYPE_SHADOW ? obs_data_get_double(settings, "glow_offset_distance") : 0.0;
		filter->padding_amount = (uint32_t)(filter->glow_size * 3.5 + offset);
	}

	filter->threshold = filter->glow_position == GLOW_POSITION_OUTER
		? (float)obs_data_get_double(settings, "threshold_outer")/100.0f
		: (float)obs_data_get_double(settings, "threshold_inner")/100.0f;

	if (filter->glow_position == GLOW_POSITION_OUTER) {
		filter->pad_b = filter->padding_amount;
		filter->pad_t = filter->padding_amount;
		filter->pad_l = filter->padding_amount;
		filter->pad_r = filter->padding_amount;
	} else {
		filter->pad_b = 0;
		filter->pad_t = 0;
		filter->pad_l = 0;
		filter->pad_r = 0;
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
		//draw_output(filter);
		glow_render_cropped_output(filter);
		return;
	}

	bool skipRender = filter->rendering || (filter->filter_type == FILTER_TYPE_GLOW && filter->glow_size <= 0.01 && !filter->fill);
	if (skipRender && filter->is_filter) {
		obs_source_skip_video_filter(filter->context);
		return;
	}
	else if (skipRender) {
		return;
	}

	filter->rendering = true;

	// 1. Get the input source as a texture renderer
	//    accessed as filter->input_texrender after call
	//get_input_source(filter);
	glow_render_padded_input(filter);

	if (!filter->input_texture_generated) {
		if (filter->is_filter) {
			obs_source_skip_video_filter(filter->context);
		}
		filter->rendering = false;
		return;
	}

	//gs_texrender_t* tmp = filter->input_texrender;
	//filter->input_texrender = filter->output_texrender;
	//filter->output_texrender = tmp;

	// 2. Generate the alpha mask to be blurred based on user
	//    threshold input.
	render_glow_alpha_mask(filter);

	// 3. Apply effect to texture, and render texture to video
	if (filter->blur_type == BLUR_TYPE_TRIANGULAR) {
		alpha_blur(filter->glow_size, filter->ignore_source_border,
			   filter->alpha_blur_data, filter->alpha_mask_texrender,
			   filter->alpha_blur_data->alpha_blur_output);
	} else {
		dual_kawase_blur((int)filter->glow_size,
				 filter->ignore_source_border,
				 filter->alpha_blur_data,
				 filter->alpha_mask_texrender);
	}

	// 4. Render glow effect to output
	render_glow_filter(filter);

	// 5. Draw result (filter->output_texrender) to source
	//draw_output(filter);
	glow_render_cropped_output(filter);

	filter->rendered = true;
	filter->rendering = false;
}

static obs_properties_t *properties(void *data, bool is_source, enum filter_type filter_type)
{
	glow_filter_data_t *filter = data;

	obs_properties_t *props = obs_properties_create();
	obs_properties_set_param(props, filter, NULL);

	if (is_source) {
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

	if (filter_type == FILTER_TYPE_GLOW) {
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

	if (is_source) {
		obs_properties_add_bool(props, "fill",
			obs_module_text("GlowShadowFilter.FillSource"));
	}

	obs_property_t* glow_padding_list = obs_properties_add_list(
		props, "glow_padding",
		obs_module_text("StrokeFilter.Padding"),
		OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);

	obs_property_list_add_int(glow_padding_list,
		obs_module_text(PADDING_NONE_LABEL),
		PADDING_NONE);

	obs_property_list_add_int(glow_padding_list,
		obs_module_text(PADDING_AUTO_LABEL),
		PADDING_AUTO);

	obs_property_list_add_int(glow_padding_list,
		obs_module_text(PADDING_MANUAL_LABEL),
		PADDING_MANUAL);

	obs_property_set_modified_callback(
		glow_padding_list, setting_glow_padding_modified);

	obs_property_t* padding_amt = obs_properties_add_int_slider(
		props, "padding_amount",
		obs_module_text("StrokeFilter.Padding.Amount"), 0, 4000, 1
	);

	obs_property_t *blur_type_list = obs_properties_add_list(
		props, "blur_type",
		obs_module_text("GlowShadowFilter.BlurType"),
		OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);

	obs_property_list_add_int(blur_type_list,
				  obs_module_text(BLUR_TYPE_TRIANGULAR_LABEL),
				  BLUR_TYPE_TRIANGULAR);
	obs_property_list_add_int(blur_type_list,
				  obs_module_text(BLUR_TYPE_DUAL_KAWASE_LABEL),
				  BLUR_TYPE_DUAL_KAWASE);

	obs_property_t* p = obs_properties_add_float_slider(
		props, "threshold_inner",
		obs_module_text("StrokeFilter.MaskThreshold"), 0.0, 100.0, 0.01
	);
	obs_property_float_set_suffix(p, "%");

	p = obs_properties_add_float_slider(
		props, "threshold_outer",
		obs_module_text("StrokeFilter.MaskThreshold"), 0.0, 100.0, 0.01
	);
	obs_property_float_set_suffix(p, "%");

	obs_property_t *prop = obs_properties_add_float_slider(
		props, "glow_size", obs_module_text("GlowShadowFilter.Size"),
		0.0, 100.0, 1.0);
	obs_property_float_set_suffix(prop, "px");

	prop = obs_properties_add_float_slider(
		props, "glow_intensity",
		obs_module_text("GlowShadowFilter.Intensity"), 0.0, 400.0, 0.1);
	obs_property_float_set_suffix(prop, "%");

	if (filter_type == FILTER_TYPE_SHADOW) {
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
	obs_enum_sources(add_source_to_list, glow_fill_source_source);
	obs_enum_scenes(add_source_to_list, glow_fill_source_source);
	obs_property_list_insert_string(glow_fill_source_source, 0, "None", "");

	obs_properties_add_text(props, "plugin_info", PLUGIN_INFO,
				OBS_TEXT_INFO);

	return props;
}

static obs_properties_t *glow_filter_properties(void *data)
{
	return properties(data, false, FILTER_TYPE_GLOW);
}

static obs_properties_t *glow_source_properties(void *data)
{
	return properties(data, true, FILTER_TYPE_GLOW);
}

static obs_properties_t *shadow_filter_properties(void *data)
{
	return properties(data, false, FILTER_TYPE_SHADOW);
}

static obs_properties_t *shadow_source_properties(void *data)
{
	return properties(data, true, FILTER_TYPE_SHADOW);
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
		filter->source_width = (uint32_t)obs_source_get_base_width(target);
		filter->source_height = (uint32_t)obs_source_get_base_height(target);
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
	obs_data_set_default_double(settings, "threshold_outer", 100.0);
	obs_data_set_default_double(settings, "threshold_inner", 0.0);
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
	obs_data_set_default_int(settings, "blur_type", BLUR_TYPE_TRIANGULAR);
	obs_data_set_default_double(settings, "glow_offset_angle", 45.0);
	obs_data_set_default_double(settings, "glow_offset_distance", 10.0);
	obs_data_set_default_bool(settings, "ignore_source_border", true);
	obs_data_set_default_double(settings, "threshold_outer", 100.0);
	obs_data_set_default_double(settings, "threshold_inner", 0.0);
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

		const enum gs_color_space preferred_spaces[] = {
			GS_CS_SRGB,
			GS_CS_SRGB_16F,
			GS_CS_709_EXTENDED,
		};
		const enum gs_color_space space = obs_source_get_color_space(
			input_source, OBS_COUNTOF(preferred_spaces),
			preferred_spaces);

		// Set up a tex renderer for source
		filter->input_texrender =
			create_or_reset_texrender(filter->input_texrender);
		uint32_t base_width = obs_source_get_width(input_source);
		uint32_t base_height = obs_source_get_height(input_source);
		filter->width = base_width;
		filter->height = base_height;
		gs_blend_state_push();
		gs_blend_function(GS_BLEND_ONE, GS_BLEND_ZERO);
		if (gs_texrender_begin_with_color_space(filter->input_texrender,
							base_width, base_height,
							space)) {
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

		obs_source_release(input_source);
	} else {
		// Use the OBS default effect file as our effect.
		gs_effect_t *pass_through =
			obs_get_base_effect(OBS_EFFECT_DEFAULT);

		// Set up our color space info.
		const enum gs_color_space preferred_spaces[] = {
			GS_CS_SRGB,
			GS_CS_SRGB_16F,
			GS_CS_709_EXTENDED,
		};

		const enum gs_color_space source_space =
			obs_source_get_color_space(
				obs_filter_get_target(input_source),
				OBS_COUNTOF(preferred_spaces),
				preferred_spaces);

		const enum gs_color_format format =
			gs_get_format_from_space(source_space);

		// Set up our input_texrender to catch the output texture.
		filter->input_texrender =
			create_or_reset_texrender(filter->input_texrender);
		filter->input_texture_generated = false;

		// Start the rendering process with our correct color space params,
		// And set up your texrender to recieve the created texture.
		if (!obs_source_process_filter_begin_with_color_space(
			    input_source, format, source_space,
			    OBS_NO_DIRECT_RENDERING))
			return;

		uint32_t base_width = obs_source_get_width(input_source);
		uint32_t base_height = obs_source_get_height(input_source);
		filter->width = base_width;
		filter->height = base_height;

		if (gs_texrender_begin(filter->input_texrender, filter->width,
				       filter->height)) {

			gs_blend_state_push();
			gs_reset_blend_state();
			gs_enable_blending(false);
			gs_blend_function(GS_BLEND_ONE, GS_BLEND_ZERO);

			gs_ortho(0.0f, (float)filter->width, 0.0f,
				 (float)filter->height, -100.0f, 100.0f);
			const char *technique = "DrawAlphaDivide";
			obs_source_process_filter_tech_end(
				input_source, pass_through, filter->width,
				filter->height, technique);
			gs_texrender_end(filter->input_texrender);
			gs_blend_state_pop();
			filter->input_texture_generated = true;
		}
	}
}

static void draw_output(glow_filter_data_t *filter)
{
	if (filter->is_source) {
		gs_texture_t *texture =
			gs_texrender_get_texture(filter->output_texrender);
		gs_effect_t *pass_through =
			obs_get_base_effect(OBS_EFFECT_DEFAULT);
		gs_eparam_t *param =
			gs_effect_get_param_by_name(pass_through, "image");
		gs_effect_set_texture(param, texture);
		uint32_t width = gs_texture_get_width(texture);
		uint32_t height = gs_texture_get_height(texture);
		while (gs_effect_loop(pass_through, "Draw")) {
			gs_draw_sprite(texture, 0, width, height);
		}
	} else {
		const enum gs_color_space preferred_spaces[] = {
			GS_CS_SRGB,
			GS_CS_SRGB_16F,
			GS_CS_709_EXTENDED,
		};

		const enum gs_color_space source_space =
			obs_source_get_color_space(
				obs_filter_get_target(filter->context),
				OBS_COUNTOF(preferred_spaces),
				preferred_spaces);

		const enum gs_color_format format =
			gs_get_format_from_space(source_space);

		if (!obs_source_process_filter_begin_with_color_space(
			    filter->context, format, source_space,
			    OBS_NO_DIRECT_RENDERING)) {
			return;
		}

		gs_texture_t *texture =
			gs_texrender_get_texture(filter->output_texrender);
		gs_effect_t *pass_through = filter->effect_output;

		if (filter->param_output_image) {
			gs_effect_set_texture(filter->param_output_image,
					      texture);
		}

		obs_source_process_filter_end(filter->context, pass_through,
					      filter->width, filter->height);
	}
}

//static void get_input_source(glow_filter_data_t *filter)
//{
//	obs_source_t *input_source = filter->context;
//	if (filter->is_source) {
//		input_source = filter->source_input_source
//				       ? obs_weak_source_get_source(
//						 filter->source_input_source)
//				       : NULL;
//		if (!input_source) {
//			filter->input_texture_generated = false;
//			return;
//		}
//	}
//
//	const enum gs_color_space preferred_spaces[] = {
//		GS_CS_SRGB,
//		GS_CS_SRGB_16F,
//		GS_CS_709_EXTENDED,
//	};
//	const enum gs_color_space space = obs_source_get_color_space(
//		input_source, OBS_COUNTOF(preferred_spaces), preferred_spaces);
//
//	// Set up a tex renderer for source
//	filter->input_texrender =
//		create_or_reset_texrender(filter->input_texrender);
//	uint32_t base_width = obs_source_get_width(input_source);
//	uint32_t base_height = obs_source_get_height(input_source);
//	filter->width = base_width;
//	filter->height = base_height;
//	gs_blend_state_push();
//	gs_blend_function(GS_BLEND_ONE, GS_BLEND_ZERO);
//	if (gs_texrender_begin_with_color_space(
//		    filter->input_texrender, base_width, base_height, space)) {
//		const float w = (float)base_width;
//		const float h = (float)base_height;
//		struct vec4 clear_color;
//
//		vec4_zero(&clear_color);
//		gs_clear(GS_CLEAR_COLOR, &clear_color, 0.0f, 0);
//		gs_ortho(0.0f, w, 0.0f, h, -100.0f, 100.0f);
//
//		obs_source_video_render(input_source);
//		gs_texrender_end(filter->input_texrender);
//		filter->input_texture_generated = w > 0 && h > 0;
//	} else {
//		filter->input_texture_generated = false;
//	}
//	gs_blend_state_pop();
//	if (filter->is_source) {
//		obs_source_release(input_source);
//	}
//}
//
//static void draw_output(glow_filter_data_t *filter)
//{
//	gs_texture_t *texture =
//		gs_texrender_get_texture(filter->output_texrender);
//	gs_effect_t *pass_through = obs_get_base_effect(OBS_EFFECT_DEFAULT);
//	gs_eparam_t *param = gs_effect_get_param_by_name(pass_through, "image");
//	gs_effect_set_texture(param, texture);
//	uint32_t width = gs_texture_get_width(texture);
//	uint32_t height = gs_texture_get_height(texture);
//	while (gs_effect_loop(pass_through, "Draw")) {
//		gs_draw_sprite(texture, 0, width, height);
//	}
//}

static void load_effects(glow_filter_data_t *filter)
{
	load_1d_alpha_blur_effect(filter->alpha_blur_data);
	load_effect_dual_kawase(filter->alpha_blur_data);
	load_glow_effect(filter);
	load_glow_output_effect(filter);
}

static bool setting_fill_type_modified(obs_properties_t *props,
				       obs_property_t *p, obs_data_t *settings)
{
	UNUSED_PARAMETER(p);
	enum glow_fill_type fill_type = (enum glow_fill_type)obs_data_get_int(settings, "glow_fill_type");
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
	bool is_source = data;

	enum glow_position position = (enum glow_position)obs_data_get_int(settings, "glow_position");
	enum padding_type paddingType = (enum padding_type)obs_data_get_int(settings, "glow_padding");

	switch (position) {
	case GLOW_POSITION_INNER:
		setting_visibility("ignore_source_border", true, props);
		setting_visibility("fill", false, props);
		setting_visibility("glow_padding", false, props);
		setting_visibility("padding_amount", false, props);
		setting_visibility("threshold_inner", true, props);
		setting_visibility("threshold_outer", false, props);
		break;
	case GLOW_POSITION_OUTER:
		setting_visibility("ignore_source_border", false, props);
		setting_visibility("fill", is_source, props);
		setting_visibility("glow_padding", true, props);
		setting_visibility("padding_amount", paddingType == PADDING_MANUAL, props);
		setting_visibility("threshold_inner", false, props);
		setting_visibility("threshold_outer", true, props);
		break;
	default:
		break;
	}
	return true;
}

static bool setting_glow_padding_modified(obs_properties_t* props,
	obs_property_t* p, obs_data_t* settings)
{
	UNUSED_PARAMETER(p);
	enum padding_type paddingType = (enum padding_type)obs_data_get_int(settings, "glow_padding");

	setting_visibility("padding_amount", paddingType == PADDING_MANUAL, props);
	return true;
}
