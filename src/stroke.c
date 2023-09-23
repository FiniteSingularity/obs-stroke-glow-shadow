#include "stroke.h"
#include <string.h>

void load_stroke_effect(stroke_filter_data_t *filter)
{
	const char *effect_file_path = "/shaders/stroke.effect";
	filter->effect_stroke =
		load_shader_effect(filter->effect_stroke, effect_file_path);
	if (filter->effect_stroke) {
		size_t effect_count =
			gs_effect_get_num_params(filter->effect_stroke);
		for (size_t effect_index = 0; effect_index < effect_count;
		     effect_index++) {
			gs_eparam_t *param = gs_effect_get_param_by_idx(
				filter->effect_stroke, effect_index);
			struct gs_effect_param_info info;
			gs_effect_get_param_info(param, &info);
			if (strcmp(info.name, "stroke_thickness") == 0) {
				filter->param_stroke_stroke_thickness = param;
			} else if (strcmp(info.name, "stroke_offset") == 0) {
				filter->param_stroke_offset = param;
			}
		}
	}
}

void load_stroke_inner_effect(stroke_filter_data_t *filter)
{
	const char *effect_file_path = "/shaders/stroke_inner.effect";
	filter->effect_stroke_inner =
		load_shader_effect(filter->effect_stroke_inner, effect_file_path);
	if (filter->effect_stroke_inner) {
		size_t effect_count =
			gs_effect_get_num_params(filter->effect_stroke_inner);
		for (size_t effect_index = 0; effect_index < effect_count;
		     effect_index++) {
			gs_eparam_t *param = gs_effect_get_param_by_idx(
				filter->effect_stroke_inner, effect_index);
			struct gs_effect_param_info info;
			gs_effect_get_param_info(param, &info);
			if (strcmp(info.name, "stroke_thickness") == 0) {
				filter->param_stroke_inner_stroke_thickness = param;
			} else if (strcmp(info.name, "stroke_offset") == 0) {
				filter->param_stroke_inner_offset = param;
			}
		}
	}
}

void load_fill_stroke_effect(stroke_filter_data_t *filter)
{
	const char *effect_file_path =
		filter->is_filter ? "/shaders/fill_stroke.effect" : "/shaders/fill_stroke_source.effect";
	filter->effect_fill_stroke =
		load_shader_effect(filter->effect_fill_stroke, effect_file_path);
	if (filter->effect_fill_stroke) {
		size_t effect_count =
			gs_effect_get_num_params(filter->effect_fill_stroke);
		for (size_t effect_index = 0; effect_index < effect_count;
		     effect_index++) {
			gs_eparam_t *param = gs_effect_get_param_by_idx(
				filter->effect_fill_stroke, effect_index);
			struct gs_effect_param_info info;
			gs_effect_get_param_info(param, &info);
			if (strcmp(info.name, "image") == 0) {
				filter->param_fill_stroke_image = param;
			} else if(strcmp(info.name, "stroke_mask") == 0) {
				filter->param_fill_stroke_stroke_mask = param;
			} else if(strcmp(info.name, "stroke_fill_source") == 0) {
				filter->param_fill_stroke_fill_source = param;
			} else if(strcmp(info.name, "stroke_fill_color") == 0) {
				filter->param_fill_stroke_fill_color = param;
			} else if (strcmp(info.name, "fill_behind") == 0) {
				filter->param_fill_stroke_fill_behind = param;
			}
		}
	}
}


void render_stroke_filter(stroke_filter_data_t *data)
{
	gs_effect_t *effect = data->stroke_position == STROKE_POSITION_OUTER
				      ? data->effect_stroke
				      : data->effect_stroke_inner;


	gs_texture_t *blur_mask_texture = gs_texrender_get_texture(data->alpha_blur_data->alpha_blur_output);
	gs_texture_t *blur_mask_offset_texture =
		data->offset_quality == OFFSET_QUALITY_HIGH
			? gs_texrender_get_texture(data->alpha_blur_data->alpha_blur_output_2)
			: NULL;
	gs_texture_t *input_texture = gs_texrender_get_texture(data->input_texrender);

	if (!effect || !input_texture || !blur_mask_texture) {
		blog(LOG_INFO, "Something is missing in render_stroke_filter!!!");
		return;
	}
	// 1. First pass- apply 1D blur kernel to horizontal dir.
	data->stroke_mask =
		create_or_reset_texrender(data->stroke_mask);

	gs_eparam_t *image =
		gs_effect_get_param_by_name(effect, "image");
	gs_effect_set_texture(image, input_texture);

	gs_eparam_t *blur_mask =
		gs_effect_get_param_by_name(effect, "blur_mask");
	gs_effect_set_texture(blur_mask, blur_mask_texture);

	if (data->stroke_position == STROKE_POSITION_OUTER) {
		if (data->param_stroke_stroke_thickness) {
			gs_effect_set_float(data->param_stroke_stroke_thickness,
					    data->stroke_size);
		}

		if (data->param_stroke_offset) {
			gs_effect_set_float(data->param_stroke_offset,
					    data->stroke_offset);
		}
	} else if (data->stroke_position == STROKE_POSITION_INNER) {
		if (data->param_stroke_inner_stroke_thickness) {
			gs_effect_set_float(data->param_stroke_inner_stroke_thickness,
					    data->stroke_size);
		}

		if (data->param_stroke_inner_offset) {
			gs_effect_set_float(data->param_stroke_inner_offset,
					    data->stroke_offset);
		}
	}


	if (data->offset_quality == OFFSET_QUALITY_HIGH) {
		gs_eparam_t *blur_mask_offset =
			gs_effect_get_param_by_name(effect, "blur_mask_inner");
		gs_effect_set_texture(blur_mask_offset,
				      blur_mask_offset_texture);
	}

	const char *offset_type = data->stroke_offset < 0.99f ? "Filled"
				  : data->offset_quality ==
						  OFFSET_QUALITY_NORMAL
					  ? "OffsetInline"
					  : "OffsetSubtract";
	set_blending_parameters();

	if (gs_texrender_begin(data->stroke_mask, data->width,
				data->height)) {
		gs_ortho(0.0f, (float)data->width, 0.0f,
				(float)data->height, -100.0f, 100.0f);
		while (gs_effect_loop(effect, offset_type))
			gs_draw_sprite(input_texture, 0, data->width,
					data->height);
		gs_texrender_end(data->stroke_mask);
	}
	gs_blend_state_pop();
}

