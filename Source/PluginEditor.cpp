#include "PluginProcessor.h"
#include "PluginEditor.h"

AIDrumMachineAudioProcessorEditor::AIDrumMachineAudioProcessorEditor(AIDrumMachineAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    setSize(600, 400);

    addAndMakeVisible(generateButton);
    addAndMakeVisible(styleMenu);
    addAndMakeVisible(statusLabel);

    // ★追加：3つ目の選択肢として「オフライン・テストモード」を追加
    styleMenu.addItem("Chaotic Polyrhythm (Div 1-9)", 1);
    styleMenu.addItem("Standard Techno (Div 4 Only)", 2);
    styleMenu.addItem("Offline Random (No API)", 3);

    // UI検証がしやすいように、デフォルトをOfflineに設定しておきます
    styleMenu.setSelectedId(3);

    addAndMakeVisible(tabButton1);
    addAndMakeVisible(tabButton2);
    addAndMakeVisible(tabButton3);
    addAndMakeVisible(tabButton4);

    statusLabel.setJustificationType(juce::Justification::centredLeft);
    statusLabel.setText("Select style & Click Generate", juce::dontSendNotification);

    auto tabClick = [this](int barIndex) {
        currentViewBar = barIndex;
        updateTabColors();
        repaint();
        };
    tabButton1.onClick = [tabClick] { tabClick(0); };
    tabButton2.onClick = [tabClick] { tabClick(1); };
    tabButton3.onClick = [tabClick] { tabClick(2); };
    tabButton4.onClick = [tabClick] { tabClick(3); };

    updateTabColors();

    juce::Component::SafePointer<AIDrumMachineAudioProcessorEditor> safeThis(this);

    generateButton.onClick = [safeThis, this]
        {
            if (safeThis == nullptr) return;

            // ★追加：オフラインモードが選ばれている場合の処理
            if (styleMenu.getSelectedId() == 3)
            {
                statusLabel.setText("SUCCESS! Offline Random generated.", juce::dontSendNotification);

                juce::Random random;
                for (int i = 0; i < 8; ++i)
                {
                    // 1〜9のランダムな分割数を決定
                    int div = random.nextInt(juce::Range<int>(1, 10));
                    safeThis->audioProcessor.trackDivisions[i] = div;

                    int totalSteps = div * 4;
                    for (int j = 0; j < 36; ++j)
                    {
                        if (j < totalSteps) {
                            // 30%の確率でノートをオンにする（ベロシティ80〜100）
                            if (random.nextFloat() > 0.7f) {
                                safeThis->audioProcessor.drumPattern[i][j] = random.nextInt(juce::Range<int>(80, 101));
                            }
                            else {
                                safeThis->audioProcessor.drumPattern[i][j] = 0;
                            }
                        }
                        else {
                            // 使用しない余剰バッファは0で初期化
                            safeThis->audioProcessor.drumPattern[i][j] = 0;
                        }
                    }
                }

                safeThis->currentViewBar = 0;
                safeThis->updateTabColors();
                safeThis->resized();
                safeThis->repaint();
            }
            else
            {
                // 今までのAPI通信モード
                statusLabel.setText("Requesting Gemini (4 Bars)...", juce::dontSendNotification);

                // ★注意：再度ご自身のAPIキーに書き換えてください！
                juce::String myKey = "AIzaSyBT2vQXyacUMdmNOF2OjkYYQ_OPtJgORtQ";

                juce::String userPrompt = "";
                if (styleMenu.getSelectedId() == 1) {
                    userPrompt = "Generate a chaotic and evolving polyrhythmic techno beat for 4 bars. Use divisions like 5 or 7 for hi-hats.";
                }
                else {
                    userPrompt = "Generate a standard 4-on-the-floor techno beat for 4 bars. CRITICAL INSTRUCTION: You MUST set 'division': 4 for ALL 8 tracks. Do NOT use any division other than 4. The 'pattern' array for each track MUST have exactly 16 items.";
                }

                gemini.fetchDrumPattern(userPrompt, myKey);
            }
        };

    gemini.onSuccess = [safeThis, this](const juce::var& data)
        {
            if (safeThis == nullptr) return;
            safeThis->statusLabel.setText("SUCCESS! Rhythm received.", juce::dontSendNotification);

            for (int i = 0; i < 8; ++i)
            {
                juce::Identifier trackKey("track" + juce::String(i + 1));

                if (data.hasProperty(trackKey))
                {
                    auto trackObj = data[trackKey];
                    juce::Identifier divKey("division");
                    juce::Identifier patKey("pattern");

                    if (trackObj.hasProperty(divKey) && trackObj.hasProperty(patKey))
                    {
                        int div = static_cast<int>(trackObj[divKey]);
                        if (div < 1) div = 1;
                        if (div > 9) div = 9;

                        safeThis->audioProcessor.trackDivisions[i] = div;

                        auto* patternArray = trackObj[patKey].getArray();
                        if (patternArray != nullptr)
                        {
                            int totalSteps = div * 4;
                            for (int j = 0; j < 36; ++j)
                            {
                                if (j < totalSteps && j < patternArray->size()) {
                                    int velocity = static_cast<int>(patternArray->getReference(j));
                                    safeThis->audioProcessor.drumPattern[i][j] = velocity;
                                }
                                else {
                                    safeThis->audioProcessor.drumPattern[i][j] = 0;
                                }
                            }
                        }
                    }
                }
            }

            safeThis->currentViewBar = 0;
            safeThis->updateTabColors();
            safeThis->resized();
            safeThis->repaint();
        };

    gemini.onError = [safeThis](const juce::String& err)
        {
            if (safeThis != nullptr)
                safeThis->statusLabel.setText("Error: " + err, juce::dontSendNotification);
        };

    startTimerHz(30);
}

