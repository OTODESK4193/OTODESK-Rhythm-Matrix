// ==============================================================================
// Source/PluginProcessor.cpp
// ==============================================================================
#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <algorithm>
#include <cmath>
#include <numeric>
#include <cstdint>

static constexpr InstrumentPatch P(int w, float fr, float pD, float pA, float aAt, float aDc, float ns, int fT, float fF, float fR, float dr, float vl) {
    return { w, fr, pD, pA, aAt, aDc, ns, fT, fF, fR, dr, vl };
}

// ★ 音色の統一: 808系基本パッチ + Pluck/Arp
static const std::array<InstrumentPatch, PATCH_MAX> patchLibrary = []() {
    std::array<InstrumentPatch, PATCH_MAX> arr{};
    arr[P_808_KICK] = P(0, 45.0f, 15.0f, 4.0f, 1.0f, 45.0f, 0.0f, 0, 800.0f, 1.0f, 2.5f, 1.2f);
    arr[P_808_SNARE] = P(2, 180.0f, 6.0f, 1.5f, 1.0f, 18.0f, 0.8f, 0, 4000.0f, 1.2f, 3.0f, 0.9f);
    arr[P_808_CHH] = P(3, 800.0f, 3.0f, 0.0f, 1.0f, 6.0f, 1.0f, 1, 5000.0f, 1.0f, 1.5f, 0.6f);
    arr[P_808_OHH] = P(3, 800.0f, 4.0f, 0.0f, 1.0f, 18.0f, 1.0f, 1, 4000.0f, 1.0f, 1.5f, 0.6f);
    arr[P_808_CLAP] = P(2, 120.0f, 6.0f, 0.0f, 2.0f, 20.0f, 1.0f, 2, 1500.0f, 1.0f, 2.5f, 0.9f);
    arr[P_808_TOM] = P(0, 150.0f, 8.0f, 1.5f, 1.0f, 25.0f, 0.1f, 0, 1000.0f, 1.0f, 2.0f, 1.0f);
    arr[P_808_PERC] = P(0, 400.0f, 4.0f, 1.5f, 1.0f, 8.0f, 0.1f, 2, 1200.0f, 3.0f, 2.0f, 1.0f);
    arr[P_808_BASS] = P(0, 55.0f, 8.0f, 0.5f, 5.0f, 40.0f, 0.0f, 0, 400.0f, 1.0f, 3.0f, 1.1f);
    arr[P_808_FX] = P(3, 1500.0f, 1.0f, 0.0f, 1.0f, 5.0f, 0.4f, 2, 4500.0f, 4.0f, 2.0f, 0.5f);

    for (int i = 0; i < 8; ++i) {
        float f = static_cast<float>(130.81 * std::pow(1.05946, i * 2));
        arr[PLUCK_1 + i] = P(2, f, 12.0f, 0.0f, 1.0f, 25.0f, 0.0f, 0, 1500.0f, 2.0f, 2.0f, 1.0f);
    }
    arr[M_ARP] = P(1, 440.0f, 5.0f, 0.0f, 0.5f, 15.0f, 0.0f, 0, 2500.0f, 1.5f, 1.5f, 1.0f);

    return arr;
    }();

