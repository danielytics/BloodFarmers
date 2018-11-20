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
struct Image
{
    int width;
    int height;
    int components;
    unsigned char* data;
};


GLuint textures::loadArray (bool filtering, const std::vector<std::string>& filenames)
{
    trace_fn();
    GLuint texture = 0;
    int max_width = 0;
    int max_height = 0;
    int max_components = 0;
    // Load the image files
    auto images = std::vector<Image>{};
    for (auto filename : filenames) {
        int w, h, c;
        std::string buffer = helpers::readToString(filename);
        unsigned char* image = stbi_load_from_memory(reinterpret_cast<const unsigned char*>(buffer.c_str()), int(buffer.size()), &w, &h, &c, STBI_rgb_alpha);
        debug("Loaded image '{}', width={} height={} components={}", filename, w, h, c);
        images.push_back({w, h, c, image});
        max_width = std::max(w, max_width);
        max_height = std::max(h, max_height);
        max_components = std::max(c, max_components);
    }
    GLenum format = 0;
    switch (max_components) { // Use the largest components for the texture array format
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
    glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_COMPRESSED_RGBA_S3TC_DXT1_EXT,  max_width, max_height, images.size(), 0, format, GL_UNSIGNED_BYTE, nullptr);

    // Load texture data into texture array
    for (unsigned index = 0; index < images.size(); ++index) {
        auto image = images[index];
        switch (image.components) { // Load individual images based on their own formats
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
        glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, index, image.width, image.height, 1, format, GL_UNSIGNED_BYTE, image.data);
        stbi_image_free(image.data);
    }
    debug("Loaded {} images into texture array ({}x{}x{})", images.size(), max_width, max_height, max_components);

    //Always set reasonable texture parameters
    float aniso = 0.0f;
    glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &aniso);
    glTexParameterf(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAX_ANISOTROPY_EXT, aniso); 
    glTexParameteri(GL_TEXTURE_2D_ARRAY,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D_ARRAY,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
    if (filtering) {
        glTexParameteri(GL_TEXTURE_2D_ARRAY,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D_ARRAY,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
        glGenerateMipmap(GL_TEXTURE_2D_ARRAY);
    } else {
        glTexParameteri(GL_TEXTURE_2D_ARRAY,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D_ARRAY,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    }

    return texture;
}