#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
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

    // ★追加：トランスポート（再生・同期）用の変数
    std::atomic<bool> isSyncEnabled{ false };
    std::atomic<bool> isPlayingInternal{ false };
    std::atomic<double> internalTempo{ 120.0 };
    std::atomic<double> currentBpm{ 120.0 }; // UI表示用（現在適用されているBPM）

    // ★追加：頭出し（ストップ2回押し）用関数
    void resetPosition() {
        samplesInLoop = 0;
        for (int i = 0; i < 8; ++i) {
            trackCurrentStep[i] = -1; // 次の再生時に確実にトリガーさせるため -1 にリセット
        }
    }

private:
    int samplesInLoop = 0;
    int trackCurrentStep[8] = { -1, -1, -1, -1, -1, -1, -1, -1 }; // 初期値を-1に変更

    float trackEnv[8] = { 0.0f };
    float trackPitchEnv[8] = { 0.0f };
    float trackPhase[8] = { 0.0f };

    juce::Random random;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AIDrumMachineAudioProcessor)
};