// ==============================================================================
// Source/PluginEditor.cpp
// ==============================================================================
#include "PluginProcessor.h"
#include "PluginEditor.h"

AIDrumMachineAudioProcessorEditor::AIDrumMachineAudioProcessorEditor(AIDrumMachineAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    setSize(1000, 480);

    addAndMakeVisible(syncButton); addAndMakeVisible(playButton); addAndMakeVisible(stopButton); addAndMakeVisible(tempoLabel);
    syncButton.setToggleState(audioProcessor.isSyncEnabled.load(), juce::dontSendNotification);
    syncButton.onClick = [this] { audioProcessor.isSyncEnabled = syncButton.getToggleState(); };
    playButton.onClick = [this] { audioProcessor.isPlayingInternal = true; };
    stopButton.onClick = [this] {
        auto now = juce::Time::currentTimeMillis();
        if (now - lastStopClickTime < 500) audioProcessor.resetPosition();
        else audioProcessor.isPlayingInternal = false;
        lastStopClickTime = now;
        };
    tempoLabel.setJustificationType(juce::Justification::centred); tempoLabel.setEditable(true);
    tempoLabel.onTextChange = [this] {
        double newTempo = tempoLabel.getText().getDoubleValue();
        if (newTempo >= 20.0 && newTempo <= 999.0) audioProcessor.internalTempo = newTempo;
        };

    addAndMakeVisible(tabSeqButton); addAndMakeVisible(tabSetupButton);
    tabSeqButton.onClick = [this] { currentView = SequencerView; updateViewVisibility(); resized(); repaint(); };
    tabSetupButton.onClick = [this] { currentView = SetupView; updateViewVisibility(); resized(); repaint(); };

    addAndMakeVisible(btnClearAll);
    btnClearAll.setColour(juce::TextButton::buttonColourId, juce::Colours::red.withAlpha(0.6f));
    btnClearAll.onClick = [this] {
        juce::NativeMessageBox::showOkCancelBox(
            juce::MessageBoxIconType::WarningIcon, "Reset All", "Reset settings to genre defaults?", this,
            juce::ModalCallbackFunction::create([this](int result) {
                if (result == 1) {
                    audioProcessor.globalBarCount = 1;
                    barCountMenu.setSelectedId(1, juce::dontSendNotification);
                    currentViewBar = 0;
                    for (int i = 0; i < 8; ++i) {
                        audioProcessor.clearTrack(i);
                        audioProcessor.trackDivisionsUI[i] = 4; divSelectors[i].setSelectedId(4, juce::dontSendNotification);
                        audioProcessor.trackComplexity[i] = 50; complexitySliders[i].setValue(50.0, juce::dontSendNotification);
                        audioProcessor.trackEntropy[i] = 0; entropySliders[i].setValue(0.0, juce::dontSendNotification);
                        audioProcessor.trackShiftUI[i] = 0; shiftSliders[i].setValue(0.0, juce::dontSendNotification);
                    }
                    audioProcessor.patternUpdated.store(true);
                    updateTabColors(); updateViewVisibility(); resized(); repaint();
                }
                }));
        };

    addAndMakeVisible(timeSigLabel); timeSigLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(timeSigNumMenu);
    timeSigNumMenu.onChange = [this] {
        audioProcessor.timeSigNumerator.store(timeSigNumMenu.getSelectedId());
        audioProcessor.patternUpdated.store(true);
        resized(); repaint();
        };

    addAndMakeVisible(timeSigSlash); timeSigSlash.setColour(juce::Label::textColourId, juce::Colours::white);
    timeSigSlash.setJustificationType(juce::Justification::centred);

    addAndMakeVisible(timeSigDenMenu);
    timeSigDenMenu.addItem("4", 4); timeSigDenMenu.addItem("8", 8); timeSigDenMenu.addItem("16", 16);
    timeSigDenMenu.setSelectedId(audioProcessor.timeSigDenominator.load(), juce::dontSendNotification);
    timeSigDenMenu.onChange = [this] {
        audioProcessor.timeSigDenominator.store(timeSigDenMenu.getSelectedId());
        updateTimeSigNumMenu();
        updateDivisionMenus();
        audioProcessor.patternUpdated.store(true);
        resized(); repaint();
        };

    addAndMakeVisible(barCountLabel); barCountLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(barCountMenu);
    barCountMenu.addItem("1 Bar", 1); barCountMenu.addItem("2 Bars", 2);
    barCountMenu.addItem("3 Bars", 3); barCountMenu.addItem("4 Bars", 4);
    barCountMenu.setSelectedId(audioProcessor.globalBarCount.load(), juce::dontSendNotification);
    barCountMenu.onChange = [this] {
        audioProcessor.globalBarCount.store(barCountMenu.getSelectedId());
        if (currentViewBar >= audioProcessor.globalBarCount.load()) currentViewBar = audioProcessor.globalBarCount.load() - 1;
        audioProcessor.patternUpdated.store(true);
        updateTabColors(); updateViewVisibility(); resized(); repaint();
        };

    addAndMakeVisible(generateButton); addAndMakeVisible(styleMenu); addAndMakeVisible(statusLabel);
    const juce::StringArray genres = {
        "0. Techno (Detroit/Berlin)", "1. House (Deep/Acid)", "2. UK Garage (2-step)", "3. Drum & Bass / Jungle",
        "4. Trap", "5. Footwork / Juke", "6. IDM (Breakcore)", "7. Dubstep",
        "8. Afrobeat", "9. Gqom", "10. Amapiano", "11. Indian Classical",
        "12. Samba / Bossa Nova", "13. Reggaeton / Dembow", "14. Gamelan",
        "15. Funk (James Brown)", "16. New Jack Swing", "17. Neo Soul (J Dilla)",
        "18. Hip Hop (Boom Bap)", "19. Math Rock", "20. Progressive Metal", "21. Minimalism (Reich)",
        "22. Pure Euclidean (Math)", "23. Pure Chaos (Random)"
    };
    for (int i = 0; i < genres.size(); ++i) styleMenu.addItem(genres[i], i + 1);

    // ★ ジャンル選択時：トラック名と推奨拍子を更新する（UXの向上）
    styleMenu.onChange = [this] {
        int genreIndex = styleMenu.getSelectedId() - 1;
        audioProcessor.currentGenre.store(genreIndex);

        const auto& def = AIDrumMachineAudioProcessor::getGenreDef(genreIndex);

        // 推奨拍子の更新
        timeSigDenMenu.setSelectedId(def.defaultDen, juce::dontSendNotification);
        audioProcessor.timeSigDenominator.store(def.defaultDen);
        updateTimeSigNumMenu();
        timeSigNumMenu.setSelectedId(def.defaultNum, juce::dontSendNotification);
        audioProcessor.timeSigNumerator.store(def.defaultNum);

        updateDivisionMenus();

        // 推奨トラック名でラベルを上書き
        for (int i = 0; i < 8; ++i) {
            trackNameLabels[i].setText(def.trackNames[i], juce::dontSendNotification);
        }

        audioProcessor.patternUpdated.store(true);
        resized();
        repaint();
        };

    addAndMakeVisible(tabButton1); addAndMakeVisible(tabButton2); addAndMakeVisible(tabButton3); addAndMakeVisible(tabButton4);
    auto tabClick = [this](int barIndex) { currentViewBar = barIndex; updateTabColors(); repaint(); };
    tabButton1.onClick = [tabClick] { tabClick(0); }; tabButton2.onClick = [tabClick] { tabClick(1); };
    tabButton3.onClick = [tabClick] { tabClick(2); }; tabButton4.onClick = [tabClick] { tabClick(3); };
    updateTabColors();

    for (int i = 0; i < 8; ++i) {
        addAndMakeVisible(trackNameLabels[i]);
        trackNameLabels[i].setEditable(true); trackNameLabels[i].setJustificationType(juce::Justification::centredLeft);
        trackNameLabels[i].setColour(juce::Label::textColourId, juce::Colours::white); trackNameLabels[i].setMinimumHorizontalScale(1.0f);

        addChildComponent(midiKeyLabels[i]); midiKeyLabels[i].setText("[" + trackNotes[i] + "]", juce::dontSendNotification);
        midiKeyLabels[i].setColour(juce::Label::textColourId, juce::Colours::grey); midiKeyLabels[i].setJustificationType(juce::Justification::centredRight);

        addAndMakeVisible(btnMute[i]); addAndMakeVisible(btnSolo[i]); addAndMakeVisible(btnClear[i]); addAndMakeVisible(btnShiftL[i]); addAndMakeVisible(btnShiftR[i]);
        btnMute[i].setButtonText("M"); btnSolo[i].setButtonText("S"); btnClear[i].setButtonText("C"); btnShiftL[i].setButtonText("<"); btnShiftR[i].setButtonText(">");
        btnMute[i].setClickingTogglesState(true); btnSolo[i].setClickingTogglesState(true);
        btnMute[i].setColour(juce::TextButton::buttonOnColourId, juce::Colours::red);
        btnSolo[i].setColour(juce::TextButton::buttonOnColourId, juce::Colours::blue);

        btnMute[i].onClick = [this, i] { audioProcessor.trackMuted[i] = btnMute[i].getToggleState(); };
        btnSolo[i].onClick = [this, i] { audioProcessor.trackSoloed[i] = btnSolo[i].getToggleState(); };

        btnClear[i].onClick = [this, i] {
            juce::NativeMessageBox::showOkCancelBox(
                juce::MessageBoxIconType::WarningIcon, "Clear Track", "Clear Track " + juce::String(i + 1) + "?", this,
                juce::ModalCallbackFunction::create([this, i](int result) {
                    if (result == 1) { audioProcessor.clearTrack(i); repaint(); }
                    }));
            };

        btnShiftL[i].onClick = [this, i] { audioProcessor.shiftTrackLeft(i); repaint(); };
        btnShiftR[i].onClick = [this, i] { audioProcessor.shiftTrackRight(i); repaint(); };

        addChildComponent(divLabels[i]); divLabels[i].setText("Div:", juce::dontSendNotification);
        addChildComponent(divSelectors[i]);
        divSelectors[i].onChange = [this, i] {
            audioProcessor.trackDivisionsUI[i] = divSelectors[i].getSelectedId();
            audioProcessor.patternUpdated.store(true);
            repaint();
            };
        addChildComponent(btnDivLock[i]); btnDivLock[i].setButtonText("L"); btnDivLock[i].setClickingTogglesState(true);
        btnDivLock[i].onClick = [this, i] { audioProcessor.trackDivLocked[i] = btnDivLock[i].getToggleState(); };

        addChildComponent(compLabels[i]); compLabels[i].setText("Cmplx:", juce::dontSendNotification);
        addChildComponent(complexitySliders[i]); complexitySliders[i].setSliderStyle(juce::Slider::LinearHorizontal);
        complexitySliders[i].setTextBoxStyle(juce::Slider::TextBoxRight, false, 40, 20);
        complexitySliders[i].setRange(0.0, 100.0, 1.0); complexitySliders[i].setValue(audioProcessor.trackComplexity[i], juce::dontSendNotification);
        complexitySliders[i].onValueChange = [this, i] { audioProcessor.trackComplexity[i] = static_cast<int>(complexitySliders[i].getValue()); };
        addChildComponent(btnCmplxLock[i]); btnCmplxLock[i].setButtonText("L"); btnCmplxLock[i].setClickingTogglesState(true);
        btnCmplxLock[i].onClick = [this, i] { audioProcessor.trackCmplxLocked[i] = btnCmplxLock[i].getToggleState(); };

        addChildComponent(entrpLabels[i]); entrpLabels[i].setText("Entrp:", juce::dontSendNotification);
        addChildComponent(entropySliders[i]); entropySliders[i].setSliderStyle(juce::Slider::LinearHorizontal);
        entropySliders[i].setTextBoxStyle(juce::Slider::TextBoxRight, false, 40, 20);
        entropySliders[i].setRange(0.0, 100.0, 1.0); entropySliders[i].setValue(audioProcessor.trackEntropy[i], juce::dontSendNotification);
        entropySliders[i].onValueChange = [this, i] { audioProcessor.trackEntropy[i] = static_cast<int>(entropySliders[i].getValue()); };
        addChildComponent(btnEntrpLock[i]); btnEntrpLock[i].setButtonText("L"); btnEntrpLock[i].setClickingTogglesState(true);
        btnEntrpLock[i].onClick = [this, i] { audioProcessor.trackEntrpLocked[i] = btnEntrpLock[i].getToggleState(); };

        addChildComponent(shiftLabels[i]); shiftLabels[i].setText("Shift:", juce::dontSendNotification);
        addChildComponent(shiftSliders[i]); shiftSliders[i].setSliderStyle(juce::Slider::LinearHorizontal);
        shiftSliders[i].setTextBoxStyle(juce::Slider::TextBoxRight, false, 40, 20);
        shiftSliders[i].setRange(-50.0, 50.0, 1.0); shiftSliders[i].setValue(audioProcessor.trackShiftUI[i], juce::dontSendNotification);
        shiftSliders[i].onValueChange = [this, i] {
            audioProcessor.trackShiftUI[i] = static_cast<int>(shiftSliders[i].getValue());
            audioProcessor.patternUpdated.store(true);
            };
        addChildComponent(btnShiftLock[i]); btnShiftLock[i].setButtonText("L"); btnShiftLock[i].setClickingTogglesState(true);
        btnShiftLock[i].onClick = [this, i] { audioProcessor.trackShiftLocked[i] = btnShiftLock[i].getToggleState(); };
    }

    // 初回のジャンル反映
    styleMenu.setSelectedId(1, juce::sendNotification);

    juce::Component::SafePointer<AIDrumMachineAudioProcessorEditor> safeThis(this);
    // ★ Generateボタン: パターンの再構築のみを行う（名前や拍子は保護される）
    generateButton.onClick = [safeThis, this] {
        if (safeThis == nullptr) return;
        audioProcessor.generateAllTracks();
        currentViewBar = 0;
        updateTabColors();
        resized();
        repaint();
        };
    updateViewVisibility(); startTimerHz(30);
}

