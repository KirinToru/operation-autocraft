import ctypes
import time
import mss
import numpy as np
import config
from logger import log_info, log_error

# Windows API
user32 = ctypes.windll.user32
MOUSEEVENTF_LEFTDOWN = 0x0002
MOUSEEVENTF_LEFTUP = 0x0004

def get_foreground_window_title():
    try:
        hwnd = user32.GetForegroundWindow()
        length = user32.GetWindowTextLengthW(hwnd)
        buf = ctypes.create_unicode_buffer(length + 1)
        user32.GetWindowTextW(hwnd, buf, length + 1)
        return buf.value
    except Exception as e:
        log_error("actions.py: get_foreground_window_title", e)
        return ""

def is_game_active():
    try:
        title = get_foreground_window_title()
        return 'FallenDoll' in title or 'Paralogue' in title
    except Exception as e:
        log_error("actions.py: is_game_active", e)
        return False

def click(x, y):
    try:
        user32.SetCursorPos(x, y)
        user32.mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0)
        time.sleep(0.05)
        user32.mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0)
        time.sleep(0.05)
    except Exception as e:
        log_error("actions.py: click", e)

def get_mouse_pos():
    try:
        class POINT(ctypes.Structure):
            _fields_ = [("x", ctypes.c_long), ("y", ctypes.c_long)]
        pt = POINT()
        user32.GetCursorPos(ctypes.byref(pt))
        return pt.x, pt.y
    except Exception as e:
        log_error("actions.py: get_mouse_pos", e)
        return 0, 0

def distribute_likes():
    try:
        log_info("actions", "Points gauge full! Starting to distribute likes...")
        orig_x, orig_y = get_mouse_pos()
        
        for i in range(3):
            # If coordinates are (0,0), it means not calibrated
            if config.PLAYER_POSITIONS[i] == (0, 0) or config.LIKE_POSITIONS[i] == (0, 0):
                log_info("actions", f"Player {i+1} not calibrated, skipping.")
                continue
                
            px, py = config.PLAYER_POSITIONS[i]
            like_btn_x, like_btn_y = config.LIKE_POSITIONS[i]
            
            click(px, py)
            time.sleep(0.3)
            
            with mss.MSS() as sct:
                btn_bbox = {"left": like_btn_x - 5, "top": like_btn_y - 5, "width": 10, "height": 10}
                btn_img = np.array(sct.grab(btn_bbox))
                
            b = np.mean(btn_img[:, :, 0])
            g = np.mean(btn_img[:, :, 1])
            r = np.mean(btn_img[:, :, 2])
            
            is_blue = (b > r + 30) and (g > r + 20)
            
            if is_blue:
                log_info("actions", f"Distributing points to player nr {i+1}")
                click(like_btn_x, like_btn_y)
                time.sleep(0.3)
                break
            else:
                log_info("actions", f"Player nr {i+1} cannot receive likes, looking further.")
                
        user32.SetCursorPos(orig_x, orig_y)
        log_info("actions", "Finished distributing likes.")
    except Exception as e:
        log_error("actions.py: distribute_likes", e)
