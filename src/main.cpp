#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <my_imgui.h>
#include <my_shader.h>
#include <my_camera.h>
#include <my_model.h>
#include <my_skybox.h>

#include <iostream>
#include <random>
#define _USE_MATH_DEFINES
#include <math.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// Callback function declarations
void frameBufferSizeCallback(GLFWwindow* window, int width, int height);
void mouseCallback(GLFWwindow* window, double xIn, double yIn);
void scrollCallback(GLFWwindow* window, double xOff, double yOff);
void processUserInput(GLFWwindow* window);

// Screen params
unsigned int SCREEN_WIDTH = 1920;
unsigned int SCREEN_HEIGHT = 1080;

// Mouse params
bool firstMouse = true;
float xPrev = static_cast<float>(SCREEN_WIDTH) / 2.0f;
float yPrev = static_cast<float>(SCREEN_HEIGHT) / 2.0f;

// Timing params
float deltaTime = 0.0f;
float prevFrame = 0.0f;
float elapsedTime = 0.0f;

// Model names
#define TEAPOT_MODEL "models/teapot.fbx"
#define DONUT_MODEL "models/donut.fbx"
#define SPHERE_MODEL "models/sphere.fbx"
#define MONKEY_MODEL "models/suzanne_monkey.fbx"
#define BUDDHA_MODEL "models/buddha.fbx"
std::vector<Model> allModels = {};

// Model matrix params
float rotY = 0.0f;

// Skyboxes
GLuint graffitiSkyboxVAO;
GLuint graffitiCubemapTexture;
GLuint nightSkyboxVAO;
GLuint nightCubemapTexture;
GLuint museumSkyboxVAO;
GLuint museumCubemapTexture;

// Backface components
GLuint backfaceFBO, backfaceNormalTex, backfaceDepthTex;

// Shader types
enum ShaderType
{
    OneSurfaceShader = 0,
    TwoSurfacesBackFaceShader = 1,
    TwoSurfacesFrontFaceShader = 2
};

// Camera specs (set later, can't call functions here)
const float cameraSpeed = 3.0f;
const float mouseSensitivity = 0.1f;
const float cameraZoom = 50.0f;
const float xPosInit = 0.0f;
const float yPosInit = 0.0f;
const float zPosInit = 5.0f;
Camera camera(glm::vec3(xPosInit, yPosInit, zPosInit));

