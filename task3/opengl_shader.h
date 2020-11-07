#pragma once

#include <string>
#include <vector>

#include <glm/glm.hpp>
#include <GL/glew.h>

class shader_t
{
public:
   shader_t(const std::string& vertex_code_fname, const std::string& fragment_code_fname);
   ~shader_t();

   void use();
   template<typename T> void set_uniform(const std::string& name, T val);
   template<typename T> void set_uniform(const std::string& name, T val1, T val2);
   template<typename T> void set_uniform(const std::string& name, T val1, T val2, T val3);
   template<typename T> void set_uniform(const std::string& name, T val1, T val2, T val3, T val4);
   template <int C> void set_uniformv(const std::string& name, glm::vec<C, float, glm::defaultp>);
private:
   void check_compile_error();
   void check_linking_error();
   void compile(const std::string& vertex_code, const std::string& fragment_code);
   void link();

   GLuint vertex_id_, fragment_id_, program_id_;
};
