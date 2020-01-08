#ifndef TYPES_H
#define TYPES_H

#include <cstdint>
#include <entt/entity/registry.hpp>
#include <util/clock.h>

namespace ecs {

using registry_type = entt::registry;
using registry_t = entt::registry;
using entity = registry_type::entity_type;
using entity_t = registry_t::entity_type;

}

#endif // TYPES_H