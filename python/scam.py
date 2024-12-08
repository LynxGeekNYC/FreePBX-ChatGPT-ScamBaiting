import os
import logging
import openai
from google.cloud import speech, texttospeech
from asterisk.agi import AGI

# Setup logging
logging.basicConfig(filename="/var/log/scambait.log", level=logging.INFO, format='%(asctime)s %(message)s')

# Google Cloud configuration
os.environ["GOOGLE_APPLICATION_CREDENTIALS"] = "/path/to/google-credentials.json"

# Initialize Google API clients
speech_client = speech.SpeechClient()
tts_client = texttospeech.TextToSpeechClient()

# OpenAI API key
openai.api_key = "your-chatgpt-api-key"

def transcribe_audio(file_path):
    """Transcribe audio file to text using Google Speech-to-Text."""
    with open(file_path, "rb") as audio_file:
        audio = speech.RecognitionAudio(content=audio_file.read())
    config = speech.RecognitionConfig(
        encoding=speech.RecognitionConfig.AudioEncoding.LINEAR16,
        sample_rate_hertz=8000,
        language_code="en-US"
    )
    response = speech_client.recognize(config=config, audio=audio)
    return response.results[0].alternatives[0].transcript if response.results else ""

def generate_chatgpt_response(user_input, context):
    """Send user input to ChatGPT and return the response."""
    context.append({"role": "user", "content": user_input})
    chat_response = openai.ChatCompletion.create(
        model="gpt-4",
        messages=context
    )
    bot_response = chat_response['choices'][0]['message']['content']
    context.append({"role": "assistant", "content": bot_response})
    return bot_response

def synthesize_and_play(text):
    """Convert text to speech using Google TTS and play it back."""
    synthesis_input = texttospeech.SynthesisInput(text=text)
    voice = texttospeech.VoiceSelectionParams(
        language_code="en-US",
        ssml_gender=texttospeech.SsmlVoiceGender.NEUTRAL
    )
    audio_config = texttospeech.AudioConfig(audio_encoding=texttospeech.AudioEncoding.LINEAR16)
    response = tts_client.synthesize_speech(input=synthesis_input, voice=voice, audio_config=audio_config)
    output_path = "/tmp/bot_response.wav"
    with open(output_path, "wb") as out:
        out.write(response.audio_content)
    os.system(f"aplay {output_path}")

# Main script
def main():
    agi = AGI()
    agi.verbose("Starting scam bait script...")
    context = [{"role": "system", "content": "You are a helpful assistant pretending to be a curious customer."}]
    
    while True:
        # Record scammer audio
        agi.record_file("/tmp/scammer_audio", "wav", "#", 5000, 0, True, 0)
        transcription = transcribe_audio("/tmp/scammer_audio.wav")
        
        if not transcription:
            logging.info("Call ended by scammer.")
            break
        
        logging.info(f"Scammer: {transcription}")
        
        # Generate response using ChatGPT
        bot_response = generate_chatgpt_response(transcription, context)
        logging.info(f"Bot: {bot_response}")
        
        # Convert response to speech and play it back
        synthesize_and_play(bot_response)

        # Simulate delay
        agi.wait(1)

if __name__ == "__main__":
    main()
