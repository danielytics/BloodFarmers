#ifndef SYSTEMS_SPRITE_RENDER_H
#define SYSTEMS_SPRITE_RENDER_H

#include <ecs/system.h>

#include <graphics/shader.h>
#include <graphics/spritepool.h>

#include <ecs/components/sprite.h>
#include <ecs/components/position.h>

#include <services/locator.h>

namespace ecs::systems {

class sprite_render : public ecs::base_system<sprite_render, ecs::components::sprite, ecs::components::position> {
public:
    sprite_render (graphics::SpritePool& pool, graphics::shader& shader)
        : sprite_pool(pool)
        , spritepool_shader(shader)
    {
        u_spritepool_view_matrix = shader.uniform("view");
    }

    ~sprite_render() noexcept = default;

    void setView (const glm::mat4& view) {
        view_matrix = view;
    }

    void update (ecs::entity, const ecs::components::sprite& sprite, const ecs::components::position& position) {
        // gather commands for renderer
        spheres.push_back({position.position, sprite.image});
    }

    void post () {
        std::size_t num_objects = spheres.size();

        sprite_pool.update(spheres);
        spritepool_shader.use();
        u_spritepool_view_matrix.set(view_matrix);
        sprite_pool.render();

        // reset for next frame
        spheres = {};
        // next frame will likely have the same number of sprites to render
        spheres.reserve(num_objects);
    }

private:
    std::vector<graphics::Sprite> spheres; // x, y, z, radius
    graphics::SpritePool& sprite_pool;
    graphics::shader& spritepool_shader;
    graphics::uniform u_spritepool_view_matrix;
    glm::mat4 view_matrix;
};

}

#endif // SYSTEMS_SPRITE_RENDER_H