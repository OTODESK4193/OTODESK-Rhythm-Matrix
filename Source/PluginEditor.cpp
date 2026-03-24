#include "PluginProcessor.h"
#include "PluginEditor.h"

AIDrumMachineAudioProcessorEditor::AIDrumMachineAudioProcessorEditor(AIDrumMachineAudioProcessor& p)
    : AudioProcessorEditor(&p)
{
    setSize(400, 300);
    addAndMakeVisible(generateButton);
    addAndMakeVisible(statusLabel);

    statusLabel.setText("Enter API Key in code or wait for result...", juce::dontSendNotification);

    generateButton.onClick = [this]
        {
            statusLabel.setText("Requesting AI...", juce::dontSendNotification);
            // ★ここにあなたのGemini APIキーを入れてください
            gemini.fetchDrumPattern("Fast Techno Beat", "AIzaSyBT2vQXyacUMdmNOF2OjkYYQ_OPtJgORtQ");
        };

    gemini.onSuccess = [this](const juce::var& data) {
        statusLabel.setText("AI Success! JSON received.", juce::dontSendNotification);
        juce::Logger::writeToLog(juce::JSON::toString(data));
        };

    gemini.onError = [this](const juce::String& err) {
        statusLabel.setText("Error: " + err, juce::dontSendNotification);
        };
}

AIDrumMachineAudioProcessorEditor::~AIDrumMachineAudioProcessorEditor() {}

void AIDrumMachineAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::darkgrey);
}

void AIDrumMachineAudioProcessorEditor::resized()
{
    generateButton.setBounds(100, 100, 200, 50);
    statusLabel.setBounds(20, 160, 360, 100);
}