static const std::array<GenreDefinition, 24> genreTable = { {
    { 4, 4, 125, 135, {"909 Kick", "909 Snare", "CHH", "OHH", "Clap", "Ride", "Tom", "Noise FX"}, {P_808_KICK, P_808_SNARE, P_808_CHH, P_808_OHH, P_808_CLAP, P_808_TOM, P_808_PERC, P_808_FX}, {{1,2,4,0}, {2,4,0,0}, {4,8,0,0}, {4,8,0,0}, {2,4,0,0}, {3,4,5,0}, {3,5,7,0}, {2,4,8,0}}, {0,0,0,0,0,0,0,0}, {0,0,2,2,0,5,5,10} },
    { 4, 4, 120, 126, {"Deep Kick", "Rimshot", "Shuff Hat", "Open Hat", "Clap", "Conga", "Bongo", "Vocal Chop"}, {P_808_KICK, P_808_SNARE, P_808_CHH, P_808_OHH, P_808_CLAP, P_808_TOM, P_808_PERC, P_808_FX}, {{1,2,4,0}, {2,4,0,0}, {4,6,0,0}, {2,4,0,0}, {2,4,0,0}, {3,5,0,0}, {4,6,8,0}, {2,3,4,0}}, {-2,0, 5,0,-2, -5, -5, 0}, {2,5, 15,5, 5, 10, 10, 15} },
    { 4, 4, 130, 138, {"Punch Kick", "Snare/Rim", "Garage Hat", "Ride", "Clap", "Sub Bass", "Perc 1", "Perc 2"}, {P_808_KICK, P_808_SNARE, P_808_CHH, P_808_OHH, P_808_CLAP, P_808_TOM, P_808_BASS, P_808_FX}, {{2,3,4,0}, {2,4,0,0}, {4,6,0,0}, {4,6,0,0}, {2,4,0,0}, {2,3,4,0}, {3,5,0,0}, {5,7,0,0}}, {-5,5, 10,5, 0, -10, 0, 0}, {5,15, 25,15, 10, 10, 15, 15} },
    { 4, 4, 165, 175, {"Heavy Kick", "Tight Snr", "Fast Hat", "Ride", "Break Rim", "Break 1", "Break 2", "Sub"}, {P_808_KICK, P_808_SNARE, P_808_CHH, P_808_OHH, P_808_CLAP, P_808_TOM, P_808_BASS, P_808_FX}, {{2,4,0,0}, {2,4,0,0}, {4,8,0,0}, {4,8,0,0}, {2,4,0,0}, {4,6,8,0}, {4,6,8,0}, {2,4,0,0}}, {0,0, -5,-5, 0, -10, -10, 0}, {2,2, 5,5, 5, 10, 10, 5} },
    { 4, 4, 135, 150, {"808 Kick", "808 Snare", "Roll Hat", "Open Hat", "Clap", "Perc", "808 Bass", "FX"}, {P_808_KICK, P_808_SNARE, P_808_CHH, P_808_OHH, P_808_CLAP, P_808_TOM, P_808_BASS, P_808_FX}, {{2,4,0,0}, {2,4,0,0}, {4,6,8,0}, {2,4,0,0}, {2,4,0,0}, {4,8,0,0}, {2,4,0,0}, {2,4,0,0}}, {0,0, 0,0, 0, 0, 0, 0}, {0,0, 0,0, 0, 0, 0, 0} },
    { 4, 4, 160, 160, {"Juke Kick", "Snare", "Fast Hat", "Hat 2", "Clap", "Tom", "Vocal 1", "Vocal 2"}, {P_808_KICK, P_808_SNARE, P_808_CHH, P_808_OHH, P_808_CLAP, P_808_TOM, P_808_PERC, P_808_FX}, {{3,4,6,0}, {3,4,6,0}, {4,6,8,0}, {4,6,8,0}, {2,4,0,0}, {3,5,6,0}, {3,4,5,0}, {4,6,7,0}}, {-5,-5, -5,-5, 0, -10, -5, -5}, {5,5, 5,5, 5, 10, 15, 15} },
    { 4, 4, 140, 180, {"Glitch Kick", "Drill Snr", "Hat 1", "Hat 2", "Noise", "Perc 1", "Perc 2", "Glitch FX"}, {P_808_KICK, P_808_SNARE, P_808_CHH, P_808_OHH, P_808_CLAP, P_808_TOM, P_808_PERC, P_808_FX}, {{4,5,7,0}, {4,6,8,0}, {5,7,9,0}, {6,8,9,0}, {3,5,7,0}, {5,7,9,0}, {4,6,8,0}, {3,5,7,0}}, {-10,-10, -15,-15, -20, -20, -20, -20}, {10,10, 15,15, 20, 20, 20, 20} },
    { 4, 4, 140, 150, {"Stomp Kick", "Fat Snare", "Hat", "Ride", "Clap", "Wobble", "Growl", "Sub"}, {P_808_KICK, P_808_SNARE, P_808_CHH, P_808_OHH, P_808_CLAP, P_808_TOM, P_808_BASS, P_808_FX}, {{2,4,0,0}, {2,4,0,0}, {4,6,0,0}, {2,4,0,0}, {2,4,0,0}, {2,4,8,0}, {2,4,0,0}, {1,2,4,0}}, {0,0, 0,0, 0, 0, 0, 0}, {0,0, 5,5, 0, 10, 5, 0} },
    { 4, 4, 95, 115,  {"Acoustic Kick", "Snare", "Shaker 1", "Shaker 2", "Clave", "Conga", "Djembe", "Agogo"}, {P_808_KICK, P_808_SNARE, P_808_CHH, P_808_OHH, P_808_CLAP, P_808_TOM, P_808_PERC, P_808_FX}, {{2,4,0,0}, {2,4,0,0}, {4,6,0,0}, {4,6,0,0}, {3,4,0,0}, {3,4,5,0}, {3,4,6,0}, {2,3,4,0}}, {0,0, 3,3, 5, 5, 5, 5}, {5,10, 15,15, 20, 20, 20, 20} },
    { 4, 4, 98, 108,  {"Heavy Kick", "Snare", "Hat", "Open Hat", "Clap", "Tom 1", "Tom 2", "Chant"}, {P_808_KICK, P_808_SNARE, P_808_CHH, P_808_OHH, P_808_CLAP, P_808_TOM, P_808_PERC, P_808_FX}, {{3,4,0,0}, {2,4,0,0}, {3,4,0,0}, {2,4,0,0}, {2,4,0,0}, {3,4,5,0}, {3,4,5,0}, {2,3,4,0}}, {-5,0, 0,0, -5, -5, -5, 0}, {0,5, 10,5, 0, 10, 10, 10} },
    { 4, 4, 110, 115, {"Log Drum", "Snare/Rim", "Shaker", "Open Hat", "Clap", "Conga", "Woodblock", "Whistle"}, {P_808_KICK, P_808_SNARE, P_808_CHH, P_808_OHH, P_808_CLAP, P_808_TOM, P_808_PERC, P_808_FX}, {{3,4,0,0}, {2,4,0,0}, {4,6,0,0}, {2,4,0,0}, {2,4,0,0}, {3,4,5,0}, {3,4,5,0}, {2,3,4,0}}, {0,0, 5,0, 0, 5, 5, 0}, {10,5, 15,5, 5, 15, 15, 10} },
    { 7, 8, 40, 60,   {"Bayan", "Dayan", "Tabla", "Manjira", "Ghungroo", "Dholak 1", "Dholak 2", "Vocal"}, {P_808_KICK, P_808_SNARE, P_808_CHH, P_808_OHH, P_808_CLAP, P_808_TOM, P_808_PERC, P_808_FX}, {{2,3,4,0}, {2,3,4,5}, {2,3,4,5}, {2,3,4,0}, {3,4,5,6}, {2,3,4,0}, {2,3,4,0}, {1,2,3,0}}, {-5,-5, -5, -2, -2, -5, -5, -10}, {5,5, 5, 5, 5, 5, 5, 10} },
    { 4, 4, 90, 120,  {"Surdo", "Caixa", "Pandeiro", "Ganza", "Tamborim", "Agogo", "Cuica", "Repique"}, {P_808_KICK, P_808_SNARE, P_808_CHH, P_808_OHH, P_808_CLAP, P_808_TOM, P_808_PERC, P_808_FX}, {{2,4,0,0}, {2,4,0,0}, {4,6,0,0}, {4,6,0,0}, {3,4,0,0}, {3,4,0,0}, {3,4,0,0}, {4,6,0,0}}, {0,5, 5,5, 5, 5, 5, 5}, {5,15, 20,20, 20, 20, 20, 20} },
    { 4, 4, 90, 105,  {"Kick", "Snare (Tresillo)", "Hat", "Open Hat", "Clap", "Timbales", "Perc", "Vocal FX"}, {P_808_KICK, P_808_SNARE, P_808_CHH, P_808_OHH, P_808_CLAP, P_808_TOM, P_808_PERC, P_808_FX}, {{2,4,0,0}, {3,4,0,0}, {2,4,0,0}, {2,4,0,0}, {2,4,0,0}, {3,4,0,0}, {3,4,0,0}, {2,4,0,0}}, {0,-6, 0,0, 0, 0, 0, 0}, {5,0, 5,5, 5, 10, 10, 10} },
    { 8, 4, 40, 55,   {"Gong", "Kempul", "Kendang", "Bonang", "Saron", "Kenong", "Kethuk", "Slenthem"}, {P_808_KICK, P_808_SNARE, P_808_CHH, P_808_OHH, P_808_CLAP, P_808_TOM, P_808_PERC, P_808_FX}, {{1,2,0,0}, {1,2,0,0}, {1,2,0,0}, {1,2,0,0}, {1,2,0,0}, {1,2,0,0}, {1,2,0,0}, {1,2,0,0}}, {-10,-10, -5,-5, -5, -10, -5, -10}, {10,10, 10,10, 10, 10, 10, 10} },
    { 4, 4, 100, 115, {"Kick", "Snare", "Hi-Hat", "Open Hat", "Clap", "Tom", "Conga", "Tambourine"}, {P_808_KICK, P_808_SNARE, P_808_CHH, P_808_OHH, P_808_CLAP, P_808_TOM, P_808_PERC, P_808_FX}, {{2,4,0,0}, {2,4,0,0}, {4,6,0,0}, {2,4,0,0}, {2,4,0,0}, {3,4,0,0}, {3,4,6,0}, {4,6,0,0}}, {0,0, 5,0, 0, 0, 5, 5}, {5,10, 20,5, 5, 10, 15, 20} },
    { 4, 4, 100, 112, {"Punch Kick", "Snare", "Swing Hat", "Open Hat", "Clap", "Tom 1", "Tom 2", "Orch Hit"}, {P_808_KICK, P_808_SNARE, P_808_CHH, P_808_OHH, P_808_CLAP, P_808_TOM, P_808_PERC, P_808_FX}, {{2,4,0,0}, {2,4,0,0}, {6,12,0,0}, {2,4,0,0}, {2,4,0,0}, {2,4,0,0}, {2,4,0,0}, {2,4,0,0}}, {0,0, 15,0, 0, 0, 0, 0}, {5,5, 25,5, 5, 5, 5, 5} },
    { 4, 4, 80, 95,   {"Soft Kick", "Rimshot", "Loose Hat", "Ride", "Snap", "Tom", "Shaker", "Vinyl FX"}, {P_808_KICK, P_808_SNARE, P_808_CHH, P_808_OHH, P_808_CLAP, P_808_TOM, P_808_PERC, P_808_FX}, {{2,4,0,0}, {2,4,0,0}, {3,4,6,0}, {3,4,0,0}, {2,4,0,0}, {2,3,4,0}, {4,6,0,0}, {1,2,0,0}}, {-5, 10, 20, 10, 5, 0, 15, 0}, {2, 25, 40, 25, 20, 10, 30, 0} },
    { 4, 4, 85, 95,   {"Gritty Kick", "Fat Snare", "Hi-Hat", "Open Hat", "Clap", "Perc", "Scratch", "Sample"}, {P_808_KICK, P_808_SNARE, P_808_CHH, P_808_OHH, P_808_CLAP, P_808_TOM, P_808_PERC, P_808_FX}, {{2,4,0,0}, {2,4,0,0}, {3,4,6,0}, {2,4,0,0}, {2,4,0,0}, {3,4,0,0}, {2,4,0,0}, {1,2,4,0}}, {2, 5, 5, 0, 0, 0, 0, 0}, {8, 12, 15, 5, 5, 10, 5, 0} },
    { 5, 4, 120, 160, {"Kick", "Snare", "Hi-Hat", "Ride", "Ghost Snr", "Tom 1", "Tom 2", "Crash"}, {P_808_KICK, P_808_SNARE, P_808_CHH, P_808_OHH, P_808_CLAP, P_808_TOM, P_808_PERC, P_808_FX}, {{2,3,4,0}, {2,3,4,0}, {4,5,6,0}, {3,4,5,0}, {4,6,8,0}, {3,4,5,0}, {3,4,5,0}, {1,2,0,0}}, {0,0, 0,0, 0, 0, 0, 0}, {5,5, 5,5, 5, 5, 5, 5} },
    { 13, 8, 60, 85,  {"Kick (Click)", "Snare (Fat)", "Max Stax", "Hat Bark", "High Tom", "Mid Tom", "Floor Tom", "Splash"}, {P_808_KICK, P_808_SNARE, P_808_CHH, P_808_OHH, P_808_CLAP, P_808_TOM, P_808_PERC, P_808_FX}, {{2,4,0,0}, {2,4,0,0}, {4,6,8,0}, {2,4,0,0}, {3,4,5,0}, {3,4,5,0}, {3,4,5,0}, {1,2,0,0}}, {0, 2, 0, 4, 1, 2, 3, 0}, {0, 2, 0, 6, 1, 2, 3, 0} },
    { 12, 8, 60, 75,  {"Clap 1", "Clap 2", "Marimba 1", "Marimba 2", "Woodblock", "Pulse", "Phase 1", "Phase 2"}, {P_808_KICK, P_808_SNARE, P_808_CHH, P_808_OHH, P_808_CLAP, P_808_TOM, P_808_PERC, P_808_FX}, {{1,2,0,0}, {1,2,0,0}, {1,2,0,0}, {1,2,0,0}, {1,2,0,0}, {1,2,0,0}, {1,2,0,0}, {1,2,0,0}}, {0,0, 0,0, 0, 0, -20, 20}, {0,0, 0,0, 0, 0, -20, 20} },
    { 4, 4, 120, 150, {"Node C", "Node D", "Node F", "Node G", "Node A", "Node C^", "Node D^", "Node F^"}, {PLUCK_1, PLUCK_2, PLUCK_3, PLUCK_4, PLUCK_5, PLUCK_6, PLUCK_7, PLUCK_8}, {{2,3,5,7}, {2,3,5,7}, {2,3,5,7}, {2,3,5,7}, {2,3,5,7}, {2,3,5,7}, {2,3,5,7}, {2,3,5,7}}, {0,0,0,0,0,0,0,0}, {0,0,0,0,0,0,0,0} },
    { 4, 4, 120, 150, {"Chaos C", "Chaos D", "Chaos F", "Chaos G", "Chaos A", "Chaos C^", "Chaos D^", "Chaos F^"}, {PLUCK_1, PLUCK_2, PLUCK_3, PLUCK_4, PLUCK_5, PLUCK_6, PLUCK_7, PLUCK_8}, {{1,3,6,9}, {1,3,6,9}, {1,3,6,9}, {1,3,6,9}, {1,3,6,9}, {1,3,6,9}, {1,3,6,9}, {1,3,6,9}}, {-20,-20,-20,-20,-20,-20,-20,-20}, {20,20,20,20,20,20,20,20} }
} };

