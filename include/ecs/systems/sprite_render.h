#ifndef SYSTEMS_SPRITE_RENDER_H
#define SYSTEMS_SPRITE_RENDER_H

#include <ecs/system.h>

#include <graphics/shader.h>
#include <graphics/spritepool.h>

#include <ecs/components/sprite.h>
#include <ecs/components/position.h>

#include <services/locator.h>
#include <services/core/resources.h>
#include <services/core/renderer.h>

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

    void pre () {
        sprite_data_handle = services::locator::resources::ref().request("sprites"_hs);
        sprites = sprite_data_handle.buffer<graphics::Sprite>();
    }

    void update (ecs::entity, const ecs::components::sprite& sprite, const ecs::components::position& position) {
        // gather commands for renderer
        sprites.emplace_back(position.position, sprite.image);
    }

    void post () {
        sprite_data_handle.release(std::move(sprites)); // 'sprites' is no longer valid, can only be accessed through handle
        auto& renderer = services::locator::renderer::ref();
        info("About to submit sprites");
        renderer.submit(services::Renderer::RenderMode::Normal,
                        services::Renderer::Type::Sprites,
                        std::move(sprite_data_handle)); // handle no longer valid
        info("After submitting");
        // std::size_t num_objects = spheres.size();

        // sprite_pool.update(spheres);
        // spritepool_shader.use();
        // u_spritepool_view_matrix.set(view_matrix);
        // sprite_pool.render();

        // // reset for next frame

        // // next frame will likely have the same7le8uctsvn77777lumber of sprites to render
        // spheres.reserve(num_objects);
    }

private:
    std::vector<graphics::Sprite> spheres; // x, y, z, radius
    graphics::SpritePool& sprite_pool;
    graphics::shader& spritepool_shader;
    graphics::uniform u_spritepool_view_matrix;
    glm::mat4 view_matrix;

    resources::Handle sprite_data_handle;
    resources::Buffer<graphics::Sprite> sprites;
};

}

#endif // SYSTEMS_SPRITE_RENDER_H