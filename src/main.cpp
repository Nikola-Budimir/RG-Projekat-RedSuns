#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <learnopengl/filesystem.h>
#include <learnopengl/shader.h>
#include <learnopengl/camera.h>
#include <learnopengl/model.h>

#include <iostream>

void framebuffer_size_callback(GLFWwindow *window, int width, int height);

void mouse_callback(GLFWwindow *window, double xpos, double ypos);

void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);

void processInput(GLFWwindow *window);

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods);

unsigned int loadCubemap(vector<std::string> faces);

unsigned int loadTexture(const char *path);

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// camera

float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

//spotLight switch
bool spotSwitch = false;

struct DirectionLight{
    glm::vec3 direction;

    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
};

struct ProgramState {
    bool ImGuiEnabled = false;
    Camera camera;
    bool CameraMouseMovementUpdateEnabled = true;
    DirectionLight directionLight;
    bool shadows = true;
    Model *AE86, *lamps, *dumpster;

    glm::vec3 ae86pos = glm::vec3(0.0f, 0.11f, 0.0f);
    float ae86angle = 205.0f;

    ProgramState()
            : camera(glm::vec3(0.0f, 0.0f, 3.0f)) {}

    void SaveToFile(std::string filename);

    void LoadFromFile(std::string filename);
};

void ProgramState::SaveToFile(std::string filename) {
    std::ofstream out(filename);
    out << ImGuiEnabled << '\n'
        << camera.Position.x << '\n'
        << camera.Position.y << '\n'
        << camera.Position.z << '\n'
        << camera.Front.x << '\n'
        << camera.Front.y << '\n'
        << camera.Front.z << '\n'
        << ae86pos.x << '\n'
        << ae86pos.z << '\n';

}

void ProgramState::LoadFromFile(std::string filename) {
    std::ifstream in(filename);
    if (in) {
        in >> ImGuiEnabled
           >> camera.Position.x
           >> camera.Position.y
           >> camera.Position.z
           >> camera.Front.x
           >> camera.Front.y
           >> camera.Front.z
           >> ae86pos.x
           >> ae86pos.z;
    }
}

ProgramState *programState;

void DrawImGui(ProgramState *programState);

