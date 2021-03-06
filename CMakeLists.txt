# $Id: CMakeLists.txt 1320 2014-10-26 08:12:52Z justin $

if(LFL_MOBILE)
  return()
endif()

if(LFL_LIBCLANG)
  set(TU_LIB app_clang_tu)
else()
  set(TU_LIB app_null_tu)
endif()

lfl_set_os_toolkit(EDITOR)
lfl_project(TepidFusion)
add_subdirectory(imports)

lfl_add_package(TepidFusion SOURCES editor.cpp
                LINK_LIBRARIES ${LFL_APP_LIB} ${EDITOR_FRAMEWORK} ${EDITOR_GRAPHICS}
                app_null_audio app_null_camera app_null_matrix app_null_fft
                app_simple_resampler app_simple_loader ${LFL_APP_CONVERT}
                app_libpng_png app_null_jpeg app_null_gif app_null_ogg app_null_css ${LFL_APP_FONT}
                app_null_ssl app_null_js app_ide ${TU_LIB} app_cmake_daemon app_stdregex_regex
                app_null_crashreporting ${EDITOR_TOOLKIT} ${LFL_APP_OS}
                ASSET_FILES ${LFL_APP_ASSET_FILES} ${LFL_SOURCE_DIR}/core/app/assets/Nobile.*
                ${LFL_SOURCE_DIR}/core/app/assets/VeraMoBd.ttf,32,*
                ASSET_DIRS ${LFL_APP_ASSET_DIRS} ${CMAKE_CURRENT_BINARY_DIR}/imports/cmake-daemon/bin
                ${CMAKE_CURRENT_BINARY_DIR}/imports/cmake-daemon/share)
lfl_post_build_start(TepidFusion)
