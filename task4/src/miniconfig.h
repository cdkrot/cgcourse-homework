#pragma once
#include <glm/glm.hpp>
#include <string>
#include <map>

class Config {
private:
    std::map<std::string, std::string> tokens;
    std::string file;

private:
    void trim(std::string& tok);
    
public:
    Config(std::string file): file(file) {
        reload();
    }
    
    void reload();

    std::string get(std::string s);

    float get_float(std::string s);

    glm::vec3 get_vec(std::string s);

    glm::vec4 get_vec4(std::string s);
};
