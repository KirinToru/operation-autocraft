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
from lang import STRINGS

def toggle_automation():
    try:
        config.ENABLED = not config.ENABLED
        config.last_heart_press_at = 0
        config.last_j_press_at = 0
        config.last_start_press_at = 0
        
        lang = STRINGS[config.LANGUAGE]
        status = lang["status_on"] if config.ENABLED else lang["status_pause"]
        log_info("main", lang["auto_state"].format(status))
    except Exception as e:
        log_error("main.py: toggle_automation", e)

def show_diagnostics():
    try:
        lang = STRINGS[config.LANGUAGE]
        print("\n" + "="*40)
        print(lang["diagnostic_title"])
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
            
            print(lang["upper_yellow"].format(lang["yes"] if upper_yellow else lang["no"]))
            print(lang["lower_yellow"].format(lang["yes"] if lower_yellow else lang["no"]))
            print(lang["male_bar"].format(male_state.upper()))
            print(lang["male_bar_end"].format(lang["yes"] if end_pink else lang["no"]))
            print(lang["heart_vis"].format(lang["yes"] if heart_visible else lang["no"]))
            print(lang["start_vis"].format(lang["yes"] if start_visible else lang["no"]))
            print(lang["gauge_px"].format(gauge_pixels, config.GAUGE_THRESHOLD))
            print("-" * 40)
            print(lang["feat_status"])
            print(lang["lang_status"].format(config.LANGUAGE))
            print(lang["players_status"].format(config.NUM_PLAYERS))
            print(lang["like_mode"].format(config.LIKE_MODE))
            print(lang["auto_l"].format(lang["on"] if config.AUTO_L_ENABLED else lang["off"]))
            print(lang["auto_j"].format(lang["on"] if config.AUTO_J_ENABLED else lang["off"]))
            print(lang["auto_likes"].format(lang["on"] if config.AUTO_LIKE_ENABLED else lang["off"]))
            print("="*40 + "\n")
    except Exception as e:
        log_error("main.py: show_diagnostics", e)

def calibrate_p(index):
    mx, my = actions.get_mouse_pos()
    config.PLAYER_POSITIONS[index] = (mx, my)
    lang = STRINGS[config.LANGUAGE]
    print(lang["saved_player"].format(index + 1, mx, my))
    config.save_settings()

def calibrate_l(index):
    mx, my = actions.get_mouse_pos()
    config.LIKE_POSITIONS[index] = (mx, my)
    lang = STRINGS[config.LANGUAGE]
    print(lang["saved_like"].format(index + 1, mx, my))
    config.save_settings()

def toggle_auto_l():
    config.AUTO_L_ENABLED = not config.AUTO_L_ENABLED
    lang = STRINGS[config.LANGUAGE]
    log_info("main", lang["auto_l_log"].format(lang["on"] if config.AUTO_L_ENABLED else lang["off"]))

def toggle_auto_j():
    config.AUTO_J_ENABLED = not config.AUTO_J_ENABLED
    lang = STRINGS[config.LANGUAGE]
    log_info("main", lang["auto_j_log"].format(lang["on"] if config.AUTO_J_ENABLED else lang["off"]))

def toggle_auto_likes():
    config.AUTO_LIKE_ENABLED = not config.AUTO_LIKE_ENABLED
    lang = STRINGS[config.LANGUAGE]
    log_info("main", lang["auto_like_log"].format(lang["on"] if config.AUTO_LIKE_ENABLED else lang["off"]))

def toggle_language():
    config.LANGUAGE = "EN" if config.LANGUAGE == "RUS" else "RUS"
    config.save_settings()
    lang = STRINGS[config.LANGUAGE]
    log_info("main", lang["lang_log"].format(config.LANGUAGE))
    print_menu()

def toggle_players():
    config.NUM_PLAYERS = (config.NUM_PLAYERS % 4) + 1
    config.save_settings()
    lang = STRINGS[config.LANGUAGE]
    log_info("main", lang["players_log"].format(config.NUM_PLAYERS))

