layout (set = 3, binding = 1) uniform shd_uniforms {
	// @param name=u_grid_size type=vec2 target=uniform default.x=16.0 default.y=16.0
	vec2 u_grid_size;
	// @param name=u_grid_offset type=vec2 target=uniform default.x=0.0 default.y=0.0
	vec2 u_grid_offset;
	// @param name=u_grid_gap type=vec2 target=uniform default.x=0.0 default.y=0.0
	vec2 u_grid_gap;
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

	// Image pixels per screen pixel at this fragment (for scale-invariant width).
	vec2 img_px_per_screen_px = vec2(
		length(vec2(dFdx(local_px.x), dFdy(local_px.x))),
		length(vec2(dFdx(local_px.y), dFdy(local_px.y)))
	);

	// Shift by the grid offset so (0,0) is the top-left of the first tile.
	// Pixels before the offset are not part of the grid.
	local_px -= u_grid_offset;
	if (local_px.x < 0.0 || local_px.y < 0.0) {
		return color;
	}

	// A tile plus its trailing gap repeats every `pitch` pixels.
	vec2 pitch   = u_grid_size + u_grid_gap;
	vec2 cell_px = fract(local_px / pitch) * pitch;

	// u_line_width in screen pixels → image pixels.
	vec2 half_line = u_line_width * img_px_per_screen_px;

	// Draw a line on the left/top edge of the tile and on its right/bottom
	// edge (at u_grid_size), leaving the gap between tiles unmarked.
	bool on_line_x = cell_px.x < half_line.x
	              || abs(cell_px.x - u_grid_size.x) < half_line.x;
	bool on_line_y = cell_px.y < half_line.y
	              || abs(cell_px.y - u_grid_size.y) < half_line.y;

	// Only draw a vertical line while inside a tile's vertical span and vice versa,
	// so lines do not extend across the gaps.
	bool in_tile_x = cell_px.x <= u_grid_size.x + half_line.x;
	bool in_tile_y = cell_px.y <= u_grid_size.y + half_line.y;

	bool on_line = (on_line_x && in_tile_y) || (on_line_y && in_tile_x);

	return on_line ? u_grid_color : color;
}