int setupGLFW(GLFWwindow** window)
{
    // glfw init and configure
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_DECORATED, NULL); // Remove title bar

    // Screen params
    GLFWmonitor* MyMonitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(MyMonitor);
    SCREEN_WIDTH = mode->width; SCREEN_HEIGHT = mode->height;

    // glfw window creation
    GLFWwindow* glfwWindow = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Realtime Rendering Assignment 5", glfwGetPrimaryMonitor(), nullptr);
    if (glfwWindow == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(glfwWindow);

    // Callback functions
    glfwSetFramebufferSizeCallback(glfwWindow, frameBufferSizeCallback);
    glfwSetCursorPosCallback(glfwWindow, mouseCallback);
    glfwSetScrollCallback(glfwWindow, scrollCallback);

    // Mouse capture (start with cursor diabled and ImGUI hidden)
    if (ImGuiUseMouse)
        glfwSetInputMode(glfwWindow, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    else
        glfwSetInputMode(glfwWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // Load all OpenGL function pointers with GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }
    *window = glfwWindow;

    // Configure global OpenGL state
    glEnable(GL_DEPTH_TEST);    // Depth-testing
    glDepthFunc(GL_LESS);       // Smaller value as "closer" for depth-testing
    return 0;
}

void loadModels()
{
    allModels.clear();
    Model teapotModel(TEAPOT_MODEL, "Teapot"); allModels.push_back(teapotModel);
    Model donutModel(DONUT_MODEL, "Donut"); allModels.push_back(donutModel);
    Model sphereModel(SPHERE_MODEL, "Sphere"); allModels.push_back(sphereModel);
    Model monkeyModel(MONKEY_MODEL, "Monkey"); allModels.push_back(monkeyModel);
    Model buddhaModel(BUDDHA_MODEL, "Buddha"); allModels.push_back(buddhaModel);
}

void setupSkybox(GLuint* skyboxVAO, GLuint* cubemapTexture, const std::string skyboxName)
{
    // Setup skybox VAO
    *skyboxVAO = setupSkyboxVAO();

    std::vector<std::string> facesCubemap =
    {
        "skybox/" + skyboxName + "/px.png",   
        "skybox/" + skyboxName + "/nx.png",  
        "skybox/" + skyboxName + "/py.png",     
        "skybox/" + skyboxName + "/ny.png",   
        "skybox/" + skyboxName + "/pz.png",    
        "skybox/" + skyboxName + "/nz.png"      
    };

    // Skybox cubemap
    *cubemapTexture = loadCubemap(facesCubemap);
}

void setupCamera()
{
    // Fine tune camera params
    camera.setMouseSensitivity(mouseSensitivity);
    camera.setCameraMovementSpeed(cameraSpeed);
    camera.setZoom(cameraZoom);
    camera.setFPSCamera(false, yPosInit);
    camera.setZoomEnabled(false);
}

void drawSkyBox(Shader& skyboxShader, const glm::mat4& projection, const glm::mat4 view)
{
    glDisable(GL_DEPTH_TEST);
    skyboxShader.use();

    // Remove translation component from the view matrix for the skybox
    glm::mat4 viewNoTrans = glm::mat4(glm::mat3(view));
    skyboxShader.setMat4("view", viewNoTrans);
    skyboxShader.setMat4("projection", projection);

    // Bind the skybox texture and render
    glActiveTexture(GL_TEXTURE0);
    switch (selectedSkybox)
    {
    case Graffiti: 
        glBindTexture(GL_TEXTURE_CUBE_MAP, graffitiCubemapTexture);
        glBindVertexArray(graffitiSkyboxVAO);
        break;

    case NightSky:
        glBindTexture(GL_TEXTURE_CUBE_MAP, nightCubemapTexture);
        glBindVertexArray(nightSkyboxVAO);
        break;

    case Museum:
        glBindTexture(GL_TEXTURE_CUBE_MAP, museumCubemapTexture);
        glBindVertexArray(museumSkyboxVAO);
        break;

    default:
        break;
    }
    skyboxShader.setInt("skybox", 0);

    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
    glEnable(GL_DEPTH_TEST);
}

void drawModel(Shader& shader, const glm::mat4& projection, const glm::mat4 view, const ShaderType& shaderType)
{
    // Draw models with shader
    shader.use();

    // All have same model, view and projection 
    //glm::vec3 modelPosition = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::mat4 model = glm::identity<glm::mat4>();
    //model = glm::translate(model, modelPosition);
    model = glm::rotate(model, glm::radians(rotY), glm::vec3(0.0f, 1.0f, 0.0f));
    shader.setMat4("model", model);
    shader.setMat4("view", view);
    shader.setMat4("projection", projection);

    // Set the rest of the uniforms based on which shader is in use
    switch (shaderType)
    {
    case OneSurfaceShader:
        // F0, eta, and skybox
        shader.setFloat("modelIOR", IOR);
        shader.setBool("reflectEnable", enableReflect);
        shader.setInt("skybox", 0);
        break;

    case TwoSurfacesBackFaceShader:
        // No other uniforms
        break;

    case TwoSurfacesFrontFaceShader:
        // F0, eta, skybox, backface normals, backface depths
        shader.setFloat("modelIOR", IOR);
        shader.setBool("reflectEnable", enableReflect);
        shader.setBool("viewSpaceOnly", screenSpaceOnly);
        shader.setInt("skybox", 0);
        shader.setInt("backfaceNormalTex", 1);
        shader.setInt("backfaceDepthTex", 2);
        break;
    default:
        std::cerr << "Invalid shader type provided to drawModel(). Returning.\n";
        return;
    }

    // Draw
    allModels[selectedModel].draw(shader);
}

int main()
{
    // Window
    GLFWwindow* window = nullptr;
    if (setupGLFW(&window))
        return -1;

    // Shaders
    Shader skyboxShader("shaders/skyboxShader.vs", "shaders/skyboxShader.fs");
    Shader refractionShader("shaders/refractionShader.vs", "shaders/refractionShader.fs");
    Shader backfaceShader("shaders/backfaceShader.vs", "shaders/backfaceShader.fs");
    Shader frontfaceShader("shaders/frontfaceShader.vs", "shaders/frontfaceShader.fs");

    // Models
    loadModels();

    // Camera
    setupCamera();

    // IMGUI 
    ImGuiSetup(window);

    // Skyboxes
    setupSkybox(&graffitiSkyboxVAO, &graffitiCubemapTexture, "graffiti_cubemap");
    setupSkybox(&nightSkyboxVAO, &nightCubemapTexture, "nightsky_cubemap");
    setupSkybox(&museumSkyboxVAO, &museumCubemapTexture, "museum_cubemap");

    // Backface framebuffer components
    glGenFramebuffers(1, &backfaceFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, backfaceFBO);

    // Backface normals RGBA texture
    glGenTextures(1, &backfaceNormalTex);
    glBindTexture(GL_TEXTURE_2D, backfaceNormalTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCREEN_WIDTH, SCREEN_HEIGHT, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, backfaceNormalTex, 0);

    // Backface depth buffer texture
    glGenTextures(1, &backfaceDepthTex);
    glBindTexture(GL_TEXTURE_2D, backfaceDepthTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, SCREEN_WIDTH, SCREEN_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, backfaceDepthTex, 0);

    // Tell OpenGL which color attachments
    GLenum drawBuffers[1] = { GL_COLOR_ATTACHMENT0 };
    glDrawBuffers(1, drawBuffers);

    // Check completeness
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cerr << "ERROR::FRAMEBUFFER:: Backface FBO is not complete!" << std::endl;

    // Unbind when done
    glBindFramebuffer(GL_FRAMEBUFFER, 0); 

    // Render loop
    while (!glfwWindowShouldClose(window))
    {
        // Clear screen colour and buffers
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Per-frame time logic
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - prevFrame;
        elapsedTime += deltaTime;
        prevFrame = currentFrame;

        // Rotate the model slowly about the y-axis
        if (spinModel)
        {
            rotY += 20.0f * deltaTime;
            rotY = fmodf(rotY, 360.0f);
        }

        // User input handling
        processUserInput(window);

        // Setup IMGUI frame
        if (!fpsTracker.active)
            ImGuiNewFrame();

        // View and projection
        if (zoomIn)
            camera.position = glm::vec3(0.0f, 0.0f, 4.0f);
        else
            camera.position = glm::vec3(0.0f, 0.0f, 5.0f);

        glm::mat4 view = camera.getViewMatrix();
        glm::mat4 projection = glm::perspective(glm::radians(camera.zoom),
            static_cast<float>(SCREEN_WIDTH) / static_cast<float>(SCREEN_HEIGHT), 
            0.1f, 1000.0f);

        // Update FPS tracker
        if (fpsTracker.active)
            fpsTracker.update(deltaTime);

        // Skybox
        drawSkyBox(skyboxShader, projection, view);

        // Draw model
        switch (selectedRefractionMethod)
        {
        case OneSurface:
            // Draw with 1-surface refraction shader
            drawModel(refractionShader, projection, view, OneSurfaceShader);
            break;

        case TwoSurfaces:
            // First pass: backface rendering
            glBindFramebuffer(GL_FRAMEBUFFER, backfaceFBO);
            glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            glEnable(GL_CULL_FACE);
            glCullFace(GL_FRONT); // Render backfaces only

            drawModel(backfaceShader, projection, view, TwoSurfacesBackFaceShader); // Renders backface normals + depth

            glCullFace(GL_BACK); // Reset culling
            glBindFramebuffer(GL_FRAMEBUFFER, 0);

            // Bind the textures to the expected units
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, backfaceNormalTex);

            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, backfaceDepthTex);

            // Second pass: main rendering using backface data
            drawModel(frontfaceShader, projection, view, TwoSurfacesFrontFaceShader);
            break;

        default:
            // Fallback — just draw basic model
            drawModel(refractionShader, projection, view, OneSurfaceShader);
            break;
        }

        // If screenshot
        if (takeScreenshot)
        {
            std::string refractType = "_1_surface";
            if (selectedRefractionMethod == TwoSurfaces)
            {
                if (screenSpaceOnly)
                    refractType = "_2_surfaces_dv_only";
                else
                    refractType = "_2_surfaces_dn_dv";
            }
            std::ostringstream oss; // Requires C++17
            oss << std::fixed << std::setprecision(3) << IOR;
            std::string IOR_3dp = oss.str();
            std::string fileName = std::string(modelOptions[selectedModel]) + "_"
                + std::string(skyboxOptions[selectedSkybox]) + "_"
                + std::string("IOR_") + IOR_3dp + refractType + std::string(".png");
            saveScreenshot(fileName, SCREEN_WIDTH, SCREEN_HEIGHT);
            takeScreenshot = false;
        }

        // IMGUI drawing
        if (!fpsTracker.active)
            ImGuiDrawWindow();

        // Swap buffers and poll events
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Shutdown procedure
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    // Destroy window
    glfwDestroyWindow(window);

    // Terminate and return success
    glfwTerminate();
    return 0;
}

// Process keyboard inputs
bool IKeyReleased = true;
void processUserInput(GLFWwindow* window)
{
    // Escape to exit
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    // WASD to move, parse to camera processing commands
    // Positional constraints implemented in camera class
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.processKeyboardInput(GLFW_KEY_W, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.processKeyboardInput(GLFW_KEY_A, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.processKeyboardInput(GLFW_KEY_S, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.processKeyboardInput(GLFW_KEY_D, deltaTime);

    // QE for up/down
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
        camera.processKeyboardInput(GLFW_KEY_Q, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
        camera.processKeyboardInput(GLFW_KEY_E, deltaTime);

    // Reset 
    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS)
    {
        rotY = 0.0f;
        camera.position = glm::vec3(xPosInit, yPosInit, zPosInit);
        camera.setMouseSensitivity(mouseSensitivity);
        camera.setCameraMovementSpeed(cameraSpeed);
        camera.setZoom(cameraZoom);
    }

    // Change mouse control between ImGUI and OpenGL
    if (glfwGetKey(window, GLFW_KEY_I) == GLFW_PRESS && IKeyReleased)
    {
        IKeyReleased = false;

        // Give control to ImGUI
        if (!ImGuiUseMouse)
        {
            ImGuiUseMouse = true;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL); // Show cursor
        }
        // Give control to OpenGL
        else
        {
            ImGuiUseMouse = false;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED); // Hide cursor
        }
    }

    // Debouncer for I key
    if (glfwGetKey(window, GLFW_KEY_I) == GLFW_RELEASE)
        IKeyReleased = true;
}

// Window size change callback
void frameBufferSizeCallback(GLFWwindow* window, int width, int height)
{
    // Prevent zero dimension viewport
    if (width == 0 || height == 0)
        return;

    // Ensure viewport matches new window dimensions
    glViewport(0, 0, width, height);

    // Adjust screen width and height params that set the aspect ratio in the projection matrix
    SCREEN_WIDTH = width;
    SCREEN_HEIGHT = height;
}

// Mouse input callback
void mouseCallback(GLFWwindow* window, double xIn, double yIn)
{
    // If using ImGUI
    if (ImGuiUseMouse)
    {
        xPrev = static_cast<float>(xIn);
        yPrev = static_cast<float>(yIn);
        return;
    }

    // Cast doubles to floats
    float x = static_cast<float>(xIn);
    float y = static_cast<float>(yIn);

    // Check if first time this callback function is being used, set last variables if so
    if (firstMouse)
    {
        xPrev = x;
        yPrev = y;
        firstMouse = false;
    }

    // Compute offsets relative to last positions
    float xOff = x - xPrev;
    // Reverse since y-coordinates are inverted (bottom to top)
    float yOff = yPrev - y; 
    xPrev = x; yPrev = y;

    // Tell camera to process new mouse offsets
    camera.processMouseMovement(xOff, yOff);
}

// Mouse scroll wheel input callback - camera zoom must be enabled for this to work
void scrollCallback(GLFWwindow* window, double xOff, double yOff)
{
    // Tell camera to process new y-offset from mouse scroll whell
    camera.processMouseScroll(static_cast<float>(yOff));
}