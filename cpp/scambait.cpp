#include <iostream>
#include <fstream>
#include <cstdlib>
#include <curl/curl.h>
#include <json/json.h>
#include <ctime>

// Helper function to execute shell commands
std::string execCommand(const std::string &command) {
    char buffer[128];
    std::string result = "";
    FILE *pipe = popen(command.c_str(), "r");
    if (!pipe) throw std::runtime_error("popen() failed!");
    try {
        while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            result += buffer;
        }
    } catch (...) {
        pclose(pipe);
        throw;
    }
    pclose(pipe);
    return result;
}

// Helper function for libcurl to write response
size_t WriteCallback(void *contents, size_t size, size_t nmemb, std::string *output) {
    size_t totalSize = size * nmemb;
    output->append((char *)contents, totalSize);
    return totalSize;
}

// Function to send audio to Google Speech-to-Text
std::string transcribeAudio(const std::string &audioPath, const std::string &apiKey) {
    CURL *curl = curl_easy_init();
    std::string response;

    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, ("https://speech.googleapis.com/v1/speech:recognize?key=" + apiKey).c_str());
        curl_easy_setopt(curl, CURLOPT_POST, 1L);

        Json::Value requestBody;
        requestBody["config"]["encoding"] = "LINEAR16";
        requestBody["config"]["sampleRateHertz"] = 8000;
        requestBody["config"]["languageCode"] = "en-US";
        requestBody["audio"]["content"] = execCommand("base64 " + audioPath);

        Json::StreamWriterBuilder writer;
        std::string jsonData = Json::writeString(writer, requestBody);

        struct curl_slist *headers = nullptr;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonData.c_str());

        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

        curl_easy_perform(curl);
        curl_easy_cleanup(curl);
    }
    return response;
}

// Function to interact with ChatGPT
std::string chatWithGPT(const std::string &message, const std::string &apiKey) {
    CURL *curl = curl_easy_init();
    std::string response;

    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, "https://api.openai.com/v1/chat/completions");
        curl_easy_setopt(curl, CURLOPT_POST, 1L);

        Json::Value requestBody;
        requestBody["model"] = "gpt-4";
        requestBody["messages"][0]["role"] = "system";
        requestBody["messages"][0]["content"] = "You are a bot designed to waste time.";
        requestBody["messages"][1]["role"] = "user";
        requestBody["messages"][1]["content"] = message;

        Json::StreamWriterBuilder writer;
        std::string jsonData = Json::writeString(writer, requestBody);

        struct curl_slist *headers = nullptr;
        headers = curl_slist_append(headers, ("Authorization: Bearer " + apiKey).c_str());
        headers = curl_slist_append(headers, "Content-Type: application/json");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonData.c_str());

        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

        curl_easy_perform(curl);
        curl_easy_cleanup(curl);
    }
    return response;
}

// Function to convert text to speech using Google TTS
void synthesizeSpeech(const std::string &text, const std::string &outputPath, const std::string &apiKey) {
    CURL *curl = curl_easy_init();

    if (curl) {
        std::string response;

        curl_easy_setopt(curl, CURLOPT_URL, ("https://texttospeech.googleapis.com/v1/text:synthesize?key=" + apiKey).c_str());
        curl_easy_setopt(curl, CURLOPT_POST, 1L);

        Json::Value requestBody;
        requestBody["input"]["text"] = text;
        requestBody["voice"]["languageCode"] = "en-US";
        requestBody["voice"]["ssmlGender"] = "NEUTRAL";
        requestBody["audioConfig"]["audioEncoding"] = "LINEAR16";

        Json::StreamWriterBuilder writer;
        std::string jsonData = Json::writeString(writer, requestBody);

        struct curl_slist *headers = nullptr;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonData.c_str());

        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

        curl_easy_perform(curl);
        curl_easy_cleanup(curl);

        std::ofstream outFile(outputPath, std::ios::binary);
        outFile << response;
        outFile.close();
    }
}

int main() {
    const std::string googleApiKey = "YOUR_GOOGLE_API_KEY";
    const std::string openaiApiKey = "YOUR_OPENAI_API_KEY";

    while (true) {
        std::string scammerAudio = "/tmp/scammer_audio.wav";
        execCommand("arecord -D hw:0,0 -f S16_LE -r8000 -d 10 " + scammerAudio);

        std::string transcription = transcribeAudio(scammerAudio, googleApiKey);
        if (transcription.empty()) break;

        std::ofstream logFile("/var/log/scambait.log", std::ios::app);
        logFile << "Scammer: " << transcription << std::endl;

        std::string gptResponse = chatWithGPT(transcription, openaiApiKey);
        logFile << "Bot: " << gptResponse << std::endl;
        logFile.close();

        std::string botResponseAudio = "/tmp/bot_response.wav";
        synthesizeSpeech(gptResponse, botResponseAudio, googleApiKey);

        execCommand("aplay " + botResponseAudio);
    }
    return 0;
}
