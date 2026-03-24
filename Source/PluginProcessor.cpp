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

AIDrumMachineAudioProcessor::~AIDrumMachineAudioProcessor()
{
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

void AIDrumMachineAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentStep = 0;
    samplesSinceLastStep = 0;
    for (int i = 0; i < 8; ++i) {
        trackEnv[i] = 0.0f;
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

    // テンポ120BPMの16分音符の長さ
    int samplesPerStep = (int)(sampleRate * 0.125f);

    auto* leftChannel = buffer.getWritePointer(0);
    auto* rightChannel = buffer.getNumChannels() > 1 ? buffer.getWritePointer(1) : nullptr;

    for (int i = 0; i < numSamples; ++i)
    {
        // 1. シーケンサー（再生ヘッドを進める）
        samplesSinceLastStep++;
        if (samplesSinceLastStep >= samplesPerStep)
        {
            samplesSinceLastStep = 0;
            currentStep = (currentStep + 1) % 16;

            // ステップが切り替わった瞬間、1が立っているトラックを鳴らす
            for (int trk = 0; trk < 8; ++trk)
            {
                if (drumPattern[trk][currentStep] > 0)
                {
                    trackEnv[trk] = 1.0f;
                }
            }
        }

        // 2. シンセサイザー（音を作る）
        float mixOut = 0.0f;
        for (int trk = 0; trk < 8; ++trk)
        {
            if (trackEnv[trk] > 0.001f)
            {
                float osc = 0.0f;

                if (trk == 0) {
                    // キック（低音ドン！）
                    float freq = 60.0f;
                    trackPhase[trk] += juce::MathConstants<float>::twoPi * freq / sampleRate;
                    if (trackPhase[trk] > juce::MathConstants<float>::twoPi) trackPhase[trk] -= juce::MathConstants<float>::twoPi;
                    osc = std::sin(trackPhase[trk]) * trackEnv[trk];
                    trackEnv[trk] *= 0.9995f;
                }
                else if (trk == 1) {
                    // スネア（タン！）
                    float noise = random.nextFloat() * 2.0f - 1.0f;
                    osc = noise * trackEnv[trk];
                    trackEnv[trk] *= 0.999f;
                }
                else if (trk == 2) {
                    // ハイハット（チッ！）
                    float noise = random.nextFloat() * 2.0f - 1.0f;
                    osc = noise * trackEnv[trk];
                    trackEnv[trk] *= 0.995f;
                }
                else {
                    // その他のピコピコ音
                    float freq = 300.0f + (trk * 200.0f);
                    trackPhase[trk] += juce::MathConstants<float>::twoPi * freq / sampleRate;
                    if (trackPhase[trk] > juce::MathConstants<float>::twoPi) trackPhase[trk] -= juce::MathConstants<float>::twoPi;
                    osc = std::sin(trackPhase[trk]) * trackEnv[trk];
                    trackEnv[trk] *= 0.999f;
                }

                // ★音量を大きくしました (0.2f -> 0.6f)
                mixOut += osc * 0.6f;
            }
        }

        // 出力が大きすぎたときのためにリミッター（音割れ防止）をかける
        if (mixOut > 1.0f) mixOut = 1.0f;
        if (mixOut < -1.0f) mixOut = -1.0f;

        // 3. スピーカーへ出力
        leftChannel[i] = mixOut;
        if (rightChannel != nullptr)
            rightChannel[i] = mixOut;
    }
}

bool AIDrumMachineAudioProcessor::hasEditor() const { return true; }
juce::AudioProcessorEditor* AIDrumMachineAudioProcessor::createEditor() { return new AIDrumMachineAudioProcessorEditor(*this); }
void AIDrumMachineAudioProcessor::getStateInformation(juce::MemoryBlock& destData) {}
void AIDrumMachineAudioProcessor::setStateInformation(const void* data, int sizeInBytes) {}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new AIDrumMachineAudioProcessor(); }