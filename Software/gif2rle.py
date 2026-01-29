from PIL import Image, ImageSequence
import os

def rgb888_to_rgb565(r, g, b):
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)

def rle_compress(rgb565_data):
    rle = []
    prev = rgb565_data[0]
    count = 1
    for pix in rgb565_data[1:]:
        if pix == prev and count < 0xFFFF:
            count += 1
        else:
            rle.append((count, prev))
            prev = pix
            count = 1
    rle.append((count, prev))
    return rle

def process_gif(filename, output_c="gif.c", output_h="gif.h", width=128, height=128):
    im = Image.open(filename)

    frames_rle = []
    total_frames = 0

    with open(output_c, "w") as fc, open(output_h, "w") as fh:
        # Header guard
        fh.write("#pragma once\n\n")
        fh.write("#include <stdint.h>\n\n")
        fh.write("typedef struct {\n")
        fh.write("    uint16_t count;\n")
        fh.write("    uint16_t color;\n")
        fh.write("} RLE_Pixel;\n\n")

        fc.write('#include "gif.h"\n\n')

        for i, frame in enumerate(ImageSequence.Iterator(im)):
            frame = frame.convert("RGB").resize((width, height))
            rgb565_data = [
                rgb888_to_rgb565(r, g, b)
                for (r, g, b) in frame.getdata()
            ]
            rle_data = rle_compress(rgb565_data)
            frames_rle.append((f"frame_{i}", rle_data))
            total_frames += 1

            fc.write(f"const RLE_Pixel frame_{i}[] = {{\n")
            for count, color in rle_data:
                fc.write(f"    {{ {count}, 0x{color:04X} }},\n")
            fc.write("};\n\n")
            fc.write(f"const uint32_t frame_{i}_len = {len(rle_data)};\n\n")

        # Write array of pointers to frames and lengths
        fc.write("const RLE_Pixel* gif_frames[] = {\n")
        for i in range(total_frames):
            fc.write(f"    frame_{i},\n")
        fc.write("};\n\n")

        fc.write("const uint32_t gif_frame_lengths[] = {\n")
        for i in range(total_frames):
            fc.write(f"    frame_{i}_len,\n")
        fc.write("};\n\n")

        fc.write(f"const uint32_t gif_frame_count = {total_frames};\n")

        # Header declarations
        fh.write("extern const RLE_Pixel* gif_frames[];\n")
        fh.write("extern const uint32_t gif_frame_lengths[];\n")
        fh.write("extern const uint32_t gif_frame_count;\n")

    print(f"✅ Generated: {output_c} and {output_h} with {total_frames} frames")

if __name__ == "__main__":
    process_gif("robot_eye.gif")
