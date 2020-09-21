#include <iostream>
#include <vector>
#include <chrono>

#include <fmt/format.h>

#include <GL/glew.h>

// Imgui + bindings
#include "imgui.h"
#include "bindings/imgui_impl_glfw.h"
#include "bindings/imgui_impl_opengl3.h"

// Include glfw3.h after our OpenGL definitions
#include <GLFW/glfw3.h>

// Math constant and routines for OpenGL interop
#include <glm/gtc/constants.hpp>

#include "opengl_shader.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

static void glfw_error_callback(int error, const char *description) {
    std::cerr << fmt::format("Glfw Error {}: {}\n", error, description);
}

class OpenGL {
private:
    GLFWwindow* window;

    std::function<void(double, double)> on_scroll = [&](double a, double b) {};
    std::function<void(int, int, int)> on_mouse_button = [&](int a, int b, int c) {};

    static void impl_scroll_call(GLFWwindow* window, double xoffset, double yoffset) {
        OpenGL* ths = (OpenGL*)glfwGetWindowUserPointer(window);

        ths->on_scroll(xoffset, yoffset);
    }

    static void impl_mouse_button_call(GLFWwindow* window, int a, int b, int c) {
        OpenGL* ths = (OpenGL*)glfwGetWindowUserPointer(window);

        ths->on_mouse_button(a, b, c);
    }
    
public:
    OpenGL() {
        // Use GLFW to create a simple window
        glfwSetErrorCallback(glfw_error_callback);
        if (!glfwInit())
            throw std::runtime_error("glfwInit failed");

        // GL 3.3 + GLSL 330
        const char *glsl_version = "#version 330";
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // 3.0+ only

        // Create window with graphics context
        window = glfwCreateWindow(1280, 720, "Fractal", NULL, NULL);
        if (window == NULL)
            throw std::runtime_error("Failed to create window");
   
        glfwMakeContextCurrent(window);
        glfwSwapInterval(1); // Enable vsync

        // Initialize GLEW, i.e. fill all possible function pointers for current OpenGL context
        if (glewInit() != GLEW_OK)
            throw std::runtime_error("Failed to initialize GLEW!\n");

        // Setup GUI context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO &io = ImGui::GetIO();
        ImGui_ImplGlfw_InitForOpenGL(window, true);
        ImGui_ImplOpenGL3_Init(glsl_version);
        ImGui::StyleColorsDark();

        // save this for callbacks
        glfwSetWindowUserPointer(window, this);
        glfwSetScrollCallback(window, impl_scroll_call);
        glfwSetMouseButtonCallback(window, impl_mouse_button_call);
    }

    OpenGL(const OpenGL& other) = delete;
    OpenGL& operator=(const OpenGL& other) = delete;
    
    ~OpenGL() {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        
        glfwDestroyWindow(window);
        glfwTerminate();
    }

    template <typename Call>
    void main_loop(Call call) {
        while (not glfwWindowShouldClose(window)) {
            glfwPollEvents();
            // Get windows size
            int display_w, display_h;
            glfwGetFramebufferSize(window, &display_w, &display_h);

            // Set viewport to fill the whole window area
            glViewport(0, 0, display_w, display_h);

            // Fill background with solid color
            glClearColor(0.30f, 0.55f, 0.60f, 1.00f);
            glClear(GL_COLOR_BUFFER_BIT);
            
            call();

            // Swap the backbuffer with the frontbuffer that is used for screen display
            glfwSwapBuffers(window);
        }
    }

    // y over x
    double aspect_ratio() {
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);

        return double(display_h) / display_w;
    }

    void get_mouse_coordinates(double& x, double& y) {
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);

        glfwGetCursorPos(window, &x, &y);

        x /= double(display_w);
        y /= double(display_h);

        x = 2 * x - 1;
        y = 1 - 2 * y;
    }

    GLFWwindow* get_window() {
        return window;
    }

    template <typename Call>
    void set_on_scrool(Call call) {
        on_scroll = call;
    }

    template <typename Call>
    void set_on_mouse_button(Call call) {
        on_mouse_button = call;
    }
};

class Texture {
private:
    int width, height;
    GLuint texture;
    
public:
    Texture(const Texture& other) = delete;
    Texture& operator=(const Texture& other) = delete;
    
    Texture(const char* path) {
        stbi_set_flip_vertically_on_load(true);
        int comps;
        
        float *data = stbi_loadf(path, &width, &height, &comps, 3);

        if (not data)
            throw std::runtime_error("failed to load");
        
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, data); 
        
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }

    int get_width() const {
        return width;
    }

    int get_height() const {
        return height;
    }

    GLuint get() const {
        return texture;
    }
};

