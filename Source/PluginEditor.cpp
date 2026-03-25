#include "PluginProcessor.h"
#include "PluginEditor.h"

AIDrumMachineAudioProcessorEditor::AIDrumMachineAudioProcessorEditor(AIDrumMachineAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    setSize(1000, 460);

    addAndMakeVisible(syncButton);
    addAndMakeVisible(playButton);
    addAndMakeVisible(stopButton);
    addAndMakeVisible(tempoLabel);

    syncButton.setToggleState(audioProcessor.isSyncEnabled.load(), juce::dontSendNotification);
    syncButton.onClick = [this] { audioProcessor.isSyncEnabled = syncButton.getToggleState(); };
    playButton.onClick = [this] { audioProcessor.isPlayingInternal = true; };
    stopButton.onClick = [this] {
        auto now = juce::Time::currentTimeMillis();
        if (now - lastStopClickTime < 500) audioProcessor.resetPosition();
        else audioProcessor.isPlayingInternal = false;
        lastStopClickTime = now;
        };
    tempoLabel.setJustificationType(juce::Justification::centred);
    tempoLabel.setEditable(true);
    tempoLabel.onTextChange = [this] {
        double newTempo = tempoLabel.getText().getDoubleValue();
        if (newTempo >= 20.0 && newTempo <= 999.0) audioProcessor.internalTempo = newTempo;
        };

    addAndMakeVisible(tabSeqButton);
    addAndMakeVisible(tabSetupButton);
    tabSeqButton.onClick = [this] { currentView = SequencerView; updateViewVisibility(); resized(); repaint(); };
    tabSetupButton.onClick = [this] { currentView = SetupView; updateViewVisibility(); resized(); repaint(); };

    addAndMakeVisible(btnClearAll);
    btnClearAll.setColour(juce::TextButton::buttonColourId, juce::Colours::red.withAlpha(0.6f));
    btnClearAll.onClick = [this] {
        juce::NativeMessageBox::showOkCancelBox(juce::MessageBoxIconType::WarningIcon, "Clear All", "Do you really want to clear ALL tracks?", this,
            juce::ModalCallbackFunction::create([this](int result) { if (result == 1) { for (int i = 0; i < 8; ++i) audioProcessor.clearTrack(i); repaint(); } }));
        };

    addAndMakeVisible(generateButton);
    addAndMakeVisible(styleMenu);
    addAndMakeVisible(statusLabel);

    styleMenu.addItem("Chaotic Polyrhythm (Div 1-9)", 1);
    styleMenu.addItem("Standard Techno (Div 4 Only)", 2);
    styleMenu.addItem("Offline Random (No API)", 3);
    styleMenu.setSelectedId(3);

    addAndMakeVisible(tabButton1); addAndMakeVisible(tabButton2);
    addAndMakeVisible(tabButton3); addAndMakeVisible(tabButton4);

    statusLabel.setJustificationType(juce::Justification::centredLeft);
    statusLabel.setText("Select style & Click Generate", juce::dontSendNotification);

    auto tabClick = [this](int barIndex) { currentViewBar = barIndex; updateTabColors(); repaint(); };
    tabButton1.onClick = [tabClick] { tabClick(0); }; tabButton2.onClick = [tabClick] { tabClick(1); };
    tabButton3.onClick = [tabClick] { tabClick(2); }; tabButton4.onClick = [tabClick] { tabClick(3); };

    updateTabColors();

    // ★追加：グローバルな小節数メニューの設定
    addChildComponent(barCountLabel);
    barCountLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    addChildComponent(barCountMenu);
    barCountMenu.addItem("1 Bar", 1); barCountMenu.addItem("2 Bars", 2);
    barCountMenu.addItem("3 Bars", 3); barCountMenu.addItem("4 Bars", 4);
    barCountMenu.setSelectedId(audioProcessor.globalBarCount);
    barCountMenu.onChange = [this] {
        audioProcessor.globalBarCount = barCountMenu.getSelectedId();
        repaint();
        };

    juce::String defaultNames[8] = { "Kick", "Snare", "CHH", "OHH", "Clap", "L.Tom", "M.Tom", "H.Tom" };
    for (int i = 0; i < 8; ++i) {
        addAndMakeVisible(trackNameLabels[i]);
        trackNameLabels[i].setText(defaultNames[i], juce::dontSendNotification);
        trackNameLabels[i].setEditable(true);
        trackNameLabels[i].setJustificationType(juce::Justification::centredLeft);
        trackNameLabels[i].setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);
        trackNameLabels[i].setColour(juce::Label::textColourId, juce::Colours::white);
        trackNameLabels[i].setColour(juce::Label::textWhenEditingColourId, juce::Colours::orange);

        // Sequencer UI
        addAndMakeVisible(btnMute[i]); btnMute[i].setButtonText("M");
        addAndMakeVisible(btnSolo[i]); btnSolo[i].setButtonText("S");
        addAndMakeVisible(btnClear[i]); btnClear[i].setButtonText("C");
        addAndMakeVisible(btnShiftL[i]); btnShiftL[i].setButtonText("<");
        addAndMakeVisible(btnShiftR[i]); btnShiftR[i].setButtonText(">");

        btnMute[i].setClickingTogglesState(true);
        btnMute[i].setColour(juce::TextButton::buttonOnColourId, juce::Colours::red);
        btnMute[i].onClick = [this, i] { audioProcessor.trackMuted[i] = btnMute[i].getToggleState(); };

        btnSolo[i].setClickingTogglesState(true);
        btnSolo[i].setColour(juce::TextButton::buttonOnColourId, juce::Colours::blue);
        btnSolo[i].onClick = [this, i] { audioProcessor.trackSoloed[i] = btnSolo[i].getToggleState(); };

        btnClear[i].onClick = [this, i] {
            juce::NativeMessageBox::showOkCancelBox(juce::MessageBoxIconType::WarningIcon, "Clear Track", "Clear Track " + juce::String(i + 1) + "?", this,
                juce::ModalCallbackFunction::create([this, i](int result) { if (result == 1) { audioProcessor.clearTrack(i); repaint(); } }));
            };

        btnShiftL[i].onClick = [this, i] { audioProcessor.shiftTrackLeft(i); repaint(); };
        btnShiftR[i].onClick = [this, i] { audioProcessor.shiftTrackRight(i); repaint(); };

        // ★追加：Setup UI (Division & Complexity)
        addChildComponent(divLabels[i]);
        divLabels[i].setText("Div:", juce::dontSendNotification);
        divLabels[i].setColour(juce::Label::textColourId, juce::Colours::lightgrey);

        addChildComponent(divSelectors[i]);
        for (int d = 1; d <= 9; ++d) divSelectors[i].addItem(juce::String(d), d);
        divSelectors[i].setSelectedId(audioProcessor.trackDivisions[i]);
        divSelectors[i].onChange = [this, i] {
            audioProcessor.trackDivisions[i] = divSelectors[i].getSelectedId();
            repaint();
            };

        addChildComponent(compLabels[i]);
        compLabels[i].setText("Cmplx:", juce::dontSendNotification);
        compLabels[i].setColour(juce::Label::textColourId, juce::Colours::lightgrey);

        addChildComponent(complexitySliders[i]);
        complexitySliders[i].setSliderStyle(juce::Slider::LinearHorizontal);
        complexitySliders[i].setTextBoxStyle(juce::Slider::TextBoxRight, false, 40, 20);
        complexitySliders[i].setRange(0.0, 100.0, 1.0);
        complexitySliders[i].setValue(audioProcessor.trackComplexity[i]);
        complexitySliders[i].onValueChange = [this, i] {
            audioProcessor.trackComplexity[i] = static_cast<int>(complexitySliders[i].getValue());
            };
    }

    juce::Component::SafePointer<AIDrumMachineAudioProcessorEditor> safeThis(this);

    // AI/ランダム処理（既存のまま）
    generateButton.onClick = [safeThis, this] {
        if (safeThis == nullptr) return;
        if (styleMenu.getSelectedId() == 3) {
            statusLabel.setText("SUCCESS! Offline Random generated.", juce::dontSendNotification);
            juce::Random random;
            for (int i = 0; i < 8; ++i) {
                if (safeThis->audioProcessor.trackLocked[i]) continue;
                int div = random.nextInt(juce::Range<int>(1, 10));
                safeThis->audioProcessor.trackDivisions[i] = div;
                safeThis->divSelectors[i].setSelectedId(div, juce::dontSendNotification); // Setup画面も同期
                int totalSteps = div * 4;
                for (int j = 0; j < 36; ++j) {
                    if (j < totalSteps) {
                        if (random.nextFloat() > 0.7f) safeThis->audioProcessor.drumPattern[i][j] = random.nextInt(juce::Range<int>(80, 101));
                        else safeThis->audioProcessor.drumPattern[i][j] = 0;
                    }
                    else safeThis->audioProcessor.drumPattern[i][j] = 0;
                }
            }
            safeThis->currentViewBar = 0; safeThis->updateTabColors(); safeThis->resized(); safeThis->repaint();
        }
        else {
            statusLabel.setText("Requesting Gemini (4 Bars)...", juce::dontSendNotification);
            juce::String myKey = "AIzaSyBT2vQXyacUMdmNOF2OjkYYQ_OPtJgORtQ";
            juce::String userPrompt = (styleMenu.getSelectedId() == 1) ? "Generate chaotic polyrhythmic techno." : "Generate standard 4-on-the-floor techno. division MUST be 4.";
            gemini.fetchDrumPattern(userPrompt, myKey);
        }
        };

    gemini.onSuccess = [safeThis, this](const juce::var& data) {
        if (safeThis == nullptr) return;
        safeThis->statusLabel.setText("SUCCESS! Rhythm received.", juce::dontSendNotification);
        for (int i = 0; i < 8; ++i) {
            if (safeThis->audioProcessor.trackLocked[i]) continue;
            juce::Identifier trackKey("track" + juce::String(i + 1));
            if (data.hasProperty(trackKey)) {
                auto trackObj = data[trackKey];
                if (trackObj.hasProperty("division") && trackObj.hasProperty("pattern")) {
                    int div = static_cast<int>(trackObj["division"]);
                    if (div < 1) div = 1; if (div > 9) div = 9;
                    safeThis->audioProcessor.trackDivisions[i] = div;
                    safeThis->divSelectors[i].setSelectedId(div, juce::dontSendNotification); // UI同期
                    auto* patternArray = trackObj["pattern"].getArray();
                    if (patternArray != nullptr) {
                        int totalSteps = div * 4;
                        for (int j = 0; j < 36; ++j) {
                            if (j < totalSteps && j < patternArray->size()) safeThis->audioProcessor.drumPattern[i][j] = static_cast<int>(patternArray->getReference(j));
                            else safeThis->audioProcessor.drumPattern[i][j] = 0;
                        }
                    }
                }
            }
        }
        safeThis->currentViewBar = 0; safeThis->updateTabColors(); safeThis->resized(); safeThis->repaint();
        };
    gemini.onError = [safeThis](const juce::String& err) { if (safeThis != nullptr) safeThis->statusLabel.setText("Error: " + err, juce::dontSendNotification); };

    updateViewVisibility();
    startTimerHz(30);
}

