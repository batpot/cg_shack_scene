#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <learnopengl/filesystem.h>
#include <learnopengl/shader.h>
#include <learnopengl/camera.h>
#include <learnopengl/model.h>

#include <iostream>

using namespace std;

void framebuffer_size_callback(GLFWwindow *window, int width, int height);
void mouse_callback(GLFWwindow *window, double xpos, double ypos);
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);
void processInput(GLFWwindow *window);
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods);
unsigned int loadCubemap(vector<string> faces);
unsigned int loadTexture(char const *path);
void setWoodenBox(Shader &lightingShader, unsigned int diffuseMap, unsigned int specularMap, unsigned int boxVAO);
void renderModel(Shader &ourShader, Model &ourModel, const glm::vec3 &translateVec, const glm::vec3 &scalarVec,
                 const glm::vec3 &rotateVec, float angle, bool rotate = false);
void renderQuad();

// settings
const unsigned int SCR_WIDTH = 1200;
const unsigned int SCR_HEIGHT = 900;
bool hdr = false;
bool hdrKeyPressed = false;
float exposure = 1.0f;
bool bloom = false;
bool bloomKeyPressed = false;

// quad
unsigned int quadVAO = 0;
unsigned int quadVBO;

// camera
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

struct PointLight {
    glm::vec3 position;
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;

    float constant;
    float linear;
    float quadratic;
};

struct ProgramState {
    glm::vec3 clearColor = glm::vec3(0);
    bool ImGuiEnabled = false;
    Camera camera;
    bool CameraMouseMovementUpdateEnabled = true;
    glm::vec3 backpackPosition = glm::vec3(0.0f, 3.0f, 0.0f);
    float backpackScale = 1.0f;
    PointLight pointLight;
    ProgramState()
            : camera(glm::vec3(-7.0f, 0.0f, 26.0f)) {}

    void SaveToFile(string filename);

    void LoadFromFile(string filename);
};

void ProgramState::SaveToFile(string filename) {
    ofstream out(filename);
    out << clearColor.r << '\n'
        << clearColor.g << '\n'
        << clearColor.b << '\n'
        << ImGuiEnabled << '\n';
    //<< camera.Position.x << '\n'
    //<< camera.Position.y << '\n'
    //<< camera.Position.z << '\n'
    //<< camera.Front.x << '\n'
    //<< camera.Front.y << '\n'
    //<< camera.Front.z << '\n';
}

void ProgramState::LoadFromFile(string filename) {
    ifstream in(filename);
    if (in) {
        in >> clearColor.r
           >> clearColor.g
           >> clearColor.b
           >> ImGuiEnabled;
        //>> camera.Position.x
        //>> camera.Position.y
        //>> camera.Position.z
        //>> camera.Front.x
        //>> camera.Front.y
        //>> camera.Front.z;
    }
}

ProgramState *programState;

void DrawImGui(ProgramState *programState);

