#include "anti-alias.h"

extern void anti_alias(stroke_filter_data_t *data)
{
	gs_effect_t *effect = data->effect_anti_alias;
	gs_texture_t *texture = gs_texrender_get_texture(data->stroke_mask);

	if (!effect || !texture) {
		return;
	}

	gs_texrender_t *tmp = data->stroke_mask;
	data->stroke_mask = data->alpha_blur_data->alpha_blur_pass_1;
	data->alpha_blur_data->alpha_blur_pass_1 = tmp;


	// 1. First pass- apply 1D blur kernel to horizontal dir.
	data->stroke_mask =
		create_or_reset_texrender(data->stroke_mask);

	if (data->param_aa_image) {
		gs_effect_set_texture(data->param_aa_image, texture);
	}
	int size = 15;
	if (data->param_aa_size) {
		gs_effect_set_int(data->param_aa_size, size);
	}

	struct vec2 texel_step;
	texel_step.x = 1.0f / data->width;
	texel_step.y = 1.0f / data->height;
	if (data->param_aa_texel_step) {
		gs_effect_set_vec2(data->param_aa_texel_step, &texel_step);
	}

	set_blending_parameters();

	if (gs_texrender_begin(data->stroke_mask, data->width,
			       data->height)) {
		gs_ortho(0.0f, (float)data->width, 0.0f, (float)data->height,
			 -100.0f, 100.0f);
		while (gs_effect_loop(effect, "Draw"))
			gs_draw_sprite(texture, 0, data->width, data->height);
		gs_texrender_end(data->stroke_mask);
	}

	//// 2. Save texture from first pass in variable "texture"
	//texture = gs_texrender_get_texture(data->alpha_blur_pass_1);

	//// 3. Second Pass- Apply 1D blur kernel vertically.
	//
	//if (data->param_aa_image) {
	//	gs_effect_set_texture(data->param_aa_image, texture);
	//}

	//texel_step.x = 0.0f;
	//texel_step.y = 1.0f / data->height;
	//if (data->param_aa_texel_step) {
	//	gs_effect_set_vec2(data->param_aa_texel_step, &texel_step);
	//}

	//data->stroke_mask = create_or_reset_texrender(data->stroke_mask);

	//if (gs_texrender_begin(data->stroke_mask, data->width, data->height)) {
	//	gs_ortho(0.0f, (float)data->width, 0.0f, (float)data->height,
	//		 -100.0f, 100.0f);
	//	while (gs_effect_loop(effect, "Draw"))
	//		gs_draw_sprite(texture, 0, data->width, data->height);
	//	gs_texrender_end(data->stroke_mask);
	//}
	gs_blend_state_pop();
}

extern void load_1d_anti_alias_effect(stroke_filter_data_t *filter)
{
	const char *effect_file_path = "/shaders/anti_alias_1d.effect";
	filter->effect_anti_alias =
		load_shader_effect(filter->effect_anti_alias, effect_file_path);
	if (filter->effect_anti_alias) {
		size_t effect_count =
			gs_effect_get_num_params(filter->effect_anti_alias);
		for (size_t effect_index = 0; effect_index < effect_count;
		     effect_index++) {
			gs_eparam_t *param = gs_effect_get_param_by_idx(
				filter->effect_anti_alias, effect_index);
			struct gs_effect_param_info info;
			gs_effect_get_param_info(param, &info);
			if (strcmp(info.name, "texel_step") == 0) {
				filter->param_aa_texel_step = param;
			} else if (strcmp(info.name, "size") == 0) {
				filter->param_aa_size = param;
			} else if (strcmp(info.name, "image") == 0) {
				filter->param_aa_image = param;
			}
		}
	}
}