AIDrumMachineAudioProcessorEditor::~AIDrumMachineAudioProcessorEditor() { stopTimer(); }

void AIDrumMachineAudioProcessorEditor::updateViewVisibility()
{
    tabSeqButton.setColour(juce::TextButton::buttonColourId, currentView == SequencerView ? juce::Colours::orange : juce::Colours::darkgrey);
    tabSetupButton.setColour(juce::TextButton::buttonColourId, currentView == SetupView ? juce::Colours::orange : juce::Colours::darkgrey);

    bool isSeq = (currentView == SequencerView);
    btnClearAll.setVisible(isSeq);

    barCountLabel.setVisible(!isSeq);
    barCountMenu.setVisible(!isSeq);

    for (int i = 0; i < 8; ++i) {
        // Sequencer UI
        btnMute[i].setVisible(isSeq); btnSolo[i].setVisible(isSeq);
        btnClear[i].setVisible(isSeq); btnShiftL[i].setVisible(isSeq); btnShiftR[i].setVisible(isSeq);

        // Setup UI
        divLabels[i].setVisible(!isSeq); divSelectors[i].setVisible(!isSeq);
        compLabels[i].setVisible(!isSeq); complexitySliders[i].setVisible(!isSeq);
    }
}

bool AIDrumMachineAudioProcessorEditor::needsPagination() const {
    for (int i = 0; i < 8; ++i) if (audioProcessor.trackDivisions[i] >= 5) return true;
    return false;
}