AIDrumMachineAudioProcessorEditor::~AIDrumMachineAudioProcessorEditor() { stopTimer(); }

void AIDrumMachineAudioProcessorEditor::updateTimeSigNumMenu() {
    int den = audioProcessor.timeSigDenominator.load();
    int maxNum = 7;
    if (den == 8) maxNum = 9;
    else if (den == 16) maxNum = 17;

    int currentNum = audioProcessor.timeSigNumerator.load();
    if (currentNum > maxNum) {
        currentNum = maxNum;
        audioProcessor.timeSigNumerator.store(currentNum);
    }

    timeSigNumMenu.clear();
    for (int i = 1; i <= maxNum; ++i) {
        timeSigNumMenu.addItem(juce::String(i), i);
    }
    timeSigNumMenu.setSelectedId(currentNum, juce::dontSendNotification);
}

void AIDrumMachineAudioProcessorEditor::updateDivisionMenus() {
    int den = audioProcessor.timeSigDenominator.load();
    int maxDiv = (den == 16) ? 2 : ((den == 8) ? 4 : 8);

    for (int i = 0; i < 8; ++i) {
        divSelectors[i].clear();
        for (int d = 1; d <= maxDiv; ++d) {
            divSelectors[i].addItem(juce::String(d), d);
        }
        if (audioProcessor.trackDivisionsUI[i] > maxDiv) {
            audioProcessor.trackDivisionsUI[i] = maxDiv;
        }
        divSelectors[i].setSelectedId(audioProcessor.trackDivisionsUI[i], juce::dontSendNotification);
    }
}

