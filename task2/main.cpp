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

class CubemapTexture {
private:
    GLuint texture;

public:
    CubemapTexture(std::vector<std::string> faces) {
        stbi_set_flip_vertically_on_load(false);

        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_CUBE_MAP, texture);

        for (int i = 0; i < 6; i++) {
            int width, height, nrChannels;

            unsigned char *data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);

            if (not data)
                throw std::runtime_error(std::string("failed to load cubemap texture ") + faces[i].c_str());

            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
        }

    }

    void bind(GLuint slot = GL_TEXTURE0) {
        glActiveTexture(slot);
        glBindTexture(GL_TEXTURE_CUBE_MAP, texture);

        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    }

    static void unbind(GLuint slot=GL_TEXTURE0) {
        glActiveTexture(slot);
        glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
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

class ObjModel: public ModelBase {
private:
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;

    shader_t shader = std::move(shader_t("obj-shader.vs", "obj-shader.fs"));
    Texture texture = std::move(Texture("checkers.jpg"));
    CubemapTexture skybox;
    glm::vec3 camera;
    float u_base_color_weight = 0.2, u_refract_coeff = 1.5;

    GLuint vbo, vao, ebo;
    int num_triangles = 0;

    void init_opengl_objects() {
        for (int tp = 0; tp < 3; ++tp) {
            float mn = 1e9, mx = -1e9;

            for (int i = tp; i < attrib.vertices.size(); i += 3) {
                mn = std::min(mn, attrib.vertices[i]);
                mx = std::max(mx, attrib.vertices[i]);
            }

            float mid = mn + (mx - mn) / 2;
            for (int i = tp; i < attrib.vertices.size(); i += 3) {
                attrib.vertices[i] -= mid;
            }
        }

        std::vector<float> vertices;
        std::vector<unsigned int> triangle_indices;

        std::map<std::pair<int, int>, int> idmap;
        auto get_vertex_id = [&](int vertex, int normal) {
            if (not idmap.count(std::make_pair(vertex, normal))) {
                int new_id = idmap.size();
                idmap[std::make_pair(vertex, normal)] = new_id;

                vertices.push_back(attrib.vertices[3 * vertex]);
                vertices.push_back(attrib.vertices[3 * vertex + 1]);
                vertices.push_back(attrib.vertices[3 * vertex + 2]);

                vertices.push_back(attrib.normals[3 * normal]);
                vertices.push_back(attrib.normals[3 * normal + 1]);
                vertices.push_back(attrib.normals[3 * normal + 2]);
            }

            return idmap[std::make_pair(vertex, normal)];
        };

        for (int s = 0; s < shapes.size(); ++s) {
            assert(shapes[s].mesh.indices.size() % 3 == 0);

            for (int v = 0; v < shapes[s].mesh.indices.size(); ++v) {
                tinyobj::index_t idx = shapes[s].mesh.indices[v];
                triangle_indices.push_back(get_vertex_id(idx.vertex_index, idx.normal_index));
            }
        }

        num_triangles = triangle_indices.size() / 3;

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
    
protected:
    virtual void render_mvp(glm::mat4 mvp) {
        shader.use();
        shader.set_uniform("u_mvp", glm::value_ptr(mvp));
        shader.set_uniform("u_tex", int(0));
        shader.set_uniformv("u_color", glm::vec4 {0.8f, 0.8f, 0.f, 1.0f});
        shader.set_uniformv("u_camera", camera);
        shader.set_uniform("u_base_color_weight", u_base_color_weight);
        shader.set_uniform("u_refract_coeff", u_refract_coeff);
        skybox.bind();

        glBindVertexArray(vao);
        glDrawElements(GL_TRIANGLES, num_triangles * 3, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);

        skybox.unbind();
    }

public:
    ObjModel(const char* filename, CubemapTexture& skybox): skybox(skybox) {
        std::string err;
        bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &err, filename);
        
        if (!err.empty())
            fprintf(stderr, "loading %s, error: %s\n", filename, err.c_str());
        
        if (!ret) {
            fprintf(stderr, "failed to load %s\n", filename);
            throw std::runtime_error(std::string("failed to load ") + filename);
        }

        init_opengl_objects();
    }

    void set_camera(glm::vec3 camera) {
        this->camera = camera;
    }

    void set_light(float u_base_color_weight, float u_refract_coeff) {
        this->u_base_color_weight = u_base_color_weight;
        this->u_refract_coeff = u_refract_coeff;
    }
};

class Skybox: public ModelBase {
private:
    GLuint vbo, ebo, vao;

    shader_t shader;
    CubemapTexture& cubemap;
    int num_triangles;

public:
    Skybox(CubemapTexture &cubemap) : shader("skybox-shader.vs", "skybox-shader.fs"), cubemap(cubemap) {
        float vertices[] = {
                -1, -1, -1,
                -1, -1, +1,

                -1, +1, -1,
                -1, +1, +1,

                +1, -1, -1,
                +1, -1, +1,

                +1, +1, -1,
                +1, +1, +1,
        };

#define QUAD(a, b, d, c) a,b,c,c,d,a
        unsigned int indices[] = {QUAD(0,1,2,3), QUAD(4,5,6,7),
                                  QUAD(0,1,4,5), QUAD(2,3,6,7),
                                  QUAD(0,2,4,6), QUAD(1,3,5,7)};
#undef QUAD

        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
        glGenBuffers(1, &ebo);

        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

        num_triangles = sizeof(indices) / sizeof(indices[0]) / 3;

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *) 0);
        glEnableVertexAttribArray(0);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }
