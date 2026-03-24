#include "PluginProcessor.h"
#include "PluginEditor.h"

AIDrumMachineAudioProcessorEditor::AIDrumMachineAudioProcessorEditor(AIDrumMachineAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    setSize(600, 400);

    addAndMakeVisible(generateButton);
    addAndMakeVisible(statusLabel);

    // タブボタンの追加
    addAndMakeVisible(tabButton1);
    addAndMakeVisible(tabButton2);
    addAndMakeVisible(tabButton3);
    addAndMakeVisible(tabButton4);

    statusLabel.setJustificationType(juce::Justification::centredLeft);
    statusLabel.setText("Click to generate 4-Bar Polyrhythm", juce::dontSendNotification);

    // タブクリック時の動作（表示ページの切り替え）
    auto tabClick = [this](int barIndex) {
        currentViewBar = barIndex;
        updateTabColors();
        repaint();
        };
    tabButton1.onClick = [tabClick] { tabClick(0); };
    tabButton2.onClick = [tabClick] { tabClick(1); };
    tabButton3.onClick = [tabClick] { tabClick(2); };
    tabButton4.onClick = [tabClick] { tabClick(3); };

    updateTabColors(); // 初期のタブ色を設定

    juce::Component::SafePointer<AIDrumMachineAudioProcessorEditor> safeThis(this);

    generateButton.onClick = [safeThis, this]
        {
            if (safeThis == nullptr) return;
            statusLabel.setText("Requesting Gemini (4 Bars)...", juce::dontSendNotification);

            // ★ここに成功したAPIキーを再度貼り付けてください
            juce::String myKey = "AIzaSyBT2vQXyacUMdmNOF2OjkYYQ_OPtJgORtQ";

            gemini.fetchDrumPattern("Generate a chaotic and evolving polyrhythmic techno beat for 4 bars. Use divisions like 5 or 7 for hi-hats.", myKey);
        };

    gemini.onSuccess = [safeThis, this](const juce::var& data)
        {
            if (safeThis == nullptr) return;
            safeThis->statusLabel.setText("SUCCESS! 4-Bars Polyrhythm received.", juce::dontSendNotification);

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
                            int totalSteps = div * 4; // 4小節分の総ステップ数
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
    // 上部のボタンやタブエリアを避けてグリッド領域を確保
    auto gridArea = area.removeFromBottom(250);

    int rows = 8;
    float cellH = static_cast<float>(gridArea.getHeight()) / rows;

    for (int row = 0; row < rows; ++row)
    {
        int trackIndex = (rows - 1) - row;
        int div = audioProcessor.trackDivisions[trackIndex];
        if (div < 1) div = 1;

        float cellW = static_cast<float>(gridArea.getWidth()) / div;

        for (int col = 0; col < div; ++col)
        {
            juce::Rectangle<float> cell(gridArea.getX() + col * cellW,
                gridArea.getY() + row * cellH,
                cellW - 2.0f,
                cellH - 2.0f);

            // ★修正: 現在見ているタブ(小節)に応じたデータ配列の位置を計算
            int globalStep = (currentViewBar * div) + col;
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

        // --- ポリリズム用プレイヘッド（現在選択中のタブにいる時だけ描画） ---
        int currentStep = audioProcessor.getTrackCurrentStep(trackIndex);

        // プレイヘッドが現在見ているBarの範囲内にあるかチェック
        int startStepOfThisBar = currentViewBar * div;
        int endStepOfThisBar = startStepOfThisBar + div;

        if (currentStep >= startStepOfThisBar && currentStep < endStepOfThisBar)
        {
            int localStep = currentStep - startStepOfThisBar; // タブ内での相対位置
            float playheadX = gridArea.getX() + localStep * cellW;
            juce::Rectangle<float> playheadRect(playheadX,
                gridArea.getY() + row * cellH,
                cellW - 2.0f,
                cellH - 2.0f);

            g.setColour(juce::Colours::white.withAlpha(0.4f));
            g.fillRoundedRectangle(playheadRect, 4.0f);
        }
    }
}

void AIDrumMachineAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced(20);

    // 上部コントロール（Generateボタンとラベル）
    auto topControls = area.removeFromTop(40);
    generateButton.setBounds(topControls.removeFromLeft(180));
    statusLabel.setBounds(topControls.removeFromRight(200));

    // タブボタンのレイアウト
    auto tabArea = area.removeFromTop(40).withTrimmedTop(10);
    int tabW = tabArea.getWidth() / 4;
    tabButton1.setBounds(tabArea.removeFromLeft(tabW).reduced(2));
    tabButton2.setBounds(tabArea.removeFromLeft(tabW).reduced(2));
    tabButton3.setBounds(tabArea.removeFromLeft(tabW).reduced(2));
    tabButton4.setBounds(tabArea.removeFromLeft(tabW).reduced(2));
}