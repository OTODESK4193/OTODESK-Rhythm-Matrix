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
    // ★諸悪の根源だった「float vl」の「float」を削除しました
    return { w, fr, pD, pA, aAt, aDc, ns, fT, fF, fR, dr, vl };
}

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

static const std::array<GenreDefinition, 26> genreTable = { {
    { 4, 4, 125, 135, {"909 Kick", "909 Snare", "CHH", "OHH", "Clap", "Ride", "Tom", "Noise FX"}, {P_808_KICK, P_808_SNARE, P_808_CHH, P_808_OHH, P_808_CLAP, P_808_TOM, P_808_PERC, P_808_FX}, {{1,2,4,0}, {2,4,0,0}, {4,8,0,0}, {4,8,0,0}, {2,4,0,0}, {3,4,5,0}, {3,5,7,0}, {2,4,8,0}}, {0,0,0,0,0,0,0,0}, {0,0,2,2,0,5,5,10} },
    { 4, 4, 120, 126, {"Deep Kick", "Rimshot", "Shuff Hat", "Open Hat", "Clap", "Conga", "Bongo", "Vocal Chop"}, {P_808_KICK, P_808_SNARE, P_808_CHH, P_808_OHH, P_808_CLAP, P_808_TOM, P_808_PERC, P_808_FX}, {{1,2,4,0}, {2,4,0,0}, {4,6,0,0}, {2,4,0,0}, {2,4,0,0}, {3,5,0,0}, {4,6,8,0}, {2,3,4,0}}, {-2,0, 5,0,-2, -5, -5, 0}, {2,5, 15,5, 5, 10, 10, 15} },
    { 4, 4, 130, 138, {"Punch Kick", "Snare/Rim", "Garage Hat", "Ride", "Clap", "Perc 1", "Perc 2", "Vocal FX"}, {P_808_KICK, P_808_SNARE, P_808_CHH, P_808_OHH, P_808_CLAP, P_808_PERC, P_808_PERC, P_808_FX}, {{2,3,4,0}, {2,4,0,0}, {4,6,0,0}, {4,6,0,0}, {2,4,0,0}, {2,3,4,0}, {3,5,0,0}, {5,7,0,0}}, {-5,5, 10,5, 0, -10, 0, 0}, {5,15, 25,15, 10, 10, 15, 15} },
    { 4, 4, 165, 175, {"Heavy Kick", "Tight Snr", "Fast Hat", "Ride", "Break Rim", "Shaker 1", "Shaker 2", "Sub Bass"}, {P_808_KICK, P_808_SNARE, P_808_CHH, P_808_OHH, P_808_CLAP, P_808_CHH, P_808_CHH, P_808_BASS}, {{2,4,0,0}, {2,4,0,0}, {4,8,0,0}, {4,8,0,0}, {2,4,0,0}, {4,6,8,0}, {4,6,8,0}, {2,4,0,0}}, {0,0, -5,-5, 0, -10, -10, 0}, {2,2, 5,5, 5, 10, 10, 5} },
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
    { 4, 4, 100, 115, {"Kick", "Main Snare", "Hi-Hat", "Ghost Snr", "Clap", "Tom", "Conga", "Tambourine"}, {P_808_KICK, P_808_SNARE, P_808_CHH, P_808_SNARE, P_808_CLAP, P_808_TOM, P_808_PERC, P_808_FX}, {{2,4,0,0}, {2,4,0,0}, {4,6,0,0}, {2,4,0,0}, {2,4,0,0}, {3,4,0,0}, {3,4,6,0}, {4,6,0,0}}, {0,0, 5,0, 0, 0, 5, 5}, {5,10, 20,5, 5, 10, 15, 20} },
    { 4, 4, 100, 112, {"Punch Kick", "Snare", "Swing Hat", "Open Hat", "Clap", "Tom 1", "Tom 2", "Orch Hit"}, {P_808_KICK, P_808_SNARE, P_808_CHH, P_808_OHH, P_808_CLAP, P_808_TOM, P_808_PERC, P_808_FX}, {{2,4,0,0}, {2,4,0,0}, {6,12,0,0}, {2,4,0,0}, {2,4,0,0}, {2,4,0,0}, {2,4,0,0}, {2,4,0,0}}, {0,0, 15,0, 0, 0, 0, 0}, {5,5, 25,5, 5, 5, 5, 5} },
    { 4, 4, 80, 95,   {"Soft Kick", "Rimshot", "Loose Hat", "Ride", "Snap", "Tom", "Shaker", "Vinyl FX"}, {P_808_KICK, P_808_SNARE, P_808_CHH, P_808_OHH, P_808_CLAP, P_808_TOM, P_808_PERC, P_808_FX}, {{2,4,0,0}, {2,4,0,0}, {3,4,6,0}, {3,4,0,0}, {2,4,0,0}, {2,3,4,0}, {4,6,0,0}, {1,2,0,0}}, {-5, 10, 20, 10, 5, 0, 15, 0}, {2, 25, 40, 25, 20, 10, 30, 0} },
    { 4, 4, 85, 95,   {"Gritty Kick", "Fat Snare", "Hi-Hat", "Open Hat", "Clap", "Perc", "Scratch", "Sample"}, {P_808_KICK, P_808_SNARE, P_808_CHH, P_808_OHH, P_808_CLAP, P_808_TOM, P_808_PERC, P_808_FX}, {{2,4,0,0}, {2,4,0,0}, {3,4,6,0}, {2,4,0,0}, {2,4,0,0}, {3,4,0,0}, {2,4,0,0}, {1,2,4,0}}, {2, 5, 5, 0, 0, 0, 0, 0}, {8, 12, 15, 5, 5, 10, 5, 0} },
    { 5, 4, 120, 160, {"Kick", "Snare", "Hi-Hat", "Ride", "Ghost Snr", "Tom 1", "Tom 2", "Crash"}, {P_808_KICK, P_808_SNARE, P_808_CHH, P_808_OHH, P_808_CLAP, P_808_TOM, P_808_PERC, P_808_FX}, {{2,3,4,0}, {2,3,4,0}, {4,5,6,0}, {3,4,5,0}, {4,6,8,0}, {3,4,5,0}, {3,4,5,0}, {1,2,0,0}}, {0,0, 0,0, 0, 0, 0, 0}, {5,5, 5,5, 5, 5, 5, 5} },
    { 13, 8, 60, 85,  {"D.Kick (Gallop)", "Main Snare", "Max Stax", "Hat Bark", "High Tom", "Mid Tom", "Floor Tom", "Splash"}, {P_808_KICK, P_808_SNARE, P_808_CHH, P_808_OHH, P_808_TOM, P_808_TOM, P_808_TOM, P_808_FX}, {{2,4,0,0}, {2,4,0,0}, {4,6,8,0}, {2,4,0,0}, {3,4,5,0}, {3,4,5,0}, {3,4,5,0}, {1,2,0,0}}, {0, 2, 0, 4, 1, 2, 3, 0}, {0, 2, 0, 6, 1, 2, 3, 0} },
    { 12, 8, 60, 75,  {"Clap 1", "Clap 2", "Marimba 1", "Marimba 2", "Woodblock", "Pulse", "Phase 1", "Phase 2"}, {P_808_KICK, P_808_SNARE, P_808_CHH, P_808_OHH, P_808_CLAP, P_808_TOM, P_808_PERC, P_808_FX}, {{1,2,0,0}, {1,2,0,0}, {1,2,0,0}, {1,2,0,0}, {1,2,0,0}, {1,2,0,0}, {1,2,0,0}, {1,2,0,0}}, {0,0, 0,0, 0, 0, -20, 20}, {0,0, 0,0, 0, 0, -20, 20} },
    { 4, 4, 120, 150, {"Node C", "Node D", "Node F", "Node G", "Node A", "Node C^", "Node D^", "Node F^"}, {PLUCK_1, PLUCK_2, PLUCK_3, PLUCK_4, PLUCK_5, PLUCK_6, PLUCK_7, PLUCK_8}, {{2,3,5,7}, {2,3,5,7}, {2,3,5,7}, {2,3,5,7}, {2,3,5,7}, {2,3,5,7}, {2,3,5,7}, {2,3,5,7}}, {0,0,0,0,0,0,0,0}, {0,0,0,0,0,0,0,0} },
    { 4, 4, 120, 150, {"Chaos C", "Chaos D", "Chaos F", "Chaos G", "Chaos A", "Chaos C^", "Chaos D^", "Chaos F^"}, {PLUCK_1, PLUCK_2, PLUCK_3, PLUCK_4, PLUCK_5, PLUCK_6, PLUCK_7, PLUCK_8}, {{1,3,6,9}, {1,3,6,9}, {1,3,6,9}, {1,3,6,9}, {1,3,6,9}, {1,3,6,9}, {1,3,6,9}, {1,3,6,9}}, {-20,-20,-20,-20,-20,-20,-20,-20}, {20,20,20,20,20,20,20,20} },
    { 4, 4, 80, 140,  {"Drill Kick", "Drill Snr", "Drill Hat", "Open Hat", "Clap", "Perc", "Counter Snr", "808 Glide"}, {P_808_KICK, P_808_SNARE, P_808_CHH, P_808_OHH, P_808_CLAP, P_808_PERC, P_808_SNARE, P_808_BASS}, {{2,4,0,0}, {2,4,0,0}, {4,6,8,0}, {4,8,0,0}, {2,4,0,0}, {4,6,8,0}, {4,6,8,0}, {2,4,0,0}}, {0,0, 0,0, 0, 0, 0, 0}, {0,0, 0,0, 0, 0, 0, 0} },
    { 4, 4, 80, 100,  {"Base(Div2)", "Back(Div4)", "Trip(Div3)", "Quin(Div5)", "Sept(Div7)", "Sext(Div6)", "Oct(Div8)", "Prim(Div1)"}, {P_808_KICK, P_808_SNARE, P_808_CHH, P_808_PERC, P_808_PERC, P_808_TOM, P_808_FX, P_808_FX}, {{2,4,0,0}, {2,4,0,0}, {2,4,0,0}, {2,4,0,0}, {2,4,0,0}, {2,4,0,0}, {2,4,0,0}, {2,4,0,0}}, {0,0, 0,0, 0, 0, 0, 0}, {0,0, 0,0, 0, 0, 0, 0} }
} };

const int scalePatterns[19][8] = {
    {0, 2, 4, 5, 7, 9, 11, -1}, {0, 2, 3, 5, 7, 8, 10, -1}, {0, 2, 4, 7, 9, -1, -1, -1}, {0, 3, 5, 7, 10, -1, -1, -1},
    {0, 2, 3, 5, 7, 9, 10, -1}, {0, 2, 3, 5, 7, 8, 11, -1}, {0, 2, 4, 6, 7, 9, 11, -1}, {0, 2, 4, 5, 7, 9, 10, -1},
    {0, 1, 3, 5, 7, 8, 10, -1}, {0, 1, 3, 5, 6, 8, 10, -1}, {0, 2, 4, 6, 8, 10, -1, -1}, {0, 3, 5, 6, 7, 10, -1, -1},
    {0, 2, 3, 5, 7, 8, 10, -1}, {0, 2, 3, 5, 7, 9, 11, -1}, {0, 1, 3, 5, 7, 9, 10, -1}, {0, 2, 4, 6, 8, 9, 11, -1},
    {0, 1, 3, 4, 6, 8, 10, -1}, {0, 1, 3, 4, 6, 7, 9, 10}, {0, 3, 4, 7, 8, 11, -1, -1}
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
    for (int g = 0; g < 26; ++g) {
        if (g == 24) {
            userTuning[g].tempo.min = 80;
            userTuning[g].tempo.max = 140;
        }
        else if (g == 25) {
            userTuning[g].tempo.min = 80;
            userTuning[g].tempo.max = 100;
        }
        else {
            userTuning[g].tempo.min = genreTable[g].minTempo;
            userTuning[g].tempo.max = genreTable[g].maxTempo;
        }

        userTuning[g].tempoLocked = false;

        for (int t = 0; t < 8; ++t) {
            userTuning[g].allowedTimeSigs[t] = false;
            userTuning[g].tracks[t].divLocked = false;
            for (int d = 0; d < 8; ++d) userTuning[g].tracks[t].allowedDivs[d] = false;

            if (g == 25) {
                userTuning[g].tracks[t].cmplx.min = 0;
                userTuning[g].tracks[t].cmplx.max = 3;
                userTuning[g].tracks[t].entrp.min = 0;
                userTuning[g].tracks[t].entrp.max = 10;
            }
            else if (g >= 22 && g <= 23) {
                userTuning[g].tracks[t].cmplx.min = 30;
                userTuning[g].tracks[t].cmplx.max = 70;
                userTuning[g].tracks[t].entrp.min = 0;
                userTuning[g].tracks[t].entrp.max = 0;
            }
            else {
                userTuning[g].tracks[t].cmplx.min = 0;
                userTuning[g].tracks[t].cmplx.max = 0;
                userTuning[g].tracks[t].entrp.min = 0;
                userTuning[g].tracks[t].entrp.max = 0;
            }

            userTuning[g].tracks[t].cmplxLocked = false;
            userTuning[g].tracks[t].entrpLocked = false;
            userTuning[g].tracks[t].shift.min = 0; userTuning[g].tracks[t].shift.max = 0;
            userTuning[g].tracks[t].shiftLocked = false;
        }

        for (int f = 0; f < 4; ++f) userTuning[g].allowedFills[f] = false;
        if (g == 0 || g == 1 || g == 4 || g == 7 || g == 9 || g == 10 || g == 13 || g == 18 || g == 24) { userTuning[g].allowedFills[1] = true; userTuning[g].allowedFills[3] = true; }
        else if (g == 3 || g == 5 || g == 6 || g == 19 || g == 20 || g == 22 || g == 23 || g == 25) { userTuning[g].allowedFills[0] = true; userTuning[g].allowedFills[2] = true; }
        else { userTuning[g].allowedFills[0] = true; userTuning[g].allowedFills[1] = true; }
    }
}

