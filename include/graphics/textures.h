#ifndef TEXTURES_H
#define TEXTURES_H

#ifndef GL3_PROTOTYPES
#define GL3_PROTOTYPES 1
#endif
#include <GL/glew.h>

#include <string>
#include <vector>

namespace textures {
    GLuint load (const std::string& filename);
    GLuint loadArray (bool filtering, const std::vector<std::string>& filenames);
}



#endif // TEXTURES_H