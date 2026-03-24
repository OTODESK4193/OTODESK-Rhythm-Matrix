#pragma once
#include "PluginProcessor.h"
#include "Network/GeminiClient.h"

class AIDrumMachineAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    AIDrumMachineAudioProcessorEditor(AIDrumMachineAudioProcessor&);
    ~AIDrumMachineAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    juce::TextButton generateButton{ "Generate AI Beat" };
    juce::Label statusLabel;
    GeminiClient gemini;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AIDrumMachineAudioProcessorEditor)
};