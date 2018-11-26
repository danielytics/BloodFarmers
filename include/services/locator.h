#ifndef SERVICES_LOCATOR_H
#define SERVICES_LOCATOR_H

#include <entt/locator/locator.hpp>

#include <graphics/camera.h>

namespace services {

using Camera = graphics::camera;
class Renderer;
class Physics;
class Resources;

struct locator {
    using camera = entt::service_locator<Camera>;
    using renderer = entt::service_locator<Renderer>;
    using physics = entt::service_locator<Physics>;
    using resources = entt::service_locator<Resources>;
};

}

#endif // SERVICES_LOCATOR_H