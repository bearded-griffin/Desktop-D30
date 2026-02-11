import os
import json
import re

ICON_ROOT = "assets/icons/built-in"
OUTPUT_FILE = "assets/icons/metadata.json"

def get_tokens(filename):
    # "pickup-truck.png" -> ["pickup", "truck"]
    name_only = os.path.splitext(filename)[0]
    tokens = re.split(r'[^a-zA-Z0-9]', name_only.lower())
    return [t for t in tokens if t]

def format_name(filename):
    # "pickup-truck.png" -> "Pickup Truck"
    name = os.path.splitext(filename)[0]
    name = name.replace('-', ' ').replace('_', ' ')
    return name.title()

def generate_metadata():
    if not os.path.exists(ICON_ROOT):
        print(f"Error: {ICON_ROOT} not found.")
        return

    metadata = {}
    
    # Load existing if any, so we don't overwrite user changes
    if os.path.exists(OUTPUT_FILE):
        try:
            with open(OUTPUT_FILE, 'r') as f:
                metadata = json.load(f)
        except:
            pass

    processed_count = 0
    
    for root, dirs, files in os.walk(ICON_ROOT):
        category = os.path.basename(root)
        
        for filename in files:
            if not filename.lower().endswith(('.png', '.jpg', '.jpeg')):
                continue
                
            # Path relative to assets/icons root is complex because of "built-in" vs "user"
            # The C++ app uses full paths or relative to execution. 
            # AssetManager uses `entry.path().string()` which is relative to CWD (project root).
            # So "assets/icons/built-in/Office/pen.png"
            
            full_path = os.path.join(root, filename)
            
            # Generate smart data
            pretty_name = format_name(filename)
            tags = get_tokens(filename)
            
            # Add category as a tag if it's not "built-in" or "icons"
            if category not in ["built-in", "icons"]:
                tags.append(category.lower())
            
            # Update metadata if entry doesn't exist
            # We key by the PATH as C++ sees it
            if full_path not in metadata:
                metadata[full_path] = {
                    "name": pretty_name,
                    "tags": list(set(tags)) # Unique tags
                }
                processed_count += 1

    with open(OUTPUT_FILE, "w") as f:
        json.dump(metadata, f, indent=4)
        
    print(f"Generated metadata for {processed_count} new icons.")
    print(f"Saved to {OUTPUT_FILE}")

if __name__ == "__main__":
    generate_metadata()