const int scalePatterns[19][8] = {
    {0, 2, 4, 5, 7, 9, 11, -1},  // 0: Major
    {0, 2, 3, 5, 7, 8, 10, -1},  // 1: Natural Minor
    {0, 2, 4, 7, 9, -1, -1, -1}, // 2: Pentatonic Major
    {0, 3, 5, 7, 10, -1, -1, -1},// 3: Pentatonic Minor
    {0, 2, 3, 5, 7, 9, 10, -1},  // 4: Dorian
    {0, 2, 3, 5, 7, 8, 11, -1},  // 5: Harmonic Minor
    {0, 2, 4, 6, 7, 9, 11, -1},  // 6: Lydian
    {0, 2, 4, 5, 7, 9, 10, -1},  // 7: Mixolydian
    {0, 1, 3, 5, 7, 8, 10, -1},  // 8: Phrygian
    {0, 1, 3, 5, 6, 8, 10, -1},  // 9: Locrian
    {0, 2, 4, 6, 8, 10, -1, -1}, // 10: Whole Tone
    {0, 3, 5, 6, 7, 10, -1, -1}, // 11: Blues
    {0, 2, 3, 5, 7, 8, 10, -1},  // 12: Aeolian
    {0, 2, 3, 5, 7, 9, 11, -1},  // 13: Dorian ♮7
    {0, 1, 3, 5, 7, 9, 10, -1},  // 14: Phrygian ♮6
    {0, 2, 4, 6, 8, 9, 11, -1},  // 15: Lydian #5
    {0, 1, 3, 4, 6, 8, 10, -1},  // 16: Locrian ♭4
    {0, 1, 3, 4, 6, 7, 9, 10},   // 17: Combination of Diminished
    {0, 3, 4, 7, 8, 11, -1, -1}  // 18: Augmented
};
const int scaleLengths[19] = { 7, 7, 5, 5, 7, 7, 7, 7, 7, 7, 6, 6, 7, 7, 7, 7, 7, 8, 6 };

AIDrumMachineAudioProcessor::AIDrumMachineAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
    : AudioProcessor(BusesProperties()
#if ! JucePlugin_IsMidiEffect
#if ! JucePlugin_IsSynth
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
#endif
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)
#endif
    )
#endif
{
    formatManager.registerBasicFormats();
    initializeUserTunings();
}

AIDrumMachineAudioProcessor::~AIDrumMachineAudioProcessor() {}

void AIDrumMachineAudioProcessor::initializeUserTunings() {
    for (int g = 0; g < 24; ++g) {
        userTuning[g].tempo.min = genreTable[g].minTempo;
        userTuning[g].tempo.max = genreTable[g].maxTempo;
        userTuning[g].tempoLocked = false;

        for (int t = 0; t < 8; ++t) {
            userTuning[g].allowedTimeSigs[t] = false;
            userTuning[g].tracks[t].divLocked = false;
            for (int d = 0; d < 8; ++d) userTuning[g].tracks[t].allowedDivs[d] = false;

            // ★ 提案A: ID 22(Euclidean)と23(Chaos)のみデフォルトCmplxを設定
            if (g == 22 || g == 23) {
                userTuning[g].tracks[t].cmplx.min = 30;
                userTuning[g].tracks[t].cmplx.max = 70;
            }
            else {
                userTuning[g].tracks[t].cmplx.min = 0;
                userTuning[g].tracks[t].cmplx.max = 0;
            }
            userTuning[g].tracks[t].cmplxLocked = false;

            userTuning[g].tracks[t].entrp.min = 0;
            userTuning[g].tracks[t].entrp.max = 0;
            userTuning[g].tracks[t].entrpLocked = false;

            userTuning[g].tracks[t].shift.min = 0;
            userTuning[g].tracks[t].shift.max = 0;
            userTuning[g].tracks[t].shiftLocked = false;
        }

        for (int f = 0; f < 4; ++f) userTuning[g].allowedFills[f] = false;
        if (g == 0 || g == 1 || g == 4 || g == 7 || g == 9 || g == 10 || g == 13 || g == 18) {
            userTuning[g].allowedFills[1] = true;
            userTuning[g].allowedFills[3] = true;
        }
        else if (g == 3 || g == 5 || g == 6 || g == 19 || g == 20 || g == 22 || g == 23) {
            userTuning[g].allowedFills[0] = true;
            userTuning[g].allowedFills[2] = true;
        }
        else {
            userTuning[g].allowedFills[0] = true;
            userTuning[g].allowedFills[1] = true;
        }
    }
}

const GenreDefinition& AIDrumMachineAudioProcessor::getGenreDef(int index) {
    if (index < 0 || index >= 24) return genreTable[0];
    return genreTable[index];
}

const InstrumentPatch& AIDrumMachineAudioProcessor::getPatch(PatchID id) {
    if (id < 0 || id >= PATCH_MAX) return patchLibrary[P_808_KICK];
    return patchLibrary[id];
}

