#ifndef ECS_SYSTEMS_PHYSICS_SIMULATION_H
#define ECS_SYSTEMS_PHYSICS_SIMULATION_H

#include <ecs/system.h>

#include <ecs/components/physics_body.h>
#include <ecs/components/position.h>

#include <services/core/physics.h>

namespace ecs::systems {

class physics_simulation : public ecs::base_system<physics_simulation, ecs::components::physics_body, ecs::components::position> {
public:
    physics_simulation () : physics(services::locator::physics::get().lock()) {
    }

    ~physics_simulation() {
    }

    void update (const ecs::entity e, const ecs::components::physics_body& physics_body, ecs::components::position& position) {
        physics->getBodyPosition(e, position.position);
    }

    void notify (ecs::registry_type& registry, ecs::EntityNotification notification, const std::vector<ecs::entity>& entities) {
        switch (notification) {
            case ecs::EntityNotification::ADDED:
                for (auto entity : entities) {
                    info("Entity {} added to physics system", entity);
                    auto position = registry.get<ecs::components::position>(entity);
                    physics->addBody(entity,
                            services::Physics::Body{position.position + glm::vec3(0, 1.0f, 0), 1.0f, 0.5f, 1.0f},
                            services::Physics::Shape{glm::vec3(0.5f, 1.0f, 0.5f)});
                }
                break;
            case ecs::EntityNotification::REMOVED:
                 for (auto entity : entities) {
                    info("Entity {} removed from physics system", entity);
                    physics->removeBody(entity);
                }
                break;
        };
    }

private:
    std::shared_ptr<services::Physics> physics;
};

}

#endif // ECS_SYSTEMS_PHYSICS_SIMULATION_H