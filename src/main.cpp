#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <iostream>
#include <vector>
#include <cmath>

#define USE_ASM_RENDERER

extern "C" void newton_asm(
    int width, int height, float* rgb_out,
    float centerX, float centerY, float zoom, int maxIter
);

void newton_cpp(
    int width, int height, float* rgb_out,
    float centerX, float centerY, float zoom, int maxIter
);

#ifdef USE_ASM_RENDERER
    #define NEWTON_RENDERER newton_asm
#else
    #define NEWTON_RENDERER newton_cpp
#endif

// ---------- struktura parametrów widoku ----------
struct ViewParams {
    float centerX = 0.0f;
    float centerY = 0.0f;
    float zoom     = 1.0f;
    int   maxIter  = 50;
};


constexpr int WIN_W = 800;
constexpr int WIN_H = 800;


static std::vector<float> framebuffer(WIN_W * WIN_H * 3);
static ViewParams* g_view = nullptr;
static bool dirty = true;

static bool   mousePressed = false;
static double lastMouseX   = 0.0;
static double lastMouseY   = 0.0;


void scrollCallback(GLFWwindow*, double, double);
void mouseButtonCallback(GLFWwindow*, int, int, int);
void cursorPosCallback(GLFWwindow*, double, double);


bool initGLFW(GLFWwindow*& window);
bool initGLAD();
void initImGui(GLFWwindow* window);
GLuint compileShader(GLenum type, const char* src);
void initOpenGLResources(GLuint& shaderProgram, GLuint& vao, GLuint& vbo, GLuint& texture);
void renderFrame(GLuint shaderProgram, GLuint vao, GLuint texture);
void cleanup(GLuint shaderProgram, GLuint vao, GLuint vbo, GLuint texture);


void newton_cpp(
    int width, int height, float* rgb_out,
    float centerX, float centerY, float zoom, int maxIter
) {
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int idx = (y * width + x) * 3;

            float re = centerX + (x - width * 0.5f) / zoom;
            float im = centerY - (y - height * 0.5f) / zoom;

            float zr = re;
            float zi = im;
            int iter = 0;

            for (; iter < maxIter; iter++) {

                // Calculate Z^2
                float zr2 = zr * zr - zi * zi;
                float zi2 = 2.0f * zr * zi;

                // Calculate Z^3
                float zr3 = zr2 * zr - zi2 * zi;
                float zi3 = zr2 * zi + zi2 * zr;

                // F(Z) = Z^3 - 1
                float fr = zr3 - 1.0f;
                float fi = zi3;

                // F`(Z) = 3Z
                float dr = 3.0f * zr2;
                float di = 3.0f * zi2;

                // Denominator Re(f`)^2 + Im(f`)^2
                float denom = dr * dr + di * di;
                if (denom == 0.0f) break;

                // Step: [(ac + bd) + i(bc-ad)]/c^2 + d^2
                float tr = (fr * dr + fi * di) / denom;
                float ti = (fi * dr - fr * di) / denom;

                zr -= tr;
                zi -= ti;

                // Break if close enough
                if (fr * fr + fi * fi < 1e-6f) break;
            }

            // Roots
            float r1r = 1.0f, r1i = 0.0f;
            float r2r = -0.5f, r2i = 0.8660254f;
            float r3r = -0.5f, r3i = -0.8660254f;

            // Distances to every root |z-root|
            float d1 = (zr - r1r) * (zr - r1r) + (zi - r1i) * (zi - r1i);
            float d2 = (zr - r2r) * (zr - r2r) + (zi - r2i) * (zi - r2i);
            float d3 = (zr - r3r) * (zr - r3r) + (zi - r3i) * (zi - r3i);

            int root = 0;
            float minD = d1;

            if (d2 < minD) { minD = d2; root = 1; }
            if (d3 < minD) { root = 2; }

            float t = (float)iter / (float)maxIter;
            float baseBrightness[3] = { 1.0f, 0.7f, 0.4f }; // Different brightness for roots
            float gray = baseBrightness[root] * (1.0f - t);

            rgb_out[idx + 0] = gray;
            rgb_out[idx + 1] = 0.6 * gray;
            rgb_out[idx + 2] = gray;
        }
    }
}

