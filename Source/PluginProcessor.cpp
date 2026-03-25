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
    int totalSteps = trackDivisions[trk] * globalBarCount;
    int firstStep = drumPattern[trk][0];
    for (int i = 0; i < totalSteps - 1; ++i) drumPattern[trk][i] = drumPattern[trk][i + 1];
    drumPattern[trk][totalSteps - 1] = firstStep;
}

void AIDrumMachineAudioProcessor::shiftTrackRight(int trk) {
    if (trk < 0 || trk >= 8 || trackLocked[trk]) return;
    int totalSteps = trackDivisions[trk] * globalBarCount;
    int lastStep = drumPattern[trk][totalSteps - 1];
    for (int i = totalSteps - 1; i > 0; --i) drumPattern[trk][i] = drumPattern[trk][i - 1];
    drumPattern[trk][0] = lastStep;
}

void AIDrumMachineAudioProcessor::clearTrack(int trk) {
    if (trk < 0 || trk >= 8 || trackLocked[trk]) return;
    for (int j = 0; j < 36; ++j) drumPattern[trk][j] = 0;
}

// ★【超重要コア】全22ジャンル＋2アルゴリズムの階層化アンカー＆Divisionルール
void AIDrumMachineAudioProcessor::randomizeTrack(int trk) {
    if (trk < 0 || trk >= 8 || trackLocked[trk]) return;

    int genre = currentGenre.load();

    // 1. ジャンル別 Division の自動最適化（ロックされていなければ）
    if (!trackDivLocked[trk]) {
        int newDiv = 4;
        switch (genre) {
        case 0: newDiv = (trk >= 5) ? (random.nextBool() ? 3 : 5) : 4; break; // Techno: タムは奇数
        case 1: newDiv = 6; break; // House: 16分3連
        case 2: newDiv = (trk == 2 || trk == 3) ? (random.nextBool() ? 4 : 6) : 6; break; // UK Garage
        case 3: newDiv = (trk == 2 || trk == 3 || trk == 7) ? (random.nextBool() ? 8 : 9) : 4; break; // DnB: ハットとタムは超高速ロール
        case 4: newDiv = (trk == 2 || trk == 3) ? (random.nextBool() ? 6 : 9) : (random.nextBool() ? 3 : 6); break; // Trap: 3連ベース、ハットは可変
        case 5: newDiv = (trk == 0 || trk == 2) ? 6 : 4; break; // Footwork: キックとハットは3連、スネアは4
        case 6: newDiv = random.nextInt(juce::Range<int>(5, 10)); break; // IDM: カオス奇数
        case 7: newDiv = (trk == 2 || trk == 3) ? 3 : 4; break; // Dubstep: ハーフタイム＋3連ハット
        case 8: newDiv = (trk == 2 || trk == 3) ? 6 : 4; break; // Afrobeat: ポリリズム
        case 9: newDiv = (trk == 6) ? 6 : 4; break; // Gqom: ゴーストキックは3連
        case 10: newDiv = (trk >= 5 && random.nextFloat() > 0.7f) ? 6 : 4; break; // Amapiano: ログドラムフィルのみ3連
        case 11: newDiv = random.nextBool() ? 5 : 7; break; // Indian: 5拍子か7拍子
        case 12: newDiv = 4; break; // Samba: 16分固定
        case 13: newDiv = (trk == 2) ? 8 : 4; break; // Reggaeton: ハットのみ32分許可
        case 14: newDiv = (trk == 0) ? 1 : ((trk == 1) ? 4 : 8); break; // Gamelan: トラックで階層化
        case 15: newDiv = (trk == 1 && random.nextFloat() > 0.8f) ? 9 : 4; break; // Funk: 稀にスネアのゴーストロール
        case 16: newDiv = 6; break; // New Jack Swing: スウィング固定
        case 17: newDiv = (trk == 2 || trk == 3) ? 5 : 4; break; // Neo Soul: ハットだけDiv5でヨレさせる
        case 18: newDiv = (random.nextFloat() > 0.8f) ? 6 : 4; break; // Hip Hop: たまに3連ハネ
        case 19: newDiv = random.nextInt(juce::Range<int>(4, 8)); break; // Math Rock: 4,5,6,7の混在
        case 20: newDiv = (trk == 0) ? 8 : ((trk == 2) ? 7 : 4); break; // Prog Metal: キック連打と7連ポリ
        case 21: newDiv = random.nextBool() ? 3 : 6; break; // Minimalism: 12/8ベース
        case 22: // Pure Euclidean
        case 23: // Pure Chaos
            newDiv = random.nextInt(juce::Range<int>(1, 10)); break; // 完全にランダム
        default: newDiv = 4; break;
        }
        trackDivisions[trk] = newDiv;
    }

    if (!trackCmplxLocked[trk]) trackComplexity[trk] = random.nextInt(juce::Range<int>(10, 90));

    int div = trackDivisions[trk];
    int n = div * globalBarCount;
    int cmplx = trackComplexity[trk];
    int k = juce::jmax(1, (n * cmplx) / 100);
    int offset = random.nextInt(juce::Range<int>(0, n)); // ユークリッドのズラし

    for (int j = 0; j < 36; ++j) {
        if (j >= n) { drumPattern[trk][j] = 0; continue; }

        int beatPos = j % div;
        int beatNum = j / div;
        bool isDownbeat = (beatPos == 0);
        bool isBackbeat = (beatPos == 0 && (beatNum == 1 || beatNum == 3));

        bool isAnchor = false;
        bool isNegativeAnchor = false;
        int anchorVel = 0;

        // 2. ジャンル別 アンカー（絶対配置）＆ネガティブ（絶対消去）辞書
        switch (genre) {
        case 0: // Techno
            if (trk == 0 && isDownbeat) { isAnchor = true; anchorVel = 100; }
            if (trk == 1 && isDownbeat && !isBackbeat) { isNegativeAnchor = true; }
            if (trk == 2 && beatPos == div / 2) { isAnchor = true; anchorVel = 90; }
            break;
        case 1: // House
            if (trk == 0 && isDownbeat) { isAnchor = true; anchorVel = 100; }
            if ((trk == 1 || trk == 4) && isBackbeat) { isAnchor = true; anchorVel = 100; }
            if ((trk == 1 || trk == 4) && isDownbeat && !isBackbeat) { isNegativeAnchor = true; }
            break;
        case 2: // UK Garage
            if (trk == 0 && (j == 0 || j == div * 2 + div / 2 + 1)) { isAnchor = true; anchorVel = 100; }
            if (trk == 1 && isBackbeat) { isAnchor = true; anchorVel = 100; }
            if (trk == 0 && isBackbeat) { isNegativeAnchor = true; }
            break;
        case 3: // Drum & Bass
            if (trk == 0 && (j == 0 || j == div * 2 + div / 2)) { isAnchor = true; anchorVel = 100; }
            if (trk == 1 && isBackbeat) { isAnchor = true; anchorVel = 100; }
            break;
        case 4: // Trap
            if (trk == 0 && j == 0) { isAnchor = true; anchorVel = 100; }
            if (trk == 1 && beatNum == 2 && beatPos == 0) { isAnchor = true; anchorVel = 100; } // 3拍目ハーフタイム
            break;
        case 5: // Footwork
            if (trk == 0 && (j == 0 || j == div * 2 || j == div * 4)) { isAnchor = true; anchorVel = 100; }
            if (trk == 1 && isBackbeat) { isAnchor = true; anchorVel = 90; }
            break;
        case 6: // IDM
            if (j == 0 && trk != 0) { isNegativeAnchor = true; } // 頭はキックのみ
            break;
        case 7: // Dubstep
            if (trk == 0 && j == 0) { isAnchor = true; anchorVel = 100; }
            if (trk == 1 && beatNum == 2 && beatPos == 0) { isAnchor = true; anchorVel = 100; }
            if (trk == 1 && beatNum != 2) { isNegativeAnchor = true; }
            break;
        case 8: // Afrobeat
            if (trk == 0 && isDownbeat) { isAnchor = true; anchorVel = 100; }
            if (trk == 2 && (j == 0 || j == 3 || j == 5 || j == 8 || j == 10)) { isAnchor = true; anchorVel = 90; } // 12/8クラーベ
            break;
        case 9: // Gqom
            if (trk == 0 && isDownbeat && beatNum != 3) { isAnchor = true; anchorVel = 100; }
            if (trk == 0 && beatNum == 3) { isNegativeAnchor = true; }
            break;
        case 10: // Amapiano
            if (trk == 0 && isDownbeat) { isAnchor = true; anchorVel = 80; }
            if (trk >= 5 && isDownbeat) { isNegativeAnchor = true; } // ログドラムは裏主体
            break;
        case 11: // Indian Classical
            if (trk == 0 && j == 0) { isNegativeAnchor = true; } // 頭抜き
            break;
        case 12: // Samba / Bossa Nova
            if (trk == 0 && (beatNum == 1 || beatNum == 3) && beatPos == 0) { isAnchor = true; anchorVel = 100; } // 2,4強調
            if (trk == 1 && (j == 0 || j == 3 || j == 6 || j == 10 || j == 13)) { isAnchor = true; anchorVel = 90; } // クラーベ
            break;
        case 13: // Reggaeton
            if (trk == 0 && isDownbeat) { isAnchor = true; anchorVel = 100; }
            if (trk == 1 && (j == 3 || j == 6 || j == 11 || j == 14)) { isAnchor = true; anchorVel = 95; } // トレシージョ
            break;
        case 14: // Gamelan
            if (trk == 0 && j == 0) { isAnchor = true; anchorVel = 100; }
            if (trk == 0 && j != 0) { isNegativeAnchor = true; }
            break;
        case 15: // Funk
            if (j == 0 && trk == 0) { isAnchor = true; anchorVel = 100; } // The One
            if (trk == 1 && isBackbeat) { isAnchor = true; anchorVel = 100; }
            break;
        case 16: // New Jack Swing
            if (trk == 0 && (j == 0 || j == div * 2 + 1)) { isAnchor = true; anchorVel = 100; }
            if (trk == 1 && isBackbeat) { isAnchor = true; anchorVel = 100; }
            break;
        case 17: // Neo Soul
            if (trk == 0 && isDownbeat) { isAnchor = true; anchorVel = 90; }
            if (trk == 1 && isBackbeat) { isAnchor = true; anchorVel = 95; }
            break;
        case 18: // Hip Hop (Boom Bap)
            if (trk == 0 && (j == 0 || j == div * 2 || j == div * 2 + div / 2)) { isAnchor = true; anchorVel = 95; }
            if (trk == 1 && isBackbeat) { isAnchor = true; anchorVel = 100; }
            if (trk == 1 && isDownbeat && !isBackbeat) { isNegativeAnchor = true; }
            break;
        case 19: // Math Rock
            if (trk == 0 && j == 0) { isAnchor = true; anchorVel = 100; }
            break;
        case 20: // Prog Metal
            if (trk == 1 && isBackbeat) { isAnchor = true; anchorVel = 100; }
            break;
        case 21: // Minimalism
            if (j == 0) { isAnchor = true; anchorVel = 90; }
            break;
        case 22: // Pure Euclidean
        case 23: // Pure Chaos
            // アンカー一切なし。完全にアルゴリズムに委ねる
            isAnchor = false;
            isNegativeAnchor = false;
            break;
        default:
            break;
        }

        // 3. 隙間（フィラー）への流し込み処理
        if (isAnchor) {
            drumPattern[trk][j] = anchorVel;
        }
        else if (isNegativeAnchor) {
            drumPattern[trk][j] = 0;
        }
        else {
            // Genre 23 (Chaos) の場合はユークリッドを無視して純粋な確率で打つ
            if (genre == 23) {
                float probThreshold = 1.0f - (cmplx / 100.0f);
                if (random.nextFloat() > probThreshold) {
                    drumPattern[trk][j] = random.nextInt(juce::Range<int>(50, 101));
                }
                else {
                    drumPattern[trk][j] = 0;
                }
            }
            else {
                // 通常（ユークリッド補助）
                bool isHit = (((j + offset) * k) % n) < k;
                if (isHit) {
                    drumPattern[trk][j] = random.nextInt(juce::Range<int>(50, 95));
                }
                else {
                    drumPattern[trk][j] = 0;
                }
            }
        }
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

    int numSamples = buffer.getNumSamples();
    float sampleRate = (float)getSampleRate();
    if (sampleRate <= 0.0f) sampleRate = 44100.0f;

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

    int samplesPerBar = (int)(sampleRate * (60.0 / bpm) * 4.0);
    int samplesPerLoop = samplesPerBar * globalBarCount;
    float pi2 = juce::MathConstants<float>::twoPi;

    bool anySolo = false;
    for (int i = 0; i < 8; ++i) { if (trackSoloed[i]) { anySolo = true; break; } }

    auto* leftChannel = buffer.getWritePointer(0);
    auto* rightChannel = buffer.getNumChannels() > 1 ? buffer.getWritePointer(1) : nullptr;

    juce::ScopedLock sl(sampleLock);

    for (int i = 0; i < numSamples; ++i)
    {
        if (isPlaying) {
            samplesInLoop++;
            if (samplesInLoop >= samplesPerLoop) samplesInLoop = 0;

            for (int trk = 0; trk < 8; ++trk)
            {
                int div = trackDivisions[trk];
                if (div < 1) div = 1;
                int totalStepsInLoop = div * globalBarCount;
                int currentStepForTrack = (samplesInLoop * totalStepsInLoop) / samplesPerLoop;

                if (currentStepForTrack != trackCurrentStep[trk])
                {
                    trackCurrentStep[trk] = currentStepForTrack;

                    bool shouldPlay = true;
                    if (anySolo && !trackSoloed[trk]) shouldPlay = false;
                    if (trackMuted[trk] && !trackSoloed[trk]) shouldPlay = false;

                    int velocity = drumPattern[trk][trackCurrentStep[trk]];
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

bool AIDrumMachineAudioProcessor::hasEditor() const { return true; }
juce::AudioProcessorEditor* AIDrumMachineAudioProcessor::createEditor() { return new AIDrumMachineAudioProcessorEditor(*this); }
void AIDrumMachineAudioProcessor::getStateInformation(juce::MemoryBlock& destData) {}
void AIDrumMachineAudioProcessor::setStateInformation(const void* data, int sizeInBytes) {}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new AIDrumMachineAudioProcessor(); }