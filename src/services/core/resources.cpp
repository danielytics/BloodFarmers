#include "services/core/resources.h"

std::uint32_t services::Resources::total_type_ids = 0;

resources::Handle services::Resources::create (const entt::hashed_string::hash_type& resource_type) {
    auto& [type_id, factory] = resource_factories[resource_type];
    auto uid = resources.size();
    // resources.push_back({std::make_unique<std::atomic_int>(1), type_id, factory->load()});
    return resources::Handle(uid);
}

void services::Resources::destroy (resources::Handle&& handle, bool force) {
    // ResourceEntry& entry = resources[handle.uid];
    // if (*entry.refcount == 1 || force) {
    //     // Only the current handle should be live, unless force is specified
    //     auto resource_type = type_for_id[entry.type_id];
    //     auto& factory = resource_factories[resource_type].second;
    //     factory->unload(entry.ptr);
    //     entry.type_id = idForType<InvalidType>();
    // }
}

void services::Resources::cleanup (bool force) {
    // for (ResourceEntry& entry : resources) {
    //     if (*entry.refcount == 0 || force) {
    //         // Should have no live handles, unless force is specified
    //         auto resource_type = type_for_id[entry.type_id];
    //         auto& factory = resource_factories[resource_type].second;
    //         factory->unload(entry.ptr);
    //         entry.type_id = idForType<InvalidType>();
    //     }
    // }
}
