#!/usr/bin/env python3

import sys
import socket
import subprocess

# --- Config ---
# Match the sample rate and format to what your source is sending
SAMPLE_RATE = 16000
AUDIO_FORMAT = 's16le'  # Signed 16-bit Little-Endian
CHANNELS = 1

# Network settings to receive audio
UDP_IP = ''  # Listen on all available interfaces
UDP_PORT = 2012 # The port your audio source is sending to

# The NAME of the virtual sink we created in Step 1
PULSEAUDIO_SINK = 'VitaMic'

def main():
    """
    Listens for raw audio on a UDP port and pipes it to a PulseAudio virtual sink,
    making it available as a system-wide microphone.
    """
    print("--- PS Vita Virtual Microphone ---")
    print(f"[*] Listening for UDP audio on port {UDP_PORT}...")

    # The command to pipe raw audio into our virtual sink's monitor source
    # pacat is the native PulseAudio tool for playing/recording raw audio
    command = [
        'pacat',
        '--playback',
        f'--device={PULSEAUDIO_SINK}',
        f'--rate={SAMPLE_RATE}',
        f'--format={AUDIO_FORMAT}',
        f'--channels={CHANNELS}'
    ]

    try:
        # Start the pacat process, with its standard input connected to a pipe
        pacat_process = subprocess.Popen(command, stdin=subprocess.PIPE)
        print(f"[*] Piping audio to virtual mic: '{PULSEAUDIO_SINK}'")
        print("[*] You can now select 'PS_Vita_Virtual_Mic' in any application.")
        print("[*] Press Ctrl+C to stop.")

        # Set up the UDP socket
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        sock.bind((UDP_IP, UDP_PORT))

        while True:
            # Receive raw audio data from the network
            data, _ = sock.recvfrom(512) # Adjust buffer size if needed
            
            # Write the received audio data directly to pacat's standard input
            try:
                pacat_process.stdin.write(data)
                pacat_process.stdin.flush()
            except BrokenPipeError:
                print("\n[!] pacat process terminated unexpectedly. Restarting...")
                # Attempt to restart the process if it fails
                pacat_process = subprocess.Popen(command, stdin=subprocess.PIPE)
            except Exception as e:
                print(f"\n[!] An error occurred while writing to stdin: {e}")
                break

    except FileNotFoundError:
        print("\n[!] Error: 'pacat' command not found. Is PulseAudio installed?")
    except KeyboardInterrupt:
        print("\n[*] Shutting down gracefully.")
    except Exception as e:
        print(f"\n[!] An unexpected error occurred: {e}")
    finally:
        if 'pacat_process' in locals() and pacat_process.poll() is None:
            pacat_process.terminate()
            pacat_process.wait()
        if 'sock' in locals():
            sock.close()
        print("[*] Script terminated.")

if __name__ == "__main__":
    main()