int main() {

    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    // --------------------
    GLFWwindow *window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "shack scene", nullptr, nullptr);
    if (window == nullptr) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetKeyCallback(window, key_callback);
    // tell GLFW to capture our mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // tell stb_image.h to flip loaded texture's on the y-axis (before loading model).
    //stbi_set_flip_vertically_on_load(true);

    programState = new ProgramState;
    programState->LoadFromFile("resources/program_state.txt");
    if (programState->ImGuiEnabled) {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
    // Init Imgui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void) io;

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");

    // configure global opengl state
    // -----------------------------
    glEnable(GL_DEPTH_TEST);

    //face culling
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    // build and compile shaders
    Shader objShader("resources/shaders/model_lighting.vs", "resources/shaders/model_lighting.fs");
    Shader skyboxShader("resources/shaders/skybox.vs", "resources/shaders/skybox.fs");
    Shader lightingShader("resources/shaders/lightning_maps.vs", "resources/shaders/lightning_maps.fs");
    Shader transparentShader("resources/shaders/transparent.vs", "resources/shaders/transparent.fs");
    Shader bloomShader("resources/shaders/bloom.vs", "resources/shaders/bloom.fs");
    Shader blurShader("resources/shaders/blur.vs", "resources/shaders/blur.fs");
    Shader lightSourceShader("resources/shaders/light_source.vs", "resources/shaders/light_source.fs");

    // load models
    Model deadTree("resources/objects/dead_tree/dead_tree.obj");
    deadTree.SetShaderTextureNamePrefix("material.");

    Model scene("resources/objects/shack_scene/untitled.obj");
    scene.SetShaderTextureNamePrefix("material.");

    Model redLantern("resources/objects/red_lantern/red_lantern.obj");
    redLantern.SetShaderTextureNamePrefix("material.");

    Model plant("resources/objects/plant/plant.obj");
    plant.SetShaderTextureNamePrefix("material.");

    Model bronzeLantern("resources/objects/bronze_lantern/bronze_lantern.obj");
    bronzeLantern.SetShaderTextureNamePrefix("material.");

    Model oldTap("resources/objects/old_tap/old_tap.obj");
    oldTap.SetShaderTextureNamePrefix("material.");

    Model trees("resources/objects/trees_pack/trees_pack.obj");
    trees.SetShaderTextureNamePrefix("material.");

    Model rockA("resources/objects/rock_set/rockA.obj");
    rockA.SetShaderTextureNamePrefix("material.");

    Model rockB("resources/objects/rock_set/rockB.obj");
    rockB.SetShaderTextureNamePrefix("material.");

    Model rockC("resources/objects/rock_set/rockC.obj");
    rockC.SetShaderTextureNamePrefix("material.");

    Model rockD("resources/objects/rock_set/rockD.obj");
    rockD.SetShaderTextureNamePrefix("material.");

    Model rockE("resources/objects/rock_set/rockE.obj");
    rockE.SetShaderTextureNamePrefix("material.");

    Model rockF("resources/objects/rock_set/rockF.obj");
    rockF.SetShaderTextureNamePrefix("material.");

    Model rockG("resources/objects/rock_set/rockG.obj");
    rockG.SetShaderTextureNamePrefix("material.");

    Model cactusPot("resources/objects/cactus_pot/CACTUS_CONCRETE_POT_10K.obj");
    cactusPot.SetShaderTextureNamePrefix("material.");

    Model drop("resources/objects/drop/drop.obj");
    drop.SetShaderTextureNamePrefix("material.");


    float transparentVertices[] = {
            // positions         // texture Coords (swapped y coordinates because texture is flipped upside down)
            0.0f,  0.5f,  0.0f,  0.0f,  0.0f,
            0.0f, -0.5f,  0.0f,  0.0f,  1.0f,
            1.0f, -0.5f,  0.0f,  1.0f,  1.0f,

            0.0f,  0.5f,  0.0f,  0.0f,  0.0f,
            1.0f, -0.5f,  0.0f,  1.0f,  1.0f,
            1.0f,  0.5f,  0.0f,  1.0f,  0.0f
    };

    //set up skybox vertex data
    float skyboxVertices[] = {
            // positions
            -1.0f,  1.0f, -1.0f,
            -1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,

            -1.0f, -1.0f,  1.0f,
            -1.0f, -1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f,  1.0f,
            -1.0f, -1.0f,  1.0f,

            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,

            -1.0f, -1.0f,  1.0f,
            -1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f, -1.0f,  1.0f,
            -1.0f, -1.0f,  1.0f,

            -1.0f,  1.0f, -1.0f,
            1.0f,  1.0f, -1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            -1.0f,  1.0f,  1.0f,
            -1.0f,  1.0f, -1.0f,

            -1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f,  1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f,  1.0f,
            1.0f, -1.0f,  1.0f
    };

    // set up vertex data (and buffer(s)) and configure vertex attributes
    float vertices[] = {
            // positions          // normals           // texture coords
            -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,
            0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  0.0f,
            0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  1.0f,
            0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  1.0f,
            -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  1.0f,
            -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,

            -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,
            0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  0.0f,
            0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  1.0f,
            0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  1.0f,
            -0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  1.0f,
            -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,

            -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  0.0f,
            -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  1.0f,
            -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
            -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
            -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
            -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  0.0f,

            0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,
            0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  1.0f,
            0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
            0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
            0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
            0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,

            -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  1.0f,
            0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  1.0f,
            0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  0.0f,
            0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  0.0f,
            -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  0.0f,
            -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  1.0f,

            -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  1.0f,
            0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  1.0f,
            0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  0.0f,
            0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  0.0f,
            -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,
            -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  1.0f
    };
    //the box's VAO (and boxVBO)
    unsigned int boxVBO, boxVAO;
    glGenVertexArrays(1, &boxVAO);
    glGenBuffers(1, &boxVBO);

    glBindBuffer(GL_ARRAY_BUFFER, boxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindVertexArray(boxVAO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)nullptr);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    // skybox VAO
    unsigned int skyboxVAO, skyboxVBO;
    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);
    glBindVertexArray(skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)nullptr);
    glEnableVertexAttribArray(0);

    // transparent VAO
    unsigned int transparentVAO, transparentVBO;
    glGenVertexArrays(1, &transparentVAO);
    glGenBuffers(1, &transparentVBO);
    glBindVertexArray(transparentVAO);
    glBindBuffer(GL_ARRAY_BUFFER, transparentVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(transparentVertices), transparentVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)nullptr);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindVertexArray(0);

    // configure floating point framebuffer
    unsigned int hdrFBO;
    glGenFramebuffers(1, &hdrFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);
    unsigned int colorBuffers[2]; // create floating point color buffer
    glGenTextures(2, colorBuffers);
    for (unsigned int i = 0; i < 2; i++)
    {
        glBindTexture(GL_TEXTURE_2D, colorBuffers[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, colorBuffers[i], 0); // attach texture to framebuffer
    }
    unsigned int rboDepth; // create depth buffer (renderbuffer)
    glGenRenderbuffers(1, &rboDepth);
    glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, SCR_WIDTH, SCR_HEIGHT);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth);
    glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO); // attach buffers
    unsigned int attachments[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
    glDrawBuffers(2, attachments);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorBuffers[0], 0); //<=
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth);//<=
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        cout << "Framebuffer not complete!" << endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // ping-pong-framebuffer for blurring
    unsigned int pingPongFBO[2];
    unsigned int pingPongColorBuffers[2];
    glGenFramebuffers(2, pingPongFBO);
    glGenTextures(2, pingPongColorBuffers);
    for (unsigned int i = 0; i < 2; i++)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, pingPongFBO[i]);
        glBindTexture(GL_TEXTURE_2D, pingPongColorBuffers[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); // we clamp to the edge as the blur filter would otherwise sample repeated texture values!
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pingPongColorBuffers[i], 0);
        // also check if framebuffers are complete (no need for depth buffer)
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            cout << "Framebuffer not complete!" << endl;
    }

    // load skybox
    vector<std::string> faces
            {
                    FileSystem::getPath("resources/textures/skybox/right.jpg"),
                    FileSystem::getPath("resources/textures/skybox/left.jpg"),
                    FileSystem::getPath("resources/textures/skybox/bottom.jpg"),
                    FileSystem::getPath("resources/textures/skybox/top.jpg"),
                    FileSystem::getPath("resources/textures/skybox/front.jpg"),
                    FileSystem::getPath("resources/textures/skybox/back.jpg")
            };

    // load cubemap
    stbi_set_flip_vertically_on_load(true); // darker parts of the clouds are 'above' sunny parts because of the mood
    // of the scene
    unsigned int cubemapTexture = loadCubemap(faces);
    stbi_set_flip_vertically_on_load(false);

    // load textures for the box
    unsigned int diffuseMap = loadTexture(FileSystem::getPath("resources/textures/box/Wood_Shingles_001_basecolor.jpg").c_str());
    unsigned int specularMap = loadTexture(FileSystem::getPath("resources/textures/box/Wood_Shingles_001_height.png").c_str());

    //load grass texture
    unsigned int transparentTexture = loadTexture(FileSystem::getPath("resources/textures/grass.png").c_str());

    // water position
    vector<glm::vec3> vegetation
            {
                    glm::vec3(-15.5f, -9.0f, -23.48f),
                    glm::vec3( 15.5f, -9.0f, 17.51f),
                    glm::vec3( 16.0f, -9.0f, 13.7f),
                    glm::vec3(-20.3f, -9.0f, -20.3f),
                    glm::vec3 (10.5f, -9.0f, -10.6f),
                    glm::vec3(-17.0f, -9.0f, -13.5f)
            };

    // lighting shader configuration
    lightingShader.use();
    lightingShader.setInt("material.diffuse", 0);
    lightingShader.setInt("material.specular", 1);

    // water (transparent) shader configuration
    transparentShader.use();
    transparentShader.setInt("texture1", 0);

    // skybox shader configuration
    skyboxShader.use();
    skyboxShader.setInt("skybox", 0);

    // bloom shader configuration
    bloomShader.use();
    bloomShader.setInt("scene", 0);
    bloomShader.setInt("bloomBlur", 1);

    // blur shader configuration
    blurShader.use();
    blurShader.setInt("image", 0);

    PointLight& pointLight = programState->pointLight;

    // render loop
    // -----------
    while (!glfwWindowShouldClose(window)) {
        // per-frame time logic
        // --------------------
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);

        glClearColor(programState->clearColor.r, programState->clearColor.g, programState->clearColor.b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        setWoodenBox(lightingShader, diffuseMap, specularMap, boxVAO);

        // view/projection transformations + activate shader
        glm::mat4 projection = glm::perspective(glm::radians(programState->camera.Zoom),
                                                (float) SCR_WIDTH / (float) SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = programState->camera.GetViewMatrix();

        objShader.use();
        objShader.setVec3("viewPosition", programState->camera.Position);
        objShader.setFloat("material.shininess", 32.0f);
        objShader.setMat4("projection", projection);
        objShader.setMat4("view", view);

        lightSourceShader.use();
        lightSourceShader.setMat4("projection", projection);
        lightSourceShader.setMat4("view", view);

        // directional light
        objShader.use();
        objShader.setVec3("dirLight.direction", 30.0f, -10.0f, 30.0f);
        objShader.setVec3("dirLight.ambient", 0.5f, 0.5f, 0.5f);
        objShader.setVec3("dirLight.diffuse", 0.4f, 0.4f, 0.6f);
        objShader.setVec3("dirLight.specular", 1.0f, 1.0f, 0.7f);

        // point light 1 - green lantern
        objShader.setVec3("pointLights[0].position", glm::vec3(10.0f, -11.0f, -25.0f));
        objShader.setVec3("pointLights[0].ambient", 0.05f, 0.05f, 0.05f);
        objShader.setVec3("pointLights[0].diffuse", 0.94f, 0.98f, 0.78f);
        objShader.setVec3("pointLights[0].specular", 0.9f, 0.98f, 0.78f);
        objShader.setFloat("pointLights[0].constant", 1.0f);
        objShader.setFloat("pointLights[0].linear", 0.2);
        objShader.setFloat("pointLights[0].quadratic", 0.1);

        //point light 2 - red lantern
        objShader.setVec3("pointLights[1].position", glm::vec3(-24.0f, -7.0f, -0.5f));
        objShader.setVec3("pointLights[1].ambient",  0.05f, 0.05f, 0.05f);
        objShader.setVec3("pointLights[1].diffuse", 0.94f, 0.98f, 0.78f);
        objShader.setVec3("pointLights[1].specular", 0.94f, 0.98f, 0.78f);
        objShader.setFloat("pointLights[1].constant", 1.0f);
        objShader.setFloat("pointLights[1].linear", 0.2f);
        objShader.setFloat("pointLights[1].quadratic", 0.5f);

        //point light 3 - bronze lantern
        objShader.setVec3("pointLights[2].position", glm::vec3(17.0f, -12.5f, -7.0f));
        objShader.setVec3("pointLights[2].ambient",  0.05f, 0.05f, 0.05f);
        objShader.setVec3("pointLights[2].diffuse", 0.94f, 0.98f, 0.78f);
        objShader.setVec3("pointLights[2].specular", 0.94f, 0.98f, 0.78f);
        objShader.setFloat("pointLights[2].constant", 1.0f);
        objShader.setFloat("pointLights[2].linear", 0.2f);
        objShader.setFloat("pointLights[2].quadratic", 0.1f);

        // lantern with movement
        glm::mat4 movementMat = glm::mat4(1.0f);
        movementMat = glm::translate(movementMat, glm::vec3(-10.7f, -4.6f, -16.7f));
        movementMat = glm::scale(movementMat, glm::vec3(0.4f, 0.4f, 0.4f));
        movementMat = glm::rotate(movementMat, sin(currentFrame * 1.5f) * glm::radians(60.0f), glm::vec3(0, 0, 1));
        movementMat = glm::translate(movementMat, glm::vec3(0.0f, -3.0f, 0.0f));

        glm::mat4 modelMovement = glm::mat4(1.0f);
        modelMovement = glm::translate(movementMat, glm::vec3(-10.9f, -10.0f, 3.7f));
        glm::vec3 positionMovement = movementMat * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
        glm::vec3 base = modelMovement * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
        glm::vec3 spotlightMovement = normalize(positionMovement - base);

        objShader.use();
        objShader.setVec3("spotLights[0].position", base);
        objShader.setVec3("spotLights[0].direction", spotlightMovement);
        objShader.setVec3("spotLights[0].ambient", 0.0f, 0.0f, 0.0f);
        objShader.setVec3("spotLights[0].diffuse", 0.5f, 0.5f, 0.5f);
        objShader.setVec3("spotLights[0].specular", 1.0f, 1.0f, 1.0f);
        objShader.setFloat("spotLights[0].constant", 1.0f);
        objShader.setFloat("spotLights[0].linear", 0.09);
        objShader.setFloat("spotLights[0].quadratic", 0.032);
        objShader.setFloat("spotLights[0].cutOff", glm::cos(glm::radians(16.5f)));
        objShader.setFloat("spotLights[0].outerCutOff", glm::cos(glm::radians(25.0f)));

        // rendering the loaded models

        //scene
        glDisable(GL_CULL_FACE);
        renderModel(objShader, scene, glm::vec3(0.0f, -10.0f, -10.0f),
                    glm::vec3(3.0f), glm::vec3(0.0f), 0.0f, false);
        glEnable(GL_CULL_FACE);

        //tree1 - front, right
        renderModel(objShader, deadTree, glm::vec3(20.0f, -10.0f, -10.0f),
                    glm::vec3(3.0f), glm::vec3(0.0f), 0.0f, false);

        //tree2 - back, right
        renderModel(objShader, deadTree, glm::vec3(15.0f, -10.0f, -30.0f),
                    glm::vec3(3.0f), glm::vec3(0,1,0), 145.0f,true);

        //tree3 - back, left
        renderModel(objShader, deadTree, glm::vec3(-30.0f, -10.0f, -30.0f),
                    glm::vec3(3.0f), glm::vec3(0,1,0), 55.0f,true);

        // red lantern - on the table
        renderModel(lightSourceShader, redLantern, glm::vec3(-24.0f, -7.3f, -0.5f),
                    glm::vec3(0.2f), glm::vec3(0,1,0), 55.0f,true);

        // red2 lantern - behind the cabin
        renderModel(lightSourceShader, redLantern, glm::vec3(10.0f, -10.0f, -25.0f),
                  glm::vec3(0.4f), glm::vec3(0,1,0), 55.0f,true);

        // bronze lantern - the one that is moving
        lightSourceShader.use();
        lightSourceShader.setMat4("model", movementMat);
        bronzeLantern.Draw(lightSourceShader);

        // bronze lantern
        renderModel(lightSourceShader, bronzeLantern, glm::vec3(17.0f, -9.5f, -7.0f),
                   glm::vec3(0.4f), glm::vec3(0,1,0), 55.0f,false);

        // tap & oldTap
        renderModel(objShader, oldTap, glm::vec3(-17.0f, -9.0f, -15.5f),
                    glm::vec3(0.4f), glm::vec3(0,1,0), 90.0f, false);
       // renderModel(objShader, drop, glm::vec3(-19.0f, -2.3f, -15.5f),
         //           glm::vec3(0.4f), glm::vec3(0,1,0), 90.0f, false);

        glm::mat4 dropMat = glm::mat4(1.0f);
        dropMat = glm::translate(dropMat, glm::vec3(-18.0f, -2.3f, -15.5f));
        dropMat = glm::scale(dropMat, glm::vec3(0.4f, 0.4f, 0.4f));
        dropMat = glm::rotate(dropMat, sin(currentFrame) * glm::radians(30.0f), glm::vec3(0, 0, 1));
        dropMat = glm::translate(dropMat, glm::vec3(-3.0f, 0.0f, 0.0f));
        objShader.use();
        objShader.setMat4("model", dropMat);
        //drop.Draw(objShader);

        // cactus pot
        renderModel(objShader, cactusPot, glm::vec3(-10.7f, -4.25f, -16.5f),
                    glm::vec3(4.0f), glm::vec3(0,1,0), 55.0f,false);

        // plant with big leaves
        renderModel(objShader, plant, glm::vec3(13.0f, -10.0f, -7.0f),
                    glm::vec3(0.2f), glm::vec3(0,1,0), 30.0f,true);

        // trees
        renderModel(objShader, trees, glm::vec3(-1.0f, -10.0f, -27.0f),
                    glm::vec3(1.0f), glm::vec3(0,1,0), 55.0f,false);

        // rocks
        renderModel(objShader, rockA, glm::vec3(-10.0f, -10.0f, -3.0f),
                    glm::vec3(0.7f), glm::vec3(0,1,0), 55.0f,false);
        renderModel(objShader, rockB, glm::vec3(-8.5f, -10.0f, -2.5f),
                    glm::vec3(0.5f), glm::vec3(0,1,0), 55.0f,true);
        renderModel(objShader, rockC, glm::vec3(-9.0f, -10.0f, -0.7f),
                    glm::vec3(0.7f), glm::vec3(0,1,0), 30.0f,true);
        renderModel(objShader, rockG, glm::vec3(-10.5f, -10.0f, -0.9f),
                    glm::vec3(1.0f), glm::vec3(0,1,0), 30.0f,true);

        renderModel(objShader, rockE, glm::vec3(7.0f, -10.0f, -25.0f),
                    glm::vec3(1.0f), glm::vec3(0,1,0), 30.0f,true);
        renderModel(objShader, rockF, glm::vec3(8.0f, -10.0f, -26.7f),
                    glm::vec3(1.0f), glm::vec3(0,1,0), 30.0f,true);

        renderModel(objShader, rockC, glm::vec3(7.0f, -10.0f, 4.0f),
                    glm::vec3(0.5f), glm::vec3(0,1,0), 55.0f,false);
        renderModel(objShader, rockE, glm::vec3(8.0f, -10.0f, 3.0f),
                    glm::vec3(0.5f), glm::vec3(0,1,0), 55.0f,false);



        // transparent shader
        transparentShader.use();
        transparentShader.setMat4("projection", projection);
        transparentShader.setMat4("view", view);
        glm::mat4 model = glm::mat4(1.0f);
        glBindVertexArray(transparentVAO);
        glBindTexture(GL_TEXTURE_2D, transparentTexture);
        transparentShader.use();
        for (unsigned int i = 0; i < vegetation.size(); i++)
        {
            model = glm::mat4(1.0f);
            model = glm::translate(model, vegetation[i]);
            transparentShader.setMat4("model", model);
            glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }

        //skybox
        glDepthMask(GL_FALSE);
        glDepthFunc(GL_LEQUAL);  // change depth function so depth test passes when values are equal to depth buffer's content
        skyboxShader.use();
        view = glm::mat4(glm::mat3(programState->camera.GetViewMatrix())); // remove translation from the view matrix
        skyboxShader.setMat4("view", view);
        skyboxShader.setMat4("projection", projection);

        // skybox cube
        glBindVertexArray(skyboxVAO);
        //glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
        glDepthFunc(GL_LESS); // set depth function back to default
        glDepthMask(GL_TRUE);

        // blur bright fragments with two-pass Gaussian Blur
        bool horizontal = true, first_iteration = true;
        unsigned int amount = 5;
        blurShader.use();
        for (unsigned int i = 0; i < amount; i++)
        {
            glBindFramebuffer(GL_FRAMEBUFFER, pingPongFBO[horizontal]);
            blurShader.setInt("horizontal", horizontal);
            glBindTexture(GL_TEXTURE_2D, first_iteration ? colorBuffers[1] : pingPongColorBuffers[!horizontal]);  // bind texture of other framebuffer (or scene if first iteration)
            renderQuad();
            horizontal = !horizontal;
            if (first_iteration)
                first_iteration = false;
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        bloomShader.use();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, colorBuffers[0]);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, pingPongColorBuffers[!horizontal]);
        bloomShader.setInt("bloom", bloom);
        bloomShader.setFloat("exposure", exposure);
        renderQuad();

        //cout << "hdr: " << (hdr ? "on" : "off") << "| exposure: " << exposure << endl;


        //if (programState->ImGuiEnabled)
        //    DrawImGui(programState);


        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    programState->SaveToFile("resources/program_state.txt");
    delete programState;
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------

    glDeleteVertexArrays(1, &boxVAO);
    glDeleteVertexArrays(1, &skyboxVAO);
    glDeleteVertexArrays(1, &transparentVAO);
    glDeleteBuffers(1, &boxVBO);
    glDeleteBuffers(1, &skyboxVBO);
    glDeleteBuffers(1, &transparentVBO);

    glfwTerminate();
    return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(RIGHT, deltaTime);
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow *window, double xpos, double ypos) {
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

    if (programState->CameraMouseMovementUpdateEnabled)
        programState->camera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset) {
    programState->camera.ProcessMouseScroll(yoffset);
}

