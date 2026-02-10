import os
import shutil

# Root path where your 729 icons are
SOURCE_DIR = "assets/icons/built-in"
# Where they will end up (can be the same dir)
TARGET_DIR = "assets/icons/built-in"

# Mapping of Category Name -> Keywords found in filenames
MAPPING = {
    "Office": ["pen", "paper", "printer", "folder", "file", "mail", "desk", "book", "calc", "note", "clip", "signature", "stamp", "calendar"],
    "Kitchen": ["fork", "knife", "spoon", "plate", "mug", "cook", "food", "chef", "wine", "drink", "eat", "oven", "stove", "fridge", "bowl"],
    "Home": ["house", "bed", "bath", "sofa", "key", "lamp", "clock", "chair", "door", "window", "garden", "living", "room", "pillow", "fan"],
    "Electrical": ["bolt", "plug", "wire", "battery", "bulb", "flash", "power", "tech", "cpu", "chip", "circuit", "cable", "signal", "wifi"],
    "Warning": ["alert", "danger", "stop", "hazard", "warning", "caution", "forbidden", "no", "exit", "emergency", "biohazard", "fire", "radioactive"],
    "Sports": ["ball", "bat", "goal", "run", "swim", "bike", "gym", "fitness", "yoga", "soccer", "trophy", "winner", "medal", "sport"],
    "Weather": ["sun", "moon", "cloud", "rain", "snow", "wind", "temp", "thermometer", "storm", "sunny", "cloudy", "snowflake", "lightning"],
    "Health": ["pill", "heart", "med", "cross", "tooth", "doctor", "hospital", "dna", "fitness", "surgery", "aid", "clinic", "nurse"],
    "Tools": ["hammer", "wrench", "screw", "saw", "drill", "build", "fix", "repair", "tape", "tool", "pliers", "level", "shovel", "axe"],
    "Media": ["play", "video", "music", "audio", "mic", "cam", "photo", "image", "sound", "speaker", "record", "movie", "lens"],
    "Animals": ["dog", "cat", "bird", "fish", "paw", "animal", "pet", "zoo", "wild", "bear", "lion", "tiger", "horse", "cow", "pig", "sheep"],
    "Travel": ["plane", "car", "bus", "ship", "map", "pin", "globe", "luggage", "hotel", "beach", "mountain", "compass", "tent", "camping"],
    "Beauty": ["comb", "scissors", "mirror", "lipstick", "perfume", "hair", "makeup"],
    "Education": ["graduation", "school", "pencil", "student", "teacher", "classroom", "diploma"],
    "Clothing": ["shirt", "pants", "dress", "shoe", "hat", "jacket", "sock", "tie", "bag"],
    "Traffic": ["traffic", "light", "sign", "pedestrian", "bicycle", "parking", "street", "road"]
}

def sort_icons():
    if not os.path.exists(SOURCE_DIR):
        print(f"Error: Source directory '{SOURCE_DIR}' not found!")
        return

    # Get all files in the base built-in directory
    # (Note: We only look at files in the ROOT of SOURCE_DIR)
    files = [f for f in os.listdir(SOURCE_DIR) if os.path.isfile(os.path.join(SOURCE_DIR, f))]
    
    print(f"Analyzing {len(files)} icons...")
    
    moved_count = 0
    
    for filename in files:
        # Check extensions
        if not filename.lower().endswith((".png", ".jpg", ".jpeg")):
            continue
            
        lower_name = filename.lower()
        assigned_category = "Uncategorized"
        
        # Check against keywords
        # We sort keywords by length descending to match "lightning" before "light"
        for category in MAPPING:
            keywords = sorted(MAPPING[category], key=len, reverse=True)
            if any(key in lower_name for key in keywords):
                assigned_category = category
                break
        
        # Create destination path
        dest_path = os.path.join(TARGET_DIR, assigned_category)
        os.makedirs(dest_path, exist_ok=True)
        
        # Source and Destination
        src = os.path.join(SOURCE_DIR, filename)
        dst = os.path.join(dest_path, filename)
        
        try:
            # Using move instead of copy to "clean up" the root
            shutil.move(src, dst)
            moved_count += 1
        except Exception as e:
            print(f"Failed to move {filename}: {e}")

    print(f"Finished! Organized {moved_count} icons into categorized folders.")
    print(f"Check '{os.path.join(TARGET_DIR, 'Uncategorized')}' for any items that didn't match keywords.")

if __name__ == "__main__":
    sort_icons()