protected:
    virtual void render_mvp(glm::mat4 mvp) {
        shader.use();
        shader.set_uniform("u_mvp", glm::value_ptr(mvp));
        shader.set_uniform("u_tex", int(0));
        cubemap.bind();

        glBindVertexArray(vao);
        glDrawElements(GL_TRIANGLES, num_triangles * 3, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);

        cubemap.unbind();
    }
};

int main(int, char **) {
    OpenGL opengl("Task2");
    CubemapTexture cubemap(std::vector<std::string> {"skybox/right.jpg",
                                                     "skybox/left.jpg",
                                                     "skybox/top.jpg",
                                                     "skybox/bottom.jpg",
                                                     "skybox/front.jpg",
                                                     "skybox/back.jpg"});

    ObjModel model("piper_pa18.obj", cubemap);
    Skybox skybox(cubemap);

    double ang_xz = 0, ang_y = 0, distance = 6;

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

        ang_xz -= factor * (mouse_x - oldx);
        ang_y += factor * (mouse_y - oldy);

        ang_y = std::min(ang_y, glm::pi<double>() * 0.35);
        ang_y = std::max(ang_y, -glm::pi<double>() * 0.35);
    };

    opengl.set_on_scroll([&](double xoffset, double yoffset) {
        distance -= yoffset / 3;
        distance = std::min(distance, 10.);
        distance = std::max(distance, 3.);
    });

    float u_base_color_weight = 0.2;
    float u_refract_coeff = 1.5;
    
    opengl.main_loop([&]() {
        process_drag();

        auto camera = glm::rotate<float>(glm::rotate<float>(glm::vec3 {0, 0, distance}, ang_y, glm::vec3 {1, 0, 0}),
                                         ang_xz, glm::vec3 {0, 1, 0});

        auto view = glm::lookAt(camera, glm::vec3 {0,0,0}, glm::vec3 {0, 1, 0} + camera * (camera * glm::vec3 {0,1,0}));
        auto projection = glm::perspective<float>(90, opengl.width_over_height(), 0.1, 100);
        auto vp = projection * view;

        //glDisable(GL_DEPTH_TEST);
        glDepthMask(GL_FALSE);
        skybox.render(projection * glm::lookAt(glm::vec3 {0,0,0}, -camera, glm::vec3 {0, 1, 0} + camera * (camera * glm::vec3 {0,1,0})));
        glDepthMask(GL_TRUE);
        //glEnable(GL_DEPTH_TEST);

        model.set_camera(camera);
        model.render(vp);

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("Lights");
        ImGui::SliderFloat("basecolor", &u_base_color_weight, 0, 1);
        ImGui::SliderFloat("refract_coeff", &u_refract_coeff, 1, 2);
        ImGui::End();

        // Generate gui render commands
        ImGui::Render();

        // Execute gui render commands using OpenGL backend
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        model.set_light(u_base_color_weight, u_refract_coeff);
    });

    return 0;
}
