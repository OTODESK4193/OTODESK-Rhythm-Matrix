// ==============================================================================
// Source/PluginEditor.cpp
// ==============================================================================
#include "PluginProcessor.h"
#include "PluginEditor.h"

// ★ ARPピッチ計算用外部配列の参照
extern const int scalePatterns[19][8];

// ==============================================================================
// ★ GUI負荷を劇的に下げるための専用シーケンサーグリッドコンポーネント
// ==============================================================================
SequencerGrid::SequencerGrid(AIDrumMachineAudioProcessor& p) : audioProcessor(p) {}

void SequencerGrid::paint(juce::Graphics& g) {
    int rows = 8;
    float cellH = mainGridArea.getHeight() / (float)juce::jmax(1, rows);
    int numBeats = audioProcessor.timeSigNumerator.load();
    if (numBeats < 1) numBeats = 4;

    for (int row = 0; row < rows; ++row) {
        int idx = 7 - row;

        // --- Lock Button Draw ---
        juce::Rectangle<float> lockBtn((float)lockArea.getX(), lockArea.getY() + row * cellH, lockArea.getWidth() - 2.0f, cellH - 2.0f);
        g.setColour(audioProcessor.trackLocked[idx] ? juce::Colours::red.withAlpha(0.8f) : juce::Colours::grey.withAlpha(0.5f));
        g.fillRoundedRectangle(lockBtn, 4.0f);
        g.setColour(juce::Colours::white); g.setFont(10.0f);
        g.drawText("L", lockBtn, juce::Justification::centred, false);

        // --- MIDI Drag Area Draw ---
        juce::Rectangle<float> mDrag((float)midiDragArea.getX() + 4.0f, midiDragArea.getY() + row * cellH, midiDragArea.getWidth() - 4.0f, cellH - 2.0f);
        g.setColour(juce::Colours::cyan.withAlpha(0.3f));
        g.fillRoundedRectangle(mDrag, 4.0f);
        g.setColour(juce::Colours::white); g.setFont(11.0f);
        g.drawText("MIDI", mDrag, juce::Justification::centred, false);

        // --- Grid Background Draw ---
        int div = audioProcessor.trackDivisionsUI[idx];
        if (div < 1) div = 1;
        int colsToDraw = numBeats * div;
        float cellW = mainGridArea.getWidth() / (float)juce::jmax(1, colsToDraw);

        // 背景のシマシマ（拍ごと）- ★ float に明示的キャスト
        for (int b = 0; b < numBeats; ++b) {
            g.setColour(b % 2 == 0 ? juce::Colours::black.withAlpha(0.15f) : juce::Colours::white.withAlpha(0.05f));
            g.fillRect((float)(mainGridArea.getX() + b * div * cellW),
                (float)(mainGridArea.getY() + row * cellH),
                (float)(div * cellW),
                cellH);
        }

        // 各ステップの区切り線と、トラックの横線を描画 - ★ float に明示的キャスト
        for (int col = 0; col <= colsToDraw; ++col) {
            if (col > 0 && col < colsToDraw) {
                if (col % div == 0) {
                    // 拍の区切り（太めの白線で目立たせる）
                    g.setColour(juce::Colours::white.withAlpha(0.25f));
                    g.fillRect((float)(mainGridArea.getX() + col * cellW - 1.0f),
                        (float)(mainGridArea.getY() + row * cellH),
                        2.0f,
                        cellH);
                }
                else {
                    // 各ステップの区切り（細い黒線）
                    g.setColour(juce::Colours::black.withAlpha(0.3f));
                    g.fillRect((float)(mainGridArea.getX() + col * cellW),
                        (float)(mainGridArea.getY() + row * cellH),
                        1.0f,
                        cellH);
                }
            }
        }

        // トラック（行）を分ける横線 - ★ float に明示的キャスト
        g.setColour(juce::Colours::black.withAlpha(0.4f));
        g.fillRect((float)mainGridArea.getX(),
            (float)(mainGridArea.getY() + row * cellH + cellH - 1.0f),
            (float)mainGridArea.getWidth(),
            1.0f);


        // --- Grid Notes Draw ---
        for (int col = 0; col < colsToDraw; ++col) {
            int globalStep = (currentViewBar * colsToDraw) + col;
            if (globalStep < 1024) {
                int vel = audioProcessor.drumPatternUI[idx][globalStep];
                if (vel > 0) {
                    juce::Rectangle<float> cell((float)(mainGridArea.getX() + col * cellW + 1),
                        (float)(mainGridArea.getY() + row * cellH + 1),
                        cellW - 2.0f,
                        cellH - 2.0f);
                    float alpha = juce::jlimit(0.0f, 1.0f, 0.2f + 0.8f * (vel / 100.0f));
                    g.setColour(juce::Colours::orange.withAlpha(alpha));
                    g.fillRoundedRectangle(cell, 2.0f);
                }
            }
        }

        // --- Playhead Draw ---
        int cur = audioProcessor.getTrackCurrentStep(idx);
        int startStep = currentViewBar * colsToDraw;
        if (cur >= startStep && cur < startStep + colsToDraw) {
            g.setColour(juce::Colours::white.withAlpha(0.3f));
            g.fillRect((float)(mainGridArea.getX() + (cur - startStep) * cellW),
                (float)(mainGridArea.getY() + row * cellH),
                cellW,
                cellH);
        }
    }
}

void SequencerGrid::mouseDown(const juce::MouseEvent& e) {
    auto pos = e.getPosition();
    if (lockArea.contains(pos)) {
        int row = (int)((pos.y - lockArea.getY()) / juce::jmax(1, lockArea.getHeight() / 8));
        if (row >= 0 && row < 8) {
            int idx = 7 - row;
            audioProcessor.trackLocked[idx] = !audioProcessor.trackLocked[idx];
            repaint();
        }
    }
    else if (mainGridArea.contains(pos)) {
        int row = (int)((pos.y - mainGridArea.getY()) / juce::jmax(1, mainGridArea.getHeight() / 8));
        int idx = 7 - row;
        if (idx >= 0 && idx < 8) {
            int div = audioProcessor.trackDivisionsUI[idx];
            if (div < 1) div = 1;
            int numBeats = audioProcessor.timeSigNumerator.load();
            if (numBeats < 1) numBeats = 4;

            float cellW = mainGridArea.getWidth() / (float)juce::jmax(1, div * numBeats);
            int col = (int)((pos.x - mainGridArea.getX()) / juce::jmax(1.0f, cellW));
            int globalStep = (currentViewBar * div * numBeats) + col;

            if (globalStep >= 0 && globalStep < 1024) {
                audioProcessor.drumPatternUI[idx][globalStep] = (audioProcessor.drumPatternUI[idx][globalStep] > 0) ? 0 : 100;
                audioProcessor.patternUpdated.store(true);
                repaint();
            }
        }
    }
}
// ==============================================================================

