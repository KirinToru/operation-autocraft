import time
import keyboard
import mss
import numpy as np
import sys
import os
import ctypes

import config
import vision
import actions
from logger import log_info, log_error

def toggle_automation():
    try:
        config.ENABLED = not config.ENABLED
        config.last_heart_press_at = 0
        config.last_j_press_at = 0
        config.last_start_press_at = 0
        status = "ON" if config.ENABLED else "PAUSE"
        log_info("main", f"Fallen Doll auto: {status}")
    except Exception as e:
        log_error("main.py: toggle_automation", e)

def show_diagnostics():
    try:
        print("\n" + "="*40)
        print("Fallen Doll diagnostic (F8)")
        print("="*40)
        with mss.MSS() as sct:
            img = np.array(sct.grab(config.BBOX))
            
            upper_yellow = vision.area_has_yellow(img, 303 - config.OFFSET_X, 804 - config.OFFSET_Y, 317 - config.OFFSET_X, 810 - config.OFFSET_Y)
            lower_yellow = vision.area_has_yellow(img, 303 - config.OFFSET_X, 904 - config.OFFSET_Y, 317 - config.OFFSET_X, 913 - config.OFFSET_Y)
            
            male_state = vision.fast_bar_state(img)
            if male_state == "":
                male_state = "UNKNOWN"
                
            end_pink = vision.area_has_bar_end_pink(img)
            heart_visible = vision.area_has_heart_prompt(img)
            start_visible = vision.area_has_start_prompt(img)
            
            gauge_pixels = vision.get_gauge_fullness()
            
            print(f"Upper yellow:        {'YES' if upper_yellow else 'NO'}")
            print(f"Lower yellow:        {'YES' if lower_yellow else 'NO'}")
            print(f"Male bar:            {male_state.upper()}")
            print(f"Male bar END pink:   {'YES' if end_pink else 'NO'}")
            print(f"HEART visible:       {'YES' if heart_visible else 'NO'}")
            print(f"Play/Repeat visible: {'YES' if start_visible else 'NO'}")
            print(f"Gauge cyan pixels:   {gauge_pixels} (Threshold is {config.GAUGE_THRESHOLD})")
            print("-" * 40)
            print("=== FEATURES STATUS ===")
            print(f"Music (F4):      {'ON' if config.MUSIC_ENABLED else 'OFF'}")
            print(f"Auto L (F5):     {'ON' if config.AUTO_L_ENABLED else 'OFF'}")
            print(f"Auto J (F6):     {'ON' if config.AUTO_J_ENABLED else 'OFF'}")
            print(f"Auto Likes (F7): {'ON' if config.AUTO_LIKE_ENABLED else 'OFF'}")
            print("="*40 + "\n")
    except Exception as e:
        log_error("main.py: show_diagnostics", e)

calibration_state = 0

def run_calibration():
    global calibration_state
    try:
        mx, my = actions.get_mouse_pos()
        if calibration_state == 0:
            print("\n" + "="*40)
            print("CALIBRATION MODE STARTED!")
            print("Step 1: Hover over PLAYER 1 on the list and press F9.")
            print("="*40 + "\n")
            calibration_state = 1
        elif calibration_state == 1:
            config.PLAYER_POSITIONS[0] = (mx, my)
            print("Saved Player 1. Now hover over LIKE BUTTON for Player 1 and press F9.")
            calibration_state = 2
        elif calibration_state == 2:
            config.LIKE_POSITIONS[0] = (mx, my)
            print("Saved Like 1. Now hover over PLAYER 2 and press F9.")
            calibration_state = 3
        elif calibration_state == 3:
            config.PLAYER_POSITIONS[1] = (mx, my)
            print("Saved Player 2. Now hover over LIKE BUTTON for Player 2 and press F9.")
            calibration_state = 4
        elif calibration_state == 4:
            config.LIKE_POSITIONS[1] = (mx, my)
            print("Saved Like 2. Now hover over PLAYER 3 and press F9.")
            calibration_state = 5
        elif calibration_state == 5:
            config.PLAYER_POSITIONS[2] = (mx, my)
            print("Saved Player 3. Now hover over LIKE BUTTON for Player 3 and press F9.")
            calibration_state = 6
        elif calibration_state == 6:
            config.LIKE_POSITIONS[2] = (mx, my)
            print("Saved Like 3. FINAL STEP: Ensure gauge is 10/10 and press F9.")
            calibration_state = 7
        elif calibration_state == 7:
            cyan_px = vision.get_gauge_fullness()
            config.GAUGE_THRESHOLD = max(10, cyan_px - 200)
            print(f"Gauge set to {config.GAUGE_THRESHOLD} (Read: {cyan_px}). Calibration complete!")
            config.save_settings()
            calibration_state = 0
            print("Settings saved to settings.json.")
    except Exception as e:
        log_error("main.py: run_calibration", e)

def toggle_auto_l():
    config.AUTO_L_ENABLED = not config.AUTO_L_ENABLED
    log_info("main", f"Auto L is now {'ON' if config.AUTO_L_ENABLED else 'OFF'}")

def toggle_auto_j():
    config.AUTO_J_ENABLED = not config.AUTO_J_ENABLED
    log_info("main", f"Auto J is now {'ON' if config.AUTO_J_ENABLED else 'OFF'}")