void AIDrumMachineAudioProcessorEditor::updateTabColors() {
    tabButton1.setColour(juce::TextButton::buttonColourId, currentViewBar == 0 ? juce::Colours::orange : juce::Colours::darkgrey);
    tabButton2.setColour(juce::TextButton::buttonColourId, currentViewBar == 1 ? juce::Colours::orange : juce::Colours::darkgrey);
    tabButton3.setColour(juce::TextButton::buttonColourId, currentViewBar == 2 ? juce::Colours::orange : juce::Colours::darkgrey);
    tabButton4.setColour(juce::TextButton::buttonColourId, currentViewBar == 3 ? juce::Colours::orange : juce::Colours::darkgrey);
}

void AIDrumMachineAudioProcessorEditor::timerCallback() {
    if (audioProcessor.isSyncEnabled.load()) {
        playButton.setEnabled(false); stopButton.setEnabled(false); tempoLabel.setEditable(false);
        tempoLabel.setText("DAW: " + juce::String(audioProcessor.currentBpm.load(), 1) + " BPM", juce::dontSendNotification);
    }
    else {
        playButton.setEnabled(true); stopButton.setEnabled(true); tempoLabel.setEditable(true);
        if (!tempoLabel.isBeingEdited()) tempoLabel.setText(juce::String(audioProcessor.internalTempo.load(), 1) + " BPM", juce::dontSendNotification);
    }
    repaint();
}

void AIDrumMachineAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced(20);

    auto row1 = area.removeFromTop(30);
    syncButton.setBounds(row1.removeFromLeft(60));
    playButton.setBounds(row1.removeFromLeft(60).reduced(2));
    stopButton.setBounds(row1.removeFromLeft(60).reduced(2));
    row1.removeFromLeft(10);
    tempoLabel.setBounds(row1.removeFromLeft(80));

    row1.removeFromLeft(20);
    tabSeqButton.setBounds(row1.removeFromLeft(100).reduced(2));
    tabSetupButton.setBounds(row1.removeFromLeft(100).reduced(2));
    row1.removeFromLeft(20);
    btnClearAll.setBounds(row1.removeFromLeft(100).reduced(2));

    dragAllArea = row1.removeFromLeft(140).toFloat();

    area.removeFromTop(10);
    auto row2 = area.removeFromTop(30);
    generateButton.setBounds(row2.removeFromLeft(120));
    row2.removeFromLeft(10);
    styleMenu.setBounds(row2.removeFromLeft(200));
    row2.removeFromLeft(10);
    statusLabel.setBounds(row2);

    bool paginate = needsPagination();
    tabButton1.setVisible(paginate); tabButton2.setVisible(paginate);
    tabButton3.setVisible(paginate); tabButton4.setVisible(paginate);

    if (paginate) {
        auto tabArea = area.removeFromTop(40).withTrimmedTop(10);
        int tabW = tabArea.getWidth() / 4;
        tabButton1.setBounds(tabArea.removeFromLeft(tabW).reduced(2));
        tabButton2.setBounds(tabArea.removeFromLeft(tabW).reduced(2));
        tabButton3.setBounds(tabArea.removeFromLeft(tabW).reduced(2));
        tabButton4.setBounds(tabArea.removeFromLeft(tabW).reduced(2));
    }

    auto bottomArea = area.removeFromBottom(250).toFloat();

    if (currentView == SequencerView) {
        lockArea = bottomArea.removeFromLeft(30);
        auto controlArea = bottomArea.removeFromLeft(150);
        sampleArea = bottomArea.removeFromLeft(60);

        int rows = 8; float cellH = sampleArea.getHeight() / rows;
        for (int row = 0; row < rows; ++row) {
            int trackIndex = (rows - 1) - row;
            juce::Rectangle<float> rowArea(sampleArea.getX(), sampleArea.getY() + row * cellH, sampleArea.getWidth(), cellH);
            trackNameLabels[trackIndex].setBounds(rowArea.removeFromLeft(50).reduced(2).toNearestInt());

            auto ctrlRow = controlArea.withY(controlArea.getY() + row * cellH).withHeight(cellH).reduced(1);
            btnMute[trackIndex].setBounds(ctrlRow.removeFromLeft(30).reduced(1).toNearestInt());
            btnSolo[trackIndex].setBounds(ctrlRow.removeFromLeft(30).reduced(1).toNearestInt());
            btnClear[trackIndex].setBounds(ctrlRow.removeFromLeft(30).reduced(1).toNearestInt());
            btnShiftL[trackIndex].setBounds(ctrlRow.removeFromLeft(30).reduced(1).toNearestInt());
            btnShiftR[trackIndex].setBounds(ctrlRow.removeFromLeft(30).reduced(1).toNearestInt());
        }
        midiDragArea = bottomArea.removeFromRight(40);
        mainGridArea = bottomArea;
    }
    else {
        // ★ SETUP VIEW のレイアウト
        lockArea = juce::Rectangle<float>();
        midiDragArea = bottomArea.removeFromRight(40);

        auto setupTop = bottomArea.removeFromTop(30);
        barCountLabel.setBounds(setupTop.removeFromLeft(60).toNearestInt());
        barCountMenu.setBounds(setupTop.removeFromLeft(80).reduced(2).toNearestInt());

        bottomArea.removeFromTop(10); // 余白
        sampleArea = bottomArea.removeFromLeft(200); // D&D用にエリアを確保

        int rows = 8; float cellH = sampleArea.getHeight() / rows;
        for (int row = 0; row < rows; ++row) {
            int trackIndex = (rows - 1) - row;
            auto rowArea = bottomArea.withY(bottomArea.getY() + row * cellH).withHeight(cellH).reduced(2);

            // Name (SampleAreaの中の左端)
            juce::Rectangle<float> nameRect(sampleArea.getX(), sampleArea.getY() + row * cellH, 60, cellH);
            trackNameLabels[trackIndex].setBounds(nameRect.reduced(2).toNearestInt());

            // Division & Complexity
            divLabels[trackIndex].setBounds(rowArea.removeFromLeft(30).toNearestInt());
            divSelectors[trackIndex].setBounds(rowArea.removeFromLeft(60).toNearestInt());
            rowArea.removeFromLeft(20); // 余白
            compLabels[trackIndex].setBounds(rowArea.removeFromLeft(50).toNearestInt());
            complexitySliders[trackIndex].setBounds(rowArea.removeFromLeft(150).toNearestInt());
        }
        mainGridArea = bottomArea;
    }
}

void AIDrumMachineAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::darkgrey);

    g.setColour(juce::Colours::cyan.withAlpha(0.6f));
    g.fillRoundedRectangle(dragAllArea, 4.0f);
    g.setColour(juce::Colours::white);
    g.setFont(14.0f);
    g.drawText("DRAG ALL MIDI", dragAllArea, juce::Justification::centred, false);

    int rows = 8;
    float cellH = mainGridArea.getHeight() / rows;
    bool paginate = needsPagination();

    if (currentView == SequencerView) {
        for (int row = 0; row < rows; ++row)
        {
            int trackIndex = (rows - 1) - row;
            juce::Rectangle<float> lockBtn(lockArea.getX(), lockArea.getY() + row * cellH, lockArea.getWidth() - 2.0f, cellH - 2.0f);
            g.setColour(audioProcessor.trackLocked[trackIndex] ? juce::Colours::red.withAlpha(0.8f) : juce::Colours::grey.withAlpha(0.5f));
            g.fillRoundedRectangle(lockBtn, 4.0f);
            g.setColour(juce::Colours::white);
            g.setFont(10.0f);
            g.drawText("L", lockBtn, juce::Justification::centred, false);

            juce::Rectangle<float> sArea(sampleArea.getX() + 2.0f, sampleArea.getY() + row * cellH + 2.0f, sampleArea.getWidth() - 4.0f, cellH - 4.0f);
            if (dragTargetTrack == trackIndex) g.setColour(juce::Colours::white.withAlpha(0.3f));
            else if (audioProcessor.hasSampleLoaded(trackIndex)) g.setColour(juce::Colours::lightblue.withAlpha(0.2f));
            else g.setColour(juce::Colours::darkgrey.darker(0.8f));
            g.fillRoundedRectangle(sArea, 4.0f);

            juce::Rectangle<float> mDrag(midiDragArea.getX() + 4.0f, midiDragArea.getY() + row * cellH, midiDragArea.getWidth() - 4.0f, cellH - 2.0f);
            g.setColour(juce::Colours::cyan.withAlpha(0.3f));
            g.fillRoundedRectangle(mDrag, 4.0f);
            g.setColour(juce::Colours::white);
            g.setFont(11.0f);
            g.drawText("MIDI", mDrag, juce::Justification::centred, false);

            int div = audioProcessor.trackDivisions[trackIndex];
            if (div < 1) div = 1; int colsToDraw = paginate ? div : (div * 4);
            float cellW = mainGridArea.getWidth() / colsToDraw;

            for (int col = 0; col < colsToDraw; ++col) {
                juce::Rectangle<float> cell(mainGridArea.getX() + col * cellW, mainGridArea.getY() + row * cellH, cellW - 2.0f, cellH - 2.0f);
                int globalStep = paginate ? ((currentViewBar * div) + col) : col;
                int velocity = audioProcessor.drumPattern[trackIndex][globalStep];
                if (velocity > 0) g.setColour(juce::Colours::orange.withAlpha(0.3f + 0.7f * (velocity / 100.0f)));
                else g.setColour(juce::Colours::black.withAlpha(0.4f));
                g.fillRoundedRectangle(cell, 4.0f);
            }

            int currentStep = audioProcessor.getTrackCurrentStep(trackIndex);
            if (paginate) {
                int startStepOfThisBar = currentViewBar * div;
                if (currentStep >= startStepOfThisBar && currentStep < startStepOfThisBar + div) {
                    float playheadX = mainGridArea.getX() + (currentStep - startStepOfThisBar) * cellW;
                    g.setColour(juce::Colours::white.withAlpha(0.4f));
                    g.fillRoundedRectangle(juce::Rectangle<float>(playheadX, mainGridArea.getY() + row * cellH, cellW - 2.0f, cellH - 2.0f), 4.0f);
                }
            }
            else {
                if (currentStep < colsToDraw) {
                    float playheadX = mainGridArea.getX() + currentStep * cellW;
                    g.setColour(juce::Colours::white.withAlpha(0.4f));
                    g.fillRoundedRectangle(juce::Rectangle<float>(playheadX, mainGridArea.getY() + row * cellH, cellW - 2.0f, cellH - 2.0f), 4.0f);
                }
            }
        }
    }
    else {
        // ★ SETUP VIEW 描画 (サンプラースロットをわかりやすく)
        for (int row = 0; row < rows; ++row) {
            int trackIndex = (rows - 1) - row;
            // トラック名の右側をドロップエリアとして描画
            juce::Rectangle<float> sArea(sampleArea.getX() + 60.0f, sampleArea.getY() + row * cellH + 2.0f, sampleArea.getWidth() - 62.0f, cellH - 4.0f);

            if (dragTargetTrack == trackIndex) g.setColour(juce::Colours::white.withAlpha(0.3f));
            else if (audioProcessor.hasSampleLoaded(trackIndex)) g.setColour(juce::Colours::lightblue.withAlpha(0.2f));
            else g.setColour(juce::Colours::darkgrey.darker(0.8f));

            g.fillRoundedRectangle(sArea, 4.0f);

            g.setColour(juce::Colours::grey);
            g.setFont(12.0f);
            g.drawText(audioProcessor.hasSampleLoaded(trackIndex) ? "Sample Loaded" : "Drop Sample Here", sArea, juce::Justification::centred, false);
        }
    }
}

