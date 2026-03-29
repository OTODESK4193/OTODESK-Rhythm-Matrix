// ==============================================================================
// Source/PluginProcessor.h
// ==============================================================================
#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
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

// ★ 音色の統一：808系基本パッチとArp/Pluckパッチのみに圧縮
enum PatchID {
    P_808_KICK, P_808_SNARE, P_808_CHH, P_808_OHH,
    P_808_CLAP, P_808_TOM, P_808_PERC, P_808_BASS, P_808_FX,
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

struct TuningRange { int min; int max; };
struct TimeSigDef { int num; int den; };

struct TrackTuning {
    bool allowedDivs[8] = { false, false, false, false, false, false, false, false }; // デフォルトゼロ
    bool divLocked = false;
    TuningRange cmplx; bool cmplxLocked = false;
    TuningRange entrp; bool entrpLocked = false;
    TuningRange shift; bool shiftLocked = false;
};

struct GenreTuning {
    TuningRange tempo; bool tempoLocked = false;
    bool allowedTimeSigs[8] = { false, false, false, false, false, false, false, false }; // デフォルトゼロ
    TimeSigDef timeSigOptions[8] = { {4,4}, {3,4}, {5,4}, {7,8}, {12,8}, {13,8}, {15,16}, {5,8} };
    bool allowedFills[4] = { false, false, false, false }; // デフォルトでジャンルごとに設定
    TrackTuning tracks[8];
};

// ★ スケールを19種類、最大8音構成に拡張
extern const int scalePatterns[19][8];
extern const int scaleLengths[19];

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

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    // ★ 修正点: 配列サイズを24から26へ拡張し、メモリ破壊を防止
    GenreTuning userTuning[26];
    void initializeUserTunings();

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

    // ★ Setting2 (Arp) のロックとDynamic用フラグ
    bool trackDegreeLocked[8] = { false, false, false, false, false, false, false, false };
    bool trackOctaveLocked[8] = { false, false, false, false, false, false, false, false };
    bool trackDynamic[8] = { false, false, false, false, false, false, false, false };
    int trackDynamicAmount[8] = { 30, 30, 30, 30, 30, 30, 30, 30 }; // Dynamicスライダー用

    std::atomic<bool> arpMode{ false }; std::atomic<bool> arpMono{ true }; std::atomic<int> arpKey{ 0 }; std::atomic<int> arpScale{ 1 };
    int trackOctaveUI[8] = { -1, -1, 0, 0, 1, 1, 2, 2 }; int trackDegreeUI[8] = { 0, 2, 4, 0, 2, 4, 0, 2 };

    std::atomic<int> currentGenre{ 0 }; SavedPattern savedPatterns[4]; bool isPatternSaved[4] = { false, false, false, false };
    std::atomic<bool> isSyncEnabled{ false }; std::atomic<bool> isPlayingInternal{ false }; std::atomic<bool> autoFollowEnabled{ true };
    std::atomic<bool> tempoLocked{ false }; std::atomic<double> internalTempo{ 120.0 }; std::atomic<double> currentBpm{ 120.0 };
    std::atomic<bool> timeSigLocked{ false }; // ★ Time Sigのロック用

    // ★ Editorからアクセスできるよう public へ移動
    juce::Random random;

    int getTrackCurrentStep(int trackIndex) const { return (trackIndex >= 0 && trackIndex < 8) ? trackCurrentStep[trackIndex] : 0; }
    void resetPosition() { samplesInLoop = 0; currentPlayingBar.store(0); for (int i = 0; i < 8; ++i) trackCurrentStep[i] = -1; }
    bool loadSample(int trackIndex, const juce::String& filePath); bool hasSampleLoaded(int trackIndex) const { return hasSample[trackIndex]; } // ★ void を bool に変更！
    void clearSample(int trackIndex); void generateAllTracks(); void shiftTrackLeft(int trackIndex); void shiftTrackRight(int trackIndex); void clearTrack(int trackIndex);

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

    DrumVoice synthVoices[8]; juce::AudioFormatManager formatManager;
    juce::AudioSampleBuffer sampleBuffers[8]; bool hasSample[8] = { false, false, false, false, false, false, false, false };
    // ★ ここを追加：サンプルのファイルパスを記憶しておくための配列
    juce::String loadedSamplePaths[8];
    int samplePlayPos[8] = { -1, -1, -1, -1, -1, -1, -1, -1 };
    float sampleVolume[8] = { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f };
    juce::CriticalSection sampleLock;
    // =========================================================================
        // ★ ここを追加：MIDI出力用のノート状態管理
        // =========================================================================
    int activeNote[8] = { -1, -1, -1, -1, -1, -1, -1, -1 };
    int noteOffCountdown[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AIDrumMachineAudioProcessor)
};