juce::String AIDrumMachineAudioProcessor::getNoteName(int trackIndex) const {
    if (trackIndex < 0 || trackIndex >= 8) return "";
    const char* notesStr[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    int note = 60 + arpKey.load() + (trackOctaveUI[trackIndex] * 12) + scalePatterns[arpScale.load()][trackDegreeUI[trackIndex]];
    int n = note % 12;
    int oct = (note / 12) - 1;
    return juce::String(notesStr[n]) + " " + juce::String(oct);
}

void AIDrumMachineAudioProcessor::shiftTrackLeft(int trk) {
    if (trk < 0 || trk >= 8 || trackLocked[trk]) return;
    int totalSteps = trackDivisionsUI[trk] * timeSigNumerator.load() * globalBarCount.load();
    if (totalSteps <= 0) return;
    int firstStep = drumPatternUI[trk][0];
    for (int i = 0; i < totalSteps - 1; ++i) drumPatternUI[trk][i] = drumPatternUI[trk][i + 1];
    drumPatternUI[trk][totalSteps - 1] = firstStep;
    patternUpdated.store(true);
}

void AIDrumMachineAudioProcessor::shiftTrackRight(int trk) {
    if (trk < 0 || trk >= 8 || trackLocked[trk]) return;
    int totalSteps = trackDivisionsUI[trk] * timeSigNumerator.load() * globalBarCount.load();
    if (totalSteps <= 0) return;
    int lastStep = drumPatternUI[trk][totalSteps - 1];
    for (int i = totalSteps - 1; i > 0; --i) drumPatternUI[trk][i] = drumPatternUI[trk][i - 1];
    drumPatternUI[trk][0] = lastStep;
    patternUpdated.store(true);
}

void AIDrumMachineAudioProcessor::clearTrack(int trk) {
    if (trk < 0 || trk >= 8 || trackLocked[trk]) return;
    for (int j = 0; j < 1024; ++j) drumPatternUI[trk][j] = 0;
    patternUpdated.store(true);
}

void AIDrumMachineAudioProcessor::generateAllTracks() {
    int genre = currentGenre.load();
    GenreTuning& tuning = userTuning[genre];
    bool isAlgorithmMode = (genre >= 22);

    int fillBar = fillBarTarget.load();
    bool isArp = arpMode.load();
    bool isMono = arpMono.load();
    int curScale = arpScale.load();

    if (!tuning.tempoLocked && !tempoLocked.load() && !isSyncEnabled.load() && !isArp) {
        int tMin = std::min(tuning.tempo.min, tuning.tempo.max);
        int tMax = std::max(tuning.tempo.min, tuning.tempo.max);
        if (tMax > tMin) {
            int newBpm = random.nextInt(juce::Range<int>(tMin, tMax + 1));
            internalTempo.store((double)newBpm);
        }
        else {
            internalTempo.store((double)tMin);
        }
    }

    int num = timeSigNumerator.load();
    int den = timeSigDenominator.load();

    if (!isArp && !timeSigLocked.load()) {
        int tsChoices[8];
        int numTsChoices = 0;
        for (int i = 0; i < 8; ++i) {
            if (tuning.allowedTimeSigs[i]) tsChoices[numTsChoices++] = i;
        }
        if (numTsChoices > 0) {
            int pickedTs = tsChoices[random.nextInt(numTsChoices)];
            num = tuning.timeSigOptions[pickedTs].num;
            den = tuning.timeSigOptions[pickedTs].den;
            timeSigNumerator.store(num);
            timeSigDenominator.store(den);
        }
    }

    int maxDiv = (den == 16) ? 2 : ((den == 8) ? 4 : 8);
    int bars = globalBarCount.load();

    if (isArp) {
        int arpPreset = currentGenre.load();
        std::vector<int> usedNotes;

        for (int trk = 0; trk < 8; ++trk) {
            if (trackLocked[trk] || (trackDegreeLocked[trk] && trackOctaveLocked[trk])) {
                usedNotes.push_back(trackOctaveUI[trk] * 12 + trackDegreeUI[trk]);
            }
        }

        int masterArpDiv = 1 + random.nextInt(4);

        for (int trk = 0; trk < 8; ++trk) {
            if (trackLocked[trk]) continue;

            for (int j = 0; j < 1024; ++j) drumPatternUI[trk][j] = 0;
            if (!trackDivLocked[trk]) trackDivisionsUI[trk] = masterArpDiv;

            // ★ Arpモード時のSetup 1スライダーのランダマイズ
            if (!trackCmplxLocked[trk]) trackComplexity[trk] = random.nextInt(101);
            if (!trackEntrpLocked[trk]) trackEntropy[trk] = random.nextInt(101);
            if (!trackShiftLocked[trk]) trackShiftUI[trk] = random.nextInt(21) - 10;

            int degree = trackDegreeUI[trk];
            int octave = trackOctaveUI[trk];
            int attempts = 0;
            bool unique = false;

            while (!unique && attempts < 50) {
                if (!trackDegreeLocked[trk]) {
                    switch (arpPreset) {
                    case 0: degree = (trk * 2) % scaleLengths[curScale]; break;
                    case 1: degree = ((7 - trk) * 3) % scaleLengths[curScale]; break;
                    case 2: degree = (trk % 3) * 2; break;
                    case 3: degree = (trk % 4) * 2; break;
                    case 4: degree = (trk * 4) % scaleLengths[curScale]; break;
                    case 5: degree = (trk * 5) % scaleLengths[curScale]; break;
                    case 6: degree = (trk * 3) % scaleLengths[curScale]; break;
                    case 9: degree = (trk == 0 || trk == 4) ? 0 : random.nextInt(scaleLengths[curScale]); break;
                    default: degree = random.nextInt(scaleLengths[curScale]); break;
                    }
                    if (random.nextInt(100) < 30) degree = (degree + random.nextInt(3)) % scaleLengths[curScale];
                }

                if (!trackOctaveLocked[trk]) {
                    switch (arpPreset) {
                    case 0: octave = (trk < 4) ? -1 : 0; break;
                    case 1: octave = (trk < 4) ? 0 : -1; break;
                    case 2: octave = (trk / 3) - 1; break;
                    case 3: octave = (trk / 4) - 1; break;
                    case 4: octave = (trk % 2 == 0) ? -1 : 0; break;
                    case 5: octave = (trk % 2 == 0) ? -1 : 1; break;
                    case 6: octave = (trk / 3) - 1; break;
                    case 9: octave = 0; break;
                    default: octave = random.nextInt(3) - 1; break;
                    }
                    octave += random.nextInt(juce::Range<int>(-1, 2));
                    octave = juce::jlimit(-2, 2, octave);
                }

                int noteVal = octave * 12 + degree;
                if (trackDegreeLocked[trk] || trackOctaveLocked[trk] || std::find(usedNotes.begin(), usedNotes.end(), noteVal) == usedNotes.end()) {
                    unique = true; usedNotes.push_back(noteVal);
                }
                attempts++;
            }
            if (!trackDegreeLocked[trk]) trackDegreeUI[trk] = degree;
            if (!trackOctaveLocked[trk]) trackOctaveUI[trk] = octave;
        }

        int baseDiv = trackDivisionsUI[0];
        int n = baseDiv * num * bars;
        int currentTrk = 0;
        int currentChordBase = 0;
        bool stepOccupied[1024] = { false };

        std::vector<std::vector<int>> popMotif;

        for (int j = 0; j < 1024; ++j) {
            if (j >= n) break;
            int currentBarOfStep = j / (baseDiv * num);
            int beatInBar = (j % (baseDiv * num)) / baseDiv;
            int stepInBar = j % (baseDiv * num);
            bool isFillPortion = (fillBar > 0) && (currentBarOfStep == (fillBar - 1)) && (beatInBar >= num - 2);

            bool stepActive = false;
            if (arpPreset == 8) {
                int k = (n * (20 + random.nextInt(60))) / 100;
                int eucOffset = random.nextInt(juce::jmax(1, n));
                stepActive = (((static_cast<int64_t>(j) + eucOffset) * k) % n < k);
            }
            else if (arpPreset == 9) {
                if (currentBarOfStep < 2) {
                    bool isAnticipation = (stepInBar % baseDiv == baseDiv - 1) && (random.nextInt(100) < 60);
                    bool isStrongBeat = (stepInBar % baseDiv == 0) && (random.nextInt(100) < 80);
                    stepActive = isAnticipation || isStrongBeat;
                    if (stepActive) {
                        currentTrk = random.nextInt(8);
                        popMotif.push_back({ j, currentTrk });
                    }
                }
                else if (currentBarOfStep == 2) {
                    int localJ = j % (baseDiv * num * 2);
                    for (auto& m : popMotif) {
                        if (m[0] == localJ) { stepActive = true; currentTrk = m[1]; break; }
                    }
                    if (random.nextInt(100) < 20) stepActive = !stepActive;
                }
                else {
                    stepActive = (stepInBar % (baseDiv * 2) == 0) || (random.nextInt(100) < 40);
                    if (stepActive) currentTrk = random.nextInt(8);
                }
            }
            else {
                stepActive = (random.nextInt(100) < 70);
            }

            if (isFillPortion) stepActive = (arpPreset % 2 == 0) ? true : false;
            if (!stepActive) continue;

            int vel = 70 + random.nextInt(30);
            std::vector<int> tracksToHit;

            switch (arpPreset) {
            case 0: tracksToHit.push_back(currentTrk); currentTrk = (currentTrk + 1) % 8; break;
            case 1: tracksToHit.push_back(currentTrk); currentTrk = (currentTrk + 7) % 8; break;
            case 2: tracksToHit.push_back(currentChordBase % 8); tracksToHit.push_back((currentChordBase + 2) % 8); tracksToHit.push_back((currentChordBase + 4) % 8); currentChordBase = (currentChordBase + 1) % 8; break;
            case 3: tracksToHit.push_back(currentChordBase % 8); tracksToHit.push_back((currentChordBase + 2) % 8); tracksToHit.push_back((currentChordBase + 4) % 8); tracksToHit.push_back((currentChordBase + 6) % 8); currentChordBase = (currentChordBase + 1) % 8; break;
            case 4: tracksToHit.push_back(currentTrk); currentTrk = (currentTrk + 4) % 8; break;
            case 5: tracksToHit.push_back(currentTrk); currentTrk = (currentTrk + 5) % 8; break;
            case 6: tracksToHit.push_back(currentChordBase % 8); tracksToHit.push_back((currentChordBase + 3) % 8); tracksToHit.push_back((currentChordBase + 6) % 8); currentChordBase = (currentChordBase + 1) % 8; break;
            case 7: break;
            case 8: tracksToHit.push_back(currentTrk); currentTrk = (currentTrk + 1) % 8; break;
            case 9: tracksToHit.push_back(currentTrk); break;
            default:
                int nextTrk = random.nextInt(8);
                while (nextTrk == currentTrk) nextTrk = random.nextInt(8);
                currentTrk = nextTrk; tracksToHit.push_back(currentTrk); break;
            }

            if (arpPreset != 7) {
                for (int t : tracksToHit) {
                    if (!trackLocked[t]) {
                        int cmplx = trackComplexity[t];
                        int entrp = trackEntropy[t];
                        int prob = cmplx; if (prob < 5) prob = 5;

                        if (random.nextInt(100) < prob) {
                            if (isMono && stepOccupied[j]) continue;

                            int finalVel = vel;
                            if (entrp > 0) {
                                int jitter = (int)((entrp / 100.0f) * 50.0f);
                                finalVel -= random.nextInt(jitter + 1);
                            }
                            if (trackDynamic[t] && (stepInBar % baseDiv == 0 || stepInBar % baseDiv == baseDiv - 1)) {
                                finalVel = juce::jlimit(1, 127, finalVel + trackDynamicAmount[t]);
                            }
                            drumPatternUI[t][j] = finalVel;
                            if (isMono) stepOccupied[j] = true;
                        }
                    }
                }
            }
        }

        if (arpPreset == 7) {
            std::vector<int> trkOrder = { 0, 1, 2, 3, 4, 5, 6, 7 };
            for (int i = 7; i > 0; --i) {
                int rIdx = random.nextInt(i + 1);
                std::swap(trkOrder[i], trkOrder[rIdx]);
            }

            for (int j = 0; j < 1024; ++j) {
                if (j >= n) break;
                for (int i = 7; i > 0; --i) {
                    int rIdx = random.nextInt(i + 1);
                    std::swap(trkOrder[i], trkOrder[rIdx]);
                }

                for (int trk : trkOrder) {
                    if (trackLocked[trk]) continue;
                    int tDiv = trackDivisionsUI[trk];
                    int cmplx = trackComplexity[trk];
                    int entrp = trackEntropy[trk];

                    int absoluteStepT = static_cast<int>((static_cast<int64_t>(j) * tDiv) / baseDiv);

                    if ((static_cast<int64_t>(j) * tDiv) % baseDiv == 0) {
                        int prob = cmplx; if (prob < 5) prob = 5;
                        if (random.nextInt(100) < prob) {
                            if (isMono && stepOccupied[j]) continue;
                            int finalVel = 70 + random.nextInt(30);
                            if (entrp > 0) {
                                int jitter = (int)((entrp / 100.0f) * 50.0f);
                                finalVel -= random.nextInt(jitter + 1);
                            }
                            if (trackDynamic[trk] && absoluteStepT % (tDiv * 2) == 0) finalVel = juce::jlimit(1, 127, finalVel + trackDynamicAmount[trk]);
                            drumPatternUI[trk][absoluteStepT] = finalVel;
                            if (isMono) stepOccupied[j] = true;
                        }
                    }
                }
            }
        }
    }
    else {
        int fillChoices[4] = { 0, 0, 0, 0 };
        int numFillChoices = 0;
        for (int i = 0; i < 4; ++i) if (tuning.allowedFills[i]) fillChoices[numFillChoices++] = i;
        int fillTypology = numFillChoices > 0 ? fillChoices[random.nextInt(numFillChoices)] : 0;

        for (int trk = 0; trk < 8; ++trk) {
            if (trackLocked[trk]) continue;
            TrackTuning& tt = tuning.tracks[trk];

            bool divLck = trackDivLocked[trk] || (!isAlgorithmMode && tt.divLocked);
            if (!divLck) {
                std::vector<int> candidates;
                for (int i = 0; i < 8; ++i) {
                    if (tt.allowedDivs[i]) candidates.push_back(i + 1);
                }
                int newDiv = 4;
                if (!candidates.empty()) newDiv = candidates[random.nextInt((int)candidates.size())];
                if (newDiv > maxDiv) newDiv = maxDiv;
                trackDivisionsUI[trk] = newDiv;
            }

            bool cmplxLck = trackCmplxLocked[trk] || (!isAlgorithmMode && tt.cmplxLocked);
            if (!cmplxLck) {
                int cMin = std::min(tt.cmplx.min, tt.cmplx.max);
                int cMax = std::max(tt.cmplx.min, tt.cmplx.max);
                if (cMax > cMin) trackComplexity[trk] = random.nextInt(juce::Range<int>(cMin, cMax + 1));
                else trackComplexity[trk] = cMin;
            }

            bool entrpLck = trackEntrpLocked[trk] || (!isAlgorithmMode && tt.entrpLocked);
            if (!entrpLck) {
                int eMin = std::min(tt.entrp.min, tt.entrp.max);
                int eMax = std::max(tt.entrp.min, tt.entrp.max);
                if (eMax > eMin) trackEntropy[trk] = random.nextInt(juce::Range<int>(eMin, eMax + 1));
                else trackEntropy[trk] = eMin;
            }

            bool shiftLck = trackShiftLocked[trk] || (!isAlgorithmMode && tt.shiftLocked);
            if (!shiftLck) {
                int sMin = std::min(tt.shift.min, tt.shift.max);
                int sMax = std::max(tt.shift.min, tt.shift.max);
                if (sMax > sMin) trackShiftUI[trk] = random.nextInt(juce::Range<int>(sMin, sMax + 1));
                else trackShiftUI[trk] = sMin;
            }

            int div = trackDivisionsUI[trk];
            int n = div * num * bars;
            int offset = random.nextInt(juce::Range<int>(0, juce::jmax(1, n)));
            int cmplx = trackComplexity[trk];
            int entrp = trackEntropy[trk];

            for (int j = 0; j < 1024; ++j) {
                if (j >= n) { drumPatternUI[trk][j] = 0; continue; }

                int currentBarOfStep = j / (div * num);
                int beatInBar = (j % (div * num)) / div;
                int stepInBar = j % (div * num);

                bool isFillActiveBar = (fillBar > 0) && (currentBarOfStep == (fillBar - 1));
                bool isFillPortion = isFillActiveBar && (beatInBar >= num - 2);

                bool isAnchor = false;
                bool isNegativeAnchor = false;
                int anchorVel = 100;

                if (isFillPortion && !isAlgorithmMode) {
                    int localStep = stepInBar - div * (num - 2);
                    if (localStep < 0) localStep = stepInBar;

                    if (fillTypology == 0) {
                        isNegativeAnchor = true;
                        int prime = 7;
                        int targetTrk = ((localStep * prime) + currentBarOfStep) % 8;
                        if (entrp > 30) targetTrk = (targetTrk + (entrp % 5)) % 8;
                        if (trk == targetTrk) { isAnchor = true; anchorVel = 75 + random.nextInt(25); isNegativeAnchor = false; }
                    }
                    else if (fillTypology == 1) {
                        isNegativeAnchor = true;
                        if (trk != 0 && trk != 5 && trk != 6) {
                            int targetTrk = 1 + ((localStep * 3) % 4);
                            if (trk == targetTrk) { isAnchor = true; anchorVel = 60 + random.nextInt(40); isNegativeAnchor = false; }
                            if (stepInBar == (div * num) - (div > 1 ? div / 2 : 1) && (trk == 1 || trk == 7)) { isAnchor = true; anchorVel = 100; isNegativeAnchor = false; }
                        }
                    }
                    else if (fillTypology == 2) {
                        isNegativeAnchor = true;
                        int fillLen = div * 2;
                        if (fillLen > 0) {
                            int k = 5;
                            if ((localStep * k) % fillLen < k) {
                                int trkChoices[] = { 1, 2, 4, 5, 7 };
                                int chosenTrk = trkChoices[(localStep + currentBarOfStep) % 5];
                                if (trk == chosenTrk) { isAnchor = true; anchorVel = 80 + random.nextInt(20); isNegativeAnchor = false; }
                            }
                        }
                    }
                    else {
                        isNegativeAnchor = true;
                        if (trk != 0 && trk != 6) {
                            int activeTrk = 1 + (localStep / (div > 0 ? div : 1)) % 4;
                            if (trk == activeTrk) {
                                if (stepInBar % (div > 1 ? div / 2 : 1) == 0) {
                                    isAnchor = true; isNegativeAnchor = false;
                                    float prog = (float)(stepInBar % (div * 2)) / (float)(juce::jmax(1, div * 2));
                                    anchorVel = 50 + (int)(50.0f * prog);
                                }
                            }
                        }
                    }
                }
                else if (!isAlgorithmMode) {
                    switch (genre) {
                    case 0: case 1:
                        if (trk == 0) { if (stepInBar % div == 0) { isAnchor = true; if ((currentBarOfStep % 2 != 0) && beatInBar == num - 1 && random.nextInt(100) < 15) { isAnchor = false; isNegativeAnchor = true; } } }
                        if (trk == 1 || trk == 4) { if (stepInBar == div || stepInBar == div * 3) isAnchor = true; }
                        if (trk == 2) { if (stepInBar % div == div / 2) isAnchor = true; }
                        if (trk == 3) { if (stepInBar % div == div / 2) { isAnchor = true; anchorVel = 90; } }
                        if (trk == 5 || trk == 6) { if (stepInBar % div == (div > 1 ? div - 1 : 0) && random.nextInt(100) < 40) { isAnchor = true; anchorVel = 60; } }
                        if (trk == 7) { if (currentBarOfStep % 4 == 0 && stepInBar == 0) { isAnchor = true; anchorVel = 80; } }
                        break;
                    case 2:
                        if (trk == 0) {
                            if (stepInBar == 0) isAnchor = true;
                            int barType = currentBarOfStep % 4;
                            if (barType == 0 || barType == 2) { if (stepInBar == div * 2 + div / 2) isAnchor = true; }
                            else if (barType == 1) { if (stepInBar == div * 3 + div / 2) isAnchor = true; }
                            else { if (stepInBar == div + div / 2 || stepInBar == div * 3 + div / 2) isAnchor = true; }
                        }
                        if (trk == 1 || trk == 4) { if (stepInBar == div || stepInBar == div * 3) { isAnchor = true; anchorVel = 90; } }
                        if (trk == 2) { if (stepInBar % div == div / 2) { isAnchor = true; anchorVel = 70; } if (stepInBar % div == div - 1 && random.nextInt(100) < 30) { isAnchor = true; anchorVel = 50; } }
                        if (trk == 3) { if (stepInBar == div * 2 + div / 2) { isAnchor = true; anchorVel = 85; } }
                        if (trk == 5) { if (stepInBar == 0 || stepInBar == div * 2 + div / 2) { isAnchor = true; anchorVel = 90; } }
                        if (trk == 6 || trk == 7) { if (stepInBar == div + div / 2 || stepInBar == div * 3 + div / 2) { isAnchor = true; anchorVel = 60; } }
                        break;
                    case 4: case 7:
                        if (trk == 0) { if (stepInBar == 0) isAnchor = true; if (genre == 4 && stepInBar == div + div / 2) isAnchor = true; if (genre == 7 && stepInBar == div * 2 + div / 2) isAnchor = true; }
                        if (trk == 1 || trk == 4) { if (stepInBar == div * 2) { isAnchor = true; anchorVel = 100; } }
                        if (genre == 4 && trk == 2) { if (stepInBar % (div > 1 ? div / 2 : 1) == 0) isAnchor = true; if ((currentBarOfStep % 2 != 0) && beatInBar == 3 && random.nextInt(100) < 50) { isAnchor = true; anchorVel = 80; } }
                        if (trk == 3) { if (stepInBar == 0 || stepInBar == div * 2) { isAnchor = true; anchorVel = 80; } }
                        if (trk == 5 || trk == 6) { if (stepInBar == div * 3 + div / 2) { isAnchor = true; anchorVel = 70; } }
                        if (trk == 7) { if (stepInBar == 0) { isAnchor = true; anchorVel = 100; } }
                        break;
                    case 3:
                        if (trk == 0) { if (stepInBar == 0) isAnchor = true; if (currentBarOfStep % 2 == 0 && stepInBar == div * 2 + div / 2) isAnchor = true; if (currentBarOfStep % 2 != 0 && stepInBar == div * 3) isAnchor = true; }
                        if (trk == 1 || trk == 4) { if (stepInBar == div || stepInBar == div * 3) { isAnchor = true; anchorVel = 90; } }
                        if (trk == 2) { if (stepInBar % div == 0) isAnchor = true; }
                        if (trk == 3) { if (stepInBar % div == div / 2) { isAnchor = true; anchorVel = 70; } }
                        if (trk == 5 || trk == 6) { if (stepInBar == div + div / 2) { isAnchor = true; anchorVel = 60; } }
                        if (trk == 7) { if (stepInBar == 0) { isAnchor = true; anchorVel = 95; } }
                        break;
                    case 15: case 16:
                        if (trk == 0) { if (stepInBar == 0) isAnchor = true; if (currentBarOfStep % 2 == 0 && stepInBar == div * 2 + div / 2) isAnchor = true; if (currentBarOfStep % 2 != 0 && stepInBar == div * 2 + div - 1) isAnchor = true; }
                        if (trk == 1 || trk == 4) { if (stepInBar == div || stepInBar == div * 3) isAnchor = true; }
                        if (trk == 2) { if (stepInBar % div == 0) isAnchor = true; else if (random.nextInt(100) < 30) { isAnchor = true; anchorVel = 40; } }
                        if (trk == 3) { if (stepInBar == div * 3 + div / 2) { isAnchor = true; anchorVel = 80; } }
                        if (trk == 5 || trk == 6) { if (stepInBar == div - 1 || stepInBar == div * 3 - 1) { isAnchor = true; anchorVel = 50; } }
                        if (trk == 7) { if (currentBarOfStep % 2 == 0 && stepInBar == 0) { isAnchor = true; anchorVel = 90; } }
                        break;
                    case 17: case 18:
                        if (trk == 0) { if (stepInBar == 0) isAnchor = true; if (stepInBar == div * 2 + div / 2 && random.nextInt(100) < 70) isAnchor = true; }
                        if (trk == 1 || trk == 4) { if (stepInBar == div || stepInBar == div * 3) isAnchor = true; }
                        if (trk == 3) { if (stepInBar % (div * 2) == div) { isAnchor = true; anchorVel = 60; } }
                        if (trk == 5 || trk == 6) { if (stepInBar == div * 2 - 1) { isAnchor = true; anchorVel = 40; } }
                        if (trk == 7) { if (currentBarOfStep % 4 == 0 && stepInBar == 0) { isAnchor = true; anchorVel = 85; } }
                        break;
                    case 8: case 12:
                        if (trk == 0) { if (stepInBar == 0 || stepInBar == div * 2 + div / 2) isAnchor = true; }
                        if (trk == 1 || trk == 4) { if (stepInBar == 0 || stepInBar == div + div / 2 || stepInBar == div * 2 + div || stepInBar == div * 3 || stepInBar == div * 4) { isAnchor = true; anchorVel = 85; } }
                        if (trk == 2) { if (stepInBar % div == 0) isAnchor = true; }
                        if (trk == 3) { if (stepInBar % div == div / 2) { isAnchor = true; anchorVel = 75; } }
                        if (trk >= 5 && trk <= 7) { if (stepInBar == div - 1 || stepInBar == div * 3 + div / 2) { isAnchor = true; anchorVel = 80; } }
                        break;
                    case 9: case 10: case 13:
                        if (trk == 0) { if (stepInBar % div == 0) isAnchor = true; }
                        if (trk == 1 || trk == 4) { if (stepInBar == div - 1 || stepInBar == div * 2 + div / 2) isAnchor = true; }
                        if (trk == 2 || trk == 3) { if (stepInBar % (div > 1 ? div / 2 : 1) == 0) { isAnchor = true; anchorVel = 70; } }
                        if (trk == 5) { if (stepInBar == div + div / 2) { isAnchor = true; anchorVel = 80; } }
                        if (genre == 10 && (trk == 6 || trk == 7)) { if (currentBarOfStep % 2 != 0 && (stepInBar == div * 2 + div / 2 || stepInBar == div * 3 + div / 2)) { isAnchor = true; anchorVel = 100; } }
                        else if (trk == 6 || trk == 7) { if (stepInBar == div * 3 + div / 2) { isAnchor = true; anchorVel = 80; } }
                        break;
                    case 11: case 14:
                        if (trk == 0) { if (currentBarOfStep % 4 == 0 && stepInBar == 0) { isAnchor = true; anchorVel = 100; } else { isNegativeAnchor = true; } }
                        if (trk >= 1 && trk <= 4) { if (stepInBar % (div * 2) == 0) { isAnchor = true; anchorVel = 70; } }
                        if (trk >= 5 && trk <= 7) { if (stepInBar % div == div / 2) { isAnchor = true; anchorVel = 60; } }
                        break;
                    case 19:
                        if (trk == 0) { if (stepInBar == 0 || stepInBar == div * 3) isAnchor = true; }
                        if (trk == 1 || trk == 4) { if (stepInBar == div * 2 || stepInBar == div * 4) isAnchor = true; }
                        if (trk == 2) { if (stepInBar % div == 0) isAnchor = true; if (random.nextInt(100) < 20) { isAnchor = true; anchorVel = 50; } }
                        if (trk == 3) { if (stepInBar == div * 3 + div / 2) { isAnchor = true; anchorVel = 80; } }
                        if (trk == 5 || trk == 6) { if (stepInBar == div + div / 2 || stepInBar == div * 4 + div / 2) { isAnchor = true; anchorVel = 75; } }
                        if (trk == 7) { if (currentBarOfStep % 2 == 0 && stepInBar == 0) { isAnchor = true; anchorVel = 90; } }
                        break;
                    case 20:
                        if (trk == 0) { if (stepInBar == 0 || stepInBar == div * 3 || stepInBar == div * 5) isAnchor = true; }
                        if (trk == 1) { if (stepInBar == div * 2 || stepInBar == div * 4 || stepInBar == div * 6) isAnchor = true; }
                        if (trk == 2) { if (stepInBar % (div > 1 ? div / 2 : 1) == 0) { isAnchor = true; anchorVel = 70; } }
                        if (trk == 3) { if (stepInBar == div * 2 + (div > 1 ? div / 2 : 0) || stepInBar == div * 4 + (div > 1 ? div / 2 : 0)) { isAnchor = true; anchorVel = 85; } }
                        if (trk == 4) { if (currentBarOfStep % 2 == 1 && stepInBar == div * (num - 1)) { isAnchor = true; anchorVel = 90; } }
                        if (trk == 5) { if (currentBarOfStep % 2 == 1 && stepInBar == div * (num - 1) + (div > 1 ? div / 2 : 0)) { isAnchor = true; anchorVel = 90; } }
                        if (trk == 6) { if (currentBarOfStep % 2 == 1 && stepInBar == div * num - 1) { isAnchor = true; anchorVel = 90; } }
                        if (trk == 7) { if (currentBarOfStep % 2 == 0 && stepInBar == 0) { isAnchor = true; anchorVel = 100; } }
                        break;
                    case 21:
                        if (trk == 0) { if (stepInBar % (div * 3) == 0) isAnchor = true; }
                        if (trk == 1) { if ((stepInBar + div) % (div * 3) == 0) isAnchor = true; }
                        if (trk >= 2 && trk <= 4) { if (stepInBar % 2 == 0) isAnchor = true; }
                        if (trk >= 5 && trk <= 7) { if (stepInBar % 3 == 0) { isAnchor = true; anchorVel = 70; } }
                        break;
                    default:
                        if (trk == 0 && stepInBar == 0) isAnchor = true;
                        if ((trk == 1 || trk == 4) && (stepInBar == div || stepInBar == div * 3) && num >= 4) isAnchor = true;
                        if (trk >= 5) { if (stepInBar == div * 2 + div / 2) { isAnchor = true; anchorVel = 60; } }
                        break;
                    }

                    if (trk == 0 && !isAnchor && !isNegativeAnchor) {
                        if (stepInBar % div == div - 1) {
                            if (cmplx > 0 && entrp > 0) {
                                if (random.nextInt(100) < (entrp / 2)) {
                                    drumPatternUI[trk][j] = random.nextInt(juce::Range<int>(40, 75));
                                    continue;
                                }
                            }
                        }
                    }
                }

                int k = juce::jmax(1, (n * cmplx) / 100);
                int vel = 0;

                if (isAnchor) {
                    int velJitter = (int)((entrp / 100.0f) * 20.0f);
                    vel = anchorVel - random.nextInt(juce::Range<int>(0, velJitter + 1));
                    if (trackDynamic[trk]) vel = juce::jlimit(80, 127, vel + trackDynamicAmount[trk]);
                }
                else if (isNegativeAnchor) {
                    vel = 0;
                }
                else if (cmplx > 0) {
                    bool isHit = false;
                    // ★ ID 23 (Pure Chaos) の専用ロジック：純粋な確率によるランダム配置
                    if (genre == 23) {
                        isHit = (random.nextInt(100) < cmplx);
                    }
                    else {
                        isHit = (((static_cast<int64_t>(j) + offset) * k) % n) < k;
                    }

                    if (entrp > 0 && random.nextInt(100) < (entrp / 3)) isHit = !isHit;
                    vel = isHit ? random.nextInt(juce::Range<int>(40, 90 + (entrp / 10))) : 0;
                    if (isHit && trackDynamic[trk] && stepInBar % div == div - 1) vel = juce::jlimit(80, 127, vel + trackDynamicAmount[trk]);
                }
                drumPatternUI[trk][j] = vel;
            }
        }
    }

    patternUpdated.store(true);
    uiNeedsUpdate.store(true);
}

void AIDrumMachineAudioProcessor::loadSample(int trackIndex, const juce::String& filePath) {
    if (trackIndex < 0 || trackIndex >= 8) return;
    juce::File file(filePath);
    if (auto* reader = formatManager.createReaderFor(file)) {
        juce::AudioSampleBuffer tempBuffer(reader->numChannels, (int)reader->lengthInSamples);
        reader->read(&tempBuffer, 0, (int)reader->lengthInSamples, 0, true, true);
        juce::ScopedLock sl(sampleLock);
        sampleBuffers[trackIndex] = tempBuffer;
        hasSample[trackIndex] = true;
        samplePlayPos[trackIndex] = -1;
        delete reader;
    }
}

void AIDrumMachineAudioProcessor::clearSample(int trackIndex) {
    if (trackIndex < 0 || trackIndex >= 8) return;
    juce::ScopedLock sl(sampleLock);
    hasSample[trackIndex] = false;
    samplePlayPos[trackIndex] = -1;
}

void AIDrumMachineAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {
    resetPosition();
    for (int i = 0; i < 8; ++i) {
        synthVoices[i].setSampleRate((float)sampleRate);
        samplePlayPos[i] = -1;
    }

    for (int i = 0; i < 8; ++i) {
        std::memcpy(drumPatternDSP[i], drumPatternUI[i], sizeof(drumPatternUI[i]));
        trackDivisionsDSP[i] = trackDivisionsUI[i];
        trackShiftDSP[i] = trackShiftUI[i];
        trackOctaveDSP[i] = trackOctaveUI[i];
        trackDegreeDSP[i] = trackDegreeUI[i];
    }
    timeSigNumDSP = timeSigNumerator.load();
    timeSigDenDSP = timeSigDenominator.load();
    globalBarCountDSP = globalBarCount.load();
    patternUpdated.store(false);
}

void AIDrumMachineAudioProcessor::releaseResources() {}

#ifndef JucePlugin_PreferredChannelConfigurations
bool AIDrumMachineAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const {
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono() && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo()) return false;
    return true;
}
#endif

void AIDrumMachineAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i) buffer.clear(i, 0, buffer.getNumSamples());

    auto* leftChannel = buffer.getWritePointer(0);
    auto* rightChannel = buffer.getNumChannels() > 1 ? buffer.getWritePointer(1) : nullptr;

    if (patternUpdated.exchange(false)) {
        for (int i = 0; i < 8; ++i) {
            std::memcpy(drumPatternDSP[i], drumPatternUI[i], sizeof(drumPatternUI[i]));
            trackDivisionsDSP[i] = trackDivisionsUI[i];
            trackShiftDSP[i] = trackShiftUI[i];
            trackOctaveDSP[i] = trackOctaveUI[i];
            trackDegreeDSP[i] = trackDegreeUI[i];
        }
        timeSigNumDSP = timeSigNumerator.load();
        timeSigDenDSP = timeSigDenominator.load();
        globalBarCountDSP = globalBarCount.load();
    }

    int numSamples = buffer.getNumSamples();
    float sampleRate = (float)getSampleRate(); if (sampleRate <= 0.0f) sampleRate = 44100.0f;

    double bpm = internalTempo.load();
    bool isPlaying = isPlayingInternal.load();

    if (isSyncEnabled.load()) {
        if (auto* playHead = getPlayHead()) {
            if (auto pos = playHead->getPosition()) {
                if (pos->getBpm().hasValue()) bpm = *pos->getBpm();
                isPlaying = pos->getIsPlaying();
            }
        }
    }
    currentBpm.store(bpm);

    double samplesPerQuarterNote = sampleRate * (60.0 / bpm);
    double samplesPerBeat = samplesPerQuarterNote * (4.0 / (double)timeSigDenDSP);
    int samplesPerBar = (int)(samplesPerBeat * timeSigNumDSP);
    int samplesPerLoop = samplesPerBar * globalBarCountDSP;

    if (samplesPerBar > 0) {
        currentPlayingBar.store((samplesInLoop / samplesPerBar) % globalBarCountDSP);
    }

    bool anySolo = false;
    for (int i = 0; i < 8; ++i) { if (trackSoloed[i]) { anySolo = true; break; } }

    const auto& def = getGenreDef(currentGenre.load());
    bool isArp = arpMode.load();
    int curScale = arpScale.load();

    juce::ScopedLock sl(sampleLock);

    for (int i = 0; i < numSamples; ++i)
    {
        if (isPlaying && samplesPerLoop > 0) {
            samplesInLoop++;
            if (samplesInLoop >= samplesPerLoop) samplesInLoop = 0;

            for (int trk = 0; trk < 8; ++trk)
            {
                int div = trackDivisionsDSP[trk]; if (div < 1) div = 1;
                int totalStepsInLoop = div * timeSigNumDSP * globalBarCountDSP;
                double samplesPerStep = (double)samplesPerLoop / (double)totalStepsInLoop;
                int shiftInSamples = (int)((trackShiftDSP[trk] / 100.0) * samplesPerStep);

                int virtualSamplesInLoop = samplesInLoop - shiftInSamples;
                while (virtualSamplesInLoop < 0) virtualSamplesInLoop += samplesPerLoop;
                virtualSamplesInLoop %= samplesPerLoop;

                int currentStepForTrack = (virtualSamplesInLoop * totalStepsInLoop) / samplesPerLoop;

                if (currentStepForTrack != trackCurrentStep[trk] && currentStepForTrack < 1024)
                {
                    trackCurrentStep[trk] = currentStepForTrack;
                    bool shouldPlay = true;
                    if (anySolo && !trackSoloed[trk]) shouldPlay = false;
                    if (trackMuted[trk] && !trackSoloed[trk]) shouldPlay = false;

                    int velocity = drumPatternDSP[trk][trackCurrentStep[trk]];
                    if (velocity > 0 && shouldPlay)
                    {
                        if (hasSample[trk]) {
                            samplePlayPos[trk] = 0;
                            sampleVolume[trk] = velocity / 100.0f;
                        }
                        else {
                            InstrumentPatch p = getPatch(isArp ? M_ARP : def.trackPatches[trk]);
                            if (isArp) {
                                int note = 60 + arpKey.load() + (trackOctaveDSP[trk] * 12) + scalePatterns[curScale][trackDegreeDSP[trk]];
                                p.freq = 440.0f * std::pow(2.0f, (note - 69) / 12.0f);
                            }
                            synthVoices[trk].trigger((float)velocity, p);
                        }
                    }
                }
            }
        }

        float mixOut = 0.0f;
        for (int trk = 0; trk < 8; ++trk)
        {
            float osc = 0.0f;
            if (hasSample[trk]) {
                if (samplePlayPos[trk] >= 0 && samplePlayPos[trk] < sampleBuffers[trk].getNumSamples()) {
                    osc = sampleBuffers[trk].getSample(0, samplePlayPos[trk]) * sampleVolume[trk];
                    samplePlayPos[trk]++;
                }
            }
            else {
                osc = synthVoices[trk].process();
            }
            mixOut += osc * 0.5f;
        }

        if (mixOut > 1.0f) mixOut = 1.0f;
        if (mixOut < -1.0f) mixOut = -1.0f;

        if (leftChannel != nullptr) leftChannel[i] = mixOut;
        if (rightChannel != nullptr) rightChannel[i] = mixOut;
    }
}

