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

// ★【Phase 2 & 1】階層化アンカーシステムとユークリッド補助生成
void AIDrumMachineAudioProcessor::randomizeTrack(int trk) {
    if (trk < 0 || trk >= 8 || trackLocked[trk]) return;

    int genre = currentGenre.load();

    // ジャンルに応じたDivisionの自動決定
    if (!trackDivLocked[trk]) {
        if (genre == 0 || genre == 1 || genre == 18) trackDivisions[trk] = 4; // Techno, House, Boom Bap
        else if (genre == 2) trackDivisions[trk] = (trk == 2 || trk == 3) ? 6 : 4; // UK Garage
        else if (genre == 4 || genre == 7) trackDivisions[trk] = (trk == 2) ? 6 : 4; // Trap, Dubstep
        else if (genre == 16) trackDivisions[trk] = 6; // New Jack Swing
        else trackDivisions[trk] = random.nextInt(juce::Range<int>(3, 8));
    }

    if (!trackCmplxLocked[trk]) trackComplexity[trk] = random.nextInt(juce::Range<int>(10, 90));

    int div = trackDivisions[trk];
    int n = div * globalBarCount;
    int cmplx = trackComplexity[trk];
    int k = juce::jmax(1, (n * cmplx) / 100);

    // ユークリッドの開始位置をランダムにずらす（頭に集中させないため）
    int offset = random.nextInt(juce::Range<int>(0, n));

    for (int j = 0; j < 36; ++j) {
        if (j >= n) { drumPattern[trk][j] = 0; continue; }

        int beatPos = j % div;
        int beatNum = j / div; // 0=1拍目, 1=2拍目, 2=3拍目, 3=4拍目
        bool isDownbeat = (beatPos == 0);
        bool isBackbeat = (beatPos == 0 && (beatNum == 1 || beatNum == 3));

        bool isAnchor = false;
        bool isNegativeAnchor = false;
        int anchorVel = 0;

        // 【Phase 2】ジャンル別のアンカー（絶対配置）とネガティブアンカー（絶対消去）
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
            if (trk == 1 && j == div * 2) { isAnchor = true; anchorVel = 100; } // ハーフタイム
            break;
        case 7: // Dubstep
            if (trk == 0 && j == 0) { isAnchor = true; anchorVel = 100; }
            if (trk == 1 && j == div * 2) { isAnchor = true; anchorVel = 100; }
            if (trk == 1 && j != div * 2) { isNegativeAnchor = true; }
            break;
        case 8: // Afrobeat
            if (trk == 0 && isDownbeat) { isAnchor = true; anchorVel = 100; }
            break;
        case 9: // Gqom
            if (trk == 0 && isDownbeat && beatNum != 3) { isAnchor = true; anchorVel = 100; }
            if (trk == 0 && beatNum == 3) { isNegativeAnchor = true; }
            break;
        case 10: // Amapiano
            if (trk == 0 && isDownbeat) { isAnchor = true; anchorVel = 90; }
            if (trk == 5 && beatNum > 0) { isAnchor = true; anchorVel = 95; } // ログドラム
            break;
        case 13: // Reggaeton / Dembow
            if (trk == 0 && isDownbeat) { isAnchor = true; anchorVel = 100; }
            if (trk == 1 && (j == div - 1 || j == div * 2 + div / 2)) { isAnchor = true; anchorVel = 90; }
            break;
        case 15: // Funk
            if (j == 0) { isAnchor = true; anchorVel = 100; } // The One
            if (trk == 1 && isBackbeat) { isAnchor = true; anchorVel = 100; }
            break;
        case 16: // New Jack Swing
            if (trk == 0 && (j == 0 || j == div * 2 + 1)) { isAnchor = true; anchorVel = 100; }
            if (trk == 1 && isBackbeat) { isAnchor = true; anchorVel = 100; }
            break;
        case 18: // Hip Hop (Boom Bap)
            if (trk == 0 && (j == 0 || j == div * 2 || j == div * 2 + div / 2)) { isAnchor = true; anchorVel = 95; }
            if (trk == 1 && isBackbeat) { isAnchor = true; anchorVel = 100; }
            break;
        default:
            break;
        }

        // 【Phase 1】アンカーで埋まっていない部分にユークリッドを流し込む（コンディショナル・フィラー）
        if (isAnchor) {
            drumPattern[trk][j] = anchorVel;
        }
        else if (isNegativeAnchor) {
            drumPattern[trk][j] = 0;
        }
        else {
            // オフセットを加算してユークリッド配置を計算
            bool isHit = (((j + offset) * k) % n) < k;
            if (isHit) {
                drumPattern[trk][j] = random.nextInt(juce::Range<int>(60, 95)); // フィラーはやや弱め
            }
            else {
                drumPattern[trk][j] = 0;
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