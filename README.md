# cabin scene
- Use WASD for Camera movements
- ??

# Uputstvo
1. `git clone https://github.com/matf-racunarska-grafika/project_base.git`
2. CLion -> Open -> path/to/my/project_base
3. Main se nalazi u src/main.cpp
4. Cpp fajlovi idu u src folder
5. Zaglavlja (h i hpp) fajlovi idu u include
6. Šejderi idu u folder shaders. `Vertex shader` ima ekstenziju `.vs`, `fragment shader` ima ekstenziju `.fs`
7. ALT+SHIFT+F10 -> project_base -> run

# TODOs:
- [x] fix the objects (weird colour)
- [x] fix skybox (bad cropping)
- [x] add blending
- [ ] dodaj odakle su preuzeti modeli i teksture
- [ ] uputstvo 

# Required elements
- [x] glfw, glad, shaders, VAOs, VBOs, EBOs: used for wooden box, skybox, bloom, hdr, rendering models...
- [x] textures: wooden box
- [x] transformations: used for models and wooden box
- [x] coordinate systems - local space, world space, view space, clip space, screen space: used for models and wooden box
- [x] camera: used camera class from learnopengl
- [ ] colors
- [x] basic lighting - ambient, diffuse, specular
- [ ] materials
- [x] lighting maps, light casters, multiple lights: for box and lanterns
- [x] assimp, mesh, models
- [x] model and lighting: lanterns and object lighting
- [ ] imgui ?
- [ ] text rendering ?
- [x] blending: grass with blending (discard)
- [x] face culling
- [ ] advanced lighting: Blinn-Phong
- [x] A: Framebuffers, Cubemaps, Instancing, Anti Aliasing: implemented skybox (cubemaps)
- [ ] B: Point shadows; Normal mapping, Parallax Mapping; HDR, Bloom; Deffered Shading; SSAO

other
- [x] scene
- [x] lantern + light (s)
- [ ] lake/well/water
- [x] trees
- [ ] rocks
- [ ] moving lantern on a tree
