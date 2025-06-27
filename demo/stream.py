# For reference, speed is stable

import socket
import time
from PIL import Image, ImageDraw

HOST = '10.200.50.186'  # ESP32 IP
PORT = 12345

sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
sock.connect((HOST, PORT))

while True:
    # Generate frame (simple animation)
    img = Image.new('RGB', (64, 64), 'black')
    draw = ImageDraw.Draw(img)
    t = time.time()
    x = int((time.time() * 30) % 64)
    draw.ellipse((x, 32, x+8, 40), fill='red')

    # Compress as JPEG
    from io import BytesIO
    buffer = BytesIO()
    img.save(buffer, format='JPEG', quality=100)
    data = buffer.getvalue()

    # Send size + image
    length = len(data)
    sock.sendall(length.to_bytes(2, 'big') + data)

    time.sleep(1 / 30)  # 20 FPS