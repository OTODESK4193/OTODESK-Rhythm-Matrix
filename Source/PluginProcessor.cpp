// ==============================================================================
// Source/PluginProcessor.cpp
// ==============================================================================
#include "PluginProcessor.h"
#include "PluginEditor.h"

// ★ ジャンルごとの適正テンポ(min, max)を含むルックアップテーブル
static const std::array<GenreDefinition, 24> genreTable = { {
        // 0: Techno
        { 4, 4, 125, 135, {"909 Kick", "909 Snare", "CHH", "OHH", "Clap", "Ride", "Tom", "Noise FX"},
          {{1,2,4,0}, {2,4,0,0}, {2,4,0,0}, {2,4,0,0}, {2,4,0,0}, {3,4,5,0}, {3,5,7,0}, {2,4,8,0}},
          {0,0,0,0,0,0,0,0}, {0,0,2,2,0,5,5,10} },
          // 1: House
          { 4, 4, 120, 126, {"Deep Kick", "Rimshot", "Shuff Hat", "Open Hat", "Clap", "Conga", "Bongo", "Vocal Chop"},
            {{1,2,4,0}, {2,4,0,0}, {4,6,0,0}, {2,4,0,0}, {2,4,0,0}, {3,4,5,0}, {4,6,8,0}, {2,3,4,0}},
            {-2,0, 5,0,-2, -5, -5, 0}, {2,5, 15,5, 5, 10, 10, 15} },
            // 2: UK Garage
            { 4, 4, 130, 138, {"Punch Kick", "Snare/Rim", "Garage Hat", "Ride", "Clap", "Sub Bass", "Perc 1", "Perc 2"},
              {{2,3,4,0}, {2,4,0,0}, {4,6,0,0}, {4,6,0,0}, {2,4,0,0}, {2,3,4,0}, {3,5,0,0}, {5,7,0,0}},
              {-5,5, 10,5, 0, -10, 0, 0}, {5,15, 25,15, 10, 10, 15, 15} },
              // 3: D&B
              { 4, 4, 165, 175, {"Heavy Kick", "Tight Snr", "Fast Hat", "Ride", "Break Rim", "Break 1", "Break 2", "Sub"},
                {{2,4,0,0}, {2,4,0,0}, {4,8,0,0}, {4,8,0,0}, {2,4,0,0}, {4,6,8,0}, {4,6,8,0}, {2,4,0,0}},
                {0,0, -5,-5, 0, -10, -10, 0}, {2,2, 5,5, 5, 10, 10, 5} },
                // 4: Trap
                { 4, 4, 135, 150, {"808 Kick", "808 Snare", "Roll Hat", "Open Hat", "Clap", "Perc", "808 Bass", "FX"},
                  {{2,4,0,0}, {2,4,0,0}, {4,6,8,0}, {2,4,0,0}, {2,4,0,0}, {4,8,0,0}, {2,4,0,0}, {2,4,0,0}},
                  {0,0, 0,0, 0, 0, 0, 0}, {0,0, 0,0, 0, 0, 0, 0} },
                  // 5: Footwork
                  { 4, 4, 160, 160, {"Juke Kick", "Snare", "Fast Hat", "Hat 2", "Clap", "Tom", "Vocal 1", "Vocal 2"},
                    {{3,4,6,0}, {3,4,6,0}, {4,6,8,0}, {4,6,8,0}, {2,4,0,0}, {3,5,6,0}, {3,4,5,0}, {4,6,7,0}},
                    {-5,-5, -5,-5, 0, -10, -5, -5}, {5,5, 5,5, 5, 10, 15, 15} },
                    // 6: IDM
                    { 4, 4, 140, 180, {"Glitch Kick", "Drill Snr", "Hat 1", "Hat 2", "Noise", "Perc 1", "Perc 2", "Glitch FX"},
                      {{4,5,7,0}, {4,6,8,0}, {5,7,9,0}, {6,8,9,0}, {3,5,7,0}, {5,7,9,0}, {4,6,8,0}, {3,5,7,0}},
                      {-10,-10, -15,-15, -20, -20, -20, -20}, {10,10, 15,15, 20, 20, 20, 20} },
                      // 7: Dubstep
                      { 4, 4, 140, 150, {"Stomp Kick", "Fat Snare", "Hat", "Ride", "Clap", "Wobble", "Growl", "Sub"},
                        {{2,4,0,0}, {2,4,0,0}, {4,6,0,0}, {2,4,0,0}, {2,4,0,0}, {2,4,8,0}, {2,4,0,0}, {1,2,4,0}},
                        {0,0, 0,0, 0, 0, 0, 0}, {0,0, 5,5, 0, 10, 5, 0} },
                        // 8: Afrobeat
                        { 4, 4, 95, 115, {"Acoustic Kick", "Snare", "Shaker 1", "Shaker 2", "Clave", "Conga", "Djembe", "Agogo"},
                          {{2,4,0,0}, {2,4,0,0}, {4,6,0,0}, {4,6,0,0}, {3,4,0,0}, {3,4,5,0}, {3,4,6,0}, {2,3,4,0}},
                          {0,0, 3,3, 5, 5, 5, 5}, {5,10, 15,15, 20, 20, 20, 20} },
                          // 9: Gqom
                          { 4, 4, 98, 108, {"Heavy Kick", "Snare", "Hat", "Open Hat", "Clap", "Tom 1", "Tom 2", "Chant"},
                            {{3,4,0,0}, {2,4,0,0}, {3,4,0,0}, {2,4,0,0}, {2,4,0,0}, {3,4,5,0}, {3,4,5,0}, {2,3,4,0}},
                            {-5,0, 0,0, -5, -5, -5, 0}, {0,5, 10,5, 0, 10, 10, 10} },
                            // 10: Amapiano
                            { 4, 4, 110, 115, {"Log Drum", "Snare/Rim", "Shaker", "Open Hat", "Clap", "Conga", "Woodblock", "Whistle"},
                              {{3,4,0,0}, {2,4,0,0}, {4,6,0,0}, {2,4,0,0}, {2,4,0,0}, {3,4,5,0}, {3,4,5,0}, {2,3,4,0}},
                              {0,0, 5,0, 0, 5, 5, 0}, {10,5, 15,5, 5, 15, 15, 10} },
                              // 11: Indian
                              { 7, 8, 80, 120, {"Bayan", "Dayan", "Tabla", "Manjira", "Ghungroo", "Dholak 1", "Dholak 2", "Vocal"},
                                {{2,3,4,0}, {2,3,4,5}, {2,3,4,5}, {2,3,4,0}, {3,4,5,6}, {2,3,4,0}, {2,3,4,0}, {1,2,3,0}},
                                {-5,-5, -5, -2, -2, -5, -5, -10}, {5,5, 5, 5, 5, 5, 5, 10} },
                                // 12: Samba
                                { 4, 4, 90, 120, {"Surdo", "Caixa", "Pandeiro", "Ganza", "Tamborim", "Agogo", "Cuica", "Repique"},
                                  {{2,4,0,0}, {2,4,0,0}, {4,6,0,0}, {4,6,0,0}, {3,4,0,0}, {3,4,0,0}, {3,4,0,0}, {4,6,0,0}},
                                  {0,5, 5,5, 5, 5, 5, 5}, {5,15, 20,20, 20, 20, 20, 20} },
                                  // 13: Reggaeton
                                  { 4, 4, 90, 105, {"Kick", "Snare (Tresillo)", "Hat", "Open Hat", "Clap", "Timbales", "Perc", "Vocal FX"},
                                    {{2,4,0,0}, {3,4,0,0}, {2,4,0,0}, {2,4,0,0}, {2,4,0,0}, {3,4,0,0}, {3,4,0,0}, {2,4,0,0}},
                                    {0,-6, 0,0, 0, 0, 0, 0}, {5,0, 5,5, 5, 10, 10, 10} },
                                    // 14: Gamelan
                                    { 8, 4, 80, 110, {"Gong", "Kempul", "Kendang", "Bonang", "Saron", "Kenong", "Kethuk", "Slenthem"},
                                      {{1,2,0,0}, {1,2,4,0}, {2,3,4,0}, {3,4,5,6}, {2,4,6,0}, {1,2,4,0}, {2,4,0,0}, {1,2,4,0}},
                                      {-10,-10, -5,-5, -5, -10, -5, -10}, {10,10, 10,10, 10, 10, 10, 10} },
                                      // 15: Funk
                                      { 4, 4, 100, 115, {"Kick", "Snare", "Hi-Hat", "Open Hat", "Clap", "Tom", "Conga", "Tambourine"},
                                        {{2,4,0,0}, {2,4,0,0}, {4,6,0,0}, {2,4,0,0}, {2,4,0,0}, {3,4,0,0}, {3,4,6,0}, {4,6,0,0}},
                                        {0,0, 5,0, 0, 0, 5, 5}, {5,10, 20,5, 5, 10, 15, 20} },
                                        // 16: NJS
                                        { 4, 4, 100, 112, {"Punch Kick", "Snare", "Swing Hat", "Open Hat", "Clap", "Tom 1", "Tom 2", "Orch Hit"},
                                          {{2,4,0,0}, {2,4,0,0}, {6,8,0,0}, {2,4,0,0}, {2,4,0,0}, {3,4,0,0}, {3,4,0,0}, {1,2,0,0}},
                                          {0,0, 15,0, 0, 0, 0, 0}, {5,5, 25,5, 5, 5, 5, 5} },
                                          // 17: Neo Soul
                                          { 4, 4, 80, 95, {"Soft Kick", "Rimshot", "Loose Hat", "Ride", "Snap", "Tom", "Shaker", "Vinyl FX"},
                                            {{2,4,0,0}, {2,4,0,0}, {3,4,6,0}, {3,4,0,0}, {2,4,0,0}, {2,3,4,0}, {4,6,0,0}, {1,2,0,0}},
                                            {-5, 10, 20, 10, 5, 0, 15, 0}, {2, 25, 40, 25, 20, 10, 30, 0} },
                                            // 18: Hip Hop
                                            { 4, 4, 85, 95, {"Gritty Kick", "Fat Snare", "Hi-Hat", "Open Hat", "Clap", "Perc", "Scratch", "Sample"},
                                              {{2,4,0,0}, {2,4,0,0}, {3,4,6,0}, {2,4,0,0}, {2,4,0,0}, {3,4,0,0}, {2,4,0,0}, {1,2,4,0}},
                                              {2, 5, 5, 0, 0, 0, 0, 0}, {8, 12, 15, 5, 5, 10, 5, 0} },
                                              // 19: Math Rock
                                              { 5, 4, 120, 160, {"Kick", "Snare", "Hi-Hat", "Ride", "Ghost Snr", "Tom 1", "Tom 2", "Crash"},
                                                {{2,3,4,0}, {2,3,4,0}, {4,5,6,0}, {3,4,5,0}, {4,6,8,0}, {3,4,5,0}, {3,4,5,0}, {1,2,0,0}},
                                                {0,0, 0,0, 0, 0, 0, 0}, {5,5, 5,5, 5, 5, 5, 5} },
                                                // 20: Prog Metal
                                                { 7, 8, 140, 180, {"Heavy Kick", "Fat Snare", "Hi-Hat", "China", "Ghost Snr", "Low Tom", "Mid Tom", "High Tom"},
                                                  {{2,3,4,5}, {2,3,4,0}, {4,6,8,0}, {2,3,4,0}, {4,6,8,0}, {3,4,5,0}, {3,4,5,0}, {3,4,5,0}},
                                                  {0,0, 0,0, 0, 0, 0, 0}, {2,2, 2,2, 5, 5, 5, 5} },
                                                  // 21: Minimalism
                                                  { 12, 8, 140, 170, {"Clap 1", "Clap 2", "Marimba 1", "Marimba 2", "Woodblock", "Pulse", "Phase 1", "Phase 2"},
                                                    {{2,3,4,0}, {2,3,4,0}, {3,4,5,0}, {3,4,5,0}, {2,3,4,0}, {2,3,4,0}, {3,4,5,0}, {3,4,5,0}},
                                                    {0,0, 0,0, 0, 0, -20, 20}, {0,0, 0,0, 0, 0, -20, 20} },
                                                    // 22: Pure Euclidean
                                                    { 4, 4, 120, 150, {"Node 1", "Node 2", "Node 3", "Node 4", "Node 5", "Node 6", "Node 7", "Node 8"},
                                                      {{2,3,5,7}, {2,3,5,7}, {2,3,5,7}, {2,3,5,7}, {2,3,5,7}, {2,3,5,7}, {2,3,5,7}, {2,3,5,7}},
                                                      {0,0,0,0,0,0,0,0}, {0,0,0,0,0,0,0,0} },
                                                      // 23: Pure Chaos
                                                      { 4, 4, 120, 150, {"Chaos 1", "Chaos 2", "Chaos 3", "Chaos 4", "Chaos 5", "Chaos 6", "Chaos 7", "Chaos 8"},
                                                        {{1,3,6,9}, {1,3,6,9}, {1,3,6,9}, {1,3,6,9}, {1,3,6,9}, {1,3,6,9}, {1,3,6,9}, {1,3,6,9}},
                                                        {-20,-20,-20,-20,-20,-20,-20,-20}, {20,20,20,20,20,20,20,20} }
                                                  } };

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
}