void renderScene(Shader &ourShader);

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
    GLFWwindow *window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
    if (window == NULL) {
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
    stbi_set_flip_vertically_on_load(true);

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

    // build and compile shaders
    // -------------------------
    stbi_set_flip_vertically_on_load(false);
    Shader ourShader("resources/shaders/2.model_lighting.vs", "resources/shaders/2.model_lighting.fs");
    Shader skyboxShader("resources/shaders/skybox.vs", "resources/shaders/skybox.fs");
    Shader depthShader("resources/shaders/shadows_depth.vs", "resources/shaders/shadows_depth.fs", "resources/shaders/shadows_depth.gs");

    // load models
    // -----------


    DirectionLight& directionLight = programState->directionLight;

    directionLight.direction = glm::vec3(0.0f, -1.0f, 1.0f);
    directionLight.ambient = glm::vec3(0.1, 0.1, 0.1);
    directionLight.diffuse = glm::vec3(0.3f, 0.3f, 0.12f);
    directionLight.specular = glm::vec3(0.1, 0.1, 0.1);


    //enabling blending
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);

    Model AE86("resources/objects/jdm/AE86Trueno.obj");
    AE86.SetShaderTextureNamePrefix("material.");
    programState->AE86 = &AE86;

    Model lamps("resources/objects/lamps/lamps.obj");
    lamps.SetShaderTextureNamePrefix("material.");
    programState->lamps = &lamps;

    Model dumpster("resources/objects/dumpster/dumpster_obj.obj");
    dumpster.SetShaderTextureNamePrefix("material.");
    programState->dumpster = &dumpster;

    //enabling faceculling
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);


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

    //skyboxVAO
    unsigned int skyboxVAO, skyboxVBO;
    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);
    glBindVertexArray(skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

    //lightCube
    float vertices[] = {
            -0.5f, -0.5f, -0.5f,
            0.5f, -0.5f, -0.5f,
            0.5f,  0.5f, -0.5f,
            0.5f,  0.5f, -0.5f,
            -0.5f,  0.5f, -0.5f,
            -0.5f, -0.5f, -0.5f,

            -0.5f, -0.5f,  0.5f,
            0.5f, -0.5f,  0.5f,
            0.5f,  0.5f,  0.5f,
            0.5f,  0.5f,  0.5f,
            -0.5f,  0.5f,  0.5f,
            -0.5f, -0.5f,  0.5f,

            -0.5f,  0.5f,  0.5f,
            -0.5f,  0.5f, -0.5f,
            -0.5f, -0.5f, -0.5f,
            -0.5f, -0.5f, -0.5f,
            -0.5f, -0.5f,  0.5f,
            -0.5f,  0.5f,  0.5f,

            0.5f,  0.5f,  0.5f,
            0.5f,  0.5f, -0.5f,
            0.5f, -0.5f, -0.5f,
            0.5f, -0.5f, -0.5f,
            0.5f, -0.5f,  0.5f,
            0.5f,  0.5f,  0.5f,

            -0.5f, -0.5f, -0.5f,
            0.5f, -0.5f, -0.5f,
            0.5f, -0.5f,  0.5f,
            0.5f, -0.5f,  0.5f,
            -0.5f, -0.5f,  0.5f,
            -0.5f, -0.5f, -0.5f,

            -0.5f,  0.5f, -0.5f,
            0.5f,  0.5f, -0.5f,
            0.5f,  0.5f,  0.5f,
            0.5f,  0.5f,  0.5f,
            -0.5f,  0.5f,  0.5f,
            -0.5f,  0.5f, -0.5f,
    };

    // load textures
    // -------------

    vector<std::string> faces
            {
                    FileSystem::getPath("resources/textures/cloudskybox/right.jpg"),
                    FileSystem::getPath("resources/textures/cloudskybox/left.jpg"),
                    FileSystem::getPath("resources/textures/cloudskybox/top.jpg"),
                    FileSystem::getPath("resources/textures/cloudskybox/bottom.jpg"),
                    FileSystem::getPath("resources/textures/cloudskybox/front.jpg"),
                    FileSystem::getPath("resources/textures/cloudskybox/back.jpg")
            };

    stbi_set_flip_vertically_on_load(false);
    unsigned int cubemapTexture = loadCubemap(faces);
    stbi_set_flip_vertically_on_load(true);

    // configure depth map FBO
    // -----------------------
    const unsigned int SHADOW_WIDTH = 1024, SHADOW_HEIGHT = 1024;
    unsigned int depthMap1FBO;
    glGenFramebuffers(1, &depthMap1FBO);
    // create depth cubemap texture
    unsigned int depthCubemap1;
    glGenTextures(1, &depthCubemap1);
    glBindTexture(GL_TEXTURE_CUBE_MAP, depthCubemap1);
    for (unsigned int i = 0; i < 6; ++i)
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    // attach depth texture as FBO's depth buffer
    glBindFramebuffer(GL_FRAMEBUFFER, depthMap1FBO);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthCubemap1, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    unsigned int depthMap2FBO;
    glGenFramebuffers(1, &depthMap2FBO);
    // create depth cubemap texture
    unsigned int depthCubemap2;
    glGenTextures(1, &depthCubemap2);
    glBindTexture(GL_TEXTURE_CUBE_MAP, depthCubemap2);
    for (unsigned int i = 0; i < 6; ++i)
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    // attach depth texture as FBO's depth buffer
    glBindFramebuffer(GL_FRAMEBUFFER, depthMap2FBO);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthCubemap2, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);




    // shader configuration
    // --------------------
    ourShader.use();
    ourShader.setInt("material.texture_diffuse1", 0);
    ourShader.setInt("depthMap1", 1);
    ourShader.setInt("depthMap2", 2);


    skyboxShader.use();
    skyboxShader.setInt("texture1", 0);

    //buffering of vertexes and loading textures

            //vertices                      //normals                      //tex coordinates
    float roadVertices[] = {
            -1.0f, 0.0f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
            1.0f, 0.0f, -0.5f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f,
            1.0f, 0.0f, 0.5f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f,
            1.0f, 0.0f, 0.5f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f,
            -1.0f, 0.0f, 0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
            -1.0f, 0.0f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
    };
    unsigned int VBO, VAO;
    glGenBuffers(1, &VBO);
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(roadVertices), roadVertices, GL_STATIC_DRAW);


    glVertexAttribPointer(0,3, GL_FLOAT, GL_FALSE, 8*sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8*sizeof(float), (void*)(3*sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8*sizeof(float), (void*)(6*sizeof(float)));
    glEnableVertexAttribArray(2);




    unsigned int roadTex = loadTexture(FileSystem::getPath("resources/textures/parking.jpg").c_str());

    // draw in wireframe
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);


    glm::vec3 lightPos[2] = {glm::vec3 (-1.74f, 1.48f, -0.12f), glm::vec3(1.74f, 1.48f, 0.12f)};

    // render loop
    // -----------
    while (!glfwWindowShouldClose(window)) {
        // per-frame time logic
        // --------------------
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // input
        // -----
        processInput(window);

        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        //depth cubemap transformation matrices
        // -----------------------------------------------
        float near_plane = 1.0f;
        float far_plane  = 25.0f;
        glm::mat4 shadowProj = glm::perspective(glm::radians(90.0f), (float)SHADOW_WIDTH / (float)SHADOW_HEIGHT, near_plane, far_plane);
        std::vector<glm::mat4> shadowTransforms1;
        shadowTransforms1.push_back(shadowProj * glm::lookAt(lightPos[0], lightPos[0] + glm::vec3( 1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)));
        shadowTransforms1.push_back(shadowProj * glm::lookAt(lightPos[0], lightPos[0] + glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)));
        shadowTransforms1.push_back(shadowProj * glm::lookAt(lightPos[0], lightPos[0] + glm::vec3( 0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)));
        shadowTransforms1.push_back(shadowProj * glm::lookAt(lightPos[0], lightPos[0] + glm::vec3( 0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)));
        shadowTransforms1.push_back(shadowProj * glm::lookAt(lightPos[0], lightPos[0] + glm::vec3( 0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)));
        shadowTransforms1.push_back(shadowProj * glm::lookAt(lightPos[0], lightPos[0] + glm::vec3( 0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f)));

        std::vector<glm::mat4> shadowTransforms2;
        shadowTransforms2.push_back(shadowProj * glm::lookAt(lightPos[1], lightPos[1] + glm::vec3( 1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)));
        shadowTransforms2.push_back(shadowProj * glm::lookAt(lightPos[1], lightPos[1] + glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)));
        shadowTransforms2.push_back(shadowProj * glm::lookAt(lightPos[1], lightPos[1] + glm::vec3( 0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)));
        shadowTransforms2.push_back(shadowProj * glm::lookAt(lightPos[1], lightPos[1] + glm::vec3( 0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)));
        shadowTransforms2.push_back(shadowProj * glm::lookAt(lightPos[1], lightPos[1] + glm::vec3( 0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)));
        shadowTransforms2.push_back(shadowProj * glm::lookAt(lightPos[1], lightPos[1] + glm::vec3( 0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f)));

        // render scene to depth cubemap1
        // --------------------------------
        glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
        glBindFramebuffer(GL_FRAMEBUFFER, depthMap1FBO);
        glClear(GL_DEPTH_BUFFER_BIT);
        depthShader.use();
        for (unsigned int i = 0; i < 6; ++i)
            depthShader.setMat4("shadowMatrices[" + std::to_string(i) + "]", shadowTransforms1[i]);
        depthShader.setFloat("far_plane", far_plane);
        depthShader.setVec3("lightPos", lightPos[0]);
        renderScene(depthShader);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // render scene to depth cubemap2
        // --------------------------------
        glBindFramebuffer(GL_FRAMEBUFFER, depthMap2FBO);
        glClear(GL_DEPTH_BUFFER_BIT);
        depthShader.use();
        for (unsigned int i = 0; i < 6; ++i)
            depthShader.setMat4("shadowMatrices[" + std::to_string(i) + "]", shadowTransforms2[i]);
        depthShader.setFloat("far_plane", far_plane);
        depthShader.setVec3("lightPos", lightPos[1]);
        renderScene(depthShader);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);


        //rendering scene with shadows
        glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        ourShader.use();

        // view/projection transformations
        glm::mat4 projection = glm::perspective(glm::radians(programState->camera.Zoom),(float) SCR_WIDTH / (float) SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = programState->camera.GetViewMatrix();
        ourShader.setMat4("projection", projection);
        ourShader.setMat4("view", view);

        ourShader.setVec3("viewPosition", programState->camera.Position);
        ourShader.setInt("shadows", programState->shadows);
        ourShader.setFloat("far_plane", far_plane);

        //point light 1
        ourShader.setVec3("pointLight[0].position", lightPos[0]);
        ourShader.setVec3("pointLight[0].ambient", glm::vec3(0.05f, 0.05f, 0.05f));
        ourShader.setVec3("pointLight[0].diffuse", glm::vec3(0.4f, 0.4f, 0.4f));
        ourShader.setVec3("pointLight[0].specular", glm::vec3(1.0f, 1.0f, 1.0f));
        ourShader.setFloat("pointLight[0].constant", 1.0f);
        ourShader.setFloat("pointLight[0].linear", 0.09f);
        ourShader.setFloat("pointLight[0].quadratic", 0.032f);
        //point light 2
        ourShader.setVec3("pointLight[1].position", lightPos[1]);
        ourShader.setVec3("pointLight[1].ambient", glm::vec3(0.05f, 0.05f, 0.05f));
        ourShader.setVec3("pointLight[1].diffuse", glm::vec3(0.4f, 0.4f, 0.4f));
        ourShader.setVec3("pointLight[1].specular", glm::vec3(1.0f, 1.0f, 1.0f));
        ourShader.setFloat("pointLight[1].constant", 1.0f);
        ourShader.setFloat("pointLight[1].linear", 0.09f);
        ourShader.setFloat("pointLight[1].quadratic", 0.032f);
        //spotlight
        ourShader.setVec3("spotLight.ambient",  0.1f, 0.1f, 0.1f);
        ourShader.setVec3("spotLight.diffuse",  0.5f, 0.5f, 0.5f);
        ourShader.setVec3("spotLight.specular", 1.0f, 1.0f, 1.0f);
        ourShader.setVec3("spotLight.direction", programState->camera.Front);
        ourShader.setVec3("spotLight.position", programState->camera.Position);
        ourShader.setFloat("spotLight.cutOff", glm::cos(glm::radians(12.5f)));
        ourShader.setFloat("spotLight.outerCutOff", glm::cos(glm::radians(17.5f)));
        ourShader.setFloat("spotLight.constant",  1.0f);
        ourShader.setFloat("spotLight.linear",    0.09f);
        ourShader.setFloat("spotLight.quadratic", 0.032f);
        ourShader.setBool("spotLight.spotSwitch", spotSwitch);
        //direction light
        ourShader.setVec3("directionLight.ambient",  programState->directionLight.ambient);
        ourShader.setVec3("directionLight.diffuse",  programState->directionLight.diffuse);
        ourShader.setVec3("directionLight.specular", programState->directionLight.specular);
        ourShader.setVec3("directionLight.direction", programState->directionLight.direction);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, roadTex);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_CUBE_MAP, depthCubemap1);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_CUBE_MAP, depthCubemap2);

        renderScene(ourShader);

        //rendering terrain
        ourShader.use();
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::scale(model, glm::vec3(3.0f));
        model = glm::rotate(model, glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        ourShader.setMat4("model", model);
        ourShader.setFloat("material.shininess", 32.0f);
        glBindVertexArray(VAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, roadTex);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindTexture(GL_TEXTURE_2D, 0);


        //skybox rendering
        glDepthFunc(GL_LEQUAL);  // change depth function so depth test passes when values are equal to depth buffer's content
        skyboxShader.use();
        view = glm::mat4(glm::mat3(programState->camera.GetViewMatrix())); // remove translation from the view matrix
        skyboxShader.setMat4("view", view);
        skyboxShader.setMat4("projection", projection);
        // skybox cube
        glBindVertexArray(skyboxVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
        glDepthFunc(GL_LESS); // set depth function back to default

        if (programState->ImGuiEnabled)
            DrawImGui(programState);



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
    glfwTerminate();
    return 0;
}
void renderScene(Shader &ourShader){
    //car
    glm::mat4 model2 = glm::mat4(1.0f);
    model2 = glm::translate(model2, programState->ae86pos);
    model2 = glm::rotate(model2, glm::radians(programState->ae86angle), glm::vec3(0.0f, 1.0f, 0.0f));
    model2 = glm::scale(model2, glm::vec3(0.2f));
    ourShader.setMat4("model", model2);
    ourShader.setFloat("material.shininess", 128.0f);

    programState->AE86->Draw(ourShader);

    //lamps

    glm::mat4 model3 = glm::mat4(1.0f);
    model3 = glm::translate(model3, glm::vec3(0.0f, 0.11f, 0.0f));
    model3 = glm::scale(model3, glm::vec3(0.2f));
    ourShader.setMat4("model", model3);


    programState->lamps->Draw(ourShader);

    glm::mat4 model4 = glm::mat4(1.0f);
    model4 = glm::translate(model4, glm::vec3(2.6f, 0.0f, 0.9f));
    model4 = glm::rotate(model4, glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    model4 = glm::scale(model4, glm::vec3(0.25f));
    ourShader.setMat4("model", model4);

    programState->dumpster->Draw(ourShader);

    model4 = glm::translate(model4, glm::vec3(-2.1f, 0.0f, 0.0f));
    model4 = glm::rotate(model4, glm::radians(-10.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    ourShader.setMat4("model", model4);

    programState->dumpster->Draw(ourShader);

    model4 = glm::rotate(model4, glm::radians(10.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    model4 = glm::translate(model4, glm::vec3(-4.5f, 0.0f, 0.0f));
    model4 = glm::rotate(model4, glm::radians(10.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    ourShader.setMat4("model", model4);

    programState->dumpster->Draw(ourShader);

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
        ImGui::Begin("Settings");
        ImGui::Text("Lighting");

        ImGui::DragFloat("directionLight.direction.x", &programState->directionLight.direction.x, 0.05, -1.0, 1.0);
        ImGui::DragFloat("directionLight.direction.y", &programState->directionLight.direction.y, 0.05, -1.0, 1.0);
        ImGui::DragFloat("directionLight.direction.z", &programState->directionLight.direction.z, 0.05, -1.0, 1.0);
        ImGui::Checkbox("shadows", &programState->shadows);

        ImGui::Text("Car positioning");
        ImGui::DragFloat("x coordinate", &programState->ae86pos.x, 0.05, -1.5, 1.5);
        ImGui::DragFloat("z coordinate", &programState->ae86pos.z, 0.05, -1.0, 1.0);
        ImGui::DragFloat("angle", &programState->ae86angle, 1.0f, 0.0, 360.0);

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

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_F1 && action == GLFW_PRESS) {
        programState->ImGuiEnabled = !programState->ImGuiEnabled;
        if (programState->ImGuiEnabled) {
            programState->CameraMouseMovementUpdateEnabled = false;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        } else {
            programState->CameraMouseMovementUpdateEnabled = true;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }
    }
    if(key==GLFW_KEY_Y && action == GLFW_PRESS){
        spotSwitch = !spotSwitch;
    }
}
unsigned int loadTexture(char const * path)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char *data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data)
    {
        GLenum format;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, format == GL_RGBA ? GL_CLAMP_TO_EDGE : GL_REPEAT); // for this tutorial: use GL_CLAMP_TO_EDGE to prevent semi-transparent borders. Due to interpolation it takes texels from next repeat
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, format == GL_RGBA ? GL_CLAMP_TO_EDGE : GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else
    {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
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
