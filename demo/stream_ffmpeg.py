# For reference only... I have a lot of glitches.... streaming speed is not stable

import subprocess

import time
from PIL import Image, ImageDraw
from io import BytesIO

frame_count = 0
start = time.time()

# ffmpeg command to stream MJPEG over TCP
ffmpeg_cmd = [
    'ffmpeg',
    '-f', 'image2pipe',
    '-vcodec', 'mjpeg',
    '-r', '30',  # FPS
    '-re',
    '-i', '-',
    '-f', 'mjpeg',
    'tcp://10.200.50.186:12345'
]

# Start ffmpeg process
process = subprocess.Popen(ffmpeg_cmd, stdin=subprocess.PIPE, bufsize=12288)

FPS = 30
FRAME_DURATION = 1.0 / FPS
last_time = time.time()

try:
    while True:
        # Create a new 64x64 RGB image
        img = Image.new('RGB', (64, 64), 'black')
        draw = ImageDraw.Draw(img)

        # Draw something animated (moving dot)
        t = time.time()
        x = int((t * 30) % 64)
        draw.ellipse((x, 32, x + 8, 40), fill='lime')

        # Save frame as JPEG into memory
        buffer = BytesIO()
        img.save(buffer, format='JPEG', quality=100) #, quality=100, optimize=True, progressive=False)
        jpeg_data = buffer.getvalue()

        # Write to ffmpeg stdin
        process.stdin.write(jpeg_data)
        process.stdin.flush()
        
        frame_count += 1
        if time.time() - start >= 1.0:
            print(f"FPS: {frame_count}")
            frame_count = 0
            start = time.time()

        now = time.time()
        elapsed = now - last_time
        sleep_time = FRAME_DURATION - elapsed
        if sleep_time > 0:
            time.sleep(sleep_time)

        last_time = time.time()

except KeyboardInterrupt:
    print("Stopping stream...")
finally:
    process.stdin.close()
    process.wait()