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

void load_jump_flood_sdf_effect(stroke_filter_data_t* filter)
{
	const char* effect_file_path = filter->is_source ? "/shaders/jump-flood-sdf-source.effect" : "/shaders/jump-flood-sdf.effect";
	filter->effect_jump_flood_sdf =
		load_shader_effect(filter->effect_jump_flood_sdf, effect_file_path);
	if (filter->effect_jump_flood_sdf) {
		size_t effect_count =
			gs_effect_get_num_params(filter->effect_jump_flood_sdf);
		for (size_t effect_index = 0; effect_index < effect_count;
			effect_index++) {
			gs_eparam_t* param = gs_effect_get_param_by_idx(
				filter->effect_jump_flood_sdf, effect_index);
			struct gs_effect_param_info info;
			gs_effect_get_param_info(param, &info);
			if (strcmp(info.name, "threshold") == 0) {
				filter->param_jump_flood_threshold = param;
			} else if(strcmp(info.name, "uv_size") == 0) {
				filter->param_stroke_uv_size = param;
			} else if (strcmp(info.name, "offset") == 0) {
				filter->param_jump_flood_offset = param;
			} else if (strcmp(info.name, "stroke_offset") == 0) {
				filter->param_jump_flood_stroke_offset = param;
			} else if (strcmp(info.name, "stroke_extent") == 0) {
				filter->param_jump_flood_stroke_extent = param;
			} else if (strcmp(info.name, "overlay") == 0) {
				filter->param_jump_flood_overlay = param;
			} else if (strcmp(info.name, "inner_distance_field") == 0){
				filter->param_inner_distance_field = param;
			} else if (strcmp(info.name, "outer_distance_field") == 0){
				filter->param_outer_distance_field = param;
			} else if (strcmp(info.name, "stroke_fill_color") == 0) {
				filter->param_jump_flood_stroke_color = param;
			} else if (strcmp(info.name, "stroke_fill_source") == 0) {
				filter->param_jump_flood_fill_source = param;
			} else if (strcmp(info.name, "contour_offset") == 0) {
				filter->param_jump_flood_contour_offset = param;
			} else if (strcmp(info.name, "contour_spacing") == 0) {
				filter->param_jump_flood_contour_spacing = param;
			} else if (strcmp(info.name, "contour_falloff_start") == 0) {
				filter->param_jump_flood_contour_falloff_start = param;
			} else if (strcmp(info.name, "contour_falloff_end") == 0) {
				filter->param_jump_flood_contour_falloff_end = param;
			} else if (strcmp(info.name, "contour_spacing_power") == 0) {
				filter->param_jump_flood_contour_spacing_power = param;
			} else if (strcmp(info.name, "mul_val") == 0) {
				filter->param_mul_val = param;
			} else if (strcmp(info.name, "add_val") == 0) {
				filter->param_add_val = param;
			} else if (strcmp(info.name, "infill") == 0) {
				filter->param_infill = param;
			}
		}
	}
}

void load_output_effect(stroke_filter_data_t *filter)
{
	const char *effect_file_path = "/shaders/render_output.effect";
	filter->effect_output =
		load_shader_effect(filter->effect_output, effect_file_path);
	if (filter->effect_output) {
		size_t effect_count =
			gs_effect_get_num_params(filter->effect_output);
		for (size_t effect_index = 0; effect_index < effect_count;
		     effect_index++) {
			gs_eparam_t *param = gs_effect_get_param_by_idx(
				filter->effect_output, effect_index);
			struct gs_effect_param_info info;
			gs_effect_get_param_info(param, &info);
			if (strcmp(info.name, "output_image") == 0) {
				filter->param_output_image = param;
			}
		}
	}
}

void load_stroke_inner_effect(stroke_filter_data_t *filter)
{
	const char *effect_file_path = "/shaders/stroke_inner.effect";
	filter->effect_stroke_inner = load_shader_effect(
		filter->effect_stroke_inner, effect_file_path);
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
				filter->param_stroke_inner_stroke_thickness =
					param;
			} else if (strcmp(info.name, "stroke_offset") == 0) {
				filter->param_stroke_inner_offset = param;
			}
		}
	}
}

