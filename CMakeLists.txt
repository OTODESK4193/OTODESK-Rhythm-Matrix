cmake_minimum_required(VERSION 3.22)
project(AIDrumMachine VERSION 1.0.0)

# VST3の互換エラーと過去キャッシュの干渉を徹底排除
set(JUCE_VST3_AUTO_MANIFEST OFF)
add_compile_definitions(JUCE_VST3_CAN_REPLACE_VST2=0)

# 1. C++20の有効化（非同期処理に必須）
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 2. JUCEディレクトリの指定
set(JUCE_DIR "C:/JUCE")
add_subdirectory(${JUCE_DIR} build_juce)

# 3. プラグインの定義（IS_SYNTH TRUE に変更）
juce_add_plugin(AIDrumMachine
    COMPANY_NAME "OTODESK"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT TRUE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD FALSE 
    VST3_AUTO_MANIFEST FALSE
    PLUGIN_MANUFACTURER_CODE "Otdk"
    PLUGIN_CODE "Aidm"
    FORMATS VST3 Standalone
    PRODUCT_NAME "AI Drum Machine")

# 4. SIMD最適化フラグと文字コードエラー無視の設定
target_compile_options(AIDrumMachine PRIVATE
    $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>,$<CXX_COMPILER_ID:GNU>>:
        $<$<CONFIG:Release>:-O3 -ffast-math> -mavx2 -mfma
    >
    $<$<CXX_COMPILER_ID:MSVC>:
        $<$<CONFIG:Release>:/O2 /fp:fast> /arch:AVX2 /wd4819 /utf-8
    >
)

# 5. ソースコードの追加（Networkフォルダを追加）
target_sources(AIDrumMachine PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h
    Source/Network/GeminiClient.cpp
    Source/Network/GeminiClient.h
)

# 6. 使用するJUCEモジュールのリンク
target_link_libraries(AIDrumMachine PRIVATE
    juce::juce_audio_utils
    juce::juce_audio_processors
    juce::juce_dsp
    juce::juce_gui_basics
    juce::juce_gui_extra
    juce::juce_core        # HTTP通信・JSONパースに必要
    juce::juce_events      # 非同期メッセージに必要
)