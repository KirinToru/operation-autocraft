# FD-autoL

FD-autoL is an automation script for Fallen Doll: Operation Lovecraft, designed to automatically interact with game features like distributing likes, handling hearts, and managing excitement bars. It features an interactive, on-screen calibration wizard to match any display resolution without manual code edits.

## Features

- Auto-L (Hearts): Automatically handles heart interactions.
- Auto-J (Bars): Automatically clicks to manage excitement bars.
- Auto-Likes: Distributes likes to selected players automatically when the gauge reaches 10/10.
- Built-in Music: Plays the Polish anthem in the background with a hotkey to toggle.
- Interactive Calibration Wizard: Set up click coordinates directly in the game using your mouse.

## Installation

Ensure you have Python installed. The script depends on `numpy`, `mss`, and `keyboard` libraries.
If you prefer a pre-compiled version, use the executable generated in the `dist` folder.

1. Clone or download the repository.
2. Install requirements (if running from source):
   pip install mss keyboard numpy
3. Run `main.py` as Administrator (required for `keyboard` hooks to work in background).
   python main.py

## Hotkeys

The script is controlled entirely via background hotkeys:

- `Insert`: Main Pause/Start toggle for the entire script.
- `F4`: Pause/Resume the background music.
- `F5`: Toggle Auto-L (Hearts) feature.
- `F6`: Toggle Auto-J (Bars) feature.
- `F7`: Toggle Auto-Likes feature.
- `F8`: Diagnostics display (prints current status and gauge pixel readings in the console).
- `F9`: Calibration Wizard.

## Calibration Wizard (F9)

If the Auto-Likes feature does not click on players or the like button correctly, or fails to detect the 10/10 gauge, run the Calibration Wizard. The script will save your settings into `settings.json`.

1. Launch the game and start the script.
2. Press `F9` to begin calibration mode. Look at the script's console window for instructions.
3. Hover your mouse over the 1st player on the list and press `F9`.
4. Hover your mouse over the like button for the 1st player and press `F9`.
5. Repeat steps 3 and 4 for the 2nd and 3rd players.
6. Wait until the game's excitement gauge hits 10/10 (fully blue), then press `F9` one last time.

The script will save these exact screen coordinates and pixel counts. Calibration only needs to be done once per display resolution.

## Configuration

Settings are saved automatically in `settings.json` after running the calibration wizard. If `settings.json` is deleted, the script will prompt you to recalibrate or will not execute clicks until calibrated.

## License

This project is intended for personal use and automation testing.
