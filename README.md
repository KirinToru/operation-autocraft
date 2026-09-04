<div align="center">
  <img src="assets/icon/icon3000x3000.png" width="150" alt="Operation Autocraft Logo">
  <br><br>
  
  <img src="https://img.shields.io/badge/C++-00599C?style=for-the-badge&logo=c%2B%2B&logoColor=white" alt="C++">
  <img src="https://img.shields.io/badge/Dear_ImGui-0F0F0F?style=for-the-badge" alt="ImGui">
  <img src="https://img.shields.io/badge/DirectX_11-107C10?style=for-the-badge&logo=windows&logoColor=white" alt="DirectX 11">
  <img src="https://img.shields.io/badge/Desktop_Duplication_API-0078D7?style=for-the-badge&logo=windows&logoColor=white" alt="Desktop Duplication API">
  <img src="https://img.shields.io/badge/nlohmann_json-F3E5AB?style=for-the-badge&logo=json&logoColor=black" alt="nlohmann/json">
  <img src="https://img.shields.io/badge/CMake-064F8C?style=for-the-badge&logo=cmake&logoColor=white" alt="CMake">
</div>

# Operation Autocraft

Operation Autocraft is a sophisticated automation overlay for Fallen Doll: Operation Lovecraft, designed to automatically interact with game features like distributing likes, handling hearts, and managing stamina bars.

This tool has been completely rewritten in **C++** using **DirectX 11** and **Dear ImGui**, providing a seamless, transparent, and ultra-fast in-game overlay without requiring the user to alt-tab or use separate command-line windows.

## Features

- **DirectX 11 ImGui Overlay**: Completely transparent, click-through overlay that seamlessly integrates with the game.
- **Dynamic Click-Through**: The overlay intelligently passes mouse clicks to the game while allowing you to interact with its own floating menus.
- **Auto-L (Hearts)**: Automatically presses the heart button when both male and female indicators turn yellow.
- **Auto-J (Bars)**: Automatically manages excitement bars.
- **Auto-Likes**: Intelligent liking system with two modes (Immediate and Accumulative) that scans the 10-segment gauge.
- **Interactive Calibration Wizard**: Set up click coordinates and color-detection zones directly on your screen.
- **Full Keybinding Customization**: Change any hotkey (keyboard and mouse buttons) directly from the GUI.
- **Bilingual Interface**: Support for English and Russian (RU) languages.

## Installation & Running

1. Download the latest compiled `Operation Autocraft.exe` from the releases page (or build it from source using CMake and Visual Studio).
2. Run the `.exe` file.
3. The transparent overlay will appear on your screen. 
4. Press **Insert** to toggle the menu visibility.

## Configuration & Calibration

Settings are saved automatically in `settings.json` after running the calibration wizard. 
To configure the bot for your specific resolution:
1. Open the **Settings** menu.
2. Click **START CALIBRATION**.
3. Follow the on-screen instructions, pressing the requested keys while hovering your mouse over the specific in-game UI elements.
4. Press **ESC** to exit calibration.

## License

This project is closed-source (Proprietary). Detailed terms of use can be found in the [LICENSE](./LICENSE) file.
