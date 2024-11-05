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
	.get_width = stroke_filter_width,
	.get_height = stroke_filter_height,
	.get_properties = stroke_source_properties,
	.get_defaults = stroke_filter_defaults,
	.icon_type = OBS_ICON_TYPE_COLOR};

struct obs_source_info obs_stroke_filter = {
	.id = "obs_stroke_filter",
	.type = OBS_SOURCE_TYPE_FILTER,
	.output_flags = OBS_SOURCE_VIDEO | OBS_SOURCE_SRGB,
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
	if (filter->effect_stroke_inner) {
		gs_effect_destroy(filter->effect_stroke_inner);
	}
	if (filter->effect_anti_alias) {
		gs_effect_destroy(filter->effect_anti_alias);
	}
	if (filter->effect_fill_stroke) {
		gs_effect_destroy(filter->effect_fill_stroke);
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

	alpha_blur_destroy(filter->alpha_blur_data);

	obs_leave_graphics();
	bfree(filter->alpha_blur_data);
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

	filter->stroke_offset =
		(float)obs_data_get_double(settings, "stroke_offset");

	vec4_from_rgba(&filter->stroke_color,
		       (uint32_t)obs_data_get_int(settings,
						  "stroke_fill_color"));

	filter->fill_type =
		(enum stroke_fill_type)obs_data_get_int(settings, "stroke_fill_type");

	filter->offset_quality =
		(enum offset_quality)obs_data_get_int(settings, "stroke_offset_quality");
	filter->stroke_position =
		(enum stroke_position)obs_data_get_int(settings, "stroke_position");
	filter->anti_alias = obs_data_get_bool(settings, "anti_alias");
	filter->ignore_source_border =
		obs_data_get_bool(settings, "ignore_source_border");
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
		draw_output(filter);
		return;
	}

	if (filter->rendering && filter->is_filter) {
		obs_source_skip_video_filter(filter->context);
		return;
	} else if (filter->rendering) {
		return;
	}

	filter->rendering = true;

	// 1. Get the input source as a texture renderer
	//    accessed as filter->input_texrender after call
	get_input_source(filter);
	if (!filter->input_texture_generated) {
		filter->rendering = false;
		if (filter->is_filter) {
			obs_source_skip_video_filter(filter->context);
		}
		return;
	}

	// 2. Apply effect to texture, and render texture to video
	alpha_blur(filter->stroke_size + filter->stroke_offset,
		   filter->ignore_source_border, filter->alpha_blur_data,
		   filter->input_texrender,
		   filter->alpha_blur_data->alpha_blur_output);
	if (filter->offset_quality == OFFSET_QUALITY_HIGH) {
		alpha_blur(filter->stroke_offset, filter->ignore_source_border,
			   filter->alpha_blur_data, filter->input_texrender,
			   filter->alpha_blur_data->alpha_blur_output_2);
	}

	// 3. Create Stroke Mask
	render_stroke_filter(filter);

	if (filter->anti_alias) {
		anti_alias(filter);
	}
	render_fill_stroke_filter(filter);

	// 3. Draw result (filter->output_texrender) to source
	draw_output(filter);
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

	obs_property_set_modified_callback2(
		stroke_position_list, setting_stroke_position_modified, (void*)is_source);

	obs_properties_add_bool(
		props, "ignore_source_border",
		obs_module_text("StrokeCommon.IgnoreSourceBorder"));

	obs_properties_add_bool(props, "fill",
				obs_module_text("StrokeFilter.FillSource"));

	obs_properties_add_float_slider(
		props, "stroke_size",
		obs_module_text("StrokeFilter.StrokeSize"), 0.0, 100.0, 1.0);

	obs_properties_add_float_slider(
		props, "stroke_offset",
		obs_module_text("StrokeFilter.StrokeOffset"), 0.0, 50.0, 1.0);

	obs_property_t *stroke_offset_quality_list = obs_properties_add_list(
		props, "stroke_offset_quality",
		obs_module_text("StrokeFilter.StrokeOffsetQuality"),
		OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);

	obs_property_list_add_int(stroke_offset_quality_list,
				  obs_module_text(OFFSET_QUALITY_NORMAL_LABEL),
				  OFFSET_QUALITY_NORMAL);
	obs_property_list_add_int(stroke_offset_quality_list,
				  obs_module_text(OFFSET_QUALITY_HIGH_LABEL),
				  OFFSET_QUALITY_HIGH);

	obs_properties_add_bool(props, "anti_alias",
				obs_module_text("StrokeFilter.AntiAlias"));

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

static void stroke_filter_video_tick(void *data, float seconds)
{
	UNUSED_PARAMETER(seconds);
	stroke_filter_data_t *filter = data;
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

static void stroke_filter_defaults(obs_data_t *settings)
{
	obs_data_set_default_double(settings, "stroke_size", 4.0);
	obs_data_set_default_double(settings, "stroke_offset", 0.0);
	obs_data_set_default_bool(settings, "anti_alias", false);
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

static bool setting_stroke_position_modified(void *data,
					     obs_properties_t *props,
					     obs_property_t *p,
					     obs_data_t *settings)
{
	UNUSED_PARAMETER(p);
	bool is_source = data;

	enum stroke_position position = (enum stroke_position)obs_data_get_int(settings, "stroke_position");
	switch (position) {
	case STROKE_POSITION_INNER:
		setting_visibility("ignore_source_border", true, props);
		setting_visibility("fill", false, props);
		break;
	case STROKE_POSITION_OUTER:
		setting_visibility("ignore_source_border", false, props);
		setting_visibility("fill", is_source, props);
		break;
	default:
		break;
	}
	return true;
}
