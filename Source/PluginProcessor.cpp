// ==============================================================================
// Source/PluginProcessor.cpp
// ==============================================================================
#include "PluginProcessor.h"
#include "PluginEditor.h"

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

void AIDrumMachineAudioProcessor::shiftTrackLeft(int trk) {
    if (trk < 0 || trk >= 8 || trackLocked[trk]) return;
    int totalSteps = trackDivisions[trk] * timeSigNumerator.load() * globalBarCount;
    if (totalSteps <= 0) return;
    int firstStep = drumPattern[trk][0];
    for (int i = 0; i < totalSteps - 1; ++i) drumPattern[trk][i] = drumPattern[trk][i + 1];
    drumPattern[trk][totalSteps - 1] = firstStep;
    patternUpdated.store(true);
}

void AIDrumMachineAudioProcessor::shiftTrackRight(int trk) {
    if (trk < 0 || trk >= 8 || trackLocked[trk]) return;
    int totalSteps = trackDivisions[trk] * timeSigNumerator.load() * globalBarCount;
    if (totalSteps <= 0) return;
    int lastStep = drumPattern[trk][totalSteps - 1];
    for (int i = totalSteps - 1; i > 0; --i) drumPattern[trk][i] = drumPattern[trk][i - 1];
    drumPattern[trk][0] = lastStep;
    patternUpdated.store(true);
}

void AIDrumMachineAudioProcessor::clearTrack(int trk) {
    if (trk < 0 || trk >= 8 || trackLocked[trk]) return;
    for (int j = 0; j < 1024; ++j) drumPattern[trk][j] = 0;
    patternUpdated.store(true);
}

