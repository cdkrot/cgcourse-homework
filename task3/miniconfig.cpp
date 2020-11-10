#include <fstream>
#include <glm/glm.hpp>
#include <inttypes.h>
#include <stdint.h>
#include <stdlib.h>
#include <string>
#include <map>

class Config {
private:
    std::map<std::string, std::string> tokens;
    std::string file;

public:
    Config(std::string file): file(file) {
        reload();
    }

    void trim(std::string& tok) {
        while (not tok.empty() and std::isspace(tok.back()))
            tok.pop_back();

        int p = 0;
        while (p < (int)(tok.size()) and std::isspace(tok[p]))
            ++p;
        tok.erase(tok.begin(), tok.begin() + p);
    }
    
    void reload() {
        tokens.clear();

        std::ifstream stream(file);
        std::string line;
        
        while (std::getline(stream, line)) {
            trim(line);
                
            if (line.empty() or line[0] == '#')
                continue;
                
            int peq = std::find(line.begin(), line.end(), '=') - line.begin();

            if (peq == (int)(line.size()))
                throw std::runtime_error("bad config");

            std::string key = line.substr(0, peq);
            std::string val = line.substr(peq + 1, (int)(line.size()) - peq - 1);
            trim(key);
            trim(val);

            tokens[key] = val;
        }

        if (stream.bad())
            throw std::runtime_error("failed to read");
    }

    std::string get(std::string s) {
        auto it = tokens.find(s);
        if (it == tokens.end())
            throw std::runtime_error(std::string("no key ") + s);
        return it->second;
    }

    /*
    int get_int(std::string s) {
        std::string val = get(s);

        int e;
        int res = strtoi(val.c_str(), NULL, 10, INT_MIN, INT_MAX, &e);
        if (e)
            throw std::runtime_error("can't convert to int");

        return res;
        }*/

    float get_float(std::string s) {
        std::string val = get(s);

        int other_errno = errno;
        errno = 0;
        
        float res = strtof(val.c_str(), NULL);
        std::swap(other_errno, errno);

        if (other_errno)
            throw std::runtime_error(std::string("can't convert to float for key") + s);

        return res;
    }

    glm::vec3 get_vec(std::string s) {
        return glm::vec3 {get_float(s + ".x"), get_float(s + ".y"), get_float(s + ".z")};
    }

    glm::vec4 get_vec4(std::string s) {
        return glm::vec4 {get_float(s + ".x"), get_float(s + ".y"), get_float(s + ".z"), get_float(s + ".w")};
    }
};
