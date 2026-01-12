import socket
import struct
import random
import time

UDP_IP = "192.168.4.1"   # ESP32 AP IP
UDP_PORT = 3333

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

# -------------------------------
# GENERATE ABNORMAL (ATTACK) FEATURES
# -------------------------------
# Attack traffic typically has unusually large, suspicious values.
# Generate 122 random floats between 5 and 15 (like before)
features = [random.uniform(5.0, 15.0) for _ in range(122)]

# Pack into binary format (122 floats)
packet = struct.pack('%sf' % len(features), *features)

# Send multiple packets to trigger consecutive detections
for i in range(3):
    sock.sendto(packet, (UDP_IP, UDP_PORT))
    print(f"Sent ATTACK packet {i+1}")
    time.sleep(0.2)
