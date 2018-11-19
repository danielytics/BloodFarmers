#ifndef GL3_PROTOTYPES
#define GL3_PROTOTYPES 1
#endif
#include <GL/glew.h>
#include <SDL.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <FastNoiseSIMD.h>
#include <physfs.hpp>
#include <cpptoml.h>
#include <entt/entt.hpp>

#include <functional>
#include <cmath>
#include <sstream>

#include "util/helpers.h"
#include "util/logging.h"
#include "util/clock.h"
#include "graphics/shader.h"
#include "graphics/textures.h"
#include "graphics/mesh.h"
#include "graphics/camera.h"




std::map<GLenum,std::string> GL_ERROR_STRINGS = {
    {GL_INVALID_ENUM, "GL_INVALID_ENUM"},
    {GL_INVALID_VALUE, "GL_INVALID_VALUE"},
    {GL_INVALID_OPERATION, "GL_INVALID_OPERATION"},
    {GL_STACK_OVERFLOW, "GL_STACK_OVERFLOW"},
    {GL_STACK_UNDERFLOW, "GL_STACK_UNDERFLOW"},
    {GL_OUT_OF_MEMORY, "GL_OUT_OF_MEMORY"},
    {GL_INVALID_FRAMEBUFFER_OPERATION, "GL_INVALID_FRAMEBUFFER_OPERATION"}
};


void setupPhysFS (const char* argv0)
{
    PhysFS::init(argv0);
    // Mount game sources to search path
    {
        std::vector<std::string> paths = {"game/", "game.data"};
        for (auto path : paths) {
            PhysFS::mount(path, "/", 1);
        }
    }
}

struct Config_tag {};
struct Metrics_tag {};
using config = semi::static_map<std::string, int, Config_tag>;
using metrics = semi::static_map<std::string, float, Metrics_tag>;

#include <spdlog/fmt/fmt.h>

using Tilesets = std::pair<std::vector<GLuint>, std::map<entt::hashed_string::hash_type, int>>;

const Tilesets loadImagesets (const std::string& config_file)
{
    std::vector<GLuint> texture_arrays;
    std::map<entt::hashed_string::hash_type, int> texture_units;
    try {
        std::istringstream iss;
        iss.str(helpers::readToString(config_file));
        cpptoml::parser parser{iss};
        std::shared_ptr<cpptoml::table> config = parser.parse();
        auto tarr = config->get_table_array("imageset");
        int tileset_idx = 0;
        for (const auto& imageset_table : *tarr) {
            auto name = imageset_table->get_as<std::string>("id");
            auto id = entt::hashed_string{name->data()};
            texture_units[id] = tileset_idx;
            std::vector<std::string> tiles;
            
            auto images = imageset_table->get_table_array("images");
            for (const auto& image_table : *images) {
                auto directory = image_table->get_as<std::string>("directory");
                auto pattern = image_table->get_as<std::string>("file-pattern");
                auto range = image_table->get_array_of<int64_t>("file-range");
                for (auto i = (*range)[0]; i <= (*range)[1]; ++i) {
                    tiles.push_back(*directory + fmt::format(*pattern, i));
                }
            }
            info("Loading {} images for tileset '{}'", tiles.size(), id);
            glActiveTexture(GL_TEXTURE0 + tileset_idx++);
            auto texture_array = textures::loadArray(tiles);
            glBindTexture(GL_TEXTURE_2D_ARRAY, texture_array);
            texture_arrays.push_back(texture_array);
        }
    }
    catch (const cpptoml::parse_exception& e) {
        fatal("Parsing failed: {}", e.what());
    }
    return {texture_arrays, texture_units};
}

void unloadTilesets (const Tilesets& tilesets) {
    glDeleteTextures(tilesets.first.size(), tilesets.first.data());
}

class Surface {
public:
    Surface(graphics::mesh mesh, glm::mat4 model_matrix, int texture_unit) :
        mesh(mesh),
        model_matrix(std::move(model_matrix)),
        texture_unit(texture_unit)
    {}

