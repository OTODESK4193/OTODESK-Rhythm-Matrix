#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "Network/GeminiClient.h"

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

    juce::TextButton tabSeqButton{ "SEQUENCER" };
    juce::TextButton tabSetupButton{ "SETUP" };
    enum ViewMode { SequencerView, SetupView };
    ViewMode currentView = SequencerView;

    juce::TextButton btnClearAll{ "CLEAR ALL" };

    juce::Label barCountLabel{ "", "Bars:" };
    juce::ComboBox barCountMenu;

    juce::TextButton generateButton{ "Generate" };
    juce::ComboBox styleMenu;
    juce::Label statusLabel;

    juce::TextButton tabButton1{ "Bar 1" };
    juce::TextButton tabButton2{ "Bar 2" };
    juce::TextButton tabButton3{ "Bar 3" };
    juce::TextButton tabButton4{ "Bar 4" };
    int currentViewBar = 0;

    // ★追加：サンプル削除時のラベル復元用に配列をメンバ化
    const juce::String defaultTrackNames[8] = { "Kick", "Snare", "CHH", "OHH", "Clap", "L.Tom", "M.Tom", "H.Tom" };
    juce::Label trackNameLabels[8];

    juce::TextButton btnMute[8];
    juce::TextButton btnSolo[8];
    juce::TextButton btnClear[8];
    juce::TextButton btnShiftL[8];
    juce::TextButton btnShiftR[8];

    juce::ComboBox divSelectors[8];
    juce::Slider complexitySliders[8];
    juce::Label divLabels[8];
    juce::Label compLabels[8];
    juce::TextButton btnDivLock[8];
    juce::TextButton btnCmplxLock[8];

    juce::Rectangle<float> dragAllArea;
    juce::Rectangle<float> lockArea;
    juce::Rectangle<float> sampleArea;
    juce::Rectangle<float> mainGridArea;
    juce::Rectangle<float> midiDragArea;
    bool isDragging = false;

    int dragTargetTrack = -1;
    int getTrackIndexFromMouseY(int y);

    void updateTabColors();
    void updateViewVisibility();
    bool needsPagination() const;
    juce::File exportMidi(int trackIndex);

    GeminiClient gemini;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AIDrumMachineAudioProcessorEditor)
};