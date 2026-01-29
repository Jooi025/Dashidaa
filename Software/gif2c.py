from PIL import Image, ImageSequence
import os

# === CONFIG ===
INPUT_GIF = "yawn.gif"
OUTPUT_HEADER = "gif_frames.h"
WIDTH, HEIGHT = 128, 128
SYMBOL_NAME = "gif_frames"
FRAME_LIMIT = 6

# === RGB888 to RGB565 ===
def rgb888_to_rgb565(r, g, b):
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)

# === Load GIF ===
img = Image.open(INPUT_GIF)
frames = [frame.copy().convert("RGB").resize((WIDTH, HEIGHT)) for frame in ImageSequence.Iterator(img)]

# === Convert to RGB565 ===
rgb565_data = []

for frame_index, frame in enumerate(frames):
    if frame_index >= FRAME_LIMIT:
        break
    pixels = frame.load()
    frame_data = []
    for y in range(HEIGHT):
        for x in range(WIDTH):
            r, g, b = pixels[x, y]
            color = rgb888_to_rgb565(r, g, b)
            frame_data.append((color >> 8) & 0xFF)
            frame_data.append(color & 0xFF)
    rgb565_data.append(frame_data)

# === Write C Header ===
with open(OUTPUT_HEADER, "w") as f:
    f.write(f"#ifndef {SYMBOL_NAME.upper()}_H\n")
    f.write(f"#define {SYMBOL_NAME.upper()}_H\n\n")
    f.write("#include <stdint.h>\n\n")
    f.write(f"#define {SYMBOL_NAME}_frame_count {len(rgb565_data)}\n\n")
    f.write(f"const uint8_t {SYMBOL_NAME}[{len(rgb565_data)}][{WIDTH * HEIGHT * 2}] = {{\n")

    for frame in rgb565_data:
        f.write("  {\n")
        for i in range(0, len(frame), 12):
            line = ", ".join(f"0x{b:02X}" for b in frame[i:i+12])
            f.write(f"    {line},\n")
        f.write("  },\n")

    f.write("};\n\n")
    f.write(f"#endif // {SYMBOL_NAME.upper()}_H\n")

print(f"✅ Done! Output written to {OUTPUT_HEADER}")
