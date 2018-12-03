#ifndef SERVICES_CORE_RESOURCES_H
#define SERVICES_CORE_RESOURCES_H

#include <type_traits>

#include <vector>
#include <map>
#include <atomic>

#include <glm/glm.hpp>
#include <entt/core/hashed_string.hpp>

#include <services/locator.h>
#include <util/logging.h>

namespace resources {

class Factory {
public:
    virtual ~Factory() {}
    virtual void* load () = 0;
    virtual void unload (void*) = 0;
};

struct MemoryBuffer {
    const std::size_t capacity; // total available space in bytes
    void* const data; // pointer to the data
    std::size_t count; // number of items currently stored in buffer
};

template <typename T>
struct Buffer {
    static_assert(std::is_trivial<T>::value, "Buffer<T> must contain a trivial type");

    Buffer () : memory_buffer(nullptr), capacity(0), data(nullptr) {} // Uninitialised buffer must not be accessed before asignment
    Buffer (MemoryBuffer* membuf)
        : memory_buffer(membuf)
        , capacity(membuf->capacity / sizeof(T))
        , data(reinterpret_cast<T*>(membuf->data))
    {}
    Buffer (const MemoryBuffer& membuf)
        : memory_buffer(const_cast<MemoryBuffer*>(&membuf))
        , capacity(membuf.capacity / sizeof(T))
        , data(reinterpret_cast<T*>(membuf.data))
    {}
    Buffer (MemoryBuffer&& membuf)
        : memory_buffer(&membuf)
        , capacity(membuf.capacity / sizeof(T))
        , data(reinterpret_cast<T*>(membuf.data))
    {}
    ~Buffer () {}
    Buffer (const Buffer<T>& other)
        : memory_buffer(other.memory_buffer)
        , capacity(other.capacity)
        , data(other.data)
    {}
    Buffer (Buffer<T>&& other)
        : memory_buffer(other.memory_buffer)
        , capacity(other.capacity)
        , data(other.data)
    {}

    Buffer<T> operator= (const Buffer<T>& other) {
        memory_buffer = other.memory_buffer;
        capacity = other.capacity;
        data = other.data;
        return *this;
    }

    typedef T* iterator;
    typedef const T* const_iterator;
    iterator begin() const { return data; }
    iterator end() const { return data + memory_buffer->count; }
    template <typename... Args> void emplace_back (Args&&... args);
    void push_back (T&& item);
    T& operator[] (std::size_t index) const;
    std::size_t size () {return memory_buffer->count;}

// private:
#ifdef BUFFER_BOUNDS_CHECKING
    static constexpr bool bounds_checking_endabled = true;
#else
    static constexpr bool bounds_checking_endabled = false;
#endif
    MemoryBuffer* memory_buffer;
    std::size_t capacity; // total number of items that fit in the memory buffer
    T* data; // typed access to memory_buffer.data
};

template <typename T>
struct StackBuffer : Buffer<T> {

    T& top () const {return Buffer<T>::data[Buffer<T>::memory_buffer.count - 1];}
    void top (T&& item) {Buffer<T>::data[Buffer<T>::memory_buffer.count - 1] = std::move(item);}
    void pop_back () {--(Buffer<T>::memory_buffer.count);}
    inline void reset () {Buffer<T>::memory_buffer.count = 0;}
     
    void swap () {
        auto t = Buffer<T>::memory_buffer.count;
        if (t > 1) {
            std::swap(Buffer<T>::data[t - 1], Buffer<T>::data[t - 2]);
        }
    }
};

using ScalarBuffer = Buffer<float>;
using Vec2Buffer = Buffer<glm::vec2>;
using Vec3Buffer = Buffer<glm::vec3>;
using Vec4Buffer = Buffer<glm::vec4>;

struct Handle {
    Handle () : uid(-1) {}
    Handle (const Handle& other);
    Handle (Handle&& other) : uid(other.uid) {
        // Move constructed handle uses refcount of other instance
    }
    ~Handle ();
    Handle operator= (const Handle& other);
    Handle operator= (Handle&& other) {uid = other.uid; return *this;}
    template <typename T> inline T* get () const;
    template <typename T> MemoryBuffer& mem_buffer () const;
    template <typename T> inline Buffer<T> buffer () const;
    template <typename T> inline void release (T&& ) const {}
private:
    Handle (std::size_t uid) : uid(uid) {}
    std::size_t uid;
    friend class services::Resources;
};

}

namespace services {

class Resources {
    struct ResourceEntry {
        std::unique_ptr<std::atomic_int> refcount;
        std::uint32_t type_id;
        void* ptr;
    };
public:
    Resources () {}
    ~Resources () {}

    struct InvalidType {};

