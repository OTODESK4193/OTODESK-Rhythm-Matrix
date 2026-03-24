#include "PluginProcessor.h"
#include "PluginEditor.h"

AIDrumMachineAudioProcessorEditor::AIDrumMachineAudioProcessorEditor(AIDrumMachineAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    setSize(600, 400);

    addAndMakeVisible(generateButton);
    addAndMakeVisible(statusLabel);

    statusLabel.setJustificationType(juce::Justification::centredLeft);
    statusLabel.setText("Click to test Gemini API", juce::dontSendNotification);

    juce::Component::SafePointer<AIDrumMachineAudioProcessorEditor> safeThis(this);

    generateButton.onClick = [safeThis, this]
        {
            if (safeThis == nullptr) return;
            statusLabel.setText("Requesting Gemini...", juce::dontSendNotification);

            // ★ここに成功したAPIキーを貼り付けてください
            juce::String myKey = "AIzaSyBT2vQXyacUMdmNOF2OjkYYQ_OPtJgORtQ";

            gemini.fetchDrumPattern("Fast Techno Beat", myKey);
        };

    gemini.onSuccess = [safeThis, this](const juce::var& data)
        {
            if (safeThis == nullptr) return;
            safeThis->statusLabel.setText("SUCCESS! Rhythm received.", juce::dontSendNotification);

            if (data.hasProperty("tracks"))
            {
                auto* tracks = data["tracks"].getArray();
                if (tracks != nullptr)
                {
                    int numTracks = juce::jmin(8, tracks->size());
                    for (int i = 0; i < numTracks; ++i)
                    {
                        auto& track = tracks->getReference(i);
                        if (track.hasProperty("pattern"))
                        {
                            auto* pattern = track["pattern"].getArray();
                            if (pattern != nullptr)
                            {
                                int numSteps = juce::jmin(16, pattern->size());
                                for (int j = 0; j < numSteps; ++j)
                                {
                                    int val = static_cast<int>(pattern->getReference(j));
                                    safeThis->drumPattern[i][j] = val;
                                    safeThis->audioProcessor.drumPattern[i][j] = val;
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

    // ★追加：画面のパラパラ漫画（アニメーション）を1秒間に30回スタートさせる
    startTimerHz(30);
}

AIDrumMachineAudioProcessorEditor::~AIDrumMachineAudioProcessorEditor()
{
    stopTimer(); // アプリを閉じるときにタイマーを止める
}

// ★追加：タイマーが呼ばれるたびに画面を塗り直す
void AIDrumMachineAudioProcessorEditor::timerCallback()
{
    repaint();
}

void AIDrumMachineAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::darkgrey);

    auto area = getLocalBounds().reduced(20);
    auto gridArea = area.removeFromBottom(250);

    int cols = 16;
    int rows = 8;
    float cellW = static_cast<float>(gridArea.getWidth()) / cols;
    float cellH = static_cast<float>(gridArea.getHeight()) / rows;

    // 1. まずはオレンジと黒のブロックを描画
    for (int row = 0; row < rows; ++row)
    {
        for (int col = 0; col < cols; ++col)
        {
            juce::Rectangle<float> cell(gridArea.getX() + col * cellW,
                gridArea.getY() + row * cellH,
                cellW - 2.0f,
                cellH - 2.0f);

            // キック(トラック0)が一番下の行になるように反転
            int trackIndex = (rows - 1) - row;

            if (drumPattern[trackIndex][col] > 0)
            {
                g.setColour(juce::Colours::orange);
            }
            else
            {
                g.setColour(juce::Colours::black.withAlpha(0.4f));
            }

            g.fillRoundedRectangle(cell, 4.0f);
        }
    }

    // ★2. ここに追加：今どこを再生しているか（プレイヘッド）を白く光らせる！
    int currentStep = audioProcessor.getCurrentStep();
    float playheadX = gridArea.getX() + currentStep * cellW;
    juce::Rectangle<float> playheadRect(playheadX, gridArea.getY(), cellW - 2.0f, gridArea.getHeight());

    g.setColour(juce::Colours::white.withAlpha(0.3f)); // 半透明の白
    g.fillRoundedRectangle(playheadRect, 4.0f);
}

void AIDrumMachineAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced(20);
    generateButton.setBounds(area.getX(), area.getY(), 180, 40);
    statusLabel.setBounds(area.getX() + 200, area.getY(), area.getWidth() - 200, 40);
}