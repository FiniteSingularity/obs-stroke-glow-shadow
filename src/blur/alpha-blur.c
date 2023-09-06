#include "alpha-blur.h"

/*
 *  Performs an area blur using the conical kernel.  Blur is
 *  equal in both x and y directions.
 */
void alpha_blur(stroke_filter_data_t *data)
{
	gs_effect_t *effect = data->effect_alpha_blur;
	gs_texture_t *texture = gs_texrender_get_texture(data->input_texrender);

	if (!effect || !texture) {
		return;
	}

	const int passes = 1;
	for (int i = 0; i < passes; i++) {
		// 1. First pass- apply 1D blur kernel to horizontal dir.
		data->alpha_blur_pass_1 =
			create_or_reset_texrender(data->alpha_blur_pass_1);

		gs_eparam_t *image =
			gs_effect_get_param_by_name(effect, "image");
		gs_effect_set_texture(image, texture);

		if (data->param_blur_radius) {
			gs_effect_set_float(data->param_blur_radius, data->stroke_size);
		}

		struct vec2 texel_step;
		texel_step.x = 1.0f / data->width;
		texel_step.y = 0.0f;
		if (data->param_blur_texel_step) {
			gs_effect_set_vec2(data->param_blur_texel_step, &texel_step);
		}

		set_blending_parameters();

		if (gs_texrender_begin(data->alpha_blur_pass_1, data->width,
				       data->height)) {
			gs_ortho(0.0f, (float)data->width, 0.0f,
				 (float)data->height, -100.0f, 100.0f);
			while (gs_effect_loop(effect, "Draw"))
				gs_draw_sprite(texture, 0, data->width,
					       data->height);
			gs_texrender_end(data->alpha_blur_pass_1);
		}

		// 2. Save texture from first pass in variable "texture"
		texture = gs_texrender_get_texture(data->alpha_blur_pass_1);

		// 3. Second Pass- Apply 1D blur kernel vertically.
		image = gs_effect_get_param_by_name(effect, "image");
		gs_effect_set_texture(image, texture);

		texel_step.x = 0.0f;
		texel_step.y = 1.0f / data->height;
		if (data->param_blur_texel_step) {
			gs_effect_set_vec2(data->param_blur_texel_step, &texel_step);
		}

		data->alpha_blur_output =
			create_or_reset_texrender(data->alpha_blur_output);

		if (gs_texrender_begin(data->alpha_blur_output, data->width,
				       data->height)) {
			gs_ortho(0.0f, (float)data->width, 0.0f,
				 (float)data->height, -100.0f, 100.0f);
			while (gs_effect_loop(effect, "Draw"))
				gs_draw_sprite(texture, 0, data->width,
					       data->height);
			gs_texrender_end(data->alpha_blur_output);
		}

		texture = gs_texrender_get_texture(data->alpha_blur_output);
		gs_blend_state_pop();
	}
}

void load_1d_alpha_blur_effect(stroke_filter_data_t *filter)
{
	const char *effect_file_path = "/shaders/alpha_blur_1d.effect";
	filter->effect_alpha_blur = load_shader_effect(filter->effect_alpha_blur, effect_file_path);
	if (filter->effect_alpha_blur) {
		size_t effect_count = gs_effect_get_num_params(filter->effect_alpha_blur);
		for (size_t effect_index = 0; effect_index < effect_count;
		     effect_index++) {
			gs_eparam_t *param = gs_effect_get_param_by_idx(
				filter->effect_alpha_blur, effect_index);
			struct gs_effect_param_info info;
			gs_effect_get_param_info(param, &info);
			if (strcmp(info.name, "texel_step") == 0) {
				filter->param_blur_texel_step = param;
			} else if (strcmp(info.name, "radius") == 0) {
				filter->param_blur_radius = param;
			}
		}
	}
}
