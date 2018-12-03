#ifndef GRAPHICS_RENDERER_H
#define GRAPHICS_RENDERER_H

#include <services/core/renderer.h>

#include <graphics/shader.h>
#include <graphics/spritepool.h>
#include <graphics/imagesets.h>

namespace graphics {

class Renderer : public services::Renderer {
public:
    Renderer ();
    ~Renderer();

    void init ();
    void render ();
    void windowChanged ();

    // Public API

    void loadScene (const std::string& scene_config);
    void unloadScene ();

    void submit (const RenderMode render_mode, const Type render_type, resources::Handle&& data_handle);

private:
    std::vector<resources::Handle> sprite_data;

    glm::ivec4 viewport;
    glm::mat4 projection_matrix;

    graphics::Imagesets imagesets;
    graphics::SpritePool sprite_pool;

    graphics::shader tiles_shader;
    graphics::shader spritepool_shader;
    
    graphics::uniform u_spritepool_view_matrix;
    graphics::uniform u_spritepool_billboarding;
    graphics::uniform u_tile_pv_matrix;
    graphics::uniform u_tile_model_matrix;
    graphics::uniform u_tile_texture;
};

};

#endif // GRAPHICS_RENDERER_H