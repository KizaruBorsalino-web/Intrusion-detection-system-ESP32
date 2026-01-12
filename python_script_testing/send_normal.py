import socket
import struct
import random

UDP_IP = "192.168.4.1"
UDP_PORT = 3333

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

# Generate 122 normally distributed floats (mean=0, std=1)
features = [random.gauss(0, 1) for _ in range(122)]

# Pack into float32 format
packet = struct.pack('%sf' % len(features), *features)

sock.sendto(packet, (UDP_IP, UDP_PORT))
print("Sent NORMAL packet!")