void AIDrumMachineAudioProcessorEditor::updateViewVisibility() {
    bool isSeq = (currentView == SequencerView); btnClearAll.setVisible(isSeq);
    for (int i = 0; i < 8; ++i) {
        btnMute[i].setVisible(isSeq); btnSolo[i].setVisible(isSeq); btnClear[i].setVisible(isSeq); btnShiftL[i].setVisible(isSeq); btnShiftR[i].setVisible(isSeq);

        divLabels[i].setVisible(!isSeq); divSelectors[i].setVisible(!isSeq); btnDivLock[i].setVisible(!isSeq);
        compLabels[i].setVisible(!isSeq); complexitySliders[i].setVisible(!isSeq); btnCmplxLock[i].setVisible(!isSeq);
        entrpLabels[i].setVisible(!isSeq); entropySliders[i].setVisible(!isSeq); btnEntrpLock[i].setVisible(!isSeq);
        shiftLabels[i].setVisible(!isSeq); shiftSliders[i].setVisible(!isSeq); btnShiftLock[i].setVisible(!isSeq);

        midiKeyLabels[i].setVisible(!isSeq);
    }
}
void AIDrumMachineAudioProcessorEditor::updateTabColors() {
    tabButton1.setColour(juce::TextButton::buttonColourId, currentViewBar == 0 ? juce::Colours::orange : juce::Colours::darkgrey);
    tabButton2.setColour(juce::TextButton::buttonColourId, currentViewBar == 1 ? juce::Colours::orange : juce::Colours::darkgrey);
    tabButton3.setColour(juce::TextButton::buttonColourId, currentViewBar == 2 ? juce::Colours::orange : juce::Colours::darkgrey);
    tabButton4.setColour(juce::TextButton::buttonColourId, currentViewBar == 3 ? juce::Colours::orange : juce::Colours::darkgrey);
}
void AIDrumMachineAudioProcessorEditor::timerCallback() {
    if (audioProcessor.isSyncEnabled.load()) tempoLabel.setText("DAW: " + juce::String(audioProcessor.currentBpm.load(), 1) + " BPM", juce::dontSendNotification);
    else if (!tempoLabel.isBeingEdited()) tempoLabel.setText(juce::String(audioProcessor.internalTempo.load(), 1) + " BPM", juce::dontSendNotification);

    // Processor側でのパラメータ変更をUIに同期
    if (audioProcessor.uiNeedsUpdate.exchange(false)) {
        for (int i = 0; i < 8; ++i) {
            divSelectors[i].setSelectedId(audioProcessor.trackDivisionsUI[i], juce::dontSendNotification);
            complexitySliders[i].setValue(audioProcessor.trackComplexity[i], juce::dontSendNotification);
            entropySliders[i].setValue(audioProcessor.trackEntropy[i], juce::dontSendNotification);
            shiftSliders[i].setValue(audioProcessor.trackShiftUI[i], juce::dontSendNotification);
        }
        resized();
    }

    repaint();
}

