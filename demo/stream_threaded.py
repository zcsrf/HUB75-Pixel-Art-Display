# Reference using threads
# Animation sets the pace... something i don't really like...

import threading
import queue
import time
import socket
import numpy as np
import cv2

# Constants
WIDTH, HEIGHT = 64, 64
FPS = 30
HOST = '10.200.50.186'
PORT = 12345

# Queues
frame_queue = queue.Queue(maxsize=10)
jpeg_queue = queue.Queue(maxsize=10)

# Socket
sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
sock.connect((HOST, PORT))

# -- Thread 1: Generate animation (e.g. moving circle)
def animation_thread():
    x = 0
    while True:
        frame = np.zeros((HEIGHT, WIDTH, 3), dtype=np.uint8)
        cv2.circle(frame, (x, HEIGHT // 2), 5, (255, 255, 255), -1)
        x = (x + 1) % WIDTH
        try:
            frame_queue.put(frame, timeout=1)
        except queue.Full:
            pass
        time.sleep(1 / FPS)

# -- Thread 2: Compress to JPEG
def encode_thread():
    while True:
        try:
            frame = frame_queue.get(timeout=1)
            ret, jpeg = cv2.imencode('.jpg', frame, [int(cv2.IMWRITE_JPEG_QUALITY), 100])
            if ret:
                jpeg_queue.put(jpeg.tobytes(), timeout=1)
        except queue.Empty:
            pass
        except queue.Full:
            pass

# -- Thread 3: Send via socket
def sender_thread():
    while True:
        try:
            jpeg = jpeg_queue.get(timeout=1)
            length = len(jpeg)
            sock.sendall(length.to_bytes(2, 'big') + jpeg)
        except queue.Empty:
            pass

# Start threads
threading.Thread(target=animation_thread, daemon=True).start()
threading.Thread(target=encode_thread, daemon=True).start()
threading.Thread(target=sender_thread, daemon=True).start()

# Keep alive
while True:
    time.sleep(1)
