cmake_minimum_required(VERSION 3.25)
project(cute-slicer)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

include("deps/bgame/cmake/prelude.cmake")

#include(CheckIPOSupported)
#check_ipo_supported(RESULT IPO_SUPPORTED)
#if (IPO_SUPPORTED AND (NOT RELOADABLE))
	#set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELWITHDEBINFO TRUE)
#endif ()

# Add deps first
set(CMAKE_FOLDER "deps")
add_subdirectory(deps)

# Then everything else to avoid leaking into deps

include("deps/bgame/bgame.cmake")
set(CMAKE_FOLDER "")
add_subdirectory(src)
