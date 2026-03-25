#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_formats/juce_audio_formats.h> // ★追加：オーディオ形式管理用
#include <atomic>

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

    int drumPattern[8][36] = { {0} };
    int trackDivisions[8] = { 4, 4, 4, 4, 4, 4, 4, 4 };
    bool trackLocked[8] = { false, false, false, false, false, false, false, false };

    int getTrackCurrentStep(int trackIndex) const {
        if (trackIndex >= 0 && trackIndex < 8) return trackCurrentStep[trackIndex];
        return 0;
    }

    std::atomic<bool> isSyncEnabled{ false };
    std::atomic<bool> isPlayingInternal{ false };
    std::atomic<double> internalTempo{ 120.0 };
    std::atomic<double> currentBpm{ 120.0 };

    void resetPosition() {
        samplesInLoop = 0;
        for (int i = 0; i < 8; ++i) trackCurrentStep[i] = -1;
    }

    // ★追加：サンプラー機能の関数と変数
    void loadSample(int trackIndex, const juce::String& filePath);
    bool hasSampleLoaded(int trackIndex) const { return hasSample[trackIndex]; }

private:
    int samplesInLoop = 0;
    int trackCurrentStep[8] = { -1, -1, -1, -1, -1, -1, -1, -1 };

    // シンセ用変数
    float trackEnv[8] = { 0.0f };
    float trackPitchEnv[8] = { 0.0f };
    float trackPhase[8] = { 0.0f };
    juce::Random random;

    // ★追加：サンプラー用変数群
    juce::AudioFormatManager formatManager;
    juce::AudioSampleBuffer sampleBuffers[8];
    bool hasSample[8] = { false };
    int samplePlayPos[8] = { -1, -1, -1, -1, -1, -1, -1, -1 };
    float sampleVolume[8] = { 1.0f };
    juce::CriticalSection sampleLock; // オーディオスレッド保護用ロック

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AIDrumMachineAudioProcessor)
};