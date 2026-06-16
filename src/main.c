#include <bgame/entrypoint.h>
#include <bgame/allocator/frame.h>
#include <bgame/utils.h>
#include <bgame/shader.h>
#include <cute.h>
#include <blog.h>
#include <dcimgui.h>
#include <SDL3/SDL_video.h>
#include <SDL3/SDL_dialog.h>
#include "gen/checker_shd.h"

typedef struct {
	int x, y;
} ivec2_t;

BGAME_VAR(bool, app_created) = false;
BGAME_VAR(CF_V2, draw_offset) = { 0 };
BGAME_VAR(float, draw_scale) = 1.f;
BGAME_VAR(bool, draw_grid) = true;
BGAME_VAR(ivec2_t, grid_size) = { 16, 16 };
BGAME_VAR(CF_Sprite, active_sprite) = { };
BGAME_VAR(CF_Image, active_image) = { };
BGAME_VAR(ivec2_t, selected_pos) = { };
BGAME_VAR(CF_Shader, shd_checker) = { };

static ivec2_t grid_pos = { };
static CF_Coroutine current_modal = { };

typedef struct {
	void (*entry)(void);
} modal_ctx_t;

typedef enum {
	CMD_NONE,
	CMD_LOAD,
	CMD_SAVE,
} cmd_t;

static void
handle_resize(void) {
	int window_width, window_height;
	SDL_GetWindowSize(cf_app_get_window(), &window_width, &window_height);
	BLOG_INFO("Window size: %d x %d", window_width, window_height);

	float display_scale = SDL_GetWindowDisplayScale(cf_app_get_window());
	BLOG_INFO("Display scale: %f", display_scale);

	int backbuffer_width, backbuffer_height;
    SDL_GetWindowSizeInPixels(cf_app_get_window(), &backbuffer_width, &backbuffer_height);
	BLOG_INFO("Backbuffer size: %d x %d", backbuffer_width, backbuffer_height);

	int canvas_width  = cf_round((float)backbuffer_width  / display_scale);
	int canvas_height = cf_round((float)backbuffer_height / display_scale);
	BLOG_INFO("Canvas size: %d x %d", canvas_width, canvas_height);

	cf_app_set_canvas_size(canvas_width, canvas_height);
	cf_draw_projection(cf_ortho_2d(0.f, 0.f, canvas_width, canvas_height));
}

static void
init(int argc, const char** argv) {
	// Cute Framework
	if (!app_created) {
		int width  = 640;
		int height = 360;

		BLOG_INFO("Creating app");
		int options =
			  CF_APP_OPTIONS_WINDOW_POS_CENTERED_BIT
			| CF_APP_OPTIONS_FILE_SYSTEM_DONT_DEFAULT_MOUNT_BIT
			| CF_APP_OPTIONS_RESIZABLE_BIT
			;
		CF_Result result = cf_make_app("cute-slicer", 0, 0, 0, width, height, options, argv[0]);
		if (result.code != CF_RESULT_SUCCESS) {
			BLOG_FATAL("Could not create app: %s", result.details);
			abort();
		}

		handle_resize();

		cf_app_init_imgui();
		active_sprite = cf_sprite_defaults();

		app_created = true;
	}

	cf_set_fixed_timestep(60);
	cf_app_set_vsync(true);
	cf_clear_color(0.5f, 0.5f, 0.5f, 1.f);

	bgame_load_draw_shader(&shd_checker, checker_shd_bytecode);
}

static void
cleanup(void) {
	cf_image_free(&active_image);
	if (active_sprite.id != cf_sprite_defaults().id) {
		cf_easy_sprite_unload(&active_sprite);
	}

	cf_destroy_app();
}

static CF_Result
load_png(const char* path, CF_Image* out) {
	CF_Result result;

	void* buf = NULL;

	FILE* file = fopen(path, "rb");
	if (file == NULL) {
		result = cf_result_error("Could not open file");
		goto end;
	}

	fseek(file, 0, SEEK_END);
	long size = ftell(file);
	fseek(file, 0, SEEK_SET);
	buf = malloc(size);
	if (fread(buf, size, 1, file) != 1) {
		result = cf_result_error("Could not read file");
		goto end;
	}

	result = cf_image_load_png_from_memory(buf, size, out);
end:
	free(buf);
	if (file) { fclose(file); }

	return result;
}

