bgame_add_dependencies()

# ufa shows its dialogs through the same SDL3 that cute_framework links
if (CF_FRAMEWORK_STATIC AND TARGET SDL3::SDL3-static)
	set(UFA_SDL_TARGET SDL3::SDL3-static)
else ()
	set(UFA_SDL_TARGET SDL3::SDL3)
endif ()
add_subdirectory(ufa EXCLUDE_FROM_ALL)
# A reloadable app links it into a shared library
set_target_properties(ufa PROPERTIES POSITION_INDEPENDENT_CODE "${RELOADABLE}")
