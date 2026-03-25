#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "Network/GeminiClient.h"

class AIDrumMachineAudioProcessorEditor : public juce::AudioProcessorEditor,
    public juce::Timer,
    public juce::DragAndDropContainer
{
public:
    AIDrumMachineAudioProcessorEditor(AIDrumMachineAudioProcessor&);
    ~AIDrumMachineAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;

    void timerCallback() override;

private:
    AIDrumMachineAudioProcessor& audioProcessor;

    juce::ToggleButton syncButton{ "Sync" };
    juce::TextButton playButton{ "Play" };
    juce::TextButton stopButton{ "Stop" };
    juce::Label tempoLabel;
    juce::int64 lastStopClickTime = 0;

    juce::TextButton generateButton{ "Generate" };
    juce::ComboBox styleMenu;
    juce::Label statusLabel;

    juce::TextButton tabButton1{ "Bar 1" };
    juce::TextButton tabButton2{ "Bar 2" };
    juce::TextButton tabButton3{ "Bar 3" };
    juce::TextButton tabButton4{ "Bar 4" };
    int currentViewBar = 0;

    // ★追加：各トラックの書き換え可能な名前ラベルと、固定のノート名
    juce::Label trackNameLabels[8];
    juce::String trackNotes[8] = { "C1", "D1", "F#1", "A#1", "D#1", "F1", "A1", "D2" };

    // UIレイアウト用のエリア
    juce::Rectangle<float> dragAllArea;
    juce::Rectangle<float> lockArea;
    juce::Rectangle<float> sampleArea; // ★名称変更：将来のD&D枠
    juce::Rectangle<float> mainGridArea;
    juce::Rectangle<float> midiDragArea;
    bool isDragging = false;

    void updateTabColors();
    bool needsPagination() const;
    juce::File exportMidi(int trackIndex);

    GeminiClient gemini;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AIDrumMachineAudioProcessorEditor)
};