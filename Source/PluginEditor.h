#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "Network/GeminiClient.h"

// ★修正：FileDragAndDropTarget を追加継承してOSからのファイルドロップを受信可能に
class AIDrumMachineAudioProcessorEditor : public juce::AudioProcessorEditor,
    public juce::Timer,
    public juce::DragAndDropContainer,
    public juce::FileDragAndDropTarget
{
public:
    AIDrumMachineAudioProcessorEditor(AIDrumMachineAudioProcessor&);
    ~AIDrumMachineAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;

    // ★追加：外部ファイルドラッグ＆ドロップ用関数
    bool isInterestedInFileDrag(const juce::StringArray& files) override;
    void filesDropped(const juce::StringArray& files, int x, int y) override;
    void fileDragEnter(const juce::StringArray& files, int x, int y) override;
    void fileDragMove(const juce::StringArray& files, int x, int y) override;
    void fileDragExit(const juce::StringArray& files) override;

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

    juce::Label trackNameLabels[8];
    juce::String trackNotes[8] = { "C1", "D1", "F#1", "A#1", "D#1", "F1", "A1", "D2" };

    juce::Rectangle<float> dragAllArea;
    juce::Rectangle<float> lockArea;
    juce::Rectangle<float> sampleArea;
    juce::Rectangle<float> mainGridArea;
    juce::Rectangle<float> midiDragArea;
    bool isDragging = false;

    // ★追加：ドラッグ中にどのトラックがターゲットになっているかを保持
    int dragTargetTrack = -1;
    int getTrackIndexFromMouseY(int y); // 座標計算ヘルパー

    void updateTabColors();
    bool needsPagination() const;
    juce::File exportMidi(int trackIndex);

    GeminiClient gemini;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AIDrumMachineAudioProcessorEditor)
};