bool AIDrumMachineAudioProcessorEditor::isInterestedInFileDrag(const juce::StringArray& files) {
    for (auto file : files) { if (file.endsWithIgnoreCase(".wav") || file.endsWithIgnoreCase(".aif") || file.endsWithIgnoreCase(".aiff") || file.endsWithIgnoreCase(".mp3")) return true; } return false;
}
int AIDrumMachineAudioProcessorEditor::getTrackIndexFromMouseY(int y) {
    if (sampleArea.contains(sampleArea.getX(), (float)y)) {
        int rows = 8; float cellH = sampleArea.getHeight() / rows; int row = static_cast<int>((y - sampleArea.getY()) / cellH);
        if (row >= 0 && row < rows) return (rows - 1) - row;
    } return -1;
}
void AIDrumMachineAudioProcessorEditor::fileDragEnter(const juce::StringArray& files, int x, int y) { fileDragMove(files, x, y); }
void AIDrumMachineAudioProcessorEditor::fileDragMove(const juce::StringArray& files, int x, int y) {
    int target = getTrackIndexFromMouseY(y); if (dragTargetTrack != target) { dragTargetTrack = target; repaint(sampleArea.toNearestInt()); }
}
void AIDrumMachineAudioProcessorEditor::fileDragExit(const juce::StringArray& files) { dragTargetTrack = -1; repaint(sampleArea.toNearestInt()); }
void AIDrumMachineAudioProcessorEditor::filesDropped(const juce::StringArray& files, int x, int y) {
    dragTargetTrack = -1; int targetTrack = getTrackIndexFromMouseY(y);
    if (targetTrack != -1) {
        for (auto file : files) {
            if (file.endsWithIgnoreCase(".wav") || file.endsWithIgnoreCase(".aif") || file.endsWithIgnoreCase(".aiff") || file.endsWithIgnoreCase(".mp3")) {
                audioProcessor.loadSample(targetTrack, file);
                juce::File f(file); trackNameLabels[targetTrack].setText(f.getFileNameWithoutExtension(), juce::dontSendNotification);
                repaint(); break;
            }
        }
    }
}

