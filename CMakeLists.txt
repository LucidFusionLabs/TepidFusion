# $Id: CMakeLists.txt 1320 2014-10-26 08:12:52Z justin $

lfl_set_os_toolkit(EDITOR)
lfl_project(editor)
lfl_add_package(editor SOURCES editor.cpp
                LINK_LIBRARIES ${LFL_APP_LIB} ${EDITOR_FRAMEWORK} ${EDITOR_GRAPHICS}
                app_null_audio app_null_camera app_null_matrix app_null_fft
                app_simple_resampler app_simple_loader ${LFL_APP_CONVERT}
                app_libpng_png app_null_jpeg app_null_gif app_null_ogg app_null_css ${LFL_APP_FONT}
                app_null_ssl app_null_js app_clang_tu app_cmake_daemon app_stdregex_regex ${EDITOR_TOOLKIT} ${LFL_APP_OS})
set(LFL_APP_ASSET_FILES ${LFL_APP_ASSET_FILES} ${LFL_SOURCE_DIR}/core/app/assets/Nobile.*
    ${LFL_SOURCE_DIR}/core/app/assets/VeraMoBd.ttf,32,*)
set(LFL_APP_ASSET_DIRS ${LFL_APP_ASSET_DIRS}
    ${LFL_BINARY_DIR}/core/imports/cmake-daemon/bin ${LFL_BINARY_DIR}/core/imports/cmake-daemon/share)
lfl_post_build_start(editor editor LEditor)