void render_fill_stroke_filter(stroke_filter_data_t *data)
{
	gs_effect_t *effect = data->effect_fill_stroke;
	gs_texture_t *stroke_mask_texture =
		gs_texrender_get_texture(data->stroke_mask);
	gs_texture_t *image_texture =
		gs_texrender_get_texture(data->input_texrender);

	if (!effect || !image_texture || !stroke_mask_texture) {
		return;
	}

	data->output_texrender = create_or_reset_texrender(data->output_texrender);

	if (data->param_fill_stroke_image) {
		gs_effect_set_texture(data->param_fill_stroke_image,
				      image_texture);
	}

	if (data->param_fill_stroke_stroke_mask) {
		gs_effect_set_texture(data->param_fill_stroke_stroke_mask,
				      stroke_mask_texture);
	}

	if (data->param_fill_stroke_fill_behind) {
		gs_effect_set_float(data->param_fill_stroke_fill_behind,
				    data->fill ? 0.0f : 1.0f);
	}

	gs_texrender_t *source_render = NULL;
	if (data->fill_type == STROKE_FILL_TYPE_SOURCE) {
		obs_source_t *source =
			data->fill_source_source
				? obs_weak_source_get_source(
					  data->fill_source_source)
				: NULL;
		if (!source) {
			return;
		}

		const enum gs_color_space preferred_spaces[] = {
			GS_CS_SRGB,
			GS_CS_SRGB_16F,
			GS_CS_709_EXTENDED,
		};
		const enum gs_color_space space = obs_source_get_color_space(
			source, OBS_COUNTOF(preferred_spaces),
			preferred_spaces);
		const enum gs_color_format format =
			gs_get_format_from_space(space);

		// Set up a tex renderer for source
		source_render = gs_texrender_create(format, GS_ZS_NONE);
		uint32_t base_width = obs_source_get_width(source);
		uint32_t base_height = obs_source_get_height(source);
		gs_blend_state_push();
		gs_blend_function(GS_BLEND_ONE, GS_BLEND_ZERO);
		if (gs_texrender_begin_with_color_space(
			    source_render, base_width, base_height, space)) {
			const float w = (float)base_width;
			const float h = (float)base_height;
			uint32_t flags = obs_source_get_output_flags(source);
			const bool custom_draw =
				(flags & OBS_SOURCE_CUSTOM_DRAW) != 0;
			const bool async = (flags & OBS_SOURCE_ASYNC) != 0;
			struct vec4 clear_color;

			vec4_zero(&clear_color);
			gs_clear(GS_CLEAR_COLOR, &clear_color, 0.0f, 0);
			gs_ortho(0.0f, w, 0.0f, h, -100.0f, 100.0f);

			if (!custom_draw && !async)
				obs_source_default_render(source);
			else
				obs_source_video_render(source);
			gs_texrender_end(source_render);
		}
		gs_blend_state_pop();
		obs_source_release(source);
		gs_texture_t *source_texture = gs_texrender_get_texture(source_render);
		if (data->param_fill_stroke_fill_source) {
			gs_effect_set_texture(
				data->param_fill_stroke_fill_source,
					      source_texture);
		}
	} else if (data->fill_type == STROKE_FILL_TYPE_COLOR) {
		if (data->param_fill_stroke_fill_color) {
			gs_effect_set_vec4(data->param_fill_stroke_fill_color,
					   &data->stroke_color);
		}
	}

	const char *fill_type =
		data->fill_type == STROKE_FILL_TYPE_COLOR    ? "FilterColor"
		: data->fill_type == STROKE_FILL_TYPE_SOURCE ? "FilterSource"
							     : "FilterSource";

	const char *position = data->stroke_position == STROKE_POSITION_OUTER
				       ? "Outer"
				       : "Inner";

	char shader_id[100] = "";
	strncat(shader_id, fill_type, strlen(fill_type));
	strncat(shader_id, position, strlen(position));

	set_blending_parameters();

	if (gs_texrender_begin(data->output_texrender, data->width, data->height)) {
		gs_ortho(0.0f, (float)data->width, 0.0f, (float)data->height,
			 -100.0f, 100.0f);
		while (gs_effect_loop(effect, shader_id))
			gs_draw_sprite(image_texture, 0, data->width,
				       data->height);
		gs_texrender_end(data->output_texrender);
	}
	gs_blend_state_pop();

	if (source_render) {
		gs_texrender_destroy(source_render);
	}
}