int main() {
    GLFWwindow* window = nullptr;
    if (!initGLFW(window)) return -1;
    if (!initGLAD()) return -1;
    initImGui(window);

    ViewParams view;
    g_view = &view;

    glfwSetScrollCallback(window, scrollCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetCursorPosCallback(window, cursorPosCallback);

    GLuint shaderProgram, vao, vbo, texture;
    initOpenGLResources(shaderProgram, vao, vbo, texture);


    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("Settings", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
        if (ImGui::SliderInt("Max Iterations", &view.maxIter, 10, 200)) {
            dirty = true;
        }
        ImGui::End();

        if (dirty) {
            NEWTON_RENDERER(WIN_W, WIN_H, framebuffer.data(),
                           view.centerX, view.centerY, view.zoom, view.maxIter);
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, WIN_W, WIN_H,
                            GL_RGB, GL_FLOAT, framebuffer.data());
            dirty = false;
        }


        renderFrame(shaderProgram, vao, texture);

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    cleanup(shaderProgram, vao, vbo, texture);
    return 0;
}



void scrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
    ImGui_ImplGlfw_ScrollCallback(window, xoffset, yoffset);
    if (ImGui::GetIO().WantCaptureMouse) return;

    double mouseX, mouseY;
    glfwGetCursorPos(window, &mouseX, &mouseY);

    float oldZoom = g_view->zoom;
    float dx = (2.0f * (float)mouseX / WIN_W - 1.0f) / oldZoom;
    float dy = (1.0f - 2.0f * (float)mouseY / WIN_H) / oldZoom;

    if (yoffset > 0)
        g_view->zoom *= 1.1f;
    else if (yoffset < 0)
        g_view->zoom /= 1.1f;

    if (g_view->zoom != oldZoom) {
        g_view->centerX += dx * (1.0f/oldZoom - 1.0f/g_view->zoom);
        g_view->centerY += dy * (1.0f/oldZoom - 1.0f/g_view->zoom);
        dirty = true;
    }
}

void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    ImGui_ImplGlfw_MouseButtonCallback(window, button, action, mods);
    if (ImGui::GetIO().WantCaptureMouse) return;

    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
            mousePressed = true;
            glfwGetCursorPos(window, &lastMouseX, &lastMouseY);
        } else if (action == GLFW_RELEASE) {
            mousePressed = false;
        }
    }
}

void cursorPosCallback(GLFWwindow* window, double xpos, double ypos) {
    if (ImGui::GetIO().WantCaptureMouse) return;
    if (!mousePressed) return;

    double dx = xpos - lastMouseX;
    double dy = ypos - lastMouseY;


    g_view->centerX -= (float)dx / g_view->zoom;
    g_view->centerY -= (float)dy / g_view->zoom;

    lastMouseX = xpos;
    lastMouseY = ypos;
    dirty = true;
}


bool initGLFW(GLFWwindow*& window) {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW\n";
        return false;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    window = glfwCreateWindow(WIN_W, WIN_H, "Newton Fractal (z^3-1)", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window\n";
        glfwTerminate();
        return false;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    return true;
}

bool initGLAD() {
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD\n";
        return false;
    }
    return true;
}

void initImGui(GLFWwindow* window) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(window, false);
    ImGui_ImplOpenGL3_Init("#version 330");
}

GLuint compileShader(GLenum type, const char* src) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, NULL);
    glCompileShader(shader);
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char info[512];
        glGetShaderInfoLog(shader, 512, NULL, info);
        std::cerr << "Shader compile error (" << type << "): " << info << std::endl;
    }
    return shader;
}

void initOpenGLResources(GLuint& shaderProgram, GLuint& vao, GLuint& vbo, GLuint& texture) {
    // Shadery
    const char* vertexSrc = R"(
        #version 330 core
        layout (location = 0) in vec2 aPos;
        layout (location = 1) in vec2 aTexCoord;
        out vec2 TexCoord;
        void main() {
            gl_Position = vec4(aPos, 0.0, 1.0);
            TexCoord = aTexCoord;
        }
    )";
    const char* fragmentSrc = R"(
        #version 330 core
        in vec2 TexCoord;
        out vec4 FragColor;
        uniform sampler2D uTexture;
        void main() {
            FragColor = vec4(texture(uTexture, TexCoord).rgb, 1.0);
        }
    )";

    GLuint vs = compileShader(GL_VERTEX_SHADER, vertexSrc);
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, fragmentSrc);
    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vs);
    glAttachShader(shaderProgram, fs);
    glLinkProgram(shaderProgram);
    glDeleteShader(vs);
    glDeleteShader(fs);

    float quadVertices[] = {
        // pos        // texcoord
        -1.0f,  1.0f,  0.0f, 1.0f,
        -1.0f, -1.0f,  0.0f, 0.0f,
         1.0f, -1.0f,  1.0f, 0.0f,

        -1.0f,  1.0f,  0.0f, 1.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
         1.0f,  1.0f,  1.0f, 1.0f
    };

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);

    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, WIN_W, WIN_H, 0, GL_RGB, GL_FLOAT, nullptr);
}

void renderFrame(GLuint shaderProgram, GLuint vao, GLuint texture) {
    glClear(GL_COLOR_BUFFER_BIT);
    glUseProgram(shaderProgram);
    glBindTexture(GL_TEXTURE_2D, texture);
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

void cleanup(GLuint shaderProgram, GLuint vao, GLuint vbo, GLuint texture) {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glDeleteTextures(1, &texture);
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
    glDeleteProgram(shaderProgram);
    glfwTerminate();
}