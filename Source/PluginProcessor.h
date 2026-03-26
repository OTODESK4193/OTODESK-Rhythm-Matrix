// ==============================================================================
// Source/PluginProcessor.h
// ==============================================================================
#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <atomic>
#include <array>
#include <cstring>
#include <vector>

struct InstrumentPatch {
    int wave; float freq; float pDecay; float pAmt;
    float aAtt; float aDec; float noise; int fType;
    float fFreq; float fRes; float drive; float vol;
};

// ★ 22ジャンル×8トラック = 176パッチ + Pluck(8) + Arp(1) = 185パッチに完全独立拡張
enum PatchID {
    G0_T0, G0_T1, G0_T2, G0_T3, G0_T4, G0_T5, G0_T6, G0_T7,
    G1_T0, G1_T1, G1_T2, G1_T3, G1_T4, G1_T5, G1_T6, G1_T7,
    G2_T0, G2_T1, G2_T2, G2_T3, G2_T4, G2_T5, G2_T6, G2_T7,
    G3_T0, G3_T1, G3_T2, G3_T3, G3_T4, G3_T5, G3_T6, G3_T7,
    G4_T0, G4_T1, G4_T2, G4_T3, G4_T4, G4_T5, G4_T6, G4_T7,
    G5_T0, G5_T1, G5_T2, G5_T3, G5_T4, G5_T5, G5_T6, G5_T7,
    G6_T0, G6_T1, G6_T2, G6_T3, G6_T4, G6_T5, G6_T6, G6_T7,
    G7_T0, G7_T1, G7_T2, G7_T3, G7_T4, G7_T5, G7_T6, G7_T7,
    G8_T0, G8_T1, G8_T2, G8_T3, G8_T4, G8_T5, G8_T6, G8_T7,
    G9_T0, G9_T1, G9_T2, G9_T3, G9_T4, G9_T5, G9_T6, G9_T7,
    G10_T0, G10_T1, G10_T2, G10_T3, G10_T4, G10_T5, G10_T6, G10_T7,
    G11_T0, G11_T1, G11_T2, G11_T3, G11_T4, G11_T5, G11_T6, G11_T7,
    G12_T0, G12_T1, G12_T2, G12_T3, G12_T4, G12_T5, G12_T6, G12_T7,
    G13_T0, G13_T1, G13_T2, G13_T3, G13_T4, G13_T5, G13_T6, G13_T7,
    G14_T0, G14_T1, G14_T2, G14_T3, G14_T4, G14_T5, G14_T6, G14_T7,
    G15_T0, G15_T1, G15_T2, G15_T3, G15_T4, G15_T5, G15_T6, G15_T7,
    G16_T0, G16_T1, G16_T2, G16_T3, G16_T4, G16_T5, G16_T6, G16_T7,
    G17_T0, G17_T1, G17_T2, G17_T3, G17_T4, G17_T5, G17_T6, G17_T7,
    G18_T0, G18_T1, G18_T2, G18_T3, G18_T4, G18_T5, G18_T6, G18_T7,
    G19_T0, G19_T1, G19_T2, G19_T3, G19_T4, G19_T5, G19_T6, G19_T7,
    G20_T0, G20_T1, G20_T2, G20_T3, G20_T4, G20_T5, G20_T6, G20_T7,
    G21_T0, G21_T1, G21_T2, G21_T3, G21_T4, G21_T5, G21_T6, G21_T7,
    PLUCK_1, PLUCK_2, PLUCK_3, PLUCK_4, PLUCK_5, PLUCK_6, PLUCK_7, PLUCK_8,
    M_ARP,
    PATCH_MAX
};

struct GenreDefinition {
    int defaultNum; int defaultDen; int minTempo; int maxTempo;
    const char* trackNames[8]; PatchID trackPatches[8];
    int allowedDivs[8][4]; int shiftMin[8]; int shiftMax[8];
};

