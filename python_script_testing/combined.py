import socket
import struct
import random
import time

UDP_IP = "192.168.4.1"   # ESP32 AP IP
UDP_PORT = 3333

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

# ============================================================
# Feature generators (pure Python)
# ============================================================

def generate_normal():
    """
    Normal traffic: values around N(0,1)
    Using Python's random.gauss(mean, std_dev)
    """
    return [random.gauss(0, 1) for _ in range(122)]

def generate_attack():
    """
    Attack traffic: higher abnormal values
    Using random.uniform(min, max)
    """
    return [random.uniform(5, 15) for _ in range(122)]

# ============================================================
# Send feature packet to ESP32
# ============================================================

def send_features(features):
    # Pack list of floats into binary format
    packet = struct.pack('%sf' % len(features), *features)
    sock.sendto(packet, (UDP_IP, UDP_PORT))

# ============================================================
# Traffic simulation loop
# ============================================================

print("\n=== STARTING RANDOM NORMAL + ATTACK TRAFFIC SIMULATION (NO NUMPY) ===")
print("Sending/randomizing traffic to ESP32 IDS...\n")

while True:
    # Randomly choose packet type (2/3 normal, 1/3 attack)
    packet_type = random.choice(["NORMAL", "NORMAL", "ATTACK"])

    if packet_type == "NORMAL":
        features = generate_normal()
        send_features(features)
        print("[>] Sent NORMAL packet")

    elif packet_type == "ATTACK":
        features = generate_attack()
        send_features(features)
        print("[!!!] Sent ATTACK packet")

    # Random delay between packets
    time.sleep(random.uniform(0.5, 2.0))