import os
import openai
from google.cloud import speech, texttospeech
from asterisk.agi import AGI

def main():
    agi = AGI()
    agi.verbose("Starting scam bait script...")

    # Record the scammer's voice
    agi.record_file("/tmp/scammer_audio", "wav", "#", 5000, 0, True, 0)

    # Convert speech to text using Google Speech-to-Text
    client = speech.SpeechClient()
    with open("/tmp/scammer_audio.wav", "rb") as audio_file:
        audio = speech.RecognitionAudio(content=audio_file.read())
    config = speech.RecognitionConfig(
        encoding=speech.RecognitionConfig.AudioEncoding.LINEAR16,
        language_code="en-US"
    )
    response = client.recognize(config=config, audio=audio)
    transcription = response.results[0].alternatives[0].transcript

    # Send transcription to ChatGPT
    openai.api_key = "your-chatgpt-api-key"
    chat_response = openai.ChatCompletion.create(
        model="gpt-4",
        messages=[{"role": "system", "content": "Pretend to be a curious customer."},
                  {"role": "user", "content": transcription}]
    )
    bot_response = chat_response['choices'][0]['message']['content']

    # Convert ChatGPT response to speech using Google TTS
    tts_client = texttospeech.TextToSpeechClient()
    synthesis_input = texttospeech.SynthesisInput(text=bot_response)
    voice = texttospeech.VoiceSelectionParams(
        language_code="en-US", ssml_gender=texttospeech.SsmlVoiceGender.NEUTRAL
    )
    audio_config = texttospeech.AudioConfig(audio_encoding=texttospeech.AudioEncoding.LINEAR16)
    response = tts_client.synthesize_speech(
        input=synthesis_input, voice=voice, audio_config=audio_config
    )
    with open("/tmp/bot_response.wav", "wb") as out:
        out.write(response.audio_content)

    # Play response back to the scammer
    agi.stream_file("/tmp/bot_response")
    agi.hangup()

if __name__ == "__main__":
    main()