static void
handle_open_file(void *userdata, const char* const* filelist, int filter) {
	if (filelist == NULL || *filelist == NULL) { return; }

	CF_Image new_image = { };
	CF_Result result = load_png(*filelist, &new_image);
	if (result.code != CF_RESULT_SUCCESS) {
		BLOG_ERROR("Error while loading %s: %s", *filelist, result.details);
		return;
	}

	CF_Sprite new_sprite = cf_make_easy_sprite_from_pixels(new_image.pix, new_image.w, new_image.h);
	if (active_sprite.id != cf_sprite_defaults().id) {
		cf_easy_sprite_unload(&active_sprite);
	}
	cf_image_free(&active_image);

	active_sprite = new_sprite;
	active_image = new_image;

	draw_scale = 1.f;
	draw_offset = cf_v2(0.f);

	BLOG_INFO("Loaded %s", *filelist);
}

static bool
ends_with(const char* str, const char* suffix) {
	size_t str_len = strlen(str);
	size_t suffix_len = strlen(suffix);

	if (suffix_len > str_len) { return false; }

	return strcmp(str + str_len - suffix_len, suffix) == 0;
}

static void
handle_save_file(void *userdata, const char* const* filelist, int filter) {
	if (filelist == NULL || *filelist == NULL) { return; }

	const char* filename = *filelist;
	if (!ends_with(filename, ".png")) {
		filename = bgame_fmt("%s.png", *filelist);
	}

	CF_Image slice = {
		.w = grid_size.x,
		.h = grid_size.y,
		.pix = malloc(sizeof(CF_Pixel) * grid_size.x * grid_size.y),
	};

	for (int y = 0; y < grid_size.y; ++y) {
		int pix_y = cf_min(selected_pos.y * grid_size.y + y, active_image.h);

		for (int x = 0; x < grid_size.x; ++x) {
			int pix_x = cf_min(selected_pos.x * grid_size.x + x, active_image.w);

			slice.pix[y * slice.w + x] = active_image.pix[pix_y * active_image.w + pix_x];
		}
	}

	FILE* file = NULL;
	void* png = NULL;
	int png_len = 0;
	if (cf_image_save_png_to_memory(&slice, &png, &png_len).code != CF_RESULT_SUCCESS) {
		BLOG_ERROR("Could not encode image");
		goto end;
	}

	file = fopen(filename, "wb");
	if (file == NULL) {
		BLOG_ERROR("Could not open file");
		goto end;
	}
	if (fwrite(png, png_len, 1, file) != 1) {
		BLOG_ERROR("Could not write file");
		goto end;
	}

	BLOG_INFO("Saved %s", filename);

end:
	free(slice.pix);
	cf_free(png);
	fclose(file);
}

static bool
modal_yield(void) {
	cf_coroutine_yield(cf_coroutine_currently_running());
	return true;
}

static void
drag(void) {
	CF_V2 start_pos = { cf_mouse_x(), -cf_mouse_y() };
	CF_V2 original_offset = draw_offset;

	while (cf_mouse_down(CF_MOUSE_BUTTON_MIDDLE) && modal_yield()) {
		CF_V2 current_pos = { cf_mouse_x(), -cf_mouse_y() };
		draw_offset = cf_add(original_offset, cf_sub(current_pos, start_pos));
	}
}

static void
modal_wrapper(CF_Coroutine coro) {
	bgame_block_reload();

	modal_ctx_t ctx = *(modal_ctx_t*)cf_coroutine_get_udata(coro);
	ctx.entry();

	bgame_unblock_reload();
}

static void
start_modal(void(*entry)(void)) {
	if (current_modal.id != 0) { return; }

	modal_ctx_t ctx = {
		.entry = entry,
	};
	current_modal = cf_make_coroutine(modal_wrapper, 0, &ctx);
}

