# $Id: CMakeLists.txt 1320 2014-10-26 08:12:52Z justin $

lfl_add_target(debugger debugger.cpp)
target_compile_definitions(debugger PUBLIC ${LFL_APP_DEF})
target_include_directories(debugger PUBLIC ${LFL_APP_INCLUDE} ${JSONCPP_INCLUDE} ${LIBCLANG_INCLUDE})
target_link_libraries(debugger ${LFL_APP_LIB} ${LFL_APP_OS} ${LFL_APP_FRAMEWORK} ${LFL_APP_GRAPHICS}
                      app_null_audio app_null_camera app_null_matrix app_null_fft
                      app_simple_resampler app_simple_loader ${LFL_APP_CONVERT}
                      app_libpng_png app_null_jpeg app_null_gif app_null_ogg app_null_css app_null_ttf
                      app_null_ssl app_null_js app_clang_tu ${JSONCPP_LIB})
lfl_post_build_copy_bin(debugger debugger LEditor)

lfl_project(editor)
lfl_add_target(editor editor.cpp)
target_compile_definitions(editor PUBLIC ${LFL_APP_DEF})
target_include_directories(editor PUBLIC ${LFL_APP_INCLUDE} ${JSONCPP_INCLUDE} ${LIBCLANG_INCLUDE})
target_link_libraries(editor ${LFL_APP_LIB} ${LFL_APP_OS} ${LFL_APP_FRAMEWORK} ${LFL_APP_GRAPHICS}
                      app_null_audio app_null_camera app_null_matrix app_null_fft
                      app_simple_resampler app_simple_loader ${LFL_APP_CONVERT}
                      app_libpng_png app_null_jpeg app_null_gif app_null_ogg app_null_css app_null_ttf
                      app_null_ssl app_null_js app_clang_tu ${JSONCPP_LIB})
add_dependencies(editor cmakedaemon)
set(LFL_APP_ASSET_FILES ${LFL_APP_ASSET_FILES} ${LFL_SOURCE_DIR}/core/app/assets/Nobile.*
    ${LFL_SOURCE_DIR}/core/app/assets/VeraMoBd.ttf,32,*)
set(LFL_APP_ASSET_DIRS ${LFL_APP_ASSET_DIRS}
    ${LFL_BINARY_DIR}/core/imports/cmake-daemon/bin ${LFL_BINARY_DIR}/core/imports/cmake-daemon/share)
lfl_post_build_start(editor editor LEditor)