    inline void draw (const graphics::uniform& u_model, const graphics::uniform& u_tileset) const {
        u_model.set(model_matrix);
        u_tileset.set(texture_unit);
        mesh.bind();
        mesh.draw();
    }

    inline void unload () {
        mesh.unload();
    }

private:
    graphics::mesh mesh;
    glm::mat4 model_matrix;
    int texture_unit;
};

std::vector<Surface> loadLevel (const Tilesets& tilesets, const std::string& config_file)
{
    std::vector<Surface> surfaces;
    try {
        std::istringstream iss;
        iss.str(helpers::readToString(config_file));
        cpptoml::parser parser{iss};
        std::shared_ptr<cpptoml::table> config = parser.parse();
        auto tarr = config->get_table_array("surface");
        for (const auto& table : *tarr) {
            auto position = table->get_array_of<double>("position");
            auto rotate = table->get_array_of<double>("rotate");
            auto tileset_name = table->get_as<std::string>("tileset");
            auto tile_data = table->get_array_of<cpptoml::array>("tiles");

            auto id = entt::hashed_string{tileset_name->data()};
            auto tileset_idx = tilesets.second.at(id);

            auto pos = glm::vec3((*position)[0], (*position)[1], (*position)[2]);
            glm::mat4 matrix = glm::mat4(1);
            matrix = glm::translate(matrix, pos);
            matrix = glm::rotate(matrix, glm::radians(float((*rotate)[0])), glm::vec3(1, 0, 0));
            matrix = glm::rotate(matrix, glm::radians(float((*rotate)[1])), glm::vec3(0, 1, 0));
            matrix = glm::rotate(matrix, glm::radians(float((*rotate)[2])), glm::vec3(0, 0, 1));

            std::vector<glm::vec3> vertices;
            std::vector<glm::vec3> textureCoordinates;
            float row = tile_data->size();
            for (const auto& row_data : *tile_data) {
                float col = 0;
                auto col_data = row_data->get_array_of<int64_t>();
                for (const auto& layer : *col_data) {
                    vertices.push_back({col,   row  , 0});
                    vertices.push_back({col,   row-1, 0});
                    vertices.push_back({col+1, row-1, 0});
                    vertices.push_back({col+1, row-1, 0});
                    vertices.push_back({col+1, row  , 0});
                    vertices.push_back({col,   row  , 0});

                    textureCoordinates.push_back(glm::vec3(0, 0, layer));
                    textureCoordinates.push_back(glm::vec3(0, 1, layer));
                    textureCoordinates.push_back(glm::vec3(1, 1, layer));
                    textureCoordinates.push_back(glm::vec3(1, 1, layer));
                    textureCoordinates.push_back(glm::vec3(1, 0, layer));
                    textureCoordinates.push_back(glm::vec3(0, 0, layer));

                    ++col;
                }
                --row;
            }

            graphics::mesh mesh;
            mesh.bind();
            mesh.addBuffer(vertices, true);
            mesh.addBuffer(textureCoordinates);
            surfaces.push_back({mesh, matrix, tileset_idx});
        }
    }
    catch (const cpptoml::parse_exception& e) {
        fatal("Parsing failed: {}", e.what());
    }
    return surfaces;
}

void unloadLevel (std::vector<Surface>& surfaces)
{
    for (auto& surface : surfaces) {
        surface.unload();
    }
    surfaces.clear();
}

int main (int argc, char* argv[])
{
    logging::init();
    info("setting up physicsfs");
    setupPhysFS(argv[0]);
    info("setting up SDL");
    try {
        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER) < 0)
        {
            fatal("Failed to initialise SDL");
        }
        on_exit_scope = [](){ SDL_Quit(); };
        
        // Set the OpenGL attributes for our context
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
        SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_BUFFER_SIZE, 32);
        SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
        SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
        SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
        SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);

        // Create a centered window, using system configuration
        auto window = helpers::ptr<SDL_Window>(SDL_CreateWindow, SDL_DestroyWindow).construct(
                    "BloodFarm",
                    SDL_WINDOWPOS_CENTERED,
                    SDL_WINDOWPOS_CENTERED,
                    640,
                    480,
                    SDL_WINDOW_OPENGL);
        
        SDL_GLContext context = SDL_GL_CreateContext(window.get());
        on_exit_scope = [&context](){ SDL_GL_DeleteContext(context); };
        info("Created window with OpenGL {}", glGetString(GL_VERSION));

        glViewport(0, 0, int(640), int(480));

        // Load OpenGL 3+ functions
        glewExperimental = GL_TRUE;
        glewInit();