def toggle_auto_likes():
    config.AUTO_LIKE_ENABLED = not config.AUTO_LIKE_ENABLED
    log_info("main", f"Auto Likes is now {'ON' if config.AUTO_LIKE_ENABLED else 'OFF'}")

def toggle_music():
    config.MUSIC_ENABLED = not config.MUSIC_ENABLED
    if config.MUSIC_ENABLED:
        ctypes.windll.winmm.mciSendStringW("play anthem", None, 0, 0)
    else:
        ctypes.windll.winmm.mciSendStringW("pause anthem", None, 0, 0)
    log_info("main", f"Music is now {'ON' if config.MUSIC_ENABLED else 'OFF'}")

# Hotkey registration
keyboard.add_hotkey('insert', toggle_automation)
keyboard.add_hotkey('f4', toggle_music)
keyboard.add_hotkey('f5', toggle_auto_l)
keyboard.add_hotkey('f6', toggle_auto_j)
keyboard.add_hotkey('f7', toggle_auto_likes)
keyboard.add_hotkey('f8', show_diagnostics)
keyboard.add_hotkey('f9', run_calibration)

def get_resource_path(relative_path):
    """ Get absolute path to resource, works for dev and for PyInstaller """
    try:
        # PyInstaller creates a temp folder and stores path in _MEIPASS
        base_path = sys._MEIPASS
    except Exception:
        base_path = os.path.abspath(".")
    return os.path.join(base_path, relative_path)

def main_loop():
    try:
        config.load_settings()
        
        os.system('cls' if os.name == 'nt' else 'clear')
        print(config.GODLO_POLSKI)
        print("GLORY TO GREAT POLAND! PLAYING ANTHEM (20% VOLUME)...")
        
        mp3_path = get_resource_path("Hymn RP 1 zwrotka.mp3")
        if os.path.exists(mp3_path):
            mci = ctypes.windll.winmm.mciSendStringW
            mci(f'open "{mp3_path}" type mpegvideo alias anthem', None, 0, 0)
            mci("setaudio anthem volume to 200", None, 0, 0)
            mci("play anthem from 0", None, 0, 0)
        else:
            log_error("main", Exception("Anthem file not found!"))
        
        log_info("main", "Script started successfully!")
        print("Press 'Insert' to toggle/pause main automation.")
        print("Press 'F4' to toggle Music.")
        print("Press 'F5' to toggle Auto L (Hearts/Prompts).")
        print("Press 'F6' to toggle Auto J (Gauge State/Repeat).")
        print("Press 'F7' to toggle Auto Likes (Distributing Points).")
        print("Press 'F8' to show diagnostics.")
        print("Press 'F9' for CALIBRATION.")
        print("Press 'Esc' to exit.")
        print("Waiting...\n")

        with mss.MSS() as sct:
            while True:
                if keyboard.is_pressed('esc'):
                    log_info("main", "Closing script...")
                    break
                    
                if not config.ENABLED or not actions.is_game_active():
                    time.sleep(0.01)
                    continue

                now = time.time()
                
                # AUTO_LIKE LOGIC
                if config.AUTO_LIKE_ENABLED:
                    if now - config.last_gauge_check_at >= config.GAUGE_CHECK_INTERVAL:
                        config.last_gauge_check_at = now
                        cyan_pixels = vision.get_gauge_fullness()
                        if cyan_pixels > config.GAUGE_THRESHOLD: 
                            actions.distribute_likes()
                            time.sleep(1)
                            now = time.time()

                img = np.array(sct.grab(config.BBOX))
                
                # AUTO_L LOGIC
                if config.AUTO_L_ENABLED:
                    if vision.area_has_heart_prompt(img):
                        if config.last_heart_press_at == 0 or (now - config.last_heart_press_at >= config.HEART_PRESS_INTERVAL):
                            keyboard.send('l')
                            config.last_heart_press_at = now
                        continue
                        
                    upper_yellow = vision.area_has_yellow(img, 169, 54, 183, 60)
                    lower_yellow = vision.area_has_yellow(img, 169, 154, 183, 163)
                    
                    if upper_yellow and lower_yellow:
                        if config.last_heart_press_at == 0 or (now - config.last_heart_press_at >= config.HEART_PRESS_INTERVAL):
                            keyboard.send('l')
                            config.last_heart_press_at = now
                        continue

                config.last_heart_press_at = 0
                male_state = vision.fast_bar_state(img)

                # AUTO_J LOGIC
                if config.AUTO_J_ENABLED:
                    if male_state == "pink" and vision.area_has_bar_end_pink(img):
                        if config.last_j_press_at == 0 or (now - config.last_j_press_at >= config.FULL_BAR_J_INTERVAL):
                            keyboard.send('j')
                            config.last_j_press_at = now
                        continue

                    if male_state == "grey":
                        if config.last_j_press_at == 0 or (now - config.last_j_press_at >= config.EMPTY_BAR_J_INTERVAL):
                            keyboard.send('j')
                            config.last_j_press_at = now
                        
                        if vision.area_has_start_prompt(img):
                            if config.last_start_press_at == 0 or (now - config.last_start_press_at >= config.START_PRESS_INTERVAL):
                                keyboard.send('j')
                                config.last_start_press_at = now
                        
                        continue

                    if male_state == "pink":
                        config.last_j_press_at = 0
                        continue
                    
    except Exception as e:
        log_error("main.py: main_loop", e, "Main loop was interrupted.")
        sys.exit(1)

if __name__ == '__main__':
    main_loop()
