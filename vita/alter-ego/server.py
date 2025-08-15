import socket
import pyaudio

# --- Config ---
SAMPLE_RATE = 16000
CHANNELS = 1
SAMPLE_WIDTH = 2  # 16-bit PCM
UDP_IP = ''
UDP_PORT = 2012

# --- Setup PyAudio output ---
p = pyaudio.PyAudio()
stream = p.open(format=p.get_format_from_width(SAMPLE_WIDTH),
                channels=CHANNELS,
                rate=SAMPLE_RATE,
                output=True,
                frames_per_buffer=256)

# --- UDP Socket ---
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.bind((UDP_IP, UDP_PORT))

print("Listening for Vita UDP stream...")

try:
    while True:
        data, _ = sock.recvfrom(512)  # 256 samples × 2 bytes
        if data:
            stream.write(data)
except KeyboardInterrupt:
    pass
finally:
    stream.stop_stream()
    stream.close()
    p.terminate()
    sock.close()
