#ifndef OBJ_LOADER_H_INCLUDED
#define OBJ_LOADER_H_INCLUDED

#include <cstdint>
#include <glm/glm.hpp>
#include <vector>

// again this is not a good idea and the best way would be to use a library
// but this being a learning experience I wanted to try and use it to also learn a little
// on how obj files are made, even if just enough to read them

namespace objloader {    // created to avoid overloading the name vertex
    struct Vertex {
        glm::vec3 pos;
        glm::vec2 textCoord;

        Vertex() {
            pos       = { 0, 0, 0 };
            textCoord = { 0, 0 };
        }

        bool operator==(const Vertex &comparing) {
            if (this->pos == comparing.pos && this->textCoord == comparing.textCoord)
                return true;
            else
                return false;
        }
    };

    struct ObjData {
        std::vector<Vertex> verticies;
        std::vector<uint32_t> indexes;
    };

    void loadObj(ObjData &objData, const char *path);

}    // namespace objloader

#endif
