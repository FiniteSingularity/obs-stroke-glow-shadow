#include "obs-stroke-filter.h"
#include "obs-stroke.h"
#include "blur/alpha-blur.h"
#include "anti-alias.h"
#include "stroke.h"

struct obs_source_info obs_stroke_source = {
	.id = "obs_stroke_source",
	.type = OBS_SOURCE_TYPE_INPUT,
	.output_flags = OBS_SOURCE_VIDEO | OBS_SOURCE_CUSTOM_DRAW | OBS_SOURCE_SRGB,
	.get_name = stroke_filter_name,
	.create = stroke_filter_create,
	.destroy = stroke_filter_destroy,
	.update = stroke_filter_update,
	.video_render = stroke_filter_video_render,
	.video_tick = stroke_filter_video_tick,
	.video_get_color_space = stroke_source_get_color_space,
	.get_width = stroke_filter_width,
	.get_height = stroke_filter_height,
	.get_properties = stroke_source_properties,
	.get_defaults = stroke_filter_defaults,
	.icon_type = OBS_ICON_TYPE_COLOR};

struct obs_source_info obs_stroke_filter = {
	.id = "obs_stroke_filter",
	.type = OBS_SOURCE_TYPE_FILTER,
	.output_flags = OBS_SOURCE_VIDEO | OBS_SOURCE_SRGB | OBS_SOURCE_CUSTOM_DRAW,
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

	filter->alpha_blur_data = bzalloc(sizeof(alpha_blur_data_t));

	filter->context = source;
	filter->input_texture_generated = false;
	filter->is_source = obs_source_get_type(filter->context) ==
			    OBS_SOURCE_TYPE_INPUT;

	filter->is_filter = obs_source_get_type(filter->context) ==
			    OBS_SOURCE_TYPE_FILTER;

	filter->param_stroke_texel_step = NULL;
	filter->param_stroke_stroke_thickness = NULL;
	filter->param_stroke_offset = NULL;
	filter->param_stroke_inner_stroke_thickness = NULL;
	filter->param_stroke_inner_offset = NULL;

	filter->param_fill_stroke_image = NULL;
	filter->param_fill_stroke_stroke_mask = NULL;
	filter->param_fill_stroke_fill_source = NULL;
	filter->param_fill_stroke_fill_color = NULL;
	filter->param_fill_stroke_fill_behind = NULL;

	filter->param_aa_texel_step = NULL;
	filter->param_aa_size = NULL;
	filter->param_aa_image = NULL;

	filter->reload = true;

	filter->jump_flood_threshold = 0.5;

	alpha_blur_init(filter->alpha_blur_data);

	obs_source_update(source, settings);
	return filter;
}

