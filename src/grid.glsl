layout (set = 3, binding = 1) uniform shd_uniforms {
	// @param name=u_grid_size type=vec2 target=uniform default.x=16.0 default.y=16.0
	vec2 u_grid_size;
	// @param name=u_grid_color type=color target=uniform default.r=0.0 default.g=0.0 default.b=0.0 default.a=0.5
	vec4 u_grid_color;
	// @param name=u_line_width type=float target=uniform default=1.0 min=0.1 max=5.0 step=0.1
	float u_line_width;
};

vec4 shader(vec4 color, ShaderParams params)
{
	// Atlas texel coordinate of this fragment.
	vec2 texel = params.uv * u_texture_size;

	// uv_min is the atlas boundary including the 1px margin, so the actual
	// sprite content starts 1 texel in. Subtract that to get a coordinate
	// that is (0,0) at the sprite's top-left content pixel.
	vec2 sprite_origin = params.uv_min * u_texture_size + vec2(1.0);
	vec2 local_px      = texel - sprite_origin;

	vec2 cell_fract = fract(local_px / u_grid_size);

	// Image pixels per screen pixel at this fragment (for scale-invariant width).
	vec2 img_px_per_screen_px = vec2(
		length(vec2(dFdx(local_px.x), dFdy(local_px.x))),
		length(vec2(dFdx(local_px.y), dFdy(local_px.y)))
	);

	// u_line_width in screen pixels → cell fraction.
	vec2 half_line = (u_line_width * img_px_per_screen_px) / u_grid_size;

	bool on_line = cell_fract.x < half_line.x
	            || cell_fract.x > (1.0 - half_line.x)
	            || cell_fract.y < half_line.y
	            || cell_fract.y > (1.0 - half_line.y);

	return on_line ? u_grid_color : color;
}
