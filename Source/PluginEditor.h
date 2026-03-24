#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "Network/GeminiClient.h"

class AIDrumMachineAudioProcessorEditor : public juce::AudioProcessorEditor, public juce::Timer
{
public:
    AIDrumMachineAudioProcessorEditor(AIDrumMachineAudioProcessor&);
    ~AIDrumMachineAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

    void timerCallback() override;

private:
    AIDrumMachineAudioProcessor& audioProcessor;

    juce::TextButton generateButton{ "Generate 4-Bars" };
    juce::Label statusLabel;

    // ★追加: ページネーション用のタブボタンと変数
    juce::TextButton tabButton1{ "Bar 1" };
    juce::TextButton tabButton2{ "Bar 2" };
    juce::TextButton tabButton3{ "Bar 3" };
    juce::TextButton tabButton4{ "Bar 4" };
    int currentViewBar = 0; // 0=Bar1, 1=Bar2, 2=Bar3, 3=Bar4

    // タブの色を更新するヘルパー関数
    void updateTabColors();

    GeminiClient gemini;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AIDrumMachineAudioProcessorEditor)
};