#!/usr/bin/env python3
"""
Convert PNG to RGB565 TRUE_COLOR format for LVGL
Simple, reliable format that definitely works
"""

import sys
import os
from PIL import Image

def rgb_to_rgb565(r, g, b):
    """Convert RGB888 to RGB565"""
    return ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3)

def convert_png_to_rgb565(png_file, output_file, target_size=(180, 180)):
    """Convert PNG to RGB565 TRUE_COLOR format"""
    
    if not os.path.exists(png_file):
        print(f"Error: PNG file not found: {png_file}")
        return False
    
    print(f"Loading PNG: {png_file}")
    
    # Load image
    img = Image.open(png_file)
    
    # Handle transparency - composite on black background (matches splash screen)
    if img.mode == 'RGBA':
        print("Image has transparency - compositing on black background...")
        background = Image.new('RGB', img.size, (0, 0, 0))
        background.paste(img, mask=img.split()[3])  # Use alpha channel as mask
        img = background
    elif img.mode != 'RGB':
        print(f"Converting image from {img.mode} to RGB...")
        img = img.convert('RGB')
    
    # Resize if needed
    if img.size != target_size:
        print(f"Resizing from {img.size} to {target_size}...")
        img = img.resize(target_size, Image.Resampling.LANCZOS)
    
    width, height = img.size
    print(f"Image size: {width}x{height} pixels")
    
    # Convert to RGB565
    colors = []
    pixels = img.load()
    white_count = 0
    black_count = 0
    for y in range(height):
        for x in range(width):
            r, g, b = pixels[x, y]
            rgb565 = rgb_to_rgb565(r, g, b)
            colors.append(rgb565)
            if rgb565 == 0xFFFF:
                white_count += 1
            elif rgb565 == 0x0000:
                black_count += 1
    
    total_size = len(colors) * 2  # 2 bytes per pixel
    print(f"RGB565 size: {total_size} bytes ({total_size / 1024:.1f} KB)")
    print(f"White pixels: {white_count}, Black pixels: {black_count}")
    print(f"Sample center colors: {colors[width*height//2 - 5:width*height//2 + 5]}")
    
    # Generate C file
    output = f"""// Auto-generated logo image for splash screen (RGB565)
// Generated from {os.path.basename(png_file)}
// Format: RGB565 TRUE_COLOR (2 bytes per pixel)
// Size: {total_size} bytes ({total_size / 1024:.1f} KB)
#include <lvgl.h>

#define LOGO_SPLASH_WIDTH {width}
#define LOGO_SPLASH_HEIGHT {height}

// RGB565 pixel data (little-endian)
static const uint8_t logo_splash_data[] = {{
"""
    
    # Write RGB565 colors as bytes (little-endian)
    for i in range(0, len(colors), 8):
        colors_line = colors[i:i+8]
        color_bytes = []
        for color in colors_line:
            color_bytes.append(f"0x{color & 0xFF:02X}")
            color_bytes.append(f"0x{(color >> 8) & 0xFF:02X}")
        output += f"    {', '.join(color_bytes)},\n"
    
    output += "};\n\n"
    
    # Write image descriptor
    output += f"""const lv_img_dsc_t logo_splash_img = {{
    .header = {{
        .cf = LV_IMG_CF_TRUE_COLOR,
        .w = {width},
        .h = {height},
    }},
    .data_size = {total_size},
    .data = logo_splash_data,
}};
"""
    
    # Backup existing file
    if os.path.exists(output_file):
        backup_file = output_file + '.backup'
        if not os.path.exists(backup_file):
            import shutil
            shutil.copy2(output_file, backup_file)
            print(f"Backed up original to: {backup_file}")
    
    # Write output file
    with open(output_file, 'w') as f:
        f.write(output)
    
    print(f"\n✓ RGB565 logo written to: {output_file}")
    return True

if __name__ == '__main__':
    script_dir = os.path.dirname(os.path.abspath(__file__))
    
    if len(sys.argv) < 2:
        png_file = os.path.join(script_dir, '../../../assets/sizes/app-icon/vertical/white/small.png')
        output_file = os.path.join(script_dir, '../src/ui/logo_splash.c')
    else:
        png_file = sys.argv[1]
        if len(sys.argv) > 2:
            output_file = sys.argv[2]
        else:
            output_file = os.path.join(script_dir, '../src/ui/logo_splash.c')
    
    if convert_png_to_rgb565(png_file, output_file):
        print("\n✓ Conversion complete!")
    else:
        print("\n✗ Conversion failed")
        sys.exit(1)

