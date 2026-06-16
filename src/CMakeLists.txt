set(SOURCES
	"main.c"

	"gen/checker_shd.h"
	"gen/grid_shd.h"
)
add_bgame_app(cute-slicer "${SOURCES}")

compile_shader(
	${CMAKE_CURRENT_SOURCE_DIR}/checker.glsl
	checker_shd_bytecode
	${CMAKE_CURRENT_SOURCE_DIR}/gen/checker_shd.h
)
compile_shader(
	${CMAKE_CURRENT_SOURCE_DIR}/grid.glsl
	grid_shd_bytecode
	${CMAKE_CURRENT_SOURCE_DIR}/gen/grid_shd.h
)
