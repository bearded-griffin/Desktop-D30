import os
from PIL import Image, ImageDraw

def create_border(width, height, style_name, draw_func):
    # Create a transparent image
    img = Image.new('RGBA', (width, height), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)
    
    # Execute the specific style logic
    draw_func(draw, width, height)
    
    # Save to the assets folder
    output_dir = "assets/borders/Built-in"
    if not os.path.exists(output_dir):
        os.makedirs(output_dir)
        
    img.save(f"{output_dir}/{style_name}.png")
    print(f"Generated: {style_name}.png")

# --- STYLE FUNCTIONS ---

def style_classic_double(draw, w, h):
    # Outer thin line
    draw.rectangle([2, 2, w-3, h-3], outline="black", width=2)
    # Inner thicker line
    draw.rectangle([8, 8, w-9, h-9], outline="black", width=4)

def style_modern_geometric(draw, w, h):
    # Main frame
    draw.rectangle([4, 4, w-5, h-5], outline="black", width=3)
    # Corner "brackets"
    s = 25
    # Top Left
    draw.rectangle([0, 0, s, s], fill="black")
    # Top Right
    draw.rectangle([w-s, 0, w, s], fill="black")
    # Bottom Left
    draw.rectangle([0, h-s, s, h], fill="black")
    # Bottom Right
    draw.rectangle([w-s, h-s, w, h], fill="black")

def style_industrial_dashed(draw, w, h):
    # Thick top and bottom bars
    draw.rectangle([0, 0, w, 6], fill="black")
    draw.rectangle([0, h-7, w, h], fill="black")
    # Dashed side lines
    for y in range(10, h-10, 10):
        draw.rectangle([2, y, 6, y+5], fill="black")
        draw.rectangle([w-7, y, w-3, y+5], fill="black")

def style_elegant_minimal(draw, w, h):
    # Only draw on the ends, leaving the middle clean
    pad = 10
    draw.line([pad, pad, w-pad, pad], fill="black", width=2)
    draw.line([pad, h-pad, w-pad, h-pad], fill="black", width=2)
    # Fancy end caps
    draw.rectangle([pad, pad, pad+10, h-pad], outline="black", width=2)
    draw.rectangle([w-pad-10, pad, w-pad, h-pad], outline="black", width=2)

def style_rounded_outline(draw, w, h):
    # Simple thick rounded border
    radius = 20
    draw.rounded_rectangle([4, 4, w-5, h-5], radius=radius, outline="black", width=5)

def style_bracket_ends(draw, w, h):
    # Draws [ ] style brackets on the ends
    bw = 20 # bracket width
    draw.line([bw, 10, w-bw, 10], fill="black", width=2)
    draw.line([bw, h-11, w-bw, h-11], fill="black", width=2)
    # Brackets
    draw.line([bw, 10, 5, 10], fill="black", width=4)
    draw.line([5, 10, 5, h-11], fill="black", width=4)
    draw.line([5, h-11, bw, h-11], fill="black", width=4)
    
    draw.line([w-bw, 10, w-6, 10], fill="black", width=4)
    draw.line([w-6, 10, w-6, h-11], fill="black", width=4)
    draw.line([w-6, h-11, w-bw, h-11], fill="black", width=4)

def style_art_deco_sunburst(draw, w, h):
    # Main frame
    draw.rectangle([8, 8, w-9, h-9], outline="black", width=2)
    # Sunbursts in corners
    s = 45
    import math
    for cx, cy, start in [(0,0, 0), (w,0, 90), (0,h, 270), (w,h, 180)]:
        for a in range(0, 91, 15):
            rad = math.radians(a + start)
            draw.line([cx, cy, cx + math.cos(rad)*s, cy + math.sin(rad)*s], fill="black", width=2)

def style_art_deco_stepped(draw, w, h):
    # Tiered/Ziggurat corners
    draw.rectangle([12, 12, w-13, h-13], outline="black", width=3)
    for dx, dy in [(0,0), (w,0), (0,h), (w,h)]:
        for s in [35, 25, 15]:
            x0 = 0 if dx == 0 else w-s
            y0 = 0 if dy == 0 else h-s
            x1 = s if dx == 0 else w
            y1 = s if dy == 0 else h
            draw.rectangle([x0, y0, x1, y1], fill="black")

def style_art_deco_corners(draw, w, h):
    # Outer frame
    draw.rectangle([10, 10, w-11, h-11], outline="black", width=2)
    # Fan flourishes in the corners instead of the middle
    s = 40
    # Top Left
    draw.pieslice([-s//2, -s//2, s, s], 0, 90, fill="black")
    # Top Right
    draw.pieslice([w-s-1, -s//2, w+s//2, s], 90, 180, fill="black")
    # Bottom Left
    draw.pieslice([-s//2, h-s-1, s, h+s//2], 270, 360, fill="black")
    # Bottom Right
    draw.pieslice([w-s-1, h-s-1, w+s//2, h+s//2], 180, 270, fill="black")

# --- RUN GENERATION ---
# 400x120 is a good "standard" large label ratio (15mm x 50mm)
width, height = 400, 120

create_border(width, height, "Classic_Double", style_classic_double)
create_border(width, height, "Modern_Geo", style_modern_geometric)
create_border(width, height, "Industrial", style_industrial_dashed)
create_border(width, height, "Elegant_Endcaps", style_elegant_minimal)
create_border(width, height, "Rounded_Outline", style_rounded_outline)
create_border(width, height, "Brackets", style_bracket_ends)
create_border(width, height, "Deco_Sunburst", style_art_deco_sunburst)
create_border(width, height, "Deco_Stepped", style_art_deco_stepped)
create_border(width, height, "Deco_Corners", style_art_deco_corners)