AIDrumMachineAudioProcessor::~AIDrumMachineAudioProcessor() {}

const GenreDefinition& AIDrumMachineAudioProcessor::getGenreDef(int index) {
    if (index < 0 || index >= 24) return genreTable[0];
    return genreTable[index];
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
    const auto& def = getGenreDef(genre);
    bool isAlgorithmMode = (genre >= 22);

    // ★ テンポのランダム適用（適正範囲内）
    if (!isSyncEnabled.load()) {
        int newBpm = random.nextInt(juce::Range<int>(def.minTempo, def.maxTempo + 1));
        internalTempo.store((double)newBpm);
    }

    int num = timeSigNumerator.load();
    int den = timeSigDenominator.load();

    // ★ IDM (6) の拍子ランダム化
    if (genre == 6) {
        const int idmSigs[6][2] = { {4,4}, {5,4}, {5,8}, {7,8}, {7,16}, {15,16} };
        int idx = random.nextInt(6);
        num = idmSigs[idx][0];
        den = idmSigs[idx][1];
        timeSigNumerator.store(num);
        timeSigDenominator.store(den);
    }

    int maxDiv = (den == 16) ? 2 : ((den == 8) ? 4 : 8);
    int bars = globalBarCount.load();

    for (int trk = 0; trk < 8; ++trk) {
        if (trackLocked[trk]) continue;

        if (!trackDivLocked[trk]) {
            std::vector<int> candidates;
            for (int i = 0; i < 4; ++i) {
                if (def.allowedDivs[trk][i] > 0) candidates.push_back(def.allowedDivs[trk][i]);
            }
            int newDiv = 4;
            if (!candidates.empty()) newDiv = candidates[random.nextInt(candidates.size())];
            if (newDiv > maxDiv) newDiv = maxDiv;
            trackDivisionsUI[trk] = newDiv;

            if (!isAlgorithmMode) {
                trackComplexity[trk] = (trk == 0 || trk == 1 || trk == 4) ? 0 : (20 + random.nextInt(30));
            }
            else {
                trackComplexity[trk] = 50;
            }
        }

        if (!trackEntrpLocked[trk]) {
            trackEntropy[trk] = isAlgorithmMode ? 50 : random.nextInt(juce::Range<int>(0, 20));
        }

        if (!trackShiftLocked[trk]) {
            trackShiftUI[trk] = random.nextInt(juce::Range<int>(def.shiftMin[trk], def.shiftMax[trk] + 1));
        }

        int div = trackDivisionsUI[trk];
        int n = div * num * bars;
        int cmplx = trackComplexity[trk];
        int entrp = trackEntropy[trk];
        int k = juce::jmax(1, (n * cmplx) / 100);
        int offset = random.nextInt(juce::Range<int>(0, juce::jmax(1, n)));

        for (int j = 0; j < 1024; ++j) {
            if (j >= n) { drumPatternUI[trk][j] = 0; continue; }

            int stepInBar = j % (div * num);
            bool isAnchor = false;
            bool isNegativeAnchor = false;
            int anchorVel = 100;

            if (!isAlgorithmMode) {
                switch (genre) {
                case 0: case 1:
                    if (trk == 0 && (stepInBar % div == 0)) isAnchor = true;
                    if ((trk == 1 || trk == 4) && (stepInBar == div || stepInBar == div * 3)) isAnchor = true;
                    if (trk == 2 && (stepInBar % div == div / 2)) isAnchor = true;
                    break;
                case 2:
                    if (trk == 0 && (stepInBar == 0 || stepInBar == div + div / 2 || stepInBar == div * 3 + div / 2)) isAnchor = true;
                    if ((trk == 1 || trk == 4) && (stepInBar == div || stepInBar == div * 3)) isAnchor = true;
                    break;
                case 3:
                    if (trk == 0 && (stepInBar == 0 || stepInBar == div * 2 + div / 2)) isAnchor = true;
                    if ((trk == 1 || trk == 4) && (stepInBar == div || stepInBar == div * 3)) isAnchor = true;
                    break;
                case 4: case 7:
                    if (trk == 0 && (stepInBar == 0 || stepInBar == div + div / 2)) isAnchor = true;
                    if ((trk == 1 || trk == 4) && stepInBar == div * 2) isAnchor = true;
                    break;
                case 8:
                    if (trk == 0 && (stepInBar == 0 || stepInBar == div + div / 2 || stepInBar == div * 2 || stepInBar == div * 3 + div / 2)) isAnchor = true;
                    if ((trk == 1 || trk == 4) && (stepInBar == div || stepInBar == div * 3)) isAnchor = true;
                    break;
                case 13:
                    if (trk == 0 && (stepInBar % div == 0)) isAnchor = true;
                    if ((trk == 1 || trk == 4) && (stepInBar == div - 1 || stepInBar == div * 2 + div / 2)) isAnchor = true;
                    break;
                case 14: // ★ Gamelan
                    if (trk == 0 && stepInBar == 0 && j < div * num) isAnchor = true; // ゴングは1小節に1度
                    if (trk == 1 && stepInBar == div * 4) isAnchor = true;
                    if (trk == 2 && (stepInBar == div * 2 || stepInBar == div * 6)) isAnchor = true;
                    break;
                case 15: case 18:
                    if (trk == 0 && (stepInBar == 0 || stepInBar == div * 2 + div / 2)) isAnchor = true;
                    if ((trk == 1 || trk == 4) && (stepInBar == div || stepInBar == div * 3)) isAnchor = true;
                    break;
                case 19:
                    if (trk == 0 && (stepInBar == 0 || stepInBar == div * 3)) isAnchor = true;
                    if ((trk == 1 || trk == 4) && (stepInBar == div * 2)) isAnchor = true;
                    break;
                case 20: // ★ Prog Metal (3+2+2等 細かく散らす)
                    if (trk == 0 && (stepInBar == 0 || stepInBar == div * 3 || stepInBar == div * 5)) isAnchor = true;
                    if ((trk == 1 || trk == 4) && (stepInBar == div * 3 || stepInBar == div * 5)) isAnchor = true;
                    break;
                case 21: // ★ Minimalism (位相ズレパルス)
                    if (trk == 0 && (stepInBar == 0 || stepInBar == div * 3 || stepInBar == div * 6 || stepInBar == div * 9)) isAnchor = true;
                    if (trk == 1 && (stepInBar == div * 2 || stepInBar == div * 5 || stepInBar == div * 8 || stepInBar == div * 11)) isAnchor = true;
                    break;
                default:
                    if (trk == 0 && stepInBar == 0) isAnchor = true;
                    if ((trk == 1 || trk == 4) && (stepInBar == div || stepInBar == div * 3) && num >= 4) isAnchor = true;
                    break;
                }

                // ★ Kickのゴーストノート（各拍の裏：16分音符グリッドの4, 8, 12, 16ステップ目相当）
                if (trk == 0 && !isAnchor) {
                    if (stepInBar % div == div - 1) { // 拍の最後のステップ
                        if (random.nextInt(100) < (15 + entrp / 2)) {
                            drumPatternUI[trk][j] = random.nextInt(juce::Range<int>(40, 75)); // 弱〜中ベロシティ
                            continue;
                        }
                    }
                }
            }

            if (isAnchor) {
                int velJitter = (int)((entrp / 100.0f) * 20.0f);
                drumPatternUI[trk][j] = anchorVel - random.nextInt(juce::Range<int>(0, velJitter + 1));
            }
            else if (isNegativeAnchor) {
                int ghostProb = (cmplx / 3) + (entrp / 2);
                drumPatternUI[trk][j] = (random.nextInt(100) < ghostProb) ? random.nextInt(juce::Range<int>(10, 30 + (entrp / 5))) : 0;
            }
            else if (!trackCmplxLocked[trk] && cmplx > 0) {
                if (genre == 23) {
                    drumPatternUI[trk][j] = (random.nextFloat() > (1.0f - cmplx / 100.0f)) ? random.nextInt(juce::Range<int>(40, 101)) : 0;
                }
                else {
                    bool isHit = (((j + offset) * k) % n) < k;
                    if (entrp > 0 && random.nextInt(100) < (entrp / 3)) isHit = !isHit;
                    drumPatternUI[trk][j] = isHit ? random.nextInt(juce::Range<int>(40, 90 + (entrp / 10))) : 0;
                }
            }
            else {
                drumPatternUI[trk][j] = 0;
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
    for (int i = 0; i < 8; ++i) { trackEnv[i] = 0.0f; trackPitchEnv[i] = 0.0f; trackPhase[i] = 0.0f; samplePlayPos[i] = -1; }

    for (int i = 0; i < 8; ++i) {
        std::memcpy(drumPatternDSP[i], drumPatternUI[i], sizeof(drumPatternUI[i]));
        trackDivisionsDSP[i] = trackDivisionsUI[i];
        trackShiftDSP[i] = trackShiftUI[i];
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

    if (patternUpdated.exchange(false)) {
        for (int i = 0; i < 8; ++i) {
            std::memcpy(drumPatternDSP[i], drumPatternUI[i], sizeof(drumPatternUI[i]));
            trackDivisionsDSP[i] = trackDivisionsUI[i];
            trackShiftDSP[i] = trackShiftUI[i];
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
    int samplesPerLoop = (int)(samplesPerBeat * timeSigNumDSP * globalBarCountDSP);
    float pi2 = juce::MathConstants<float>::twoPi;

    bool anySolo = false;
    for (int i = 0; i < 8; ++i) { if (trackSoloed[i]) { anySolo = true; break; } }

    auto* leftChannel = buffer.getWritePointer(0);
    auto* rightChannel = buffer.getNumChannels() > 1 ? buffer.getWritePointer(1) : nullptr;

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
                        float vol = velocity / 100.0f;
                        trackEnv[trk] = vol;
                        trackPitchEnv[trk] = 1.0f;
                        if (hasSample[trk]) { samplePlayPos[trk] = 0; sampleVolume[trk] = vol; }
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
            else if (trackEnv[trk] > 0.001f) {
                if (trk == 0) {
                    float freq = 50.0f + 200.0f * trackPitchEnv[trk];
                    trackPhase[trk] += pi2 * freq / sampleRate;
                    if (trackPhase[trk] > pi2) trackPhase[trk] -= pi2;
                    osc = std::sin(trackPhase[trk]) * trackEnv[trk];
                    trackPitchEnv[trk] *= 0.995f; trackEnv[trk] *= 0.9995f;
                }
                else if (trk == 1) {
                    float freq = 180.0f + 50.0f * trackPitchEnv[trk];
                    trackPhase[trk] += pi2 * freq / sampleRate;
                    if (trackPhase[trk] > pi2) trackPhase[trk] -= pi2;
                    float body = std::sin(trackPhase[trk]) * trackEnv[trk];
                    float noise = (random.nextFloat() * 2.0f - 1.0f) * (trackEnv[trk] * trackEnv[trk]);
                    osc = (body * 0.5f + noise * 0.6f);
                    trackPitchEnv[trk] *= 0.99f; trackEnv[trk] *= 0.998f;
                }
                else if (trk >= 2 && trk <= 4) {
                    float noise = random.nextFloat() * 2.0f - 1.0f;
                    osc = noise * trackEnv[trk] * 0.4f;
                    if (trk == 2) trackEnv[trk] *= 0.992f;
                    else if (trk == 3) trackEnv[trk] *= 0.999f;
                    else trackEnv[trk] *= 0.996f;
                }
                else if (trk >= 5 && trk <= 7) {
                    float baseFreq = 100.0f + (trk - 5) * 50.0f;
                    float freq = baseFreq + 100.0f * trackPitchEnv[trk];
                    trackPhase[trk] += pi2 * freq / sampleRate;
                    if (trackPhase[trk] > pi2) trackPhase[trk] -= pi2;
                    osc = std::sin(trackPhase[trk]) * trackEnv[trk];
                    trackPitchEnv[trk] *= 0.996f; trackEnv[trk] *= 0.999f;
                }
            }
            mixOut += osc * 0.6f;
        }

        if (mixOut > 1.0f) mixOut = 1.0f;
        if (mixOut < -1.0f) mixOut = -1.0f;

        leftChannel[i] = mixOut;
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
void AIDrumMachineAudioProcessor::getStateInformation(juce::MemoryBlock& destData) {}
void AIDrumMachineAudioProcessor::setStateInformation(const void* data, int sizeInBytes) {}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new AIDrumMachineAudioProcessor(); }