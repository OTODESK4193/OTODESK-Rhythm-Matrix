// ==============================================================================
// Source/PluginProcessor.h
// ==============================================================================
#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <atomic>
#include <array>
#include <cstring>

// 24ジャンルの設定を定義する構造体
struct GenreDefinition {
    int defaultNum;
    int defaultDen;
    const char* trackNames[8];
    int allowedDivs[8][4]; // 各トラック最大4つのDiv候補（0は終端）
    int shiftMin[8];       // Micro-Shiftの下限
    int shiftMax[8];       // Micro-Shiftの上限
};

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

    // パラメータ（UI操作用）
    std::atomic<int> timeSigNumerator{ 4 };
    std::atomic<int> timeSigDenominator{ 4 };
    std::atomic<int> globalBarCount{ 4 };

    // メインバッファ（UIからはこちらを編集）
    int drumPatternUI[8][1024] = { {0} };
    int trackDivisionsUI[8] = { 4, 4, 4, 4, 4, 4, 4, 4 };
    int trackShiftUI[8] = { 0, 0, 0, 0, 0, 0, 0, 0 }; // -50 〜 50

    // DSPへの転送・UI再描画フラグ
    std::atomic<bool> patternUpdated{ false };
    std::atomic<bool> uiNeedsUpdate{ false };

    bool trackLocked[8] = { false, false, false, false, false, false, false, false };
    bool trackDivLocked[8] = { false, false, false, false, false, false, false, false };
    bool trackCmplxLocked[8] = { false, false, false, false, false, false, false, false };
    int trackComplexity[8] = { 50, 50, 50, 50, 50, 50, 50, 50 };

    bool trackEntrpLocked[8] = { false, false, false, false, false, false, false, false };
    int trackEntropy[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };

    bool trackShiftLocked[8] = { false, false, false, false, false, false, false, false };

    bool trackMuted[8] = { false, false, false, false, false, false, false, false };
    bool trackSoloed[8] = { false, false, false, false, false, false, false, false };

    std::atomic<int> currentGenre{ 0 };

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

    void loadSample(int trackIndex, const juce::String& filePath);
    bool hasSampleLoaded(int trackIndex) const { return hasSample[trackIndex]; }
    void clearSample(int trackIndex);

    void generateAllTracks();
    void shiftTrackLeft(int trackIndex);
    void shiftTrackRight(int trackIndex);
    void clearTrack(int trackIndex);

    static const GenreDefinition& getGenreDef(int index);

private:
    // DSPオーディオスレッド用バックバッファ（スレッドセーフ）
    int drumPatternDSP[8][1024] = { {0} };
    int trackDivisionsDSP[8] = { 4, 4, 4, 4, 4, 4, 4, 4 };
    int trackShiftDSP[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
    int timeSigNumDSP = 4;
    int timeSigDenDSP = 4;
    int globalBarCountDSP = 4;

    int samplesInLoop = 0;
    int trackCurrentStep[8] = { -1, -1, -1, -1, -1, -1, -1, -1 };

    float trackEnv[8] = { 0.0f };
    float trackPitchEnv[8] = { 0.0f };
    float trackPhase[8] = { 0.0f };
    juce::Random random;

    juce::AudioFormatManager formatManager;
    juce::AudioSampleBuffer sampleBuffers[8];
    bool hasSample[8] = { false };
    int samplePlayPos[8] = { -1, -1, -1, -1, -1, -1, -1, -1 };
    float sampleVolume[8] = { 1.0f };
    juce::CriticalSection sampleLock;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AIDrumMachineAudioProcessor)
};