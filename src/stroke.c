#include "stroke.h"

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
			if (strcmp(info.name, "texel_step") == 0) {
				filter->param_stroke_texel_step = param;
			} else if (strcmp(info.name, "stroke_thickness") == 0) {
				filter->param_stroke_stroke_thickness = param;
			} else if (strcmp(info.name, "color") == 0) {
				filter->param_stroke_color = param;
			}
		}
	}
}


void render_stroke_filter(stroke_filter_data_t *data)
{
	gs_effect_t *effect = data->effect_stroke;
	gs_texture_t *blur_mask_texture = gs_texrender_get_texture(data->alpha_blur_output);
	gs_texture_t *input_texture = gs_texrender_get_texture(data->input_texrender);

	if (!effect || !input_texture || !blur_mask_texture) {
		blog(LOG_INFO, "SOMETHING IS MISSING!!!!!!!!!!!!!!!!!");
		return;
	}

	// 1. First pass- apply 1D blur kernel to horizontal dir.
	data->output_texrender =
		create_or_reset_texrender(data->output_texrender);

	gs_eparam_t *image =
		gs_effect_get_param_by_name(effect, "image");
	gs_effect_set_texture(image, input_texture);

	gs_eparam_t *blur_mask =
		gs_effect_get_param_by_name(effect, "blur_mask");
	gs_effect_set_texture(blur_mask, blur_mask_texture);

	if (data->param_stroke_stroke_thickness) {
		gs_effect_set_float(data->param_stroke_stroke_thickness,
					data->stroke_size);
	}

	struct vec2 texel_step;
	texel_step.x = 1.0f / data->width;
	texel_step.y = 1.0f / data->height;
	if (data->param_blur_texel_step) {
		gs_effect_set_vec2(data->param_stroke_texel_step,
					&texel_step);
	}

	gs_effect_set_vec4(data->param_stroke_color, &data->stroke_color);

	set_blending_parameters();

	if (gs_texrender_begin(data->output_texrender, data->width,
				data->height)) {
		gs_ortho(0.0f, (float)data->width, 0.0f,
				(float)data->height, -100.0f, 100.0f);
		while (gs_effect_loop(effect, "Draw"))
			gs_draw_sprite(input_texture, 0, data->width,
					data->height);
		gs_texrender_end(data->output_texrender);
	}
	gs_blend_state_pop();
}