void AIDrumMachineAudioProcessorEditor::resized() {
    auto area = getLocalBounds().reduced(20);

    auto row1 = area.removeFromTop(30);
    syncButton.setBounds(row1.removeFromLeft(60)); playButton.setBounds(row1.removeFromLeft(60).reduced(2)); stopButton.setBounds(row1.removeFromLeft(60).reduced(2));
    row1.removeFromLeft(10); tempoLabel.setBounds(row1.removeFromLeft(80));
    row1.removeFromLeft(20); tabSeqButton.setBounds(row1.removeFromLeft(90).reduced(2)); tabSetupButton.setBounds(row1.removeFromLeft(90).reduced(2));
    row1.removeFromLeft(10); btnClearAll.setBounds(row1.removeFromLeft(80).reduced(2));

    row1.removeFromLeft(10);
    timeSigLabel.setBounds(row1.removeFromLeft(60));
    timeSigNumMenu.setBounds(row1.removeFromLeft(60).reduced(2));
    timeSigSlash.setBounds(row1.removeFromLeft(15));
    timeSigDenMenu.setBounds(row1.removeFromLeft(60).reduced(2));

    dragAllArea = row1.removeFromRight(120).toFloat();

    area.removeFromTop(10);
    auto row2 = area.removeFromTop(30);
    generateButton.setBounds(row2.removeFromLeft(120)); row2.removeFromLeft(10); styleMenu.setBounds(row2.removeFromLeft(200)); row2.removeFromLeft(10);

    row2.removeFromRight(10);
    barCountMenu.setBounds(row2.removeFromRight(80).reduced(2));
    barCountLabel.setBounds(row2.removeFromRight(40));

    statusLabel.setBounds(row2);

    auto tabArea = area.removeFromTop(40).withTrimmedTop(10); int tabW = tabArea.getWidth() / 4;

    int bars = audioProcessor.globalBarCount.load();
    tabButton1.setVisible(bars >= 1);
    tabButton2.setVisible(bars >= 2);
    tabButton3.setVisible(bars >= 3);
    tabButton4.setVisible(bars >= 4);

    if (bars >= 1) tabButton1.setBounds(tabArea.removeFromLeft(tabW).reduced(2));
    if (bars >= 2) tabButton2.setBounds(tabArea.removeFromLeft(tabW).reduced(2));
    if (bars >= 3) tabButton3.setBounds(tabArea.removeFromLeft(tabW).reduced(2));
    if (bars >= 4) tabButton4.setBounds(tabArea.removeFromLeft(tabW).reduced(2));

    auto bottomArea = area.removeFromBottom(250).toFloat();
    if (currentView == SequencerView) {
        lockArea = bottomArea.removeFromLeft(30); auto controlArea = bottomArea.removeFromLeft(150); sampleArea = bottomArea.removeFromLeft(60);
        int rows = 8; float cellH = sampleArea.getHeight() / rows;
        for (int row = 0; row < rows; ++row) {
            int idx = 7 - row;
            trackNameLabels[idx].setBounds(sampleArea.withY(sampleArea.getY() + row * cellH).withHeight(cellH).removeFromLeft(50).reduced(2).toNearestInt());
            auto ctrlRow = controlArea.withY(controlArea.getY() + row * cellH).withHeight(cellH).reduced(1);
            btnMute[idx].setBounds(ctrlRow.removeFromLeft(30).reduced(1).toNearestInt()); btnSolo[idx].setBounds(ctrlRow.removeFromLeft(30).reduced(1).toNearestInt()); btnClear[idx].setBounds(ctrlRow.removeFromLeft(30).reduced(1).toNearestInt()); btnShiftL[idx].setBounds(ctrlRow.removeFromLeft(30).reduced(1).toNearestInt()); btnShiftR[idx].setBounds(ctrlRow.removeFromLeft(30).reduced(1).toNearestInt());
        }
        midiDragArea = bottomArea.removeFromRight(40); mainGridArea = bottomArea;
    }
    else {
        lockArea = juce::Rectangle<float>(); midiDragArea = bottomArea.removeFromRight(40);
        auto labelArea = bottomArea.removeFromLeft(120);
        sampleArea = bottomArea.removeFromLeft(110);

        auto setupControls = bottomArea;
        int rows = 8; float cellH = sampleArea.getHeight() / rows;
        for (int row = 0; row < rows; ++row) {
            int idx = 7 - row;
            auto lRow = labelArea.withY(labelArea.getY() + row * cellH).withHeight(cellH).reduced(2);
            midiKeyLabels[idx].setBounds(lRow.removeFromLeft(40).toNearestInt()); trackNameLabels[idx].setBounds(lRow.toNearestInt());

            auto ctrlRow = setupControls.withY(setupControls.getY() + row * cellH).withHeight(cellH).reduced(2);

            ctrlRow.removeFromLeft(10);
            divLabels[idx].setBounds(ctrlRow.removeFromLeft(30).toNearestInt());
            divSelectors[idx].setBounds(ctrlRow.removeFromLeft(55).toNearestInt());
            ctrlRow.removeFromLeft(5);
            btnDivLock[idx].setBounds(ctrlRow.removeFromLeft(20).reduced(1).toNearestInt());

            float remainingWidth = ctrlRow.getWidth();
            float blockWidth = remainingWidth / 3.0f;

            auto cmplxBlock = ctrlRow.removeFromLeft(blockWidth);
            cmplxBlock.removeFromLeft(15);
            compLabels[idx].setBounds(cmplxBlock.removeFromLeft(45).toNearestInt());
            btnCmplxLock[idx].setBounds(cmplxBlock.removeFromRight(20).reduced(1).toNearestInt());
            cmplxBlock.removeFromRight(5);
            complexitySliders[idx].setBounds(cmplxBlock.toNearestInt());

            auto entrpBlock = ctrlRow.removeFromLeft(blockWidth);
            entrpBlock.removeFromLeft(15);
            entrpLabels[idx].setBounds(entrpBlock.removeFromLeft(45).toNearestInt());
            btnEntrpLock[idx].setBounds(entrpBlock.removeFromRight(20).reduced(1).toNearestInt());
            entrpBlock.removeFromRight(5);
            entropySliders[idx].setBounds(entrpBlock.toNearestInt());

            auto shiftBlock = ctrlRow;
            shiftBlock.removeFromLeft(15);
            shiftLabels[idx].setBounds(shiftBlock.removeFromLeft(40).toNearestInt());
            btnShiftLock[idx].setBounds(shiftBlock.removeFromRight(20).reduced(1).toNearestInt());
            shiftBlock.removeFromRight(5);
            shiftSliders[idx].setBounds(shiftBlock.toNearestInt());
        }
        mainGridArea = juce::Rectangle<float>();
    }
}

