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

juce_add_binary_data(VideoScores NAMESPACE VideoScores HEADER_NAME VideoScores.h
    SOURCES Presets/Video/FireflyFloat.xml Presets/Video/InfinitySpiral.xml)
target_link_libraries(PolyRhythmPro PRIVATE VideoScores)

juce_generate_juce_header(PolyRhythmPro)

set(app_sources
    Source/App/Main.cpp
    Source/App/MainComponent.cpp
    Source/Audio/TransportAudioEngine.cpp
    Source/Producer/ProducerTools.cpp
    Source/Rhythm/RhythmModel.cpp
    Source/Stage/Landscape.cpp
    Source/Stage/RhythmScene.cpp
    Source/Studio/StudioActions.cpp
    Source/Video/VideoPlayback.cpp
)
set(app_headers
    Source/App/MainComponent.h
    Source/Audio/Instrument.h
    Source/Audio/TransportAudioEngine.h
    Source/Producer/ProducerPanel.h
    Source/Rhythm/BeatGrid.h
    Source/Rhythm/NodeVoicing.h
    Source/Rhythm/RhythmModel.h
    Source/Stage/Landscape.h
    Source/Stage/RhythmScene.h
    Source/Studio/StudioSettings.h
    Source/UI/GlassTheme.h
    Source/UI/LayerInspector.h
    Source/UI/PerformanceSlider.h
    Source/UI/SaveReceipt.h
    Source/UI/SceneControls.h
    Source/UI/SettingsSheet.h
    Source/UI/VoicingPanel.h
    Source/Video/VideoEditor.h
)
target_sources(PolyRhythmPro PRIVATE ${app_sources} ${app_headers})
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/Source" PREFIX "Source" FILES ${app_sources} ${app_headers})

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
    set_property(SOURCE ${app_sources}
        APPEND PROPERTY COMPILE_OPTIONS "$<$<CONFIG:Release>:/Zi>")
    target_link_options(PolyRhythmPro PRIVATE "$<$<CONFIG:Release>:/DEBUG:FULL>")
endif()