// 全24ジャンル対応 データ駆動型アルゴリズム
void AIDrumMachineAudioProcessor::generateAllTracks() {
    int genre = currentGenre.load();
    bool isAlgorithmMode = (genre >= 22);

    // ジャンル別 デフォルトテンポ範囲
    const int minTempos[] = { 125, 120, 130, 160, 135, 160,  90, 135,  90, 120, 110,  80,  80,  90,  80, 100, 100,  85,  85, 120, 140, 120, 60, 60 };
    const int maxTempos[] = { 135, 126, 135, 180, 145, 160, 180, 145, 110, 127, 115, 120, 120,  95, 120, 115, 112,  95,  95, 160, 180, 170, 200, 200 };

    if (!isSyncEnabled.load()) {
        double newBpm = (double)random.nextInt(juce::Range<int>(minTempos[genre], maxTempos[genre] + 1));
        internalTempo.store(newBpm);
    }

    // ジャンル別 推奨拍子設定
    int recommendedNum = 4;
    int recommendedDen = 4;
    switch (genre) {
    case 11: recommendedNum = 7; recommendedDen = 8; break; // Indian Classical
    case 14: recommendedNum = 8; recommendedDen = 4; break; // Gamelan
    case 19: recommendedNum = 5; recommendedDen = 4; break; // Math Rock
    case 20: recommendedNum = 7; recommendedDen = 8; break; // Prog Metal
    case 21: recommendedNum = 12; recommendedDen = 8; break; // Minimalism
    default: recommendedNum = 4; recommendedDen = 4; break;
    }

    timeSigNumerator.store(recommendedNum);
    timeSigDenominator.store(recommendedDen);
    int num = recommendedNum;
    int den = recommendedDen;
    int maxDiv = (den == 16) ? 2 : ((den == 8) ? 4 : 8);

    for (int trk = 0; trk < 8; ++trk) {
        if (trackLocked[trk]) continue;

        // 1. Divisionの決定
        if (!trackDivLocked[trk]) {
            int newDiv = 4;
            if (!isAlgorithmMode && trk >= 5) trackComplexity[trk] = 0;
            else if (isAlgorithmMode) trackComplexity[trk] = 50;

            switch (genre) {
            case 1:  newDiv = 6; break;
            case 2:  newDiv = (trk >= 2) ? 6 : 4; break; // Garage swing
            case 4:  newDiv = (trk == 2) ? 8 : (random.nextBool() ? 4 : 6); break;
            case 5:  newDiv = (trk == 0 || trk == 2) ? 6 : 4; break;
            case 6:  newDiv = random.nextInt(juce::Range<int>(5, 10)); break;
            case 11: newDiv = random.nextBool() ? 5 : 7; break;
            case 13: newDiv = (trk == 1 || trk == 4) ? 8 : 4; break;
            case 16: newDiv = 6; break;
            case 17: newDiv = (trk == 2) ? 5 : 4; break;
            case 19: newDiv = random.nextInt(juce::Range<int>(4, 8)); break;
            case 20: newDiv = (trk == 0) ? 8 : 7; break;
            case 21: newDiv = 3; break;
            case 22: case 23: newDiv = random.nextInt(juce::Range<int>(1, 10)); break;
            default: newDiv = 4; break;
            }
            if (newDiv > maxDiv) newDiv = maxDiv;
            trackDivisions[trk] = newDiv;
        }
        else {
            if (trackDivisions[trk] > maxDiv) trackDivisions[trk] = maxDiv;
        }

        // 2. Entropyの決定
        if (!trackEntrpLocked[trk]) {
            trackEntropy[trk] = isAlgorithmMode ? 50 : random.nextInt(juce::Range<int>(0, 15));
        }

        // 3. Micro-Shiftの決定
        if (!trackShiftLocked[trk]) {
            int newShift = random.nextInt(juce::Range<int>(-3, 4));
            if (genre == 17) {
                if (trk == 0) newShift = random.nextInt(juce::Range<int>(-5, 2));
                if (trk == 1) newShift = random.nextInt(juce::Range<int>(10, 21));
                if (trk == 2) newShift = random.nextInt(juce::Range<int>(20, 36));
                if (trk == 4) newShift = random.nextInt(juce::Range<int>(5, 15));
            }
            else if (genre == 18) {
                if (trk == 0) newShift = random.nextInt(juce::Range<int>(2, 8));
                if (trk == 1) newShift = random.nextInt(juce::Range<int>(5, 12));
            }
            else if (genre == 0 || genre == 3) {
                if (trk == 2) newShift = random.nextInt(juce::Range<int>(-8, -2));
            }
            else if (genre == 13) {
                if (trk == 1) newShift = random.nextInt(juce::Range<int>(-6, 0));
            }
            else if (genre == 8) {
                if (trk == 2 || trk == 3) newShift = random.nextInt(juce::Range<int>(3, 10));
            }
            trackShift[trk] = newShift;
        }

        // 4. パターン生成（ユークリッド & アンカー）
        int div = trackDivisions[trk];
        int n = div * num * globalBarCount;
        int cmplx = trackComplexity[trk];
        int entrp = trackEntropy[trk];
        int k = juce::jmax(1, (n * cmplx) / 100);
        int offset = random.nextInt(juce::Range<int>(0, juce::jmax(1, n)));

        for (int j = 0; j < 1024; ++j) {
            if (j >= n) { drumPattern[trk][j] = 0; continue; }

            int stepInBar = j % (div * num);
            bool isAnchor = false;
            bool isNegativeAnchor = false;
            int anchorVel = 100;

            switch (genre) {
            case 0:
                if (trk == 0 && stepInBar % div == 0) isAnchor = true;
                if (trk == 1 && (stepInBar == div || stepInBar == div * 3)) isAnchor = true;
                if (trk == 2 && (stepInBar % div == div / 2)) isAnchor = true;
                break;
            case 1:
                if (trk == 0 && stepInBar % div == 0) isAnchor = true;
                if (trk == 2 && div >= 2 && (stepInBar % div == div - 2)) { isAnchor = true; anchorVel = 85; }
                break;
            case 2:
                if (trk == 0) {
                    if (stepInBar == 0 || stepInBar == div * 2 + div / 2) isAnchor = true;
                    if (stepInBar == div || stepInBar == div * 3) isNegativeAnchor = true;
                }
                if (trk == 1 && (stepInBar == div || stepInBar == div * 3)) isAnchor = true;
                break;
            case 4:
                if (trk == 0 && stepInBar == 0) isAnchor = true;
                if (trk == 1 && stepInBar == div * 2) isAnchor = true;
                break;
            case 13:
                if (trk == 0 && stepInBar % div == 0) isAnchor = true;
                if (trk == 1 && (stepInBar == div - 1 || stepInBar == div * 2 + div / 2)) isAnchor = true;
                break;
            case 15:
                if (trk == 0 && stepInBar == 0) isAnchor = true;
                if (trk == 1 && (stepInBar == div || stepInBar == div * 3)) isAnchor = true;
                break;
            case 17:
                if (trk == 0 && stepInBar % div == 0) isAnchor = true;
                if (trk == 2) isAnchor = true;
                break;
            default:
                if (trk == 0 && stepInBar == 0) isAnchor = true;
                break;
            }

            if (isAnchor) {
                int velJitter = (int)((entrp / 100.0f) * 30.0f);
                drumPattern[trk][j] = anchorVel - random.nextInt(juce::Range<int>(0, velJitter + 1));
            }
            else if (isNegativeAnchor) {
                int ghostProb = (cmplx / 3) + (entrp / 2);
                drumPattern[trk][j] = (random.nextInt(100) < ghostProb) ? random.nextInt(juce::Range<int>(10, 30 + (entrp / 5))) : 0;
            }
            else if (cmplx > 0) {
                if (genre == 23) { // Pure Chaos
                    drumPattern[trk][j] = (random.nextFloat() > (1.0f - cmplx / 100.0f)) ? random.nextInt(juce::Range<int>(40, 101)) : 0;
                }
                else { // Euclidean
                    bool isHit = (((j + offset) * k) % n) < k;
                    if (entrp > 0 && random.nextInt(100) < (entrp / 3)) {
                        isHit = !isHit;
                    }
                    drumPattern[trk][j] = isHit ? random.nextInt(juce::Range<int>(40, 90 + (entrp / 10))) : 0;
                }
            }
            else {
                drumPattern[trk][j] = 0;
            }
        }
    }

    // DSP側へのコピーとUIの更新をリクエスト
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

    // 初期状態のバッファ同期
    for (int i = 0; i < 8; ++i) {
        for (int j = 0; j < 1024; ++j) {
            drumPatternDSP[i][j] = drumPattern[i][j];
        }
    }
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

    // ★ スレッドセーフティ：UIからのパターン変更があればバックバッファにコピー（ロックフリー）
    if (patternUpdated.exchange(false)) {
        for (int i = 0; i < 8; ++i) {
            for (int j = 0; j < 1024; ++j) {
                drumPatternDSP[i][j] = drumPattern[i][j];
            }
        }
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
    int den = timeSigDenominator.load();
    double samplesPerBeat = samplesPerQuarterNote * (4.0 / (double)den);

    int num = timeSigNumerator.load();
    int samplesPerLoop = (int)(samplesPerBeat * num * globalBarCount);
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
                int div = trackDivisions[trk]; if (div < 1) div = 1;
                int totalStepsInLoop = div * num * globalBarCount;

                double samplesPerStep = (double)samplesPerLoop / (double)totalStepsInLoop;
                int shiftInSamples = (int)((trackShift[trk] / 100.0) * samplesPerStep);

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

                    // ★ DSPバッファから読み出し
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