message( "Current Project: ImGui" )

set(
	IMGUI_FILES
	imconfig.h
	imgui.cpp
	imgui.h
	imgui_demo.cpp
	imgui_draw.cpp
	imgui_impl_sdl.cpp
	imgui_impl_sdl.h
	imgui_impl_sdlrenderer.cpp
	imgui_impl_sdlrenderer.h
	imgui_internal.h
	imgui_tables.cpp
	imgui_widgets.cpp
	imstb_rectpack.h
	imstb_textedit.h
	imstb_truetype.h
)

include( "../core.cmake" )

include_directories(
	"../thirdparty/SDL2/include"
)

add_library(
	ImGui STATIC
	${IMGUI_FILES} )

set_target_properties(
	ImGui PROPERTIES
	LINKER_LANGUAGE CXX
	OUTPUT_NAME imgui
)

