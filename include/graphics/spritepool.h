#ifndef SPRITEPOOL_H
#define SPRITEPOOL_H

#include "mesh.h"
#include "shader.h"
#include "math/basic.h"

namespace graphics {

struct Sprite {
    glm::vec3 position;
    float image;
};

#define INSTANCED_SPRITES
// #define VBO_SPRITES

#ifdef INSTANCED_SPRITES

class SpritePool {
public:
    SpritePool ();
    ~SpritePool ();

    void init (const graphics::shader& spriteShader, int texture_unit);
    void update (const std::vector<Sprite>& sprites);

    void render (const math::frustum& frustum);

private:
    std::vector<Sprite> sortedBuffer;
    std::vector<Sprite> unsortedBuffer;
    graphics::mesh mesh;
    graphics::buffer_t tbo;
    graphics::buffer_t tbo_tex;
    graphics::uniform u_tbo_tex;
    graphics::uniform u_texture;

    glm::vec2 prevCenterPoint;
    std::size_t visibleSprites;

    std::vector<int> culling_results;

    std::size_t spriteCount;

    std::size_t cull_sprites (const math::frustum& frustum, std::vector<graphics::Sprite>& positions);

};
#elif VBO_SPRITES
class SpritePool {
public:
    SpritePool ();
    ~SpritePool ();

    void init (const graphics::shader& spriteShader);
    void update (const std::vector<Sprite>& sprites);

    void render (const math::rect& bounds);

private:

};
#endif

}

#endif // SPRITEPOOL_H