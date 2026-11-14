# LifeSaver

"Instantly switch between looking productive and being productive." - ChatGPT

A simple workspace manager for Windows in raw C.

It hides windows belonging to inactive workspaces and automatically manages application audio accordingly.

# Usage

- `alt` + `1` .. `9`: Switch to a specific workspace ranging from 1..9
- `alt` + `shift` + `1` .. `9`: Move the focused window to a specific workspace ranging from 1..9
- `alt` + `a`: Go to `n-1`th workspace. Rolls over to 9th workspace if you do this while on 1st workspace
- `alt` + `s`: Go to `n+1`th workspace. Rolls over to 1st workspace if you do this while on 9th workspace
- `alt` + `d`: Swap between the current workspace and the previous workspace
- `alt` + `shift` + `ctrl` + `q`: Quit LifeSaver, restoring all application windows

> [!CAUTION]
> Do not kill the process. Use the appropriate keybind instead to gracefully shut down LifeSaver and restore all application windows.


# Example

- You're playing Roblox on workspace 1 and you have a homework.pdf open in workspace 2.
- Your mom walks into the room.
- You immediately press `alt`+`s` (to switch to the next workspace) and act normal.
- Your mom sees homework.pdf

# Installation

Download the `LifeSaver-Setup.exe` from the repo's GitHub releases and run it.

Or you could use the `LifeSaver-Portable.exe` for a standalone binary.

# Building

Requires MinGW-w64 (`gcc`)

- `.\build.bat` -> Build and run the debug binary
- `.\build.bat debug` -> Build the debug binary
- `.\build.bat release` -> Build the release binary

# Disclaimer

LifeSaver may not work correctly with every application or window configuration. Use it at your own risk. The author is not responsible for any damages or losses resulting from its use.

# Contributions

PRs are appreciated.

# License

This repository is licensed under the MIT license. More info in the `LICENSE.txt`.
