# For reference only... I have a lot of glitches.... streaming speed is not stable

import numpy as np
import cv2
import ffmpeg
import time

# Dimensions
WIDTH, HEIGHT = 64, 64
FPS = 15
TARGET_IP = '10.200.50.186'  # change to your ESP32 IP

# Start ffmpeg subprocess with TCP MJPEG output
process = (
    ffmpeg
    .input('pipe:0', format='image2pipe', vcodec='mjpeg', r=15)
    .output(f'tcp://{TARGET_IP}:12345', format='mjpeg', vcodec='mjpeg', r=FPS, fflags='+flush_packets')
#    .global_args('-re')  # simulate real-time input
    .run_async(pipe_stdin=True)
)

frame_count = 0

try:
    while True:
        # Create a black frame with a moving white circle
        frame = np.zeros((HEIGHT, WIDTH, 3), dtype=np.uint8)

        x = int((time.time() * 30) % WIDTH)
        cv2.circle(frame, (x, HEIGHT // 2), 5, (255, 255, 255), -1)

        # Encode to JPEG using OpenCV
        success, jpeg = cv2.imencode('.jpg', frame, [int(cv2.IMWRITE_JPEG_QUALITY), 90])
        if not success:
            print("JPEG encode failed")
            continue

        # Send JPEG to ffmpeg via stdin
        process.stdin.write(jpeg.tobytes())
        process.stdin.flush()

        time.sleep(1 / 15)
        frame_count += 1

except KeyboardInterrupt:
    print("Stopping stream...")
    process.stdin.close()
    process.wait()