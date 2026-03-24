#include "PluginProcessor.h"
#include "PluginEditor.h"

AIDrumMachineAudioProcessor::AIDrumMachineAudioProcessor()
    : AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true))
{
}

AIDrumMachineAudioProcessor::~AIDrumMachineAudioProcessor() {}

juce::AudioProcessorEditor* AIDrumMachineAudioProcessor::createEditor()
{
    return new AIDrumMachineAudioProcessorEditor(*this);
}

// 以下の関数はホスト(DAW)にプラグインを作成させるために必須です
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AIDrumMachineAudioProcessor();
}