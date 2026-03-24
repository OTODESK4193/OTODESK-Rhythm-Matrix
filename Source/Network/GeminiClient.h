#pragma once

// JUCEモジュールの直接インクルード
#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>

// 標準ライブラリのインクルード
#include <memory>
#include <functional>

/**
 * Gemini APIと通信し、ポリリズム・ドラムパターンを取得するクラス。
 * JUCE 8 のスレッドおよび通信プロトコルに準拠した実装です。
 */
class GeminiClient : private juce::Thread
{
public:
    GeminiClient();
    ~GeminiClient() override;

    /**
     * AIへのリクエストを開始します。
     * @param userPrompt ユーザーのリクエスト
     * @param apiKey APIキー
     */
    void fetchDrumPattern(const juce::String& userPrompt, const juce::String& apiKey);

    // 通信成功時のコールバック（パース済みのJSONデータが渡されます）
    std::function<void(const juce::var&)> onSuccess;

    // エラー発生時のコールバック
    std::function<void(const juce::String&)> onError;

private:
    void run() override;

    juce::String currentPrompt;
    juce::String currentApiKey;

    // AIからの応答（Markdownなど）を純粋なJSONに掃除する
    juce::String sanitizeJsonResponse(const juce::String& response);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GeminiClient)
};