AIDrumMachineAudioProcessorEditor::~AIDrumMachineAudioProcessorEditor()
{
    stopTimer();
}

bool AIDrumMachineAudioProcessorEditor::needsPagination() const
{
    for (int i = 0; i < 8; ++i) {
        if (audioProcessor.trackDivisions[i] >= 5) {
            return true;
        }
    }
    return false;
}

void AIDrumMachineAudioProcessorEditor::updateTabColors()
{
    tabButton1.setColour(juce::TextButton::buttonColourId, currentViewBar == 0 ? juce::Colours::orange : juce::Colours::darkgrey);
    tabButton2.setColour(juce::TextButton::buttonColourId, currentViewBar == 1 ? juce::Colours::orange : juce::Colours::darkgrey);
    tabButton3.setColour(juce::TextButton::buttonColourId, currentViewBar == 2 ? juce::Colours::orange : juce::Colours::darkgrey);
    tabButton4.setColour(juce::TextButton::buttonColourId, currentViewBar == 3 ? juce::Colours::orange : juce::Colours::darkgrey);
}

void AIDrumMachineAudioProcessorEditor::timerCallback()
{
    repaint();
}

void AIDrumMachineAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::darkgrey);

    auto area = getLocalBounds().reduced(20);
    auto gridArea = area.removeFromBottom(250);

    int rows = 8;
    float cellH = static_cast<float>(gridArea.getHeight()) / rows;

    bool paginate = needsPagination();

    for (int row = 0; row < rows; ++row)
    {
        int trackIndex = (rows - 1) - row;
        int div = audioProcessor.trackDivisions[trackIndex];
        if (div < 1) div = 1;

        int colsToDraw = paginate ? div : (div * 4);
        float cellW = static_cast<float>(gridArea.getWidth()) / colsToDraw;

        for (int col = 0; col < colsToDraw; ++col)
        {
            juce::Rectangle<float> cell(gridArea.getX() + col * cellW,
                gridArea.getY() + row * cellH,
                cellW - 2.0f,
                cellH - 2.0f);

            int globalStep = paginate ? ((currentViewBar * div) + col) : col;
            int velocity = audioProcessor.drumPattern[trackIndex][globalStep];

            if (velocity > 0)
            {
                float alpha = 0.3f + 0.7f * (velocity / 100.0f);
                g.setColour(juce::Colours::orange.withAlpha(alpha));
            }
            else
            {
                g.setColour(juce::Colours::black.withAlpha(0.4f));
            }

            g.fillRoundedRectangle(cell, 4.0f);
        }

        int currentStep = audioProcessor.getTrackCurrentStep(trackIndex);

        if (paginate)
        {
            int startStepOfThisBar = currentViewBar * div;
            int endStepOfThisBar = startStepOfThisBar + div;

            if (currentStep >= startStepOfThisBar && currentStep < endStepOfThisBar)
            {
                int localStep = currentStep - startStepOfThisBar;
                float playheadX = gridArea.getX() + localStep * cellW;
                juce::Rectangle<float> playheadRect(playheadX, gridArea.getY() + row * cellH, cellW - 2.0f, cellH - 2.0f);

                g.setColour(juce::Colours::white.withAlpha(0.4f));
                g.fillRoundedRectangle(playheadRect, 4.0f);
            }
        }
        else
        {
            if (currentStep < colsToDraw)
            {
                float playheadX = gridArea.getX() + currentStep * cellW;
                juce::Rectangle<float> playheadRect(playheadX, gridArea.getY() + row * cellH, cellW - 2.0f, cellH - 2.0f);

                g.setColour(juce::Colours::white.withAlpha(0.4f));
                g.fillRoundedRectangle(playheadRect, 4.0f);
            }
        }
    }
}

void AIDrumMachineAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced(20);

    auto topControls = area.removeFromTop(30);
    generateButton.setBounds(topControls.removeFromLeft(120));
    topControls.removeFromLeft(10);
    styleMenu.setBounds(topControls.removeFromLeft(200));
    topControls.removeFromLeft(10);
    statusLabel.setBounds(topControls);

    bool paginate = needsPagination();

    tabButton1.setVisible(paginate);
    tabButton2.setVisible(paginate);
    tabButton3.setVisible(paginate);
    tabButton4.setVisible(paginate);

    if (paginate)
    {
        auto tabArea = area.removeFromTop(40).withTrimmedTop(10);
        int tabW = tabArea.getWidth() / 4;
        tabButton1.setBounds(tabArea.removeFromLeft(tabW).reduced(2));
        tabButton2.setBounds(tabArea.removeFromLeft(tabW).reduced(2));
        tabButton3.setBounds(tabArea.removeFromLeft(tabW).reduced(2));
        tabButton4.setBounds(tabArea.removeFromLeft(tabW).reduced(2));
    }
}

void AIDrumMachineAudioProcessorEditor::mouseDown(const juce::MouseEvent& e)
{
    auto area = getLocalBounds().reduced(20);
    auto gridArea = area.removeFromBottom(250);

    if (gridArea.contains(e.getPosition()))
    {
        int rows = 8;
        float cellH = static_cast<float>(gridArea.getHeight()) / rows;

        int row = static_cast<int>((e.y - gridArea.getY()) / cellH);
        if (row < 0 || row >= rows) return;

        int trackIndex = (rows - 1) - row;
        int div = audioProcessor.trackDivisions[trackIndex];
        if (div < 1) div = 1;

        bool paginate = needsPagination();
        int colsToDraw = paginate ? div : (div * 4);
        float cellW = static_cast<float>(gridArea.getWidth()) / colsToDraw;

        int col = static_cast<int>((e.x - gridArea.getX()) / cellW);
        if (col < 0 || col >= colsToDraw) return;

        int globalStep = paginate ? ((currentViewBar * div) + col) : col;

        if (audioProcessor.drumPattern[trackIndex][globalStep] == 0) {
            audioProcessor.drumPattern[trackIndex][globalStep] = 100;
        }
        else {
            audioProcessor.drumPattern[trackIndex][globalStep] = 0;
        }

        repaint();
    }
}