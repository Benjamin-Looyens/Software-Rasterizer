# Software Rasterizer
Software rasterizer implemented with SDL2 for the course "Graphics Programming" in Howest DAE Game Development. The project implements the standard rasterizer pipeline (`projection`, `rasterization`, `shading` stages).

## Features
- Perspective camera:
    - WASD for 360 movement
    - RMB for camera shifting
    - LMB for diving (forward + camera)
    - RMB + LMB for camera panning
    - R for setting position to starting one
- Rendering
    - B toggles tri bounding boxes render
    - F toggles mesh wireframe render
    - F4 toggles depth buffer render
- Mesh control
    - F5 toggles mesh rotation
    - F6 toggles mesh normal map usage
    - F7 cycles mesh lighting modes