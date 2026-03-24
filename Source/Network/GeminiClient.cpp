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

    // 4小節（Bar）分のデータを生成するようにプロンプトを拡張
    juce::String systemPrompt =
        "You are a specialized polyrhythmic drum machine AI. Output ONLY a valid JSON object. "
        "Do NOT include markdown tags like ```json or any conversational text. "
        "Generate an 8-track drum pattern for 4 bars. Each track must have a 'division' (integer from 1 to 9 representing steps PER BAR) "
        "and a 'pattern' array of integers (velocity 0-100). The length of the 'pattern' array MUST exactly match (division * 4). "
        "It MUST strictly follow this exact format: "
        "{\"track1\": {\"division\": 4, \"pattern\": [100,0,0,80, 100,0,0,80, 100,0,0,80, 100,0,0,80]}, "
        "\"track2\": {\"division\": 5, \"pattern\": [100,0,80,0,50, 100,0,80,0,50, 100,0,80,0,50, 100,0,80,0,50]}, "
        "\"track3\": {\"division\": 4, \"pattern\": [0,0,100,0, 0,0,100,0, 0,0,100,0, 0,0,100,0]}, "
        "\"track4\": {\"division\": 6, \"pattern\": [0,50,0,0,50,0, 0,50,0,0,50,0, 0,50,0,0,50,0, 0,50,0,0,50,0]}, "
        "\"track5\": {\"division\": 4, \"pattern\": [100,0,100,0, 100,0,100,0, 100,0,100,0, 100,0,100,0]}, "
        "\"track6\": {\"division\": 4, \"pattern\": [0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0]}, "
        "\"track7\": {\"division\": 4, \"pattern\": [0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0]}, "
        "\"track8\": {\"division\": 4, \"pattern\": [0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0]}}";

    juce::String strictPrompt = systemPrompt + "\n\nRequest: " + currentPrompt;
    partObj->setProperty("text", strictPrompt);

    parts.add(juce::var(partObj.get()));
    contentObj->setProperty("parts", parts);
    contents.add(juce::var(contentObj.get()));
    rootObj->setProperty("contents", contents);

    juce::String jsonRequest = juce::JSON::toString(juce::var(rootObj.get()));
    url = url.withPOSTData(jsonRequest);

    // ★修正: 4小節分の生成時間を考慮し、タイムアウトを30秒(30000ms)に大幅延長
    auto options = juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inAddress)
        .withExtraHeaders("Content-Type: application/json")
        .withConnectionTimeoutMs(30000);

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
                juce::Logger::writeToLog("--- AI JSON Result (4 Bars Polyrhythm) ---");
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