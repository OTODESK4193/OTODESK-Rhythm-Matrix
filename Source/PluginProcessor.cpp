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
}

AIDrumMachineAudioProcessor::~AIDrumMachineAudioProcessor() {}

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

void AIDrumMachineAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    resetPosition();
    for (int i = 0; i < 8; ++i) {
        trackEnv[i] = 0.0f;
        trackPitchEnv[i] = 0.0f;
        trackPhase[i] = 0.0f;
    }
}

void AIDrumMachineAudioProcessor::releaseResources() {}

#ifndef JucePlugin_PreferredChannelConfigurations
bool AIDrumMachineAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
        && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
    return true;
}
#endif

void AIDrumMachineAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    int numSamples = buffer.getNumSamples();
    float sampleRate = (float)getSampleRate();
    if (sampleRate <= 0.0f) sampleRate = 44100.0f;

    // ★追加：トランスポート（DAW同期）の処理
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

    // UI表示用と安全策（極端なBPMを防ぐ）
    if (bpm < 20.0) bpm = 20.0;
    if (bpm > 999.0) bpm = 999.0;
    currentBpm.store(bpm);

    // ★修正：固定120BPMから、現在のBPMを用いた動的計算に変更
    int samplesPerBar = (int)(sampleRate * (60.0 / bpm) * 4.0);
    int samplesPerLoop = samplesPerBar * 4;
    float pi2 = juce::MathConstants<float>::twoPi;

    auto* leftChannel = buffer.getWritePointer(0);
    auto* rightChannel = buffer.getNumChannels() > 1 ? buffer.getWritePointer(1) : nullptr;

    for (int i = 0; i < numSamples; ++i)
    {
        // ★修正：再生中(isPlaying)の時だけシーケンサーを進める
        if (isPlaying) {
            samplesInLoop++;
            if (samplesInLoop >= samplesPerLoop) {
                samplesInLoop = 0;
            }

            for (int trk = 0; trk < 8; ++trk)
            {
                int div = trackDivisions[trk];
                if (div < 1) div = 1;
                int totalStepsInLoop = div * 4;

                int currentStepForTrack = (samplesInLoop * totalStepsInLoop) / samplesPerLoop;

                if (currentStepForTrack != trackCurrentStep[trk])
                {
                    trackCurrentStep[trk] = currentStepForTrack;

                    int velocity = drumPattern[trk][trackCurrentStep[trk]];
                    if (velocity > 0)
                    {
                        trackEnv[trk] = velocity / 100.0f;
                        trackPitchEnv[trk] = 1.0f;
                    }
                }
            }
        }

        // シンセサイザー処理（既存のまま）
        float mixOut = 0.0f;
        for (int trk = 0; trk < 8; ++trk)
        {
            if (trackEnv[trk] > 0.001f)
            {
                float osc = 0.0f;
                if (trk == 0) {
                    float freq = 50.0f + 200.0f * trackPitchEnv[trk];
                    trackPhase[trk] += pi2 * freq / sampleRate;
                    if (trackPhase[trk] > pi2) trackPhase[trk] -= pi2;
                    osc = std::sin(trackPhase[trk]) * trackEnv[trk];
                    trackPitchEnv[trk] *= 0.995f;
                    trackEnv[trk] *= 0.9995f;
                }
                else if (trk == 1) {
                    float freq = 180.0f + 50.0f * trackPitchEnv[trk];
                    trackPhase[trk] += pi2 * freq / sampleRate;
                    if (trackPhase[trk] > pi2) trackPhase[trk] -= pi2;
                    float body = std::sin(trackPhase[trk]) * trackEnv[trk];
                    float noise = (random.nextFloat() * 2.0f - 1.0f) * (trackEnv[trk] * trackEnv[trk]);
                    osc = (body * 0.5f + noise * 0.6f);
                    trackPitchEnv[trk] *= 0.99f;
                    trackEnv[trk] *= 0.998f;
                }
                else if (trk == 2) {
                    float noise = random.nextFloat() * 2.0f - 1.0f;
                    osc = noise * trackEnv[trk] * 0.4f;
                    trackEnv[trk] *= 0.992f;
                }
                else if (trk == 3) {
                    float noise = random.nextFloat() * 2.0f - 1.0f;
                    osc = noise * trackEnv[trk] * 0.4f;
                    trackEnv[trk] *= 0.999f;
                }
                else if (trk == 4) {
                    float noise = random.nextFloat() * 2.0f - 1.0f;
                    osc = noise * trackEnv[trk] * 0.5f;
                    trackEnv[trk] *= 0.996f;
                }
                else if (trk >= 5 && trk <= 7) {
                    float baseFreq = 100.0f + (trk - 5) * 50.0f;
                    float freq = baseFreq + 100.0f * trackPitchEnv[trk];
                    trackPhase[trk] += pi2 * freq / sampleRate;
                    if (trackPhase[trk] > pi2) trackPhase[trk] -= pi2;
                    osc = std::sin(trackPhase[trk]) * trackEnv[trk];
                    trackPitchEnv[trk] *= 0.996f;
                    trackEnv[trk] *= 0.999f;
                }
                mixOut += osc * 0.6f;
            }
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