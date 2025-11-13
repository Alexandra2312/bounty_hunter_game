# Bounty Hunter Game 2D

A small OpenGL cowboy duel built with GLFW, GLEW and custom shaders. Ride your horse, dodge bullets, and take down the rival gunslinger outside the desert saloon.

## Features

- Hand-crafted 2D scene with animated tumbleweed, day/night toggle, and full saloon facade.
- Two armed cowboys: the player and an AI opponent that patrols, shoots, and displays muzzle flashes.
- Health UI for both characters (player hearts vs. enemy bar) and ammo tracker with warnings.
- Bullet collisions, jump arc, and animated fall when the enemy is defeated.
- Victory and game-over overlays plus a pause toggle and reload interaction near the saloon.

## Controls

- `W`/`A`/`S`/`D` or arrow keys: move.
- `Space`: jump.
- Left mouse: shoot (consumes ammo).
- `R` near the saloon: reload to 8 bullets.
- `P`: pause/unpause (shows “PAUSED” overlay).
- Click sun/moon in sky: toggle day/night.

## Building

- Open `ACG_Lab2.sln` in Visual Studio.
- Ensure the `Debug` directory contains `freeglut.dll` and `glew32.dll`.
- Build the `ACG_Lab2` project (x86, Debug recommended).

## Running

- Launch the built `ACG_Lab2.exe` from `Debug`.
- Keep shader files (`SimpleVertexShader.vertexshader`, `SimpleFragmentShader.fragmentshader`) alongside the executable.

## Repository Layout
main.cpp // Game loop & rendering
shader.[ch]pp // Shader helpers
SimpleVertexShader.vertexshader
SimpleFragmentShader.fragmentshader
dependente/ // GLFW, GLEW, GLUT, GLM dependencies
Debug/ // Built binary and required DLLs


