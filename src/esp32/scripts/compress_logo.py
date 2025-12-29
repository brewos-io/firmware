#!/usr/bin/env python3
"""
Compress logo_splash.c to indexed color format for LVGL
Reduces flash usage from 64KB to ~8-16KB for logos with few colors
"""

import sys
import os

def convert_to_indexed(input_file, output_file):
    """Convert RGB565 logo to indexed color format"""
    
    # Read the current logo file
    with open(input_file, 'r') as f:
        content = f.read()
    
    # Extract the data array - try different formats
    array_patterns = [
        'static const uint16_t logo_splash_data[] = {',
        'static const uint16_t logo_splash_data[]',
        'const uint16_t logo_splash_data[] = {',
    ]
    
    start_idx = -1
    for pattern in array_patterns:
        start_idx = content.find(pattern)
        if start_idx != -1:
            break
    
    if start_idx == -1:
        print(f"Error: Could not find logo_splash_data array in {input_file}")
        print("Looking for patterns:", array_patterns)
        return False
    
    # Find the array end
    end_idx = content.find('};', start_idx)
    if end_idx == -1:
        print("Error: Could not find array end")
        return False
    
    # Extract array content
    array_start = content.find('{', start_idx) + 1
    array_content = content[array_start:end_idx]
    
    # Parse all color values
    colors = []
    for line in array_content.split('\n'):
        line = line.strip()
        if not line or line.startswith('//'):
            continue
        # Remove trailing comma
        line = line.rstrip(',')
        # Split by comma and parse hex values
        for val in line.split(','):
            val = val.strip()
            if val:
                try:
                    # Handle hex format (0xXXXX)
                    if val.startswith('0x'):
                        color = int(val, 16)
                    else:
                        color = int(val)
                    colors.append(color)
                except ValueError:
                    continue
    
    print(f"Found {len(colors)} pixels")
    print(f"Original size: {len(colors) * 2} bytes ({len(colors) * 2 / 1024:.1f} KB)")
    
    # Build color palette (find unique colors)
    unique_colors = list(set(colors))
    num_colors = len(unique_colors)
    
    print(f"Unique colors: {num_colors}")
    
    if num_colors > 256:
        print("Warning: Logo has more than 256 colors, cannot use indexed format")
        print("Consider reducing colors in the source image or using PNG format")
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
    palette = {color: idx for idx, color in enumerate(unique_colors)}
    
    # Convert pixels to indices
    indices = [palette[color] for color in colors]
    
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
// Generated from logo-vert.png
// Compressed using indexed color format ({num_colors} colors, {bit_depth}-bit indices)
// Format: palette ({num_colors} * 2 bytes) + indices ({indices_size} bytes) = {total_size} bytes
#include <lvgl.h>

#define LOGO_SPLASH_WIDTH 180
#define LOGO_SPLASH_HEIGHT 180

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
        .w = 180,
        .h = 180,
    }},
    .data_size = {total_size},
    .data = logo_splash_data,
}};
"""
    
    # Write output file
    with open(output_file, 'w') as f:
        f.write(output)
    
    print(f"\nCompressed logo written to: {output_file}")
    return True

if __name__ == '__main__':
    script_dir = os.path.dirname(os.path.abspath(__file__))
    input_file = os.path.join(script_dir, '../src/ui/logo_splash.c')
    output_file = os.path.join(script_dir, '../src/ui/logo_splash.c')
    
    if len(sys.argv) > 1:
        input_file = sys.argv[1]
    if len(sys.argv) > 2:
        output_file = sys.argv[2]
    
    # Backup original
    backup_file = output_file + '.backup'
    if os.path.exists(output_file) and not os.path.exists(backup_file):
        import shutil
        shutil.copy2(output_file, backup_file)
        print(f"Backed up original to: {backup_file}")
    
    if convert_to_indexed(input_file, output_file):
        print("\n✓ Compression complete!")
        print("Note: You may need to adjust the image descriptor if LVGL format differs")
    else:
        print("\n✗ Compression failed")