const juce::String AIDrumMachineAudioProcessor::getName() const { return JucePlugin_Name; }
bool AIDrumMachineAudioProcessor::acceptsMidi() const { return false; }
bool AIDrumMachineAudioProcessor::producesMidi() const { return false; }
bool AIDrumMachineAudioProcessor::isMidiEffect() const { return false; }
double AIDrumMachineAudioProcessor::getTailLengthSeconds() const { return 0.0; }
int AIDrumMachineAudioProcessor::getNumPrograms() { return 1; }
int AIDrumMachineAudioProcessor::getCurrentProgram() { return 0; }
void AIDrumMachineAudioProcessor::setCurrentProgram(int index) {}
const juce::String AIDrumMachineAudioProcessor::getProgramName(int index) { return {}; }
void AIDrumMachineAudioProcessor::changeProgramName(int index, const juce::String& newName) {}

bool AIDrumMachineAudioProcessor::hasEditor() const { return true; }
juce::AudioProcessorEditor* AIDrumMachineAudioProcessor::createEditor() { return new AIDrumMachineAudioProcessorEditor(*this); }

void AIDrumMachineAudioProcessor::getStateInformation(juce::MemoryBlock& destData) {
    juce::XmlElement xml("AIDrumMachineState");
    xml.setAttribute("currentGenre", currentGenre.load());
    xml.setAttribute("globalBarCount", globalBarCount.load());

    juce::String dynStr, degLckStr, octLckStr;
    for (int i = 0; i < 8; ++i) {
        dynStr += trackDynamic[i] ? "1" : "0";
        degLckStr += trackDegreeLocked[i] ? "1" : "0";
        octLckStr += trackOctaveLocked[i] ? "1" : "0";
        xml.setAttribute("dynAmt" + juce::String(i), trackDynamicAmount[i]);
    }
    xml.setAttribute("dyn", dynStr);
    xml.setAttribute("degLck", degLckStr);
    xml.setAttribute("octLck", octLckStr);
    xml.setAttribute("tsLock", timeSigLocked.load());

    auto* tuningsXml = new juce::XmlElement("UserTunings");
    for (int g = 0; g < 24; ++g) {
        auto* gXml = new juce::XmlElement("Genre");
        gXml->setAttribute("id", g);
        gXml->setAttribute("tMin", userTuning[g].tempo.min);
        gXml->setAttribute("tMax", userTuning[g].tempo.max);
        gXml->setAttribute("tLock", userTuning[g].tempoLocked);

        juce::String tsStr, fStr;
        for (int i = 0; i < 8; ++i) tsStr += userTuning[g].allowedTimeSigs[i] ? "1" : "0";
        for (int i = 0; i < 4; ++i) fStr += userTuning[g].allowedFills[i] ? "1" : "0";
        gXml->setAttribute("ts", tsStr);
        gXml->setAttribute("fills", fStr);

        for (int t = 0; t < 8; ++t) {
            auto* tXml = new juce::XmlElement("Track");
            tXml->setAttribute("id", t);

            juce::String dStr;
            for (int d = 0; d < 8; ++d) dStr += userTuning[g].tracks[t].allowedDivs[d] ? "1" : "0";
            tXml->setAttribute("divs", dStr);

            tXml->setAttribute("dLck", userTuning[g].tracks[t].divLocked);
            tXml->setAttribute("cMin", userTuning[g].tracks[t].cmplx.min);
            tXml->setAttribute("cMax", userTuning[g].tracks[t].cmplx.max);
            tXml->setAttribute("cLck", userTuning[g].tracks[t].cmplxLocked);
            tXml->setAttribute("eMin", userTuning[g].tracks[t].entrp.min);
            tXml->setAttribute("eMax", userTuning[g].tracks[t].entrp.max);
            tXml->setAttribute("eLck", userTuning[g].tracks[t].entrpLocked);
            tXml->setAttribute("sMin", userTuning[g].tracks[t].shift.min);
            tXml->setAttribute("sMax", userTuning[g].tracks[t].shift.max);
            tXml->setAttribute("sLck", userTuning[g].tracks[t].shiftLocked);
            gXml->addChildElement(tXml);
        }
        tuningsXml->addChildElement(gXml);
    }
    xml.addChildElement(tuningsXml);

    copyXmlToBinary(xml, destData);
}