void DrawImGui(ProgramState *programState) {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();


    {
        static float f = 0.0f;
        ImGui::Begin("Hello window");
        ImGui::Text("Hello text");
        ImGui::SliderFloat("Float slider", &f, 0.0, 1.0);
        ImGui::ColorEdit3("Background color", (float *) &programState->clearColor);
        ImGui::DragFloat3("Backpack position", (float*)&programState->backpackPosition);
        ImGui::DragFloat("Backpack scale", &programState->backpackScale, 0.05, 0.1, 4.0);

        ImGui::DragFloat("pointLight.constant", &programState->pointLight.constant, 0.05, 0.0, 1.0);
        ImGui::DragFloat("pointLight.linear", &programState->pointLight.linear, 0.05, 0.0, 1.0);
        ImGui::DragFloat("pointLight.quadratic", &programState->pointLight.quadratic, 0.05, 0.0, 1.0);
        ImGui::End();
    }

    {
        ImGui::Begin("Camera info");
        const Camera& c = programState->camera;
        ImGui::Text("Camera position: (%f, %f, %f)", c.Position.x, c.Position.y, c.Position.z);
        ImGui::Text("(Yaw, Pitch): (%f, %f)", c.Yaw, c.Pitch);
        ImGui::Text("Camera front: (%f, %f, %f)", c.Front.x, c.Front.y, c.Front.z);
        ImGui::Checkbox("Camera mouse update", &programState->CameraMouseMovementUpdateEnabled);
        ImGui::End();
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods){
    if (key == GLFW_KEY_F1 && action == GLFW_PRESS) {
        programState->ImGuiEnabled = !programState->ImGuiEnabled;
        if (programState->ImGuiEnabled) {
            programState->CameraMouseMovementUpdateEnabled = false;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        } else {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }
    }
    if (glfwGetKey(window, GLFW_KEY_B) == GLFW_PRESS && !bloomKeyPressed)
    {
        bloom = !bloom;
        bloomKeyPressed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_B) == GLFW_RELEASE)
    {
        bloomKeyPressed = false;
    }
    if (glfwGetKey(window, GLFW_KEY_H) == GLFW_PRESS && !hdrKeyPressed)
    {
        hdr = !hdr;
        hdrKeyPressed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_H) == GLFW_RELEASE)
    {
        hdrKeyPressed = false;
    }
}

