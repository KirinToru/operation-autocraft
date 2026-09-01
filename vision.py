import mss
import numpy as np
import config
from logger import log_error

def is_prompt_light(img, x, y):
    try:
        rel_x = x - config.OFFSET_X
        rel_y = y - config.OFFSET_Y
        if rel_y < 0 or rel_y >= img.shape[0] or rel_x < 0 or rel_x >= img.shape[1]:
            return False
        b, g, r = int(img[rel_y, rel_x, 0]), int(img[rel_y, rel_x, 1]), int(img[rel_y, rel_x, 2])
        return (r >= 180 and g >= 180 and b >= 180 and 
                abs(r - g) <= 45 and abs(r - b) <= 45)
    except Exception as e:
        log_error("vision.py: is_prompt_light", e)
        return False

def area_has_heart_prompt(img):
    try:
        points = [
            (430, 778), (445, 772), (460, 772), (475, 778), 
            (414, 798), (476, 798), (414, 832), (476, 832), 
            (430, 852), (445, 858), (460, 858), (475, 852)
        ]
        hits = sum(1 for px, py in points if is_prompt_light(img, px, py))
        return hits >= 4
    except Exception as e:
        log_error("vision.py: area_has_heart_prompt", e)
        return False

def area_has_start_prompt(img):
    try:
        points = [(393, 915), (396, 915), (420, 887), (447, 915), (420, 943)]
        hits = sum(1 for px, py in points if is_prompt_light(img, px, py))
        return hits >= 4
    except Exception as e:
        log_error("vision.py: area_has_start_prompt", e)
        return False

def area_has_yellow(img, rel_x1, rel_y1, rel_x2, rel_y2):
    try:
        region = img[rel_y1:rel_y2+1, rel_x1:rel_x2+1]
        b = region[:, :, 0].astype(np.int16)
        g = region[:, :, 1].astype(np.int16)
        r = region[:, :, 2].astype(np.int16)
        
        mask = (r >= 170) & (g >= 135) & (b <= 115) & (r - b >= 85) & (g - b >= 65)
        return np.any(mask)
    except Exception as e:
        log_error("vision.py: area_has_yellow", e)
        return False

def fast_bar_state(img):
    try:
        region = img[160:170, 0:221]
        b = region[:, :, 0].astype(np.int16)
        g = region[:, :, 1].astype(np.int16)
        r = region[:, :, 2].astype(np.int16)
        
        pink_mask = (r >= 145) & (b >= 75) & (r - g >= 35) & (b - g >= 5)
        if np.any(pink_mask):
            return "pink"
            
        grey_mask = (r >= 55) & (r <= 180) & (np.abs(r - g) <= 18) & (np.abs(g - b) <= 18) & (np.abs(r - b) <= 18)
        if np.any(grey_mask):
            return "grey"
            
        return ""
    except Exception as e:
        log_error("vision.py: fast_bar_state", e)
        return ""

def area_has_bar_end_pink(img):
    try:
        region = img[160:170, 214:221]
        b = region[:, :, 0].astype(np.int16)
        g = region[:, :, 1].astype(np.int16)
        r = region[:, :, 2].astype(np.int16)
        
        pink_mask = (r >= 145) & (b >= 75) & (r - g >= 35) & (b - g >= 5)
        total_pixels = region.shape[0] * region.shape[1]
        hits = np.sum(pink_mask)
        
        return hits >= (0.25 * total_pixels)
    except Exception as e:
        log_error("vision.py: area_has_bar_end_pink", e)
        return False

def get_gauge_fullness():
    try:
        with mss.MSS() as sct:
            img = np.array(sct.grab(config.GAUGE_BBOX))
        
        b = img[:, :, 0].astype(np.int16)
        g = img[:, :, 1].astype(np.int16)
        r = img[:, :, 2].astype(np.int16)
        
        cyan_mask = (b > 150) & (g > 150) & (r < 150)
        cyan_pixels = np.sum(cyan_mask)
        return cyan_pixels
    except Exception as e:
        log_error("vision.py: get_gauge_fullness", e)
        return 0
