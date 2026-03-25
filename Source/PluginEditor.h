// ==============================================================================
// Source/PluginEditor.h
// ==============================================================================
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

    juce::Label timeSigLabel{ "", "Time Sig:" };
    juce::ComboBox timeSigNumMenu;
    juce::Label timeSigSlash{ "", "/" };
    juce::ComboBox timeSigDenMenu;

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

    // ★ パターン保存ボタン
    juce::TextButton btnPattern[4];

    const juce::String trackNotes[8] = { "C1", "D1", "F#1", "A#1", "D#1", "F1", "A1", "D2" };

    juce::Label trackNameLabels[8];
    juce::Label midiKeyLabels[8];

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

    juce::Slider entropySliders[8];
    juce::Label entrpLabels[8];
    juce::TextButton btnEntrpLock[8];

    juce::Slider shiftSliders[8];
    juce::Label shiftLabels[8];
    juce::TextButton btnShiftLock[8];

    juce::Rectangle<float> dragAllArea;
    juce::Rectangle<float> lockArea;
    juce::Rectangle<float> sampleArea;
    juce::Rectangle<float> mainGridArea;
    juce::Rectangle<float> midiDragArea;
    bool isDragging = false;

    int dragTargetTrack = -1;
    int getTrackIndexFromMouseY(int y);

    void updateTabColors();
    void updatePatternButtonColors();
    void updateViewVisibility();
    void updateDivisionMenus();
    void updateTimeSigNumMenu();

    juce::File exportMidi(int trackIndex);

    GeminiClient gemini;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AIDrumMachineAudioProcessorEditor)
};