void load_fill_stroke_effect(stroke_filter_data_t *filter)
{
	const char *effect_file_path =
		filter->is_filter ? "/shaders/fill_stroke.effect"
				  : "/shaders/fill_stroke_source.effect";
	filter->effect_fill_stroke = load_shader_effect(
		filter->effect_fill_stroke, effect_file_path);
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
			} else if (strcmp(info.name, "stroke_mask") == 0) {
				filter->param_fill_stroke_stroke_mask = param;
			} else if (strcmp(info.name, "stroke_fill_source") ==
				   0) {
				filter->param_fill_stroke_fill_source = param;
			} else if (strcmp(info.name, "stroke_fill_color") ==
				   0) {
				filter->param_fill_stroke_fill_color = param;
			} else if (strcmp(info.name, "fill_behind") == 0) {
				filter->param_fill_stroke_fill_behind = param;
			}
		}
	}
}

void render_padded_input(stroke_filter_data_t* data)
{
	gs_effect_t* effect = data->effect_jump_flood_sdf;
	if (!effect) {
		return;
	}
	if (data->is_source) {
		obs_source_t* input_source = data->source_input_source
			? obs_weak_source_get_source(
				data->source_input_source)
			: NULL;
		if (!input_source) {
			data->input_texture_generated = false;
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
		const enum gs_color_format format =
			gs_get_format_from_space(space);

		// Set up a tex renderer for source
		gs_texrender_t* source_render = gs_texrender_create(format, GS_ZS_NONE);
		uint32_t source_width = obs_source_get_width(input_source);
		uint32_t source_height = obs_source_get_height(input_source);
		data->source_width = source_width;
		data->source_height = source_height;
		gs_blend_state_push();
		gs_blend_function(GS_BLEND_ONE, GS_BLEND_ZERO);
		if (gs_texrender_begin_with_color_space(
			source_render, source_width, source_height, space)) {
			const float w = (float)source_width;
			const float h = (float)source_height;
			struct vec4 clear_color;

			vec4_zero(&clear_color);
			gs_clear(GS_CLEAR_COLOR, &clear_color, 0.0f, 0);
			gs_ortho(0.0f, w, 0.0f, h, -100.0f, 100.0f);
			obs_source_video_render(input_source);
			gs_texrender_end(source_render);
		}
		gs_blend_state_pop();
		obs_source_release(input_source);
		gs_texture_t* texture = gs_texrender_get_texture(source_render);

		// Set up our input_texrender to catch the output texture.
		data->input_texrender =
			create_or_reset_texrender(data->input_texrender);
		data->input_texture_generated = false;
		// Start the rendering process with our correct color space params,
		// And set up your texrender to recieve the created texture.

		data->width = source_width + data->pad_l + data->pad_r;
		data->height = source_height + data->pad_t + data->pad_b;

		data->mul_val.x = (float)data->width / (float)source_width;
		data->add_val.x = -(float)data->pad_l / (float)source_width;

		data->mul_val.y = (float)data->height / (float)source_height;
		data->add_val.y = -(float)data->pad_t / (float)source_height;

		// Grab the proper padding/
		if (data->param_mul_val) {
			gs_effect_set_vec2(data->param_mul_val, &data->mul_val);
		}
		if (data->param_add_val) {
			gs_effect_set_vec2(data->param_add_val, &data->add_val);
		}
		gs_eparam_t* param =
			gs_effect_get_param_by_name(effect, "image");
		gs_effect_set_texture(param, texture);

		if (gs_texrender_begin_with_color_space(data->input_texrender,
			data->width, data->height,
			space)) {

			gs_blend_state_push();
			gs_reset_blend_state();
			gs_enable_blending(false);
			gs_blend_function(GS_BLEND_ONE, GS_BLEND_ZERO);

			gs_ortho(0.0f, (float)data->width, 0.0f,
				(float)data->height, -100.0f, 100.0f);
			const char* technique = "DrawCropPad";
			//obs_source_process_filter_tech_end(
			//	input_source, effect, data->width,
			//	data->height, technique);
			while (gs_effect_loop(effect, technique))
				gs_draw_sprite(texture, 0, data->width, data->height);
			gs_texrender_end(data->input_texrender);
			gs_blend_state_pop();
			data->input_texture_generated = true;
		}
		//gs_texture_destroy(texture);
		gs_texrender_destroy(source_render);
	} else {
		obs_source_t* input_source = data->context;

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
		data->input_texrender =
			create_or_reset_texrender(data->input_texrender);
		data->input_texture_generated = false;

		// Start the rendering process with our correct color space params,
		// And set up your texrender to recieve the created texture.
		if (!obs_source_process_filter_begin_with_color_space(
			input_source, format, source_space,
			OBS_NO_DIRECT_RENDERING))
			return;

		data->width = data->source_width + data->pad_l + data->pad_r;
		data->height = data->source_height + data->pad_t + data->pad_b;

		data->mul_val.x = (float)data->width / (float)data->source_width;
		data->add_val.x = -(float)data->pad_l / (float)data->source_width;

		data->mul_val.y = (float)data->height / (float)data->source_height;
		data->add_val.y = -(float)data->pad_t / (float)data->source_height;

		// Grab the proper padding/
		if (data->param_mul_val) {
			gs_effect_set_vec2(data->param_mul_val, &data->mul_val);
		}
		if (data->param_add_val) {
			gs_effect_set_vec2(data->param_add_val, &data->add_val);
		}

		if (gs_texrender_begin(data->input_texrender, data->width,
			data->height)) {

			gs_blend_state_push();
			gs_reset_blend_state();
			gs_enable_blending(false);
			gs_blend_function(GS_BLEND_ONE, GS_BLEND_ZERO);

			gs_ortho(0.0f, (float)data->width, 0.0f,
				(float)data->height, -100.0f, 100.0f);
			const char* technique = "DrawCropPad";
			obs_source_process_filter_tech_end(
				input_source, effect, data->width,
				data->height, technique);
			gs_texrender_end(data->input_texrender);
			gs_blend_state_pop();
			data->input_texture_generated = true;
		}
	}
}

void render_cropped_output(stroke_filter_data_t* data)
{
	gs_effect_t* effect = data->effect_jump_flood_sdf;
	if (data->is_source) {
		gs_texture_t* texture =
			gs_texrender_get_texture(data->output_texrender);
		gs_eparam_t* param =
			gs_effect_get_param_by_name(effect, "output_image");
		gs_effect_set_texture(param, texture);

		int base_width = data->width;
		int base_height = data->height;

		const bool previous = gs_framebuffer_srgb_enabled();
		if (data->fill_type == STROKE_FILL_TYPE_COLOR) {
			const bool linear_srgb = gs_get_linear_srgb() || data->stroke_color.w < 1.0f;
			gs_enable_framebuffer_srgb(linear_srgb);
		}

		if (data->stroke_position == STROKE_POSITION_INNER || data->stroke_position == STROKE_POSITION_INNER_CONTOUR) {
			data->width = data->width - data->pad_l - data->pad_r;
			data->height = data->height - data->pad_t - data->pad_b;

			data->mul_val.x = (float)data->width / (float)base_width;
			data->add_val.x = (float)data->pad_l / (float)base_width;

			data->mul_val.y = (float)data->height / (float)base_height;
			data->add_val.y = (float)data->pad_t / (float)base_height;
		}
		else {
			data->mul_val.x = 1.0;
			data->mul_val.y = 1.0;
			data->add_val.x = 0.0;
			data->add_val.y = 0.0;
		}

		// Grab the proper padding/
		if (data->param_mul_val) {
			gs_effect_set_vec2(data->param_mul_val, &data->mul_val);
		}
		if (data->param_add_val) {
			gs_effect_set_vec2(data->param_add_val, &data->add_val);
		}

		//uint32_t width = gs_texture_get_width(texture);
		//uint32_t height = gs_texture_get_height(texture);

		//while (gs_effect_loop(effect, "DrawOutput")) {
		//	gs_draw_sprite(texture, 0, data->width, data->height);
		//}
		gs_technique_t* tech = gs_effect_get_technique(effect, "DrawOutput");
		gs_technique_begin(tech);
		gs_technique_begin_pass(tech, 0);

		gs_draw_sprite(0, 0, data->width, data->height);

		gs_technique_end_pass(tech);
		gs_technique_end(tech);

		gs_enable_framebuffer_srgb(previous);
	} else {
		const enum gs_color_space preferred_spaces[] = {
		GS_CS_SRGB,
		GS_CS_SRGB_16F,
		GS_CS_709_EXTENDED,
		};

		const enum gs_color_space source_space =
			obs_source_get_color_space(
				obs_filter_get_target(data->context),
				OBS_COUNTOF(preferred_spaces),
				preferred_spaces);

		const enum gs_color_format format =
			gs_get_format_from_space(source_space);

		if (!obs_source_process_filter_begin_with_color_space(
			data->context, format, source_space,
			OBS_NO_DIRECT_RENDERING)) {
			return;
		}

		gs_texture_t* texture =
			gs_texrender_get_texture(data->output_texrender);

		gs_eparam_t* image = gs_effect_get_param_by_name(effect, "output_image");
		gs_effect_set_texture(image, texture);

		int base_width = data->width;
		int base_height = data->height;

		if (data->stroke_position == STROKE_POSITION_INNER || data->stroke_position == STROKE_POSITION_INNER_CONTOUR) {
			data->width = data->width - data->pad_l - data->pad_r;
			data->height = data->height - data->pad_t - data->pad_b;

			data->mul_val.x = (float)data->width / (float)base_width;
			data->add_val.x = (float)data->pad_l / (float)base_width;

			data->mul_val.y = (float)data->height / (float)base_height;
			data->add_val.y = (float)data->pad_t / (float)base_height;
		}
		else {
			data->mul_val.x = 1.0;
			data->mul_val.y = 1.0;
			data->add_val.x = 0.0;
			data->add_val.y = 0.0;
		}

		// Grab the proper padding/
		if (data->param_mul_val) {
			gs_effect_set_vec2(data->param_mul_val, &data->mul_val);
		}
		if (data->param_add_val) {
			gs_effect_set_vec2(data->param_add_val, &data->add_val);
		}

		const char* technique = "DrawOutput";
		obs_source_process_filter_tech_end(data->context, effect,
			data->width, data->height, technique);
	}
}

void render_jf_passes_outer(stroke_filter_data_t* data, float maxExtent)
{
	gs_effect_t* effect = data->effect_jump_flood_sdf;

	// Swap texrender between buffer a and ..
	gs_texrender_t* tmp = data->buffer_outer_threshold;
	data->buffer_outer_threshold = data->buffer_a;
	data->buffer_a = tmp;

	if (!effect) {
		return;
	}
	//bool contour = data->stroke_position == STROKE_POSITION_OUTER_CONTOUR || data->stroke_position == STROKE_POSITION_INNER_CONTOUR;
	//float maxExtent = contour ? fmaxf((float)data->width, (float)data->height) : data->stroke_offset + data->stroke_size;
	int iExtent = (int)maxExtent + 1;

	int maxPass = 1;
	while (maxPass < iExtent) {
		maxPass *= 2;
	}
	//maxPass = maxPass *= 2;
	int pass = maxPass;

	while (pass > 0) {
		gs_texture_t* input_texture = gs_texrender_get_texture(data->buffer_a);
		if (!input_texture) {
			return;
		}

		data->buffer_b = create_or_reset_texrender_high(data->buffer_b);

		gs_eparam_t* image = gs_effect_get_param_by_name(effect, "image");
		gs_effect_set_texture(image, input_texture);

		if (data->param_jump_flood_offset) {
			//float fPass = pass != maxPass ? (float)pass : 1.0f;
			float fPass = (float)pass;
			gs_effect_set_float(data->param_jump_flood_offset, fPass);
		}

		if (data->param_stroke_uv_size) {
			struct vec2 uv_size;
			uv_size.x = (float)data->width;
			uv_size.y = (float)data->height;
			gs_effect_set_vec2(data->param_stroke_uv_size, &uv_size);
		}

		set_blending_parameters();

		if (gs_texrender_begin(data->buffer_b, data->width, data->height)) {
			gs_ortho(0.0f, (float)data->width, 0.0f, (float)data->height,
				-100.0f, 100.0f);
			while (gs_effect_loop(effect, "DrawJumpFloodStep"))
				gs_draw_sprite(input_texture, 0, data->width,
					data->height);
			gs_texrender_end(data->buffer_b);
		}
		gs_blend_state_pop();

		tmp = data->buffer_a;
		data->buffer_a = data->buffer_b;
		data->buffer_b = tmp;
		pass /= 2;
	}

	tmp = data->buffer_a;
	data->buffer_a = data->buffer_outer_distance_field;
	data->buffer_outer_distance_field = tmp;
}

void render_jf_passes_inner(stroke_filter_data_t* data, float maxExtent)
{
	gs_effect_t* effect = data->effect_jump_flood_sdf;

	//gs_texture_t* input_texture = gs_texrender_get_texture(data->stroke_mask);

	if (!effect) {
		return;
	}

	gs_texrender_t* tmp = data->buffer_inner_threshold;
	data->buffer_inner_threshold = data->buffer_a;
	data->buffer_a = tmp;

	//bool contour = data->stroke_position == STROKE_POSITION_OUTER_CONTOUR || data->stroke_position == STROKE_POSITION_INNER_CONTOUR;
	//float extent = contour ? fmaxf((float)data->width, (float)data->height) : data->stroke_offset + data->stroke_size;
	int iExtent = (int)maxExtent + 1;

	int maxPass = 1;
	while (maxPass < iExtent) {
		maxPass *= 2;
	}
	//maxPass = maxPass *= 2;
	int pass = maxPass;

	while (pass > 0) {
		gs_texture_t* input_texture = gs_texrender_get_texture(data->buffer_a);
		if (!input_texture) {
			return;
		}

		data->buffer_b = create_or_reset_texrender_high(data->buffer_b);

		gs_eparam_t* image = gs_effect_get_param_by_name(effect, "image");
		gs_effect_set_texture(image, input_texture);

		if (data->param_jump_flood_offset) {
			//float fPass = pass != maxPass ? (float)pass : 1.0f;
			float fPass = (float)pass;
			gs_effect_set_float(data->param_jump_flood_offset, fPass);
		}

		if (data->param_stroke_uv_size) {
			struct vec2 uv_size;
			uv_size.x = (float)data->width;
			uv_size.y = (float)data->height;
			gs_effect_set_vec2(data->param_stroke_uv_size, &uv_size);
		}

		set_blending_parameters();

		if (gs_texrender_begin(data->buffer_b, data->width, data->height)) {
			gs_ortho(0.0f, (float)data->width, 0.0f, (float)data->height,
				-100.0f, 100.0f);
			while (gs_effect_loop(effect, "DrawJumpFloodStep"))
				gs_draw_sprite(input_texture, 0, data->width,
					data->height);
			gs_texrender_end(data->buffer_b);
		}
		gs_blend_state_pop();

		tmp = data->buffer_a;
		data->buffer_a = data->buffer_b;
		data->buffer_b = tmp;
		pass /= 2;
	}

	tmp = data->buffer_a;
	data->buffer_a = data->buffer_inner_distance_field;
	data->buffer_inner_distance_field = tmp;
}

void render_jf_outer_threshold(stroke_filter_data_t* data)
{
	gs_effect_t* effect = data->effect_jump_flood_sdf;
	gs_texture_t *input_texture = gs_texrender_get_texture(data->input_texrender);

	if (!effect || !input_texture) {
		return;
	}

	data->buffer_outer_threshold = create_or_reset_texrender_high(data->buffer_outer_threshold);

	gs_eparam_t *image = gs_effect_get_param_by_name(effect, "image");
	gs_effect_set_texture(image, input_texture);

	if (data->param_jump_flood_threshold) {
		gs_effect_set_float(data->param_jump_flood_threshold,
				    data->jump_flood_threshold);
	}

	if (data->param_stroke_uv_size) {
		struct vec2 uv_size;
		uv_size.x = (float)data->width;
		uv_size.y = (float)data->height;
		gs_effect_set_vec2(data->param_stroke_uv_size, &uv_size);
	}

	set_blending_parameters();

	const char* technique = "DrawThreshold";

	if (gs_texrender_begin(data->buffer_outer_threshold, data->width, data->height)) {
		gs_ortho(0.0f, (float)data->width, 0.0f, (float)data->height,
			 -100.0f, 100.0f);
		while (gs_effect_loop(effect, technique))
			gs_draw_sprite(input_texture, 0, data->width,
				       data->height);
		gs_texrender_end(data->buffer_outer_threshold);
	}
	gs_blend_state_pop();
}

void render_jf_inner_threshold(stroke_filter_data_t* data)
{
	gs_effect_t* effect = data->effect_jump_flood_sdf;
	gs_texture_t* input_texture = gs_texrender_get_texture(data->input_texrender);

	if (!effect || !input_texture) {
		return;
	}

	data->buffer_inner_threshold = create_or_reset_texrender_high(data->buffer_inner_threshold);

	gs_eparam_t* image = gs_effect_get_param_by_name(effect, "image");
	gs_effect_set_texture(image, input_texture);

	if (data->param_jump_flood_threshold) {
		gs_effect_set_float(data->param_jump_flood_threshold,
			data->jump_flood_threshold);
	}

	if (data->param_stroke_uv_size) {
		struct vec2 uv_size;
		uv_size.x = (float)data->width;
		uv_size.y = (float)data->height;
		gs_effect_set_vec2(data->param_stroke_uv_size, &uv_size);
	}

	set_blending_parameters();

	const char* technique = "DrawThresholdInner";

	if (gs_texrender_begin(data->buffer_inner_threshold, data->width, data->height)) {
		gs_ortho(0.0f, (float)data->width, 0.0f, (float)data->height,
			-100.0f, 100.0f);
		while (gs_effect_loop(effect, technique))
			gs_draw_sprite(input_texture, 0, data->width,
				data->height);
		gs_texrender_end(data->buffer_inner_threshold);
	}
	gs_blend_state_pop();
}

void render_jf_distance(stroke_filter_data_t* data)
{
	gs_effect_t* effect = data->effect_jump_flood_sdf;

	gs_texture_t* overlay_texture = gs_texrender_get_texture(data->input_texrender);

	gs_texture_t* odf_texture = gs_texrender_get_texture(data->buffer_outer_distance_field);
	gs_texture_t* idf_texture = gs_texrender_get_texture(data->buffer_inner_distance_field);

	bool contours = data->stroke_position == STROKE_POSITION_INNER_CONTOUR || data->stroke_position == STROKE_POSITION_OUTER_CONTOUR;

	if (!effect || !overlay_texture || (!idf_texture && !odf_texture)) {
		return;
	}

	// Add logic here to check srgb of source stroke
	const bool linear_srgb = gs_get_linear_srgb() || data->stroke_color.w < 1.0f;

	data->output_texrender = create_or_reset_texrender(data->output_texrender);

	if (odf_texture && data->param_outer_distance_field) {
		gs_effect_set_texture(data->param_outer_distance_field, odf_texture);
	}

	if (idf_texture && data->param_inner_distance_field) {
		gs_effect_set_texture(data->param_inner_distance_field, idf_texture);
	}

	if (data->param_jump_flood_overlay) {
		gs_effect_set_texture(data->param_jump_flood_overlay, overlay_texture);
	}

	if (data->param_jump_flood_threshold) {
		gs_effect_set_float(data->param_jump_flood_threshold,
			data->jump_flood_threshold);
	}

	if (data->param_jump_flood_stroke_offset) {
		//float offset = max(data->jump_flood_threshold/2.0f, data->stroke_offset);
		float offset = data->stroke_offset;
		gs_effect_set_float(data->param_jump_flood_stroke_offset, offset);
	}

	if (data->param_jump_flood_contour_offset) {
		float offset_pct = fmodf(-data->contour_offset, 100.0f) / 100.01f;
		offset_pct = offset_pct >= 0.0f ? offset_pct : 1.0f + offset_pct;
		float offset = offset_pct * (data->contour_spacing + data->stroke_size);
		gs_effect_set_float(data->param_jump_flood_contour_offset, offset);
	}

	if (data->param_jump_flood_contour_spacing) {
		gs_effect_set_float(data->param_jump_flood_contour_spacing, data->contour_spacing);
	}

	if (data->param_jump_flood_stroke_extent) {
		float stroke_extent = contours ? data->stroke_size : data->stroke_offset + data->stroke_size;
		gs_effect_set_float(data->param_jump_flood_stroke_extent, stroke_extent);
	}
	if (data->param_jump_flood_contour_falloff_start) {
		gs_effect_set_float(data->param_jump_flood_contour_falloff_start, data->contour_falloff_start);
	}
	if (data->param_jump_flood_contour_falloff_end) {
		gs_effect_set_float(data->param_jump_flood_contour_falloff_end, data->contour_falloff_end);
	}
	if (data->param_jump_flood_contour_spacing_power) {
		gs_effect_set_float(data->param_jump_flood_contour_spacing_power, data->contour_spacing_power);
	}
	if (data->param_infill) {
		float fill = data->fill ? 1.0f : 0.0f;
		gs_effect_set_float(data->param_infill, fill);
	}


	bool source_available = (data->fill_type == STROKE_FILL_TYPE_SOURCE) &&
		data->fill_source_source;

	gs_texrender_t* source_render = NULL;
	if (data->fill_type == STROKE_FILL_TYPE_SOURCE && source_available) {
		obs_source_t* source =
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
			struct vec4 clear_color;

			vec4_zero(&clear_color);
			gs_clear(GS_CLEAR_COLOR, &clear_color, 0.0f, 0);
			gs_ortho(0.0f, w, 0.0f, h, -100.0f, 100.0f);

			obs_source_video_render(source);
			gs_texrender_end(source_render);
		}
		gs_blend_state_pop();
		obs_source_release(source);
		gs_texture_t* source_texture =
			gs_texrender_get_texture(source_render);
		if (data->param_jump_flood_fill_source) {
			gs_effect_set_texture(
				data->param_jump_flood_fill_source,
				source_texture);
		}
	} else if (data->fill_type == STROKE_FILL_TYPE_SOURCE && !source_available) {
		if (data->param_jump_flood_stroke_color) {
			struct vec4 clear_color;
			vec4_zero(&clear_color);
			gs_effect_set_vec4(data->param_jump_flood_stroke_color, &clear_color);
		}
	} else if (data->fill_type == STROKE_FILL_TYPE_COLOR) {
		if (data->param_jump_flood_stroke_color) {
			gs_effect_set_vec4(data->param_jump_flood_stroke_color,
				linear_srgb ? &data->stroke_color_srgb : &data->stroke_color);
		}
	}

	bool fill_color = (data->fill_type == STROKE_FILL_TYPE_COLOR) ||
		(data->fill_type == STROKE_FILL_TYPE_SOURCE &&
			!source_available);

	if (data->param_stroke_uv_size) {
		struct vec2 uv_size;
		uv_size.x = (float)data->width;
		uv_size.y = (float)data->height;
		gs_effect_set_vec2(data->param_stroke_uv_size, &uv_size);
	}

	set_blending_parameters();

	const char* render = data->stroke_position == STROKE_POSITION_OUTER || data->stroke_position == STROKE_POSITION_INNER ? "Stroke" : "Contour";
	const char* innieOutie = data->stroke_position == STROKE_POSITION_OUTER || data->stroke_position == STROKE_POSITION_OUTER_CONTOUR ? "" : "Inner";

	const char* fill_type =
		data->fill_type == fill_color ? "Color"
		: data->fill_type == STROKE_FILL_TYPE_SOURCE ? "Source"
		: "Source";

	char shader_id[100] = "Draw";
	strncat(shader_id, render, strlen(render));
	strncat(shader_id, innieOutie, strlen(innieOutie));
	strncat(shader_id, fill_type, strlen(fill_type));

	if (gs_texrender_begin(data->output_texrender, data->width, data->height)) {
		gs_ortho(0.0f, (float)data->width, 0.0f, (float)data->height,
			-100.0f, 100.0f);
		while (gs_effect_loop(effect, shader_id))
			gs_draw_sprite(odf_texture, 0, data->width,
				data->height);
		gs_texrender_end(data->output_texrender);
	}
	gs_blend_state_pop();

	if (source_render) {
		gs_texrender_destroy(source_render);
	}
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

	data->output_texrender =
		create_or_reset_texrender(data->output_texrender);

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

	bool source_available = (data->fill_type == STROKE_FILL_TYPE_SOURCE) &&
				data->fill_source_source;

	gs_texrender_t *source_render = NULL;
	if (data->fill_type == STROKE_FILL_TYPE_SOURCE && source_available) {
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
			struct vec4 clear_color;

			vec4_zero(&clear_color);
			gs_clear(GS_CLEAR_COLOR, &clear_color, 0.0f, 0);
			gs_ortho(0.0f, w, 0.0f, h, -100.0f, 100.0f);

			obs_source_video_render(source);
			gs_texrender_end(source_render);
		}
		gs_blend_state_pop();
		obs_source_release(source);
		gs_texture_t *source_texture =
			gs_texrender_get_texture(source_render);
		if (data->param_fill_stroke_fill_source) {
			gs_effect_set_texture(
				data->param_fill_stroke_fill_source,
				source_texture);
		}
	} else if(data->fill_type == STROKE_FILL_TYPE_SOURCE && !source_available) {
		if(data->param_fill_stroke_fill_color) {
			struct vec4 clear_color;
			vec4_zero(&clear_color);
			gs_effect_set_vec4(data->param_fill_stroke_fill_color, &clear_color);
		}
	} else if (data->fill_type == STROKE_FILL_TYPE_COLOR) {
		if (data->param_fill_stroke_fill_color) {
			gs_effect_set_vec4(data->param_fill_stroke_fill_color,
					   &data->stroke_color);
		}
	}

	bool fill_color = (data->fill_type == STROKE_FILL_TYPE_COLOR) ||
			  (data->fill_type == STROKE_FILL_TYPE_SOURCE &&
			   !source_available);
	const char *fill_type =
		data->fill_type ==  fill_color ? "FilterColor"
		: data->fill_type == STROKE_FILL_TYPE_SOURCE ? "FilterSource"
							     : "FilterSource";

	const char *position = data->stroke_position == STROKE_POSITION_OUTER
				       ? "Outer"
				       : "Inner";

	char shader_id[100] = "";
	snprintf(shader_id, sizeof(shader_id), "%s%s", fill_type, position);

	set_blending_parameters();

	if (gs_texrender_begin(data->output_texrender, data->width,
			       data->height)) {
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