void AIDrumMachineAudioProcessorEditor::paint(juce::Graphics& g) {
    g.fillAll(juce::Colours::darkgrey);
    g.setColour(juce::Colours::cyan.withAlpha(0.6f)); g.fillRoundedRectangle(dragAllArea, 4.0f);
    g.setColour(juce::Colours::white); g.setFont(14.0f); g.drawText("DRAG ALL MIDI", dragAllArea, juce::Justification::centred, false);

    if (currentView == SequencerView) {
        int rows = 8; float cellH = mainGridArea.getHeight() / rows;
        int numBeats = audioProcessor.timeSigNumerator.load();

        for (int row = 0; row < rows; ++row) {
            int idx = 7 - row;

            juce::Rectangle<float> lockBtn(lockArea.getX(), lockArea.getY() + row * cellH, lockArea.getWidth() - 2.0f, cellH - 2.0f);
            g.setColour(audioProcessor.trackLocked[idx] ? juce::Colours::red.withAlpha(0.8f) : juce::Colours::grey.withAlpha(0.5f));
            g.fillRoundedRectangle(lockBtn, 4.0f);
            g.setColour(juce::Colours::white); g.setFont(10.0f);
            g.drawText("L", lockBtn, juce::Justification::centred, false);

            juce::Rectangle<float> mDrag(midiDragArea.getX() + 4.0f, midiDragArea.getY() + row * cellH, midiDragArea.getWidth() - 4.0f, cellH - 2.0f);
            g.setColour(juce::Colours::cyan.withAlpha(0.3f));
            g.fillRoundedRectangle(mDrag, 4.0f);
            g.setColour(juce::Colours::white); g.setFont(11.0f);
            g.drawText("MIDI", mDrag, juce::Justification::centred, false);

            int div = audioProcessor.trackDivisionsUI[idx];
            int colsToDraw = numBeats * div;
            float cellW = mainGridArea.getWidth() / (float)colsToDraw;

            for (int b = 0; b < numBeats; ++b) {
                g.setColour(b % 2 == 0 ? juce::Colours::black.withAlpha(0.15f) : juce::Colours::white.withAlpha(0.05f));
                g.fillRect(mainGridArea.getX() + b * div * cellW, mainGridArea.getY() + row * cellH, div * cellW, cellH);
            }

            for (int col = 0; col < colsToDraw; ++col) {
                int globalStep = (currentViewBar * colsToDraw) + col;
                int vel = audioProcessor.drumPatternUI[idx][globalStep];
                juce::Rectangle<float> cell(mainGridArea.getX() + col * cellW + 1, mainGridArea.getY() + row * cellH + 1, cellW - 2, cellH - 2);
                float alpha = juce::jlimit(0.0f, 1.0f, 0.2f + 0.8f * (vel / 100.0f));
                g.setColour(vel > 0 ? juce::Colours::orange.withAlpha(alpha) : juce::Colours::black.withAlpha(0.3f));
                g.fillRoundedRectangle(cell, 2.0f);

                if (col % div == 0 && col > 0) {
                    g.setColour(juce::Colours::white.withAlpha(0.4f));
                    g.drawVerticalLine((int)(cell.getX() - 1), cell.getY(), cell.getBottom());
                }
            }

            int cur = audioProcessor.getTrackCurrentStep(idx);
            int startStep = currentViewBar * colsToDraw;
            if (cur >= startStep && cur < startStep + colsToDraw) {
                g.setColour(juce::Colours::white.withAlpha(0.3f));
                g.fillRect(mainGridArea.getX() + (cur - startStep) * cellW, mainGridArea.getY() + row * cellH, cellW, cellH);
            }
        }
    }
    else {
        int rows = 8; float cellH = sampleArea.getHeight() / rows;
        for (int row = 0; row < rows; ++row) {
            int idx = 7 - row; juce::Rectangle<float> sArea(sampleArea.getX(), sampleArea.getY() + row * cellH + 2, sampleArea.getWidth() - 4, cellH - 4);
            g.setColour(audioProcessor.hasSampleLoaded(idx) ? juce::Colours::lightblue.withAlpha(0.2f) : juce::Colours::darkgrey.darker());
            g.fillRoundedRectangle(sArea, 4.0f); g.setColour(juce::Colours::grey); g.drawText(audioProcessor.hasSampleLoaded(idx) ? "Sample Loaded" : "Drop Sample", sArea, juce::Justification::centred);
        }
    }
}

