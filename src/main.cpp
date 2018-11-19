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
        std::vector<std::string> paths = {"data/", "game.data"};
        for (auto path : paths) {
            PhysFS::mount(path, "/", 1);
        }
    }
}

struct Config_tag {};
struct Metrics_tag {};
using config = semi::static_map<std::string, int, Config_tag>;
using metrics = semi::static_map<std::string, float, Metrics_tag>;

struct Color {
    float r, g, b;
};

enum class MapSurface {
    Ground,
    Wall,
    SideWall,
};

graphics::mesh generate_map (MapSurface surface, const std::vector<std::vector<std::pair<unsigned int, unsigned int>>>& tiles)
{
    std::vector<glm::vec3> vertices;
    std::vector<glm::vec3> textureCoordinates;
    float z = tiles.size() - 1;
    for (const auto& tile_row : tiles) {
        float x = 0.0f;
        for (const auto pair : tile_row) {
            auto [tile_id, layer] = pair;
            int row = tile_id / 42;
            int col = tile_id - (row * 42);
            float u = (float(col) * 48.0f) * 0.000496031746031746f;
            float v = (float(row) * 48.0f) * 0.000496031746031746;
            u += 0.000248015873015873f;
            v += 0.000248015873015873f;
            debug("xyz: {},{},{} uv: {}, {} l: {}", x, 0, z, u, v, layer);
            if (surface == MapSurface::Ground) {
                vertices.push_back(glm::vec3(x,      0.0f, z));
                vertices.push_back(glm::vec3(x,      0.0f, z+1.0f));
                vertices.push_back(glm::vec3(x+1.0f, 0.0f, z));
                vertices.push_back(glm::vec3(x+1.0f, 0.0f, z));
                vertices.push_back(glm::vec3(x,      0.0f, z+1.0f));
                vertices.push_back(glm::vec3(x+1.0f, 0.0f, z+1.0f));
            } else if (surface == MapSurface::Wall) {
                vertices.push_back(glm::vec3(x,      z,      0.0f));
                vertices.push_back(glm::vec3(x,      z+1.0f, 0.0f));
                vertices.push_back(glm::vec3(x+1.0f, z,      0.0f));
                vertices.push_back(glm::vec3(x+1.0f, z,      0.0f));
                vertices.push_back(glm::vec3(x,      z+1.0f, 0.0f));
                vertices.push_back(glm::vec3(x+1.0f, z+1.0f, 0.0f));
            } else if (surface == MapSurface::SideWall) {
                vertices.push_back(glm::vec3(0.0f, z,      x     ));
                vertices.push_back(glm::vec3(0.0f, z+1.0f, x     ));
                vertices.push_back(glm::vec3(0.0f, z,      x+1.0f));
                vertices.push_back(glm::vec3(0.0f, z,      x+1.0f));
                vertices.push_back(glm::vec3(0.0f, z+1.0f, x     ));
                vertices.push_back(glm::vec3(0.0f, z+1.0f, x+1.0f));
            }
            textureCoordinates.push_back(glm::vec3(u, (v+0.023809523809523808f), layer));
            textureCoordinates.push_back(glm::vec3(u, v, layer));
            textureCoordinates.push_back(glm::vec3((u+0.023809523809523808f), (v+0.023809523809523808f), layer));
            textureCoordinates.push_back(glm::vec3((u+0.023809523809523808f), (v+0.023809523809523808f), layer));
            textureCoordinates.push_back(glm::vec3(u, v, layer));
            textureCoordinates.push_back(glm::vec3((u+0.023809523809523808f), v, layer));
            ++x;
        }
        --z;
    }
    graphics::mesh mesh;
    mesh.bind();
    mesh.addBuffer(vertices, true);
    mesh.addBuffer(textureCoordinates);
    return mesh;
}

#include <spdlog/fmt/fmt.h>

std::vector<GLuint> loadTilesets (const std::string& config_file)
{
    std::vector<GLuint> tilesets;
    try {
        std::istringstream iss;
        iss.str(helpers::readToString(config_file));
        cpptoml::parser parser{iss};
        std::shared_ptr<cpptoml::table> config = parser.parse();
        auto tarr = config->get_table_array("tileset");
        int tileset_idx = 0;
        for (const auto& table : *tarr)
        {
            auto id = table->get_as<std::string>("id");
            auto directory = table->get_as<std::string>("directory");
            auto pattern = table->get_as<std::string>("file-pattern");
            auto range = table->get_array_of<int64_t>("file-range");
            std::vector<std::string> tiles;
            for (auto i = (*range)[0]; i <= (*range)[1]; ++i) {
                tiles.push_back(*directory + fmt::format(*pattern, i));
            }
            info("Loading {} images for tileset '{}'", tiles.size(), *id);
            glActiveTexture(GL_TEXTURE0 + tileset_idx++);
            auto tileset = textures::loadArray(tiles);
            glBindTexture(GL_TEXTURE_2D_ARRAY, tileset);
            tilesets.push_back(tileset);
        }
    }
    catch (const cpptoml::parse_exception& e) {
        fatal("Parsing failed: {}", e.what());
    }
    return tilesets;
}

