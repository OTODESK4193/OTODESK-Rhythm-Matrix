#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

class AIDrumMachineAudioProcessor : public juce::AudioProcessor
{
public:
    AIDrumMachineAudioProcessor();
    ~AIDrumMachineAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

#ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
#endif

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    // AIから受け取るリズムデータ
    int drumPattern[8][16] = { {0} };

    // ★追加：画面（Editor）が「今どこを再生しているか」を知るための関数
    int getCurrentStep() const { return currentStep; }

private:
    // シーケンサーと音作り用の変数
    int currentStep = 0;
    int samplesSinceLastStep = 0;
    float trackEnv[8] = { 0.0f };
    float trackPhase[8] = { 0.0f };

    // ★追加：スネアやハイハットのノイズを作るための乱数発生器
    juce::Random random;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AIDrumMachineAudioProcessor)
};