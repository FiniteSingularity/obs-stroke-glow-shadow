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
			}
	 	}
	 }
}

void render_glow_filter(glow_filter_data_t *data)
{
	gs_effect_t *effect = data->effect_glow;

	gs_texture_t *blur_mask_texture = gs_texrender_get_texture(data->alpha_blur_data->alpha_blur_output);
	gs_texture_t *input_texture = gs_texrender_get_texture(data->input_texrender);

	if (!effect || !input_texture || !blur_mask_texture) {
		return;
	}

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
				    data->fill ? 0.0f : 1.0f);
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
		&data->glow_color);
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
	strcat(shader_id, position);
	strcat(shader_id, fill_type);

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
