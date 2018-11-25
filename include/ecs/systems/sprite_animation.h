#ifndef SPRITE_ANIMATION_H
#define SPRITE_ANIMATION_H

#include "ecs/system.h"

#include <ecs/components/bitmap_animation.h>
#include <ecs/components/sprite.h>

namespace ecs::systems {

class sprite_animation : public ecs::base_system<sprite_animation, ecs::components::bitmap_animation, ecs::components::sprite> {
public:
    sprite_animation () {}
    ~sprite_animation () noexcept = default;

    void setTime (ElapsedTime_t elapsed_time) {
        current_time = elapsed_time;
    }

    void update (ecs::entity, ecs::components::bitmap_animation& animation, ecs::components::sprite& sprite) {
        auto elapsed = current_time - animation.start_time;
        float delta = float(elapsed) * 0.000001f;
        if (delta > animation.speed) {
            if (++animation.current_frame >= animation.max_frames) {
                animation.current_frame = 0;
            }
            animation.start_time = current_time;
        }
        float image = sprite.image;
        sprite.image = animation.base_image + animation.current_frame;
    }
private:
    ElapsedTime_t current_time;
};

}

#endif // SPRITE_ANIMATION_H