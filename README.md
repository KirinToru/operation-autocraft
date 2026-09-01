# operation-autocraft

Operation Autocraft is an automation script for Fallen Doll: Operation Lovecraft, designed to automatically interact with game features like distributing likes, handling hearts, and managing excitement bars. It features an interactive, on-screen calibration wizard to match any display resolution without manual code edits.

## Features

- Auto-L (Hearts): Automatically handles heart interactions.
- Auto-J (Bars): Automatically clicks to manage excitement bars.
- Auto-Likes: Distributes likes to selected players automatically when the gauge reaches 10/10.
- Built-in Music: Plays the Polish anthem in the background with a hotkey to toggle.
- Interactive Calibration Wizard: Set up click coordinates directly in the game using your mouse.

## Installation

### Method 1: Ready-to-use Executable (Recommended)
You can download the pre-compiled version of the bot directly from GitHub without installing Python.
1. Go to the **Releases** section on the GitHub repository page.
2. Download the latest `operation-autocraft.exe`.
3. Right-click the `.exe` file and select **Run as Administrator** (required for background hotkeys to work).

### Method 2: From Source Code
Ensure you have Python installed. The script depends on `numpy`, `mss`, and `keyboard` libraries.

1. Clone or download the repository.
2. Install requirements:
   pip install mss keyboard numpy
3. Run `main.py` as Administrator:
   python main.py

## Hotkeys

The script is controlled entirely via background hotkeys:

- `Insert`: Main Pause/Start toggle for the entire script.
- `F4`: Pause/Resume the background music.
- `F5`: Toggle Auto-L (Hearts) feature.
- `F6`: Toggle Auto-J (Bars) feature.
- `F7`: Toggle Auto-Likes feature.
- `F8`: Diagnostics display (prints current status and gauge pixel readings in the console).
- `F9`: (Removed) Replaced by independent calibration hotkeys.

## Calibration Hotkeys

If the Auto-Likes feature does not click correctly or fails to detect the 10/10 gauge, use these hotkeys to calibrate. Hover your mouse over the specific element in the game and press the hotkey. You can do this in any order. The script will instantly save your settings into `settings.json`.

- `Shift + 1` : Set coordinates for Player 1
- `Shift + Q` : Set coordinates for Like Button of Player 1
- `Shift + 2` : Set coordinates for Player 2
- `Shift + W` : Set coordinates for Like Button of Player 2
- `Shift + 3` : Set coordinates for Player 3
- `Shift + E` : Set coordinates for Like Button of Player 3
- `Shift + 4` : Calibrate Gauge (Press this when the excitement gauge is exactly 10/10)

The script will save these exact screen coordinates and pixel counts. Calibration only needs to be done once per display resolution.

## Configuration

Settings are saved automatically in `settings.json` after running the calibration wizard. If `settings.json` is deleted, the script will prompt you to recalibrate or will not execute clicks until calibrated.

## License

This project is intended for personal use and automation testing.

## License

This project is closed-source (Proprietary). Detailed terms of use can be found in the [LICENSE](./LICENSE) file.