struct SavedPattern {
    int drumPattern[8][1024] = { {0} };
    int trackDivisions[8] = { 4, 4, 4, 4, 4, 4, 4, 4 };
    int trackShift[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
    int trackComplexity[8] = { 50, 50, 50, 50, 50, 50, 50, 50 };
    int trackEntropy[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
    int num = 4; int den = 4; int bars = 4;
};

const int scalePatterns[12][7] = {
    {0, 2, 4, 5, 7, 9, 11}, {0, 2, 3, 5, 7, 8, 10}, {0, 2, 4, 7, 9, -1, -1}, {0, 3, 5, 7, 10, -1, -1},
    {0, 2, 3, 5, 7, 9, 10}, {0, 2, 3, 5, 7, 8, 11}, {0, 2, 4, 6, 7, 9, 11}, {0, 2, 4, 5, 7, 9, 10},
    {0, 1, 3, 5, 7, 8, 10}, {0, 1, 3, 5, 6, 8, 10}, {0, 2, 4, 6, 8, 10, -1},{0, 3, 5, 6, 7, 10, -1}
};
const int scaleLengths[12] = { 7, 7, 5, 5, 7, 7, 7, 7, 7, 7, 6, 6 };

class DrumVoice {
public:
    void setSampleRate(float sr) { sampleRate = sr; }
    void trigger(float velocity, const InstrumentPatch& p) {
        patch = p; phase = 0.0f; pEnv = 1.0f; aEnv = 0.0f; state = 1;
        svfLp = svfHp = svfBp = 0.0f; outVol = patch.vol * (velocity / 100.0f);
    }
    float process() {
        if (state == 0) return 0.0f;
        if (state == 1) {
            aEnv += 1000.0f / (patch.aAtt * sampleRate + 1.0f);
            if (aEnv >= 1.0f) { aEnv = 1.0f; state = 2; }
        }
        else {
            aEnv *= std::exp(-1.0f / (patch.aDec * sampleRate * 0.001f + 1.0f));
            if (aEnv < 0.0001f) state = 0;
        }
        pEnv *= std::exp(-1.0f / (patch.pDecay * sampleRate * 0.001f + 1.0f));
        float currentFreq = patch.freq * (1.0f + pEnv * patch.pAmt);
        phase += currentFreq / sampleRate;
        if (phase > 1.0f) phase -= 1.0f;

        float osc = 0.0f;
        if (patch.wave == 0) osc = std::sin(phase * juce::MathConstants<float>::twoPi);
        else if (patch.wave == 1) osc = 2.0f * std::abs(2.0f * phase - 1.0f) - 1.0f;
        else if (patch.wave == 2) osc = 2.0f * phase - 1.0f;
        else if (patch.wave == 3) osc = phase < 0.5f ? 1.0f : -1.0f;

        float sig = osc + (((rand() % 2000) / 1000.0f - 1.0f) * patch.noise);

        if (patch.fType != 3) {
            float f = 2.0f * std::sin(juce::MathConstants<float>::pi * patch.fFreq / sampleRate);
            float q = 1.0f / patch.fRes;
            svfLp += f * svfBp; svfHp = sig - svfLp - q * svfBp; svfBp += f * svfHp;
            if (patch.fType == 0) sig = svfLp;
            else if (patch.fType == 1) sig = svfHp;
            else sig = svfBp;
        }
        return std::tanh(sig * patch.drive) * aEnv * outVol;
    }
private:
    float sampleRate = 48000.0f;
    float phase = 0.0f, pEnv = 0.0f, aEnv = 0.0f, outVol = 0.0f;
    int state = 0;
    float svfLp = 0.0f, svfHp = 0.0f, svfBp = 0.0f;
    InstrumentPatch patch;
};

class AIDrumMachineAudioProcessor : public juce::AudioProcessor {
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
    bool acceptsMidi() const override; bool producesMidi() const override; bool isMidiEffect() const override;
    double getTailLengthSeconds() const override; int getNumPrograms() override;
    int getCurrentProgram() override; void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override; void changeProgramName(int index, const juce::String& newName) override;
    void getStateInformation(juce::MemoryBlock& destData) override; void setStateInformation(const void* data, int sizeInBytes) override;

    std::atomic<int> timeSigNumerator{ 4 }; std::atomic<int> timeSigDenominator{ 4 };
    std::atomic<int> globalBarCount{ 4 }; std::atomic<int> fillBarTarget{ 0 };

    int drumPatternUI[8][1024] = { {0} };
    int trackDivisionsUI[8] = { 4, 4, 4, 4, 4, 4, 4, 4 };
    int trackShiftUI[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };

    std::atomic<bool> patternUpdated{ false }; std::atomic<bool> uiNeedsUpdate{ false }; std::atomic<int> currentPlayingBar{ 0 };
    bool trackLocked[8] = { false, false, false, false, false, false, false, false };
    bool trackDivLocked[8] = { false, false, false, false, false, false, false, false };
    bool trackCmplxLocked[8] = { true, true, false, false, true, false, false, false };
    int trackComplexity[8] = { 0, 0, 50, 50, 0, 30, 30, 30 };
    bool trackEntrpLocked[8] = { false, false, false, false, false, false, false, false };
    int trackEntropy[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
    bool trackShiftLocked[8] = { false, false, false, false, false, false, false, false };
    bool trackMuted[8] = { false, false, false, false, false, false, false, false };
    bool trackSoloed[8] = { false, false, false, false, false, false, false, false };

    std::atomic<bool> arpMode{ false }; std::atomic<bool> arpMono{ true }; std::atomic<int> arpKey{ 0 }; std::atomic<int> arpScale{ 1 };
    int trackOctaveUI[8] = { -1, -1, 0, 0, 1, 1, 2, 2 }; int trackDegreeUI[8] = { 0, 2, 4, 0, 2, 4, 0, 2 };

    std::atomic<int> currentGenre{ 0 }; SavedPattern savedPatterns[4]; bool isPatternSaved[4] = { false, false, false, false };
    std::atomic<bool> isSyncEnabled{ false }; std::atomic<bool> isPlayingInternal{ false }; std::atomic<bool> autoFollowEnabled{ true };
    std::atomic<bool> tempoLocked{ false }; std::atomic<double> internalTempo{ 120.0 }; std::atomic<double> currentBpm{ 120.0 };

    int getTrackCurrentStep(int trackIndex) const { return (trackIndex >= 0 && trackIndex < 8) ? trackCurrentStep[trackIndex] : 0; }
    void resetPosition() { samplesInLoop = 0; currentPlayingBar.store(0); for (int i = 0; i < 8; ++i) trackCurrentStep[i] = -1; }
    void loadSample(int trackIndex, const juce::String& filePath); bool hasSampleLoaded(int trackIndex) const { return hasSample[trackIndex]; }
    void clearSample(int trackIndex); void generateAllTracks(); void shiftTrackLeft(int trackIndex); void shiftTrackRight(int trackIndex); void clearTrack(int trackIndex);

    // ★ constを追加
    static const GenreDefinition& getGenreDef(int index);
    static const InstrumentPatch& getPatch(PatchID id);
    juce::String getNoteName(int trackIndex) const;

private:
    int drumPatternDSP[8][1024] = { {0} };
    int trackDivisionsDSP[8] = { 4, 4, 4, 4, 4, 4, 4, 4 };
    int trackShiftDSP[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
    int trackOctaveDSP[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
    int trackDegreeDSP[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };

    int timeSigNumDSP = 4, timeSigDenDSP = 4, globalBarCountDSP = 4, samplesInLoop = 0;
    int trackCurrentStep[8] = { -1, -1, -1, -1, -1, -1, -1, -1 };

    juce::Random random; DrumVoice synthVoices[8]; juce::AudioFormatManager formatManager;
    juce::AudioSampleBuffer sampleBuffers[8]; bool hasSample[8] = { false, false, false, false, false, false, false, false };
    int samplePlayPos[8] = { -1, -1, -1, -1, -1, -1, -1, -1 };
    float sampleVolume[8] = { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f };
    juce::CriticalSection sampleLock;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AIDrumMachineAudioProcessor)
};