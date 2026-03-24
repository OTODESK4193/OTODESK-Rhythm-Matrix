#include "GeminiClient.h"
#include <stdexcept>

GeminiClient::GeminiClient() : juce::Thread("GeminiNetworkThread")
{
}

GeminiClient::~GeminiClient()
{
    // スレッドが動作中の場合は安全に停止させる
    stopThread(4000);
}

void GeminiClient::fetchDrumPattern(const juce::String& userPrompt, const juce::String& apiKey)
{
    if (isThreadRunning())
        return;

    currentPrompt = userPrompt;
    currentApiKey = apiKey;

    startThread();
}

void GeminiClient::run()
{
    // 1. エンドポイントの構築
    juce::String urlString = "https://generativelanguage.googleapis.com/v1beta/models/gemini-1.5-flash:generateContent?key=" + currentApiKey;
    juce::URL url(urlString);

    // 2. システムプロンプト
    juce::String systemInstruction =
        "Generate a drum pattern JSON. Output ONLY a valid JSON object. "
        "Format: {\"tracks\": [{\"id\":0, \"division\":16, \"pattern\":[1,0,1,0]}]}";

    // 3. 送信用JSON（ペイロード）の構築
    juce::DynamicObject::Ptr rootObj = new juce::DynamicObject();
    juce::Array<juce::var> contents;
    juce::DynamicObject::Ptr contentObj = new juce::DynamicObject();
    juce::Array<juce::var> parts;
    juce::DynamicObject::Ptr partObj = new juce::DynamicObject();

    partObj->setProperty("text", systemInstruction + "\nRequest: " + currentPrompt);
    parts.add(juce::var(partObj.get()));
    contentObj->setProperty("parts", parts);
    contents.add(juce::var(contentObj.get()));
    rootObj->setProperty("contents", contents);

    juce::String jsonRequest = juce::JSON::toString(juce::var(rootObj.get()));

    // POST用データの準備
    juce::MemoryBlock postData;
    postData.append(jsonRequest.toRawUTF8(), jsonRequest.getNumBytesAsUTF8());

    // 4. HTTPリクエストの設定 (JUCE 8 修正版)
    // Constructor で inPostData を指定し、メソッド名 .withPostData を使用
    auto options = juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inPostData)
        .withExtraHeaders("Content-Type: application/json")
        .withConnectionTimeoutMs(10000)
        .withPostData(postData);

    std::unique_ptr<juce::InputStream> stream(url.createInputStream(options));

    if (stream == nullptr)
    {
        juce::MessageManager::callAsync([this] {
            if (onError) onError("Failed to connect. Check internet or API key.");
            });
        return;
    }

    juce::String response = stream->readEntireStreamAsString();

    // 5. 応答の解析
    juce::var parsedJson = juce::JSON::parse(response);

    if (parsedJson.isVoid())
    {
        juce::MessageManager::callAsync([this] {
            if (onError) onError("Empty response from Gemini API.");
            });
        return;
    }

    try {
        // AIのレスポンスからテキスト部分を取り出す
        juce::String rawText = parsedJson["candidates"][0]["content"]["parts"][0]["text"].toString();

        // Markdown等を除去して再パース
        juce::String cleanJsonText = sanitizeJsonResponse(rawText);
        juce::var drumData = juce::JSON::parse(cleanJsonText);

        if (!drumData.isVoid())
        {
            // 結果をGUIスレッドへ通知
            juce::MessageManager::callAsync([this, drumData] {
                if (onSuccess) onSuccess(drumData);
                });
        }
        else
        {
            throw std::runtime_error("JSON cleaning failed");
        }
    }
    catch (...) {
        juce::MessageManager::callAsync([this] {
            if (onError) onError("Failed to parse AI pattern data.");
            });
    }
}

juce::String GeminiClient::sanitizeJsonResponse(const juce::String& response)
{
    juce::String s = response.trim();

    // Markdownのバッククォート囲み（```json ... ```）を剥ぎ取る
    if (s.startsWith("```json")) s = s.substring(7);
    if (s.startsWith("```"))     s = s.substring(3);
    if (s.endsWith("```"))       s = s.dropLastCharacters(3);

    return s.trim();
}