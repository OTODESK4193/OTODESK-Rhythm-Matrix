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

// ★ シンセサイザーのパラメータ構造体
struct InstrumentPatch {
    int wave;          // 0:Sine, 1:Triangle, 2:Saw, 3:Square
    float freq;        // Base Frequency (Hz)
    float pDecay;      // Pitch Envelope Decay (ms)
    float pAmt;        // Pitch Envelope Amount (Multiplier)
    float aAtt;        // Amp Envelope Attack (ms)
    float aDec;        // Amp Envelope Decay (ms)
    float noise;       // Noise Mix Level
    int fType;         // Filter Type - 0:LP, 1:HP, 2:BP, 3:Off
    float fFreq;       // Filter Cutoff (Hz)
    float fRes;        // Filter Resonance (Q)
    float drive;       // Saturation Drive
    float vol;         // Output Volume
};

// ★ 50種類のパッチID
enum PatchID {
    // Kicks (10)
    K_909, K_808, K_Acoustic, K_Deep, K_Punch, K_Hard, K_Soft, K_Sub, K_Click, K_FM,
    // Snares & Claps (10)
    S_909, S_808, S_Tight, S_Fat, S_Rim, S_Clap, S_Snap, S_Noise, S_Lofi, S_Acoustic,
    // Hats & Cymbals (8)
    H_Closed, H_Open, H_Fast, H_Shaker, H_Tambourine, H_Ride, H_Crash, H_Metallic,
    // Percussions & Toms (12)
    P_TomL, P_TomM, P_TomH, P_Conga, P_Bongo, P_TablaL, P_TablaH, P_Wood, P_Cowbell, P_Gong, P_Clave, P_LogDrum,
    // FX & Synths (10)
    F_Noise, F_SubDrop, F_Chaos, F_Laser, F_Wobble, F_Pluck, F_Bell, F_Marimba, F_Chant, F_Sweep,
    PATCH_MAX
};

struct GenreDefinition {
    int defaultNum;
    int defaultDen;
    int minTempo;
    int maxTempo;
    const char* trackNames[8];
    PatchID trackPatches[8];
    int allowedDivs[8][4];
    int shiftMin[8];
    int shiftMax[8];
};

// ★ パターン保存用構造体
struct SavedPattern {
    int drumPattern[8][1024] = { {0} };
    int trackDivisions[8] = { 4, 4, 4, 4, 4, 4, 4, 4 };
    int trackShift[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
    int trackComplexity[8] = { 50, 50, 50, 50, 50, 50, 50, 50 };
    int trackEntropy[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
    int num = 4;
    int den = 4;
    int bars = 4;
};

class DrumVoice {
public:
    void setSampleRate(float sr) { sampleRate = sr; }

    void trigger(float velocity, const InstrumentPatch& p) {
        patch = p;
        phase = 0.0f;
        pEnv = 1.0f;
        aEnv = 0.0f;
        state = 1;
        svfLp = svfHp = svfBp = 0.0f;
        outVol = patch.vol * (velocity / 100.0f);
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

        float noiseSig = ((rand() % 2000) / 1000.0f - 1.0f) * patch.noise;
        float sig = osc + noiseSig;

        if (patch.fType != 3) {
            float f = 2.0f * std::sin(juce::MathConstants<float>::pi * patch.fFreq / sampleRate);
            float q = 1.0f / patch.fRes;
            svfLp += f * svfBp;
            svfHp = sig - svfLp - q * svfBp;
            svfBp += f * svfHp;

            if (patch.fType == 0) sig = svfLp;
            else if (patch.fType == 1) sig = svfHp;
            else sig = svfBp;
        }

        sig = std::tanh(sig * patch.drive);
        return sig * aEnv * outVol;
    }

private:
    float sampleRate = 48000.0f;
    float phase = 0.0f, pEnv = 0.0f, aEnv = 0.0f, outVol = 0.0f;
    int state = 0;
    float svfLp = 0.0f, svfHp = 0.0f, svfBp = 0.0f;
    InstrumentPatch patch;
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

    std::atomic<int> timeSigNumerator{ 4 };
    std::atomic<int> timeSigDenominator{ 4 };
    std::atomic<int> globalBarCount{ 4 };
    std::atomic<int> fillBarTarget{ 0 }; // 0:Off, 1:Bar1, 2:Bar2, 3:Bar3, 4:Bar4

    int drumPatternUI[8][1024] = { {0} };
    int trackDivisionsUI[8] = { 4, 4, 4, 4, 4, 4, 4, 4 };
    int trackShiftUI[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };

    std::atomic<bool> patternUpdated{ false };
    std::atomic<bool> uiNeedsUpdate{ false };
    std::atomic<int> currentPlayingBar{ 0 };

    bool trackLocked[8] = { false, false, false, false, false, false, false, false };
    bool trackDivLocked[8] = { false, false, false, false, false, false, false, false };
    bool trackCmplxLocked[8] = { true, true, false, false, true, false, false, false };
    int trackComplexity[8] = { 0, 0, 50, 50, 0, 30, 30, 30 };
    bool trackEntrpLocked[8] = { false, false, false, false, false, false, false, false };
    int trackEntropy[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
    bool trackShiftLocked[8] = { false, false, false, false, false, false, false, false };
    bool trackMuted[8] = { false, false, false, false, false, false, false, false };
    bool trackSoloed[8] = { false, false, false, false, false, false, false, false };

    std::atomic<int> currentGenre{ 0 };

    // パターン保存スロット
    SavedPattern savedPatterns[4];
    bool isPatternSaved[4] = { false, false, false, false };

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
        currentPlayingBar.store(0);
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
    static const InstrumentPatch& getPatch(PatchID id);

private:
    int drumPatternDSP[8][1024] = { {0} };
    int trackDivisionsDSP[8] = { 4, 4, 4, 4, 4, 4, 4, 4 };
    int trackShiftDSP[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
    int timeSigNumDSP = 4, timeSigDenDSP = 4, globalBarCountDSP = 4;
    int samplesInLoop = 0;
    int trackCurrentStep[8] = { -1, -1, -1, -1, -1, -1, -1, -1 };

    juce::Random random;
    DrumVoice synthVoices[8];

    juce::AudioFormatManager formatManager;
    juce::AudioSampleBuffer sampleBuffers[8];
    bool hasSample[8] = { false };
    int samplePlayPos[8] = { -1, -1, -1, -1, -1, -1, -1, -1 };
    float sampleVolume[8] = { 1.0f };
    juce::CriticalSection sampleLock;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AIDrumMachineAudioProcessor)
};