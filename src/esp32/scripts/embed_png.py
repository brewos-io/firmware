#!/usr/bin/env python3
"""
Embed PNG file directly as binary data for LVGL
LVGL supports PNG natively, so we can use the compressed PNG file as-is
"""

import sys
import os

def embed_png(png_file, output_file):
    """Embed PNG file as binary C array"""
    
    if not os.path.exists(png_file):
        print(f"Error: PNG file not found: {png_file}")
        return False
    
    # Read PNG file
    with open(png_file, 'rb') as f:
        png_data = f.read()
    
    file_size = len(png_data)
    print(f"Embedding PNG: {png_file}")
    print(f"File size: {file_size} bytes ({file_size / 1024:.1f} KB)")
    
    # Parse PNG dimensions from IHDR chunk (bytes 16-24)
    # PNG signature: 8 bytes, then IHDR chunk starts at byte 8
    # Width is bytes 16-19, Height is bytes 20-23 (big-endian)
    if len(png_data) >= 24:
        width = (png_data[16] << 24) | (png_data[17] << 16) | (png_data[18] << 8) | png_data[19]
        height = (png_data[20] << 24) | (png_data[21] << 16) | (png_data[22] << 8) | png_data[23]
        print(f"PNG dimensions: {width}x{height}")
    else:
        width = 0
        height = 0
        print("Warning: Could not parse PNG dimensions")
    
    # Generate C file with embedded PNG data
    output = f"""// Auto-generated logo image for splash screen (EMBEDDED PNG)
// Generated from {os.path.basename(png_file)}
// LVGL supports PNG natively - no conversion needed!
// File size: {file_size} bytes ({file_size / 1024:.1f} KB)
#include <lvgl.h>

// Embedded PNG data
static const uint8_t logo_splash_data[] = {{
"""
    
    # Write binary data as hex bytes (16 bytes per line for readability)
    for i in range(0, file_size, 16):
        chunk = png_data[i:i+16]
        hex_bytes = ', '.join(f'0x{b:02X}' for b in chunk)
        output += f"    {hex_bytes},\n"
    
    output += "};\n\n"
    
    # Create LVGL image descriptor
    # LVGL 8.x supports PNG through extra/libs/png
    # Use LV_IMG_CF_RAW - LVGL will decode PNG if decoder is enabled
    output += f"""const lv_img_dsc_t logo_splash_img = {{
    .header = {{
        .cf = LV_IMG_CF_RAW,  // LVGL will decode PNG if PNG decoder is enabled
        .w = {width},
        .h = {height},
    }},
    .data_size = {file_size},
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
    
    print(f"\n✓ PNG embedded to: {output_file}")
    print(f"  LVGL will automatically decode the PNG format")
    return True

if __name__ == '__main__':
    script_dir = os.path.dirname(os.path.abspath(__file__))
    
    if len(sys.argv) < 2:
        # Default: use the PNG from assets
        png_file = os.path.join(script_dir, '../../../assets/sizes/app-icon/vertical/white/small.png')
        output_file = os.path.join(script_dir, '../src/ui/logo_splash.c')
    else:
        png_file = sys.argv[1]
        if len(sys.argv) > 2:
            output_file = sys.argv[2]
        else:
            output_file = os.path.join(script_dir, '../src/ui/logo_splash.c')
    
    if embed_png(png_file, output_file):
        print("\n✓ Embedding complete!")
    else:
        print("\n✗ Embedding failed")
        sys.exit(1)

