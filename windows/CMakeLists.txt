cmake_minimum_required(VERSION 3.16)
project(smac VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_PREFIX_PATH "C:/msys64/mingw64/qt6-static")

set(app_icon_resource_windows "${CMAKE_CURRENT_SOURCE_DIR}/icon.rc")
    
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets UiTools OpenGL)
qt_standard_project_setup()

qt_add_executable(smac-gui ../smac-gui.cpp ${app_icon_resource_windows})

# Manually specify curl
target_include_directories(smac-gui PRIVATE ../3rd/curl-8.21.0_6-win64-mingw/include)
target_link_directories(smac-gui PRIVATE ../3rd/curl-8.21.0_6-win64-mingw/lib)

target_link_libraries(smac-gui PRIVATE 
    Qt6::Core 
    Qt6::Gui 
    Qt6::Widgets 
    Qt6::UiTools 
    Qt6::OpenGL
    ${CMAKE_CURRENT_SOURCE_DIR}/../3rd/curl-8.21.0_6-win64-mingw/lib/libcurl.dll.a
)


if(WIN32)
	set(QT_DLL_PATH "C:/msys64/mingw64/bin")
    
    set(REQUIRED_DLLS
	libb2-1.dll
        libfreetype-6.dll
        libharfbuzz-0.dll
        libjpeg-8.dll
        libpcre2-16-0.dll
        libpng16-16.dll
        libtiff-6.dll
        libwebp-7.dll
        libwebpdemux-2.dll
        libwebpmux-3.dll
        zlib1.dll
	#libcurl-x64.dll
	#not adverted by objdump.exe but needed by users
	libbrotlidec.dll
	libbz2-1.dll
	libgcc_s_seh-1.dll
	libglib-2.0-0.dll
	libgraphite2.dll
	libstdc++-6.dll
	libdeflate.dll
	libjbig-0.dll
	liblzma-5.dll
	libLerc.dll
	libzstd.dll
	libsharpyuv-0.dll
	libbrotlicommon.dll
	libwinpthread-1.dll
	libintl-8.dll
	libpcre2-8-0.dll
	libiconv-2.dll
    )
    
    foreach(DLL ${REQUIRED_DLLS})
        add_custom_command(TARGET smac-gui POST_BUILD
	    COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${QT_DLL_PATH}/${DLL}"
                "$<TARGET_FILE_DIR:smac-gui>/${DLL}"
        )
    endforeach()
endif()

#

