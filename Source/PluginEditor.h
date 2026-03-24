#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "Network/GeminiClient.h"

// ★追加：public juce::Timer を追加して、アニメーションできるようにする
class AIDrumMachineAudioProcessorEditor : public juce::AudioProcessorEditor, public juce::Timer
{
public:
    AIDrumMachineAudioProcessorEditor(AIDrumMachineAudioProcessor&);
    ~AIDrumMachineAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

    // ★追加：タイマーが呼ばれるたびに実行される関数
    void timerCallback() override;

private:
    AIDrumMachineAudioProcessor& audioProcessor;

    juce::TextButton generateButton{ "Generate Rhythm" };
    juce::Label statusLabel;

    GeminiClient gemini;
    int drumPattern[8][16] = { {0} };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AIDrumMachineAudioProcessorEditor)
};