    // instance name of resource owning pointer
    std::string name (void* pointer) {
        return "unknown";
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

    struct Allocator {
        virtual void allocate (std::size_t bytes) = 0;
        virtual void deallocate () = 0;
        virtual void* request (std::size_t alignment, std::size_t size, std::size_t count) = 0;
        virtual void release (void* buffer) = 0;
    };
    struct Info {
        entt::hashed_string id;
        entt::hashed_string lifecycle;
        entt::hashed_string request_type;
        entt::hashed_string::hash_type allocator;
        entt::hashed_string::hash_type contained_type;
        std::size_t alignment;
        std::size_t size;
        std::size_t num_buffers;
    };
    struct Entry {
        Allocator* allocator;
        void* buffers;
        entt::hashed_string::hash_type request_type;
        std::size_t num_buffers;
        std::size_t buffer_size;
        std::size_t alignment;
        std::uint32_t type_id;
        std::size_t next_buffer;
    };
    struct TypeInfo {
        std::size_t size;
        std::uint32_t type_id;
    };
    struct ResourceInstance {
        entt::hashed_string::hash_type resource_id;
        std::uint32_t type_id;
        void* buffer;
    };
    // TYpes
    std::map<entt::hashed_string::hash_type, TypeInfo> types;
    // Allocators
    std::map<entt::hashed_string::hash_type, Allocator*> allocators;
    // Global collection of resources, accessible by their id
    std::map<entt::hashed_string::hash_type, Entry> resources;
    // Each lifecycle maintains its own collection of resource ids
    std::vector<entt::hashed_string::hash_type> static_resources;
    std::vector<entt::hashed_string::hash_type> stage_resources;
    std::vector<entt::hashed_string::hash_type> frame_resources;
    // Handles
    std::vector<ResourceInstance> instances;

    template <typename T>
    void registerType (entt::hashed_string::hash_type id) {
        types[id] = {sizeof(T), idForType<T>()};
    }

    std::size_t sizeOf (entt::hashed_string::hash_type id) {
        return types[id].size;
    }

    void registerAllocator (entt::hashed_string::hash_type id, Allocator* allocator) {
        allocators[id] = allocator;
    }

    void registerResource (Info&& info) {
        Allocator* allocator = allocators[info.allocator];
        switch (info.lifecycle) {
            case "stage"_hs:
                stage_resources.push_back(info.id);
                break;
            case "frame"_hs:
                frame_resources.push_back(info.id);
                break;
            case "static"_hs:
            default: // Defaults to static
                static_resources.push_back(info.id);
                break;
        };
        info("Added {} {} buffers of {} {} each for: {}", info.num_buffers, info.lifecycle, info.size > 1024 ? info.size / 1024 : info.size, info.size > 1024 ? "KB" : "bytes", info.id);
        auto type_id = types[info.contained_type].type_id;
        resources[info.id] = {allocator, nullptr, info.request_type, info.num_buffers, info.size, info.alignment == 0 ? 1 : info.alignment, type_id, 0};
    }

    void init (entt::hashed_string lifecycle) {
        std::vector<entt::hashed_string::hash_type>* resource_list;
        switch (lifecycle) {
            case "stage"_hs:
                resource_list = &stage_resources;
                break;
            case "frame"_hs:
                resource_list = &frame_resources;
                break;
            case "static"_hs:
            default: // Defaults to static
                resource_list = &static_resources;
                break;
        };
        // First pass, count total memory per allocator
        std::map<Allocator*, std::size_t> memory_per_allocator;
        for (auto id : *resource_list) {
            auto& entry = resources[id];
            memory_per_allocator[entry.allocator] += (entry.buffer_size + entry.alignment) * entry.num_buffers;
        }
        // Second pass, allocate each allocators total memory pool
        std::size_t total_memory = 0;
        for (auto& kv : memory_per_allocator) {
            auto allocator = kv.first;
            auto bytes = kv.second;
            info("Allocating {} KB for allocator", bytes / 1024);
            allocator->allocate(bytes);
            total_memory += bytes;
        }
        // Final pass, request individual resources from allocators
        std::size_t total_buffers = 0;
        for (auto id : *resource_list) {
            auto& entry = resources[id];
            info("Requesting {} buffers of {} KB with alignment of {} bytes", entry.num_buffers, entry.buffer_size / 1024, entry.alignment);
            entry.buffers = entry.allocator->request(entry.alignment, entry.buffer_size, entry.num_buffers);
            entry.next_buffer = 0;
            total_buffers += entry.num_buffers;
        }
        info("Total {} KB allocated for {} buffers", total_memory / 1024, total_buffers);
    }

    resources::Handle request (const entt::hashed_string& resource_id) {
        auto& entry = resources[resource_id];
        intptr_t buffer;
        switch (entry.request_type) {
            case "static"_hs:
                buffer = reinterpret_cast<intptr_t>(entry.buffers) + (entry.buffer_size * entry.next_buffer);
                info("Found static buffer: {:x}", buffer);
                break;
            case "round-robin"_hs:
                buffer = reinterpret_cast<intptr_t>(entry.buffers) + (entry.buffer_size * entry.next_buffer);
                // if (++entry.next_buffer >= entry.num_buffers) {
                //     entry.next_buffer = 0;
                // }
                break;
            case "allocate"_hs:
            default:
                buffer = 0;
                break;
        };
        std::size_t uid = instances.size();
        instances.push_back(ResourceInstance{resource_id.value(), entry.type_id, reinterpret_cast<void*>(buffer)});
        return resources::Handle(uid);
    }


    void cleanup (entt::hashed_string::hash_type lifecycle) {
        std::vector<entt::hashed_string::hash_type>* resource_list;
        switch (lifecycle) {
            case "stage"_hs:
                resource_list = &stage_resources;
                break;
            case "frame"_hs:
                resource_list = &frame_resources;
                break;
            case "static"_hs:
            default: // Defaults to static
                resource_list = &static_resources;
                break;
        };
        for (const auto resource_id : *resource_list) {
            auto it = resources.find(resource_id);
            if (it != resources.end()) {
                auto& entry = it->second;
                entry.allocator->release(entry.buffers);
                resources.erase(it);
            }
        }
        resource_list->clear();
    }

private:
    template <typename T> resources::MemoryBuffer& get (std::size_t resource_id);
    void incref (std::size_t resource_id);
    void decref (std::size_t resource_id);

    std::vector<ResourceEntry> resources_;
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

inline resources::Handle resources::Handle::operator= (const resources::Handle& other) {
    uid = other.uid;
    services::locator::resources::ref().incref(uid);
    return *this;
}

template <typename T> inline T* resources::Handle::get () const {
    return *reinterpret_cast<T*>(services::locator::resources::ref().get<T>(uid).data);
}

template <typename T> inline resources::MemoryBuffer& resources::Handle::mem_buffer () const {
    return services::locator::resources::ref().get<T>(uid);
}

template <typename T> inline resources::Buffer<T> resources::Handle::buffer () const {
    return resources::Buffer<T>(services::locator::resources::ref().get<T>(uid));
}

template<typename T>
template <typename... Args>
inline void resources::Buffer<T>::emplace_back (Args&&... args) {
    if constexpr (bounds_checking_endabled) {
        if (memory_buffer->count >= capacity) {
        fatal("Memory buffer ({}) out of space: size {}, capacity {}",
            services::locator::resources::ref().name(memory_buffer->data),
            memory_buffer->count, memory_buffer->capacity);
        }
    }
    data[memory_buffer->count++] = T{std::move(args)...};
}

template<typename T>
inline void resources::Buffer<T>::push_back (T&& item) {
    if constexpr (bounds_checking_endabled) {
        if (memory_buffer->count >= capacity) {
        fatal("Memory buffer ({}) out of space: size {}, capacity {}",
            services::locator::resources::ref().name(memory_buffer->data),
            memory_buffer->count, memory_buffer->capacity);
        }
    }
    data[memory_buffer->count++] = std::move(item);
}

template<typename T>
inline T& resources::Buffer<T>::operator[] (std::size_t index) const {
    if constexpr (bounds_checking_endabled) {
        if (index >= memory_buffer->count) {
            fatal("Memory buffer ({}) index out of range: index {}, size {}, capacity {}",
                services::locator::resources::ref().name(memory_buffer->data),
                index, memory_buffer->count, memory_buffer->capacity);
        }
    }
    return data[index];
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
inline resources::MemoryBuffer& services::Resources::get (std::size_t resource_id) {
    std::uint32_t type_id = idForType<T>();
    ResourceInstance& instance = instances[resource_id];
    if (instance.type_id == type_id) {
        return *reinterpret_cast<resources::MemoryBuffer*>(instance.buffer);
    } else if (instance.type_id == idForType<InvalidType>()) {
        fatal("Requested resource [{}] does not exist", resource_id);
    } else {
        fatal("Requested resource [{}] type mismatch (was: {}, expected: {})", resource_id, instance.type_id, type_id);
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
    // ++(*resources[resource_id].refcount);
}

inline void services::Resources::decref (std::size_t resource_id) {
    // --(*resources[resource_id].refcount);
}

template <typename T>
inline std::uint32_t services::Resources::idForType () {
    static const std::uint32_t type_id = total_type_ids++;
    return type_id;
}

#endif // SERVICES_CORE_RESOURCES_H