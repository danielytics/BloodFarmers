#ifndef SYSTEMS_SPRITE_RENDER_H
#define SYSTEMS_SPRITE_RENDER_H

#include <ecs/system.h>

#include <ecs/components/sprite.h>
#include <ecs/components/position.h>

#include <services/locator.h>
#include <services/core/resources.h>
#include <services/core/renderer.h>

namespace ecs::systems {

class sprite_render : public ecs::base_system<sprite_render, ecs::components::sprite, ecs::components::position> {
public:

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
        renderer.submit(services::Renderer::RenderMode::Normal,
                        services::Renderer::Type::Sprites,
                        std::move(sprite_data_handle)); // handle no longer valid
    }

private:
    resources::Handle sprite_data_handle;
    resources::Buffer<graphics::Sprite> sprites;
};

}

#endif // SYSTEMS_SPRITE_RENDER_H