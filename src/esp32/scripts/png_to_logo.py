#!/usr/bin/env python3
"""
Convert PNG image to compressed LVGL logo format
Converts PNG -> RGB565 -> Indexed color format for splash screen
"""

import sys
import os
from PIL import Image

def rgb_to_rgb565(r, g, b):
    """Convert RGB888 to RGB565"""
    return ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3)

def convert_png_to_logo(png_file, output_file, target_size=(180, 180)):
    """Convert PNG to compressed LVGL logo format"""
    
    if not os.path.exists(png_file):
        print(f"Error: PNG file not found: {png_file}")
        return False
    
    print(f"Loading PNG: {png_file}")
    
    # Load and resize image
    img = Image.open(png_file)
    
    # Handle transparency - composite on black background (matches splash screen)
    if img.mode == 'RGBA':
        print("Image has transparency - compositing on black background...")
        # Create black background
        background = Image.new('RGB', img.size, (0, 0, 0))
        # Composite the image on black background
        background.paste(img, mask=img.split()[3])  # Use alpha channel as mask
        img = background
    elif img.mode != 'RGB':
        print(f"Converting image from {img.mode} to RGB...")
        img = img.convert('RGB')
    
    # Use high-quality resampling for better downscaling
    if img.size != target_size:
        print(f"Resizing from {img.size} to {target_size}...")
        # For logos with sharp edges, use LANCZOS with high-quality settings
        # LANCZOS preserves edges better than HAMMING for graphics/logos
        img = img.resize(target_size, Image.Resampling.LANCZOS)
    
    width, height = img.size
    print(f"Image size: {width}x{height} pixels")
    
    # Check what colors we have before quantization
    pixels_before = img.load()
    sample_colors = set()
    for y in range(0, min(10, height)):
        for x in range(0, min(10, width)):
            r, g, b = pixels_before[x, y]
            sample_colors.add((r, g, b))
    print(f"Sample colors before quantization: {len(sample_colors)} unique RGB values")
    
    # Don't quantize - convert directly to RGB565 to preserve all colors
    # Quantization was causing white logo to be lost
    print("Converting directly to RGB565 (no quantization)...")
    
    # Convert to RGB565 directly
    colors = []
    pixels = img.load()
    for y in range(height):
        for x in range(width):
            r, g, b = pixels[x, y]
            rgb565 = rgb_to_rgb565(r, g, b)
            colors.append(rgb565)
    
    print(f"Converted {len(colors)} pixels to RGB565")
    print(f"Original size: {len(colors) * 2} bytes ({len(colors) * 2 / 1024:.1f} KB)")
    
    # Build color palette (find unique colors)
    unique_colors = list(set(colors))
    num_colors = len(unique_colors)
    
    print(f"Unique colors after RGB565 conversion: {num_colors}")
    
    # Check if we have non-black colors
    non_black = [c for c in unique_colors if c != 0x0000]
    print(f"Non-black colors: {len(non_black)}")
    
    if num_colors > 256:
        print("Warning: Logo has more than 256 colors, cannot use indexed format")
        print("Consider reducing colors in the source image")
        return False
    
    # Determine bit depth needed
    if num_colors <= 2:
        bit_depth = 1
        cf_format = "LV_IMG_CF_INDEXED_1BIT"
    elif num_colors <= 4:
        bit_depth = 2
        cf_format = "LV_IMG_CF_INDEXED_2BIT"
    elif num_colors <= 16:
        bit_depth = 4
        cf_format = "LV_IMG_CF_INDEXED_4BIT"
    else:
        bit_depth = 8
        cf_format = "LV_IMG_CF_INDEXED_8BIT"
    
    # Create color palette (map color -> index)
    # Sort colors to ensure consistent ordering (black first, white last)
    unique_colors_sorted = sorted(unique_colors)
    palette = {color: idx for idx, color in enumerate(unique_colors_sorted)}
    
    # Debug: check a few mappings
    print(f"Palette size: {len(palette)}")
    print(f"First color (index 0): 0x{unique_colors_sorted[0]:04X}")
    if len(unique_colors_sorted) > 1:
        print(f"Last color (index {len(unique_colors_sorted)-1}): 0x{unique_colors_sorted[-1]:04X}")
    
    # Convert pixels to indices
    indices = [palette[color] for color in colors]
    
    # Debug: check index distribution from center (where logo should be)
    center_start = (width * height // 2) - (width * 5)  # Middle minus 5 rows
    center_end = (width * height // 2) + (width * 5)  # Middle plus 5 rows
    center_indices = indices[center_start:center_end]
    index_counts = {}
    for idx in center_indices:
        index_counts[idx] = index_counts.get(idx, 0) + 1
    print(f"Center index distribution: {dict(sorted(index_counts.items(), key=lambda x: x[1], reverse=True)[:10])}")
    
    # Check if we have white pixels
    white_idx = palette.get(0xFFFF, -1)
    if white_idx >= 0:
        white_count = sum(1 for idx in indices if idx == white_idx)
        print(f"White pixels (index {white_idx}): {white_count} out of {len(indices)}")
    else:
        print("WARNING: White color (0xFFFF) not found in palette!")
    
    # Update unique_colors for palette writing
    unique_colors = unique_colors_sorted
    
    # Calculate compressed size
    palette_size = len(unique_colors) * 2  # Each color is 2 bytes (RGB565)
    if bit_depth == 1:
        indices_size = (len(indices) + 7) // 8  # 1 bit per pixel
    elif bit_depth == 2:
        indices_size = (len(indices) + 3) // 4  # 2 bits per pixel
    elif bit_depth == 4:
        indices_size = (len(indices) + 1) // 2  # 4 bits per pixel
    else:  # 8 bit
        indices_size = len(indices)  # 1 byte per pixel
    
    total_size = palette_size + indices_size
    compression_ratio = (len(colors) * 2) / total_size
    
    print(f"Compressed size: {total_size} bytes ({total_size / 1024:.1f} KB)")
    print(f"Compression ratio: {compression_ratio:.1f}x")
    print(f"Using format: {cf_format}")
    
    # Generate new C file
    # LVGL indexed format: data array contains palette (uint16_t) followed by indices (packed)
    output = f"""// Auto-generated logo image for splash screen (COMPRESSED)
// Generated from {os.path.basename(png_file)}
// Compressed using indexed color format ({num_colors} colors, {bit_depth}-bit indices)
// Format: palette ({num_colors} * 2 bytes) + indices ({indices_size} bytes) = {total_size} bytes
#include <lvgl.h>

#define LOGO_SPLASH_WIDTH {width}
#define LOGO_SPLASH_HEIGHT {height}

// Combined data: palette (uint16_t colors) + packed indices
static const uint8_t logo_splash_data[] = {{
    // Palette: {num_colors} colors as little-endian uint16_t
"""
    
    # Write palette as bytes (little-endian for RGB565)
    for i in range(0, len(unique_colors), 4):
        colors_line = unique_colors[i:i+4]
        palette_bytes = []
        for color in colors_line:
            palette_bytes.append(f"0x{color & 0xFF:02X}")
            palette_bytes.append(f"0x{(color >> 8) & 0xFF:02X}")
        output += f"    {', '.join(palette_bytes)},\n"
    
    output += f"    // Indices: {len(indices)} pixels packed as {bit_depth}-bit values\n"
    
    # Write indices
    if bit_depth == 1:
        # Pack 8 pixels per byte
        for i in range(0, len(indices), 8):
            byte = 0
            for j in range(8):
                if i + j < len(indices):
                    byte |= (indices[i + j] & 0x01) << (7 - j)
            output += f"    0x{byte:02X},\n"
    elif bit_depth == 2:
        # Pack 4 pixels per byte
        for i in range(0, len(indices), 4):
            byte = 0
            for j in range(4):
                if i + j < len(indices):
                    byte |= (indices[i + j] & 0x03) << (6 - j * 2)
            output += f"    0x{byte:02X},\n"
    elif bit_depth == 4:
        # Pack 2 pixels per byte
        for i in range(0, len(indices), 2):
            byte = 0
            if i < len(indices):
                byte |= (indices[i] & 0x0F) << 4
            if i + 1 < len(indices):
                byte |= indices[i + 1] & 0x0F
            output += f"    0x{byte:02X},\n"
    else:  # 8 bit
        # 1 pixel per byte
        for i in range(0, len(indices), 16):
            indices_line = indices[i:i+16]
            indices_str = ', '.join(f'0x{idx:02X}' for idx in indices_line)
            output += f"    {indices_str},\n"
    
    output += "};\n\n"
    
    # Write image descriptor
    output += f"""const lv_img_dsc_t logo_splash_img = {{
    .header = {{
        .cf = {cf_format},
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
    
    print(f"\n✓ Compressed logo written to: {output_file}")
    return True

if __name__ == '__main__':
    script_dir = os.path.dirname(os.path.abspath(__file__))
    
    if len(sys.argv) < 2:
        # Default: use the PNG from assets
        png_file = os.path.join(script_dir, '../../../assets/sizes/app-icon/vertical/white/Brewos-White.png')
        output_file = os.path.join(script_dir, '../src/ui/logo_splash.c')
    else:
        png_file = sys.argv[1]
        if len(sys.argv) > 2:
            output_file = sys.argv[2]
        else:
            output_file = os.path.join(script_dir, '../src/ui/logo_splash.c')
    
    if convert_png_to_logo(png_file, output_file):
        print("\n✓ Conversion complete!")
    else:
        print("\n✗ Conversion failed")
        sys.exit(1)

