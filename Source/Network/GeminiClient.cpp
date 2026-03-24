#include "GeminiClient.h"

GeminiClient::GeminiClient() : juce::Thread("GeminiNetworkThread") {}
GeminiClient::~GeminiClient() { stopThread(4000); }

void GeminiClient::fetchDrumPattern(const juce::String& userPrompt, const juce::String& apiKey)
{
    if (isThreadRunning()) return;
    currentApiKey = apiKey.trim();
    currentPrompt = userPrompt;
    startThread();
}

void GeminiClient::run()
{
    juce::String urlString = "https://generativelanguage.googleapis.com/v1beta/models/gemini-3-flash-preview:generateContent?key=" + currentApiKey;
    juce::URL url(urlString);

    juce::DynamicObject::Ptr rootObj = new juce::DynamicObject();
    juce::Array<juce::var> contents;
    juce::DynamicObject::Ptr contentObj = new juce::DynamicObject();
    juce::Array<juce::var> parts;
    juce::DynamicObject::Ptr partObj = new juce::DynamicObject();

    // ★ここを厳格に修正しました：AIが勝手なフォーマットで返さないように「絶対にこの形（tracksとpattern）で返せ」と強く命令します。
    juce::String strictPrompt = "Output ONLY a valid JSON object. It MUST strictly follow this exact format: {\"tracks\": [{\"pattern\":[1,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0]}]}. Generate 8 tracks of 16 steps (1 or 0). No markdown, no other text. Request: " + currentPrompt;
    partObj->setProperty("text", strictPrompt);

    parts.add(juce::var(partObj.get()));
    contentObj->setProperty("parts", parts);
    contents.add(juce::var(contentObj.get()));
    rootObj->setProperty("contents", contents);

    juce::String jsonRequest = juce::JSON::toString(juce::var(rootObj.get()));
    url = url.withPOSTData(jsonRequest);

    auto options = juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inAddress)
        .withExtraHeaders("Content-Type: application/json")
        .withConnectionTimeoutMs(10000);

    std::unique_ptr<juce::InputStream> stream(url.createInputStream(options));

    if (stream == nullptr) {
        juce::MessageManager::callAsync([this] { if (onError) onError("Network Stream Failed"); });
        return;
    }

    juce::String response = stream->readEntireStreamAsString();
    juce::var parsedJson = juce::JSON::parse(response);

    if (parsedJson.hasProperty("error")) {
        juce::String googleMsg = parsedJson["error"]["message"].toString();
        juce::MessageManager::callAsync([this, googleMsg] { if (onError) onError(googleMsg); });
        return;
    }

    try {
        auto* candidates = parsedJson["candidates"].getArray();
        if (candidates != nullptr && candidates->size() > 0) {
            juce::String text = (*candidates)[0]["content"]["parts"][0]["text"].toString();
            juce::var drumData = juce::JSON::parse(sanitizeJsonResponse(text));

            if (!drumData.isVoid()) {
                // ★追加：AIが実際にどんなJSON（目印）を返してきたか、Visual Studioの出力ログに全て書き出します。
                juce::Logger::writeToLog("--- AI JSON Result ---");
                juce::Logger::writeToLog(juce::JSON::toString(drumData));

                juce::MessageManager::callAsync([this, drumData] { if (onSuccess) onSuccess(drumData); });
                return;
            }
        }
    }
    catch (...) {}

    juce::MessageManager::callAsync([this] { if (onError) onError("AI Response Format Error"); });
}

juce::String GeminiClient::sanitizeJsonResponse(const juce::String& response)
{
    juce::String s = response.trim();
    if (s.startsWith("```json")) s = s.substring(7);
    else if (s.startsWith("```")) s = s.substring(3);
    if (s.endsWith("```")) s = s.dropLastCharacters(3);
    return s.trim();
}