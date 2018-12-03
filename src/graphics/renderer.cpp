#include "graphics/renderer.h"

#include <services/core/resources.h>
#include <graphics/spritepool.h>

#include "graphics/shader.h"
#include "graphics/mesh.h"
#include "graphics/camera.h"
#include "graphics/imagesets.h"
#include "graphics/spritepool.h"
#include "graphics/renderer.h"

#include "util/logging.h"


graphics::Renderer::Renderer ()
{
    info("Renderer");
}

graphics::Renderer::~Renderer ()
{

}

void graphics::Renderer::init ()
{
#ifdef DEBUG_BUILD
    int max_tex_layers, max_combined_tex, max_vert_tex, max_geom_tex, max_frag_tex, max_tex_size;
    glGetIntegerv(GL_MAX_ARRAY_TEXTURE_LAYERS, &max_tex_layers);
    glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &max_combined_tex);
    glGetIntegerv(GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS, &max_vert_tex);
    glGetIntegerv(GL_MAX_GEOMETRY_TEXTURE_IMAGE_UNITS, &max_geom_tex);
    glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &max_frag_tex);
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max_tex_size);
    info("Texture limits: {} combined units, {}/{}/{} vs/gs/fs units, {} array layers, {}x{} max size", max_combined_tex, max_vert_tex, max_geom_tex, max_frag_tex, max_tex_layers, max_tex_size, max_tex_size);
    int max_vert_uniform_vec, max_frag_uniform_vec, max_varying_vec, max_vertex_attribs;
    glGetIntegerv(GL_MAX_VERTEX_UNIFORM_VECTORS, &max_vert_uniform_vec);
    glGetIntegerv(GL_MAX_FRAGMENT_UNIFORM_VECTORS, &max_frag_uniform_vec);
    glGetIntegerv(GL_MAX_VARYING_VECTORS, &max_varying_vec);
    glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &max_vertex_attribs);
    info("Shader limits: {} vertex attributes, {} varying vectors, {} vertex vectors, {} fragment vectors", max_vertex_attribs, max_varying_vec, max_vert_uniform_vec, max_frag_uniform_vec);
#endif

    imagesets.load("imagesets.toml");

    info("Loading shaders");
    tiles_shader = graphics::shader::load({
        {graphics::shader::types::Vertex,   "shaders/tiles.vert"},
        {graphics::shader::types::Fragment, "shaders/tiles.frag"},
    });
    tiles_shader.use();
    u_tile_pv_matrix = tiles_shader.uniform("projection_view");
    u_tile_model_matrix = tiles_shader.uniform("model");
    u_tile_texture = tiles_shader.uniform("texture_albedo");

    spritepool_shader = graphics::shader::load({
        {graphics::shader::types::Vertex,   "shaders/spritepool.vert"},
        {graphics::shader::types::Fragment, "shaders/spritepool.frag"},
    });
    spritepool_shader.use();
    u_spritepool_view_matrix = spritepool_shader.uniform("view");
    u_spritepool_billboarding = spritepool_shader.uniform("billboarding");

    sprite_pool.init(spritepool_shader, imagesets.get("characters"_hs));

    // Set OpenGL settings
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LEQUAL);
    glDisable(GL_BLEND);
    glDisable(GL_STENCIL_TEST);
    glClearDepth(1.0);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_MULTISAMPLE);

}

void graphics::Renderer::windowChanged ()
{
    auto field_of_view = services::locator::config<"renderer.field-of-view"_hs, float>();
    auto near_distance = services::locator::config<"renderer.near-distance"_hs, float>();
    auto far_distance = services::locator::config<"renderer.far-distance"_hs, float>();
    auto width = services::locator::config<"renderer.width"_hs, float>();
    auto height = services::locator::config<"renderer.height"_hs, float>();
    projection_matrix = glm::perspective(glm::radians(field_of_view), float(width) / float(height), near_distance, far_distance);
    viewport = glm::vec4(0, 0, int(width), int(height));

    spritepool_shader.use();
    spritepool_shader.uniform("projection").set(projection_matrix);
}

void graphics::Renderer::loadScene (const std::string& scene_config)
{
    
}

void graphics::Renderer::unloadScene ()
{

}

void graphics::Renderer::submit (const RenderMode render_mode, const Type render_type, resources::Handle&& data_handle)
{
    switch (render_type) {
        case services::Renderer::Type::Sprites:
            sprite_data.push_back(std::move(data_handle));
            break;
        default:
            break;
    }
}

void graphics::Renderer::render ()
{
    glm::mat4 view_matrix = services::locator::camera::ref().view();
    glm::mat4 projection_view_matrix = projection_matrix * view_matrix;

    auto frustum = math::frustum(projection_view_matrix);

    for (auto& handle : sprite_data) {
        resources::MemoryBuffer& buffer = handle.mem_buffer<graphics::Sprite>();
        buffer.count = 0;
    }

    glClearColor(0, 0, 0, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glViewport(viewport.x, viewport.y, viewport.z, viewport.w);

    // tiles_shader.use();
    // u_tile_pv_matrix.set(projection_view_matrix);
    // for (const auto& surface : level) {
    //     surface.draw(u_tile_texture);
    // }

    // sprite_pool.update(spheres);
    // spritepool_shader.use();
    // u_spritepool_view_matrix.set(view_matrix);
    // sprite_pool.render();
}
