#ifndef SERVICES_CORE_RESOURCES_H
#define SERVICES_CORE_RESOURCES_H

#include <vector>
#include <map>
#include <atomic>

#include <glm/glm.hpp>
#include <entt/core/hashed_string.hpp>

#include <services/locator.h>
#include <util/logging.h>

namespace resources {

struct Handle {
    Handle () = delete;
    Handle (const Handle& other);
    Handle (Handle&& other) : uid(other.uid) {
        // Move constructed handle uses refcount of other instance
    }
    ~Handle ();
    template <typename T> inline T& get ();
private:
    Handle (std::size_t uid) : uid(uid) {}
    const std::size_t uid;
    friend class services::Resources;
};

class Factory {
public:
    virtual ~Factory() {}
    virtual void* load () = 0;
    virtual void unload (void*) = 0;
};

struct MemoryBuffer {
    std::size_t size;
    void* data;
};

template <typename T>
struct TypedMemoryBuffer {
    std::size_t size;
    T* data;

    TypedMemoryBuffer (MemoryBuffer* mb)
        : size(mb->size / sizeof(T))
        , data(reinterpret_cast<T*>(mb->data))
    {}
};

struct Vec3Buffer {
    std::size_t size;
    glm::vec3* data;
};

struct Vec4Buffer {
    std::size_t size;
    glm::vec4* data;
};

struct MemoryStackBuffer {
    std::size_t size;
    std::size_t top;
    void* data;

    inline void* allocate (std::size_t amount) {
        void* ptr = reinterpret_cast<void*>(reinterpret_cast<char*>(data) + top);
        top += amount;
        if (top >= size) {
            return nullptr;
        }
        return ptr;
    }
    template <typename T, typename... Args>
    inline T& allocate (Args... args) {
        void* ptr = allocate(sizeof(T));
        if (ptr == nullptr) {
            fatal("Stack Overflow (size: {}, top: {})", size, top);
        }
        return new (ptr) T(args...);
    }
    template <typename T>
    inline void deallocate (T& obj) {
        obj.~obj();
    }
    inline void reset () {
        top = 0;
    }
};

struct Texture {

};

struct Model {

};

struct Collection {

};

}

namespace services {

struct InvalidType;

class Resources {
    struct ResourceEntry {
        std::unique_ptr<std::atomic_int> refcount;
        std::uint32_t type_id;
        void* ptr;
    };
public:
    Resources ()
    {
        idForType<InvalidType>();
    }
    ~Resources () {

    }

    template <typename T>
    void registerResourceType (const entt::hashed_string& type_name, resources::Factory* factory);
    template <typename T, typename Factory>
    void registerResourceType (const entt::hashed_string& type_name);

    resources::Handle create (const entt::hashed_string::hash_type& resource_type);
    resources::Handle create (const entt::hashed_string::hash_type& resource_type, const entt::hashed_string::hash_type& instance_name);
    template <typename T> resources::Handle create ();
    template <typename T> resources::Handle create (const entt::hashed_string::hash_type& instance_name);
    resources::Handle lookup (const entt::hashed_string& instance_name);
    void destroy (resources::Handle&& handle, bool force = false);
    void cleanup (bool force = false);


private:
    template <typename T> T& get (std::size_t resource_id);
    void incref (std::size_t resource_id);
    void decref (std::size_t resource_id);

    std::vector<ResourceEntry> resources;
    std::map<const entt::hashed_string::hash_type, std::size_t> named_resources;
    std::map<std::size_t, const entt::hashed_string> type_for_id;
    std::map<const entt::hashed_string::hash_type, std::pair<std::uint32_t, std::unique_ptr<resources::Factory>>> resource_factories;

    template <typename T> static std::uint32_t idForType ();
    static std::uint32_t total_type_ids;

    friend class resources::Handle;
};

}

inline resources::Handle::Handle (const Handle& other) : uid(other.uid) {
    services::locator::resources::ref().incref(uid);
}
inline resources::Handle::~Handle () {
    services::locator::resources::ref().decref(uid);
}

template <typename T> inline T& resources::Handle::get () {
    return services::locator::resources::ref().get<T>(uid);
}

template <typename T>
void services::Resources::registerResourceType (const entt::hashed_string& type_name, resources::Factory* factory) {
    auto resource_type = type_name.value();
    auto type_id = idForType<T>();
    type_for_id[type_id] = resource_type;
    resource_factories[resource_type] = std::make_pair(type_id, std::unique_ptr<resources::Factory>(factory));
}

template <typename T, typename Factory>
inline void services::Resources::registerResourceType (const entt::hashed_string& type_name) {
    return registerResourceType<T>(type_name, std::make_unique<Factory>());
}

template <typename T>
inline T& services::Resources::get (std::size_t resource_id) {
    std::uint32_t type_id = idForType<T>();
    ResourceEntry& entry = resources[resource_id];
    if (entry.type_id == type_id) {
        return *reinterpret_cast<T*>(entry.ptr);
    } else if (entry.type_id == idForType<InvalidType>()) {
        fatal("Requested resource [{}] does not exist", resource_id);
    } else {
        fatal("Requested resource [{}] type mismatch (was: {}, expected: {})", resource_id, entry.type_id, type_id);
    }
}

inline resources::Handle services::Resources::create (const entt::hashed_string::hash_type& resource_type, const entt::hashed_string::hash_type& instance_name) {
    auto handle = create(resource_type);
    named_resources[instance_name] = handle.uid;
    return handle;
}

template <typename T>
inline resources::Handle services::Resources::create () {
    return create(type_for_id[idForType<T>()]);
}

template <typename T>
inline resources::Handle services::Resources::create (const entt::hashed_string::hash_type& instance_name) {
    auto handle = create<T>();
    named_resources[instance_name] = handle.uid;
    return handle;
}

inline resources::Handle services::Resources::lookup (const entt::hashed_string& instance_name) {
    auto it = named_resources.find(instance_name.value());
    if (it != named_resources.end()) {
        return resources::Handle{named_resources[instance_name]};
    } else {
        fatal("Requested non-existent resource instance: '{}'", instance_name);
    }
}

inline void services::Resources::incref (std::size_t resource_id) {
    ++(*resources[resource_id].refcount);
}

inline void services::Resources::decref (std::size_t resource_id) {
    --(*resources[resource_id].refcount);
}

template <typename T>
inline std::uint32_t services::Resources::idForType () {
    static const std::uint32_t type_id = total_type_ids++;
    return type_id;
}

#endif // SERVICES_CORE_RESOURCES_H