def toggle_like_mode():
    config.LIKE_MODE = "IMMEDIATE" if config.LIKE_MODE == "ACCUMULATED" else "ACCUMULATED"
    config.save_settings()
    lang = STRINGS[config.LANGUAGE]
    log_info("main", lang["mode_log"].format(config.LIKE_MODE))

keyboard.add_hotkey('insert', toggle_automation)
keyboard.add_hotkey('f4', toggle_language)
keyboard.add_hotkey('f5', toggle_auto_l)
keyboard.add_hotkey('f6', toggle_auto_j)
keyboard.add_hotkey('f7', toggle_auto_likes)
keyboard.add_hotkey('f8', show_diagnostics)
keyboard.add_hotkey('f9', toggle_players)
keyboard.add_hotkey('f10', toggle_like_mode)

keyboard.add_hotkey('shift+1', lambda: calibrate_p(0))
keyboard.add_hotkey('shift+q', lambda: calibrate_l(0))
keyboard.add_hotkey('shift+2', lambda: calibrate_p(1))
keyboard.add_hotkey('shift+w', lambda: calibrate_l(1))
keyboard.add_hotkey('shift+3', lambda: calibrate_p(2))
keyboard.add_hotkey('shift+e', lambda: calibrate_l(2))
keyboard.add_hotkey('shift+4', lambda: calibrate_p(3))
keyboard.add_hotkey('shift+r', lambda: calibrate_l(3))

def get_resource_path(relative_path):
    try:
        base_path = sys._MEIPASS
    except Exception:
        base_path = os.path.abspath(".")
    return os.path.join(base_path, relative_path)

def print_menu():
    os.system('cls' if os.name == 'nt' else 'clear')
    lang = STRINGS[config.LANGUAGE]
    log_info("main", lang["started"])
    print(lang["hotkeys_title"])
    print("  " + lang["hk_toggle"])
    print("  " + lang["hk_lang"])
    print("  " + lang["hk_auto_l"])
    print("  " + lang["hk_auto_j"])
    print("  " + lang["hk_auto_likes"])
    print("  " + lang["hk_diag"])
    print("  " + lang["hk_players"])
    print("  " + lang["hk_mode"])
    print(lang["calib_title"])
    print("  " + lang["calib_1"])
    print("  " + lang["calib_2"])
    print("  " + lang["calib_3"])
    print("  " + lang["calib_4"])
    print("\n" + lang["hk_esc"])
    print(lang["waiting"])

def main_loop():
    try:
        config.load_settings()
        print_menu()

        with mss.MSS() as sct:
            while True:
                if keyboard.is_pressed('esc'):
                    lang = STRINGS[config.LANGUAGE]
                    log_info("main", lang["closing"])
                    break
                    
                if not config.ENABLED or not actions.is_game_active():
                    time.sleep(0.01)
                    continue

                now = time.time()
                
                # AUTO_LIKE LOGIC
                if config.AUTO_LIKE_ENABLED:
                    if config.LIKE_MODE == "ACCUMULATED":
                        if now - config.last_gauge_check_at >= config.GAUGE_CHECK_INTERVAL:
                            config.last_gauge_check_at = now
                            cyan_pixels = vision.get_gauge_fullness()
                            
                            # Start distributing when threshold is reached
                            if not config.is_distributing and cyan_pixels > config.GAUGE_THRESHOLD:
                                config.is_distributing = True
                                
                            # Keep distributing until gauge is completely empty
                            if config.is_distributing:
                                if cyan_pixels > 50:
                                    actions.distribute_likes()
                                    time.sleep(1)
                                    now = time.time()
                                else:
                                    # Gauge is empty, stop distributing and start accumulating again
                                    config.is_distributing = False
                    else:
                        # IMMEDIATE MODE
                        if now - config.last_gauge_check_at >= config.LIKE_CHECK_INTERVAL:
                            config.last_gauge_check_at = now
                            cyan_pixels = vision.get_gauge_fullness()
                            # Distribute only if there is at least 1 like in the gauge (e.g. > 50 pixels)
                            if cyan_pixels > 50:
                                actions.distribute_likes()
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

