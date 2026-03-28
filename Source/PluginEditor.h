// ==============================================================================
// Source/PluginEditor.h
// ==============================================================================
#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

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
    juce::TextButton btnTempoLock{ "L" };
    juce::int64 lastStopClickTime = 0;

    juce::TextButton tabSeqButton{ "SEQUENCER" };
    juce::TextButton tabSetupButton{ "SETUP 1" };
    juce::TextButton tabSetup2Button{ "SETUP 2" };
    juce::TextButton tabTuningButton{ "TUNING" };

    enum ViewMode { SequencerView, Setup1View, Setup2View, TuningView };
    ViewMode currentView = SequencerView;

    juce::TextButton btnClearAll{ "CLEAR ALL" };

    juce::Label timeSigLabel{ "", "Time Sig:" };
    juce::ComboBox timeSigNumMenu;
    juce::Label timeSigSlash{ "", "/" };
    juce::ComboBox timeSigDenMenu;
    juce::TextButton btnTimeSigLock{ "L" };

    juce::Label barCountLabel{ "", "Bars:" };
    juce::ComboBox barCountMenu;

    juce::TextButton generateButton{ "Generate" };
    juce::ComboBox styleMenu;
    juce::ComboBox fillBarMenu;

    juce::TextButton btnAutoFollow{ "Follow" };
    juce::TextButton btnArpMode{ "Arp Mode" };

    juce::Label statusLabel;

    juce::TextButton tabButton1{ "Bar 1" };
    juce::TextButton tabButton2{ "Bar 2" };
    juce::TextButton tabButton3{ "Bar 3" };
    juce::TextButton tabButton4{ "Bar 4" };
    int currentViewBar = 0;

    juce::TextButton btnPattern[4];

    juce::ComboBox arpKeyMenu;
    juce::ComboBox arpScaleMenu;
    juce::TextButton btnArpMono{ "Mono Mode" };

    juce::Slider octaveSliders[8];
    juce::Label octaveLabels[8];

    juce::TextButton btnDegreeLock[8];
    juce::TextButton btnOctaveLock[8];
    juce::TextButton btnDynamic[8];
    juce::Slider dynamicSliders[8];

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

    // ==========================================================
    // ファインチューニング用 UIコンポーネント
    // ==========================================================
    juce::Label tuningTempoTitle{ "", "Tempo (Min/Max/L):" };
    juce::Label tuningTsTitle{ "", "Time Sigs:" };
    juce::Label tuningFillsTitle{ "", "Fills:" };

    juce::Label tuningColDiv{ "", "Div (1-8 / L)" };
    juce::Label tuningColCmplx{ "", "Cmplx (Min/Max/L)" };
    juce::Label tuningColEntrp{ "", "Entrp (Min/Max/L)" };
    juce::Label tuningColShift{ "", "Shift (Min/Max/L)" };

    juce::Label tuningTempoMin, tuningTempoMax;
    juce::TextButton tuningTempoLock{ "L" };

    juce::TextButton btnTuningRandom{ "Random" };
    juce::TextButton btnTuningClear{ "Clear" };

    juce::ToggleButton tuningTsBtns[8];
    juce::ToggleButton tuningFillBtns[4];

    juce::TextButton tuningDivBtns[8][8];
    juce::TextButton tuningDivLock[8];

    juce::Label tuningCmplxMin[8], tuningCmplxMax[8]; juce::TextButton tuningCmplxLock[8];
    juce::Label tuningEntrpMin[8], tuningEntrpMax[8]; juce::TextButton tuningEntrpLock[8];
    juce::Label tuningShiftMin[8], tuningShiftMax[8]; juce::TextButton tuningShiftLock[8];

    void setupNumBox(juce::Label& lbl, std::function<void(int)> onChange);
    void updateTuningUIFromProcessor();
    // ==========================================================

    juce::Rectangle<int> dragAllArea;
    juce::Rectangle<int> lockArea;
    juce::Rectangle<int> sampleArea;
    juce::Rectangle<int> mainGridArea;
    juce::Rectangle<int> midiDragArea;
    bool isDragging = false;

    int dragTargetTrack = -1;
    int getTrackIndexFromMouseY(int y);

    void updateTabColors();
    void updatePatternButtonColors();
    void updateViewVisibility();
    void updateDivisionMenus();
    void updateTimeSigNumMenu();
    void updateTrackNames();
    void updateStyleMenu();

    juce::File exportMidi(int trackIndex);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AIDrumMachineAudioProcessorEditor)
};