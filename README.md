# DOOM3D

A Doom-inspired 3D first-person shooter built from scratch in C using SDL2 and
OpenGL 2.1 (fixed-function pipeline). The game is a single-level dungeon crawler
where the player must eliminate all enemies while collecting pickups scattered
around the map.

## Feladat specifikáció (Project specification)

The game implements the required features for the Computer Graphics
(Számítógépi grafika) course assignment:

### Minimum requirements

| Requirement | Implementation |
|---|---|
| Camera control (mouse/keyboard) | First-person mouse look (yaw + pitch) and WASD + strafe movement |
| 3D objects loaded from files | OBJ loader reads `cube.obj`, `sphere.obj`, `barrel.obj` from `assets/` |
| Animation / movement | Enemy bobbing, pickup rotation, barrel fire particles, muzzle flash, enemy chase AI |
| Textures | All models and level surfaces are textured (procedurally generated at runtime) |
| Adjustable lights (+/-) | Scene light brightness controlled by `+` / `-`; movable with arrow keys; color cycled with `L`; flashlight toggled with `F` |
| User guide (F1) | Pressing `F1` displays a full in-game help overlay |

### Extra features

| Extra feature | Implementation |
|---|---|
| Fog | `GL_EXP2` exponential-squared fog, density adjustable with `[` / `]`, toggle with `G` |
| Particle system | Billboard particles for barrel fire/smoke (additive blend) and blood splatter (alpha blend with gravity) |
| Transparency | Semi-transparent glass walls (`W` tiles) rendered with RGBA textures in a separate pass |
| Shadows | Hybrid system: projected geometry shadows (rank-3 projection matrix) for boxes, soft oval textures for rounded objects, stencil-buffered to avoid double-blend |
| AI | 5-state finite state machine (IDLE → PATROL → CHASE → SEARCH → ATTACK) with line-of-sight raycasting and inter-enemy alerting |
| Collision detection | Axis-aligned bounding box collision for the player and enemies vs. the grid map, with axis-separated movement for wall sliding |
| Procedural geometry and textures | All runtime textures (walls, floor, ceiling, enemies, pickups, window glass, shadow blob) are generated algorithmically in C |
| Stencil buffer usage | Used during shadow rendering to prevent double-alpha blend where shadows overlap |

## Controls

**Movement**
- `W / A / S / D` - Move forward, left, back, right
- `Mouse` - Look around
- `Space` - Jump
- `Left Click` - Shoot

**Weapons**
- `1` - Fist (melee, infinite ammo)
- `2` - Pistol
- `3` - Shotgun

**Lighting**
- `+` / `-` - Increase / decrease brightness
- `L` - Cycle light color (white / red / blue)
- `F` - Toggle flashlight
- `Arrow keys` - Move the scene light

**Fog**
- `G` - Toggle fog on/off
- `[` / `]` - Decrease / increase fog density

**Other**
- `F1` - Toggle help screen
- `R` - Restart (when dead or won)
- `ESC` - Quit

## Pickups

- **Green cube** - Health (+25, caps at 100)
- **Yellow cube** - Ammo (+15 pistol, +4 shotgun)
- **Blue cube** - Armor (+25, caps at 200, absorbs 50% of incoming damage)

## Project structure

```
.
├── README.md          Project description & controls
├── Makefile           Build rules (Windows MinGW and Linux)
├── .gitignore
├── include/           Header files
│   ├── common.h       Constants, SDL/GL includes
│   ├── math3d.h       Vec3 and helpers
│   ├── obj_loader.h
│   ├── level.h        Map grid, collision, LOS
│   ├── textures.h
│   ├── particles.h
│   ├── enemy.h        AI state machine
│   ├── decoration.h   Barrels and crates
│   ├── pickup.h
│   ├── player.h
│   ├── lighting.h     Lights + fog state
│   ├── rendering.h    Camera + level geometry
│   ├── shadows.h
│   ├── font.h         8x8 bitmap font
│   └── hud.h          Doom-style status bar
├── src/               Implementation files
│   └── ...            (one .c per header)
└── assets/            Model files (not code)
    ├── cube.obj
    ├── sphere.obj
    └── barrel.obj
```

## Build

### Linux
Requires SDL2 development headers:
```bash
sudo apt install libsdl2-dev
make
./doom3d
```

### Windows (MSYS2 / MinGW64)
Requires SDL2, OpenGL, and GLU libraries. In the MSYS2 MinGW64 terminal:
```bash
pacman -S mingw-w64-x86_64-SDL2
make
./doom3d.exe
```

The `assets/` folder must be in the same directory as the executable when it is run.