void AIDrumMachineAudioProcessor::setStateInformation(const void* data, int sizeInBytes) {
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState != nullptr && xmlState->hasTagName("AIDrumMachineState")) {
        currentGenre.store(xmlState->getIntAttribute("currentGenre", 0));
        globalBarCount.store(xmlState->getIntAttribute("globalBarCount", 4));

        juce::String dynStr = xmlState->getStringAttribute("dyn", "00000000");
        juce::String degLckStr = xmlState->getStringAttribute("degLck", "00000000");
        juce::String octLckStr = xmlState->getStringAttribute("octLck", "00000000");
        for (int i = 0; i < 8; ++i) {
            trackDynamic[i] = (dynStr[i] == '1');
            trackDegreeLocked[i] = (degLckStr[i] == '1');
            trackOctaveLocked[i] = (octLckStr[i] == '1');
            trackDynamicAmount[i] = xmlState->getIntAttribute("dynAmt" + juce::String(i), 30);
        }
        timeSigLocked.store(xmlState->getBoolAttribute("tsLock", false));

        if (auto* tuningsXml = xmlState->getChildByName("UserTunings")) {
            for (auto* gXml : tuningsXml->getChildIterator()) {
                int g = gXml->getIntAttribute("id", -1);
                if (g >= 0 && g < 24) {
                    userTuning[g].tempo.min = gXml->getIntAttribute("tMin", 0);
                    userTuning[g].tempo.max = gXml->getIntAttribute("tMax", 0);
                    userTuning[g].tempoLocked = gXml->getBoolAttribute("tLock", false);

                    juce::String tsStr = gXml->getStringAttribute("ts", "00000000");
                    for (int i = 0; i < 8; ++i) userTuning[g].allowedTimeSigs[i] = (tsStr[i] == '1');

                    juce::String fStr = gXml->getStringAttribute("fills", "0000");
                    for (int i = 0; i < 4; ++i) userTuning[g].allowedFills[i] = (fStr[i] == '1');

                    for (auto* tXml : gXml->getChildIterator()) {
                        int t = tXml->getIntAttribute("id", -1);
                        if (t >= 0 && t < 8) {
                            juce::String dStr = tXml->getStringAttribute("divs", "00000000");
                            for (int d = 0; d < 8; ++d) userTuning[g].tracks[t].allowedDivs[d] = (dStr[d] == '1');

                            userTuning[g].tracks[t].divLocked = tXml->getBoolAttribute("dLck", false);

                            userTuning[g].tracks[t].cmplx.min = tXml->getIntAttribute("cMin", 0);
                            userTuning[g].tracks[t].cmplx.max = tXml->getIntAttribute("cMax", 0);
                            userTuning[g].tracks[t].cmplxLocked = tXml->getBoolAttribute("cLck", false);

                            userTuning[g].tracks[t].entrp.min = tXml->getIntAttribute("eMin", 0);
                            userTuning[g].tracks[t].entrp.max = tXml->getIntAttribute("eMax", 0);
                            userTuning[g].tracks[t].entrpLocked = tXml->getBoolAttribute("eLck", false);

                            userTuning[g].tracks[t].shift.min = tXml->getIntAttribute("sMin", 0);
                            userTuning[g].tracks[t].shift.max = tXml->getIntAttribute("sMax", 0);
                            userTuning[g].tracks[t].shiftLocked = tXml->getBoolAttribute("sLck", false);
                        }
                    }
                }
            }
        }
        uiNeedsUpdate.store(true);
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new AIDrumMachineAudioProcessor(); }