#ifdef DEBUG_BUILD
        int max_tex_layers, max_combined_tex, max_vert_tex, max_geom_tex, max_frag_tex, max_tex_size;
        glGetIntegerv(GL_MAX_ARRAY_TEXTURE_LAYERS, &max_tex_layers);
        glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &max_combined_tex);
        glGetIntegerv(GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS, &max_vert_tex);
        glGetIntegerv(GL_MAX_GEOMETRY_TEXTURE_IMAGE_UNITS, &max_geom_tex);
        glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &max_frag_tex);
        glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max_tex_size);
        debug("Texture limits: {} combined units, {}/{}/{} vs/gs/fs units, {} array layers, {}x{} max size", max_combined_tex, max_vert_tex, max_geom_tex, max_frag_tex, max_tex_layers, max_tex_size, max_tex_size);
        int max_vert_uniform_vec, max_frag_uniform_vec, max_varying_vec, max_vertex_attribs;
        glGetIntegerv(GL_MAX_VERTEX_UNIFORM_VECTORS, &max_vert_uniform_vec);
        glGetIntegerv(GL_MAX_FRAGMENT_UNIFORM_VECTORS, &max_frag_uniform_vec);
        glGetIntegerv(GL_MAX_VARYING_VECTORS, &max_varying_vec);
        glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &max_vertex_attribs);
        debug("Shader limits: {} vertex attributes, {} varying vectors, {} vertex vectors, {} fragment vectors", max_vertex_attribs, max_varying_vec, max_vert_uniform_vec, max_frag_uniform_vec);
