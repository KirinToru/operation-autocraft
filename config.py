import json
import os
from logger import log_error

# ============================================================
# CONFIGURATION
# ============================================================

# Bot state
ENABLED = False

# Optional Features Settings
LANGUAGE = "RUS"
NUM_PLAYERS = 3
LIKE_MODE = "ACCUMULATED"  # "IMMEDIATE" or "ACCUMULATED"
AUTO_L_ENABLED = True
AUTO_J_ENABLED = True
AUTO_LIKE_ENABLED = True

# Accumulated points threshold (when LIKE_MODE is "ACCUMULATED")
GAUGE_THRESHOLD = 950
is_distributing = False

# Saved positions for likes distribution (up to 4 players)
PLAYER_POSITIONS = [(0,0), (0,0), (0,0), (0,0)]
LIKE_POSITIONS = [(0,0), (0,0), (0,0), (0,0)]

def load_settings():
    global GAUGE_THRESHOLD, PLAYER_POSITIONS, LIKE_POSITIONS, LANGUAGE, NUM_PLAYERS, LIKE_MODE
    try:
        if os.path.exists('settings.json'):
            with open('settings.json', 'r') as f:
                data = json.load(f)
                if "language" in data:
                    LANGUAGE = data["language"]
                if "num_players" in data:
                    NUM_PLAYERS = data["num_players"]
                if "like_mode" in data:
                    LIKE_MODE = data["like_mode"]
                if "gauge_threshold" in data:
                    GAUGE_THRESHOLD = data["gauge_threshold"]
                
                if "player_positions" in data:
                    positions = data["player_positions"]
                    for i in range(min(4, len(positions))):
                        PLAYER_POSITIONS[i] = tuple(positions[i])
                        
                if "like_positions" in data:
                    positions = data["like_positions"]
                    for i in range(min(4, len(positions))):
                        LIKE_POSITIONS[i] = tuple(positions[i])
    except Exception as e:
        log_error("config.py: load_settings", e)

def save_settings():
    try:
        data = {
            "language": LANGUAGE,
            "num_players": NUM_PLAYERS,
            "like_mode": LIKE_MODE,
            "gauge_threshold": GAUGE_THRESHOLD,
            "player_positions": PLAYER_POSITIONS,
            "like_positions": LIKE_POSITIONS
        }
        with open('settings.json', 'w') as f:
            json.dump(data, f, indent=4)
    except Exception as e:
        log_error("config.py: save_settings", e)

# Time limits (throttling)
last_heart_press_at = 0
last_j_press_at = 0
last_start_press_at = 0
last_gauge_check_at = 0

HEART_PRESS_INTERVAL = 0.001
EMPTY_BAR_J_INTERVAL = 0.001
FULL_BAR_J_INTERVAL = 0.001
START_PRESS_INTERVAL = 0.090
GAUGE_CHECK_INTERVAL = 1.0 # Check every second
LIKE_CHECK_INTERVAL = 0.5  # Check more frequently for IMMEDIATE mode

# Scan region (bounding box combining all points)
BBOX = {"left": 134, "top": 750, "width": 366, "height": 196}
OFFSET_X = BBOX["left"]
OFFSET_Y = BBOX["top"]

# Coordinates for reading gauge fullness
GAUGE_BBOX = {"left": 1780, "top": 15, "width": 100, "height": 100}
