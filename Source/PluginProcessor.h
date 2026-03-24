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

    // 4小節対応（1小節最大9分割 × 4 = 36）
    int drumPattern[8][36] = { {0} };
    int trackDivisions[8] = { 4, 4, 4, 4, 4, 4, 4, 4 }; // 各トラックの1小節あたりの分割数

    // 画面（Editor）が「今どこを再生しているか（0〜35）」を知るための関数
    int getTrackCurrentStep(int trackIndex) const {
        if (trackIndex >= 0 && trackIndex < 8) return trackCurrentStep[trackIndex];
        return 0;
    }

private:
    // ★修正: 4小節（ループ全体）の絶対同期用タイマー
    int samplesInLoop = 0;
    int trackCurrentStep[8] = { 0 }; // 各トラックの現在のステップ位置（0〜35）

    float trackEnv[8] = { 0.0f };      // 音量（アンプ）エンベロープ
    float trackPitchEnv[8] = { 0.0f }; // ピッチ（アタック感）エンベロープ
    float trackPhase[8] = { 0.0f };    // オシレーターの位相

    juce::Random random;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AIDrumMachineAudioProcessor)
};