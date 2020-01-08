#ifndef COMPONENTS_H
#define COMPONENTS_H

#include <types.h>

namespace ecs::components {

namespace animation {
    
    struct animated {
        ElapsedTime_t start_time;
    };

    struct bitmap {
        // Attributes
        float speed;
        std::uint16_t base_image;
        std::uint8_t max_frames;
        // Runtime data
        std::uint8_t current_frame;
    };

}
}

#endif