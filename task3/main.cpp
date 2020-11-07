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
#include <glm/gtx/transform.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

// stb image lib
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// tinyobjloader lib
#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#include "opengl_shader.h"

#define SZ(obj) int((obj).size())

static void glfw_error_callback(int error, const char *description) {
    std::cerr << fmt::format("Glfw Error {}: {}\n", error, description);
}

class OpenGL {
private:
    GLFWwindow* window;

    std::function<void(double, double)> on_scroll = [&](double a, double b) {};
    std::function<void(int, int, int)> on_mouse_button = [&](int a, int b, int c) {};
    //std::function<void(int, int, int, int)> on_key_event = [&](int a, int b, int c, int d) {};

    static void impl_scroll_call(GLFWwindow* window, double xoffset, double yoffset) {
        OpenGL* ths = (OpenGL*)glfwGetWindowUserPointer(window);

        ths->on_scroll(xoffset, yoffset);
    }

    static void impl_mouse_button_call(GLFWwindow* window, int a, int b, int c) {
        OpenGL* ths = (OpenGL*)glfwGetWindowUserPointer(window);

        ths->on_mouse_button(a, b, c);
    }

    // static void impl_on_key_event(GLFWwindow* window, int key, int scancode, int action, int mods) {
    //     OpenGL* ths = (OpenGL*)glfwGetWindowUserPointer(window);

    //     ths->on_key_event(key, scancode, mods);
    // }
    
public:
    OpenGL(const char* window_name) {
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
        window = glfwCreateWindow(1280, 720, window_name, NULL, NULL);
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

        glEnable(GL_DEPTH_TEST);
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
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            
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

    double width_over_height() {
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);

        return double(display_w) / display_h;
    }

    int get_width() const {
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);

        return display_w;
    }

    int get_height() const {
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);

        return display_h;
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
    void set_on_scroll(Call call) {
        on_scroll = call;
    }

    template <typename Call>
    void set_on_mouse_button(Call call) {
        on_mouse_button = call;
    }

    // template <typename Call>
    // void set_on_key_event(Call call) {
    //     on_key_event = call;
    // }
};


class Camera {
public:
    glm::vec3 position = {0, 2300, 0};
    double ang_xz = 0, ang_y = 0;

    glm::vec3 get_forward() {
        return glm::rotateY(glm::rotateX(glm::vec3 {0, 0, -1}, (float)ang_y),
                            (float)ang_xz);
    }

    glm::vec3 get_up() {
        auto forward = get_forward();
        auto up = glm::vec3 {0, 1, 0};
        up = glm::normalize(up - forward * dot(up, forward));
        return up;
    }

    glm::vec3 get_right() {
        auto forward = get_forward();
        auto up = get_up();
        return cross(forward, up);
    }

};

class Texture {
private:
    int width, height;
    GLuint texture;
    
public:
    Texture(const Texture& other) = delete;
    Texture& operator=(const Texture& other) = delete;
    Texture(Texture&& other) = default;
    Texture& operator=(Texture&& other) = default;
    
    Texture(const char* path) {
        stbi_set_flip_vertically_on_load(true);
        int comps;
        
        unsigned char* data = stbi_load(path, &width, &height, &comps, STBI_rgb);

        if (not data)
            throw std::runtime_error(std::string("failed to load texture ") + path);
        
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
        
        glGenerateMipmap(GL_TEXTURE_2D);
        
        stbi_image_free(data);
    }

    GLuint get() const {
        return texture;
    }

    void bind(GLuint slot = GL_TEXTURE0) {
        glActiveTexture(slot);
        glBindTexture(GL_TEXTURE_2D, texture);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }

    static void unbind(GLuint slot=GL_TEXTURE0) {
        glActiveTexture(slot);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
};


class ModelBase {
protected:
    virtual void render_mvp(glm::mat4 mvp) {
    }
    
public:
    virtual glm::mat4 model_matrix() {
        return glm::mat4(1);
    }

    // projection * view
    virtual void render(glm::mat4 vp_matrix) {
        render_mvp(vp_matrix * model_matrix());
    }
};

class HeightMap: public ModelBase {
private:
    shader_t shader = std::move(shader_t("obj-shader.vs", "obj-shader.fs"));

    float u_base_color_weight = 0.2, u_refract_coeff = 1.5;
    int u_is_schlick;
    
    GLuint vbo, vao, ebo;
    int num_triangles = 0;

