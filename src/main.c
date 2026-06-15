#include <bgame/entrypoint.h>
#include <cute.h>
#include <blog.h>
#include <SDL3/SDL_video.h>

BGAME_VAR(bool, app_created) = false;

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

		app_created = true;
	}

	cf_set_fixed_timestep(60);
	cf_app_set_vsync(true);
}

static void
update(void) {
	if (cf_app_was_resized()) {
		handle_resize();
	}

	cf_app_update(NULL);

	cf_app_draw_onto_screen(true);
}

static void
cleanup(void) {
	cf_destroy_app();
}


static bgame_app_t app = {
	.init = init,
	.cleanup = cleanup,
	.update = update,
};

BGAME_ENTRYPOINT(app)
