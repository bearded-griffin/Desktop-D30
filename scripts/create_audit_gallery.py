import os

ICON_ROOT = "assets/icons/built-in"
OUTPUT_FILE = "icon_audit.html"

def generate_gallery():
    html = """
    <html>
    <head>
        <title>Icon Audit Gallery</title>
        <style>
            body { font-family: sans-serif; background: #f0f0f0; padding: 20px; }
            .category { background: white; margin-bottom: 30px; padding: 20px; border-radius: 8px; box-shadow: 0 2px 5px rgba(0,0,0,0.1); }
            .category h2 { border-bottom: 2px solid #333; padding-bottom: 10px; color: #2c3e50; }
            .grid { display: grid; grid-template-columns: repeat(auto-fill, minmax(120px, 1fr)); gap: 15px; }
            .icon-card { text-align: center; background: #fafafa; padding: 10px; border: 1px solid #ddd; border-radius: 4px; }
            .icon-card img { max-width: 64px; max-height: 64px; display: block; margin: 0 auto 10px; }
            .icon-card span { font-size: 11px; word-break: break-all; color: #666; }
        </style>
    </head>
    <body>
        <h1>Desktop-D30 Icon Audit</h1>
    """

    if not os.path.exists(ICON_ROOT):
        html += f"<h2>Error: {ICON_ROOT} not found.</h2>"
    else:
        categories = sorted([d for d in os.listdir(ICON_ROOT) if os.path.isdir(os.path.join(ICON_ROOT, d))])
        
        for cat in categories:
            cat_path = os.path.join(ICON_ROOT, cat)
            icons = sorted([f for f in os.listdir(cat_path) if f.lower().endswith(('.png', '.jpg', '.jpeg'))])
            
            if not icons: continue

            html += f'<div class="category"><h2>{cat} ({len(icons)})</h2><div class="grid">'
            for icon in icons:
                rel_path = os.path.join(cat_path, icon)
                html += f"""
                    <div class="icon-card">
                        <img src="{rel_path}" alt="{icon}">
                        <span>{icon}</span>
                    </div>
                """
            html += "</div></div>"

    html += "</body></html>"
    
    with open(OUTPUT_FILE, "w") as f:
        f.write(html)
    
    print(f"Gallery generated: {os.path.abspath(OUTPUT_FILE)}")

if __name__ == "__main__":
    generate_gallery()
