import os
import shutil
import re

# Root path where icons live
BASE_DIR = "assets/icons/built-in"

# Mapping of Category Name -> Keywords found in filenames
MAPPING = {
    "Office": ["sd-card","broadcast","wifi","keyboard","bluetooth","mouse","mobile","computer","monitor","laptop","headphone","pen", "paper", "printer", "folder", "file", "mail", "desk", "book", "calc", "note", "clip", "signature", "stamp", "calendar", "envelope", "briefcase", "attach", "copy", "paste", "save", "print", "fax"],
    "Kitchen": ["fork", "knife", "spoon", "plate", "mug", "cook", "drink", "eat", "oven", "stove", "fridge", "bowl", "cup", "glass", "bottle", "pan", "pot", "grill", "kettle", "toaster", "blender", "microwave","cart"],
    "Games": ["paper-plane","hourglass","game", "controller", "joystick", "console", "dice", "chess", "puzzle", "card", "board"],
    "Animals": ["prawn","feather","dog", "cat", "bird", "fish", "paw", "animal", "wild", "bear", "lion", "tiger", "horse", "cow", "pig", "sheep", "duck", "owl", "rabbit", "snake", "turtle", "monkey", "elephant", "butterfly", "bug", "insect", "spider"],
    "Food": ["baguette","food", "chef", "wine", "drink", "eat", "bread", "cake", "fruit", "vege", "apple", "banana", "coffee", "tea", "meat", "pizza", "burger", "cookie", "donut", "icecream","beer", "cocktail", "soup", "salad", "sandwich", "steak", "seafood", "pasta", "rice","avacado", "carrot", "corn", "grape", "lemon", "melon", "orange", "peach", "pear", "pineapple", "strawberry", "watermelon"],                                       
    "Home": ["yarn","balloon","tv","radio","house","piggy-bank", "bed", "bath", "sofa", "key", "lamp", "clock", "chair", "door", "window", "garden", "living", "room", "pillow", "fan", "bucket", "broom", "mop", "vacuum", "iron", "laundry", "hanger", "trash", "bin"],                                    
    "Weather": ["heat","heat-wave","sun", "cloud", "rain", "snow", "wind", "temp", "thermometer", "storm", "sunny", "cloudy", "snowflake", "lightning", "umbrella", "fog"],                                                                                                                        
    "Electrical": ["night-light","bolt", "plug", "wire", "battery", "bulb", "power", "cpu", "chip", "circuit", "cable"],
    "Warning": ["bell","alert", "danger", "stop", "hazard", "warning", "caution", "forbidden", "no", "exit", "emergency", "biohazard", "fire", "radioactive"],                                                                                                                                         
    "Sports": ["crosshair","cursor-crosshair","ball", "bat", "goal", "run", "swim", "bike", "gym", "fitness", "yoga", "soccer", "trophy", "winner", "medal", "sport", "basketball", "tennis", "football", "baseball", "golf", "hiking", "climb"],                                                           
    "Health": ["vial","bacteria","selfcare","reading-glass","glasses","tablet","pill-bottle", "heart", "med", "cross", "tooth", "doctor", "hospital", "dna", "fitness", "surgery", "aid", "clinic", "nurse", "stetho", "virus", "syringe", "lungs", "brain"],                                                       
    "Tools": ["spanner","hammer", "wrench", "screw", "saw", "drill", "build", "fix", "repair", "tape", "tool", "pliers", "level", "shovel", "axe", "ruler", "paint", "brush", "measure"],                                                                                                                 
    "Media": ["camera","cctv","play", "video", "music", "audio", "microphone", "photo", "image", "sound", "speaker", "record", "movie", "lens", "film", "equalizer", "headphones"],                                                                                                                     
    "Travel": ["plane", "car", "bus", "ship", "map", "pin", "globe", "luggage", "hotel", "beach", "mountain", "compass", "tent", "camping","camp", "ticket", "passport", "bicycle", "motorcycle", "train", "anchor","petrol","traffic", "light", "sign", "pedestrian", "bicycle", "parking", "street", "road", "gas", "fuel", "garage"],                                                                                                                       
    "Beauty": ["comb", "scissors", "mirror", "lipstick", "perfume", "hair", "makeup", "ring", "crown", "fashion"],                                    
    "Education": ["psychology","certificate","graduation", "school", "pencil", "student", "teacher", "classroom", "diploma", "eraser", "ruler", "microscope",      
    "science", "atom", "brain","intellect","education","microscope"],
    "Clothing": ["skirt","shirt", "pants", "dress", "shoe", "hat", "jacket", "sock", "tie", "bag", "t-shirt", "clothes", "hangers"],                          
    "Shapes": ["spade","laurel-wreath","circle", "square", "triangle", "star", "hexagon", "octagon", "diamond", "heart", "arrow", "line"],                   
    "Emojis": ["smile", "sad", "angry", "laugh", "cry", "wink", "happy-heart-eyes", "thumb", "face", "emoji", "emotion"],                              
    "Security": ["shield","lock", "unlock", "key", "keyhole", "password", "security", "privacy", "safe", "vault", "guard", "firewall","fingerprint"], 
    "Currency": ["dollar","euro","pound", "yen", "money", "cash", "coin", "bank", "credit", "card", "wallet", "finance", "payment"],                  
    "Space": ["rocket","satellite","planet", "star", "moon", "comet", "galaxy", "astronaut", "space", "ufo", "alien", "telescope"],                   
    "Math": ["function","calculator", "math", "sum", "divide", "multiply", "subtract", "plus", "minus", "equal", "percent", "fraction", "geometry", "algebra", "graph","lambda", "sigma", "pi", "theta", "delta", "integral", "derivative"]   
}

def get_tokens(filename):
    # Split by any non-alphanumeric character (hyphen, underscore, dot, space)
    # "pickup-truck.png" -> ["pickup", "truck", "png"]
    tokens = re.split(r'[^a-zA-Z0-9]', filename.lower())
    return [t for t in tokens if t]

def sort_icons():
    moved_total = 0
    
    # SAFE MODE: Only look at root and Uncategorized
    sources = [BASE_DIR, os.path.join(BASE_DIR, "Uncategorized")]
    
    print("Running in SAFE MODE (Only sorting Root and Uncategorized)...")

    for src_dir in sources:
        if not os.path.exists(src_dir): continue
        
        # Non-recursive list
        files = [f for f in os.listdir(src_dir) if os.path.isfile(os.path.join(src_dir, f))]
        
        for filename in files:
            if not filename.lower().endswith((".png", ".jpg", ".jpeg")):
                continue
                
            tokens = get_tokens(filename)
            best_category = None
            
            # Match tokens EXACTLY against keywords
            for category, keywords in MAPPING.items():
                if any(key in tokens for key in keywords):
                    best_category = category
                    break
            
            # Only move if we found a match AND we aren't already in that folder
            # (Note: src_dir is either root or Uncategorized, so if best_category is valid, we move)
            if best_category:
                dest_dir = os.path.join(BASE_DIR, best_category)
                os.makedirs(dest_dir, exist_ok=True)
                
                src = os.path.join(src_dir, filename)
                dst = os.path.join(dest_dir, filename)
                
                # Handle collisions
                if os.path.exists(dst) and src != dst:
                    name, ext = os.path.splitext(filename)
                    dst = os.path.join(dest_dir, f"{name}_alt{ext}")

                try:
                    shutil.move(src, dst)
                    moved_total += 1
                except Exception as e:
                    print(f"Failed to move {filename}: {e}")

    print(f"Finished! Sorted {moved_total} icons from Uncategorized/Root.")

if __name__ == "__main__":
    sort_icons()