void AIDrumMachineAudioProcessorEditor::mouseDown(const juce::MouseEvent& e) {
    auto pos = e.getPosition().toFloat();
    if (e.mods.isRightButtonDown() && sampleArea.contains(pos)) {
        int idx = 7 - (int)((pos.y - sampleArea.getY()) / (sampleArea.getHeight() / 8));
        if (idx >= 0 && audioProcessor.hasSampleLoaded(idx)) {
            juce::NativeMessageBox::showOkCancelBox(juce::MessageBoxIconType::WarningIcon, "Delete", "Delete sample?", this, juce::ModalCallbackFunction::create([this, idx](int r) { if (r == 1) { audioProcessor.clearSample(idx); trackNameLabels[idx].setText("Track " + juce::String(idx + 1), juce::dontSendNotification); repaint(); } }));
        }
        return;
    }

    if (currentView == SequencerView) {
        if (lockArea.contains(pos)) {
            int row = (int)((pos.y - lockArea.getY()) / (lockArea.getHeight() / 8));
            if (row >= 0 && row < 8) {
                int idx = 7 - row;
                audioProcessor.trackLocked[idx] = !audioProcessor.trackLocked[idx];
                repaint();
            }
        }
        else if (mainGridArea.contains(pos)) {
            int idx = 7 - (int)((pos.y - mainGridArea.getY()) / (mainGridArea.getHeight() / 8));
            int div = audioProcessor.trackDivisionsUI[idx];
            int numBeats = audioProcessor.timeSigNumerator.load();
            float cellW = mainGridArea.getWidth() / (float)(div * numBeats);
            int col = (int)((pos.x - mainGridArea.getX()) / cellW);
            int globalStep = (currentViewBar * div * numBeats) + col;
            if (globalStep < 1024) {
                audioProcessor.drumPatternUI[idx][globalStep] = (audioProcessor.drumPatternUI[idx][globalStep] > 0) ? 0 : 100;
                audioProcessor.patternUpdated.store(true);
                repaint();
            }
        }
    }
}

