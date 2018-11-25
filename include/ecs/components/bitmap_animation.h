#ifndef COMPONENT_BITMAP_ANIMATION_H
#define COMPONENT_BITMAP_ANIMATION_H

#include <util/clock.h>

namespace ecs::components {

struct bitmap_animation {
    // Attributes
    float base_image;
    float max_frames;
    float speed;
    // Runtime data
    float current_frame;
    ElapsedTime_t start_time;
};

}

#endif // COMPONENT_BITMAP_ANIMATION_H