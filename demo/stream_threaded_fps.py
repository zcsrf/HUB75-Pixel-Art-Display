# Reference using threads
# If no new frame, the frame sender keeps sending the old frame 

import threading
import queue
import time
import socket
import math
import numpy as np
import cv2

# Constants
WIDTH, HEIGHT = 64, 64
CENTER = (WIDTH // 2, HEIGHT // 2)
RADIUS = WIDTH // 2 - 2

FPS = 24
HOST = '10.200.50.186'
PORT = 12345

window_name = 'image'

MAX_DEPTH = 32

# Queues
frame_queue = queue.Queue(maxsize=10)
jpeg_queue = queue.Queue(maxsize=10)

# Socket
sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
sock.connect((HOST, PORT))

def create_blank_frame():
    return np.zeros((HEIGHT, WIDTH, 3), dtype=np.uint8)

# Draw clock hands
def draw_hand(img, angle_deg, length, color, thickness=1):
    angle_rad = math.radians(angle_deg - 90)
    x = int(CENTER[0] + length * math.cos(angle_rad))
    y = int(CENTER[1] + length * math.sin(angle_rad))
    cv2.line(img, CENTER, (x, y), color, thickness)

   
def render_clock():
    frame = create_blank_frame()

    # Draw outer circle
    cv2.circle(frame, CENTER, RADIUS, (180, 180, 180), 1)

    # Hour ticks
    for h in range(12):
        angle = math.radians(h * 30 - 90)
        x1 = int(CENTER[0] + (RADIUS - 2) * math.cos(angle))
        y1 = int(CENTER[1] + (RADIUS - 2) * math.sin(angle))
        x2 = int(CENTER[0] + (RADIUS - 4) * math.cos(angle))
        y2 = int(CENTER[1] + (RADIUS - 4) * math.sin(angle))
        cv2.line(frame, (x1, y1), (x2, y2), (255, 255, 255), 1)

    # Time
    t = time.localtime()
    hour = t.tm_hour % 12
    minute = t.tm_min
    second = t.tm_sec

    # Angles
    hour_angle = (hour + minute / 60) * 30
    min_angle = (minute + second / 60) * 6
    sec_angle = second * 6

    # Hands
    draw_hand(frame, hour_angle, RADIUS * 0.5, (255, 255, 255), 1)
    draw_hand(frame, min_angle, RADIUS * 0.7, (200, 255, 255), 1)
    draw_hand(frame, sec_angle, RADIUS * 0.9, (0, 0, 255), 1)

    return frame

# -- Thread 1: Generate animation 
def animation_thread():
    while True:
        #frame = render_clock()
        frame = render_clock()


        try:
            frame_queue.put(frame, timeout=1)
            cv2.imshow("ShowFrame", frame)
            if cv2.waitKey(1) & 0xFF == ord("q"):
                break
        except queue.Full:
            pass
        time.sleep(1 / FPS)

# -- Thread 2: Compress to JPEG
def encode_thread():
    while True:
        try:
            frame = frame_queue.get(timeout=1)
            ret, jpeg = cv2.imencode('.jpg', frame, [int(cv2.IMWRITE_JPEG_QUALITY), 95, cv2.IMWRITE_JPEG_SAMPLING_FACTOR, cv2.IMWRITE_JPEG_SAMPLING_FACTOR_444])
            if ret:
                jpeg_queue.put(jpeg.tobytes(), timeout=1)
        except queue.Empty:
            pass
        except queue.Full:
            pass

# -- Thread 3: Send via socket
def sender_thread():
    last_jpeg = None
    frame_duration = 1.0 / FPS
    next_send_time = time.time()

    while True:
        try:
            # Non-blocking: only take if available
            jpeg = jpeg_queue.get_nowait()
            last_jpeg = jpeg
        except queue.Empty:
            jpeg = last_jpeg

        if jpeg:
            try:
                length = len(jpeg)
                sock.sendall(length.to_bytes(2, 'big') + jpeg)
            except Exception as e:
                print("Send error:", e)

        # Wait for next frame time
        next_send_time += frame_duration
        delay = next_send_time - time.time()
        if delay > 0:
            time.sleep(delay)
        else:
            next_send_time = time.time()  # catch up if beh
# Start threads
threading.Thread(target=animation_thread, daemon=True).start()
threading.Thread(target=encode_thread, daemon=True).start()
threading.Thread(target=sender_thread, daemon=True).start()

# Keep alive
while True:
    time.sleep(1)