void AIDrumMachineAudioProcessorEditor::mouseDrag(const juce::MouseEvent& e) {
    if (e.mouseWasDraggedSinceMouseDown() && !isDragging) {
        isDragging = true; auto startPos = e.getMouseDownPosition().toFloat(); int track = -2;
        if (dragAllArea.contains(startPos)) track = -1;
        else if (midiDragArea.contains(startPos)) track = 7 - (int)((startPos.y - midiDragArea.getY()) / (midiDragArea.getHeight() / 8));
        if (track != -2) {
            juce::File f = exportMidi(track);
            if (f.existsAsFile()) { juce::StringArray s; s.add(f.getFullPathName()); performExternalDragDropOfFiles(s, false, this); }
        }
    }
}
void AIDrumMachineAudioProcessorEditor::mouseUp(const juce::MouseEvent& e) { isDragging = false; }
bool AIDrumMachineAudioProcessorEditor::isInterestedInFileDrag(const juce::StringArray& files) { for (const auto& f : files) if (f.endsWithIgnoreCase(".wav") || f.endsWithIgnoreCase(".mp3") || f.endsWithIgnoreCase(".aif")) return true; return false; }
void AIDrumMachineAudioProcessorEditor::filesDropped(const juce::StringArray& files, int x, int y) {
    int idx = 7 - (int)((y - sampleArea.getY()) / (sampleArea.getHeight() / 8));
    if (idx >= 0 && idx < 8) { for (const auto& f : files) { audioProcessor.loadSample(idx, f); trackNameLabels[idx].setText(juce::File(f).getFileNameWithoutExtension(), juce::dontSendNotification); repaint(); break; } }
}
void AIDrumMachineAudioProcessorEditor::fileDragEnter(const juce::StringArray& f, int x, int y) {} void AIDrumMachineAudioProcessorEditor::fileDragMove(const juce::StringArray& f, int x, int y) {} void AIDrumMachineAudioProcessorEditor::fileDragExit(const juce::StringArray& f) {}
int AIDrumMachineAudioProcessorEditor::getTrackIndexFromMouseY(int y) { return 7 - (int)((y - sampleArea.getY()) / (sampleArea.getHeight() / 8)); }

