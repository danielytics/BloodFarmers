#ifndef ECS_TYPES_H
#define ECS_TYPES_H

#include <entt/entity/registry.hpp>

namespace ecs {

using registry_type = entt::registry<>;
using entity = registry_type::entity_type;

}

#endif // ECS_TYPES_H