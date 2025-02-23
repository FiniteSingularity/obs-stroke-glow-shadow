#include "glow.h"
#include <string.h>

void load_glow_effect(glow_filter_data_t *filter)
{
	const char *effect_file_path = filter->is_filter
					       ? "/shaders/glow.effect"
					       : "/shaders/glow_source.effect";
	filter->effect_glow =
	 	load_shader_effect(filter->effect_glow, effect_file_path);
	 if (filter->effect_glow) {
	 	size_t effect_count =
	 		gs_effect_get_num_params(filter->effect_glow);
	 	for (size_t effect_index = 0; effect_index < effect_count;
	 	     effect_index++) {
	 		gs_eparam_t *param = gs_effect_get_param_by_idx(
	 			filter->effect_glow, effect_index);
	 		struct gs_effect_param_info info;
	 		gs_effect_get_param_info(param, &info);
	 		if (strcmp(info.name, "input_image") == 0) {
	 			filter->param_glow_image = param;
			} else if (strcmp(info.name, "glow_mask") == 0) {
				filter->param_glow_mask = param;
			} else if (strcmp(info.name, "glow_fill_source") == 0) {
				filter->param_glow_fill_source = param;
			} else if (strcmp(info.name, "glow_fill_color") == 0) {
				filter->param_glow_fill_color = param;
			} else if (strcmp(info.name, "intensity") == 0) {
				filter->param_glow_intensity = param;
			} else if (strcmp(info.name, "offset") == 0) {
				filter->param_offset_texel = param;
			} else if (strcmp(info.name, "fill_behind") == 0) {
				filter->param_glow_fill_behind = param;
			} else if (strcmp(info.name, "fill_behind") == 0) {
				filter->param_glow_fill_behind = param;
			} else if (strcmp(info.name, "mul_val") == 0) {
				filter->param_mul_val = param;
			} else if (strcmp(info.name, "add_val") == 0) {
				filter->param_add_val = param;
			} else if (strcmp(info.name, "threshold") == 0) {
				filter->param_threshold = param;
			}
	 	}
	 }
}

void load_glow_output_effect(glow_filter_data_t *filter)
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