static void stroke_filter_destroy(void *data)
{
	stroke_filter_data_t *filter = data;

	obs_enter_graphics();

	if (filter->effect_stroke) {
		gs_effect_destroy(filter->effect_stroke);
	}
	if (filter->effect_jump_flood_sdf) {
		gs_effect_destroy(filter->effect_jump_flood_sdf);
	}
	if (filter->effect_stroke_inner) {
		gs_effect_destroy(filter->effect_stroke_inner);
	}
	if (filter->effect_anti_alias) {
		gs_effect_destroy(filter->effect_anti_alias);
	}
	if (filter->effect_fill_stroke) {
		gs_effect_destroy(filter->effect_fill_stroke);
	}
	if (filter->effect_output) {
		gs_effect_destroy(filter->effect_output);
	}

	if (filter->stroke_mask) {
		gs_texrender_destroy(filter->stroke_mask);
	}
	if (filter->input_texrender) {
		gs_texrender_destroy(filter->input_texrender);
	}
	if (filter->output_texrender) {
		gs_texrender_destroy(filter->output_texrender);
	}
	if (filter->buffer_a) {
		gs_texrender_destroy(filter->buffer_a);
	}
	if (filter->buffer_b) {
		gs_texrender_destroy(filter->buffer_b);
	}
	if (filter->buffer_outer_threshold) {
		gs_texrender_destroy(filter->buffer_outer_threshold);
	}
	if (filter->buffer_inner_threshold) {
		gs_texrender_destroy(filter->buffer_inner_threshold);
	}
	if (filter->buffer_outer_distance_field) {
		gs_texrender_destroy(filter->buffer_outer_distance_field);
	}
	if (filter->buffer_inner_distance_field) {
		gs_texrender_destroy(filter->buffer_inner_distance_field);
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

static uint32_t stroke_filter_width(void *data)
{
	stroke_filter_data_t *filter = data;
	return filter->output_width;
}

static uint32_t stroke_filter_height(void *data)
{
	stroke_filter_data_t *filter = data;
	return filter->output_height;
}

static void stroke_filter_update(void *data, obs_data_t *settings)
{
	stroke_filter_data_t *filter = data;
	filter->stroke_size =
		(float)obs_data_get_double(settings, "stroke_size");

	filter->stroke_offset =
		(float)obs_data_get_double(settings, "stroke_offset");

	// THIS WILL MESS UP FILTER VERSION.
	// REVISIT!! TODO TODO TODO
	vec4_from_rgba(&filter->stroke_color,
		(uint32_t)obs_data_get_int(settings,
			"stroke_fill_color"));

	vec4_from_rgba_srgb(&filter->stroke_color_srgb,
		       (uint32_t)obs_data_get_int(settings,
						  "stroke_fill_color"));

	filter->fill_type =
		(enum stroke_fill_type)obs_data_get_int(settings, "stroke_fill_type");
	filter->stroke_position =
		(enum stroke_position)obs_data_get_int(settings, "stroke_position");
	filter->contour_spacing = (float)obs_data_get_double(settings, "contour_spacing");
	filter->contour_offset = (float)obs_data_get_double(settings, "contour_offset");
	filter->contour_falloff_start = (float)obs_data_get_double(settings, "contour_falloff_start");
	filter->contour_falloff_end = (float)obs_data_get_double(settings, "contour_falloff_end");
	filter->contour_spacing_power = (float)obs_data_get_double(settings, "contour_spacing_power");

	filter->padding_amount = obs_data_get_int(settings, "stroke_padding") == PADDING_MANUAL ? (uint32_t)obs_data_get_int(settings, "padding_amount") : 0;

	if (obs_data_get_int(settings, "stroke_padding") == PADDING_AUTO && filter->stroke_position == STROKE_POSITION_OUTER) {
		filter->padding_amount = (uint32_t)filter->stroke_size + (uint32_t)filter->stroke_offset;
	}

	filter->ignore_source_border =
		obs_data_get_bool(settings, "ignore_source_border");

	bool is_inner = filter->stroke_position == STROKE_POSITION_INNER || filter->stroke_position == STROKE_POSITION_INNER_CONTOUR;

	if (is_inner) {
		filter->jump_flood_threshold = (float)min(max(obs_data_get_double(settings, "jump_flood_threshold_inner"), 0.01), 99.99) / 100.0f;
	} else {
		filter->jump_flood_threshold = (float)min(max(obs_data_get_double(settings, "jump_flood_threshold_outer"), 0.01), 99.99) / 100.0f;
	}

	if (!filter->ignore_source_border && is_inner) {
		filter->pad_b = 1;
		filter->pad_t = 1;
		filter->pad_l = 1;
		filter->pad_r = 1;
	}
	else if (filter->stroke_position == STROKE_POSITION_OUTER || filter->stroke_position == STROKE_POSITION_OUTER_CONTOUR) {
		filter->pad_b = filter->padding_amount;
		filter->pad_t = filter->padding_amount;
		filter->pad_l = filter->padding_amount;
		filter->pad_r = filter->padding_amount;
	}
	else {
		filter->pad_b = 0;
		filter->pad_t = 0;
		filter->pad_l = 0;
		filter->pad_r = 0;
	}

	filter->fill = obs_data_get_bool(settings, "fill");
	const char *fill_source_name =
		obs_data_get_string(settings, "stroke_fill_source");
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
		if (filter->fill_source_source) {
			obs_weak_source_release(filter->fill_source_source);
		}
		filter->fill_source_source = NULL;
	}

	if (filter->is_source) {
		const char *stroke_source_name =
			obs_data_get_string(settings, "stroke_source");
		obs_source_t *stroke_source =
			(stroke_source_name && strlen(stroke_source_name))
				? obs_get_source_by_name(stroke_source_name)
				: NULL;
		if (stroke_source) {
			obs_weak_source_release(filter->source_input_source);
			filter->source_input_source =
				obs_source_get_weak_source(stroke_source);
			filter->width =
				(uint32_t)obs_source_get_width(stroke_source);
			filter->height =
				(uint32_t)obs_source_get_height(stroke_source);
			obs_source_release(stroke_source);
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
		// draw_output(filter);
		render_cropped_output(filter);
		return;
	}

	bool skipRender = filter->rendering || filter->stroke_size <= 0.01;
	if (skipRender && filter->is_filter) {
		obs_source_skip_video_filter(filter->context);
		return;
	} else if (skipRender) {
		return;
	}

	filter->rendering = true;

	// 1. Get the input source as a texture renderer
	//    accessed as filter->input_texrender after call
	render_padded_input(filter);
	if (!filter->input_texture_generated) {
		filter->rendering = false;
		if (filter->is_filter) {
			obs_source_skip_video_filter(filter->context);
		}
		return;
	}

	// 2. Create Stroke Mask
	if (filter->stroke_position == STROKE_POSITION_OUTER || filter->stroke_position == STROKE_POSITION_OUTER_CONTOUR) {
		render_jf_outer_threshold(filter);
		bool contour = filter->stroke_position == STROKE_POSITION_OUTER_CONTOUR;
		float outerExtent = contour ? fmaxf((float)filter->width, (float)filter->height) : filter->stroke_offset + filter->stroke_size;
		render_jf_passes_outer(filter, outerExtent);
	}
	else {
		render_jf_inner_threshold(filter);
		bool contour = filter->stroke_position == STROKE_POSITION_INNER_CONTOUR;
		float innerExtent = contour ? fmaxf((float)filter->width, (float)filter->height) : filter->stroke_offset + filter->stroke_size;
		render_jf_passes_inner(filter, innerExtent);
	}
	
	render_jf_distance(filter);

	// 3. Draw result (filter->output_texrender) to source
	render_cropped_output(filter);
	filter->rendered = true;
	filter->rendering = false;
}

static obs_properties_t *properties(void *data, bool is_source)
{
	stroke_filter_data_t *filter = data;

	obs_properties_t *props = obs_properties_create();
	obs_properties_set_param(props, filter, NULL);

	if (is_source) {
		obs_property_t *stroke_source = obs_properties_add_list(
			props, "stroke_source",
			obs_module_text("StrokeSource.Source"),
			OBS_COMBO_TYPE_EDITABLE, OBS_COMBO_FORMAT_STRING);
		obs_property_list_add_string(
			stroke_source, obs_module_text("StrokeCommon.None"),
			"");
		obs_enum_sources(add_source_to_list, stroke_source);
		obs_enum_scenes(add_source_to_list, stroke_source);
	}

	obs_property_t *stroke_position_list = obs_properties_add_list(
		props, "stroke_position",
		obs_module_text("StrokeFilter.StrokePosition"),
		OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);

	obs_property_list_add_int(stroke_position_list,
				  obs_module_text(STROKE_POSITION_OUTER_LABEL),
				  STROKE_POSITION_OUTER);
	obs_property_list_add_int(stroke_position_list,
				  obs_module_text(STROKE_POSITION_INNER_LABEL),
				  STROKE_POSITION_INNER);
	obs_property_list_add_int(stroke_position_list,
				  obs_module_text(STROKE_POSITION_OUTER_CONTOUR_LABEL),
		                  STROKE_POSITION_OUTER_CONTOUR);
	obs_property_list_add_int(stroke_position_list,
				  obs_module_text(STROKE_POSITION_INNER_CONTOUR_LABEL),
				  STROKE_POSITION_INNER_CONTOUR);


	obs_property_set_modified_callback2(
		stroke_position_list, setting_stroke_position_modified, (void*)is_source);

	obs_properties_add_bool(
		props, "ignore_source_border",
		obs_module_text("StrokeCommon.IgnoreSourceBorder"));

	obs_properties_add_bool(props, "fill",
				obs_module_text("StrokeFilter.FillSource"));

	obs_property_t* stroke_padding_list = obs_properties_add_list(
		props, "stroke_padding",
		obs_module_text("StrokeFilter.Padding"),
		OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);

	obs_property_list_add_int(stroke_padding_list,
		obs_module_text(PADDING_NONE_LABEL),
		PADDING_NONE);

	obs_property_list_add_int(stroke_padding_list,
		obs_module_text(PADDING_AUTO_LABEL),
		PADDING_AUTO);

	obs_property_list_add_int(stroke_padding_list,
		obs_module_text(PADDING_MANUAL_LABEL),
		PADDING_MANUAL);


	obs_property_set_modified_callback(
		stroke_padding_list, setting_stroke_padding_modified);

	obs_property_t * p = obs_properties_add_int_slider(
		props, "padding_amount",
		obs_module_text("StrokeFilter.Padding.Amount"), 0, 4000, 1
	);

	obs_property_float_set_suffix(p, "px");

	p = obs_properties_add_float_slider(
		props, "jump_flood_threshold_inner",
		obs_module_text("StrokeFilter.MaskThreshold"), 0.0, 100.0, 0.01
	);
	obs_property_float_set_suffix(p, "%");

	p = obs_properties_add_float_slider(
		props, "jump_flood_threshold_outer",
		obs_module_text("StrokeFilter.MaskThreshold"), 0.0, 100.0, 0.01
	);
	obs_property_float_set_suffix(p, "%");

	p = obs_properties_add_float_slider(
		props, "stroke_size",
		obs_module_text("StrokeFilter.StrokeSize"), 0.0, 2000.0, 1.0);
	obs_property_float_set_suffix(p, "px");

	p = obs_properties_add_float_slider(
		props, "stroke_offset",
		obs_module_text("StrokeFilter.StrokeOffset"), 0.0, 2000.0, 1.0);
	obs_property_float_set_suffix(p, "px");

	p = obs_properties_add_float_slider(
		props, "contour_spacing",
		obs_module_text("StrokeFilter.ContourSpacing"), 0.0, 2000.0, 1.0);
	obs_property_float_set_suffix(p, "px");

	obs_properties_add_float_slider(
		props, "contour_spacing_power",
		obs_module_text("StrokeFilter.ContourSpacingPower"), 0.01, 2.0, 0.01);

	obs_property_t* c_offset = obs_properties_add_float_slider(
		props, "contour_offset",
		obs_module_text("StrokeFilter.ContourOffset"), -500.0, 500.0, 0.01);

	obs_property_float_set_suffix(c_offset, "%");

	p = obs_properties_add_float_slider(
		props, "contour_falloff_start",
		obs_module_text("StrokeFilter.ContourFalloffStart"), 0.0, 2000.0, 1.0);
	obs_property_float_set_suffix(p, "px");

	p = obs_properties_add_float_slider(
		props, "contour_falloff_end",
		obs_module_text("StrokeFilter.ContourFalloffEnd"), 0.0, 2000.0, 1.0);
	obs_property_float_set_suffix(p, "px");

	obs_property_t *stroke_fill_method_list = obs_properties_add_list(
		props, "stroke_fill_type",
		obs_module_text("StrokeFilter.FillType"), OBS_COMBO_TYPE_LIST,
		OBS_COMBO_FORMAT_INT);

	obs_property_list_add_int(stroke_fill_method_list,
				  obs_module_text(STROKE_FILL_TYPE_COLOR_LABEL),
				  STROKE_FILL_TYPE_COLOR);
	obs_property_list_add_int(
		stroke_fill_method_list,
		obs_module_text(STROKE_FILL_TYPE_SOURCE_LABEL),
		STROKE_FILL_TYPE_SOURCE);
	//obs_property_list_add_int(
	//	stroke_fill_method_list,
	//	obs_module_text(STROKE_FILL_TYPE_IMAGE_LABEL),
	//	STROKE_FILL_TYPE_IMAGE);

	obs_property_set_modified_callback(stroke_fill_method_list,
					   setting_fill_type_modified);

	obs_properties_add_color_alpha(
		props, "stroke_fill_color",
		obs_module_text("StrokeFilter.ColorFill"));

	obs_properties_add_path(
		props, "stroke_fill_image",
		obs_module_text("StrokeFilter.ImageFill"), OBS_PATH_FILE,
		"Textures (*.bmp *.tga *.png *.jpeg *.jpg *.gif);;", NULL);

	obs_property_t *stroke_fill_source_source = obs_properties_add_list(
		props, "stroke_fill_source",
		obs_module_text("StrokeFilter.SourceFill"),
		OBS_COMBO_TYPE_EDITABLE, OBS_COMBO_FORMAT_STRING);
	obs_enum_sources(add_source_to_list, stroke_fill_source_source);
	obs_enum_scenes(add_source_to_list, stroke_fill_source_source);
	obs_property_list_insert_string(stroke_fill_source_source, 0, "None",
					"");

	obs_properties_add_text(props, "plugin_info", PLUGIN_INFO,
				OBS_TEXT_INFO);

	return props;
}

static obs_properties_t *stroke_filter_properties(void *data) {
	return properties(data, false);
}

static obs_properties_t *stroke_source_properties(void *data) {
	return properties(data, true);
}

static enum gs_color_space stroke_source_get_color_space(void* data, size_t count, const enum gs_color_space* preferred_spaces) {
	stroke_filter_data_t* filter = data;
	const enum gs_color_space potential_spaces[] = {
		GS_CS_SRGB,
		GS_CS_SRGB_16F,
		GS_CS_709_EXTENDED,
	};

	obs_source_t* input_source = filter->source_input_source
		? obs_weak_source_get_source(
			filter->source_input_source)
		: NULL;

	const enum gs_color_space source_space = input_source ? obs_source_get_color_space(
		input_source, OBS_COUNTOF(potential_spaces), potential_spaces) : GS_CS_SRGB;


	if (input_source) {
		obs_source_release(input_source);
	}

	return source_space;
}

static void stroke_filter_video_tick(void *data, float seconds)
{
	UNUSED_PARAMETER(seconds);
	stroke_filter_data_t *filter = data;
	if (filter->is_filter) {
		obs_source_t *target = obs_filter_get_target(filter->context);
		if (!target) {
			return;
		}
		filter->source_width = (uint32_t)obs_source_get_base_width(target);
		filter->source_height = (uint32_t)obs_source_get_base_height(target);
		if (filter->stroke_position == STROKE_POSITION_INNER || filter->stroke_position == STROKE_POSITION_INNER_CONTOUR) {
			filter->output_width = (uint32_t)obs_source_get_base_width(target);
			filter->output_height = (uint32_t)obs_source_get_base_height(target);
		}
		else {
			filter->output_width = (uint32_t)obs_source_get_base_width(target) + filter->pad_l + filter->pad_r;
			filter->output_height = (uint32_t)obs_source_get_base_height(target) + filter->pad_t + filter->pad_b;
		}
	} else {
		if (filter->stroke_position == STROKE_POSITION_INNER || filter->stroke_position == STROKE_POSITION_INNER_CONTOUR) {
			filter->output_width = filter->source_width;
			filter->output_height = filter->source_height;
		} else {
			filter->output_width = filter->width;
			filter->output_height = filter->height;
		}

	}
	filter->rendered = false;
}

static void stroke_filter_defaults(obs_data_t *settings)
{
	obs_data_set_default_double(settings, "stroke_size", 4.0);
	obs_data_set_default_double(settings, "stroke_offset", 0.0);
	obs_data_set_default_int(settings, "stroke_fill_type",
				 STROKE_FILL_TYPE_COLOR);
	obs_data_set_default_string(settings, "stroke_fill_source", "");
	obs_data_set_default_int(settings, "stroke_fill_color", DEFAULT_COLOR);
	obs_data_set_default_int(settings, "stroke_offset_quality",
				 OFFSET_QUALITY_NORMAL);
	obs_data_set_default_int(settings, "stroke_position",
				 STROKE_POSITION_OUTER);
	obs_data_set_default_bool(settings, "ignore_source_border", true);
	obs_data_set_default_bool(settings, "fill", true);
	obs_data_set_default_double(settings, "contour_spacing", 10.0);
	obs_data_set_default_double(settings, "contour_offset", 0.0);
	obs_data_set_default_double(settings, "contour_spacing_power", 1.0);
	obs_data_set_default_double(settings, "jump_flood_threshold_outer", 100.0);
	obs_data_set_default_double(settings, "jump_flood_threshold_inner", 0.0);
	obs_data_set_default_double(settings, "contour_falloff_start", 0.0);
	obs_data_set_default_double(settings, "contour_falloff_end", 2000.0);

}

static void get_input_source(stroke_filter_data_t *filter)
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

static void draw_output(stroke_filter_data_t *filter)
{
	if (filter->is_source) {
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

static void load_effects(stroke_filter_data_t *filter)
{
	load_1d_alpha_blur_effect(filter->alpha_blur_data);
	load_stroke_effect(filter);
	load_jump_flood_sdf_effect(filter);
	load_1d_anti_alias_effect(filter);
	load_fill_stroke_effect(filter);
	load_stroke_inner_effect(filter);
	load_output_effect(filter);
}

static bool setting_fill_type_modified(obs_properties_t *props,
				       obs_property_t *p, obs_data_t *settings)
{
	UNUSED_PARAMETER(p);
	enum stroke_fill_type fill_type = (enum stroke_fill_type)obs_data_get_int(settings, "stroke_fill_type");
	switch (fill_type) {
	case STROKE_FILL_TYPE_COLOR:
		setting_visibility("stroke_fill_color", true, props);
		setting_visibility("stroke_fill_source", false, props);
		setting_visibility("stroke_fill_image", false, props);
		break;
	case STROKE_FILL_TYPE_SOURCE:
		setting_visibility("stroke_fill_color", false, props);
		setting_visibility("stroke_fill_source", true, props);
		setting_visibility("stroke_fill_image", false, props);
		break;
	case STROKE_FILL_TYPE_IMAGE:
		setting_visibility("stroke_fill_color", false, props);
		setting_visibility("stroke_fill_source", false, props);
		setting_visibility("stroke_fill_image", true, props);
		break;
	}
	return true;
}

static bool setting_stroke_padding_modified(obs_properties_t* props,
	obs_property_t* p, obs_data_t* settings)
{
	UNUSED_PARAMETER(p);
	enum padding_type paddingType = (enum padding_type)obs_data_get_int(settings, "stroke_padding");

	setting_visibility("padding_amount", paddingType == PADDING_MANUAL, props);
	return true;
}

static bool setting_stroke_position_modified(void *data,
					     obs_properties_t *props,
					     obs_property_t *p,
					     obs_data_t *settings)
{
	UNUSED_PARAMETER(p);
	bool is_source = data;

	enum stroke_position position = (enum stroke_position)obs_data_get_int(settings, "stroke_position");
	enum padding_type paddingType = (enum padding_type)obs_data_get_int(settings, "stroke_padding");
	switch (position) {
	case STROKE_POSITION_INNER:
		setting_visibility("ignore_source_border", true, props);
		setting_visibility("jump_flood_threshold_inner", true, props);
		setting_visibility("jump_flood_threshold_outer", false, props);
		setting_visibility("contour_spacing", false, props);
		setting_visibility("contour_offset", false, props);
		setting_visibility("contour_spacing_power", false, props);
		setting_visibility("contour_falloff_start", false, props);
		setting_visibility("contour_falloff_end", false, props);
		setting_visibility("stroke_offset", true, props);
		setting_visibility("fill", false, props);
		setting_visibility("stroke_padding", false, props);
		setting_visibility("padding_amount", false, props);
		break;
	case STROKE_POSITION_OUTER:
		setting_visibility("ignore_source_border", false, props);
		setting_visibility("jump_flood_threshold_inner", false, props);
		setting_visibility("jump_flood_threshold_outer", true, props);
		setting_visibility("contour_spacing", false, props);
		setting_visibility("contour_offset", false, props);
		setting_visibility("contour_spacing_power", false, props);
		setting_visibility("contour_falloff_start", false, props);
		setting_visibility("contour_falloff_end", false, props);
		setting_visibility("stroke_offset", true, props);
		setting_visibility("fill", is_source, props);
		setting_visibility("stroke_padding", true, props);
		setting_visibility("padding_amount", paddingType == PADDING_MANUAL, props);
		break;
	case STROKE_POSITION_OUTER_CONTOUR:
		setting_visibility("ignore_source_border", false, props);
		setting_visibility("jump_flood_threshold_inner", false, props);
		setting_visibility("jump_flood_threshold_outer", true, props);
		setting_visibility("contour_spacing", true, props);
		setting_visibility("contour_offset", true, props);
		setting_visibility("contour_spacing_power", true, props);
		setting_visibility("contour_falloff_start", true, props);
		setting_visibility("contour_falloff_end", true, props);
		setting_visibility("stroke_offset", false, props);
		setting_visibility("fill", false, props);
		setting_visibility("stroke_padding", true, props);
		setting_visibility("padding_amount", paddingType == PADDING_MANUAL, props);
		break;
	case STROKE_POSITION_INNER_CONTOUR:
		setting_visibility("ignore_source_border", true, props);
		setting_visibility("jump_flood_threshold_inner", true, props);
		setting_visibility("jump_flood_threshold_outer", false, props);
		setting_visibility("contour_spacing", true, props);
		setting_visibility("contour_offset", true, props);
		setting_visibility("contour_spacing_power", true, props);
		setting_visibility("contour_falloff_start", true, props);
		setting_visibility("contour_falloff_end", true, props);
		setting_visibility("stroke_offset", false, props);
		setting_visibility("fill", false, props);
		setting_visibility("stroke_padding", false, props);
		setting_visibility("padding_amount", false, props);
		break;
	default:
		break;
	}
	return true;
}
