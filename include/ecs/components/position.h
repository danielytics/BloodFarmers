#ifndef COMPONENT_POSITION_H
#define COMPONENT_POSITION_H

#include <glm/glm.hpp>

namespace ecs::components {

struct position {
    glm::vec3 position;
};

}

#endif // COMPONENT_POSITION_H