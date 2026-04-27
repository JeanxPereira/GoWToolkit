#!/usr/bin/env python3
"""
GOWToolkit - SF Symbols Auto-Builder Pipeline
Usage:
  python3 tools/build_icons.py --svgs /path/to/sf_svgs --out-ttf third_party/fonts/SFSymbols.ttf --out-header src/fonts/SFSymbols.h

This script orchestrates the generation of the TTF using Fantasticon (Node.js) 
and immediately scrubs and safely remaps the TTF cmap table using FontTools to fit identically 
inside ImGui's 16-bit text engine (U+E000 to U+FFFF), preventing corruption!
"""

import argparse
import subprocess
import sys
import shutil
import glob
import os
import tempfile
import json
from pathlib import Path

try:
    from fontTools.ttLib import TTFont
except ImportError:
    print("ERROR: 'fonttools' not found. Run: pip3 install fonttools")
    sys.exit(1)

def run(*args, **kwargs):
    print(f"Executing: {' '.join(args[0] if isinstance(args[0], list) else [args[0]])}")
    subprocess.run(*args, check=True, **kwargs)

def build_icons(svg_dir, out_ttf, out_header):
    svg_dir = Path(svg_dir).resolve()
    out_ttf = Path(out_ttf).resolve()
    out_header = Path(out_header).resolve()

    if not svg_dir.exists() or not svg_dir.is_dir():
        print(f"ERROR: SVGs folder does not exist: {svg_dir}")
        sys.exit(1)

    print("=== 1. Validating Environment ===")
    if shutil.which("npx") is None:
        print("ERROR: Node.js (npx) is required. Install with: brew install node")
        sys.exit(1)

    # Check for svgs and optionally extract
    svgs = list(svg_dir.glob("*.svg"))
    if len(svgs) == 0:
        print(f"No SVGs found in {svg_dir}. Attempting to extract directly from SF Symbols.app...")
        svg_dir.mkdir(parents=True, exist_ok=True)
        try:
            run(["npx", "sf-symbols-svg", "--weight", "regular", "--output", str(svg_dir), "--size", "24", "--padding", "0"])
            svgs = list(svg_dir.glob("*.svg"))
        except:
            print("ERROR: Failed to extract SVGs. Make sure you have macOS SF Symbols.app installed.")
            sys.exit(1)
            
    print(f"Found {len(svgs)} SVGs ready for processing.")

    with tempfile.TemporaryDirectory() as tmpdir:
        tmp_path = Path(tmpdir)
        print(f"=== 2. Generating raw TTF via Fantasticon in {tmp_path} ===")
        
        # Create .fantasticonrc.js forcing TTF output only
        rc_path = tmp_path / ".fantasticonrc.js"
        rc_path.write_text("""
module.exports = {
  inputDir: '""" + str(svg_dir) + """',
  outputDir: '""" + str(tmp_path) + """',
  fontTypes: ['ttf'],
  assetTypes: ['json'],
  name: 'raw_icons'
};
        """)
        
        run(["npx", "fantasticon", "-c", str(rc_path)])
        
        raw_ttf = tmp_path / "raw_icons.ttf"
        raw_json = tmp_path / "raw_icons.json"
        
        if not raw_ttf.exists():
            print("ERROR: Fantasticon failed to generate the font.")
            sys.exit(1)

        print("=== 3. Remapping TTF to 16-bit standard (PUA E000) ===")
        
        font = TTFont(str(raw_ttf))
        glyph_names = font.getGlyphOrder()

        new_cmap = {}
        header_content = "#pragma once\n\n"
        header_content += '// SF Symbols Auto-Generated Header Mapping\n'
        header_content += '// Automatically mapped to the 16-bit Private Use Area (E000+)\n\n'
        header_content += '#define FONT_ICON_FILE_NAME_SF "SFSymbols.ttf"\n\n'

        start_codepoint = 0xE000
        current_cp = start_codepoint

        for name in glyph_names:
            if name in ['.notdef', '.null', 'nonmarkingreturn']:
                continue
            
            new_cmap[current_cp] = name
            
            clean_name = name.upper().replace('.', '_').replace('-', '_')
            header_content += f'#define ICON_SF_{clean_name} "\\u{current_cp:04x}" // U+{current_cp:04x}\n'
            
            current_cp += 1

        if current_cp > 0xFFFF:
            print("WARNING: The number of icons exceeded FFFF! ImGui may have rendering issues.")

        print("Clearing font cmaps and injecting safe cmap...")
        cmap_table = font['cmap']
        for table in cmap_table.tables:
            table.cmap = new_cmap

        print(f"=== 4. Exporting Final Files ===")
        out_ttf.parent.mkdir(parents=True, exist_ok=True)
        out_header.parent.mkdir(parents=True, exist_ok=True)

        font.save(str(out_ttf))
        out_header.write_text(header_content)

        print(f"SUCCESS!")
        print(f"    TTF Saved: {out_ttf} ({len(new_cmap)} icons remapped)")
        print(f"    HDR Saved: {out_header}")
        print("\\nPipeline Completed Successfully.")

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description="SVG to TTF Builder for GOWToolkit")
    parser.add_argument("--svgs", required=True, help="Directory containing the SF Symbols SVGs")
    parser.add_argument("--out-ttf", required=True, help="Output SFSymbols.ttf file path")
    parser.add_argument("--out-header", required=True, help="Output SFSymbols.h file path")
    args = parser.parse_args()

    build_icons(args.svgs, args.out_ttf, args.out_header)er)
