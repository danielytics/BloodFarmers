#ifndef IMAGESET_H
#define IMAGESET_H

#ifndef GL3_PROTOTYPES
#define GL3_PROTOTYPES 1
#endif
#include <GL/glew.h>
#include <entt/core/hashed_string.hpp>

#include <vector>
#include <map>
#include <string>

namespace graphics {

class Imagesets {
public:
    Imagesets();
    ~Imagesets();

    void load (entt::hashed_string::hash_type id, bool textureFiltering, const std::vector<std::string>& filenames);
    void load (const std::string& configFilename);

    void unload ();

    inline int get (entt::hashed_string::hash_type id) const {
        return imagesets.at(id);
    }

private:
    std::vector<GLuint> texture_arrays;
    std::map<entt::hashed_string::hash_type, int> imagesets;
};

}

#endif // IMAGESET_H