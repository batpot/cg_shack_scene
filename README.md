# shack scene
This is a project done for the course of Computer graphics, 3rd year of Computer science, Faculty of Mathematics, Belgrade. It's implemented in OpenGL/C++ using GLFW library. Used `https://github.com/matf-racunarska-grafika/project_base`

![](./video.mp4)

# instructions
1. `git clone https://github.com/matf-racunarska-grafika/project_base.git`
2. CLion -> Open -> path/to/my/project_base
3. main is in src/main.cpp
4. ALT+SHIFT+F10 -> project_base -> run
5. use WASD for camera movements
6. use mouse (and scroll) for moving and zooming
7. press B to activate/deactivate bloom
8. press esc to exit the project window

# assets
[Shack scene by nowelbesi](https://www.turbosquid.com/3d-models/3d-model-shack-scene/1060364)  
[Dead tree by FATIH ORHAN](https://www.turbosquid.com/3d-models/dead-tree-model-1868329)  
[Rock set by wernie](https://www.cgtrader.com/free-3d-models/exterior/landscape/rock-set-05-base)  
[Red lantern by Shahid Abdullah](https://www.cgtrader.com/free-3d-models/interior/other/old-lantern-pbr)  
[Trees pack by Seregins](https://www.turbosquid.com/3d-models/3d-model-trees-pack/1091559)  
[Cactus pot by SPACESCAN](https://www.turbosquid.com/3d-models/3d-cactus-concrete-pot-1488079)  
[Old water tap with bucket by Shahid Abdullah](https://www.cgtrader.com/free-3d-models/household/other/old-water-tap-pbr)  
[Bronze lantern by Khan Yasin](https://www.cgtrader.com/free-3d-models/furniture/lamp/old-lantern-cbf79294-7ccd-424d-b35f-006dbfcebc52) 

[Wooden box texture by Katsukagi](https://3dtextures.me/2021/10/18/wood-shingles-001/)  
[Skybox texture by Hipshot](https://www.jose-emilio.com/scenejs/plugins/node/skybox/textures/miramarClouds.jpg)

# required elements
- [x] glfw, glad, shaders, VAOs, VBOs, EBOs: used for wooden box, skybox, bloom, hdr, rendering models...
- [x] textures: wooden box
- [x] transformations: used for models and wooden box
- [x] coordinate systems - local space, world space, view space, clip space, screen space: used for models and wooden box
- [x] camera: used camera class from learnopengl
- [x] colors
- [x] basic lighting - ambient, diffuse, specular
- [x] materials
- [x] lighting maps, light casters, multiple lights: for box and lanterns
- [x] assimp, mesh, models
- [x] model and lighting: lanterns and object lighting
- [x] blending: grass with blending (discard)
- [x] face culling
- [x] advanced lighting: Blinn-Phong
- [x] A: Framebuffers, Cubemaps, Instancing, Anti Aliasing: implemented skybox (cubemaps)
- [x] B: Point shadows; Normal mapping, Parallax Mapping; HDR, Bloom; Deffered Shading; SSAO: implemented HDR, Bloom

# other
- [x] scene
- [x] lantern + light (s)
- [ ] lake/well/water
- [x] trees
- [x] rocks
- [x] moving lantern on a tree