const GenreDefinition& AIDrumMachineAudioProcessor::getGenreDef(int index) {
    if (index < 0 || index >= 26) return genreTable[0];
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

    bool isArp = arpMode.load();
    int fillBar = fillBarTarget.load();
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
        int tsChoices[8]; int numTsChoices = 0;
        for (int i = 0; i < 8; ++i) { if (tuning.allowedTimeSigs[i]) tsChoices[numTsChoices++] = i; }
        if (numTsChoices > 0) {
            int pickedTs = tsChoices[random.nextInt(juce::jmax(1, numTsChoices))];
            num = tuning.timeSigOptions[pickedTs].num;
            den = tuning.timeSigOptions[pickedTs].den;
            timeSigNumerator.store(num); timeSigDenominator.store(den);
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
            if (!trackCmplxLocked[trk]) trackComplexity[trk] = random.nextInt(11);
            if (!trackEntrpLocked[trk]) trackEntropy[trk] = random.nextInt(11);
            if (!trackShiftLocked[trk]) trackShiftUI[trk] = (trk == 0) ? 0 : (random.nextInt(11) - 5);

            int degree = trackDegreeUI[trk]; int octave = trackOctaveUI[trk]; int attempts = 0; bool unique = false;
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
                    octave += random.nextInt(juce::Range<int>(-1, 2)); octave = juce::jlimit(-2, 2, octave);
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

        int baseDiv = juce::jmax(1, trackDivisionsUI[0]);
        int n = baseDiv * num * bars;
        n = juce::jlimit(1, 1024, n);
        int currentTrk = 0; int currentChordBase = 0; bool stepOccupied[1024] = { false };
        std::vector<std::vector<int>> popMotif;
        int masterCmplx = trackComplexity[0] * 10;

        if (arpPreset == 7) {
            std::vector<int> trkOrder = { 0, 1, 2, 3, 4, 5, 6, 7 };
            for (int i = 7; i > 0; --i) { int rIdx = random.nextInt(i + 1); std::swap(trkOrder[i], trkOrder[rIdx]); }
            for (int trk : trkOrder) {
                if (trackLocked[trk]) continue;
                int tDiv = juce::jmax(1, trackDivisionsUI[trk]);
                int cmplx = trackComplexity[trk];
                int entrp = trackEntropy[trk];
                int trkSteps = tDiv * num * bars;
                trkSteps = juce::jlimit(1, 1024, trkSteps);

                for (int s = 0; s < trkSteps; ++s) {
                    int globalJ = (s * baseDiv) / tDiv;
                    if (globalJ >= 1024) globalJ = 1023;

                    int prob = cmplx; if (prob < 5) prob = 5;
                    if (random.nextInt(100) < prob) {
                        if (isMono && stepOccupied[globalJ]) continue;
                        int finalVel = 70 + random.nextInt(30);
                        if (entrp > 0) {
                            int jitter = (int)((entrp / 100.0f) * 50.0f);
                            finalVel -= random.nextInt(juce::jmax(1, jitter + 1));
                        }
                        if (trackDynamic[trk] && s % juce::jmax(1, (tDiv * 2)) == 0) finalVel += trackDynamicAmount[trk];

                        drumPatternUI[trk][s] = juce::jlimit(1, 127, finalVel);
                        if (isMono) stepOccupied[globalJ] = true;
                    }
                }
            }
        }
        else {
            for (int j = 0; j < n; ++j) {
                int currentBarOfStep = j / juce::jmax(1, (baseDiv * num));
                int beatInBar = (j % juce::jmax(1, (baseDiv * num))) / juce::jmax(1, baseDiv);
                int stepInBar = j % juce::jmax(1, (baseDiv * num));
                bool isFillPortion = (fillBar > 0) && (currentBarOfStep == (fillBar - 1)) && (beatInBar >= juce::jmax(1, num / 2));
                bool stepActive = false;

                if (arpPreset == 8) {
                    int k = juce::jmax(1, (n * (5 + masterCmplx)) / 100);
                    int eucOffset = random.nextInt(juce::jmax(1, n));
                    stepActive = (((static_cast<int64_t>(j) + eucOffset) * k) % juce::jmax(1, n) < k);
                }
                else if (arpPreset == 9) {
                    if (currentBarOfStep < 2) {
                        bool isAnticipation = (stepInBar % juce::jmax(1, baseDiv) == baseDiv - 1) && (random.nextInt(100) < (20 + masterCmplx * 0.8f));
                        bool isStrongBeat = (stepInBar % juce::jmax(1, baseDiv) == 0) && (random.nextInt(100) < (40 + masterCmplx * 0.6f));
                        bool isPassingNote = (masterCmplx > 40) && (stepInBar % juce::jmax(1, (baseDiv > 1 ? baseDiv / 2 : 1)) == 0) && (random.nextInt(100) < (masterCmplx - 40));
                        stepActive = isAnticipation || isStrongBeat || isPassingNote;
                        if (stepActive) { currentTrk = random.nextInt(8); popMotif.push_back({ j, currentTrk }); }
                    }
                    else if (currentBarOfStep == 2) {
                        int localJ = j % juce::jmax(1, (baseDiv * num * 2));
                        for (auto& m : popMotif) { if (m[0] == localJ) { stepActive = true; currentTrk = m[1]; break; } }
                        if (random.nextInt(100) < (10 + masterCmplx * 0.3f)) stepActive = !stepActive;
                    }
                    else {
                        stepActive = (stepInBar % juce::jmax(1, (baseDiv * 2)) == 0) || (random.nextInt(100) < (20 + masterCmplx * 0.5f));
                        if (stepActive) currentTrk = random.nextInt(8);
                    }
                }
                else {
                    stepActive = (random.nextInt(100) < (30 + masterCmplx * 0.5f));
                }

                if (isFillPortion) stepActive = (arpPreset % 2 == 0) ? true : false;
                if (!stepActive) continue;

                int vel = 70 + random.nextInt(30); std::vector<int> tracksToHit;
                switch (arpPreset) {
                case 0: tracksToHit.push_back(currentTrk); currentTrk = (currentTrk + 1) % 8; break;
                case 1: tracksToHit.push_back(currentTrk); currentTrk = (currentTrk + 7) % 8; break;
                case 2: tracksToHit.push_back(currentChordBase % 8); tracksToHit.push_back((currentChordBase + 2) % 8); tracksToHit.push_back((currentChordBase + 4) % 8); currentChordBase = (currentChordBase + 1) % 8; break;
                case 3: tracksToHit.push_back(currentChordBase % 8); tracksToHit.push_back((currentChordBase + 2) % 8); tracksToHit.push_back((currentChordBase + 4) % 8); tracksToHit.push_back((currentChordBase + 6) % 8); currentChordBase = (currentChordBase + 1) % 8; break;
                case 4: tracksToHit.push_back(currentTrk); currentTrk = (currentTrk + 3) % 8; break;
                case 5: tracksToHit.push_back(currentTrk); currentTrk = (currentTrk + 5) % 8; break;
                case 6: tracksToHit.push_back(currentChordBase % 8); tracksToHit.push_back((currentChordBase + 3) % 8); tracksToHit.push_back((currentChordBase + 6) % 8); currentChordBase = (currentChordBase + 1) % 8; break;
                case 8: tracksToHit.push_back(currentTrk); currentTrk = (currentTrk + 1) % 8; break;
                case 9: tracksToHit.push_back(currentTrk); break;
                default: int nextTrk = random.nextInt(8); while (nextTrk == currentTrk) nextTrk = random.nextInt(8); currentTrk = nextTrk; tracksToHit.push_back(currentTrk); break;
                }

                for (int t : tracksToHit) {
                    if (!trackLocked[t]) {
                        int entrp = trackEntropy[t] * 10;
                        if (isMono && stepOccupied[j]) continue;
                        int finalVel = vel;
                        if (entrp > 0) { int jitter = (int)((entrp / 100.0f) * 50.0f); finalVel -= random.nextInt(juce::jmax(1, jitter + 1)); }
                        if (trackDynamic[t] && (stepInBar % juce::jmax(1, baseDiv) == 0 || stepInBar % juce::jmax(1, baseDiv) == baseDiv - 1)) { finalVel = juce::jlimit(1, 127, finalVel + trackDynamicAmount[t]); }
                        drumPatternUI[t][j] = juce::jlimit(1, 127, finalVel);
                        if (isMono) stepOccupied[j] = true;
                    }
                }
            }
        }
    }
    else {
        int fillChoices[4] = { 0, 0, 0, 0 }; int numFillChoices = 0;
        for (int i = 0; i < 4; ++i) if (tuning.allowedFills[i]) fillChoices[numFillChoices++] = i;
        int fillTypology = numFillChoices > 0 ? fillChoices[random.nextInt(juce::jmax(1, numFillChoices))] : 0;

        for (int trk = 0; trk < 8; ++trk) {
            if (trackLocked[trk]) continue;
            TrackTuning& tt = tuning.tracks[trk];

            bool divLck = trackDivLocked[trk] || tt.divLocked;

            if (genre == 25) {
                int targetDiv = (trk == 0) ? 2 : (trk == 1) ? 4 : (trk == 2) ? 3 : (trk == 3) ? 5 : (trk == 4) ? 7 : (trk == 5) ? 6 : (trk == 6) ? 8 : 1;
                if (!trackDivLocked[trk]) trackDivisionsUI[trk] = targetDiv;
            }
            else {
                if (!divLck) {
                    std::vector<int> candidates; for (int i = 0; i < 8; ++i) { if (tt.allowedDivs[i]) candidates.push_back(i + 1); }
                    int newDiv = 4; if (!candidates.empty()) newDiv = candidates[random.nextInt((int)candidates.size())];
                    if (newDiv > maxDiv) newDiv = maxDiv; trackDivisionsUI[trk] = newDiv;
                }
            }

            bool cmplxLck = trackCmplxLocked[trk] || tt.cmplxLocked;
            if (!cmplxLck) {
                int cMin = std::min(tt.cmplx.min, tt.cmplx.max); int cMax = std::max(tt.cmplx.min, tt.cmplx.max);
                if (cMax > cMin) trackComplexity[trk] = random.nextInt(juce::Range<int>(cMin, cMax + 1)); else trackComplexity[trk] = cMin;
            }

            bool entrpLck = trackEntrpLocked[trk] || tt.entrpLocked;
            if (!entrpLck) {
                int eMin = std::min(tt.entrp.min, tt.entrp.max); int eMax = std::max(tt.entrp.min, tt.entrp.max);
                if (eMax > eMin) trackEntropy[trk] = random.nextInt(juce::Range<int>(eMin, eMax + 1)); else trackEntropy[trk] = eMin;
            }

            bool shiftLck = trackShiftLocked[trk] || tt.shiftLocked;
            if (!shiftLck) {
                int sMin = std::min(tt.shift.min, tt.shift.max); int sMax = std::max(tt.shift.min, tt.shift.max);
                if (sMax > sMin) trackShiftUI[trk] = (trk == 0) ? 0 : random.nextInt(juce::Range<int>(sMin, sMax + 1)); else trackShiftUI[trk] = (trk == 0) ? 0 : sMin;
            }

            int div = juce::jmax(1, trackDivisionsUI[trk]); int n = div * num * bars;
            n = juce::jlimit(1, 1024, n);
            int offset = random.nextInt(juce::Range<int>(0, juce::jmax(1, n)));
            int cmplx = trackComplexity[trk];
            int entrp = trackEntropy[trk];

            for (int j = 0; j < n; ++j) {
                int currentBarOfStep = j / juce::jmax(1, (div * num));
                int beatInBar = (j % juce::jmax(1, (div * num))) / juce::jmax(1, div);
                int stepInBar = j % juce::jmax(1, (div * num));

                bool isFillActiveBar = (fillBar > 0) && (currentBarOfStep == (fillBar - 1));
                bool isFillPortion = isFillActiveBar && (beatInBar >= juce::jmax(1, num / 2));
                bool isAnchor = false; bool isNegativeAnchor = false; int anchorVel = 100;
                bool isSubAnchor = false; int subAnchorProb = cmplx;

                if (isFillPortion) {
                    int localStep = stepInBar - div * (num / 2);
                    if (localStep < 0) localStep = stepInBar;
                    isNegativeAnchor = true;

                    switch (genre) {
                    case 0: // Techno
                        if (fillTypology == 0) { if (trk == 7 || trk == 4) { if (localStep % juce::jmax(1, div / 2) == 0 && random.nextInt(100) < (50 + cmplx / 2)) { isAnchor = true; anchorVel = 70 + random.nextInt(30); isNegativeAnchor = false; } } if (trk == 0 && localStep % div == 0) { isAnchor = true; isNegativeAnchor = false; } }
                        else if (fillTypology == 1) { if (localStep >= div * (num / 2) - juce::jmax(1, div / 2)) { isNegativeAnchor = true; } else if (trk == 0 || trk == 1) { isAnchor = true; anchorVel = 90; isNegativeAnchor = false; } }
                        else if (fillTypology == 2) { int k = 5 + (trk % 2); if ((localStep * k) % juce::jmax(1, div * 2) < k) { if (trk >= 4) { isAnchor = true; anchorVel = 80; isNegativeAnchor = false; } } if (trk == 0 && localStep % div == 0) { isAnchor = true; isNegativeAnchor = false; } }
                        else if (fillTypology == 3) { if (trk == 1) { if (random.nextInt(100) < (30 + (localStep * 70) / juce::jmax(1, div * 2))) { isAnchor = true; anchorVel = 60 + (int)(40.0f * localStep / juce::jmax(1, div * 2)); isNegativeAnchor = false; } } else if (trk == 4 && localStep > div) { isAnchor = true; anchorVel = 70; isNegativeAnchor = false; } if (trk == 0 && localStep % div == 0) { isAnchor = true; isNegativeAnchor = false; } }
                        break;
                    case 1: // House
                        if (fillTypology == 0) { if (trk == 5 || trk == 6) { if (localStep % juce::jmax(1, div / 4) == 0 && random.nextInt(100) < 40) { isAnchor = true; anchorVel = 70 + random.nextInt(20); isNegativeAnchor = false; } } if (trk == 0 && localStep == 0) { isAnchor = true; isNegativeAnchor = false; } }
                        else if (fillTypology == 1) { if (localStep == div + juce::jmax(1, div / 2)) { if (trk == 4) { isAnchor = true; anchorVel = 100; isNegativeAnchor = false; } } else if (localStep > 0) { isNegativeAnchor = true; } if (trk == 0 && localStep == 0) { isAnchor = true; isNegativeAnchor = false; } }
                        else if (fillTypology == 2) { int k = 3; if ((localStep * k) % juce::jmax(1, div * 2) < k) { if (trk == 4 || trk == 5) { isAnchor = true; anchorVel = 85; isNegativeAnchor = false; } } if (trk == 2 && localStep % juce::jmax(1, div / 2) == 0) { isAnchor = true; anchorVel = 50; isNegativeAnchor = false; } }
                        else if (fillTypology == 3) { if (trk == 1 || trk == 4) { if (random.nextInt(100) < (50 + cmplx / 2)) { isAnchor = true; anchorVel = 70 + (int)(30.0f * localStep / juce::jmax(1, div * 2)); isNegativeAnchor = false; } } if (trk == 0 && localStep % div == 0) { isAnchor = true; isNegativeAnchor = false; } }
                        break;
                    case 2: // UK Garage
                        if (fillTypology == 0) { if (trk >= 4 && trk <= 6) { if (localStep % juce::jmax(1, div / 2) != 0 && random.nextInt(100) < 60) { isAnchor = true; anchorVel = 60 + random.nextInt(30); isNegativeAnchor = false; } } }
                        else if (fillTypology == 1) { if (localStep == div * (num / 2) - juce::jmax(1, div / 2)) { if (trk == 1) { isAnchor = true; anchorVel = 100; isNegativeAnchor = false; } } else if (trk == 7 && localStep % div == 0) { isAnchor = true; anchorVel = 70; isNegativeAnchor = false; } else { isNegativeAnchor = true; } }
                        else if (fillTypology == 2) { int k = 7; if ((localStep * k) % juce::jmax(1, div * 2) < k) { if (trk == 2 || trk == 5) { isAnchor = true; anchorVel = 75; isNegativeAnchor = false; } } if (trk == 0 && localStep == 0) { isAnchor = true; isNegativeAnchor = false; } }
                        else if (fillTypology == 3) { if (trk == 5) { int p = localStep % juce::jmax(1, div); if (p == 0 || p == juce::jmax(1, div / 3) || p == juce::jmax(1, div * 2 / 3)) { isAnchor = true; anchorVel = 80 + random.nextInt(20); isNegativeAnchor = false; } } if (trk == 0 && localStep == 0) { isAnchor = true; isNegativeAnchor = false; } }
                        break;
                    case 3: // D&B
                        if (fillTypology == 0) { if (trk == 1 || trk == 0) { if (localStep % juce::jmax(1, div / 2) == 0 && random.nextInt(100) < 50) { isAnchor = true; anchorVel = 80 + random.nextInt(20); isNegativeAnchor = false; } } if (trk == 5 || trk == 6) { if (localStep % juce::jmax(1, div / 4) == 0 && random.nextInt(100) < 40) { isAnchor = true; anchorVel = 40; isNegativeAnchor = false; } } }
                        else if (fillTypology == 1) { if (localStep == div * (num / 2) - juce::jmax(1, div / 2)) { if (trk == 1) { isAnchor = true; anchorVel = 100; isNegativeAnchor = false; } } else if (localStep > 0) { isNegativeAnchor = true; } if (trk == 7 && localStep == 0) { isAnchor = true; anchorVel = 90; isNegativeAnchor = false; } }
                        else if (fillTypology == 2) { int k = 11; if ((localStep * k) % juce::jmax(1, div * 2) < k) { if (trk == 2 || trk == 3 || trk == 5) { isAnchor = true; anchorVel = 70; isNegativeAnchor = false; } } if (trk == 1 && localStep == div) { isAnchor = true; isNegativeAnchor = false; } }
                        else if (fillTypology == 3) { if (trk == 1) { if (random.nextInt(100) < 80) { isAnchor = true; anchorVel = 70 + random.nextInt(30); isNegativeAnchor = false; } } if (trk == 0 && (localStep == 0 || localStep == div + juce::jmax(1, div / 2))) { isAnchor = true; isNegativeAnchor = false; } }
                        break;
                    case 4: // Trap
                        if (fillTypology == 0) { if (trk == 2) { if (localStep % juce::jmax(1, div / 4) == 0 && random.nextInt(100) < (70 + cmplx / 3)) { isAnchor = true; anchorVel = 50 + random.nextInt(50); isNegativeAnchor = false; } } if (trk == 0 && localStep == 0) { isAnchor = true; isNegativeAnchor = false; } }
                        else if (fillTypology == 1) { if (localStep == div) { if (trk == 1 || trk == 4) { isAnchor = true; anchorVel = 100; isNegativeAnchor = false; } } else if (localStep > 0) { isNegativeAnchor = true; } }
                        else if (fillTypology == 2) { int k = 9; if ((localStep * k) % juce::jmax(1, div * 2) < k) { if (trk == 2 || trk == 5) { isAnchor = true; anchorVel = 80; isNegativeAnchor = false; } } if (trk == 0 && localStep == 0) { isAnchor = true; isNegativeAnchor = false; } }
                        else if (fillTypology == 3) { if (trk == 2) { isAnchor = true; anchorVel = 100 - (localStep * 5); isNegativeAnchor = false; } if (trk == 0 && localStep == 0) { isAnchor = true; isNegativeAnchor = false; } }
                        break;
                    case 5: // Footwork
                        if (fillTypology == 0) { if (trk >= 4) { if (localStep % juce::jmax(1, div / 4) == 0 && random.nextInt(100) < 50) { isAnchor = true; anchorVel = 70 + random.nextInt(20); isNegativeAnchor = false; } } }
                        else if (fillTypology == 1) { if (localStep == div) { if (trk == 1) { isAnchor = true; anchorVel = 100; isNegativeAnchor = false; } } else { isNegativeAnchor = true; } if (trk == 7 && localStep == 0) { isAnchor = true; isNegativeAnchor = false; } }
                        else if (fillTypology == 2) { int k = 5; if ((localStep * k) % juce::jmax(1, div * 2) < k) { if (trk == 5 || trk == 6) { isAnchor = true; anchorVel = 80; isNegativeAnchor = false; } } }
                        else if (fillTypology == 3) { if (trk == 1 || trk == 4) { if (localStep % juce::jmax(1, div / 2) == 0) { isAnchor = true; anchorVel = 90; isNegativeAnchor = false; } } if (trk == 0 && localStep % juce::jmax(1, div * 3) == 0) { isAnchor = true; isNegativeAnchor = false; } }
                        break;
                    case 6: // IDM/Breakcore
                        if (fillTypology == 0) { if (random.nextInt(100) < 40) { isAnchor = true; anchorVel = 10 + random.nextInt(110); isNegativeAnchor = false; } }
                        else if (fillTypology == 1) { if (trk == 6 && localStep % 3 == 0) { isAnchor = true; anchorVel = 50 - (localStep * 2); isNegativeAnchor = false; } else { isNegativeAnchor = true; } }
                        else if (fillTypology == 2) { int k = (trk % 2 == 0) ? 5 : 7; if ((localStep * k) % juce::jmax(1, div * 2) < k) { isAnchor = true; anchorVel = 70; isNegativeAnchor = false; } }
                        else if (fillTypology == 3) { if (trk == 1 || trk == 4) { if (localStep % juce::jmax(1, div / 4) == 0) { isAnchor = true; anchorVel = 90; isNegativeAnchor = false; } } }
                        break;
                    case 7: // Dubstep
                        if (fillTypology == 0) { if (trk == 5 || trk == 6) { if (localStep % juce::jmax(1, div / 2) == 0 && random.nextInt(100) < 60) { isAnchor = true; anchorVel = 90; isNegativeAnchor = false; } } if (trk == 0 && localStep == 0) { isAnchor = true; isNegativeAnchor = false; } }
                        else if (fillTypology == 1) { if (localStep == 0) { if (trk == 1) { isAnchor = true; anchorVel = 120; isNegativeAnchor = false; } } else { isNegativeAnchor = true; } }
                        else if (fillTypology == 2) { int k = 7; if ((localStep * k) % juce::jmax(1, div * 2) < k) { if (trk == 2 || trk == 3) { isAnchor = true; anchorVel = 85; isNegativeAnchor = false; } } }
                        else if (fillTypology == 3) { if (trk == 1) { if (localStep >= div) { isAnchor = true; anchorVel = 80 + (localStep * 2); isNegativeAnchor = false; } } if (trk == 2) { isAnchor = true; anchorVel = 90 - (localStep * 3); isNegativeAnchor = false; } }
                        break;
                    case 8: // Afrobeat
                        if (fillTypology == 0) { if (trk >= 4) { if (localStep % juce::jmax(1, div / 2) == 0 && random.nextInt(100) < 70) { isAnchor = true; anchorVel = 70 + random.nextInt(20); isNegativeAnchor = false; } } }
                        else if (fillTypology == 1) { if (localStep == div + juce::jmax(1, div / 2)) { if (trk == 7 || trk == 4) { isAnchor = true; anchorVel = 90; isNegativeAnchor = false; } } else if (localStep > 0) { isNegativeAnchor = true; } }
                        else if (fillTypology == 2) { int k = 5; if ((localStep * k) % juce::jmax(1, div * 2) < k) { if (trk == 5 || trk == 6) { isAnchor = true; anchorVel = 85; isNegativeAnchor = false; } } if (trk == 0 && localStep == 0) { isAnchor = true; isNegativeAnchor = false; } }
                        else if (fillTypology == 3) { if (trk == 5 || trk == 6) { if (random.nextInt(100) < 60) { isAnchor = true; anchorVel = 60 + (int)(40.0f * localStep / juce::jmax(1, div * 2)); isNegativeAnchor = false; } } if (trk == 0 && localStep % div == 0) { isAnchor = true; isNegativeAnchor = false; } }
                        break;
                    case 9: // Gqom
                        if (fillTypology == 0) { if (trk >= 5) { if (localStep % juce::jmax(1, div / 4) == 0 && random.nextInt(100) < 50) { isAnchor = true; anchorVel = 80 + random.nextInt(20); isNegativeAnchor = false; } } }
                        else if (fillTypology == 1) { if (localStep == div * (num / 2) - 1) { if (trk == 0) { isAnchor = true; anchorVel = 100; isNegativeAnchor = false; } } else { isNegativeAnchor = true; } }
                        else if (fillTypology == 2) { int k = 3; if ((localStep * k) % juce::jmax(1, div * 2) < k) { if (trk == 5 || trk == 6) { isAnchor = true; anchorVel = 85; isNegativeAnchor = false; } } }
                        else if (fillTypology == 3) { if (trk == 0) { if (localStep % juce::jmax(1, div / 2) == 0) { isAnchor = true; anchorVel = 90; isNegativeAnchor = false; } } if (trk == 7 && localStep == 0) { isAnchor = true; isNegativeAnchor = false; } }
                        break;
                    case 10: // Amapiano
                        if (fillTypology == 0) { if (trk >= 5) { if (localStep % juce::jmax(1, div / 2) == 0 && random.nextInt(100) < 60) { isAnchor = true; anchorVel = 85; isNegativeAnchor = false; } } if (trk == 0 && localStep == 0) { isAnchor = true; isNegativeAnchor = false; } }
                        else if (fillTypology == 1) { if (localStep == div) { if (trk == 7) { isAnchor = true; anchorVel = 100; isNegativeAnchor = false; } } else if (localStep > 0) { isNegativeAnchor = true; } }
                        else if (fillTypology == 2) { int k = 5; if ((localStep * k) % juce::jmax(1, div * 2) < k) { if (trk == 6) { isAnchor = true; anchorVel = 100; isNegativeAnchor = false; } } }
                        else if (fillTypology == 3) { if (trk == 6) { if (random.nextInt(100) < 70) { isAnchor = true; anchorVel = 80 + random.nextInt(20); isNegativeAnchor = false; } } if (trk == 0 && localStep == 0) { isAnchor = true; isNegativeAnchor = false; } }
                        break;
                    case 11: // Indian Classical
                        if (fillTypology == 0) { if (trk >= 4) { if (localStep % juce::jmax(1, div / 4) == 0 && random.nextInt(100) < 50) { isAnchor = true; anchorVel = 60 + random.nextInt(30); isNegativeAnchor = false; } } }
                        else if (fillTypology == 1) { if (localStep == 0) { if (trk == 0) { isAnchor = true; anchorVel = 90; isNegativeAnchor = false; } } else { isNegativeAnchor = true; } }
                        else if (fillTypology == 2) { int k = 7; if ((localStep * k) % juce::jmax(1, div * 2) < k) { if (trk == 5 || trk == 6) { isAnchor = true; anchorVel = 80; isNegativeAnchor = false; } } }
                        else if (fillTypology == 3) { if (trk >= 1 && trk <= 3) { if (random.nextInt(100) < 70) { isAnchor = true; anchorVel = 60 + random.nextInt(40); isNegativeAnchor = false; } } }
                        break;
                    case 12: // Samba/Bossa
                        if (fillTypology == 0) { if (trk >= 4) { if (localStep % juce::jmax(1, div / 2) != 0 && random.nextInt(100) < 60) { isAnchor = true; anchorVel = 75; isNegativeAnchor = false; } } }
                        else if (fillTypology == 1) { if (localStep == div * (num / 2) - 1) { if (trk == 0 || trk == 7) { isAnchor = true; anchorVel = 100; isNegativeAnchor = false; } } else if (localStep == div * (num / 2) - juce::jmax(1, div / 2)) { if (trk == 1) { isAnchor = true; anchorVel = 90; isNegativeAnchor = false; } } else { isNegativeAnchor = true; } }
                        else if (fillTypology == 2) { int k = 5; if ((localStep * k) % juce::jmax(1, div * 2) < k) { if (trk == 5 || trk == 6) { isAnchor = true; anchorVel = 80; isNegativeAnchor = false; } } }
                        else if (fillTypology == 3) { if (trk >= 4 && trk <= 6) { if (localStep % juce::jmax(1, div / 2) == 0) { isAnchor = true; anchorVel = 60 + (int)(40.0f * localStep / juce::jmax(1, div * (num / 2))); isNegativeAnchor = false; } } }
                        break;
                    case 13: // Reggaeton
                        if (fillTypology == 0) { if (trk == 4 || trk == 5) { if (localStep % juce::jmax(1, div / 2) == 0 && random.nextInt(100) < 70) { isAnchor = true; anchorVel = 80; isNegativeAnchor = false; } } if (trk == 0 && localStep == 0) { isAnchor = true; isNegativeAnchor = false; } }
                        else if (fillTypology == 1) { if (localStep == 0) { if (trk == 7) { isAnchor = true; anchorVel = 100; isNegativeAnchor = false; } } else { isNegativeAnchor = true; } }
                        else if (fillTypology == 2) { int k = 3; if ((localStep * k) % juce::jmax(1, div * 2) < k) { if (trk == 5 || trk == 6) { isAnchor = true; anchorVel = 85; isNegativeAnchor = false; } } if (trk == 0 && localStep % div == 0) { isAnchor = true; isNegativeAnchor = false; } }
                        else if (fillTypology == 3) { if (trk == 1) { if (random.nextInt(100) < (40 + (localStep * 60) / juce::jmax(1, div * 2))) { isAnchor = true; anchorVel = 60 + (int)(40.0f * localStep / juce::jmax(1, div * 2)); isNegativeAnchor = false; } } if (trk == 0 && localStep % div == 0) { isAnchor = true; isNegativeAnchor = false; } }
                        break;
                    case 14: // Gamelan
                        if (fillTypology == 0) { if (trk >= 4) { if (localStep % juce::jmax(1, div / 4) == 0 && random.nextInt(100) < 50) { isAnchor = true; anchorVel = 80; isNegativeAnchor = false; } } }
                        else if (fillTypology == 1) { if (localStep == 0) { if (trk == 0) { isAnchor = true; anchorVel = 100; isNegativeAnchor = false; } } else { isNegativeAnchor = true; } }
                        else if (fillTypology == 2) { int k = 5; if ((localStep * k) % juce::jmax(1, div * 2) < k) { if (trk == 5 || trk == 6) { isAnchor = true; anchorVel = 80; isNegativeAnchor = false; } } }
                        else if (fillTypology == 3) { if (trk >= 1 && trk <= 3) { if (random.nextInt(100) < 80) { isAnchor = true; anchorVel = 90; isNegativeAnchor = false; } } }
                        break;
                    case 15: // Funk
                        if (fillTypology == 0) { if (trk == 3) { if (localStep % juce::jmax(1, div / 2) != 0 && random.nextInt(100) < 70) { isAnchor = true; anchorVel = 40 + random.nextInt(30); isNegativeAnchor = false; } } }
                        else if (fillTypology == 1) { if (localStep == div * (num / 2) - 1) { if (trk == 0 || trk == 1) { isAnchor = true; anchorVel = 100; isNegativeAnchor = false; } } else if (localStep == div * (num / 2) - juce::jmax(1, div / 2)) { if (trk == 3) { isAnchor = true; anchorVel = 50; isNegativeAnchor = false; } } else { isNegativeAnchor = true; } }
                        else if (fillTypology == 2 || fillTypology == 3) {
                            bool occ = false; for (int t = 0; t < trk; ++t) { if (drumPatternUI[t][j] > 0) occ = true; }
                            if (!occ) { int phase = localStep % juce::jmax(1, div); if (phase == 0 && trk == 1) { isAnchor = true; anchorVel = 90; isNegativeAnchor = false; } else if (phase == juce::jmax(1, div / 4) && trk == 3) { isAnchor = true; anchorVel = 60; isNegativeAnchor = false; } else if (phase == juce::jmax(1, div / 2) && trk == 0) { isAnchor = true; anchorVel = 85; isNegativeAnchor = false; } else if (phase == juce::jmax(1, div * 3 / 4) && (trk == 0 || trk == 4)) { isAnchor = true; anchorVel = 80; isNegativeAnchor = false; } else { isNegativeAnchor = true; } }
                            else { isNegativeAnchor = true; }
                        }
                        break;
                    case 16: // New Jack Swing
                        if (fillTypology == 0) { if (trk == 4 || trk == 5) { if (localStep % juce::jmax(1, div / 2) == 0 && random.nextInt(100) < 70) { isAnchor = true; anchorVel = 80; isNegativeAnchor = false; } } if (trk == 0 && localStep == 0) { isAnchor = true; isNegativeAnchor = false; } }
                        else if (fillTypology == 1) { if (localStep == div) { if (trk == 7) { isAnchor = true; anchorVel = 100; isNegativeAnchor = false; } } else if (localStep > 0) { isNegativeAnchor = true; } }
                        else if (fillTypology == 2) { int k = 3; if ((localStep * k) % juce::jmax(1, div * 2) < k) { if (trk == 4 || trk == 5) { isAnchor = true; anchorVel = 85; isNegativeAnchor = false; } } }
                        else if (fillTypology == 3) { if (trk == 1 || trk == 4) { if (localStep % juce::jmax(1, div / 2) == 0) { isAnchor = true; anchorVel = 90; isNegativeAnchor = false; } } if (trk == 0 && localStep == 0) { isAnchor = true; isNegativeAnchor = false; } }
                        break;
                    case 17: // Neo Soul
                        if (fillTypology == 0) { if (trk >= 5) { if (localStep % juce::jmax(1, div / 2) != 0 && random.nextInt(100) < 50) { isAnchor = true; anchorVel = 70; isNegativeAnchor = false; } } if (trk == 0 && localStep == 0) { isAnchor = true; isNegativeAnchor = false; } }
                        else if (fillTypology == 1) { if (localStep == div * (num / 2) - 1) { if (trk == 0 || trk == 1) { isAnchor = true; anchorVel = 85; isNegativeAnchor = false; } } else { isNegativeAnchor = true; } }
                        else if (fillTypology == 2) { int k = 5; if ((localStep * k) % juce::jmax(1, div * 2) < k) { if (trk == 5 || trk == 6) { isAnchor = true; anchorVel = 75; isNegativeAnchor = false; } } }
                        else if (fillTypology == 3) { if (trk == 1 || trk == 4) { if (random.nextInt(100) < 60) { isAnchor = true; anchorVel = 60 + (int)(30.0f * localStep / juce::jmax(1, div * 2)); isNegativeAnchor = false; } } if (trk == 0 && localStep == 0) { isAnchor = true; isNegativeAnchor = false; } }
                        break;
                    case 18: // Hip Hop
                        if (fillTypology == 0) { if (trk >= 4) { if (localStep % juce::jmax(1, div / 2) == 0 && random.nextInt(100) < 60) { isAnchor = true; anchorVel = 80; isNegativeAnchor = false; } } if (trk == 0 && localStep == 0) { isAnchor = true; isNegativeAnchor = false; } }
                        else if (fillTypology == 1) { if (localStep == div * (num / 2) - 1) { if (trk == 0) { isAnchor = true; anchorVel = 95; isNegativeAnchor = false; } } else { isNegativeAnchor = true; } }
                        else if (fillTypology == 2) { int k = 3; if ((localStep * k) % juce::jmax(1, div * 2) < k) { if (trk == 4 || trk == 5) { isAnchor = true; anchorVel = 85; isNegativeAnchor = false; } } }
                        else if (fillTypology == 3) { if (trk == 1) { if (random.nextInt(100) < (60 + cmplx / 3)) { isAnchor = true; anchorVel = 70 + (int)(30.0f * localStep / juce::jmax(1, div * 2)); isNegativeAnchor = false; } } if (trk == 0 && localStep == 0) { isAnchor = true; isNegativeAnchor = false; } }
                        break;
                    case 19: // Math Rock
                        if (fillTypology == 0) { if (trk >= 4) { if (localStep % juce::jmax(1, div / 4) == 0 && random.nextInt(100) < 50) { isAnchor = true; anchorVel = 85; isNegativeAnchor = false; } } if (trk == 0 && localStep == 0) { isAnchor = true; isNegativeAnchor = false; } }
                        else if (fillTypology == 1) { if (localStep == 0) { if (trk == 7) { isAnchor = true; anchorVel = 100; isNegativeAnchor = false; } } else { isNegativeAnchor = true; } }
                        else if (fillTypology == 2) { int k = 7; if ((localStep * k) % juce::jmax(1, div * 2) < k) { if (trk == 5 || trk == 6) { isAnchor = true; anchorVel = 90; isNegativeAnchor = false; } } if (trk == 0 && localStep % div == 0) { isAnchor = true; isNegativeAnchor = false; } }
                        else if (fillTypology == 3) { if (trk == 1 || trk == 4) { if (random.nextInt(100) < 70) { isAnchor = true; anchorVel = 80 + (int)(20.0f * localStep / juce::jmax(1, div * 2)); isNegativeAnchor = false; } } if (trk == 0 && localStep % div == 0) { isAnchor = true; isNegativeAnchor = false; } }
                        break;
                    case 20: // Prog Metal 
                        if (fillTypology == 0) { if (trk >= 4 && trk <= 6) { if (localStep % juce::jmax(1, div / 2) == 0 && random.nextInt(100) < 60) { isAnchor = true; anchorVel = 80 + random.nextInt(20); isNegativeAnchor = false; } } if (trk == 0 && localStep % div == 0) { isAnchor = true; anchorVel = 90; isNegativeAnchor = false; } }
                        else if (fillTypology == 1) { if (localStep >= div * ((num / 2) - 1)) { isNegativeAnchor = true; } else { if (trk == 0 && localStep % juce::jmax(1, div / 2) == 0) { isAnchor = true; anchorVel = 90; isNegativeAnchor = false; } if ((trk == 1 || trk == 7) && localStep == 0) { isAnchor = true; anchorVel = 100; isNegativeAnchor = false; } } }
                        else if (fillTypology == 2) { int k = 7; if ((localStep * k) % juce::jmax(1, div * 2) < k) { if (trk == 4 || trk == 5 || trk == 6) { isAnchor = true; anchorVel = 85; isNegativeAnchor = false; } } if (trk == 0 && localStep % div == 0) { isAnchor = true; isNegativeAnchor = false; } }
                        else if (fillTypology == 3) { int pLen = juce::jmax(1, div); int phase = localStep % pLen; if (phase == 0 && trk == 1) { isAnchor = true; anchorVel = 100; isNegativeAnchor = false; } else if (phase == juce::jmax(1, pLen / 4) && (trk == 4 || trk == 5)) { isAnchor = true; anchorVel = 90; isNegativeAnchor = false; } else if (phase == juce::jmax(1, pLen / 2) && trk == 0) { isAnchor = true; anchorVel = 100; isNegativeAnchor = false; } else if (phase == juce::jmax(1, pLen * 3 / 4) && trk == 0) { isAnchor = true; anchorVel = 90; isNegativeAnchor = false; } else if (random.nextInt(100) < 20 && trk == 6) { isAnchor = true; anchorVel = 85; isNegativeAnchor = false; } }
                        break;
                    case 21: // Minimalism
                        if (fillTypology == 0) { if (trk >= 5 && trk <= 7) { isAnchor = true; anchorVel = 70; isNegativeAnchor = false; } }
                        else if (fillTypology == 1) { if (localStep == 0) { if (trk == 0) { isAnchor = true; anchorVel = 90; isNegativeAnchor = false; } } else { isNegativeAnchor = true; } }
                        else if (fillTypology == 2) { int k = 5; if ((localStep * k) % juce::jmax(1, div * 2) < k) { if (trk == 5 || trk == 6) { isAnchor = true; anchorVel = 80; isNegativeAnchor = false; } } }
                        else if (fillTypology == 3) { if (trk >= 1 && trk <= 4) { if (random.nextInt(100) < 60) { isAnchor = true; anchorVel = 80; isNegativeAnchor = false; } } }
                        break;
                    case 22: // Euclidean Math
                        if (fillTypology == 0) { if (trk % 2 != 0) { if (random.nextInt(100) < 50) { isAnchor = true; anchorVel = 60 + random.nextInt(40); isNegativeAnchor = false; } } }
                        else if (fillTypology == 1) { if (localStep >= div * ((num / 2) - 1)) { isNegativeAnchor = true; } else if (trk == 0 || trk == 1) { if (localStep == 0) { isAnchor = true; anchorVel = 90; isNegativeAnchor = false; } } }
                        else if (fillTypology == 2) { int k = 7 + (trk % 5); if ((localStep * k) % juce::jmax(1, div * 2) < k) { isAnchor = true; anchorVel = 60 + random.nextInt(40); isNegativeAnchor = false; } }
                        else if (fillTypology == 3) { if (trk % 2 == 0) { if (random.nextInt(100) < (40 + localStep * 10)) { isAnchor = true; anchorVel = 50 + (localStep * 2); isNegativeAnchor = false; } } }
                        break;
                    case 23: // Chaos Math
                        if (fillTypology == 0) { if (trk % 2 != 0) { if (random.nextInt(100) < 60) { isAnchor = true; anchorVel = 70 + random.nextInt(30); isNegativeAnchor = false; } } }
                        else if (fillTypology == 1) { if (localStep >= div * ((num / 2) - 1)) { isNegativeAnchor = true; } else if (trk == 0) { if (localStep == 0) { isAnchor = true; anchorVel = 100; isNegativeAnchor = false; } } }
                        else if (fillTypology == 2) { int k = 5 + (trk % 3); if ((localStep * k) % juce::jmax(1, div * 2) < k) { isAnchor = true; anchorVel = 50 + random.nextInt(50); isNegativeAnchor = false; } }
                        else if (fillTypology == 3) { if (trk == 1 || trk == 2 || trk == 4) { if (random.nextInt(100) < (50 + localStep * 5)) { isAnchor = true; anchorVel = 40 + (localStep * 3); isNegativeAnchor = false; } } }
                        break;
                    case 24: // UK Drill
                        if (fillTypology == 0) { if (trk == 2 || trk == 6) { if (localStep % juce::jmax(1, div / 8) == 0 && random.nextInt(100) < 50) { isAnchor = true; anchorVel = 60 + random.nextInt(40); isNegativeAnchor = false; } } }
                        else if (fillTypology == 1) { if (localStep >= div * (num / 2) - juce::jmax(1, div)) { isNegativeAnchor = true; } else if (trk == 0) { if (localStep == 0) { isAnchor = true; anchorVel = 100; isNegativeAnchor = false; } } }
                        else if (fillTypology == 2) { int k = 7; if ((localStep * k) % juce::jmax(1, div * 2) < k) { if (trk == 2 || trk == 5) { isAnchor = true; anchorVel = 85; isNegativeAnchor = false; } } }
                        else if (fillTypology == 3) { if (trk == 2) { isAnchor = true; anchorVel = 100 - (localStep * 3); isNegativeAnchor = false; } if (trk == 0 && localStep == 0) { isAnchor = true; isNegativeAnchor = false; } }
                        break;
                    case 25: // Polyrhythm Matrix
                        if (fillTypology == 0) { if (random.nextInt(100) < 60) { isAnchor = true; anchorVel = 40 + random.nextInt(60); isNegativeAnchor = false; } }
                        else if (fillTypology == 1) { if (localStep == div * (num / 2) - 1) { if (trk == 0) { isAnchor = true; anchorVel = 120; isNegativeAnchor = false; } } else { isNegativeAnchor = true; } }
                        else if (fillTypology == 2) { int k = trk + 2; if ((localStep * k) % juce::jmax(1, div * 2) < k) { isAnchor = true; anchorVel = 80; isNegativeAnchor = false; } }
                        else if (fillTypology == 3) { if (trk == localStep % 8) { isAnchor = true; anchorVel = 90; isNegativeAnchor = false; } }
                        break;
                    }
                }
                else {
                    switch (genre) {
                    case 0: // Techno
                        if (trk == 0) { if (stepInBar % juce::jmax(1, div) == 0) isAnchor = true; else if (currentBarOfStep % 2 != 0 && stepInBar == (div * num) - juce::jmax(1, div / 2) && random.nextInt(100) < 15) { isAnchor = true; anchorVel = 60; } else if (stepInBar % juce::jmax(1, div / 4) == 0) { isSubAnchor = true; subAnchorProb = cmplx * 2; } }
                        else if (trk == 1) { if (stepInBar == div || stepInBar == div * 3) { isAnchor = true; anchorVel = 80; } else if (stepInBar % juce::jmax(1, div) == juce::jmax(1, div * 3 / 4)) { isSubAnchor = true; subAnchorProb = cmplx + 20; } }
                        else if (trk == 2) { if (stepInBar % juce::jmax(1, div) == juce::jmax(1, div / 2)) isAnchor = true; else if (stepInBar % juce::jmax(1, div) == juce::jmax(1, div / 2 + div / 4)) { isSubAnchor = true; subAnchorProb = cmplx * 2; } }
                        else if (trk == 3) { if (stepInBar % juce::jmax(1, div) == juce::jmax(1, div / 2)) { isAnchor = true; anchorVel = 90; } }
                        else if (trk == 4) { if (stepInBar == div || stepInBar == div * 3) { isAnchor = true; anchorVel = 70; } }
                        else if (trk == 5) { if (stepInBar % juce::jmax(1, div * 2) == div + juce::jmax(1, div / 2) && random.nextInt(100) < 30) { isAnchor = true; anchorVel = 60; } else if (stepInBar % juce::jmax(1, div * 2) == juce::jmax(1, div + div / 4)) { isSubAnchor = true; subAnchorProb = cmplx * 2; } }
                        else if (trk == 6) { if (stepInBar % juce::jmax(1, div * 3) == 0 && random.nextInt(100) < 25) { isAnchor = true; anchorVel = 50; } }
                        else if (trk == 7) { if (currentBarOfStep % 4 == 0 && stepInBar == 0) { isAnchor = true; anchorVel = 80; } }
                        break;
                    case 1: // House
                        if (trk == 0) { if (stepInBar % juce::jmax(1, div) == 0) isAnchor = true; else if (stepInBar == div * 4 - juce::jmax(1, div / 4)) { isSubAnchor = true; subAnchorProb = cmplx * 2; } }
                        else if (trk == 1) { if (stepInBar == div || stepInBar == div * 3) { isAnchor = true; anchorVel = 85; } }
                        else if (trk == 2) { if (stepInBar % juce::jmax(1, div) == juce::jmax(1, div / 2)) isAnchor = true; else if (stepInBar % juce::jmax(1, div) == juce::jmax(1, div * 3 / 4)) { isSubAnchor = true; subAnchorProb = cmplx * 2; } }
                        else if (trk == 3) { if (stepInBar % juce::jmax(1, div) == juce::jmax(1, div / 2)) { isAnchor = true; anchorVel = 80; } }
                        else if (trk == 4) { if (stepInBar == div || stepInBar == div * 3) { isAnchor = true; anchorVel = 100; } else if (stepInBar == div + juce::jmax(1, div / 4)) { isSubAnchor = true; subAnchorProb = cmplx * 2; } }
                        else if (trk == 5) { if (stepInBar % juce::jmax(1, div * 2) == div + juce::jmax(1, div / 2) && random.nextInt(100) < 40) { isAnchor = true; anchorVel = 60; } }
                        else if (trk == 6) { if (stepInBar % juce::jmax(1, div * 2) == div * 2 - 1 && random.nextInt(100) < 30) { isAnchor = true; anchorVel = 50; } }
                        else if (trk == 7) { if (stepInBar == div * 2 + juce::jmax(1, div / 2) && random.nextInt(100) < 20) { isAnchor = true; anchorVel = 70; } }
                        break;
                    case 2: // UK Garage
                        if (trk == 0) { if (stepInBar == 0 || stepInBar == div * 2 + juce::jmax(1, div / 2)) isAnchor = true; else if (stepInBar % juce::jmax(1, div * 2) == juce::jmax(1, div + div / 2 + div / 4)) { isSubAnchor = true; subAnchorProb = cmplx * 2; } }
                        else if (trk == 1) { if (stepInBar == div || stepInBar == div * 3) { isAnchor = true; anchorVel = 95; } }
                        else if (trk == 2) { if (stepInBar % juce::jmax(1, div / 2) == 0) isAnchor = true; else if (stepInBar % juce::jmax(1, div) == juce::jmax(1, div / 4) || stepInBar % juce::jmax(1, div) == juce::jmax(1, div * 3 / 4)) { isSubAnchor = true; subAnchorProb = cmplx * 2; } }
                        else if (trk == 3) { if (stepInBar == div * 2 + juce::jmax(1, div / 2)) { isAnchor = true; anchorVel = 70; } }
                        else if (trk == 4) { if (stepInBar == div || stepInBar == div * 3) { isAnchor = true; anchorVel = 80; } }
                        else if (trk == 5) { if (stepInBar % juce::jmax(1, div) == juce::jmax(1, div / 3)) { isSubAnchor = true; subAnchorProb = cmplx * 2; } }
                        else if (trk == 6) { if (stepInBar % juce::jmax(1, div / 4) == 0 && random.nextInt(100) < 25) { isAnchor = true; anchorVel = 40; } }
                        else if (trk == 7) { if (currentBarOfStep % 2 != 0 && stepInBar == div * 3) { isAnchor = true; anchorVel = 80; } }
                        break;
                    case 3: // D&B
                        if (trk == 0) { if (stepInBar == 0 || (stepInBar == div * 2 + juce::jmax(1, div / 2) && random.nextInt(100) < 80)) isAnchor = true; else if (stepInBar == div - juce::jmax(1, div / 4) || stepInBar == div - juce::jmax(1, div / 8)) { isSubAnchor = true; subAnchorProb = cmplx * 2; } }
                        else if (trk == 1) { if (stepInBar == div || stepInBar == div * 3) { isAnchor = true; anchorVel = 100; } else if (stepInBar == div + juce::jmax(1, div / 2 + div / 4)) { isSubAnchor = true; subAnchorProb = cmplx * 2; } }
                        else if (trk == 2) { if (stepInBar % juce::jmax(1, div / 2) == 0) { isAnchor = true; anchorVel = 70; } }
                        else if (trk == 3) { if (stepInBar % juce::jmax(1, div) == juce::jmax(1, div / 2)) { isAnchor = true; anchorVel = 85; } }
                        else if (trk == 4) { if (stepInBar == div || stepInBar == div * 3) { isAnchor = true; anchorVel = 80; } }
                        else if (trk == 5) { if (stepInBar % juce::jmax(1, div / 4) == 0) { isSubAnchor = true; subAnchorProb = cmplx * 2; } }
                        else if (trk == 6) { if (stepInBar % juce::jmax(1, div / 4) == 0) { isAnchor = true; anchorVel = 50; } }
                        else if (trk == 7) { if (stepInBar == 0 && currentBarOfStep == 0) { isAnchor = true; anchorVel = 90; } }
                        break;
                    case 4: // Trap
                        if (trk == 0) { if (stepInBar == 0 || stepInBar == div * 2 + juce::jmax(1, div / 2)) isAnchor = true; else if (stepInBar >= div * 3 && stepInBar % juce::jmax(1, div / 4) == 0) { isSubAnchor = true; subAnchorProb = cmplx * 2; } }
                        else if (trk == 1) { if (stepInBar == div * 2) { isAnchor = true; anchorVel = 100; } }
                        else if (trk == 2) { if (stepInBar % juce::jmax(1, div / 2) == 0) isAnchor = true; else if (stepInBar % juce::jmax(1, div / 8) == 0) { isSubAnchor = true; subAnchorProb = cmplx * 2; } }
                        else if (trk == 3) { if (stepInBar == 0 || stepInBar == div * 2) { isAnchor = true; anchorVel = 80; } }
                        else if (trk == 4) { if (stepInBar == div * 2) { isAnchor = true; anchorVel = 90; } else if (stepInBar % juce::jmax(1, div * 2) == juce::jmax(1, div + div / 2)) { isSubAnchor = true; subAnchorProb = cmplx * 2; } }
                        else if (trk == 5) { if (stepInBar == div + juce::jmax(1, div / 2) && random.nextInt(100) < 30) { isAnchor = true; anchorVel = 60; } }
                        else if (trk == 6) { if (stepInBar == div * 3 + juce::jmax(1, div / 2) && random.nextInt(100) < 30) { isAnchor = true; anchorVel = 60; } }
                        else if (trk == 7) { if (stepInBar == 0) { isAnchor = true; anchorVel = 100; } }
                        break;
                    case 5: // Footwork
                        if (trk == 0) { if (stepInBar % juce::jmax(1, div * 3) == 0 || stepInBar % juce::jmax(1, div * 3) == div) isAnchor = true; else if (stepInBar % juce::jmax(1, (div * 3) / 2) == 0) { isSubAnchor = true; subAnchorProb = cmplx * 2; } }
                        else if (trk == 1) { if (stepInBar == div * 2 || stepInBar == div * 4) isAnchor = true; else if (stepInBar % juce::jmax(1, div * 2) == juce::jmax(1, div + div / 4)) { isSubAnchor = true; subAnchorProb = cmplx * 2; } }
                        else if (trk == 2) { if (stepInBar % juce::jmax(1, div / 2) == 0) { isAnchor = true; anchorVel = 70; } }
                        else if (trk == 3) { if (stepInBar % juce::jmax(1, div * 3) == div * 2 && random.nextInt(100) < 60) { isAnchor = true; anchorVel = 80; } }
                        else if (trk == 4) { if (stepInBar == div * 2 || stepInBar == div * 4) { isAnchor = true; anchorVel = 80; } }
                        else if (trk >= 5 && trk <= 6) { if (stepInBar % juce::jmax(1, div / 4) == 0) { isSubAnchor = true; subAnchorProb = cmplx * 2; } }
                        else if (trk == 7) { if (stepInBar % juce::jmax(1, div * 3) == 0 && random.nextInt(100) < 40) { isAnchor = true; anchorVel = 70; } }
                        break;
                    case 6: // IDM/Breakcore
                        if (trk == 0) { if (stepInBar == 0 || stepInBar == div * 2 + juce::jmax(1, div / 2)) { isAnchor = true; anchorVel = 90; } else if (stepInBar % juce::jmax(1, div / 8) == 0) { isSubAnchor = true; subAnchorProb = cmplx * 2; } }
                        else if (trk == 1) { if (stepInBar == div || stepInBar == div * 3) { isAnchor = true; anchorVel = 100; } }
                        else if (trk == 2) { if (stepInBar % juce::jmax(1, div / 4) == 0 && random.nextInt(100) < 40) { isAnchor = true; anchorVel = 40 + random.nextInt(40); } else if (stepInBar % juce::jmax(1, div / 8) == 0) { isSubAnchor = true; subAnchorProb = cmplx * 2; } }
                        else if (trk == 3) { if (stepInBar % juce::jmax(1, div / 2) == 0 && random.nextInt(100) < 60) { isAnchor = true; anchorVel = 60; } }
                        else if (trk == 4) { if (j % 5 == 0) { isAnchor = true; anchorVel = 70; } }
                        else if (trk == 5) { if (j % 7 == 0) { isAnchor = true; anchorVel = 65; } }
                        else if (trk == 6) { if (stepInBar % juce::jmax(1, div * 3) == 0 && random.nextInt(100) < 50) { isAnchor = true; anchorVel = 60; } }
                        else if (trk == 7) { if (stepInBar == 0 && random.nextInt(100) < 30) { isAnchor = true; anchorVel = 90; } }
                        break;
                    case 7: // Dubstep
                        if (trk == 0) { if (stepInBar == 0) isAnchor = true; else if (stepInBar == div + juce::jmax(1, div / 2)) { isSubAnchor = true; subAnchorProb = cmplx * 2; } }
                        else if (trk == 1) { if (stepInBar == div * 2) { isAnchor = true; anchorVel = 100; } else if (stepInBar == div * 2 - juce::jmax(1, div / 8)) { isSubAnchor = true; subAnchorProb = cmplx * 2; } }
                        else if (trk == 2) { if (stepInBar % juce::jmax(1, div) == 0) isAnchor = true; else if (stepInBar % juce::jmax(1, div / 4) == 0) { isSubAnchor = true; subAnchorProb = cmplx * 2; } }
                        else if (trk == 3) { if (stepInBar == div * 3 + juce::jmax(1, div / 2)) { isAnchor = true; anchorVel = 80; } }
                        else if (trk == 4) { if (stepInBar == div * 2) { isAnchor = true; anchorVel = 90; } }
                        else if (trk == 5) { if (stepInBar == div + juce::jmax(1, div / 2) && random.nextInt(100) < 30) { isAnchor = true; anchorVel = 70; } }
                        else if (trk == 6) { if (stepInBar == div * 3 && random.nextInt(100) < 30) { isAnchor = true; anchorVel = 70; } }
                        else if (trk == 7) { if (stepInBar == 0) { isAnchor = true; anchorVel = 100; } }
                        break;
                    case 8: // Afrobeat
                        if (trk == 0) { if (stepInBar == 0 || stepInBar == div + juce::jmax(1, div / 2) || stepInBar == div * 3) isAnchor = true; else if (stepInBar % juce::jmax(1, div * 2) == juce::jmax(1, div + div / 2)) { isSubAnchor = true; subAnchorProb = cmplx * 2; } }
                        else if (trk == 1) { if (stepInBar == div || stepInBar == div * 2 + juce::jmax(1, div / 2) || stepInBar == div * 3 + juce::jmax(1, div / 2)) isAnchor = true; }
                        else if (trk == 2) { if (stepInBar % juce::jmax(1, div / 2) == 0) { isAnchor = true; anchorVel = 75; } }
                        else if (trk == 3) { if (stepInBar % juce::jmax(1, div) == 0 && random.nextInt(100) < 40) { isAnchor = true; anchorVel = 60; } }
                        else if (trk == 4) { if (stepInBar == div || stepInBar == div * 3) { isAnchor = true; anchorVel = 70; } else if (stepInBar % juce::jmax(1, div) == juce::jmax(1, div / 4)) { isSubAnchor = true; subAnchorProb = cmplx * 2; } }
                        else if (trk >= 5 && trk <= 6) { if (stepInBar % juce::jmax(1, div) == juce::jmax(1, div / 4)) { isSubAnchor = true; subAnchorProb = cmplx * 2; } }
                        else if (trk == 7) { if (stepInBar == 0 && random.nextInt(100) < 70) { isAnchor = true; anchorVel = 80; } }
                        break;
                    case 9: // Gqom
                        if (trk == 0) { if (stepInBar == 0 || stepInBar == div + juce::jmax(1, div / 4) || stepInBar == div * 2 + juce::jmax(1, div / 2) || stepInBar == div * 3 + juce::jmax(1, div / 2)) isAnchor = true; else if (stepInBar % juce::jmax(1, div * 3 / 2) == 0) { isSubAnchor = true; subAnchorProb = cmplx * 2; } }
                        else if (trk == 1) { if (stepInBar == div * 3 + juce::jmax(1, div / 2) && random.nextInt(100) < 60) isAnchor = true; }
                        else if (trk == 2) { if (stepInBar % juce::jmax(1, div) == juce::jmax(1, div / 2)) { isAnchor = true; anchorVel = 70; } else if (stepInBar % juce::jmax(1, div * 2) == juce::jmax(1, div / 2)) { isSubAnchor = true; subAnchorProb = cmplx * 2; } }
                        else if (trk == 3) { if (stepInBar % juce::jmax(1, div * 2) == 0 && random.nextInt(100) < 50) { isAnchor = true; anchorVel = 60; } }
                        else if (trk == 4) { if (stepInBar == div * 3 + juce::jmax(1, div / 2) && random.nextInt(100) < 40) { isAnchor = true; anchorVel = 70; } }
                        else if (trk >= 5 && trk <= 6) { if (stepInBar % juce::jmax(1, div * 2) == juce::jmax(1, div / 2) && random.nextInt(100) < 40) { isAnchor = true; anchorVel = 65; } }
                        else if (trk == 7) { if (stepInBar == 0 && currentBarOfStep % 2 != 0) { isAnchor = true; anchorVel = 90; } }
                        break;
                    case 10: // Amapiano
                        if (trk == 0) { if (stepInBar % juce::jmax(1, div) == 0) isAnchor = true; else if (stepInBar == div * 2 + juce::jmax(1, div / 2)) { isSubAnchor = true; subAnchorProb = cmplx * 2; } }
                        else if (trk == 1) { if (stepInBar == div * 3 && random.nextInt(100) < 50) { isAnchor = true; anchorVel = 80; } }
                        else if (trk == 2) { if (stepInBar % juce::jmax(1, div) == juce::jmax(1, div / 2)) { isAnchor = true; anchorVel = 70; } }
                        else if (trk == 3) { if (stepInBar % juce::jmax(1, div * 2) == 0 && random.nextInt(100) < 30) { isAnchor = true; anchorVel = 60; } }
                        else if (trk == 4) { if (stepInBar == div * 3) { isAnchor = true; anchorVel = 70; } }
                        else if (trk == 5) { if (stepInBar == div + juce::jmax(1, div / 2)) { isAnchor = true; anchorVel = 80; } }
                        else if (trk >= 6) { if (stepInBar == div * 2 + juce::jmax(1, div / 2) || stepInBar == div * 3 + juce::jmax(1, div / 2)) { isAnchor = true; anchorVel = 100; } else if (stepInBar % juce::jmax(1, div) == juce::jmax(1, div / 2 + div / 4)) { isSubAnchor = true; subAnchorProb = cmplx * 2; } }
                        break;
                    case 11: // Indian Classical
                        if (trk == 0) { if (stepInBar == 0 || stepInBar == div * 2) isAnchor = true; else if (stepInBar % juce::jmax(1, div * 2) == juce::jmax(1, div + div / 2)) { isSubAnchor = true; subAnchorProb = cmplx * 2; } }
                        else if (trk >= 1 && trk <= 3) { if (stepInBar % juce::jmax(1, div) == juce::jmax(1, div / 2) && random.nextInt(100) < 60) { isAnchor = true; anchorVel = 70; } }
                        else if (trk >= 4 && trk <= 6) { if (stepInBar % juce::jmax(1, div) == juce::jmax(1, div / 4) && random.nextInt(100) < 30) { isAnchor = true; anchorVel = 50; } else if (stepInBar % juce::jmax(1, div / 8) == 0) { isSubAnchor = true; subAnchorProb = cmplx * 2; } }
                        else if (trk == 7) { if (stepInBar == 0 && currentBarOfStep == 0) { isAnchor = true; anchorVel = 90; } }
                        break;
                    case 12: // Samba/Bossa
                        if (trk == 0) { if (stepInBar == 0) { isAnchor = true; anchorVel = 70; } else if (stepInBar == div * 2) { isAnchor = true; anchorVel = 100; } else if (stepInBar % juce::jmax(1, div * 2) == juce::jmax(1, div * 2 - div / 4)) { isSubAnchor = true; subAnchorProb = cmplx * 2; } }
                        else if (trk == 1) { if (stepInBar == div * 4 - 1) { isAnchor = true; anchorVel = 100; } else if (stepInBar == div + juce::jmax(1, div / 2) || stepInBar == div * 2 + juce::jmax(1, div / 2)) { isAnchor = true; anchorVel = 85; } }
                        else if (trk == 2) { if (stepInBar == 0 || stepInBar == div * 1 + juce::jmax(1, div / 2) || stepInBar == div * 2 + juce::jmax(1, div / 2)) { isAnchor = true; anchorVel = 85; } else if (stepInBar % juce::jmax(1, div) == juce::jmax(1, div / 2)) { isSubAnchor = true; subAnchorProb = cmplx * 2; } }
                        else if (trk == 3) { if (stepInBar % juce::jmax(1, div / 2) == 0) { isAnchor = true; anchorVel = 60 + ((stepInBar / juce::jmax(1, div / 2)) % 2) * 30; } }
                        else if (trk == 4) { if (stepInBar % juce::jmax(1, div) == 0 && random.nextInt(100) < 50) { isAnchor = true; anchorVel = 65; } }
                        else if (trk == 5) { if (stepInBar % juce::jmax(1, div) != 0 && random.nextInt(100) < 30) { isAnchor = true; anchorVel = 70; } }
                        else if (trk == 6) { if (stepInBar == div * 3 + juce::jmax(1, div / 2)) { isAnchor = true; anchorVel = 75; } }
                        else if (trk == 7) { if (stepInBar == 0 && currentBarOfStep % 2 != 0) { isAnchor = true; anchorVel = 80; } }
                        break;
                    case 13: // Reggaeton
                        if (trk == 0) { if (stepInBar % juce::jmax(1, div) == 0) isAnchor = true; else if (stepInBar == div * 4 - juce::jmax(1, div / 8)) { isSubAnchor = true; subAnchorProb = cmplx * 2; } }
                        else if (trk == 1) { if (stepInBar == div - juce::jmax(1, div / 4) || stepInBar == div * 2 + juce::jmax(1, div / 2)) isAnchor = true; }
                        else if (trk == 2) { if (stepInBar % juce::jmax(1, div / 2) == 0) { isAnchor = true; anchorVel = 60; } }
                        else if (trk == 3) { if (stepInBar % juce::jmax(1, div) == 0 && random.nextInt(100) < 30) { isAnchor = true; anchorVel = 50; } }
                        else if (trk == 4) { if (stepInBar == div - juce::jmax(1, div / 4) || stepInBar == div * 2 + juce::jmax(1, div / 2)) { isAnchor = true; anchorVel = 70; } else if (stepInBar % juce::jmax(1, div) == juce::jmax(1, div * 3 / 4)) { isSubAnchor = true; subAnchorProb = cmplx * 2; } }
                        else if (trk == 5 || trk == 6) { if (stepInBar == div * 3 + juce::jmax(1, div / 2) && random.nextInt(100) < 40) { isAnchor = true; anchorVel = 60; } }
                        else if (trk == 7) { if (stepInBar == 0) { isAnchor = true; anchorVel = 90; } }
                        break;
                    case 14: // Gamelan
                        if (trk == 0) { if (currentBarOfStep % 2 == 0 && stepInBar == 0) { isAnchor = true; anchorVel = 100; } else { isNegativeAnchor = true; } }
                        else if (trk >= 1 && trk <= 4) { if (stepInBar % juce::jmax(1, div * 2) == 0) { isAnchor = true; anchorVel = 70; } }
                        else if (trk >= 5 && trk <= 6) { if (stepInBar % juce::jmax(1, div) == juce::jmax(1, div / 2) && random.nextInt(100) < 50) { isAnchor = true; anchorVel = 60; } else if (stepInBar % juce::jmax(1, div * 2 - div / 4) == 0) { isSubAnchor = true; subAnchorProb = cmplx * 2; } }
                        else if (trk == 7) { if (stepInBar % juce::jmax(1, div * 4) == 0 && random.nextInt(100) < 40) { isAnchor = true; anchorVel = 80; } }
                        break;
                    case 15: // Funk
                        if (trk == 0) { if (stepInBar == 0) { isAnchor = true; anchorVel = 110; } else if (stepInBar == div * 2 + juce::jmax(1, div / 2)) { isAnchor = true; anchorVel = 80 - (entrp / 2); } else if (stepInBar % juce::jmax(1, div) == juce::jmax(1, div * 3 / 4)) { isSubAnchor = true; subAnchorProb = cmplx * 2; } }
                        else if (trk == 1) { if (stepInBar == div || stepInBar == div * 3) { isAnchor = true; anchorVel = 100; } }
                        else if (trk == 2) { if (stepInBar % juce::jmax(1, div / 2) == 0) { isAnchor = true; anchorVel = (stepInBar % div == 0) ? 80 : 50; } }
                        else if (trk == 3) { int p = stepInBar % juce::jmax(1, div * 4); if (p == juce::jmax(1, div / 4) || p == juce::jmax(1, div - div / 4) || p == juce::jmax(1, div * 2 - div / 4) || p == juce::jmax(1, div * 2 + div / 2) || p == juce::jmax(1, div * 4 - div / 4)) { if (random.nextInt(100) < 40 + cmplx / 2) { isAnchor = true; anchorVel = 30 + random.nextInt(20); } } else if (stepInBar % juce::jmax(1, div) == juce::jmax(1, div / 2 + div / 4)) { isSubAnchor = true; subAnchorProb = cmplx * 2; } }
                        else if (trk == 4) { if (stepInBar == div * 2 - juce::jmax(1, div / 4) && random.nextInt(100) < 50) { isAnchor = true; anchorVel = 60; } }
                        else if (trk == 5) { if (stepInBar == div * 3 + juce::jmax(1, div / 2) && random.nextInt(100) < 30) { isAnchor = true; anchorVel = 70; } }
                        else if (trk == 6) { if (stepInBar % juce::jmax(1, div) == juce::jmax(1, div / 4) && random.nextInt(100) < 20) { isAnchor = true; anchorVel = 65; } }
                        else if (trk == 7) { if (stepInBar == 0 && currentBarOfStep == 0) { isAnchor = true; anchorVel = 90; } }
                        break;
                    case 16: // New Jack Swing
                        if (trk == 0) { if (stepInBar == 0 || stepInBar == div * 2 + juce::jmax(1, div / 2)) isAnchor = true; else if (stepInBar % juce::jmax(1, div) == juce::jmax(1, div / 2 + div / 4)) { isSubAnchor = true; subAnchorProb = cmplx * 2; } }
                        else if (trk == 1) { if (stepInBar == div || stepInBar == div * 3) isAnchor = true; else if (stepInBar == div + juce::jmax(1, div / 2 + div / 4)) { isSubAnchor = true; subAnchorProb = cmplx * 2; } }
                        else if (trk == 2) { if (stepInBar % juce::jmax(1, div) == 0) isAnchor = true; else if (stepInBar % juce::jmax(1, div) == div - 1 && random.nextInt(100) < 80) { isAnchor = true; anchorVel = 60; } }
                        else if (trk == 3) { if (stepInBar % juce::jmax(1, div) == juce::jmax(1, div / 2) && random.nextInt(100) < 50) { isAnchor = true; anchorVel = 70; } }
                        else if (trk == 4) { if (stepInBar == div || stepInBar == div * 3) { isAnchor = true; anchorVel = 90; } }
                        else if (trk == 5 || trk == 6) { if (stepInBar == div * 2 - 1 && random.nextInt(100) < 40) { isAnchor = true; anchorVel = 50; } }
                        else if (trk == 7) { if (stepInBar == 0) { isAnchor = true; anchorVel = 95; } }
                        break;
                    case 17: // Neo Soul
                        if (trk == 0) { if (stepInBar == 0 || (stepInBar == div * 2 + juce::jmax(1, div / 2) && random.nextInt(100) < 60)) isAnchor = true; else if (stepInBar % juce::jmax(1, div) == juce::jmax(1, div - div / 8)) { isSubAnchor = true; subAnchorProb = cmplx * 2; } }
                        else if (trk == 1) { if (stepInBar == div || stepInBar == div * 3) isAnchor = true; }
                        else if (trk == 2) { if (stepInBar % juce::jmax(1, div * 2) == div && random.nextInt(100) < 80) { isAnchor = true; anchorVel = 60; } else if (stepInBar % juce::jmax(1, div) == juce::jmax(1, div / 2 + div / 8)) { isSubAnchor = true; subAnchorProb = cmplx * 2; } }
                        else if (trk == 3) { if (stepInBar % juce::jmax(1, div) == 0 && random.nextInt(100) < 30) { isAnchor = true; anchorVel = 50; } }
                        else if (trk == 4) { if (stepInBar == div || stepInBar == div * 3) { isAnchor = true; anchorVel = 70; } }
                        else if (trk == 5 || trk == 6) { if (stepInBar == div * 3 + juce::jmax(1, div / 2) && random.nextInt(100) < 20) { isAnchor = true; anchorVel = 40; } }
                        else if (trk == 7) { if (stepInBar == 0 && currentBarOfStep % 2 == 0) { isAnchor = true; anchorVel = 80; } }
                        break;
                    case 18: // Hip Hop
                        if (trk == 0) { if (stepInBar == 0 || stepInBar == div * 2 + juce::jmax(1, div / 2)) isAnchor = true; else if (stepInBar % juce::jmax(1, div * 2) == juce::jmax(1, div + div / 2 + div / 8)) { isSubAnchor = true; subAnchorProb = cmplx * 2; } }
                        else if (trk == 1) { if (stepInBar == div || stepInBar == div * 3) isAnchor = true; else if (stepInBar % juce::jmax(1, div * 2) == juce::jmax(1, div * 2 - div / 4)) { isSubAnchor = true; subAnchorProb = cmplx * 2; } }
                        else if (trk == 2) { if (stepInBar % juce::jmax(1, div) == 0) isAnchor = true; else if (stepInBar % juce::jmax(1, div) == juce::jmax(1, div / 2) && random.nextInt(100) < 30) { isAnchor = true; anchorVel = 40; } }
                        else if (trk == 3) { if (stepInBar % juce::jmax(1, div) == juce::jmax(1, div / 4) && random.nextInt(100) < 20) { isAnchor = true; anchorVel = 50; } }
                        else if (trk == 4) { if (stepInBar == div || stepInBar == div * 3) { isAnchor = true; anchorVel = 80; } }
                        else if (trk == 5 || trk == 6) { if (stepInBar == div * 2 - 1 && random.nextInt(100) < 20) { isAnchor = true; anchorVel = 45; } }
                        else if (trk == 7) { if (stepInBar == 0) { isAnchor = true; anchorVel = 90; } }
                        break;
                    case 19: // Math Rock
                        if (trk == 0) { if (stepInBar == 0 || stepInBar == div * 3) isAnchor = true; else if (stepInBar % juce::jmax(1, div * 3 + div / 2) == 0) { isSubAnchor = true; subAnchorProb = cmplx * 2; } }
                        else if (trk == 1) { if (stepInBar == div * 2 || stepInBar == div * 4) isAnchor = true; else if (stepInBar % juce::jmax(1, div * 2) == juce::jmax(1, div + div / 4)) { isSubAnchor = true; subAnchorProb = cmplx * 2; } }
                        else if (trk == 2) { if (stepInBar % juce::jmax(1, div) == 0) isAnchor = true; else if (random.nextInt(100) < 20) { isAnchor = true; anchorVel = 50; } }
                        else if (trk == 3) { if (stepInBar % juce::jmax(1, div) == juce::jmax(1, div / 2) && random.nextInt(100) < 30) { isAnchor = true; anchorVel = 60; } }
                        else if (trk == 4) { if (stepInBar == div * 2 || stepInBar == div * 4) { isAnchor = true; anchorVel = 80; } }
                        else if (trk == 5 || trk == 6) { if (stepInBar == div + juce::jmax(1, div / 2) || stepInBar == div * 4 + juce::jmax(1, div / 2)) { isAnchor = true; anchorVel = 75; } }
                        else if (trk == 7) { if (stepInBar == 0 && currentBarOfStep % 2 == 0) { isAnchor = true; anchorVel = 90; } }
                        break;
                    case 20: // Prog Metal 
                        if (trk == 0) {
                            if (stepInBar % juce::jmax(1, div) == 0) isAnchor = true;
                            else if (stepInBar % juce::jmax(1, div) == juce::jmax(1, div - div / 4) && random.nextInt(100) < 60) { isAnchor = true; anchorVel = 80; }
                            else if (stepInBar % juce::jmax(1, div / 4) == 0) { isSubAnchor = true; subAnchorProb = cmplx * 2; }
                        }
                        else if (trk == 1) { if (stepInBar == div || stepInBar == div * 3) { isAnchor = true; anchorVel = 100; } }
                        else if (trk == 2) { if (stepInBar % juce::jmax(1, div / 2) == 0) { isAnchor = true; anchorVel = 80; } }
                        else if (trk == 3) { if (stepInBar % juce::jmax(1, div) == juce::jmax(1, div / 2) && random.nextInt(100) < 70) { isAnchor = true; anchorVel = 85; } }
                        else if (trk == 4 || trk == 5 || trk == 6) { if (stepInBar % juce::jmax(1, div * 3) == 0) { isSubAnchor = true; subAnchorProb = cmplx * 2; } }
                        else if (trk == 7) { if (stepInBar == 0) { isAnchor = true; anchorVel = 100; } }
                        break;
                    case 21: // Minimalism
                        if (trk == 0) { if (stepInBar % juce::jmax(1, div * 3) == 0) isAnchor = true; else if (stepInBar % juce::jmax(1, div + div / 4) == 0) { isSubAnchor = true; subAnchorProb = cmplx * 2; } }
                        else if (trk == 1) { if ((stepInBar + div) % juce::jmax(1, div * 3) == 0) isAnchor = true; else if (stepInBar % juce::jmax(1, div + div / 4) == 0) { isSubAnchor = true; subAnchorProb = cmplx * 2; } }
                        else if (trk >= 2 && trk <= 4) { if (stepInBar % 2 == 0) isAnchor = true; }
                        else if (trk >= 5 && trk <= 6) { if (stepInBar % 3 == 0) { isAnchor = true; anchorVel = 70; } }
                        else if (trk == 7) { if (stepInBar % 5 == 0 && random.nextInt(100) < 40) { isAnchor = true; anchorVel = 60; } }
                        break;
                    case 22: // Euclidean Math
                        if (trk == 0) { if (stepInBar % juce::jmax(1, div) == 0 && random.nextInt(100) < 80) isAnchor = true; else if (stepInBar % juce::jmax(1, div / 2) == 0) { isSubAnchor = true; subAnchorProb = cmplx * 2; } else isNegativeAnchor = true; }
                        else if (trk == 1) { if (stepInBar == div || stepInBar == div * 3) isAnchor = true; else isNegativeAnchor = true; }
                        else {
                            int k = 3 + (trk % 4);
                            if (((static_cast<int64_t>(stepInBar) + trk) * k) % juce::jmax(1, div * num) < k) {
                                isAnchor = true; anchorVel = 70 + random.nextInt(20);
                            }
                            else {
                                isNegativeAnchor = true;
                            }
                        }
                        break;
                    case 23: // Chaos Math
                        if (trk == 0) { if (stepInBar == 0 || (stepInBar == div * 2 && random.nextInt(100) < 50)) isAnchor = true; else if (stepInBar % juce::jmax(1, div / 4) == 0) { isSubAnchor = true; subAnchorProb = cmplx * 2; } else isNegativeAnchor = true; }
                        else if (trk == 1) { if (stepInBar == div || stepInBar == div * 3) { isAnchor = true; anchorVel = 90; } else isNegativeAnchor = true; }
                        else {
                            int prob = 10 + (trk * 5);
                            if (stepInBar % juce::jmax(1, div / 2) == 0) prob += 30;
                            if (random.nextInt(100) < prob) {
                                isAnchor = true; anchorVel = 40 + random.nextInt(60);
                            }
                            else {
                                isNegativeAnchor = true;
                            }
                        }
                        break;
                        // ★ UK Drill (ハーフタイムスネア、疎なキック配置への最適化)
                    case 24:
                        if (trk == 0) {
                            if (stepInBar == 0 || stepInBar == div * 2 + div / 2) { isAnchor = true; anchorVel = 100; }
                            else if (stepInBar == div * 3 - juce::jmax(1, div / 4)) { isSubAnchor = true; subAnchorProb = cmplx * 2; }
                        }
                        else if (trk == 1 || trk == 4) {
                            if (stepInBar == div * 2 + juce::jmax(1, div / 2)) { isAnchor = true; anchorVel = 100; }
                        }
                        else if (trk == 2) {
                            if (stepInBar % juce::jmax(1, div / 2) == 0) { isAnchor = true; anchorVel = 70; }
                            else if (stepInBar % juce::jmax(1, div / 4) == 0) { isSubAnchor = true; subAnchorProb = cmplx * 2; }
                        }
                        else if (trk == 3) {
                            if (stepInBar == div * 2) { isAnchor = true; anchorVel = 80; }
                        }
                        else if (trk == 5 || trk == 6) {
                            if (stepInBar % juce::jmax(1, div * 2) == div + div / 2 && random.nextInt(100) < 40) { isAnchor = true; anchorVel = 60; }
                        }
                        else if (trk == 7) {
                            if (stepInBar == 0 || stepInBar == div * 2 + juce::jmax(1, div / 2) + juce::jmax(1, div / 4)) { isAnchor = true; anchorVel = 100; }
                        }
                        break;
                        // ★ Polyrhythm Matrix (オリジナルコードと同じ div*4, div*2 による美しい分散配置の復元)
                    case 25:
                        if (trk == 0) {
                            if (stepInBar % juce::jmax(1, div) == 0) { isAnchor = true; anchorVel = 100; }
                            else if (stepInBar % juce::jmax(1, div / 2) == 0) { isSubAnchor = true; subAnchorProb = cmplx * 15; }
                        }
                        else {
                            // ★ 空間のサイズを定義
                            int base_n = juce::jmax(1, div * 4);

                            // ★ 打撃数(k)が絶対に空間(base_n)をオーバーしないように制限（最大でも密度の半分程度）
                            int k = juce::jlimit(1, base_n - 1, trk + 1);

                            if (((static_cast<int64_t>(stepInBar) * k) % base_n) < k) {
                                isAnchor = true; anchorVel = 60 + random.nextInt(30);
                            }
                            else if (cmplx > 0 && (((static_cast<int64_t>(stepInBar) * k) % juce::jmax(1, div * 2)) < k)) {
                                isSubAnchor = true; subAnchorProb = cmplx * 15;
                            }
                            else {
                                isNegativeAnchor = true;
                            }
                        }
                        break;
                    }
                }

                // ★ 一般ステップの配置 (ユーザー様のオリジナルコードを完全に維持し復元)
                int k = juce::jmax(1, (n * cmplx) / 100);
                int vel = 0;

                if (isAnchor) {
                    int velJitter = (int)((entrp / 100.0f) * 20.0f);
                    vel = anchorVel - random.nextInt(juce::jmax(1, velJitter + 1));
                    if (trackDynamic[trk]) vel = juce::jlimit(80, 127, vel + trackDynamicAmount[trk]);
                }
                else if (isNegativeAnchor) {
                    vel = 0;
                }
                else if (cmplx > 0) {
                    bool isHit = false;
                    if (isSubAnchor) {
                        if (random.nextInt(100) < subAnchorProb) {
                            isHit = true;
                            vel = random.nextInt(juce::Range<int>(50, 90 + (entrp / 10)));
                        }
                    }
                    else {
                        if (genre == 23) {
                            isHit = (random.nextInt(100) < cmplx);
                        }
                        else {
                            isHit = (((static_cast<int64_t>(j) + offset) * k) % juce::jmax(1, n)) < k;
                            if (isHit && random.nextInt(100) > cmplx + 20) isHit = false;
                        }
                        if (isHit) {
                            vel = random.nextInt(juce::Range<int>(40, 80 + (entrp / 10)));
                        }
                    }

                    if (isHit && entrp > 0 && random.nextInt(100) < (entrp / 3)) {
                        isHit = false;
                        vel = 0;
                    }

                    if (isHit && trackDynamic[trk] && stepInBar % juce::jmax(1, div) == div - 1) {
                        vel = juce::jlimit(80, 127, vel + trackDynamicAmount[trk]);
                    }
                }
                drumPatternUI[trk][j] = vel;
            }
        }
    }

    patternUpdated.store(true);
    uiNeedsUpdate.store(true);
}
bool AIDrumMachineAudioProcessor::loadSample(int trackIndex, const juce::String& filePath) {
    if (trackIndex < 0 || trackIndex >= 8) return false;
    juce::File file(filePath);

    // =========================================================================
    // ★ Ableton特有の「ファイルロック問題」を完全回避する最強の対策
    // 直接ファイルを開かず、一度メモリに丸ごとコピーしてからJUCEに渡します。
    // =========================================================================
    juce::MemoryBlock memoryBlock;
    if (!file.loadFileAsData(memoryBlock)) return false; // 読み込み失敗時は抜ける

    // メモリ上のデータからInputStreamを作成
    auto memoryInputStream = std::make_unique<juce::MemoryInputStream>(memoryBlock, true);

    if (auto* reader = formatManager.createReaderFor(std::move(memoryInputStream))) {
        int length = (int)reader->lengthInSamples;
        int channels = reader->numChannels;

        // 空ファイルや異常なデータをブロック
        if (length <= 0 || channels <= 0) {
            delete reader;
            return false;
        }

        juce::AudioSampleBuffer tempBuffer(channels, length);
        bool success = reader->read(&tempBuffer, 0, length, 0, true, true);
        delete reader;

        if (!success) return false;

        juce::ScopedLock sl(sampleLock);
        sampleBuffers[trackIndex] = tempBuffer;
        hasSample[trackIndex] = true;
        samplePlayPos[trackIndex] = -1;
        // ★ これを追加！パスを記憶する
        loadedSamplePaths[trackIndex] = filePath;

        return true; // 成功！
    }
    return false;
}