AIDrumMachineAudioProcessorEditor::AIDrumMachineAudioProcessorEditor(AIDrumMachineAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p), seqGrid(p)
{
    setSize(1080, 520);

    // ★ 重なりを防ぐため一番最初にグリッドを追加
    addAndMakeVisible(seqGrid);

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

    addAndMakeVisible(btnTempoLock);
    btnTempoLock.setClickingTogglesState(true);
    btnTempoLock.setColour(juce::TextButton::buttonOnColourId, juce::Colours::orange);
    btnTempoLock.setToggleState(audioProcessor.tempoLocked.load(), juce::dontSendNotification);
    btnTempoLock.onClick = [this] {
        bool state = btnTempoLock.getToggleState();
        audioProcessor.tempoLocked.store(state);
        audioProcessor.userTuning[audioProcessor.currentGenre.load()].tempoLocked = state;
        tuningTempoLock.setToggleState(state, juce::dontSendNotification);
        };

    addAndMakeVisible(tabSeqButton); addAndMakeVisible(tabSetupButton); addAndMakeVisible(tabSetup2Button); addAndMakeVisible(tabTuningButton);
    tabSeqButton.onClick = [this] { currentView = SequencerView; updateViewVisibility(); resized(); repaint(); };
    tabSetupButton.onClick = [this] { currentView = Setup1View; updateViewVisibility(); resized(); repaint(); };
    tabSetup2Button.onClick = [this] { currentView = Setup2View; updateViewVisibility(); resized(); repaint(); };

    tabTuningButton.onClick = [this] {
        currentView = TuningView; updateTuningUIFromProcessor(); updateViewVisibility(); resized(); repaint();
        };

    addAndMakeVisible(btnClearAll);
    btnClearAll.setColour(juce::TextButton::buttonColourId, juce::Colours::red.withAlpha(0.6f));
    btnClearAll.onClick = [this] {
        juce::NativeMessageBox::showOkCancelBox(
            juce::MessageBoxIconType::WarningIcon, "Reset All", "Reset settings to defaults?", this,
            juce::ModalCallbackFunction::create([this](int result) {
                if (result == 1) {
                    audioProcessor.globalBarCount = 1;
                    barCountMenu.setSelectedId(1, juce::dontSendNotification);
                    currentViewBar = 0;
                    for (int i = 0; i < 8; ++i) {
                        audioProcessor.clearTrack(i);
                        audioProcessor.trackDivisionsUI[i] = 4; divSelectors[i].setSelectedId(4, juce::dontSendNotification);
                        bool isCore = (i == 0 || i == 1 || i == 4);
                        audioProcessor.trackCmplxLocked[i] = isCore;
                        btnCmplxLock[i].setToggleState(isCore, juce::dontSendNotification);
                        audioProcessor.trackComplexity[i] = isCore ? 0 : 30;
                        complexitySliders[i].setValue(audioProcessor.trackComplexity[i], juce::dontSendNotification);
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
        int newNum = timeSigNumMenu.getSelectedId();
        if (newNum < 1) newNum = 4;
        audioProcessor.timeSigNumerator.store(newNum);
        audioProcessor.patternUpdated.store(true);
        resized(); repaint();
        };

    addAndMakeVisible(timeSigSlash); timeSigSlash.setColour(juce::Label::textColourId, juce::Colours::white);
    timeSigSlash.setJustificationType(juce::Justification::centred);

    addAndMakeVisible(timeSigDenMenu);
    timeSigDenMenu.addItem("4", 4); timeSigDenMenu.addItem("8", 8); timeSigDenMenu.addItem("16", 16);
    timeSigDenMenu.setSelectedId(audioProcessor.timeSigDenominator.load(), juce::dontSendNotification);
    timeSigDenMenu.onChange = [this] {
        int newDen = timeSigDenMenu.getSelectedId();
        if (newDen < 1) newDen = 4;
        audioProcessor.timeSigDenominator.store(newDen);
        updateTimeSigNumMenu();
        updateDivisionMenus();
        audioProcessor.patternUpdated.store(true);
        resized(); repaint();
        };

    addAndMakeVisible(btnTimeSigLock);
    btnTimeSigLock.setButtonText("L");
    btnTimeSigLock.setClickingTogglesState(true);
    btnTimeSigLock.setColour(juce::TextButton::buttonOnColourId, juce::Colours::orange);
    btnTimeSigLock.setToggleState(audioProcessor.timeSigLocked.load(), juce::dontSendNotification);
    btnTimeSigLock.onClick = [this] { audioProcessor.timeSigLocked.store(btnTimeSigLock.getToggleState()); };

    addAndMakeVisible(barCountLabel); barCountLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(barCountMenu);
    barCountMenu.addItem("1 Bar", 1); barCountMenu.addItem("2 Bars", 2);
    barCountMenu.addItem("3 Bars", 3); barCountMenu.addItem("4 Bars", 4);
    barCountMenu.setSelectedId(audioProcessor.globalBarCount.load(), juce::dontSendNotification);
    barCountMenu.onChange = [this] {
        int newBars = barCountMenu.getSelectedId();
        if (newBars < 1) newBars = 1;
        audioProcessor.globalBarCount.store(newBars);
        if (currentViewBar >= audioProcessor.globalBarCount.load()) currentViewBar = audioProcessor.globalBarCount.load() - 1;
        audioProcessor.patternUpdated.store(true);
        updateTabColors(); updateViewVisibility(); resized(); repaint();
        };

    addAndMakeVisible(generateButton); addAndMakeVisible(styleMenu); addAndMakeVisible(statusLabel);
    updateStyleMenu();

    addAndMakeVisible(fillBarMenu);
    fillBarMenu.addItem("Fill: Off", 1);
    fillBarMenu.addItem("Fill: Bar 1", 2);
    fillBarMenu.addItem("Fill: Bar 2", 3);
    fillBarMenu.addItem("Fill: Bar 3", 4);
    fillBarMenu.addItem("Fill: Bar 4", 5);
    fillBarMenu.setSelectedId(audioProcessor.fillBarTarget.load() + 1, juce::dontSendNotification);
    fillBarMenu.onChange = [this] { audioProcessor.fillBarTarget.store(fillBarMenu.getSelectedId() - 1); };

    addAndMakeVisible(btnAutoFollow);
    btnAutoFollow.setClickingTogglesState(true);
    btnAutoFollow.setColour(juce::TextButton::buttonOnColourId, juce::Colours::orange);
    btnAutoFollow.setToggleState(audioProcessor.autoFollowEnabled.load(), juce::dontSendNotification);
    btnAutoFollow.onClick = [this] { audioProcessor.autoFollowEnabled.store(btnAutoFollow.getToggleState()); };

    addAndMakeVisible(btnArpMode);
    btnArpMode.setClickingTogglesState(true);
    btnArpMode.setColour(juce::TextButton::buttonOnColourId, juce::Colours::orange);
    btnArpMode.setToggleState(audioProcessor.arpMode.load(), juce::dontSendNotification);
    btnArpMode.onClick = [this] {
        audioProcessor.arpMode.store(btnArpMode.getToggleState());
        updateStyleMenu(); updateTrackNames(); updateViewVisibility();
        };

    addAndMakeVisible(arpKeyMenu);
    const char* keys[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    for (int i = 0; i < 12; ++i) arpKeyMenu.addItem(keys[i], i + 1);
    arpKeyMenu.setSelectedId(audioProcessor.arpKey.load() + 1, juce::dontSendNotification);
    arpKeyMenu.onChange = [this] { audioProcessor.arpKey.store(arpKeyMenu.getSelectedId() - 1); updateTrackNames(); };

    addAndMakeVisible(arpScaleMenu);
    const char* scales[] = {
        "Major", "Natural Minor", "Pentatonic Major", "Pentatonic Minor", "Dorian", "Harmonic Minor", "Lydian", "Mixolydian", "Phrygian", "Locrian", "Whole Tone", "Blues",
        "Aeolian", "Dorian nat7", "Phrygian nat6", "Lydian #5", "Locrian b4", "Comb of Diminished", "Augmented"
    };
    for (int i = 0; i < 19; ++i) arpScaleMenu.addItem(scales[i], i + 1);
    arpScaleMenu.setSelectedId(audioProcessor.arpScale.load() + 1, juce::dontSendNotification);
    arpScaleMenu.onChange = [this] { audioProcessor.arpScale.store(arpScaleMenu.getSelectedId() - 1); updateTrackNames(); };

    addAndMakeVisible(btnArpMono);
    btnArpMono.setButtonText("Mono Mode");
    btnArpMono.setClickingTogglesState(true);
    btnArpMono.setColour(juce::TextButton::buttonOnColourId, juce::Colours::orange);
    btnArpMono.setToggleState(audioProcessor.arpMono.load(), juce::dontSendNotification);
    btnArpMono.onClick = [this] { audioProcessor.arpMono.store(btnArpMono.getToggleState()); };

    styleMenu.onChange = [this] {
        int genreIndex = styleMenu.getSelectedId() - 1;
        if (genreIndex < 0) genreIndex = 0;
        audioProcessor.currentGenre.store(genreIndex);

        if (genreIndex == 25) {
            int targetDivs[8] = { 2, 4, 3, 5, 7, 6, 8, 1 };
            for (int i = 0; i < 8; ++i) {
                audioProcessor.userTuning[25].tracks[i].divLocked = true;
                audioProcessor.trackDivLocked[i] = true;
                btnDivLock[i].setToggleState(true, juce::dontSendNotification);
                tuningDivLock[i].setToggleState(true, juce::dontSendNotification);
                audioProcessor.trackDivisionsUI[i] = targetDivs[i];
                divSelectors[i].setSelectedId(targetDivs[i], juce::dontSendNotification);
            }
        }
        else {
            for (int i = 0; i < 8; ++i) {
                bool isLocked = audioProcessor.userTuning[genreIndex].tracks[i].divLocked;
                audioProcessor.trackDivLocked[i] = isLocked;
                btnDivLock[i].setToggleState(isLocked, juce::dontSendNotification);
                tuningDivLock[i].setToggleState(isLocked, juce::dontSendNotification);
            }
        }

        if (!audioProcessor.arpMode.load()) {
            const auto& def = AIDrumMachineAudioProcessor::getGenreDef(genreIndex);
            if (!audioProcessor.tempoLocked.load() && !audioProcessor.isSyncEnabled.load()) {
                audioProcessor.internalTempo.store((def.minTempo + def.maxTempo) / 2.0);
            }
            timeSigDenMenu.setSelectedId(def.defaultDen, juce::dontSendNotification);
            audioProcessor.timeSigDenominator.store(def.defaultDen);
            updateTimeSigNumMenu();
            timeSigNumMenu.setSelectedId(def.defaultNum, juce::dontSendNotification);
            audioProcessor.timeSigNumerator.store(def.defaultNum);

            for (int i = 0; i < 8; ++i) {
                if ((genreIndex >= 22 || genreIndex == 14 || genreIndex == 21) && (i == 0 || i == 1 || i == 4)) {
                    audioProcessor.trackCmplxLocked[i] = false;
                    btnCmplxLock[i].setToggleState(false, juce::dontSendNotification);
                }
            }
        }
        updateDivisionMenus(); updateTrackNames();
        if (currentView == TuningView) updateTuningUIFromProcessor();
        audioProcessor.patternUpdated.store(true);
        resized(); repaint();
        };

    addAndMakeVisible(tabButton1); addAndMakeVisible(tabButton2); addAndMakeVisible(tabButton3); addAndMakeVisible(tabButton4);
    auto tabClick = [this](int barIndex) { currentViewBar = barIndex; updateTabColors(); seqGrid.updateBar(barIndex); repaint(); };
    tabButton1.onClick = [tabClick] { tabClick(0); }; tabButton2.onClick = [tabClick] { tabClick(1); };
    tabButton3.onClick = [tabClick] { tabClick(2); }; tabButton4.onClick = [tabClick] { tabClick(3); };
    updateTabColors();

    for (int i = 0; i < 4; ++i) {
        addAndMakeVisible(btnPattern[i]);
        btnPattern[i].setButtonText("Pat " + juce::String(i + 1));
        btnPattern[i].setTriggeredOnMouseDown(true);
        btnPattern[i].onClick = [this, i] {
            auto mods = juce::ModifierKeys::getCurrentModifiers();
            if (mods.isPopupMenu()) {
                if (audioProcessor.isPatternSaved[i]) {
                    juce::NativeMessageBox::showOkCancelBox(juce::MessageBoxIconType::QuestionIcon, "Clear Pattern", "Clear Pattern " + juce::String(i + 1) + "?", this,
                        juce::ModalCallbackFunction::create([this, i](int result) {
                            if (result == 1) { audioProcessor.isPatternSaved[i] = false; updatePatternButtonColors(); }
                            }));
                }
            }
            else {
                if (!audioProcessor.isPatternSaved[i]) {
                    SavedPattern& sp = audioProcessor.savedPatterns[i];
                    for (int t = 0; t < 8; ++t) {
                        std::memcpy(sp.drumPattern[t], audioProcessor.drumPatternUI[t], sizeof(sp.drumPattern[t]));
                        sp.trackDivisions[t] = audioProcessor.trackDivisionsUI[t];
                        sp.trackShift[t] = audioProcessor.trackShiftUI[t];
                        sp.trackComplexity[t] = audioProcessor.trackComplexity[t];
                        sp.trackEntropy[t] = audioProcessor.trackEntropy[t];
                    }
                    sp.num = audioProcessor.timeSigNumerator.load();
                    sp.den = audioProcessor.timeSigDenominator.load();
                    sp.bars = audioProcessor.globalBarCount.load();
                    audioProcessor.isPatternSaved[i] = true;
                }
                else {
                    SavedPattern& sp = audioProcessor.savedPatterns[i];
                    for (int t = 0; t < 8; ++t) {
                        std::memcpy(audioProcessor.drumPatternUI[t], sp.drumPattern[t], sizeof(sp.drumPattern[t]));
                        audioProcessor.trackDivisionsUI[t] = sp.trackDivisions[t];
                        audioProcessor.trackShiftUI[t] = sp.trackShift[t];
                        audioProcessor.trackComplexity[t] = sp.trackComplexity[t];
                        audioProcessor.trackEntropy[t] = sp.trackEntropy[t];
                    }
                    audioProcessor.timeSigNumerator.store(sp.num);
                    audioProcessor.timeSigDenominator.store(sp.den);
                    audioProcessor.globalBarCount.store(sp.bars);
                    audioProcessor.patternUpdated.store(true);
                    audioProcessor.uiNeedsUpdate.store(true);
                }
                updatePatternButtonColors();
            }
            };
    }
    updatePatternButtonColors();

    for (int i = 0; i < 8; ++i) {
        addAndMakeVisible(trackNameLabels[i]);
        trackNameLabels[i].setJustificationType(juce::Justification::centredLeft);
        trackNameLabels[i].setColour(juce::Label::textColourId, juce::Colours::white);
        trackNameLabels[i].setFont(juce::Font(13.0f)); trackNameLabels[i].setMinimumHorizontalScale(0.8f);

        addChildComponent(midiKeyLabels[i]); midiKeyLabels[i].setText("[" + trackNotes[i] + "]", juce::dontSendNotification);
        midiKeyLabels[i].setColour(juce::Label::textColourId, juce::Colours::grey); midiKeyLabels[i].setJustificationType(juce::Justification::centredRight);

        addAndMakeVisible(btnDegreeLock[i]);
        btnDegreeLock[i].setButtonText("L"); btnDegreeLock[i].setClickingTogglesState(true);
        btnDegreeLock[i].setColour(juce::TextButton::buttonOnColourId, juce::Colours::orange);
        btnDegreeLock[i].setToggleState(audioProcessor.trackDegreeLocked[i], juce::dontSendNotification);
        btnDegreeLock[i].onClick = [this, i] { audioProcessor.trackDegreeLocked[i] = btnDegreeLock[i].getToggleState(); };

        addAndMakeVisible(btnOctaveLock[i]);
        btnOctaveLock[i].setButtonText("L"); btnOctaveLock[i].setClickingTogglesState(true);
        btnOctaveLock[i].setColour(juce::TextButton::buttonOnColourId, juce::Colours::orange);
        btnOctaveLock[i].setToggleState(audioProcessor.trackOctaveLocked[i], juce::dontSendNotification);
        btnOctaveLock[i].onClick = [this, i] { audioProcessor.trackOctaveLocked[i] = btnOctaveLock[i].getToggleState(); };

        addAndMakeVisible(btnDynamic[i]);
        btnDynamic[i].setButtonText("Dyn"); btnDynamic[i].setClickingTogglesState(true);
        btnDynamic[i].setColour(juce::TextButton::buttonOnColourId, juce::Colours::yellow.darker());
        btnDynamic[i].setToggleState(audioProcessor.trackDynamic[i], juce::dontSendNotification);
        btnDynamic[i].onClick = [this, i] { audioProcessor.trackDynamic[i] = btnDynamic[i].getToggleState(); };

        addChildComponent(dynamicSliders[i]);
        dynamicSliders[i].setSliderStyle(juce::Slider::LinearHorizontal);
        dynamicSliders[i].setTextBoxStyle(juce::Slider::TextBoxRight, false, 40, 20);
        dynamicSliders[i].setRange(0.0, 100.0, 1.0);
        dynamicSliders[i].setValue(audioProcessor.trackDynamicAmount[i], juce::dontSendNotification);
        dynamicSliders[i].onValueChange = [this, i] {
            audioProcessor.trackDynamicAmount[i] = static_cast<int>(dynamicSliders[i].getValue());
            audioProcessor.patternUpdated.store(true);
            };

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
                juce::ModalCallbackFunction::create([this, i](int result) { if (result == 1) { audioProcessor.clearTrack(i); seqGrid.repaint(); } }));
            };
        btnShiftL[i].onClick = [this, i] { audioProcessor.shiftTrackLeft(i); seqGrid.repaint(); };
        btnShiftR[i].onClick = [this, i] { audioProcessor.shiftTrackRight(i); seqGrid.repaint(); };

        addChildComponent(divLabels[i]); divLabels[i].setText("Div:", juce::dontSendNotification);
        addChildComponent(divSelectors[i]);
        divSelectors[i].onChange = [this, i] { audioProcessor.trackDivisionsUI[i] = divSelectors[i].getSelectedId(); audioProcessor.patternUpdated.store(true); seqGrid.repaint(); };

        addChildComponent(btnDivLock[i]); btnDivLock[i].setButtonText("L"); btnDivLock[i].setClickingTogglesState(true);
        btnDivLock[i].setColour(juce::TextButton::buttonOnColourId, juce::Colours::orange);
        btnDivLock[i].setToggleState(audioProcessor.trackDivLocked[i], juce::dontSendNotification);
        btnDivLock[i].onClick = [this, i] {
            bool state = btnDivLock[i].getToggleState();
            audioProcessor.trackDivLocked[i] = state;
            audioProcessor.userTuning[audioProcessor.currentGenre.load()].tracks[i].divLocked = state;
            tuningDivLock[i].setToggleState(state, juce::dontSendNotification);
            };

        addChildComponent(compLabels[i]); compLabels[i].setText("Cmplx:", juce::dontSendNotification);
        addChildComponent(complexitySliders[i]); complexitySliders[i].setSliderStyle(juce::Slider::LinearHorizontal);
        complexitySliders[i].setTextBoxStyle(juce::Slider::TextBoxRight, false, 40, 20);
        complexitySliders[i].setRange(0.0, 100.0, 1.0); complexitySliders[i].setValue(audioProcessor.trackComplexity[i], juce::dontSendNotification);
        complexitySliders[i].onValueChange = [this, i] { audioProcessor.trackComplexity[i] = static_cast<int>(complexitySliders[i].getValue()); };

        addChildComponent(btnCmplxLock[i]); btnCmplxLock[i].setButtonText("L"); btnCmplxLock[i].setClickingTogglesState(true);
        btnCmplxLock[i].setColour(juce::TextButton::buttonOnColourId, juce::Colours::orange);
        btnCmplxLock[i].setToggleState(audioProcessor.trackCmplxLocked[i], juce::dontSendNotification);
        btnCmplxLock[i].onClick = [this, i] {
            bool state = btnCmplxLock[i].getToggleState();
            audioProcessor.trackCmplxLocked[i] = state;
            audioProcessor.userTuning[audioProcessor.currentGenre.load()].tracks[i].cmplxLocked = state;
            tuningCmplxLock[i].setToggleState(state, juce::dontSendNotification);
            };

        addChildComponent(entrpLabels[i]); entrpLabels[i].setText("Entrp:", juce::dontSendNotification);
        addChildComponent(entropySliders[i]); entropySliders[i].setSliderStyle(juce::Slider::LinearHorizontal);
        entropySliders[i].setTextBoxStyle(juce::Slider::TextBoxRight, false, 40, 20);
        entropySliders[i].setRange(0.0, 100.0, 1.0); entropySliders[i].setValue(audioProcessor.trackEntropy[i], juce::dontSendNotification);
        entropySliders[i].onValueChange = [this, i] { audioProcessor.trackEntropy[i] = static_cast<int>(entropySliders[i].getValue()); };

        addChildComponent(btnEntrpLock[i]); btnEntrpLock[i].setButtonText("L"); btnEntrpLock[i].setClickingTogglesState(true);
        btnEntrpLock[i].setColour(juce::TextButton::buttonOnColourId, juce::Colours::orange);
        btnEntrpLock[i].setToggleState(audioProcessor.trackEntrpLocked[i], juce::dontSendNotification);
        btnEntrpLock[i].onClick = [this, i] {
            bool state = btnEntrpLock[i].getToggleState();
            audioProcessor.trackEntrpLocked[i] = state;
            audioProcessor.userTuning[audioProcessor.currentGenre.load()].tracks[i].entrpLocked = state;
            tuningEntrpLock[i].setToggleState(state, juce::dontSendNotification);
            };

        addChildComponent(shiftLabels[i]); shiftLabels[i].setText("Shift:", juce::dontSendNotification);
        addChildComponent(shiftSliders[i]); shiftSliders[i].setSliderStyle(juce::Slider::LinearHorizontal);
        shiftSliders[i].setTextBoxStyle(juce::Slider::TextBoxRight, false, 40, 20);
        shiftSliders[i].setRange(-50.0, 50.0, 1.0); shiftSliders[i].setValue(audioProcessor.trackShiftUI[i], juce::dontSendNotification);
        shiftSliders[i].onValueChange = [this, i] { audioProcessor.trackShiftUI[i] = static_cast<int>(shiftSliders[i].getValue()); audioProcessor.patternUpdated.store(true); };

        addChildComponent(btnShiftLock[i]); btnShiftLock[i].setButtonText("L"); btnShiftLock[i].setClickingTogglesState(true);
        btnShiftLock[i].setColour(juce::TextButton::buttonOnColourId, juce::Colours::orange);
        btnShiftLock[i].setToggleState(audioProcessor.trackShiftLocked[i], juce::dontSendNotification);
        btnShiftLock[i].onClick = [this, i] {
            bool state = btnShiftLock[i].getToggleState();
            audioProcessor.trackShiftLocked[i] = state;
            audioProcessor.userTuning[audioProcessor.currentGenre.load()].tracks[i].shiftLocked = state;
            tuningShiftLock[i].setToggleState(state, juce::dontSendNotification);
            };

        addChildComponent(octaveLabels[i]); octaveLabels[i].setText("Octave:", juce::dontSendNotification);
        addChildComponent(octaveSliders[i]); octaveSliders[i].setSliderStyle(juce::Slider::LinearHorizontal);
        octaveSliders[i].setTextBoxStyle(juce::Slider::TextBoxRight, false, 40, 20);
        octaveSliders[i].setRange(-2.0, 2.0, 1.0); octaveSliders[i].setValue(audioProcessor.trackOctaveUI[i], juce::dontSendNotification);
        octaveSliders[i].onValueChange = [this, i] {
            audioProcessor.trackOctaveUI[i] = static_cast<int>(octaveSliders[i].getValue());
            updateTrackNames(); audioProcessor.patternUpdated.store(true);
            };
    }

    addChildComponent(tuningTempoTitle); tuningTempoTitle.setColour(juce::Label::textColourId, juce::Colours::white);
    addChildComponent(tuningTsTitle); tuningTsTitle.setColour(juce::Label::textColourId, juce::Colours::white);
    addChildComponent(tuningFillsTitle); tuningFillsTitle.setColour(juce::Label::textColourId, juce::Colours::white);

    addChildComponent(tuningColDiv); tuningColDiv.setColour(juce::Label::textColourId, juce::Colours::white); tuningColDiv.setJustificationType(juce::Justification::centred);
    addChildComponent(tuningColCmplx); tuningColCmplx.setColour(juce::Label::textColourId, juce::Colours::white); tuningColCmplx.setJustificationType(juce::Justification::centred);
    addChildComponent(tuningColEntrp); tuningColEntrp.setColour(juce::Label::textColourId, juce::Colours::white); tuningColEntrp.setJustificationType(juce::Justification::centred);
    addChildComponent(tuningColShift); tuningColShift.setColour(juce::Label::textColourId, juce::Colours::white); tuningColShift.setJustificationType(juce::Justification::centred);

    setupNumBox(tuningTempoMin, [this](int v) { audioProcessor.userTuning[audioProcessor.currentGenre.load()].tempo.min = v; });
    setupNumBox(tuningTempoMax, [this](int v) { audioProcessor.userTuning[audioProcessor.currentGenre.load()].tempo.max = v; });

    addChildComponent(tuningTempoLock);
    tuningTempoLock.setButtonText("L"); tuningTempoLock.setClickingTogglesState(true);
    tuningTempoLock.setColour(juce::TextButton::buttonOnColourId, juce::Colours::orange);
    tuningTempoLock.onClick = [this] {
        bool state = tuningTempoLock.getToggleState();
        audioProcessor.userTuning[audioProcessor.currentGenre.load()].tempoLocked = state;
        audioProcessor.tempoLocked.store(state);
        btnTempoLock.setToggleState(state, juce::dontSendNotification);
        };

    addChildComponent(btnTuningRandom);
    btnTuningRandom.setColour(juce::TextButton::buttonColourId, juce::Colours::teal);
    btnTuningRandom.onClick = [this] {
        int g = audioProcessor.currentGenre.load();
        auto& t = audioProcessor.userTuning[g];
        for (int i = 0; i < 8; ++i) {
            if (!t.tracks[i].cmplxLocked) {
                if (g == 25) {
                    t.tracks[i].cmplx.min = audioProcessor.random.nextInt(4);
                    t.tracks[i].cmplx.max = audioProcessor.random.nextInt(4);
                }
                else {
                    t.tracks[i].cmplx.min = audioProcessor.random.nextInt(11);
                    t.tracks[i].cmplx.max = audioProcessor.random.nextInt(11);
                }
                if (t.tracks[i].cmplx.min > t.tracks[i].cmplx.max) std::swap(t.tracks[i].cmplx.min, t.tracks[i].cmplx.max);
            }

            if (!t.tracks[i].entrpLocked) {
                if (g == 25) {
                    t.tracks[i].entrp.min = audioProcessor.random.nextInt(11);
                    t.tracks[i].entrp.max = audioProcessor.random.nextInt(11);
                }
                else {
                    t.tracks[i].entrp.min = audioProcessor.random.nextInt(11);
                    t.tracks[i].entrp.max = audioProcessor.random.nextInt(11);
                }
                if (t.tracks[i].entrp.min > t.tracks[i].entrp.max) std::swap(t.tracks[i].entrp.min, t.tracks[i].entrp.max);
            }

            if (!t.tracks[i].shiftLocked) {
                if (i == 0 || g == 24 || g == 25) {
                    t.tracks[i].shift.min = 0;
                    t.tracks[i].shift.max = 0;
                }
                else {
                    t.tracks[i].shift.min = audioProcessor.random.nextInt(11) - 5;
                    t.tracks[i].shift.max = audioProcessor.random.nextInt(11) - 5;
                    if (t.tracks[i].shift.min > t.tracks[i].shift.max) std::swap(t.tracks[i].shift.min, t.tracks[i].shift.max);
                }
            }
        }
        updateTuningUIFromProcessor();
        };

    addChildComponent(btnTuningClear);
    btnTuningClear.setColour(juce::TextButton::buttonColourId, juce::Colours::red.withAlpha(0.6f));
    btnTuningClear.onClick = [this] {
        int g = audioProcessor.currentGenre.load();
        auto& t = audioProcessor.userTuning[g];
        for (int i = 0; i < 8; ++i) {
            if (!t.tracks[i].divLocked) {
                for (int d = 0; d < 8; ++d) t.tracks[i].allowedDivs[d] = false;
            }

            if (g == 25) {
                if (!t.tracks[i].cmplxLocked) { t.tracks[i].cmplx.min = 0; t.tracks[i].cmplx.max = 3; }
                if (!t.tracks[i].entrpLocked) { t.tracks[i].entrp.min = 0; t.tracks[i].entrp.max = 10; }
            }
            else if (g == 24) {
                if (!t.tracks[i].cmplxLocked) { t.tracks[i].cmplx.min = 0; t.tracks[i].cmplx.max = 0; }
                if (!t.tracks[i].entrpLocked) { t.tracks[i].entrp.min = 0; t.tracks[i].entrp.max = 0; }
            }
            else if (g >= 22) {
                if (!t.tracks[i].cmplxLocked) { t.tracks[i].cmplx.min = 30; t.tracks[i].cmplx.max = 70; }
                if (!t.tracks[i].entrpLocked) { t.tracks[i].entrp.min = 0; t.tracks[i].entrp.max = 0; }
            }
            else {
                if (!t.tracks[i].cmplxLocked) { t.tracks[i].cmplx.min = 0; t.tracks[i].cmplx.max = 0; }
                if (!t.tracks[i].entrpLocked) { t.tracks[i].entrp.min = 0; t.tracks[i].entrp.max = 0; }
            }

            if (!t.tracks[i].shiftLocked) { t.tracks[i].shift.min = 0; t.tracks[i].shift.max = 0; }
        }
        updateTuningUIFromProcessor();
        };

    const char* tsLabels[8] = { "4/4", "3/4", "5/4", "7/8", "12/8", "13/8", "15/16", "5/8" };
    for (int i = 0; i < 8; ++i) {
        addChildComponent(tuningTsBtns[i]);
        tuningTsBtns[i].setButtonText(tsLabels[i]);
        tuningTsBtns[i].onClick = [this, i] { audioProcessor.userTuning[audioProcessor.currentGenre.load()].allowedTimeSigs[i] = tuningTsBtns[i].getToggleState(); };
    }

    const char* fillLabels[4] = { "Scatter", "Drop", "Euclid", "Roll" };
    for (int i = 0; i < 4; ++i) {
        addChildComponent(tuningFillBtns[i]);
        tuningFillBtns[i].setButtonText(fillLabels[i]);
        tuningFillBtns[i].onClick = [this, i] { audioProcessor.userTuning[audioProcessor.currentGenre.load()].allowedFills[i] = tuningFillBtns[i].getToggleState(); };
    }

    for (int t = 0; t < 8; ++t) {
        for (int d = 0; d < 8; ++d) {
            addChildComponent(tuningDivBtns[t][d]);
            tuningDivBtns[t][d].setButtonText(juce::String(d + 1));
            tuningDivBtns[t][d].setClickingTogglesState(true);
            tuningDivBtns[t][d].setColour(juce::TextButton::buttonOnColourId, juce::Colours::cyan.withAlpha(0.6f));
            tuningDivBtns[t][d].onClick = [this, t, d] {
                audioProcessor.userTuning[audioProcessor.currentGenre.load()].tracks[t].allowedDivs[d] = tuningDivBtns[t][d].getToggleState();
                };
        }

        addChildComponent(tuningDivLock[t]);
        tuningDivLock[t].setButtonText("L"); tuningDivLock[t].setClickingTogglesState(true); tuningDivLock[t].setColour(juce::TextButton::buttonOnColourId, juce::Colours::orange);
        tuningDivLock[t].onClick = [this, t] {
            bool state = tuningDivLock[t].getToggleState();
            audioProcessor.userTuning[audioProcessor.currentGenre.load()].tracks[t].divLocked = state;
            audioProcessor.trackDivLocked[t] = state;
            btnDivLock[t].setToggleState(state, juce::dontSendNotification);
            };

        setupNumBox(tuningCmplxMin[t], [this, t](int v) { audioProcessor.userTuning[audioProcessor.currentGenre.load()].tracks[t].cmplx.min = v; });
        setupNumBox(tuningCmplxMax[t], [this, t](int v) { audioProcessor.userTuning[audioProcessor.currentGenre.load()].tracks[t].cmplx.max = v; });
        addChildComponent(tuningCmplxLock[t]);
        tuningCmplxLock[t].setButtonText("L"); tuningCmplxLock[t].setClickingTogglesState(true); tuningCmplxLock[t].setColour(juce::TextButton::buttonOnColourId, juce::Colours::orange);
        tuningCmplxLock[t].onClick = [this, t] {
            bool state = tuningCmplxLock[t].getToggleState();
            audioProcessor.userTuning[audioProcessor.currentGenre.load()].tracks[t].cmplxLocked = state;
            audioProcessor.trackCmplxLocked[t] = state;
            btnCmplxLock[t].setToggleState(state, juce::dontSendNotification);
            };

        setupNumBox(tuningEntrpMin[t], [this, t](int v) { audioProcessor.userTuning[audioProcessor.currentGenre.load()].tracks[t].entrp.min = v; });
        setupNumBox(tuningEntrpMax[t], [this, t](int v) { audioProcessor.userTuning[audioProcessor.currentGenre.load()].tracks[t].entrp.max = v; });
        addChildComponent(tuningEntrpLock[t]);
        tuningEntrpLock[t].setButtonText("L"); tuningEntrpLock[t].setClickingTogglesState(true); tuningEntrpLock[t].setColour(juce::TextButton::buttonOnColourId, juce::Colours::orange);
        tuningEntrpLock[t].onClick = [this, t] {
            bool state = tuningEntrpLock[t].getToggleState();
            audioProcessor.userTuning[audioProcessor.currentGenre.load()].tracks[t].entrpLocked = state;
            audioProcessor.trackEntrpLocked[t] = state;
            btnEntrpLock[t].setToggleState(state, juce::dontSendNotification);
            };

        setupNumBox(tuningShiftMin[t], [this, t](int v) { audioProcessor.userTuning[audioProcessor.currentGenre.load()].tracks[t].shift.min = v; });
        setupNumBox(tuningShiftMax[t], [this, t](int v) { audioProcessor.userTuning[audioProcessor.currentGenre.load()].tracks[t].shift.max = v; });
        addChildComponent(tuningShiftLock[t]);
        tuningShiftLock[t].setButtonText("L"); tuningShiftLock[t].setClickingTogglesState(true); tuningShiftLock[t].setColour(juce::TextButton::buttonOnColourId, juce::Colours::orange);
        tuningShiftLock[t].onClick = [this, t] {
            bool state = tuningShiftLock[t].getToggleState();
            audioProcessor.userTuning[audioProcessor.currentGenre.load()].tracks[t].shiftLocked = state;
            audioProcessor.trackShiftLocked[t] = state;
            btnShiftLock[t].setToggleState(state, juce::dontSendNotification);
            };
    }

    styleMenu.setSelectedId(1, juce::sendNotification);

    juce::Component::SafePointer<AIDrumMachineAudioProcessorEditor> safeThis(this);
    generateButton.onClick = [safeThis, this] {
        if (safeThis == nullptr) return;
        audioProcessor.generateAllTracks();

        for (int i = 0; i < 8; ++i) {
            complexitySliders[i].setValue(audioProcessor.trackComplexity[i], juce::dontSendNotification);
            entropySliders[i].setValue(audioProcessor.trackEntropy[i], juce::dontSendNotification);
            shiftSliders[i].setValue(audioProcessor.trackShiftUI[i], juce::dontSendNotification);
            octaveSliders[i].setValue(audioProcessor.trackOctaveUI[i], juce::dontSendNotification);
        }

        updateTrackNames();
        currentViewBar = 0;
        updateTabColors();
        seqGrid.updateBar(0);
        resized();
        repaint();
        };
    updateViewVisibility(); startTimerHz(30);
}

AIDrumMachineAudioProcessorEditor::~AIDrumMachineAudioProcessorEditor() { stopTimer(); }

void AIDrumMachineAudioProcessorEditor::setupNumBox(juce::Label& lbl, std::function<void(int)> onChange) {
    addChildComponent(lbl);
    lbl.setEditable(true);
    lbl.setColour(juce::Label::backgroundColourId, juce::Colours::darkgrey);
    lbl.setColour(juce::Label::outlineColourId, juce::Colours::grey);
    lbl.setColour(juce::Label::textColourId, juce::Colours::white);
    lbl.setJustificationType(juce::Justification::centred);
    lbl.setFont(juce::Font(14.0f));
    lbl.onTextChange = [&lbl, onChange] { onChange(lbl.getText().getIntValue()); };
}

void AIDrumMachineAudioProcessorEditor::updateTuningUIFromProcessor() {
    int g = audioProcessor.currentGenre.load();
    const auto& t = audioProcessor.userTuning[g];

    tuningTempoMin.setText(juce::String(t.tempo.min), juce::dontSendNotification);
    tuningTempoMax.setText(juce::String(t.tempo.max), juce::dontSendNotification);
    tuningTempoLock.setToggleState(t.tempoLocked, juce::dontSendNotification);

    for (int i = 0; i < 8; ++i) tuningTsBtns[i].setToggleState(t.allowedTimeSigs[i], juce::dontSendNotification);
    for (int i = 0; i < 4; ++i) tuningFillBtns[i].setToggleState(t.allowedFills[i], juce::dontSendNotification);

    for (int i = 0; i < 8; ++i) {
        for (int d = 0; d < 8; ++d) {
            tuningDivBtns[i][d].setToggleState(t.tracks[i].allowedDivs[d], juce::dontSendNotification);
        }
        tuningDivLock[i].setToggleState(t.tracks[i].divLocked, juce::dontSendNotification);

        tuningCmplxMin[i].setText(juce::String(t.tracks[i].cmplx.min), juce::dontSendNotification);
        tuningCmplxMax[i].setText(juce::String(t.tracks[i].cmplx.max), juce::dontSendNotification);
        tuningCmplxLock[i].setToggleState(t.tracks[i].cmplxLocked, juce::dontSendNotification);

        tuningEntrpMin[i].setText(juce::String(t.tracks[i].entrp.min), juce::dontSendNotification);
        tuningEntrpMax[i].setText(juce::String(t.tracks[i].entrp.max), juce::dontSendNotification);
        tuningEntrpLock[i].setToggleState(t.tracks[i].entrpLocked, juce::dontSendNotification);

        tuningShiftMin[i].setText(juce::String(t.tracks[i].shift.min), juce::dontSendNotification);
        tuningShiftMax[i].setText(juce::String(t.tracks[i].shift.max), juce::dontSendNotification);
        tuningShiftLock[i].setToggleState(t.tracks[i].shiftLocked, juce::dontSendNotification);
    }
}

void AIDrumMachineAudioProcessorEditor::updateStyleMenu() {
    styleMenu.clear();
    if (audioProcessor.arpMode.load()) {
        const juce::StringArray arpGenres = {
            "0. Basic Up (3rds)", "1. Basic Down (4ths)", "2. Poly Triads", "3. Poly 7ths",
            "4. 5th Cascades", "5. 6th Leaps", "6. Quartal Harmony", "7. Polyrhythm Plucks",
            "8. Euclidean Arp", "9. Pop/Hit Melody Maker"
        };
        for (int i = 0; i < arpGenres.size(); ++i) styleMenu.addItem(arpGenres[i], i + 1);
    }
    else {
        const juce::StringArray genres = {
            "0. Techno (Detroit/Berlin)", "1. House (Deep/Acid)", "2. UK Garage (2-step)", "3. Drum & Bass / Jungle",
            "4. Trap", "5. Footwork / Juke", "6. IDM (Breakcore)", "7. Dubstep",
            "8. Afrobeat", "9. Gqom", "10. Amapiano", "11. Indian Classical",
            "12. Samba / Bossa Nova", "13. Reggaeton / Dembow", "14. Gamelan",
            "15. Funk (James Brown)", "16. New Jack Swing", "17. Neo Soul (J Dilla)",
            "18. Hip Hop (Boom Bap)", "19. Math Rock", "20. Progressive Metal", "21. Minimalism (Reich)",
            "22. Pure Euclidean (Math)", "23. Pure Chaos (Random)",
            "24. UK Drill", "25. Polyrhythm Matrix"
        };
        for (int i = 0; i < genres.size(); ++i) styleMenu.addItem(genres[i], i + 1);
    }
    styleMenu.setSelectedId(1, juce::sendNotification);
}

void AIDrumMachineAudioProcessorEditor::updateTrackNames() {
    if (audioProcessor.arpMode.load()) {
        for (int i = 0; i < 8; ++i) {
            trackNameLabels[i].setText(audioProcessor.getNoteName(i), juce::dontSendNotification);
        }
    }
    else {
        const auto& def = AIDrumMachineAudioProcessor::getGenreDef(audioProcessor.currentGenre.load());
        for (int i = 0; i < 8; ++i) {
            trackNameLabels[i].setText(def.trackNames[i], juce::dontSendNotification);
        }
    }
}

void AIDrumMachineAudioProcessorEditor::updateTimeSigNumMenu() {
    int maxNum = 32;
    int currentNum = audioProcessor.timeSigNumerator.load();
    if (currentNum < 1) currentNum = 4;
    if (currentNum > maxNum) {
        currentNum = maxNum;
        audioProcessor.timeSigNumerator.store(currentNum);
    }
    timeSigNumMenu.clear();
    for (int i = 1; i <= maxNum; ++i) timeSigNumMenu.addItem(juce::String(i), i);
    timeSigNumMenu.setSelectedId(currentNum, juce::dontSendNotification);
}

void AIDrumMachineAudioProcessorEditor::updateDivisionMenus() {
    int den = audioProcessor.timeSigDenominator.load();
    if (den < 1) den = 4;
    int maxDiv = (den == 16) ? 2 : ((den == 8) ? 4 : 8);

    for (int i = 0; i < 8; ++i) {
        divSelectors[i].clear();
        for (int d = 1; d <= maxDiv; ++d) divSelectors[i].addItem(juce::String(d), d);
        if (audioProcessor.trackDivisionsUI[i] > maxDiv) audioProcessor.trackDivisionsUI[i] = maxDiv;
        divSelectors[i].setSelectedId(audioProcessor.trackDivisionsUI[i], juce::dontSendNotification);
    }
}
void AIDrumMachineAudioProcessorEditor::updateViewVisibility() {
    bool isSeq = (currentView == SequencerView);
    bool isS1 = (currentView == Setup1View);
    bool isS2 = (currentView == Setup2View);
    bool isTuning = (currentView == TuningView);

    tabTuningButton.setEnabled(!audioProcessor.arpMode.load());
    btnArpMode.setEnabled(currentView != TuningView);

    tabSeqButton.setColour(juce::TextButton::buttonColourId, isSeq ? juce::Colours::orange : juce::Colours::darkgrey);
    tabSetupButton.setColour(juce::TextButton::buttonColourId, isS1 ? juce::Colours::orange : juce::Colours::darkgrey);
    tabSetup2Button.setColour(juce::TextButton::buttonColourId, isS2 ? juce::Colours::orange : juce::Colours::darkgrey);
    tabTuningButton.setColour(juce::TextButton::buttonColourId, isTuning ? juce::Colours::orange : juce::Colours::darkgrey);

    btnClearAll.setVisible(isSeq);
    btnTimeSigLock.setVisible(isSeq || isS1 || isS2);

    arpKeyMenu.setVisible(isS2);
    arpScaleMenu.setVisible(isS2);
    btnArpMono.setVisible(isS2);

    seqGrid.setVisible(isSeq); // ★ シーケンサーグリッドの表示切り替え

    for (int i = 0; i < 8; ++i) {
        btnMute[i].setVisible(isSeq); btnSolo[i].setVisible(isSeq); btnClear[i].setVisible(isSeq); btnShiftL[i].setVisible(isSeq); btnShiftR[i].setVisible(isSeq);

        divLabels[i].setVisible(isS1); divSelectors[i].setVisible(isS1); btnDivLock[i].setVisible(isS1);
        compLabels[i].setVisible(isS1); complexitySliders[i].setVisible(isS1); btnCmplxLock[i].setVisible(isS1);
        entrpLabels[i].setVisible(isS1); entropySliders[i].setVisible(isS1); btnEntrpLock[i].setVisible(isS1);
        shiftLabels[i].setVisible(isS1); shiftSliders[i].setVisible(isS1); btnShiftLock[i].setVisible(isS1);

        trackNameLabels[i].setEditable(!isS2);

        btnDegreeLock[i].setVisible(isS2);
        octaveLabels[i].setVisible(isS2);
        octaveSliders[i].setVisible(isS2);
        btnOctaveLock[i].setVisible(isS2);
        btnDynamic[i].setVisible(isS2);
        dynamicSliders[i].setVisible(isS2);

        midiKeyLabels[i].setVisible(isS1);

        for (int d = 0; d < 8; ++d) tuningDivBtns[i][d].setVisible(isTuning);
        tuningDivLock[i].setVisible(isTuning);

        tuningCmplxMin[i].setVisible(isTuning); tuningCmplxMax[i].setVisible(isTuning); tuningCmplxLock[i].setVisible(isTuning);
        tuningEntrpMin[i].setVisible(isTuning); tuningEntrpMax[i].setVisible(isTuning); tuningEntrpLock[i].setVisible(isTuning);
        tuningShiftMin[i].setVisible(isTuning); tuningShiftMax[i].setVisible(isTuning); tuningShiftLock[i].setVisible(isTuning);
    }
    for (int i = 0; i < 4; ++i) btnPattern[i].setVisible(isSeq);

    tuningTempoTitle.setVisible(isTuning);
    tuningTsTitle.setVisible(isTuning);
    tuningFillsTitle.setVisible(isTuning);
    tuningColDiv.setVisible(isTuning);
    tuningColCmplx.setVisible(isTuning);
    tuningColEntrp.setVisible(isTuning);
    tuningColShift.setVisible(isTuning);

    tuningTempoMin.setVisible(isTuning); tuningTempoMax.setVisible(isTuning); tuningTempoLock.setVisible(isTuning);

    btnTuningRandom.setVisible(isTuning);
    btnTuningClear.setVisible(isTuning);

    for (int i = 0; i < 8; ++i) tuningTsBtns[i].setVisible(isTuning);
    for (int i = 0; i < 4; ++i) tuningFillBtns[i].setVisible(isTuning);
}

void AIDrumMachineAudioProcessorEditor::updateTabColors() {
    tabButton1.setColour(juce::TextButton::buttonColourId, currentViewBar == 0 ? juce::Colours::orange : juce::Colours::darkgrey);
    tabButton2.setColour(juce::TextButton::buttonColourId, currentViewBar == 1 ? juce::Colours::orange : juce::Colours::darkgrey);
    tabButton3.setColour(juce::TextButton::buttonColourId, currentViewBar == 2 ? juce::Colours::orange : juce::Colours::darkgrey);
    tabButton4.setColour(juce::TextButton::buttonColourId, currentViewBar == 3 ? juce::Colours::orange : juce::Colours::darkgrey);
}

void AIDrumMachineAudioProcessorEditor::updatePatternButtonColors() {
    for (int i = 0; i < 4; ++i) {
        btnPattern[i].setColour(juce::TextButton::buttonColourId, audioProcessor.isPatternSaved[i] ? juce::Colours::cyan.withAlpha(0.6f) : juce::Colours::darkgrey);
    }
}

void AIDrumMachineAudioProcessorEditor::timerCallback() {
    if (audioProcessor.isSyncEnabled.load()) tempoLabel.setText("DAW: " + juce::String(audioProcessor.currentBpm.load(), 1) + " BPM", juce::dontSendNotification);
    else if (!tempoLabel.isBeingEdited()) tempoLabel.setText(juce::String(audioProcessor.internalTempo.load(), 1) + " BPM", juce::dontSendNotification);

    // ★ GUI更新が必要な場合のみレイアウト(resized)と全画面描画を走らせる
    if (audioProcessor.uiNeedsUpdate.exchange(false)) {
        timeSigDenMenu.setSelectedId(audioProcessor.timeSigDenominator.load(), juce::dontSendNotification);
        updateTimeSigNumMenu();
        timeSigNumMenu.setSelectedId(audioProcessor.timeSigNumerator.load(), juce::dontSendNotification);
        barCountMenu.setSelectedId(audioProcessor.globalBarCount.load(), juce::dontSendNotification);
        fillBarMenu.setSelectedId(audioProcessor.fillBarTarget.load() + 1, juce::dontSendNotification);

        for (int i = 0; i < 8; ++i) {
            updateDivisionMenus();
            divSelectors[i].setSelectedId(audioProcessor.trackDivisionsUI[i], juce::dontSendNotification);
            complexitySliders[i].setValue(audioProcessor.trackComplexity[i], juce::dontSendNotification);
            entropySliders[i].setValue(audioProcessor.trackEntropy[i], juce::dontSendNotification);
            shiftSliders[i].setValue(audioProcessor.trackShiftUI[i], juce::dontSendNotification);
            octaveSliders[i].setValue(audioProcessor.trackOctaveUI[i], juce::dontSendNotification);
            dynamicSliders[i].setValue(audioProcessor.trackDynamicAmount[i], juce::dontSendNotification);
        }
        updateTrackNames();
        resized();
        repaint();
        seqGrid.repaint();
    }

    if (audioProcessor.autoFollowEnabled.load() && (audioProcessor.isPlayingInternal.load() || audioProcessor.isSyncEnabled.load())) {
        int activeBar = audioProcessor.currentPlayingBar.load();
        if (activeBar != currentViewBar && activeBar < audioProcessor.globalBarCount.load() && activeBar >= 0) {
            currentViewBar = activeBar;
            updateTabColors();
            seqGrid.updateBar(activeBar);
        }
    }

    // ★ 最適化ポイント：再生中は重い全画面repaintを呼ばず、軽量なseqGridのみを更新する
    if (currentView == SequencerView && (audioProcessor.isPlayingInternal.load() || audioProcessor.isSyncEnabled.load())) {
        seqGrid.repaint();
    }
}

void AIDrumMachineAudioProcessorEditor::resized() {
    auto area = getLocalBounds().reduced(20);

    // ★ グリッドのオーバーレイ用。座標はEditorと同一で絶対座標参照させる
    seqGrid.setBounds(getLocalBounds());

    auto row1 = area.removeFromTop(30);
    syncButton.setBounds(row1.removeFromLeft(60)); playButton.setBounds(row1.removeFromLeft(60).reduced(2)); stopButton.setBounds(row1.removeFromLeft(60).reduced(2));
    row1.removeFromLeft(10); tempoLabel.setBounds(row1.removeFromLeft(80));
    btnTempoLock.setBounds(row1.removeFromLeft(25).reduced(2));
    row1.removeFromLeft(10);

    tabSeqButton.setBounds(row1.removeFromLeft(90).reduced(2));
    tabSetupButton.setBounds(row1.removeFromLeft(90).reduced(2));
    tabSetup2Button.setBounds(row1.removeFromLeft(90).reduced(2));
    tabTuningButton.setBounds(row1.removeFromLeft(90).reduced(2));

    row1.removeFromLeft(10); btnClearAll.setBounds(row1.removeFromLeft(80).reduced(2));

    row1.removeFromLeft(10);
    timeSigLabel.setBounds(row1.removeFromLeft(60));
    timeSigNumMenu.setBounds(row1.removeFromLeft(60).reduced(2));
    timeSigSlash.setBounds(row1.removeFromLeft(15));
    timeSigDenMenu.setBounds(row1.removeFromLeft(60).reduced(2));
    btnTimeSigLock.setBounds(row1.removeFromLeft(25).reduced(2));

    area.removeFromTop(10);
    auto row2 = area.removeFromTop(30);
    generateButton.setBounds(row2.removeFromLeft(120)); row2.removeFromLeft(10);
    styleMenu.setBounds(row2.removeFromLeft(200)); row2.removeFromLeft(10);
    fillBarMenu.setBounds(row2.removeFromLeft(100)); row2.removeFromLeft(10);
    btnAutoFollow.setBounds(row2.removeFromLeft(70)); row2.removeFromLeft(10);
    btnArpMode.setBounds(row2.removeFromLeft(80)); row2.removeFromLeft(10);

    dragAllArea = row2.removeFromLeft(100).reduced(2);

    row2.removeFromRight(10);
    barCountMenu.setBounds(row2.removeFromRight(80).reduced(2));
    barCountLabel.setBounds(row2.removeFromRight(40));

    statusLabel.setBounds(row2);

    auto tabArea = area.removeFromTop(40).withTrimmedTop(10);
    int tabW = tabArea.getWidth() / 4;

    int bars = audioProcessor.globalBarCount.load();
    tabButton1.setVisible(bars >= 1); tabButton2.setVisible(bars >= 2); tabButton3.setVisible(bars >= 3); tabButton4.setVisible(bars >= 4);

    if (bars >= 1) tabButton1.setBounds(tabArea.removeFromLeft(tabW).reduced(2));
    if (bars >= 2) tabButton2.setBounds(tabArea.removeFromLeft(tabW).reduced(2));
    if (bars >= 3) tabButton3.setBounds(tabArea.removeFromLeft(tabW).reduced(2));
    if (bars >= 4) tabButton4.setBounds(tabArea.removeFromLeft(tabW).reduced(2));

    if (currentView == SequencerView) {
        auto patArea = area.removeFromTop(40).withTrimmedTop(10).withTrimmedBottom(5);
        int patW = patArea.getWidth() / 4;
        for (int i = 0; i < 4; ++i) btnPattern[i].setBounds(patArea.removeFromLeft(patW).reduced(4, 0));
    }
    else {
        for (int i = 0; i < 4; ++i) btnPattern[i].setBounds(0, 0, 0, 0);
    }

    area.removeFromTop(10);
    juce::Rectangle<int> bottomAreaInt = area;

    if (currentView == TuningView) {
        seqGrid.lockArea = juce::Rectangle<int>(); seqGrid.midiDragArea = juce::Rectangle<int>();

        auto topTuningRow = bottomAreaInt.removeFromTop(90);

        auto tRow1 = topTuningRow.removeFromTop(30);

        tuningTempoTitle.setBounds(tRow1.removeFromLeft(140));
        tuningTempoMin.setBounds(tRow1.removeFromLeft(50).reduced(2));
        tuningTempoMax.setBounds(tRow1.removeFromLeft(50).reduced(2));
        tuningTempoLock.setBounds(tRow1.removeFromLeft(30).reduced(2));
        tRow1.removeFromLeft(10);
        btnTuningRandom.setBounds(tRow1.removeFromLeft(80).reduced(2));
        btnTuningClear.setBounds(tRow1.removeFromLeft(80).reduced(2));

        auto tRow2 = topTuningRow.removeFromTop(30);
        tuningTsTitle.setBounds(tRow2.removeFromLeft(140));
        for (int i = 0; i < 8; ++i) tuningTsBtns[i].setBounds(tRow2.removeFromLeft(65).reduced(2));

        auto tRow3 = topTuningRow.removeFromTop(30);
        tuningFillsTitle.setBounds(tRow3.removeFromLeft(140));
        for (int i = 0; i < 4; ++i) tuningFillBtns[i].setBounds(tRow3.removeFromLeft(75).reduced(2));

        auto headerRow = bottomAreaInt.removeFromTop(25);
        headerRow.removeFromLeft(110);
        tuningColDiv.setBounds(headerRow.removeFromLeft(270));
        tuningColCmplx.setBounds(headerRow.removeFromLeft(150));
        tuningColEntrp.setBounds(headerRow.removeFromLeft(150));
        tuningColShift.setBounds(headerRow.removeFromLeft(150));

        sampleArea = bottomAreaInt.removeFromLeft(110);
        auto setupControls = bottomAreaInt;
        int rows = 8; int cellH = sampleArea.getHeight() / rows;

        for (int row = 0; row < rows; ++row) {
            int idx = 7 - row;
            trackNameLabels[idx].setBounds(sampleArea.withY(sampleArea.getY() + row * cellH).withHeight(cellH).reduced(2));
            auto ctrlRow = setupControls.withY(setupControls.getY() + row * cellH).withHeight(cellH).reduced(2);

            auto divBlock = ctrlRow.removeFromLeft(270); divBlock.removeFromLeft(10);
            for (int d = 0; d < 8; ++d) { tuningDivBtns[idx][d].setBounds(divBlock.removeFromLeft(28).reduced(1)); }
            divBlock.removeFromLeft(5); tuningDivLock[idx].setBounds(divBlock.removeFromLeft(25).reduced(1));

            auto cmplxBlock = ctrlRow.removeFromLeft(150); cmplxBlock.removeFromLeft(10);
            tuningCmplxMin[idx].setBounds(cmplxBlock.removeFromLeft(45).reduced(2)); tuningCmplxMax[idx].setBounds(cmplxBlock.removeFromLeft(45).reduced(2)); tuningCmplxLock[idx].setBounds(cmplxBlock.removeFromLeft(25).reduced(1));

            auto entrpBlock = ctrlRow.removeFromLeft(150); entrpBlock.removeFromLeft(10);
            tuningEntrpMin[idx].setBounds(entrpBlock.removeFromLeft(45).reduced(2)); tuningEntrpMax[idx].setBounds(entrpBlock.removeFromLeft(45).reduced(2)); tuningEntrpLock[idx].setBounds(entrpBlock.removeFromLeft(25).reduced(1));

            auto shiftBlock = ctrlRow.removeFromLeft(150); shiftBlock.removeFromLeft(10);
            tuningShiftMin[idx].setBounds(shiftBlock.removeFromLeft(45).reduced(2)); tuningShiftMax[idx].setBounds(shiftBlock.removeFromLeft(45).reduced(2)); tuningShiftLock[idx].setBounds(shiftBlock.removeFromLeft(25).reduced(1));
        }
    }
    else if (currentView == Setup2View) {
        auto s2top = bottomAreaInt.removeFromTop(30);
        arpKeyMenu.setBounds(s2top.removeFromLeft(100).reduced(2));
        arpScaleMenu.setBounds(s2top.removeFromLeft(150).reduced(2));
        btnArpMono.setBounds(s2top.removeFromLeft(100).reduced(2));
        bottomAreaInt.removeFromTop(10);

        seqGrid.lockArea = juce::Rectangle<int>(); seqGrid.midiDragArea = bottomAreaInt.removeFromRight(40);
        auto labelArea = bottomAreaInt.removeFromLeft(140);
        sampleArea = bottomAreaInt.removeFromLeft(110);
        auto setupControls = bottomAreaInt;
        int rows = 8; int cellH = sampleArea.getHeight() / rows;
        for (int row = 0; row < rows; ++row) {
            int idx = 7 - row;
            auto lRow = labelArea.withY(labelArea.getY() + row * cellH).withHeight(cellH).reduced(2);
            trackNameLabels[idx].setBounds(lRow.removeFromLeft(60));
            btnDegreeLock[idx].setBounds(lRow.removeFromLeft(25).reduced(2));

            auto ctrlRow = setupControls.withY(setupControls.getY() + row * cellH).withHeight(cellH).reduced(2);
            ctrlRow.removeFromLeft(10);

            octaveLabels[idx].setBounds(ctrlRow.removeFromLeft(50));
            octaveSliders[idx].setBounds(ctrlRow.removeFromLeft(120));
            btnOctaveLock[idx].setBounds(ctrlRow.removeFromLeft(25).reduced(2));
            ctrlRow.removeFromLeft(15);

            btnDynamic[idx].setBounds(ctrlRow.removeFromLeft(45).reduced(2));
            dynamicSliders[idx].setBounds(ctrlRow.removeFromLeft(120));
        }
    }
    else if (currentView == SequencerView) {
        // ★ 分離したシーケンサーグリッドへ座標を渡す
        seqGrid.lockArea = bottomAreaInt.removeFromLeft(30);
        auto controlArea = bottomAreaInt.removeFromLeft(150);
        sampleArea = bottomAreaInt.removeFromLeft(110);

        int rows = 8; int cellH = sampleArea.getHeight() / rows;
        for (int row = 0; row < rows; ++row) {
            int idx = 7 - row;
            trackNameLabels[idx].setBounds(sampleArea.withY(sampleArea.getY() + row * cellH).withHeight(cellH).removeFromLeft(105).reduced(2));
            auto ctrlRow = controlArea.withY(controlArea.getY() + row * cellH).withHeight(cellH).reduced(1);
            btnMute[idx].setBounds(ctrlRow.removeFromLeft(30).reduced(1)); btnSolo[idx].setBounds(ctrlRow.removeFromLeft(30).reduced(1)); btnClear[idx].setBounds(ctrlRow.removeFromLeft(30).reduced(1)); btnShiftL[idx].setBounds(ctrlRow.removeFromLeft(30).reduced(1)); btnShiftR[idx].setBounds(ctrlRow.removeFromLeft(30).reduced(1));
        }
        seqGrid.midiDragArea = bottomAreaInt.removeFromRight(40);
        seqGrid.mainGridArea = bottomAreaInt;
    }
    else if (currentView == Setup1View) {
        seqGrid.lockArea = juce::Rectangle<int>(); seqGrid.midiDragArea = bottomAreaInt.removeFromRight(40);
        auto labelArea = bottomAreaInt.removeFromLeft(140);
        sampleArea = bottomAreaInt.removeFromLeft(110);

        auto setupControls = bottomAreaInt;
        int rows = 8; int cellH = sampleArea.getHeight() / rows;
        for (int row = 0; row < rows; ++row) {
            int idx = 7 - row;
            auto lRow = labelArea.withY(labelArea.getY() + row * cellH).withHeight(cellH).reduced(2);
            midiKeyLabels[idx].setBounds(lRow.removeFromLeft(40)); trackNameLabels[idx].setBounds(lRow);

            auto ctrlRow = setupControls.withY(setupControls.getY() + row * cellH).withHeight(cellH).reduced(2);
            ctrlRow.removeFromLeft(10);
            divLabels[idx].setBounds(ctrlRow.removeFromLeft(30)); divSelectors[idx].setBounds(ctrlRow.removeFromLeft(55));
            ctrlRow.removeFromLeft(5); btnDivLock[idx].setBounds(ctrlRow.removeFromLeft(20).reduced(1));

            int remainingWidth = ctrlRow.getWidth(); int blockWidth = remainingWidth / 3;

            auto cmplxBlock = ctrlRow.removeFromLeft(blockWidth);
            cmplxBlock.removeFromLeft(15); compLabels[idx].setBounds(cmplxBlock.removeFromLeft(45));
            btnCmplxLock[idx].setBounds(cmplxBlock.removeFromRight(20).reduced(1));
            cmplxBlock.removeFromRight(5); complexitySliders[idx].setBounds(cmplxBlock);

            auto entrpBlock = ctrlRow.removeFromLeft(blockWidth);
            entrpBlock.removeFromLeft(15); entrpLabels[idx].setBounds(entrpBlock.removeFromLeft(45));
            btnEntrpLock[idx].setBounds(entrpBlock.removeFromRight(20).reduced(1));
            entrpBlock.removeFromRight(5); entropySliders[idx].setBounds(entrpBlock);

            auto shiftBlock = ctrlRow;
            shiftBlock.removeFromLeft(15); shiftLabels[idx].setBounds(shiftBlock.removeFromLeft(40));
            btnShiftLock[idx].setBounds(shiftBlock.removeFromRight(20).reduced(1));
            shiftBlock.removeFromRight(5); shiftSliders[idx].setBounds(shiftBlock);
        }
        seqGrid.mainGridArea = juce::Rectangle<int>();
    }
}

void AIDrumMachineAudioProcessorEditor::paint(juce::Graphics& g) {
    g.fillAll(juce::Colours::darkgrey);

    g.setColour(juce::Colours::cyan.withAlpha(0.6f)); g.fillRoundedRectangle(dragAllArea.toFloat(), 4.0f);
    g.setColour(juce::Colours::white); g.setFont(14.0f); g.drawText("DragMIDI", dragAllArea, juce::Justification::centred, false);

    // =========================================================================
    // ★ ロゴの代わりに「RhythmMatrix」と太字の白色テキストで描画
    // =========================================================================
    auto textArea = juce::Rectangle<float>(
        dragAllArea.getRight() + 15.0f, // DragMIDIボタンの右から15px空ける
        dragAllArea.getY(),             // 高さはDragMIDIボタンと合わせる
        200.0f,                         // 表示幅を確保
        dragAllArea.getHeight()
    );
    g.setColour(juce::Colours::white);
    g.setFont(juce::Font(24.0f, juce::Font::bold)); // ★ 22pxの太字(ボールド)に設定
    g.drawText("Rhythm Matrix", textArea, juce::Justification::centredLeft, false);
    // =========================================================================

    // ★ グリッドの描画は seqGrid.paint() に任せるため、ここでは不要になりました

    if (currentView == Setup1View) {
        int rows = 8; float cellH = sampleArea.getHeight() / (float)juce::jmax(1, rows);
        for (int row = 0; row < rows; ++row) {
            int idx = 7 - row; juce::Rectangle<float> sArea(sampleArea.getX(), sampleArea.getY() + row * cellH + 2, sampleArea.getWidth() - 4, cellH - 4);
            g.setColour(audioProcessor.hasSampleLoaded(idx) ? juce::Colours::lightblue.withAlpha(0.2f) : juce::Colours::darkgrey.darker());
            g.fillRoundedRectangle(sArea, 4.0f); g.setColour(juce::Colours::grey); g.drawText(audioProcessor.hasSampleLoaded(idx) ? "Sample Loaded" : "Drop Sample", sArea, juce::Justification::centred);
        }
    }
}

void AIDrumMachineAudioProcessorEditor::mouseDown(const juce::MouseEvent& e) {
    auto pos = e.getPosition();

    // ★ 右クリックでのサンプル削除処理のみ残し、グリッドのクリック判定は seqGrid.mouseDown へ委譲
    if (e.mods.isRightButtonDown() && sampleArea.contains(pos) && currentView != Setup2View && currentView != TuningView) {
        int idx = getTrackIndexFromMouseY(pos.y);
        if (idx >= 0 && idx < 8 && audioProcessor.hasSampleLoaded(idx)) {
            juce::NativeMessageBox::showOkCancelBox(juce::MessageBoxIconType::WarningIcon, "Delete", "Delete sample?", this, juce::ModalCallbackFunction::create([this, idx](int r) { if (r == 1) { audioProcessor.clearSample(idx); updateTrackNames(); repaint(); } }));
        }
        return;
    }
}

void AIDrumMachineAudioProcessorEditor::mouseDrag(const juce::MouseEvent& e) {
    if (e.mouseWasDraggedSinceMouseDown() && !isDragging) {
        isDragging = true; auto startPos = e.getMouseDownPosition(); int track = -2;
        if (dragAllArea.contains(startPos)) track = -1;
        else if (seqGrid.midiDragArea.contains(startPos)) track = 7 - (int)((startPos.y - seqGrid.midiDragArea.getY()) / juce::jmax(1, seqGrid.midiDragArea.getHeight() / 8));
        if (track >= -1 && track < 8) {
            juce::File f = exportMidi(track);
            if (f.existsAsFile()) { juce::StringArray s; s.add(f.getFullPathName()); performExternalDragDropOfFiles(s, false, this); }
        }
    }
}

void AIDrumMachineAudioProcessorEditor::mouseUp(const juce::MouseEvent& e) { isDragging = false; }

bool AIDrumMachineAudioProcessorEditor::isInterestedInFileDrag(const juce::StringArray& files) {
    if (currentView == Setup2View || currentView == TuningView) return false;
    for (const auto& f : files) if (f.endsWithIgnoreCase(".wav") || f.endsWithIgnoreCase(".mp3") || f.endsWithIgnoreCase(".aif")) return true; return false;
}

void AIDrumMachineAudioProcessorEditor::filesDropped(const juce::StringArray& files, int x, int y) {
    if (currentView == Setup2View || currentView == TuningView) return;
    int idx = getTrackIndexFromMouseY(y);
    if (idx >= 0 && idx < 8) { for (const auto& f : files) { audioProcessor.loadSample(idx, f); trackNameLabels[idx].setText(juce::File(f).getFileNameWithoutExtension(), juce::dontSendNotification); repaint(); break; } }
}

void AIDrumMachineAudioProcessorEditor::fileDragEnter(const juce::StringArray& f, int x, int y) {}
void AIDrumMachineAudioProcessorEditor::fileDragMove(const juce::StringArray& f, int x, int y) {}
void AIDrumMachineAudioProcessorEditor::fileDragExit(const juce::StringArray& f) {}

int AIDrumMachineAudioProcessorEditor::getTrackIndexFromMouseY(int y) {
    return 7 - (int)((y - sampleArea.getY()) / juce::jmax(1, sampleArea.getHeight() / 8));
}

// ==============================================================================
// ★ MIDI Export: 安全な境界チェックを搭載したエクスポート処理
// ==============================================================================
juce::File AIDrumMachineAudioProcessorEditor::exportMidi(int trackIndex) {
    juce::MidiMessageSequence seq;
    int notes[8] = { 36, 38, 42, 46, 39, 41, 45, 50 };
    int startT = (trackIndex == -1) ? 0 : trackIndex;
    int endT = (trackIndex == -1) ? 7 : trackIndex;
    double ppq = 960.0;
    int den = audioProcessor.timeSigDenominator.load();
    if (den < 1) den = 4;
    double ticksPerBeat = ppq * (4.0 / (double)den);

    bool isArp = audioProcessor.arpMode.load();
    int curScale = audioProcessor.arpScale.load();
    int curKey = audioProcessor.arpKey.load();

    for (int t = startT; t <= endT; ++t) {
        int div = audioProcessor.trackDivisionsUI[t];
        if (div < 1) div = 1;
        double ticksPerStep = ticksPerBeat / (double)div;
        double shiftOffsetTicks = (audioProcessor.trackShiftUI[t] / 100.0) * ticksPerStep;

        int totalSteps = div * audioProcessor.timeSigNumerator.load() * audioProcessor.globalBarCount.load();
        totalSteps = juce::jlimit(1, 1024, totalSteps);

        int note = notes[t];
        if (isArp) {
            int offset = scalePatterns[curScale][audioProcessor.trackDegreeUI[t]];
            if (offset != -1) {
                note = 60 + curKey + (audioProcessor.trackOctaveUI[t] * 12) + offset;
            }
        }
        note = juce::jlimit(0, 127, note);

        for (int s = 0; s < totalSteps; ++s) {
            int vel = audioProcessor.drumPatternUI[t][s];
            if (vel > 0) {
                double start = (s * ticksPerStep) + shiftOffsetTicks;
                if (start < 0.0) start = 0.0;

                seq.addEvent(juce::MidiMessage::noteOn(10, note, (juce::uint8)vel), start);
                seq.addEvent(juce::MidiMessage::noteOff(10, note), start + ticksPerStep * 0.85);
            }
        }
    }

    seq.updateMatchedPairs();
    juce::MidiFile mf;
    mf.setTicksPerQuarterNote((short)ppq);
    mf.addTrack(seq);
    juce::File f = juce::File::getSpecialLocation(juce::File::tempDirectory).getChildFile("DrumPattern.mid");
    f.deleteFile();
    juce::FileOutputStream os(f);
    mf.writeTo(os);
    os.flush();
    return f;
}