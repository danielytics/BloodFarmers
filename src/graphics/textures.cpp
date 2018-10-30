#include "graphics/textures.h"
#include "util/logging.h"
#include "util/helpers.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

GLuint textures::load (const std::string& filename)
{
    trace_fn();
    info("Loading {}", filename);
    GLuint texture = 0;
    int width, height, comp;
    std::string buffer = helpers::readToString(filename);
    unsigned char* image = stbi_load_from_memory(reinterpret_cast<const unsigned char*>(buffer.c_str()), int(buffer.size()), &width, &height, &comp, 0/*STBI_rgb_alpha*/);

    if (image) {
        info("Loading image '{}', width={} height={} components={}", filename, width, height, comp);

        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);

        GLenum format = 0;
        switch (comp) {
        case 1:
            format = GL_RED;
            break;
        case 3:
            format = GL_RGB;
            break;
        case 4:
            format = GL_RGBA;
            break;
        }
        glTexImage2D(GL_TEXTURE_2D, 0, GLint(format), width, height, 0, format, GL_UNSIGNED_BYTE, image);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);

        stbi_image_free(image);
    } else {
        error("Could not load texture: {}", filename);
    }
    return texture;
}

GLuint textures::loadArray (const std::vector<std::string>& filenames)
{
    trace_fn();
    GLuint texture = 0;
    int width=0, height=0;
    int components = 0;
    // Load the image files
    auto images = std::vector<unsigned char*>{};
    for (auto filename : filenames) {
        int w, h, c;
        std::string buffer = helpers::readToString(filename);
        unsigned char* image = stbi_load_from_memory(reinterpret_cast<const unsigned char*>(buffer.c_str()), int(buffer.size()), &w, &h, &c, STBI_rgb_alpha);
        info("Loaded image '{}', width={} height={} components={}", filename, w, h, c);
        images.push_back(image);
        width = std::max(w, width);
        height = std::max(h, height);
        components = std::max(c, components);
    }
    GLenum format = 0;
    switch (components) { // Assume all textures have the same input format
    case 1:
        format = GL_RED;
        break;
    case 3:
        format = GL_RGB;
        break;
    case 4:
        format = GL_RGBA;
        break;
    }

    // Create the texture array
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D_ARRAY, texture);
    glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GLint(format),  width, height, images.size(), 0, format, GL_UNSIGNED_BYTE, nullptr);

    // Load texture data into texture array
    for (unsigned index = 0; index < images.size(); ++index) {
        auto image = images[index];
        glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, index, width, height, 1, format, GL_UNSIGNED_BYTE, image);
        stbi_image_free(image);
    }
    info("Loaded {} images into texture array ({}x{}x{})", images.size(), width, height, components);

    //Always set reasonable texture parameters
    glTexParameteri(GL_TEXTURE_2D_ARRAY,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D_ARRAY,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);

    return texture;
}