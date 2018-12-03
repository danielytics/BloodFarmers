#ifndef SERVICES_CORE_RENDERER_H
#define SERVICES_CORE_RENDERER_H

#include <string>
#include <cstdint>
#include <utility>

#include <util/helpers.h>

namespace resources {

class Handle;

}

// data will store exactly count T's, aligned to Alignment bytes and count will be a multiple of Width
template <typename T, std::uint32_t Width, std::uint32_t Alignment>
struct aligned_vector {
    T* const data; // stores `count` T's
    const std::uint32_t offset; // bytes `data` was offset from buffer start to align
    const std::uint32_t count; // a multiple of `Width`
    const std::uint32_t lost; // bytes lost due to Alignment and Width

    aligned_vector (void* buffer, std::uint32_t bytes)
        : data(helpers::align<T>(buffer, Alignment))
        , offset(reinterpret_cast<intptr_t>(data) - reinterpret_cast<intptr_t>(buffer))
        , count(helpers::roundDown<std::uint32_t>((bytes - offset) / sizeof(T), Width))
        , lost(bytes - (count * sizeof(T)))
    {}

    T& operator[] (std::size_t idx) {
        if (idx < count) {
            return data[idx];
        }
        fatal("Attempted to out of bounds access to aligned_vector<T, {}, {}> (idx: {}, size: {})", Width, Alignment, idx, count);
    }
};

namespace services {

class Renderer {
public:
    enum class RenderMode {
        Normal,
    };
    enum class Type {
        Meshes,
        Sprites,
    };
 
    // Resource management.

    // Loads shaders, textures, meshes for an entire scene
    virtual void loadScene (const std::string& scene_config) = 0;

    // Unload shaders, textures, meshes for an entire scene
    virtual void unloadScene () = 0;

    // Rendering.

    virtual void submit (const RenderMode render_mode, const Type render_type, resources::Handle&& data_handle) = 0;

    inline void submitMeshes (const RenderMode render_mode, resources::Handle&& meshes) {
        submit(render_mode, Type::Meshes, std::forward<resources::Handle>(meshes));
    }
    inline void submitSprites (const RenderMode render_mode, resources::Handle&& sprites) {
        submit(render_mode, Type::Sprites, std::forward<resources::Handle>(sprites));
    }
};

}

#endif // SERVICES_CORE_RENDERER_H