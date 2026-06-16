set(SOURCES
	"main.c"

	"gen/checker_shd.h"
)
add_bgame_app(cute-slicer "${SOURCES}")

compile_shader(
	${CMAKE_CURRENT_SOURCE_DIR}/checker.glsl
	checker_shd_bytecode
	${CMAKE_CURRENT_SOURCE_DIR}/gen/checker_shd.h
)