class Triangle {
private:
    GLuint vbo, vao, ebo;

public:
    shader_t triangle_shader;
    
public:
    Triangle(): triangle_shader("simple-shader.vs", "simple-shader.fs") {
        // create the triangle
        float triangle_vertices[] = {
            0.0f, 0.25f, 0.0f,	// position vertex 1
            1.0f, 0.0f, 0.0f,	 // color vertex 1

            0.25f, -0.25f, 0.0f,  // position vertex 1
            0.0f, 1.0f, 0.0f,	 // color vertex 1

            -0.25f, -0.25f, 0.0f, // position vertex 1
            0.0f, 0.0f, 1.0f,	 // color vertex 1
        };
        unsigned int triangle_indices[] = {
            0, 1, 2 };
        
        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
        glGenBuffers(1, &ebo);
        
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        
        glBufferData(GL_ARRAY_BUFFER, sizeof(triangle_vertices), triangle_vertices, GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(triangle_indices), triangle_indices, GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }

    void draw() {
        // Bind triangle shader
        triangle_shader.use();
        // Bind vertex array = buffers + indices
        glBindVertexArray(vao);
        // Execute draw call
        glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);        
    }
};

class Fractal {
private:
    GLuint vbo, ebo, vao;

    shader_t fractal_shader;

    float R = 2, scale = 1, aspect_ratio = 1;
    float coordinates[2] = {0, 0};
    float cvec[2] = {0, 0};
    int num_it = 1;

    const Texture& texture;
    
public:
    Fractal(const Texture& texture): fractal_shader("frac-shader.vs", "frac-shader.fs"), texture(texture) {
        float vertices[] = {
            -1, -1, 0, // left-btm
            -1, +1, 0,
            +1, +1, 0,
            +1, -1, 0
        };

        unsigned int indices[] = {0, 1, 2, 2, 3, 0};
        
        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
        glGenBuffers(1, &ebo);
        
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
        
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
        glEnableVertexAttribArray(0);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }

    void draw() {
        fractal_shader.set_uniform("u_translation", coordinates[0], coordinates[1]);
        fractal_shader.set_uniform("u_scale", scale);
        fractal_shader.set_uniform("u_aspect_ratio", aspect_ratio);

        fractal_shader.set_uniform("u_cvec", cvec[0], cvec[1]);
        fractal_shader.set_uniform("u_R", R);
        fractal_shader.set_uniform("u_num_it", num_it);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture.get());
        
        fractal_shader.use();
        glBindVertexArray(vao);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }

    void setPosition(float x0, float y0, float scale, float aspect_ratio) {
        coordinates[0] = x0;
        coordinates[1] = y0;

        this->scale = scale;
        this->aspect_ratio = aspect_ratio;
    }

    void setParameters(float c_real, float c_imag, float r, int num_it) {
        cvec[0] = c_real;
        cvec[1] = c_imag;
        R = r;
        this->num_it = num_it;
    }
};

int main(int, char **) {
    OpenGL opengl;
    // Triangle triangle;
    Texture gradient("grad.png");    
    Fractal fractal(gradient);
    
    // GUI
    static float translation[] = { 0.0, 0.0 };
    static float cvec[] = {0.069, -0.644};
    static float R = 0.178, scale = 0.5;
    static int numiter = 35;
    
    auto get_world_coordinates_under_pointer = [&](double& x, double& y) {
        opengl.get_mouse_coordinates(x, y);

        x /= scale, y /= scale / opengl.aspect_ratio();
        x += translation[0], y += translation[1];
    };

    bool is_dragged = false;
    double drag_point_x, drag_point_y;
    
    opengl.set_on_mouse_button([&](int button, int action, int mods) {
        if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
            is_dragged = true;

            get_world_coordinates_under_pointer(drag_point_x, drag_point_y);
        }

        if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE) {
            is_dragged = false;
        }
    });
    
    opengl.set_on_scrool([&](double a, double b) {
        double x, y;
        get_world_coordinates_under_pointer(x, y);
        
        if (b > 0)
            scale = 1.05 * scale;
        else if (b < 0)
            scale = scale / 1.05;

        scale = std::max(scale, 0.1f);
        scale = std::min(scale, 10.0f);
        
        double newx, newy;
        get_world_coordinates_under_pointer(newx, newy);

        translation[0] += (x - newx);
        translation[1] += (y - newy);
    });

    
    opengl.main_loop([&]() {
        if (is_dragged) {
            double cur_x, cur_y;
            get_world_coordinates_under_pointer(cur_x, cur_y);
            
            translation[0] += drag_point_x - cur_x;
            translation[1] += drag_point_y - cur_y;
        }
        
        fractal.setPosition(translation[0], translation[1], scale, opengl.aspect_ratio());
        fractal.setParameters(cvec[0], cvec[1], R, numiter);
        fractal.draw();
        // triangle.draw();
        
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        
        ImGui::Begin("Fractal");        
        ImGui::SliderFloat2("position", translation, -5.0, 5.0);
        ImGui::SliderFloat("scale", &scale, 0.1, 10);
        ImGui::SliderFloat2("c", cvec, -2, 2);
        ImGui::SliderFloat("R", &R, 0, 2);
        ImGui::SliderInt("numit", &numiter, 1, 100);
        ImGui::End();
        
        // Generate gui render commands
        ImGui::Render();

        // Execute gui render commands using OpenGL backend
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    });
   
    return 0;
}