void AIDrumMachineAudioProcessorEditor::mouseDown(const juce::MouseEvent& e) {
    juce::Point<float> pos = e.getPosition().toFloat(); int rows = 8;
    if (currentView == SequencerView) {
        if (lockArea.contains(pos)) {
            int row = static_cast<int>((pos.y - lockArea.getY()) / (lockArea.getHeight() / rows));
            if (row >= 0 && row < rows) { int trackIndex = (rows - 1) - row; audioProcessor.trackLocked[trackIndex] = !audioProcessor.trackLocked[trackIndex]; repaint(); }
        }
        else if (mainGridArea.contains(pos)) {
            int row = static_cast<int>((pos.y - mainGridArea.getY()) / (mainGridArea.getHeight() / rows));
            if (row >= 0 && row < rows) {
                int trackIndex = (rows - 1) - row; int div = audioProcessor.trackDivisions[trackIndex]; if (div < 1) div = 1;
                bool paginate = needsPagination(); int colsToDraw = paginate ? div : (div * 4);
                float cellW = mainGridArea.getWidth() / colsToDraw;
                int col = static_cast<int>((pos.x - mainGridArea.getX()) / cellW);
                if (col >= 0 && col < colsToDraw) {
                    int globalStep = paginate ? ((currentViewBar * div) + col) : col;
                    audioProcessor.drumPattern[trackIndex][globalStep] = (audioProcessor.drumPattern[trackIndex][globalStep] == 0) ? 100 : 0;
                    repaint();
                }
            }
        }
    }
}