juce::File AIDrumMachineAudioProcessorEditor::exportMidi(int trackIndex) {
    juce::MidiMessageSequence seq; int notes[8] = { 36, 38, 42, 46, 39, 41, 45, 50 };
    int startT = (trackIndex == -1) ? 0 : trackIndex; int endT = (trackIndex == -1) ? 7 : trackIndex;
    double ppq = 960.0;
    int den = audioProcessor.timeSigDenominator.load();
    double ticksPerBeat = ppq * (4.0 / (double)den);

    for (int t = startT; t <= endT; ++t) {
        int div = audioProcessor.trackDivisionsUI[t];
        double ticksPerStep = ticksPerBeat / (double)div;
        double shiftOffsetTicks = (audioProcessor.trackShiftUI[t] / 100.0) * ticksPerStep;
        int totalSteps = div * audioProcessor.timeSigNumerator.load() * audioProcessor.globalBarCount.load();

        for (int s = 0; s < totalSteps; ++s) {
            int vel = audioProcessor.drumPatternUI[t][s];
            if (vel > 0) {
                double start = (s * ticksPerStep) + shiftOffsetTicks;
                if (start < 0.0) start = 0.0;
                seq.addEvent(juce::MidiMessage::noteOn(10, notes[t], (juce::uint8)((vel / 100.0f) * 127)), start);
                seq.addEvent(juce::MidiMessage::noteOff(10, notes[t]), start + ticksPerStep * 0.5);
            }
        }
    }
    seq.updateMatchedPairs(); juce::MidiFile mf; mf.setTicksPerQuarterNote((short)ppq); mf.addTrack(seq);
    juce::File f = juce::File::getSpecialLocation(juce::File::tempDirectory).getChildFile("DrumPattern.mid");
    f.deleteFile(); juce::FileOutputStream os(f); mf.writeTo(os); os.flush(); return f;
}