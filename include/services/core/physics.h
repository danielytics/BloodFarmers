#ifndef SERVICES_CORE_PHYSICS_H
#define SERVICES_CORE_PHYSICS_H

#include <glm/glm.hpp>

#include <ecs/types.h>

namespace services {

class Physics {
public:
    struct Body {
        glm::vec3 position;
        float mass;
        float friction;
        float restitution;
    };
    struct Shape {
        glm::vec3 halfExtents;
    };

    virtual void addBody (const ecs::entity entity, const Body& body, const Shape& shape) = 0;
    virtual void addBodies (const std::vector<ecs::entity>& entities, const std::vector<Body>& bodies, const std::vector<Shape>& shapes) = 0;

    virtual void getBodyPosition (const ecs::entity entity, glm::vec3& position) = 0;
    virtual void getBodyPositions (const std::vector<ecs::entity> entities, std::vector<glm::vec3>& positions) = 0;

    virtual void removeBody (const ecs::entity entity) = 0;
    virtual void removeBodies (const std::vector<ecs::entity>& entities) = 0;
};

}

#endif // SERVICES_CORE_PHYSICS_H