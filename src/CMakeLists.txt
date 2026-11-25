set(SOURCES
	"main.c"

	"gen/checker_shd.h"
	"gen/grid_shd.h"
)
add_bgame_app(cute-slicer "${SOURCES}")
target_link_libraries(cute-slicer PRIVATE ufa)

compile_draw_shader(
	${CMAKE_CURRENT_SOURCE_DIR}/checker.glsl
	checker_shd_bytecode
	${CMAKE_CURRENT_SOURCE_DIR}/gen/checker_shd.h
)
compile_draw_shader(
	${CMAKE_CURRENT_SOURCE_DIR}/grid.glsl
	grid_shd_bytecode
	${CMAKE_CURRENT_SOURCE_DIR}/gen/grid_shd.h
)

if (EMSCRIPTEN)
	target_link_libraries(cute-slicer PRIVATE bgame-emscripten-shell)
	target_link_options(cute-slicer PRIVATE
		-sMALLOC=emmalloc
		-gseparate-dwarf
	)
endif ()
