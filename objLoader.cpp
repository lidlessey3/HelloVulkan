#include "objLoader.h"
#include <cstdlib>
#include <fstream>
#include <iostream>

void objloader::loadObj(ObjData &objData, const char *path) {
    std::fstream objFile(path, std::ios::in | std::ios::binary);    // opening the file in input in text mode
    char buffer;
    std::vector<glm::vec3> bufferVertexPos;              // will keep track of the verticies positions as they are read
    std::vector<glm::vec2> bufferTexturePos;             // will keep track of textures positions as they are read
    uint32_t vertexPosCount = 0, texturePosCount = 0;    // will count the number of entreis

    std::cout << "Attempting to load model in \"" << path << "\"." << std::endl;

    if(!objFile.is_open())  // if the file wasn't opened return
        return;

    std::cout << "Successfull read of the obj file." << std::endl;

    do {
        objFile.read(&buffer, 1);
        switch (buffer) {
        case 'v':
            objFile.read(&buffer, 1);
            switch (buffer) {
            case ' ': {    // it is a new vertex position

                glm::vec3 newPos;
                // read the 3D position
                objFile >> newPos.x;
                objFile >> newPos.y;
                objFile >> newPos.z;

                // increment the counter and add it to the buffer
                vertexPosCount++;
                bufferVertexPos.push_back(newPos);
            }; break;
            case 't': {
                glm::vec2 newTextPos;
                // read the texture position
                objFile >> newTextPos.x;
                objFile >> newTextPos.y;
                // increment the counter and add it to the buffer
                bufferTexturePos.push_back(newTextPos);
                texturePosCount++;

                objFile.read(&buffer, 1);

                if (buffer != '\n')    // if there is a third value just disregard it
                    objFile >> newTextPos.x;
            }; break;
            default:    // if I dont recognise the type I just skip the line
                do {
                    objFile.read(&buffer, 1);
                } while (buffer != '\n');
                break;
            }
            break;
        case 'f': {    // read the faces and puts together the final postion
            uint32_t index;
            Vertex vertex;

            for (int i = 0; i < 3; i++) {                   // each face has at least three verticies
                objFile >> index;                           // read the position index
                vertex.pos = bufferVertexPos[index - 1];    // save it into the position of the vertex

                objFile.read(&buffer, 1);    // read a / thing

                objFile >> index;                                  // read the texture position index
                vertex.textCoord = bufferTexturePos[index - 1];    // save it into the vertex struct

                // read a the normal vector index, tough I don't need it yet so I do nothing with it :)
                objFile.read(&buffer, 1);
                if (buffer == '/')
                    objFile >> index;

                bool unique = true;
                uint32_t indexV;

                for (indexV = 0; indexV < objData.verticies.size() && unique; indexV++) {    // check if the vertex is unique
                    if (objData.verticies[indexV] == vertex)
                        unique = false;
                }

                if (unique) {    // if it is add it
                    objData.verticies.push_back(vertex);
                } else {    // otherwise reduce index by one
                    indexV--;
                }

                objData.indexes.push_back(indexV);
            }

            objFile.read(&buffer, 1);

            while (buffer == ' ') {                            // check if there is a forth vertex and if so add it
                objFile >> index;                           // read the position index
                vertex.pos = bufferVertexPos[index - 1];    // save it into the position of the vertex

                objFile.read(&buffer, 1);    // read a / thing

                objFile >> index;                                  // read the texture position index
                vertex.textCoord = bufferTexturePos[index - 1];    // save it into the vertex struct

                // read a the normal vector index, tough I don't need it yet so I do nothing with it :)
                objFile.read(&buffer, 1);
                if (buffer == '/')
                    objFile >> index;

                bool unique = true;
                uint32_t indexV;

                for (indexV = 0; indexV < objData.verticies.size() && unique; indexV++) {    // check if the vertex is unique
                    if (objData.verticies[indexV] == vertex)
                        unique = false;
                }

                if (unique) {    // if it is add it
                    objData.verticies.push_back(vertex);
                } else {    // otherwise reduce index by one
                    indexV--;
                }

                // before adding the forth I read the last and the second to last so to have a valid triangle
                objData.indexes.push_back(objData.indexes[objData.indexes.size() - 3]);
                objData.indexes.push_back(objData.indexes[objData.indexes.size() - 2]);

                objData.indexes.push_back(indexV);
                objFile.read(&buffer, 1);
            }
        } break;
        default:    // if I dont recognise the type I just skip the line
            while (buffer != '\n') {    // it is necessary that this be while and not a do while cause I don't always eat the last '\n' from a line
                objFile.read(&buffer, 1);
            }
            break;
        }
    } while (!objFile.eof());

    objFile.close();    // closing the file

    // debug messages
    std::cout << "Read " << vertexPosCount << " Verticies and " << texturePosCount << " texture coordinates." << std::endl;
    std::cout << "There were " << objData.verticies.size() << " distinct verticies and " << objData.indexes.size() / 3 << " triangles." << std::endl;
}
