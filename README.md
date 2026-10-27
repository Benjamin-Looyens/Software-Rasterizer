# Software Rasterizer
Software rasterizer implemented with SDL2 for the course "Graphics Programming" in Howest DAE Game Development. The project implements the standard rasterizer pipeline (`projection`, `rasterization`, `shading` stages). The project runs entirely on the local CPU.

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

## Example
<img width="642" height="514" alt="Rasterizer" src="https://github.com/user-attachments/assets/e9410b95-cc66-44d4-8886-796a3d176ca6"/>
