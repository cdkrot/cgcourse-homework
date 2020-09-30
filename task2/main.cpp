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

    double width_over_height() {
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);

        return double(display_w) / display_h;
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
    Texture(Texture&& other) = default;
    Texture& operator=(Texture&& other) = default;
    
    Texture(const char* path) {
        stbi_set_flip_vertically_on_load(true);
        int comps;
        
        unsigned char* data = stbi_load(path, &width, &height, &comps, STBI_rgb);

        if (not data)
            throw std::runtime_error("failed to load");
        
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
        
        glGenerateMipmap(GL_TEXTURE_2D);
        
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

/*
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
*/

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

void create_triangle(GLuint &vbo, GLuint &vao, GLuint &ebo) {
    // create the triangle
    float triangle_vertices[] = {
        -1, 1, 0,	// position vertex 1
        0, 1, 0.0f,	 // color vertex 1

        -1, -1, 0.0f,  // position vertex 1
        0, 0, 0.0f,	 // color vertex 1

        1, -1, 0.0f, // position vertex 1
        1, 0, 0,	 // color vertex 1

        1, 1, 0.0f, // position vertex 1
        1, 1, 0,	 // color vertex 1
    };
    unsigned int triangle_indices[] = {
        0, 1, 2, 0, 3, 2 };
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

class ObjModel: public ModelBase {
private:
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    shader_t shader = std::move(shader_t("simple-shader.fs", "simple-shader.vs"));
    Texture texture = std::move(Texture("checkers.jpg"));

    void init_opengl_objects() {
        
    }
    
protected:
    virtual void render_mvp(glm::mat4 mvp) {
        shader.use();
        shader.set_uniform("u_mvp", glm::value_ptr(mvp));

        shader.set_uniform("u_tex", int(0));
        texture.bind();

        texture.unbind();
    }

public:
    ObjModel(const char* filename) {        
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
};

int main(int, char **) {
    OpenGL opengl("Task2");

    Texture texture("lena.jpg");

    // create our geometries
    GLuint vbo, vao, ebo;
    create_triangle(vbo, vao, ebo);

    // init shader
    // shader_t triangle_shader("simple-shader.vs", "simple-shader.fs");

    //auto const start_time = std::chrono::steady_clock::now();

    ObjModel model("cube.obj");
        
    opengl.main_loop([&]() {
        auto view = glm::lookAt<float>(glm::vec3(0, 0, -1), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
        auto projection = glm::perspective<float>(90, opengl.width_over_height(), 0.1, 100);

        model.render(projection * view);
        
        /*
        // Gui start new frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // GUI
        ImGui::Begin("Triangle Position/Color");
        static float rotation = 0.0;
        ImGui::SliderFloat("rotation", &rotation, 0, 2 * glm::pi<float>());
        static float translation[] = { 0.0, 0.0 };
        ImGui::SliderFloat2("position", translation, -1.0, 1.0);
        static float color[4] = { 1.0f,1.0f,1.0f,1.0f };
        ImGui::ColorEdit3("color", color);
        ImGui::End();

        // Pass the parameters to the shader as uniforms
        triangle_shader.set_uniform("u_rotation", rotation);
        triangle_shader.set_uniform("u_translation", translation[0], translation[1]);
        //float const time_from_start = (float)(std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start_time).count() / 1000.0);
        //triangle_shader.set_uniform("u_time", time_from_start);
        triangle_shader.set_uniform("u_color", color[0], color[1], color[2]);


        auto model = glm::rotate(glm::mat4(1), glm::radians(rotation * 60), glm::vec3(0, 1, 0)) * glm::scale(glm::vec3(2, 2, 2));
        auto view = glm::lookAt<float>(glm::vec3(0, 0, -1), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
        auto projection = glm::perspective<float>(90, opengl.width_over_height(), 0.1, 100);
        auto mvp = projection * view * model;
        triangle_shader.set_uniform("u_mvp", glm::value_ptr(mvp));
        triangle_shader.set_uniform("u_tex", int(0));


        // Bind triangle shader
        triangle_shader.use();
        texture.bind();
        // Bind vertex array = buffers + indices
        glBindVertexArray(vao);
        // Execute draw call
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        texture.unbind();
        glBindVertexArray(0);
        
        ImGui::Render(); // Generate gui render commands
        // Execute gui render commands using OpenGL backend
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        */
    });

    return 0;
}
