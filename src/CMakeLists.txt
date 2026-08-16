set(SOURCES
	"main.c"

	"ufa/ufa.c"

	"gen/checker_shd.h"
	"gen/grid_shd.h"
)
add_bgame_app(cute-slicer "${SOURCES}")

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
		"--js-library=${CMAKE_CURRENT_LIST_DIR}/ufa/ufa.js"
		-sMALLOC=emmalloc
		-gseparate-dwarf
	)
endif ()