#endif

        SDL_GameController* gameController;
        {
            std::string controllerMapping = helpers::readToString("gamecontrollerdb.txt");
            if (SDL_GameControllerAddMappingsFromRW(SDL_RWFromMem(controllerMapping.data(), controllerMapping.size()), 0) < 0) {
                fatal("Could not read gamepad mapping database.");
            } 
        }

        // auto myNoise = helpers::ptr<FastNoiseSIMD>(FastNoiseSIMD::NewFastNoiseSIMD).construct(1337);
        // auto noiseSet = helpers::ptr<float>(myNoise->GetSimplexFractalSet(0, 0, 0, 16, 16, 16));

        // Set OpenGL settings
        glEnable(GL_DEPTH_TEST);
        glDepthMask(GL_TRUE);
        glDepthFunc(GL_LEQUAL);
        glDisable(GL_BLEND);
        glDisable(GL_STENCIL_TEST);
        glClearDepth(1.0);
        glEnable(GL_CULL_FACE);
        glDisable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        glFrontFace(GL_CCW);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glEnable(GL_MULTISAMPLE);

        auto tilesets = loadImagesets("imagesets.toml");

        info("Loading shader");
        auto myShader = graphics::shader::load({
            {graphics::shader::types::Vertex,   "shaders/model.vert"},
            {graphics::shader::types::Fragment, "shaders/model.frag"},
        });

        auto level = loadLevel(tilesets, "maps/level.toml");

        glm::mat4 projection_matrix = glm::perspective(glm::radians(60.0f), 640.0f / 480.0f, 0.1f, 100.0f);
        graphics::camera camera{0.0f, 2.0f, 10.0f};

        myShader.use();
        myShader.uniform("projection").set(projection_matrix);

        auto model_uniform = myShader.uniform("model");
        auto texture_uniform = myShader.uniform("texture_albedo");

        SDL_Event event;
        bool running = true;

        // Initialise timekeeping
        DeltaTime_t frame_time = 0;
        ElapsedTime_t time_since_start = 0L; // microseconds
        auto start_time = Clock::now();
        auto previous_time = start_time;
        auto current_time = start_time;

        Uint8 current_button_states[SDL_CONTROLLER_BUTTON_MAX];
        Uint8 prev_button_states[SDL_CONTROLLER_BUTTON_MAX];
        float axis_values[SDL_CONTROLLER_AXIS_MAX];

        info("Ready");
        // Run the main processing loop
        do {
            trace_block("gameloop");
            camera.beginFrame(frame_time);

            // Gather and dispatch input
            while (SDL_PollEvent(&event))
            {
                switch (event.type) {
                    case SDL_QUIT:
                        running = false;
                        break;
                    case SDL_KEYDOWN:
                        switch (event.key.keysym.sym)
                        {
                        case SDLK_ESCAPE:
                            running = false;
                            break;
                        default:
                            break;
                        };
                        break;
                    case SDL_CONTROLLERDEVICEADDED:
                        gameController = SDL_GameControllerOpen(event.cdevice.which);
                        if (gameController == 0) {
                            error("Could not open gamepad {}: {}", event.cdevice.which, SDL_GetError());
                        } else {
                            info("Gamepad detected {}", SDL_GameControllerName(gameController));
                        }
                        break;
                    case SDL_CONTROLLERDEVICEREMOVED:
                        SDL_GameControllerClose(gameController);
                        break;
                    case SDL_CONTROLLERBUTTONDOWN:
                    case SDL_CONTROLLERBUTTONUP:
                        prev_button_states[event.cbutton.button] = current_button_states[event.cbutton.button];
                        current_button_states[event.cbutton.button] = event.cbutton.state;
                        break;
                    case SDL_CONTROLLERAXISMOTION:
                    {
                        float value = float(event.caxis.value) * 0.00003052;
                        if (std::fabs(value) > 0.15f /* TODO: configurable deadzones */) {
                            axis_values[event.caxis.axis] = value;
                        } else {
                            axis_values[event.caxis.axis] = 0.0f;
                        }
                        break;
                    }
                    default:
                        break;
                };
            }


            camera.panX(axis_values[SDL_CONTROLLER_AXIS_LEFTX] * 5.0f);
            camera.panZ(axis_values[SDL_CONTROLLER_AXIS_LEFTY] * 5.0f);
            if (current_button_states[SDL_CONTROLLER_BUTTON_LEFTSHOULDER]) {
                camera.tiltBy(-45.0f);
            }
            if (current_button_states[SDL_CONTROLLER_BUTTON_RIGHTSHOULDER]) {
                camera.tiltBy(45.0f);
            }

            glm::mat4 view = camera.view();


            // glClearColor(0.3f, 0.3f, 0.3f, 1.0f);
            glClearColor(0, 0, 0, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            glEnable(GL_BLEND);
            myShader.use();
            myShader.uniform("view").set(view);

            for (const auto& surface : level) {
                surface.draw(model_uniform, texture_uniform);
            }

            SDL_GL_SwapWindow(window.get());

            // Update timekeeping
            previous_time = current_time;
            current_time = Clock::now();
            frame_time = std::chrono::duration_cast<DeltaTime>(current_time - previous_time).count();
            auto frame_time_micros = std::chrono::duration_cast<std::chrono::microseconds>(current_time - previous_time).count();
            if (frame_time > 1.0f) {
                // If frame took over a second, assume debugger breakpoint
                previous_time = current_time;
            } else {
                // Accumulate time. Do this rather than measuring from the start time so that we can 'omit' time, eg when paused.
                time_since_start += frame_time_micros;
            }
        } while (running);

        unloadLevel(level);

    } catch (std::exception& e) {
        error("Uncaught exception: {}", e.what());
        error("Terminating.");
    }
    PhysFS::deinit();
    logging::term();
}