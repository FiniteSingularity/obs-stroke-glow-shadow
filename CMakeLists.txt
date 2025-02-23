# --- Detect if the plugin is build out of tree or not ---
if(CMAKE_PROJECT_NAME STREQUAL "obs-studio")
  set(BUILD_OUT_OF_TREE OFF)
  if(OBS_CMAKE_VERSION VERSION_GREATER_EQUAL 3.0.0)
    legacy_check()
  endif()
else()
  set(BUILD_OUT_OF_TREE ON)
  cmake_minimum_required(VERSION 3.18)
endif()

project(obs-stroke-glow-shadow VERSION 1.0.2)
set(PROJECT_FULL_NAME "Stroke Glow Shadow")

# Set new UUIDs when you start to create a new plugin.
set(MACOS_PACKAGE_UUID "e3e3b46c-5052-4739-8dfb-22806d02f0e2")
set(MACOS_INSTALLER_UUID "28bbaaeb-f93b-4d11-af82-5d9eaa745ee4")
set(OBS_BUNDLE_CODESIGN_IDENTITY, "Developer ID Application: Mark Dobossy (NSMTB4HHNL)")
set(OBS_BUNDLE_CODESIGN_TEAM, "NSMTB4HHNL")

add_library(${PROJECT_NAME} MODULE)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/version.h.in ${CMAKE_CURRENT_SOURCE_DIR}/src/version.h)

target_sources(${PROJECT_NAME} PRIVATE
	src/anti-alias.c
	src/anti-alias.h
	src/glow.c
	src/glow.h
	src/obs-glow-filter.c
	src/obs-glow-filter.h
	src/obs-glow.h
	src/obs-stroke-filter.c
	src/obs-stroke-plugin.c
	src/obs-stroke-filter.h
	src/obs-stroke.h
	src/obs-utils.c
	src/obs-utils.h
	src/stroke.c
	src/stroke.h
	src/version.h
	src/blur/alpha-blur.c
	src/blur/alpha-blur.h
	src/blur/dual-kawase.c
	src/blur/dual-kawase.h)

if(BUILD_OUT_OF_TREE)
	find_package(libobs REQUIRED)
	include(cmake/ObsPluginHelpers.cmake)
endif()

if(OS_WINDOWS)
	get_filename_component(ISS_FILES_DIR "${CMAKE_BINARY_DIR}\\..\\package" ABSOLUTE)
	file(TO_NATIVE_PATH "${ISS_FILES_DIR}" ISS_FILES_DIR)
	get_filename_component(ISS_PACKAGE_DIR "${CMAKE_PACKAGE_PREFIX}\\.." ABSOLUTE)
	file(TO_NATIVE_PATH "${ISS_PACKAGE_DIR}" ISS_PACKAGE_DIR)
	get_filename_component(ISS_SOURCE_DIR "${PROJECT_SOURCE_DIR}" ABSOLUTE)
	file(TO_NATIVE_PATH "${ISS_SOURCE_DIR}" ISS_SOURCE_DIR)
	configure_file("installer.iss.in"
			"${PROJECT_BINARY_DIR}/installer.iss"
		)
	configure_file(${CMAKE_CURRENT_SOURCE_DIR}/resource.rc.in ${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME}.rc)
	target_sources(${PROJECT_NAME} PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME}.rc)
elseif(OS_MACOS)
		set(MACOS_BUNDLEID "com.finitesingularity.${PROJECT_NAME}")
		set(MACOSX_PLUGIN_GUI_IDENTIFIER "${MACOS_BUNDLEID}")
		set(MACOSX_PLUGIN_BUNDLE_VERSION "${PROJECT_VERSION}")
		set(MACOSX_PLUGIN_SHORT_VERSION_STRING "1")
		configure_file(cmake/bundle/macos/installer-macos.pkgproj.in ${CMAKE_BINARY_DIR}/installer-macos.generated.pkgproj)
		target_compile_options(${PROJECT_NAME} PRIVATE -Wall)
endif()

target_link_libraries(${PROJECT_NAME}
	OBS::libobs)

if(BUILD_OUT_OF_TREE)
    if(NOT LIB_OUT_DIR)
        set(LIB_OUT_DIR "/lib/obs-plugins")
    endif()
    if(NOT DATA_OUT_DIR)
        set(DATA_OUT_DIR "/share/obs/obs-plugins/${PROJECT_NAME}")
    endif()
    set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")
    install(TARGETS ${PROJECT_NAME}
        LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/${LIB_OUT_DIR})
    install(DIRECTORY data/locale
        DESTINATION ${CMAKE_INSTALL_PREFIX}/${DATA_OUT_DIR})
    setup_plugin_target(${PROJECT_NAME})
else()
    if(OBS_CMAKE_VERSION VERSION_GREATER_EQUAL 3.0.0)
        set_target_properties_obs(${PROJECT_NAME} PROPERTIES FOLDER "plugins/finitesingularity" PREFIX "")
    else()
        set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER "plugins/finitesingularity")
        setup_plugin_target(${PROJECT_NAME})
    endif()
endif()