unsigned int loadCubemap(vector<std::string> faces)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    int width, height, nrChannels;
    for (unsigned int i = 0; i < faces.size(); i++)
    {
        unsigned char *data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
        if (data)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                         0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data
            );
            stbi_image_free(data);
        }
        else
        {
            std::cout << "Cubemap tex failed to load at path: " << faces[i] << std::endl;
            stbi_image_free(data);
        }
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return textureID;
}

unsigned int loadTexture(char const *path)
{
    unsigned textureId;
    glGenTextures(1, &textureId);

    int width, height, nrComponents;
    unsigned char *data = stbi_load(path, &width, &height, &nrComponents, 0);

    if(data)
    {
        GLenum format;
        if(nrComponents == 1)
            format = GL_RED;
        else if(nrComponents == 3)
            format = GL_RGB;
        else if(nrComponents == 4)
            format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureId);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);

    }
    else
    {
        cout << "Texture failed to load!" << path << "\n";
        stbi_image_free(data);
    }

    return textureId;
}

void setWoodenBox(Shader &lightingShader, unsigned int diffuseMap, unsigned int specularMap, unsigned int boxVAO)
{
    //set shader
    lightingShader.use();
    lightingShader.setVec3("light.position", glm::vec3(10.0, 10.0, 5.0));
    lightingShader.setVec3("viewPos", programState->camera.Position);

    // light properties
    lightingShader.setVec3("light.ambient", 0.2f, 0.2f, 0.2f);
    lightingShader.setVec3("light.diffuse", 0.5f, 0.5f, 0.5f);
    lightingShader.setVec3("light.specular", 1.0f, 1.0f, 1.0f);

    // material properties
    lightingShader.setFloat("material.shininess", 64.0f);

    // view/projection transformations
    glm::mat4 projection = glm::perspective(glm::radians(programState->camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
    glm::mat4 view = programState->camera.GetViewMatrix();
    lightingShader.setMat4("projection", projection);
    lightingShader.setMat4("view", view);

    // world transformation
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(17.0f, -10.0f, -7.0f));
    lightingShader.setMat4("model", model);

    // bind diffuse map
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, diffuseMap);
    // bind specular map
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, specularMap);

    // render the cube
    glBindVertexArray(boxVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);

}

void renderModel(Shader &ourShader, Model &ourModel, const glm::vec3 &translateVec, const glm::vec3 &scalarVec,
                 const glm::vec3 &rotateVec, float angle, bool rotate)
{
    ourShader.use();
    glm::mat4 modelMat = glm::mat4(1.0f);
    modelMat = glm::translate(modelMat, translateVec);
    modelMat = glm::scale(modelMat, scalarVec);
    if(rotate)
        modelMat = glm::rotate(modelMat, angle, rotateVec);
    ourShader.setMat4("model", modelMat);
    ourModel.Draw(ourShader);
}
void renderQuad()
{
    if (quadVAO == 0)
    {
        float quadVertices[] = {
                // positions        // texture Coords
                -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
                -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
                1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
                1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
        };
        // setup plane VAO
        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);
        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)nullptr);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    }
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}