void unloadTilesets (const std::vector<GLuint>& tilesets) {
    glDeleteTextures(tilesets.size(), tilesets.data());
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
        
        loadTilesets("images/tilesets.toml");

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

        info("Loading shader");
        auto myShader = graphics::shader::load("shaders/model.vert", "shaders/model.frag");

        // #define A(x,y) {((y)*42)+x, 0}
        // #define B(x,y) {((y)*42)+x, 1}
        // #define C(x,y) {((y)*42)+x, 2}
        // auto ground = generate_map(MapSurface::Ground, {
        //     {A(20, 30), A(20, 30), A(20, 30), A(6, 8),  A(6, 8),   A(20, 30), A(20, 30), A(20, 30), A(20, 30), A(20, 30)},
        //     {A(20, 30), A(20, 30), A(20, 30), A(6, 8),  A(6, 8),   A(20, 30), A(20, 30), A(20, 30), A(20, 30), A(20, 30)},
        //     {A(20, 30), A(20, 30), A(20, 30), A(6, 8),  A(6, 8),   A(20, 30), A(20, 30), A(20, 30), A(20, 30), A(20, 30)},
        //     {A(20, 30), A(20, 30), A(20, 30), A(6, 8),  A(6, 8),   A(20, 30), A(20, 30), A(20, 30), A(20, 30), A(20, 30)},
        //     {A(20, 30), A(20, 30), A(20, 30), A(6, 8),  A(6, 8),   A(20, 30), A(20, 30), A(20, 30), A(20, 30), A(20, 30)},
        //     {A(20, 30), A(20, 30), A(20, 30), A(6, 8),  A(6, 8),   A(20, 30), A(20, 30), A(20, 30), A(20, 30), A(20, 30)},
        //     {A(20, 30), A(20, 30), A(20, 30), A(6, 8),  A(6, 8),   A(20, 30), A(20, 30), A(20, 30), A(20, 30), A(20, 30)},
        //     {A(20, 30), A(20, 30), A(20, 30), A(6, 8),  A(6, 8),  A(20, 30), A(20, 30), A(20, 30), A(20, 30), A(20, 30)},
        //     {A(20, 30), A(20, 30), A(20, 30), A(6, 8),  A(6, 8),  A(20, 30), A(20, 30), A(20, 30), A(20, 30), A(20, 30)},
        //     {A(20, 30), A(20, 30), A(20, 30), A(6, 8),  A(6, 8),  A(20, 30), A(20, 30), A(20, 30), A(20, 30), A(20, 30)},
        // });
        // auto wall = generate_map(MapSurface::Wall, {
        //     {C(21, 0),  C(21, 0),  C(21, 0),  C(21, 0),  C(21, 0) },
        //     {C(8, 1),   C(8, 1),   C(8, 1),   C(8, 1),   C(8, 1)  },
        // });
        // auto sideWall = generate_map(MapSurface::SideWall, {
        //     {C(21, 0),  C(21, 0),  C(21, 0),  C(21, 0)},
        //     {C(8, 1),   C(8, 1),   C(8, 1),   C(8, 1) },
        // });
        // #undef A
        // #undef B
        // #undef C

        glm::mat4 projection_matrix = glm::perspective(glm::radians(60.0f), 640.0f / 480.0f, 0.1f, 100.0f);
        graphics::camera camera{0.0f, 2.0f, 10.0f};

        info("Ready");
        SDL_Event event;
        bool running = true;
        // Initialise timekeeping
        DeltaTime_t frame_time = 0;
        ElapsedTime_t time_since_start = 0L; // microseconds
        auto start_time = Clock::now();
        auto previous_time = start_time;
        auto current_time = start_time;
        // Run the main processing loop
        ElapsedTime_t last_change_time = 0;
        std::vector<Color> colors{
            {1.0f, 0.0f, 0.0f},
            {0.0f, 1.0f, 0.0f},
            {0.0f, 0.0f, 1.0f},
            {1.0f, 1.0f, 0.0f},
            {1.0f, 0.0f, 1.0f},
            {0.0f, 1.0f, 1.0f},
        };
        int clear_color_idx = 0;
        float direction = 1.0f;

        Uint8 current_button_states[SDL_CONTROLLER_BUTTON_MAX];
        Uint8 prev_button_states[SDL_CONTROLLER_BUTTON_MAX];
        float axis_values[SDL_CONTROLLER_AXIS_MAX];

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
            glm::mat4 ground_matrix = glm::translate(glm::mat4(1.0), glm::vec3(-5.0f, 0.0f, -2.0f));
            glm::mat4 wall_matrix = glm::translate(glm::mat4(1.0), glm::vec3(0.0f, 0.0f, 3.0f));
            glm::mat4 sidewall_matrix = glm::translate(glm::mat4(1.0), glm::vec3(0.0f, 0.0f, -1.0f));

            glClearColor(0.3f, 0.3f, 0.3f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            glEnable(GL_BLEND);
            myShader.use();
            myShader.uniform("projection").set(projection_matrix);
            myShader.uniform("view").set(view);
            myShader.uniform("model").set(ground_matrix);
            myShader.uniform("texture_albedo").set(0);
            // ground.bind();
            // ground.draw();
            // myShader.uniform("model").set(wall_matrix);
            // wall.bind();
            // wall.draw();
            // myShader.uniform("model").set(sidewall_matrix);
            // sideWall.bind();
            // sideWall.draw();

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

    } catch (std::exception& e) {
        error("Uncaught exception: {}", e.what());
        error("Terminating.");
    }
    PhysFS::deinit();
    logging::term();
}