    Camera& camera;
    
protected:
    virtual void render_mvp(glm::mat4 mvp) {
        shader.use();
        shader.set_uniform("u_mvp", glm::value_ptr(mvp));
        shader.set_uniformv("u_color", glm::vec4 {0.0f, 0.8f, 0.1f, 1.0f});
        shader.set_uniformv("u_sun_direction", glm::normalize(glm::vec3 {0.1f, 0.8f, 0.1f}));
        shader.set_uniform("u_light_ambient", 0.2f);
        shader.set_uniform("u_light_diffuse", 0.8f);
        shader.set_uniformv("u_camera", camera.position);
        shader.set_uniform("u_water_level", 45.f);
        shader.set_uniformv("u_water_color", glm::vec4 {0.3f, 0.3f, 1.0f, 1.0f});

        glBindVertexArray(vao);
        glDrawElements(GL_TRIANGLES, num_triangles * 3, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }

public:
    HeightMap(const char* path, Camera& camera): camera(camera) {
        std::vector<float> vertices;
        std::vector<unsigned int> triangle_indices;
        auto add_triangle = [&](glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3 n) {            
            triangle_indices.push_back(SZ(vertices) / 6 + 0);
            triangle_indices.push_back(SZ(vertices) / 6 + 1);
            triangle_indices.push_back(SZ(vertices) / 6 + 2);
            for (glm::vec3 v: {a, n, b, n, c, n}) {
                vertices.push_back(v.x);
                vertices.push_back(v.y);
                vertices.push_back(v.z);
            }
        };

        
        stbi_set_flip_vertically_on_load(true);
        int comps;
        int width, height;
        
        unsigned short* data = stbi_load_16(path, &width, &height, &comps, STBI_grey);
        //std::fill(data, data + width * height, 0);
        
        if (not data)
            throw std::runtime_error(std::string("failed to load texture ") + path);

        const double scale = 1000;
        for (int i = 1; i < height; ++i)
            for (int j = 1; j < width; ++j) {
                glm::vec3 p00 = glm::vec3 {i - 1, data[(i - 1) * width + j - 1], j - 1};
                glm::vec3 p01 = glm::vec3 {i - 1, data[(i - 1) * width + j    ], j};
                glm::vec3 p10 = glm::vec3 {i    , data[(i    ) * width + j - 1], j - 1};
                glm::vec3 p11 = glm::vec3 {i    , data[(i    ) * width + j    ], j};

                p00.x -= height / 2; p00.z -= width / 2; p00.x *= scale; p00.z *= scale;
                p01.x -= height / 2; p01.z -= width / 2; p01.x *= scale; p01.z *= scale;
                p10.x -= height / 2; p10.z -= width / 2; p10.x *= scale; p10.z *= scale;
                p11.x -= height / 2; p11.z -= width / 2; p11.x *= scale; p11.z *= scale;
                
                add_triangle(p00, p01, p10, -cross(p01 - p00, p10 - p00));
                add_triangle(p11, p01, p10, cross(p01 - p11, p10 - p11));
            }
        
        stbi_image_free(data);

        num_triangles = SZ(triangle_indices) / 3;
        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
        glGenBuffers(1, &ebo);

        glBindVertexArray(vao);

        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices[0]) * vertices.size(), vertices.data(), GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(triangle_indices[0]) * triangle_indices.size(), triangle_indices.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }

    void set_light(float u_base_color_weight, float u_refract_coeff, int u_is_schlick) {
        this->u_base_color_weight = u_base_color_weight;
        this->u_refract_coeff = u_refract_coeff;
        this->u_is_schlick = u_is_schlick;
    }

    glm::mat4 model_matrix() {
        return glm::scale(glm::mat4(1), glm::vec3(0.2f));
    }
};

int main(int, char **) {
    OpenGL opengl("Task3");
    Camera camera;
    HeightMap heightmap("heightmap.png", camera);

    bool is_dragged = false;
    double mouse_x, mouse_y;
    
    opengl.set_on_mouse_button([&](int button, int action, int mods) {
        if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
            is_dragged = true;

            opengl.get_mouse_coordinates(mouse_x, mouse_y);
        }

        if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE) {
            is_dragged = false;
        }
    });

    auto process_drag = [&]() {
        if (not is_dragged)
            return;

        auto oldx = mouse_x;
        auto oldy = mouse_y;
        opengl.get_mouse_coordinates(mouse_x, mouse_y);

        double factor = 2000 / std::max(1, std::min(opengl.get_width(), opengl.get_height()));
        camera.ang_xz -= factor * (mouse_x - oldx);
        camera.ang_y += factor * (mouse_y - oldy);

        camera.ang_y = std::min(camera.ang_y, glm::pi<double>() * 0.35);
        camera.ang_y = std::max(camera.ang_y, -glm::pi<double>() * 0.35);
    };

    float speed = 10;
    
    opengl.main_loop([&]() {
        process_drag();

        glm::vec3 forward = camera.get_forward();
        glm::vec3 up = camera.get_up();
        glm::vec3 right = camera.get_right();

        auto window = opengl.get_window();        
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            camera.position += speed * forward;
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            camera.position -= speed * forward;
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            camera.position += speed * right;
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            camera.position -= speed * right;
        if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
            camera.position += speed * up;
        if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS)
            camera.position -= speed * up;
            
        auto view = glm::lookAt(camera.position, camera.position + forward, up);
        auto projection = glm::perspective<float>(70, opengl.width_over_height(), 10, 100000);
        auto vp = projection * view;

        heightmap.render(vp);

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("Coordinates");
        ImGui::Text("x=%0.2f, y=%0.2f, z=%0.2f", camera.position.x, camera.position.y, camera.position.z);
        ImGui::Text("up");
        ImGui::Text("x=%0.2f, y=%0.2f, z=%0.2f", up.x, up.y, up.z);
        ImGui::Text("forward");
        ImGui::Text("x=%0.2f, y=%0.2f, z=%0.2f", forward.x, forward.y, forward.z);
        ImGui::SliderFloat("speed", &speed, 0.1, 100, "%0.2f", 2.0f);
        ImGui::End();

        // Generate gui render commands
        ImGui::Render();

        // Execute gui render commands using OpenGL backend
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    });

    return 0;
}
