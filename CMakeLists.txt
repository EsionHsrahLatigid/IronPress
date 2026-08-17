cmake_minimum_required(VERSION 3.22)

project(IronPress VERSION 0.1.1 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(IRONPRESS_BUILD_PLUGIN "Build the JUCE plug-in targets" ON)
option(IRONPRESS_BUILD_TESTS "Build the test targets" ON)
set(IRONPRESS_ARTIFACT_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/artifacts" CACHE PATH "Stable product staging root")
set(EHL_JUCE_SOURCE_DIR "" CACHE PATH "Existing JUCE source checkout; avoids network when set")

add_library(ironpress_dsp STATIC
    Source/dsp/FoundationDSP.cpp)
target_include_directories(ironpress_dsp PUBLIC Source)
target_compile_features(ironpress_dsp PUBLIC cxx_std_17)
if(MSVC)
    target_compile_options(ironpress_dsp PRIVATE /W4)
else()
    target_compile_options(ironpress_dsp PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(IRONPRESS_BUILD_TESTS)
    enable_testing()
    add_executable(ironpress_dsp_tests Tests/DSPTests.cpp)
    target_link_libraries(ironpress_dsp_tests PRIVATE ironpress_dsp)
    add_test(NAME ironpress_dsp_tests COMMAND ironpress_dsp_tests)
endif()

if(IRONPRESS_BUILD_PLUGIN)
    include(FetchContent)
    if(EHL_JUCE_SOURCE_DIR)
        if(NOT EXISTS "${EHL_JUCE_SOURCE_DIR}/CMakeLists.txt")
            message(FATAL_ERROR "EHL_JUCE_SOURCE_DIR does not contain JUCE: ${EHL_JUCE_SOURCE_DIR}")
        endif()
        add_subdirectory("${EHL_JUCE_SOURCE_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/juce")
    elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/JUCE/CMakeLists.txt")
        add_subdirectory(JUCE)
    else()
        FetchContent_Declare(
            JUCE
            GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
            GIT_TAG 91ad83ae34a81e0833b1a2b0866f54846370ae53
            GIT_SHALLOW FALSE)
        FetchContent_MakeAvailable(JUCE)
    endif()

    add_subdirectory(modules/juce-ehl-design-module)

    set(IRONPRESS_FORMATS VST3 Standalone)
    if(APPLE)
        list(APPEND IRONPRESS_FORMATS AU)
    endif()

    set(EHL_COPY_PLUGIN_AFTER_BUILD_DEFAULT OFF)
    if(APPLE AND NOT DEFINED ENV{CI})
        set(EHL_COPY_PLUGIN_AFTER_BUILD_DEFAULT ON)
    endif()
    option(EHL_COPY_PLUGIN_AFTER_BUILD
        "Copy built plug-in formats to the current user's standard plug-in folders"
        ${EHL_COPY_PLUGIN_AFTER_BUILD_DEFAULT})

    set(EHL_PLUGIN_COPY_ARGS
        COPY_PLUGIN_AFTER_BUILD ${EHL_COPY_PLUGIN_AFTER_BUILD})
    if(APPLE)
        list(APPEND EHL_PLUGIN_COPY_ARGS
            VST3_COPY_DIR "$ENV{HOME}/Library/Audio/Plug-Ins/VST3"
            AU_COPY_DIR "$ENV{HOME}/Library/Audio/Plug-Ins/Components")
    endif()

    juce_add_plugin(IronPress
        VERSION ${PROJECT_VERSION}
        COMPANY_NAME "EsionHsrahLatigid"
        BUNDLE_ID "jp.ehl.ironpress"
        PRODUCT_NAME "IronPress"
        DESCRIPTION "feed-forward log-domain saturation compressor"
        PLUGIN_MANUFACTURER_CODE EHL_
        PLUGIN_CODE IrPr
        FORMATS ${IRONPRESS_FORMATS}
        IS_SYNTH FALSE
        NEEDS_MIDI_INPUT FALSE
        NEEDS_MIDI_OUTPUT FALSE
        IS_MIDI_EFFECT FALSE
        EDITOR_WANTS_KEYBOARD_FOCUS TRUE
        MICROPHONE_PERMISSION_ENABLED TRUE
        MICROPHONE_PERMISSION_TEXT "IronPress processes live audio input."
        ${EHL_PLUGIN_COPY_ARGS})

    target_sources(IronPress PRIVATE
        Source/ParameterIDs.h
        Source/PluginEditor.cpp
        Source/PluginEditor.h
        Source/PluginProcessor.cpp
        Source/PluginProcessor.h)
    target_include_directories(IronPress PRIVATE Source)
    target_compile_definitions(IronPress PUBLIC
        JUCE_REPORT_APP_USAGE=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_WEB_BROWSER=0)
    target_link_libraries(IronPress
        PRIVATE
            ironpress_dsp
            EHL::JuceDesign
            juce::juce_audio_utils
            juce::juce_dsp
        PUBLIC
            juce::juce_recommended_config_flags
            juce::juce_recommended_lto_flags
            juce::juce_recommended_warning_flags)

    target_sources(IronPress_Standalone PRIVATE Source/StandaloneApp.cpp)
    target_compile_definitions(IronPress_Standalone PRIVATE JUCE_USE_CUSTOM_PLUGIN_STANDALONE_APP=1)

    set(ehl_stage_platform "linux-x64")
    if(APPLE)
        set(ehl_stage_platform "macos-arm64")
    elseif(WIN32)
        set(ehl_stage_platform "windows-x64")
    endif()
    set(ehl_juce_products "${CMAKE_CURRENT_BINARY_DIR}/IronPress_artefacts/$<CONFIG>")
    set(ehl_staged_products "${IRONPRESS_ARTIFACT_ROOT}/plugin-release/${ehl_stage_platform}")

    set(ehl_stage_dependencies IronPress_VST3 IronPress_Standalone)
    set(ehl_stage_commands
        COMMAND ${CMAKE_COMMAND} -E make_directory "${ehl_staged_products}/vst3"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${ehl_staged_products}/standalone"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${ehl_staged_products}/au")

    if(APPLE)
        list(APPEND ehl_stage_commands
            COMMAND ${CMAKE_COMMAND} -E rm -rf "${ehl_staged_products}/vst3/ironpress_vst3_plugin.vst3"
            COMMAND ${CMAKE_COMMAND} -E rm -rf "${ehl_staged_products}/standalone/ironpress_standalone_plugin.app"
            COMMAND ${CMAKE_COMMAND} -E rm -rf "${ehl_staged_products}/au/ironpress_au_plugin.component"
            COMMAND ${CMAKE_COMMAND} -E copy_directory "${ehl_juce_products}/VST3/IronPress.vst3" "${ehl_staged_products}/vst3/ironpress_vst3_plugin.vst3"
            COMMAND ${CMAKE_COMMAND} -E copy_directory "${ehl_juce_products}/Standalone/IronPress.app" "${ehl_staged_products}/standalone/ironpress_standalone_plugin.app"
            COMMAND ${CMAKE_COMMAND} -E copy_directory "${ehl_juce_products}/AU/IronPress.component" "${ehl_staged_products}/au/ironpress_au_plugin.component"
            COMMAND codesign --force --deep --sign - "${ehl_staged_products}/vst3/ironpress_vst3_plugin.vst3"
            COMMAND codesign --force --deep --sign - "${ehl_staged_products}/standalone/ironpress_standalone_plugin.app"
            COMMAND codesign --force --deep --sign - "${ehl_staged_products}/au/ironpress_au_plugin.component")
        list(APPEND ehl_stage_dependencies IronPress_AU)
    elseif(WIN32)
        list(APPEND ehl_stage_commands
            COMMAND ${CMAKE_COMMAND} -E rm -rf "${ehl_staged_products}/vst3/ironpress_vst3_plugin.vst3"
            COMMAND ${CMAKE_COMMAND} -E copy_directory "${ehl_juce_products}/VST3/IronPress.vst3" "${ehl_staged_products}/vst3/ironpress_vst3_plugin.vst3"
            COMMAND ${CMAKE_COMMAND} -E copy "${ehl_juce_products}/Standalone/IronPress.exe" "${ehl_staged_products}/standalone/ironpress_standalone_plugin.exe")
    else()
        list(APPEND ehl_stage_commands
            COMMAND ${CMAKE_COMMAND} -E copy_directory "${ehl_juce_products}/VST3/IronPress.vst3" "${ehl_staged_products}/vst3/ironpress_vst3_plugin.vst3"
            COMMAND ${CMAKE_COMMAND} -E copy "${ehl_juce_products}/Standalone/IronPress" "${ehl_staged_products}/standalone/ironpress_standalone_plugin")
    endif()

    list(APPEND ehl_stage_commands
        COMMAND ${CMAKE_COMMAND}
            -DPRODUCT_NAME=IronPress
            -DSLUG=ironpress
            -DBUNDLE_ID=jp.ehl.ironpress
            -DPLUGIN_CODE=IrPr
            -DSTAGE_DIR=${ehl_staged_products}
            -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/WriteArtifactsManifest.cmake")

    add_custom_target(ehl_stage_products
        ${ehl_stage_commands}
        DEPENDS ${ehl_stage_dependencies}
        COMMENT "Staging IronPress products under ${IRONPRESS_ARTIFACT_ROOT}/plugin-release")

    if(IRONPRESS_BUILD_TESTS)
        add_executable(ironpress_plugin_tests Tests/PluginTests.cpp)
        target_include_directories(ironpress_plugin_tests PRIVATE Source)
        target_link_libraries(ironpress_plugin_tests PRIVATE IronPress juce::juce_audio_utils)
        add_test(NAME ironpress_plugin_tests COMMAND ironpress_plugin_tests)

        add_executable(ironpress_editor_tests Tests/EditorTests.cpp)
        target_include_directories(ironpress_editor_tests PRIVATE Source)
        target_link_libraries(ironpress_editor_tests PRIVATE IronPress EHL::JuceDesign juce::juce_audio_utils)
        add_test(NAME ironpress_editor_tests COMMAND ironpress_editor_tests)

        add_test(
            NAME ironpress_artifact_contract
            COMMAND ${CMAKE_COMMAND}
                -DSTAGE_DIR=${ehl_staged_products}
                -DSLUG=ironpress
                -DEXPECT_AU=$<BOOL:${APPLE}>
                -P "${CMAKE_CURRENT_SOURCE_DIR}/Tests/VerifyArtifacts.cmake")
    endif()
endif()