void AIDrumMachineAudioProcessorEditor::mouseDrag(const juce::MouseEvent& e) {
    if (e.mouseWasDraggedSinceMouseDown() && !isDragging) {
        isDragging = true; juce::Point<float> startPos = e.getMouseDownPosition().toFloat(); int trackToExport = -2;
        if (dragAllArea.contains(startPos)) trackToExport = -1;
        else if (midiDragArea.contains(startPos)) { int rows = 8; int row = static_cast<int>((startPos.y - midiDragArea.getY()) / (midiDragArea.getHeight() / rows)); if (row >= 0 && row < rows) trackToExport = (rows - 1) - row; }
        if (trackToExport != -2) {
            juce::File midiFile = exportMidi(trackToExport);
            if (midiFile.existsAsFile()) {
                juce::StringArray files; files.add(midiFile.getFullPathName());
                if (auto* dragContainer = juce::DragAndDropContainer::findParentDragContainerFor(this)) dragContainer->performExternalDragDropOfFiles(files, false, this);
                else performExternalDragDropOfFiles(files, false, this);
            }
        }
    }
}
void AIDrumMachineAudioProcessorEditor::mouseUp(const juce::MouseEvent& e) { isDragging = false; }

juce::File AIDrumMachineAudioProcessorEditor::exportMidi(int trackIndex) {
    juce::MidiMessageSequence sequence; int midiNotes[8] = { 36, 38, 42, 46, 39, 41, 45, 50 };
    int startTrack = (trackIndex == -1) ? 0 : trackIndex; int endTrack = (trackIndex == -1) ? 7 : trackIndex;
    double ppq = 960.0; double ticksPerBar = ppq * 4.0;
    for (int trk = startTrack; trk <= endTrack; ++trk) {
        int div = audioProcessor.trackDivisions[trk]; if (div < 1) div = 1; double ticksPerStep = ticksPerBar / static_cast<double>(div);
        for (int step = 0; step < div * 4; ++step) {
            int vel = audioProcessor.drumPattern[trk][step];
            if (vel > 0) {
                double startTime = step * ticksPerStep; double endTime = startTime + (ticksPerStep * 0.5);
                juce::uint8 midiVel = static_cast<juce::uint8>((vel / 100.0f) * 127.0f); if (midiVel == 0) midiVel = 1;
                auto noteOn = juce::MidiMessage::noteOn(10, midiNotes[trk], midiVel); noteOn.setTimeStamp(startTime); sequence.addEvent(noteOn);
                auto noteOff = juce::MidiMessage::noteOff(10, midiNotes[trk]); noteOff.setTimeStamp(endTime); sequence.addEvent(noteOff);
            }
        }
    }
    sequence.updateMatchedPairs(); juce::MidiFile midiFile; midiFile.setTicksPerQuarterNote(static_cast<short>(ppq)); midiFile.addTrack(sequence);
    juce::String filename = (trackIndex == -1) ? "Polyrhythm_All.mid" : ("Polyrhythm_Track_" + juce::String(trackIndex + 1) + ".mid");
    juce::File tempFile = juce::File::getSpecialLocation(juce::File::tempDirectory).getChildFile(filename); tempFile.deleteFile();
    juce::FileOutputStream outStream(tempFile); midiFile.writeTo(outStream); outStream.flush();
    return tempFile;
}