static void
update(void) {
	if (cf_app_was_resized()) {
		handle_resize();
	}

	cf_app_update(NULL);

	cmd_t cmd = CMD_NONE;

	if (ImGui_Begin("Slicer", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
		if (ImGui_Button("Load image")) {
			cmd = CMD_LOAD;
		}
		ImGui_Separator();

		ImGui_InputFloatEx("Zoom", &draw_scale, 0.1f, 0.2f, "%f", ImGuiInputTextFlags_None);
		ImGui_InputInt2("Grid size", &grid_size.x, ImGuiInputTextFlags_None);
		ImGui_Checkbox("Show grid", &draw_grid);
		grid_size.x = cf_max(1, grid_size.x);
		grid_size.y = cf_max(1, grid_size.y);

		ImGui_Separator();
		ImGui_LabelText("Grid x", "%d", grid_pos.x);
		ImGui_LabelText("Grid y", "%d", grid_pos.y);
		ImGui_LabelText("Width", "%d", active_image.w);

		ImGui_Separator();
		ImGui_LabelText("Height", "%d", active_image.h);
		ImGui_LabelText("Rows", "%d", active_image.w / grid_size.x);
		ImGui_LabelText("Cols", "%d", active_image.h / grid_size.y);

		ImGui_Separator();
		if (ImGui_Button("Save")) {
			cmd = CMD_SAVE;
		}
	}
	ImGui_End();

	if (cf_mouse_just_pressed(CF_MOUSE_BUTTON_MIDDLE)) {
		start_modal(drag);
	}

	draw_scale += cf_mouse_wheel_motion() * 0.1f;
	draw_scale = cf_max(draw_scale, 0.1f);

	CF_V2 world_mouse;
	BGAME_SCOPE(cf_draw_push(), cf_draw_pop()) {
		cf_draw_translate_v2(draw_offset);
		cf_draw_scale(draw_scale, draw_scale);
		world_mouse = cf_screen_to_world(cf_v2(cf_mouse_x(), cf_mouse_y()));
		cf_draw_translate_v2(draw_offset);
		cf_draw_scale(draw_scale, draw_scale);
		grid_pos.x = cf_clamp(
			(int)cf_floor((active_sprite.w * 0.5f + world_mouse.x) / (float)grid_size.x),
			0, (int)(active_sprite.w / grid_size.x)
		);
		grid_pos.y = cf_clamp(
			(int)cf_floor((active_sprite.h * 0.5f - world_mouse.y) / (float)grid_size.y),
			0, (int)(active_sprite.h / grid_size.y)
		);
	}

	ImGuiIO* io = ImGui_GetIO();
	if (!io->WantCaptureMouse) {
		if (cf_mouse_just_pressed(CF_MOUSE_BUTTON_LEFT)) {
			selected_pos = grid_pos;
		}
	}

	if (!io->WantCaptureKeyboard) {
		if (cf_key_just_pressed(CF_KEY_UP)) {
			selected_pos.y -= 1;
		}

		if (cf_key_just_pressed(CF_KEY_DOWN)) {
			selected_pos.y += 1;
		}

		if (cf_key_just_pressed(CF_KEY_LEFT)) {
			selected_pos.x -= 1;
		}

		if (cf_key_just_pressed(CF_KEY_RIGHT)) {
			selected_pos.x += 1;
		}

		if (cf_key_down(CF_KEY_LCTRL) || cf_key_down(CF_KEY_LCTRL)) {
			if (cf_key_just_pressed(CF_KEY_O)) {
				cmd = CMD_LOAD;
			}

			if (cf_key_just_pressed(CF_KEY_S)) {
				cmd = CMD_SAVE;
			}
		}
	}

	if (current_modal.id != 0) {
		cf_coroutine_resume(current_modal);
		if (cf_coroutine_state(current_modal) == CF_COROUTINE_STATE_DEAD) {
			cf_destroy_coroutine(current_modal);
			current_modal.id = 0;
		}
	}

	switch (cmd) {
		case CMD_NONE:
			break;
		case CMD_LOAD:
			SDL_ShowOpenFileDialog(
				handle_open_file, NULL,
				cf_app_get_window(),
				&(SDL_DialogFileFilter){
					.name = "PNG",
					.pattern = "png",
				},
				1,
				NULL,
				false
			);
			break;
		case CMD_SAVE:
			SDL_ShowSaveFileDialog(
				handle_save_file, NULL,
				cf_app_get_window(),
				&(SDL_DialogFileFilter){
					.name = "PNG",
					.pattern = "png",
				},
				1,
				NULL
			);
			break;
	}

	BGAME_SCOPE(cf_draw_push_shape_aa(0.f), cf_draw_pop_shape_aa())
	BGAME_SCOPE(cf_draw_push_shader(shd_checker), cf_draw_pop_shader())
	{
		cf_draw_set_uniform_float("u_grid_size", 32.f);
		cf_draw_set_uniform_float("u_screen_w", cf_app_get_width());
		cf_draw_set_uniform_float("u_screen_h", cf_app_get_height());
		cf_draw_set_uniform_color("u_color1", cf_make_color_hex(0xcccccc));
		cf_draw_set_uniform_color("u_color2", cf_make_color_hex(0x999999));
		cf_draw_box(
			cf_make_aabb_pos_w_h(cf_v2(0.f), cf_app_get_width(), cf_app_get_height()),
			0.f,
			0.f
		);
	}

	BGAME_SCOPE(cf_draw_push(), cf_draw_pop()) {
		cf_draw_translate_v2(draw_offset);
		cf_draw_scale(draw_scale, draw_scale);

		if (active_sprite.easy_sprite_id != cf_sprite_defaults().id) {
			cf_draw_sprite(&active_sprite);

			if (draw_grid) {
				BGAME_SCOPE(
					cf_draw_push_color(cf_make_color_rgba(0, 0, 0, 120)),
					cf_draw_pop_color()
				) {
					for (int x = 0; x < active_sprite.w; x += grid_size.x) {
						for (int y = 0; y < active_sprite.h; y += grid_size.y) {
							CF_Color color = cf_make_color_rgba(0, 0, 0, 128);

							CF_Aabb cell = cf_make_aabb_from_top_left(
								cf_v2(
									-active_sprite.w * 0.5f + x,
									 active_sprite.h * 0.5f - y
								),
								grid_size.x, grid_size.y
							);

							cf_draw_box(cell, 0.01f, 0.01f);
						}
					}
				}
			}

			CF_Aabb bounding_box = cf_make_aabb_pos_w_h(cf_v2(0.f), active_sprite.w, active_sprite.h);

			BGAME_SCOPE(
				cf_draw_push_color(cf_color_black()),
				cf_draw_pop_color()
			) {
				cf_draw_box(bounding_box, 1.f, 0.1f);
			}

			if (cf_contains_point(bounding_box, world_mouse)) {
				BGAME_SCOPE(
					cf_draw_push_color(cf_make_color_rgba(255, 255, 255, 255)),
					cf_draw_pop_color()
				) {
					CF_Aabb hovered_cell = cf_make_aabb_from_top_left(
						cf_v2(
							-active_sprite.w * 0.5f + grid_pos.x * grid_size.x,
							active_sprite.h * 0.5f - grid_pos.y * grid_size.y
							),
						grid_size.x, grid_size.y
					);
					cf_draw_box(hovered_cell, 1.0f, 0.1f);
				}
			}

			BGAME_SCOPE(
				cf_draw_push_color(cf_make_color_rgba(255, 255, 255, 255)),
				cf_draw_pop_color()
			) {
				CF_Aabb selected_cell = cf_make_aabb_from_top_left(
					cf_v2(
						-active_sprite.w * 0.5f + selected_pos.x * grid_size.x,
						active_sprite.h * 0.5f - selected_pos.y * grid_size.y
						),
					grid_size.x, grid_size.y
				);
				cf_draw_box(selected_cell, 0.1f, 0.5f + cf_sin(CF_SECONDS * 5.f) * 0.2f);
			}
		}
	}

	cf_app_draw_onto_screen(true);
}

BGAME_APP {
	.init = init,
	.cleanup = cleanup,
	.update = update,
};
