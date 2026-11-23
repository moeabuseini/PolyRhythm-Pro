cmake_minimum_required(VERSION 3.22)

project(PolyRhythmPro VERSION 0.2.0 LANGUAGES C CXX)

include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 9.0.6
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(JUCE)

juce_add_gui_app(PolyRhythmPro
    PRODUCT_NAME "PolyRhythm Pro"
    COMPANY_NAME "PolyRhythm Pro"
    BUNDLE_ID "com.polyrhythmpro.desktop"
)

juce_generate_juce_header(PolyRhythmPro)

target_sources(PolyRhythmPro PRIVATE
    Source/Main.cpp
    Source/MainComponent.cpp
    Source/StudioActions.cpp
    Source/VideoPlayback.cpp
    Source/RhythmModel.cpp
    Source/TransportAudioEngine.cpp
    Source/RhythmScene.cpp
    Source/Landscape.cpp
)

target_compile_features(PolyRhythmPro PRIVATE cxx_std_17)
target_compile_definitions(PolyRhythmPro PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_APPLICATION_NAME_STRING="$<TARGET_PROPERTY:PolyRhythmPro,JUCE_PRODUCT_NAME>"
    JUCE_APPLICATION_VERSION_STRING="$<TARGET_PROPERTY:PolyRhythmPro,JUCE_VERSION>"
)

target_link_libraries(PolyRhythmPro PRIVATE
    juce::juce_dsp
    juce::juce_audio_utils
    juce::juce_gui_extra
    PUBLIC juce::juce_recommended_config_flags
           juce::juce_recommended_lto_flags
           juce::juce_recommended_warning_flags
)

# Retain matching application symbols for local Release minidumps.
if(MSVC)
    set_property(SOURCE Source/Main.cpp Source/MainComponent.cpp Source/StudioActions.cpp
        Source/VideoPlayback.cpp Source/TransportAudioEngine.cpp Source/RhythmScene.cpp Source/Landscape.cpp
        APPEND PROPERTY COMPILE_OPTIONS "$<$<CONFIG:Release>:/Zi>")
    target_link_options(PolyRhythmPro PRIVATE "$<$<CONFIG:Release>:/DEBUG:FULL>")
endif()
