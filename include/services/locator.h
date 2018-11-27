#ifndef SERVICES_LOCATOR_H
#define SERVICES_LOCATOR_H

#include <entt/locator/locator.hpp>
#include <entt/core/monostate.hpp>

#include <util/helpers.h>
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
    template <const entt::hashed_string::hash_type Key, typename T> static T config () {return entt::monostate<Key>{};}
    template <const entt::hashed_string::hash_type Key, typename T> static void config (const T& value) {entt::monostate<Key>{} = value;}

private:
    struct ConfigTag {};
};

}

#endif // SERVICES_LOCATOR_H