void AIDrumMachineAudioProcessor::clearSample(int trackIndex) {
    if (trackIndex < 0 || trackIndex >= 8) return;
    juce::ScopedLock sl(sampleLock);
    hasSample[trackIndex] = false;
    samplePlayPos[trackIndex] = -1;
    // ★ これを追加！パスを忘れさせる
    loadedSamplePaths[trackIndex] = "";
}

void AIDrumMachineAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {
    resetPosition();
    for (int i = 0; i < 8; ++i) {
        synthVoices[i].setSampleRate((float)sampleRate);
        samplePlayPos[i] = -1;

        // ★ 追加：MIDI状態の初期化
        activeNote[i] = -1;
        noteOffCountdown[i] = 0;
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

    // ★ DAWから入力されたMIDIをクリア（このプラグイン専用のMIDI出力を妨げないため）
    midiMessages.clear();

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

    if (bpm < 20.0) bpm = 20.0;
    if (bpm > 999.0) bpm = 999.0;
    currentBpm.store(bpm);

    double samplesPerQuarterNote = sampleRate * (60.0 / bpm);
    double samplesPerBeat = samplesPerQuarterNote * (4.0 / (double)timeSigDenDSP);
    int samplesPerBar = (int)(samplesPerBeat * timeSigNumDSP);
    int samplesPerLoop = samplesPerBar * globalBarCountDSP;

    if (samplesPerBar > 0) {
        currentPlayingBar.store((samplesInLoop / samplesPerBar) % juce::jmax(1, globalBarCountDSP));
    }

    bool anySolo = false;
    for (int i = 0; i < 8; ++i) { if (trackSoloed[i]) { anySolo = true; break; } }

    const auto& def = getGenreDef(currentGenre.load());
    bool isArp = arpMode.load();
    int curScale = arpScale.load();

    // ★ Drumモード時のデフォルトMIDIノート（Kick=36, Snare=38 など）
    int defaultNotes[8] = { 36, 38, 42, 46, 39, 41, 45, 50 };

    bool isSafeToProcessSamples = sampleLock.tryEnter();

    for (int i = 0; i < numSamples; ++i)
    {
        // ★ DAWが停止した瞬間に、鳴りっぱなしのMIDIノートをすべて強制オフ
        if (!isPlaying) {
            for (int trk = 0; trk < 8; ++trk) {
                if (activeNote[trk] != -1) {
                    midiMessages.addEvent(juce::MidiMessage::noteOff(1, activeNote[trk]), i);
                    activeNote[trk] = -1;
                    noteOffCountdown[trk] = 0;
                }
            }
        }

        if (isPlaying && samplesPerLoop > 0) {
            samplesInLoop++;
            if (samplesInLoop >= samplesPerLoop) samplesInLoop = 0;

            for (int trk = 0; trk < 8; ++trk)
            {
                int div = trackDivisionsDSP[trk]; if (div < 1) div = 1;
                int totalStepsInLoop = div * timeSigNumDSP * globalBarCountDSP;
                if (totalStepsInLoop <= 0) continue;

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
                        // --- 1. オーディオの発音処理 ---
                        if (hasSample[trk]) {
                            if (isSafeToProcessSamples) {
                                samplePlayPos[trk] = 0;
                                sampleVolume[trk] = velocity / 100.0f;
                            }
                        }
                        else {
                            InstrumentPatch p = getPatch(isArp ? M_ARP : def.trackPatches[trk]);
                            if (isArp) {
                                int note = 60 + arpKey.load() + (trackOctaveDSP[trk] * 12) + scalePatterns[curScale][trackDegreeDSP[trk]];
                                p.freq = 440.0f * std::pow(2.0f, (note - 69) / 12.0f);
                            }
                            synthVoices[trk].trigger((float)velocity, p);
                        }

                        // --- 2. ★ DAWへのMIDI出力処理 ---
                        int noteNum = defaultNotes[trk]; // デフォルトはDrumキー
                        if (isArp) {
                            int offset = scalePatterns[curScale][trackDegreeDSP[trk]];
                            if (offset != -1) {
                                noteNum = 60 + arpKey.load() + (trackOctaveDSP[trk] * 12) + offset;
                            }
                        }
                        noteNum = juce::jlimit(0, 127, noteNum); // 念のため0〜127の範囲に収める

                        // もし前の音が鳴りっぱなしなら、一度消す
                        if (activeNote[trk] != -1) {
                            midiMessages.addEvent(juce::MidiMessage::noteOff(1, activeNote[trk]), i);
                        }

                        // 新しいノートをオン送信 (MIDIチャンネル1)
                        midiMessages.addEvent(juce::MidiMessage::noteOn(1, noteNum, (juce::uint8)velocity), i);
                        activeNote[trk] = noteNum;
                        noteOffCountdown[trk] = (int)(samplesPerStep * 0.8); // 長さはステップの80%
                    }
                }

                // --- 3. ★ MIDIノートオフのタイマー処理 ---
                if (noteOffCountdown[trk] > 0) {
                    noteOffCountdown[trk]--;
                    if (noteOffCountdown[trk] == 0 && activeNote[trk] != -1) {
                        midiMessages.addEvent(juce::MidiMessage::noteOff(1, activeNote[trk]), i);
                        activeNote[trk] = -1;
                    }
                }
            }
        }

        float mixOut = 0.0f;
        for (int trk = 0; trk < 8; ++trk)
        {
            float osc = 0.0f;
            if (hasSample[trk]) {
                if (isSafeToProcessSamples && samplePlayPos[trk] >= 0 && samplePlayPos[trk] < sampleBuffers[trk].getNumSamples()) {
                    float env = 1.0f;
                    int remaining = sampleBuffers[trk].getNumSamples() - samplePlayPos[trk];
                    if (remaining < 100) env = remaining / 100.0f;

                    osc = sampleBuffers[trk].getSample(0, samplePlayPos[trk]) * sampleVolume[trk] * env;
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

    if (isSafeToProcessSamples) {
        sampleLock.exit();
    }
}

const juce::String AIDrumMachineAudioProcessor::getName() const { return JucePlugin_Name; }
bool AIDrumMachineAudioProcessor::acceptsMidi() const { return false; }
bool AIDrumMachineAudioProcessor::producesMidi() const { return true; }
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
    xml.setAttribute("timeSigNum", timeSigNumerator.load());
    xml.setAttribute("timeSigDen", timeSigDenominator.load());
    xml.setAttribute("fillBarTarget", fillBarTarget.load());
    xml.setAttribute("arpMode", arpMode.load());
    xml.setAttribute("arpMono", arpMono.load());
    xml.setAttribute("arpKey", arpKey.load());
    xml.setAttribute("arpScale", arpScale.load());
    xml.setAttribute("autoFollow", autoFollowEnabled.load());
    xml.setAttribute("tsLock", timeSigLocked.load());

    // 各トラックのパラメータとシーケンスを保存
    for (int i = 0; i < 8; ++i) {
        auto* trackXml = new juce::XmlElement("TrackData");
        trackXml->setAttribute("id", i);
        trackXml->setAttribute("div", trackDivisionsUI[i]);
        trackXml->setAttribute("shift", trackShiftUI[i]);
        trackXml->setAttribute("cmplx", trackComplexity[i]);
        trackXml->setAttribute("entrp", trackEntropy[i]);
        trackXml->setAttribute("oct", trackOctaveUI[i]);
        trackXml->setAttribute("deg", trackDegreeUI[i]);

        trackXml->setAttribute("lock", trackLocked[i]);
        trackXml->setAttribute("divLck", trackDivLocked[i]);
        trackXml->setAttribute("cmplxLck", trackCmplxLocked[i]);
        trackXml->setAttribute("entrpLck", trackEntrpLocked[i]);
        trackXml->setAttribute("shiftLck", trackShiftLocked[i]);
        trackXml->setAttribute("degLck", trackDegreeLocked[i]);
        trackXml->setAttribute("octLck", trackOctaveLocked[i]);

        trackXml->setAttribute("mute", trackMuted[i]);
        trackXml->setAttribute("solo", trackSoloed[i]);
        trackXml->setAttribute("dyn", trackDynamic[i]);
        trackXml->setAttribute("dynAmt", trackDynamicAmount[i]);

        // サンプルパスの保存
        trackXml->setAttribute("samplePath", loadedSamplePaths[i]);

        // シーケンスパターンをカンマ区切りの文字列で保存
        juce::String patStr;
        for (int s = 0; s < 1024; ++s) patStr << drumPatternUI[i][s] << ",";
        trackXml->setAttribute("pattern", patStr);

        xml.addChildElement(trackXml);
    }

    // Pat1〜4（保存されたパターン）の保存
    for (int p = 0; p < 4; ++p) {
        auto* patXml = new juce::XmlElement("SavedPattern");
        patXml->setAttribute("id", p);
        patXml->setAttribute("isSaved", isPatternSaved[p]);
        if (isPatternSaved[p]) {
            patXml->setAttribute("num", savedPatterns[p].num);
            patXml->setAttribute("den", savedPatterns[p].den);
            patXml->setAttribute("bars", savedPatterns[p].bars);
            for (int t = 0; t < 8; ++t) {
                patXml->setAttribute("div" + juce::String(t), savedPatterns[p].trackDivisions[t]);
                patXml->setAttribute("shift" + juce::String(t), savedPatterns[p].trackShift[t]);
                patXml->setAttribute("cmplx" + juce::String(t), savedPatterns[p].trackComplexity[t]);
                patXml->setAttribute("entrp" + juce::String(t), savedPatterns[p].trackEntropy[t]);
                juce::String ptStr;
                for (int s = 0; s < 1024; ++s) ptStr << savedPatterns[p].drumPattern[t][s] << ",";
                patXml->setAttribute("patData" + juce::String(t), ptStr);
            }
        }
        xml.addChildElement(patXml);
    }

    // UserTunings の保存 (既存コードと同様)
    auto* tuningsXml = new juce::XmlElement("UserTunings");
    for (int g = 0; g < 26; ++g) {
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
        timeSigNumerator.store(xmlState->getIntAttribute("timeSigNum", 4));
        timeSigDenominator.store(xmlState->getIntAttribute("timeSigDen", 4));
        fillBarTarget.store(xmlState->getIntAttribute("fillBarTarget", 0));
        arpMode.store(xmlState->getBoolAttribute("arpMode", false));
        arpMono.store(xmlState->getBoolAttribute("arpMono", true));
        arpKey.store(xmlState->getIntAttribute("arpKey", 0));
        arpScale.store(xmlState->getIntAttribute("arpScale", 1));
        autoFollowEnabled.store(xmlState->getBoolAttribute("autoFollow", true));
        timeSigLocked.store(xmlState->getBoolAttribute("tsLock", false));

        // 各トラックの復元
        for (auto* trackXml : xmlState->getChildWithTagNameIterator("TrackData")) {
            int i = trackXml->getIntAttribute("id", -1);
            if (i >= 0 && i < 8) {
                trackDivisionsUI[i] = trackXml->getIntAttribute("div", 4);
                trackShiftUI[i] = trackXml->getIntAttribute("shift", 0);
                trackComplexity[i] = trackXml->getIntAttribute("cmplx", 50);
                trackEntropy[i] = trackXml->getIntAttribute("entrp", 0);
                trackOctaveUI[i] = trackXml->getIntAttribute("oct", 0);
                trackDegreeUI[i] = trackXml->getIntAttribute("deg", 0);

                trackLocked[i] = trackXml->getBoolAttribute("lock", false);
                trackDivLocked[i] = trackXml->getBoolAttribute("divLck", false);
                trackCmplxLocked[i] = trackXml->getBoolAttribute("cmplxLck", false);
                trackEntrpLocked[i] = trackXml->getBoolAttribute("entrpLck", false);
                trackShiftLocked[i] = trackXml->getBoolAttribute("shiftLck", false);
                trackDegreeLocked[i] = trackXml->getBoolAttribute("degLck", false);
                trackOctaveLocked[i] = trackXml->getBoolAttribute("octLck", false);

                trackMuted[i] = trackXml->getBoolAttribute("mute", false);
                trackSoloed[i] = trackXml->getBoolAttribute("solo", false);
                trackDynamic[i] = trackXml->getBoolAttribute("dyn", false);
                trackDynamicAmount[i] = trackXml->getIntAttribute("dynAmt", 30);

                // サンプルの復元
                juce::String sPath = trackXml->getStringAttribute("samplePath", "");
                if (sPath.isNotEmpty()) {
                    loadSample(i, sPath);
                }
                else {
                    clearSample(i);
                }

                // シーケンスの復元
                juce::String patStr = trackXml->getStringAttribute("pattern", "");
                juce::StringArray tokens;
                tokens.addTokens(patStr, ",", "");
                for (int s = 0; s < juce::jmin(1024, tokens.size()); ++s) {
                    drumPatternUI[i][s] = tokens[s].getIntValue();
                }
            }
        }

        // Pat1〜4 の復元
        for (auto* patXml : xmlState->getChildWithTagNameIterator("SavedPattern")) {
            int p = patXml->getIntAttribute("id", -1);
            if (p >= 0 && p < 4) {
                isPatternSaved[p] = patXml->getBoolAttribute("isSaved", false);
                if (isPatternSaved[p]) {
                    savedPatterns[p].num = patXml->getIntAttribute("num", 4);
                    savedPatterns[p].den = patXml->getIntAttribute("den", 4);
                    savedPatterns[p].bars = patXml->getIntAttribute("bars", 4);
                    for (int t = 0; t < 8; ++t) {
                        savedPatterns[p].trackDivisions[t] = patXml->getIntAttribute("div" + juce::String(t), 4);
                        savedPatterns[p].trackShift[t] = patXml->getIntAttribute("shift" + juce::String(t), 0);
                        savedPatterns[p].trackComplexity[t] = patXml->getIntAttribute("cmplx" + juce::String(t), 50);
                        savedPatterns[p].trackEntropy[t] = patXml->getIntAttribute("entrp" + juce::String(t), 0);
                        juce::String ptStr = patXml->getStringAttribute("patData" + juce::String(t), "");
                        juce::StringArray tks;
                        tks.addTokens(ptStr, ",", "");
                        for (int s = 0; s < juce::jmin(1024, tks.size()); ++s) {
                            savedPatterns[p].drumPattern[t][s] = tks[s].getIntValue();
                        }
                    }
                }
            }
        }

        // UserTunings の復元 (既存コードと同様)
        if (auto* tuningsXml = xmlState->getChildByName("UserTunings")) {
            for (auto* gXml : tuningsXml->getChildIterator()) {
                int g = gXml->getIntAttribute("id", -1);
                if (g >= 0 && g < 26) {
                    if (g == 24) { userTuning[g].tempo.min = 80; userTuning[g].tempo.max = 140; }
                    else if (g == 25) { userTuning[g].tempo.min = 80; userTuning[g].tempo.max = 100; }
                    else {
                        userTuning[g].tempo.min = gXml->getIntAttribute("tMin", 0);
                        userTuning[g].tempo.max = gXml->getIntAttribute("tMax", 0);
                    }
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

        patternUpdated.store(true);
        uiNeedsUpdate.store(true);
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new AIDrumMachineAudioProcessor(); }