import speech_recognition as speech
import asyncio
from bleak import BleakScanner, BleakClient

voiceRec = speech.Recognizer()
DEVICE = "Nano"
UUID = "19B10001-EB67-592D-B9AC-A76E4D89BC63"
running = True

async def send(command):
    arduino=await BleakScanner.find_device_by_name(DEVICE)
    if arduino is None:
        print("Error with finding arduino")
        return
    else:
        async with BleakClient(arduino) as client:
            await client.write_gatt_char(UUID, command.encode("utf-8"), response=True)
            print("Sent: ",command)

while (running):
    with speech.Microphone() as source:
        
        voiceRec.adjust_for_ambient_noise(source, duration=1)
        
        print("Say a command...")
        audio =voiceRec.listen(source)

    try:
        text = voiceRec.recognize_google(audio)
        text = text.lower()
        print("You said: ", text)
        if (text == "quit"):
            running=False
        else:
            asyncio.run(send(text))
        
    except speech.UnknownValueError:
        print("Could not understand audio")
    except speech.RequestError:
        print("Speech recognition  service error")

