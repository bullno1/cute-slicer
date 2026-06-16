layout (set = 3, binding = 1) uniform shd_uniforms {
	// @param name=u_grid_size type=float2 target=uniform step=1.0 min=1.0 default.x=16.0
	vec2 u_grid_size;
	// @param name=u_screen_w type=float source=screen.w
	float u_screen_w;
	// @param name=u_screen_h type=float source=screen.h
	float u_screen_h;
	// @param name=u_color1 type=color target=uniform default.r=1.0 default.g=1.0 default.b=1.0 default.a=1.0
	vec4 u_color1;
	// @param name=u_color2 type=color target=uniform default.r=0.5 default.g=0.5 default.b=0.5 default.a=1.0
	vec4 u_color2;
};

vec4 shader(vec4 color, ShaderParams params) {
	vec2 grid = floor(params.screen_uv * (vec2(u_screen_w, u_screen_h) / u_grid_size));
	float checker = mod(grid.x + grid.y, 2.0);
	return mix(u_color1, u_color2, checker);
}
