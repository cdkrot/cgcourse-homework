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
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "stb_image.h"
#include "tiny_obj_loader.h"
#include "opengl_shader.h"
#include "miniconfig.h"

#define SZ(obj) int((obj).size())

Config config("config.cfg");

static void glfw_error_callback(int error, const char *description) {
    std::cerr << fmt::format("Glfw Error {}: {}\n", error, description);
}

unsigned long millis() {
    static auto millis0 = std::chrono::system_clock::now().time_since_epoch() / std::chrono::milliseconds(1);;
    auto millis_now = std::chrono::system_clock::now().time_since_epoch() / std::chrono::milliseconds(1);;
    return millis_now - millis0;
}

float seconds() {
    return millis() / 1000.0f;
}

GLuint shadowmap_tex; // messy, create a subclass for that later
glm::mat4 last_light_matrix;  // messy, create a subclass for that later

class OpenGL {
private:
    GLFWwindow* window;

    std::function<void(double, double)> on_scroll = [&](double a, double b) {};
    std::function<void(int, int, int)> on_mouse_button = [&](int a, int b, int c) {};
    std::function<void(int, int, int, int)> on_key_event = [&](int a, int b, int c, int d) {};

    static void impl_scroll_call(GLFWwindow* window, double xoffset, double yoffset) {
        OpenGL* ths = (OpenGL*)glfwGetWindowUserPointer(window);

        ths->on_scroll(xoffset, yoffset);
    }

    static void impl_mouse_button_call(GLFWwindow* window, int a, int b, int c) {
        OpenGL* ths = (OpenGL*)glfwGetWindowUserPointer(window);

        ths->on_mouse_button(a, b, c);
    }

    static void impl_on_key_event(GLFWwindow* window, int key, int scancode, int action, int mods) {
        OpenGL* ths = (OpenGL*)glfwGetWindowUserPointer(window);

        ths->on_key_event(key, scancode, action, mods);
    }
    
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
        glfwSetKeyCallback(window, impl_on_key_event);

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

    template <typename Call>
    void set_on_key_event(Call call) {
        on_key_event = call;
    }
};


class Camera {
public:
    glm::vec3 position = config.get_vec("camera");
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

class TrivialModel: public ModelBase {
private:
    shader_t shader;
    
    GLuint vbo, vao, ebo;
    int num_triangles = 2;

    Camera& camera;
    
    void init_opengl_objects() {
        std::vector<float> vertex_data = {
            -1, -1, -1,
            -1, +1, -1,
            +1, +1, -1,

            -1, -1, -1,
            +1, +1, -1,
            +1, -1, -1
        };

        std::vector<int> triangle_indices = {0,1,2,3,4,5};
        
        
        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
        glGenBuffers(1, &ebo);

        glBindVertexArray(vao);

        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertex_data[0]) * vertex_data.size(), vertex_data.data(), GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(triangle_indices[0]) * triangle_indices.size(), triangle_indices.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
        glEnableVertexAttribArray(0);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }
    
protected:
    virtual void render_mvp(glm::mat4 mvp) {        
        shader.use();

        auto vp_matrix_inv = glm::inverse(mvp);

        using std::cout;        
        
        shader.set_uniform("u_vp_inv", glm::value_ptr(vp_matrix_inv));
        shader.set_uniformv("u_camera", camera.position);

        glBindVertexArray(vao);
        glDrawElements(GL_TRIANGLES, num_triangles * 3, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }
    
public:
    TrivialModel(Camera& camera): camera(camera) {
        init_opengl_objects();
        reload_shader();
    }

    void reload_shader() {
        shader = std::move(shader_t("TheShader.vs", "TheShader.fs"));
    }
};

int main(int, char **) {
    OpenGL opengl("Task4");
    Camera camera;
    TrivialModel model(camera);
    
    bool is_dragged = false;
    double mouse_x, mouse_y;    
    
    auto post_cfg_reload = [&]() {
        model.reload_shader();
    };

    post_cfg_reload();
    
    opengl.set_on_mouse_button([&](int button, int action, int mods) {
        if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
            is_dragged = true;

            opengl.get_mouse_coordinates(mouse_x, mouse_y);
        }

        if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE) {
            is_dragged = false;
        }
    });

    opengl.set_on_key_event([&](int key, int scancode, int action, int mods) {
        if (key == GLFW_KEY_R and action == GLFW_PRESS) {
            std::cerr << "Reloading cfg" << std::endl;
            config.reload();
            post_cfg_reload();
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

    float speed = 0.4;

    auto last_time = millis();
    const int averaging_factor = 60;
    int frame_counter = 0;
    
    int FPS = 0;
    double avg_render_time = 0;
    
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

        // step2, normal render
        auto view = glm::lookAt(camera.position, camera.position + forward, up);
        auto projection = glm::perspective<float>(70, opengl.width_over_height(),
                                                  config.get_float("clip_near"),
                                                  config.get_float("clip_far"));
        model.render(projection * view);

        if ((++frame_counter) % averaging_factor == 0) {
            auto old_time = last_time;
            last_time = millis();
            
            avg_render_time = (last_time - old_time) / double(averaging_factor);
            FPS = floor(1000 / avg_render_time);
        }
        
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        
        ImGui::NewFrame();

        ImGui::Begin("Info");
        ImGui::Text("FPS: %d, %0.1f ms per frame", FPS, avg_render_time);
        ImGui::Text("");
        ImGui::Text("Coordinates");
        ImGui::Text("x=%0.2f, y=%0.2f, z=%0.2f", camera.position.x, camera.position.y, camera.position.z);
        ImGui::Text("up");
        ImGui::Text("x=%0.2f, y=%0.2f, z=%0.2f", up.x, up.y, up.z);
        ImGui::Text("forward");
        ImGui::Text("x=%0.2f, y=%0.2f, z=%0.2f", forward.x, forward.y, forward.z);
        ImGui::SliderFloat("speed", &speed, 0.01, 5, "%0.2f", 2.0f);
        ImGui::Text("");
        ImGui::Text("Controls: WASD (forward, left, right, backward)");
        ImGui::Text("Controls: QZ (up, down)");
        ImGui::Text("Controls: R (reload cfg and shaders)");
        ImGui::End();

        // Generate gui render commands
        ImGui::Render();

        // Execute gui render commands using OpenGL backend
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    });

    return 0;
}