void glow_render_padded_input(glow_filter_data_t* data)
{

	gs_effect_t* effect = data->effect_glow;
	if (!effect) {
		return;
	}

	if (data->is_source) { // this is for the source version.
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
		gs_texrender_t *source_render = gs_texrender_create(format, GS_ZS_NONE);
		uint32_t source_width = obs_source_get_width(input_source);
		uint32_t source_height = obs_source_get_height(input_source);
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

		// This is for the filter version
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

void glow_render_cropped_output(glow_filter_data_t* data)
{
	gs_effect_t* effect = data->effect_glow;
	if (data->is_source) {
		//gs_texture_t* texture =
		//	gs_texrender_get_texture(data->output_texrender);
		//gs_effect_t* pass_through =
		//	obs_get_base_effect(OBS_EFFECT_DEFAULT);
		//gs_eparam_t* param =
		//	gs_effect_get_param_by_name(pass_through, "image");
		//gs_effect_set_texture(param, texture);
		//uint32_t width = gs_texture_get_width(texture);
		//uint32_t height = gs_texture_get_height(texture);
		//while (gs_effect_loop(pass_through, "Draw")) {
		//	gs_draw_sprite(texture, 0, width, height);
		//}
		gs_texture_t* texture =
			gs_texrender_get_texture(data->output_texrender);
		gs_eparam_t* param =
			gs_effect_get_param_by_name(effect, "output_image");
		gs_effect_set_texture(param, texture);

		int base_width = data->width;
		int base_height = data->height;

		const bool previous = gs_framebuffer_srgb_enabled();
		if (data->fill_type == GLOW_FILL_TYPE_COLOR) {
			const bool linear_srgb = gs_get_linear_srgb() || data->glow_color.w < 1.0f;
			gs_enable_framebuffer_srgb(linear_srgb);
		}

		if (data->glow_position == GLOW_POSITION_INNER) {
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
	}
	else {

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
			OBS_ALLOW_DIRECT_RENDERING)) {
			return;
		}

		gs_texture_t* texture =
			gs_texrender_get_texture(data->output_texrender);

		gs_eparam_t* image = gs_effect_get_param_by_name(effect, "output_image");
		gs_effect_set_texture(image, texture);

		int output_width = data->source_width + data->pad_l + data->pad_r;
		int output_height = data->source_height + data->pad_t + data->pad_b;

		data->mul_val.x = 1.0;
		data->mul_val.y = 1.0;
		data->add_val.x = 0.0;
		data->add_val.y = 0.0;

		// Grab the proper padding/
		if (data->param_mul_val) {
			gs_effect_set_vec2(data->param_mul_val, &data->mul_val);
		}
		if (data->param_add_val) {
			gs_effect_set_vec2(data->param_add_val, &data->add_val);
		}

		const char* technique = "DrawOutput";
		obs_source_process_filter_tech_end(data->context, effect,
			output_width, output_height, technique);
	}
}

void render_glow_alpha_mask(glow_filter_data_t* data)
{
	gs_effect_t* effect = data->effect_glow;
	gs_texture_t* input_texture = gs_texrender_get_texture(data->input_texrender);

	gs_eparam_t* image = gs_effect_get_param_by_name(effect, "image");
	gs_effect_set_texture(image, input_texture);

	if (data->param_threshold) {
		gs_effect_set_float(data->param_threshold,
			data->threshold);
	}

	data->alpha_mask_texrender =
		create_or_reset_texrender(data->alpha_mask_texrender);

	set_blending_parameters();

	if (gs_texrender_begin(data->alpha_mask_texrender, data->width, data->height)) {
		gs_ortho(0.0f, (float)data->width, 0.0f, (float)data->height,
			-100.0f, 100.0f);
		while (gs_effect_loop(effect, "ThresholdMask"))
			gs_draw_sprite(NULL, 0, data->width,
				data->height);
		gs_texrender_end(data->alpha_mask_texrender);
	}

	gs_blend_state_pop();
}

void render_glow_filter(glow_filter_data_t *data)
{
	gs_effect_t *effect = data->effect_glow;

	gs_texture_t *blur_mask_texture = gs_texrender_get_texture(data->alpha_blur_data->alpha_blur_output);
	gs_texture_t *input_texture = gs_texrender_get_texture(data->input_texrender);

	if (!effect || !input_texture || !blur_mask_texture) {
		return;
	}

	const bool linear_srgb = gs_get_linear_srgb() || data->glow_color.w < 1.0f;

	gs_eparam_t *image = gs_effect_get_param_by_name(effect, "image");
	gs_effect_set_texture(image, input_texture);

	gs_eparam_t *glow_mask = gs_effect_get_param_by_name(effect, "glow_mask");
	gs_effect_set_texture(glow_mask, blur_mask_texture);

	struct vec2 scaled_texel;
	scaled_texel.x = data->offset_texel.x / data->width;
	scaled_texel.y = data->offset_texel.y / data->height;

	if (data->param_offset_texel) {
		gs_effect_set_vec2(data->param_offset_texel,
				   &scaled_texel);
	}

	if (data->param_glow_intensity) {
		gs_effect_set_float(data->param_glow_intensity,
				    data->intensity);
	}

	if (data->param_glow_fill_behind) {
		gs_effect_set_float(data->param_glow_fill_behind,
				    data->fill ? 1.0f : 0.0f);
	}
	if (data->param_threshold) {
		gs_effect_set_float(data->param_threshold,
			data->threshold);
	}

	bool source_available = (data->fill_type == GLOW_FILL_TYPE_SOURCE) &&
				data->fill_source_source;

	gs_texrender_t *source_render = NULL;
	if (source_available) {
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
	 	gs_texture_t *source_texture = gs_texrender_get_texture(source_render);
	 	if (data->param_glow_fill_source) {
	 		gs_effect_set_texture(
	 			data->param_glow_fill_source,
	 				      source_texture);
	 	}
	} else if(data->fill_type == GLOW_FILL_TYPE_SOURCE && !source_available) {
		if(data->param_glow_fill_color) {
			struct vec4 clear_color;
			vec4_zero(&clear_color);
			gs_effect_set_vec4(data->param_glow_fill_color, &clear_color);
		}
	} else if (data->fill_type == GLOW_FILL_TYPE_COLOR &&
	    data->param_glow_fill_color) {
		gs_effect_set_vec4(data->param_glow_fill_color,
			linear_srgb ? &data->glow_color_srgb : &data->glow_color);
	}

	data->output_texrender =
		create_or_reset_texrender(data->output_texrender);

	bool fill_color =
		(data->fill_type == GLOW_FILL_TYPE_COLOR) ||
		(data->fill_type == GLOW_FILL_TYPE_SOURCE && !source_available);
	const char *fill_type =
		fill_color ? "Color"
		: data->fill_type == GLOW_FILL_TYPE_SOURCE ? "Source"
							     : "Source";
	const char *position = data->glow_position == GLOW_POSITION_OUTER
				       ? "FilterOuterGlow"
				       : "FilterInnerGlow";

	char shader_id[100] = "";
	strncat(shader_id, position, strlen(position));
	strncat(shader_id, fill_type, strlen(fill_type));

	set_blending_parameters();

	if (gs_texrender_begin(data->output_texrender, data->width, data->height)) {
		gs_ortho(0.0f, (float)data->width, 0.0f, (float)data->height,
	 		 -100.0f, 100.0f);
	 	while (gs_effect_loop(effect, shader_id))
	 		gs_draw_sprite(NULL, 0, data->width,
	 			       data->height);
	 	gs_texrender_end(data->output_texrender);
	}

	gs_blend_state_pop();

	if (source_render) {
		gs_texrender_destroy(source_render);
	}
}
