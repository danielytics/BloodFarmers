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
#include <cxxopts.hpp>

#include <functional>
#include <cmath>
#include <sstream>
#include <random>

#include "util/helpers.h"
#include "util/logging.h"
#include "util/clock.h"

#include "math/basic.h"

#include "graphics/shader.h"
#include "graphics/mesh.h"
#include "graphics/camera.h"
#include "graphics/imagesets.h"
#include "graphics/spritepool.h"

#include "graphics/generators/surfaces.h"

#include "ecs/systems/sprite_render.h"
#include "ecs/systems/sprite_animation.h"
#include "ecs/systems/physics_simulation.h"

#include "services/locator.h"
#include "services/core/resources.h"
#include "services/core/renderer.h"
#include "services/core/physics.h"

#include "physics/engine.h"


std::map<GLenum,std::string> GL_ERROR_STRINGS = {
    {GL_INVALID_ENUM, "GL_INVALID_ENUM"},
    {GL_INVALID_VALUE, "GL_INVALID_VALUE"},
    {GL_INVALID_OPERATION, "GL_INVALID_OPERATION"},
    {GL_STACK_OVERFLOW, "GL_STACK_OVERFLOW"},
    {GL_STACK_UNDERFLOW, "GL_STACK_UNDERFLOW"},
    {GL_OUT_OF_MEMORY, "GL_OUT_OF_MEMORY"},
    {GL_INVALID_FRAMEBUFFER_OPERATION, "GL_INVALID_FRAMEBUFFER_OPERATION"}
};

void setupPhysFS (const char* argv0, std::vector<std::string> sourcePaths)
{
    PhysFS::init(argv0);
    // Mount game sources to search path
    {
        for (auto path : sourcePaths) {
            debug("Adding {} to search path", path);
            PhysFS::mount(path, "/", 1);
        }
    }
}

std::vector<graphics::Surface> loadLevel (const graphics::Imagesets& imagesets, const std::string& config_file)
{
    graphics::generators::SurfacesGen generator(imagesets);
    try {
        std::istringstream iss;
        iss.str(helpers::readToString(config_file));
        cpptoml::parser parser{iss};
        std::shared_ptr<cpptoml::table> config = parser.parse();
        auto tarr = config->get_table_array("surface");
        for (const auto& table : *tarr) {
            auto position = table->get_array_of<double>("position");
            auto rotate = table->get_array_of<double>("rotate");
            auto imageset_name = table->get_as<std::string>("imageset");
            auto tile_data = table->get_array_of<cpptoml::array>("tiles");

            auto pos = glm::vec3((*position)[0], (*position)[1], (*position)[2]);
            glm::mat4 matrix = glm::mat4(1);
            matrix = glm::translate(matrix, pos);
            matrix = glm::rotate(matrix, glm::radians(float((*rotate)[0])), glm::vec3(1, 0, 0));
            matrix = glm::rotate(matrix, glm::radians(float((*rotate)[1])), glm::vec3(0, 1, 0));
            matrix = glm::rotate(matrix, glm::radians(float((*rotate)[2])), glm::vec3(0, 0, 1));

            generator.newSurface(entt::hashed_string{imageset_name->data()}, tile_data->size());
            for (const auto& row_data : *tile_data) {
                auto col_data = row_data->get_array_of<int64_t>();
                generator.addRow<int64_t>(*col_data, [&matrix](auto vec){return matrix * vec;});
            }
        }
        return generator.complete();
    }
    catch (const cpptoml::parse_exception& e) {
        fatal("Parsing failed: {}", e.what());
    }
}

void unloadLevel (std::vector<graphics::Surface>& surfaces)
{
    for (auto& surface : surfaces) {
        surface.unload();
    }
    surfaces.clear();
}

struct Settings {
    std::vector<std::string> sources;
    std::string log_level;

    bool start;
};

Settings readSettings (int argc, char* argv[])
{
    Settings settings;
    cxxopts::Options options("BloodFarm", "Game Engine");
    options.add_options()
#ifdef DEBUG_BUILD
        ("d,debug", "Enable debug rendering")
        ("p,profiling", "Enable profiling")
#endif
        ("l,loglevel", "Log level", cxxopts::value<std::string>())
        ("i,init", "Initialisation file", cxxopts::value<std::string>()->default_value("init.toml"));
    auto result = options.parse(argc, argv);

    auto config = cpptoml::parse_file(result["init"].as<std::string>());
    auto telemetry = config->get_table("telemetry");
    if (result["loglevel"].count() == 0) {
        settings.log_level = *telemetry->get_as<std::string>("logging");
    } else {
        settings.log_level = result["loglevel"].as<std::string>();
    }
    auto game = config->get_table("game");
    auto sources = game->get_array_of<std::string>("sources");
    for (const auto& source : *sources) {
        settings.sources.push_back(source);
    }
    return settings;
}

int main (int argc, char* argv[])
{
    Settings settings = readSettings(argc, argv);
    logging::init(settings.log_level);
    setupPhysFS(argv[0], settings.sources);
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

        glm::vec4 viewport = glm::vec4(0, 0, 640, 480);
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
        info("Texture limits: {} combined units, {}/{}/{} vs/gs/fs units, {} array layers, {}x{} max size", max_combined_tex, max_vert_tex, max_geom_tex, max_frag_tex, max_tex_layers, max_tex_size, max_tex_size);
        int max_vert_uniform_vec, max_frag_uniform_vec, max_varying_vec, max_vertex_attribs;
        glGetIntegerv(GL_MAX_VERTEX_UNIFORM_VECTORS, &max_vert_uniform_vec);
        glGetIntegerv(GL_MAX_FRAGMENT_UNIFORM_VECTORS, &max_frag_uniform_vec);
        glGetIntegerv(GL_MAX_VARYING_VECTORS, &max_varying_vec);
        glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &max_vertex_attribs);
        info("Shader limits: {} vertex attributes, {} varying vectors, {} vertex vectors, {} fragment vectors", max_vertex_attribs, max_varying_vec, max_vert_uniform_vec, max_frag_uniform_vec);
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
        glCullFace(GL_BACK);
        glFrontFace(GL_CCW);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glEnable(GL_MULTISAMPLE);

        glm::mat4 projection_matrix = glm::perspective(glm::radians(60.0f), 640.0f / 480.0f, 0.1f, 100.0f);
        services::locator::resources::set<services::Resources>();
        services::locator::camera::set<graphics::camera>();

        auto physicsEngine = std::make_shared<physics::Engine>();
        services::locator::physics::set(std::shared_ptr<services::Physics>(physicsEngine));
        physicsEngine->init();

        graphics::Imagesets imagesets;
        imagesets.load("imagesets.toml");

        info("Loading shaders");
        auto tiles_shader = graphics::shader::load({
            {graphics::shader::types::Vertex,   "shaders/tiles.vert"},
            {graphics::shader::types::Fragment, "shaders/tiles.frag"},
        });
        tiles_shader.use();
        auto u_tile_pv_matrix = tiles_shader.uniform("projection_view");
        auto u_tile_model_matrix = tiles_shader.uniform("model");
        auto u_tile_texture = tiles_shader.uniform("texture_albedo");

        auto spritepool_shader = graphics::shader::load({
            {graphics::shader::types::Vertex,   "shaders/spritepool.vert"},
            {graphics::shader::types::Fragment, "shaders/spritepool.frag"},
        });
        spritepool_shader.use();
        spritepool_shader.uniform("projection").set(projection_matrix);
        auto u_spritepool_view_matrix = spritepool_shader.uniform("view");
        auto u_spritepool_billboarding = spritepool_shader.uniform("billboarding");
        auto u_spritepool_billboarding_spherical = spritepool_shader.uniform("spherical_billboarding");

        bool billboarded_sprites = false;
        bool spherical_billboarding = false;

        info("Loading level");
        auto level = loadLevel(imagesets, "maps/level.toml");

        graphics::SpritePool spritePool;
        spritePool.init(spritepool_shader, imagesets.get("characters"_hs));
        ecs::registry_type registry;

        auto physics_simulation_system = new ecs::systems::physics_simulation;
        auto sprite_animation_system = new ecs::systems::sprite_animation;
        auto sprite_render_system = new ecs::systems::sprite_render(spritePool, spritepool_shader);
        auto systems = std::vector<ecs::system*>{
            physics_simulation_system,
            sprite_animation_system,
            sprite_render_system,
        };
        {
            std::random_device rd;
            std::mt19937 mt(rd());
            std::uniform_real_distribution<float> dist(-50.0f, 50.0f);
            std::uniform_int_distribution<int> rnd(0, 32);
            for (unsigned i=0; i<10; ++i) {
                glm::vec3 position = {dist(mt), 0, dist(mt)-50.0f};
                float base_image = float(rnd(mt)) * 3.0f;
                auto entity = registry.create();
                registry.assign<ecs::components::position>(entity, position);
                registry.assign<ecs::components::sprite>(entity, base_image);
                registry.assign<ecs::components::bitmap_animation>(entity, base_image, 3.f, 0.2f, 0.f, 0);
            }
        }

        SDL_Event event;
        bool running = true;

        // Initialise timekeeping
        DeltaTime_t frame_time = 0;
        ElapsedTime_t time_since_start = 0L; // microseconds
        auto start_time = Clock::now();
        auto previous_time = start_time;
        auto current_time = start_time;
        long total_frames = 0;
        bool buttons_dirty = false;

        Uint8 current_button_states[SDL_CONTROLLER_BUTTON_MAX];
        Uint8 prev_button_states[SDL_CONTROLLER_BUTTON_MAX];
        float axis_values[SDL_CONTROLLER_AXIS_MAX];

        graphics::camera& camera = services::locator::camera::ref();

        info("Ready");
        // Run the main processing loop
        do {
            trace_block("gameloop");
            camera.beginFrame(frame_time);

            if (buttons_dirty) {
                buttons_dirty = false;
                for (std::size_t i=0; i<SDL_CONTROLLER_BUTTON_MAX; ++i) {
                    prev_button_states[i] = current_button_states[i];
                }
            }

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
                        current_button_states[event.cbutton.button] = event.cbutton.state;
                        buttons_dirty = true;
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

            if (current_button_states[SDL_CONTROLLER_BUTTON_DPAD_UP]) {
                camera.move(graphics::camera::Movement::UP_DOWN, 5.0f);
            }
            if (current_button_states[SDL_CONTROLLER_BUTTON_DPAD_DOWN]) {
                camera.move(graphics::camera::Movement::UP_DOWN, -5.0f);
            }
            if (current_button_states[SDL_CONTROLLER_BUTTON_LEFTSHOULDER]) {
                camera.move(graphics::camera::Movement::FORWARD_BACK, axis_values[SDL_CONTROLLER_AXIS_LEFTY] * 5.0f);
                camera.move(graphics::camera::Movement::LEFT_RIGHT, axis_values[SDL_CONTROLLER_AXIS_LEFTX] * 5.0f);
            } else {
                camera.pan({axis_values[SDL_CONTROLLER_AXIS_LEFTX] * 5.0f, 0, axis_values[SDL_CONTROLLER_AXIS_LEFTY] * 5.0f});
            }
            camera.orient(axis_values[SDL_CONTROLLER_AXIS_RIGHTX] * 5.0f, -axis_values[SDL_CONTROLLER_AXIS_RIGHTY] * 5.0f);
            // if (current_button_states[SDL_CONTROLLER_BUTTON_LEFTSHOULDER]) {
            //     camera.tiltBy(-45.0f);
            // }
            // if (current_button_states[SDL_CONTROLLER_BUTTON_RIGHTSHOULDER]) {
            //     camera.tiltBy(45.0f);
            // }
            if (current_button_states[SDL_CONTROLLER_BUTTON_Y] && !prev_button_states[SDL_CONTROLLER_BUTTON_Y]) {
                billboarded_sprites = !billboarded_sprites;
            }
            if (current_button_states[SDL_CONTROLLER_BUTTON_B] && !prev_button_states[SDL_CONTROLLER_BUTTON_B]) {
                spherical_billboarding = !spherical_billboarding;
            }

            if (current_button_states[SDL_CONTROLLER_BUTTON_RIGHTSHOULDER] && !prev_button_states[SDL_CONTROLLER_BUTTON_RIGHTSHOULDER]) {
                auto entity = registry.create();
                registry.assign<ecs::components::position>(entity, glm::vec3(0, 0, 0));
                registry.assign<ecs::components::sprite>(entity, 0.f);
                registry.assign<ecs::components::physics_body>(entity);
            }

            glm::mat4 view_matrix = camera.view();
            glm::mat4 projection_view_matrix = projection_matrix * view_matrix;

            auto frustum = math::frustum(projection_view_matrix);

            physicsEngine->stepSimulation(frame_time);

            sprite_render_system->setView(view_matrix);
            sprite_animation_system->setTime(time_since_start);

            // glClearColor(0.3f, 0.3f, 0.3f, 1.0f);
            glClearColor(0, 0, 0, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            tiles_shader.use();
            u_tile_pv_matrix.set(projection_view_matrix);
            for (const auto& surface : level) {
                surface.draw(u_tile_texture);
            }

            for (auto system : systems) {
                system->run(registry);
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
            ++total_frames;
        } while (running);
        
        auto millis = float(time_since_start) * 0.001f;
        auto seconds = millis * 0.001f;
        info("Average frame time: {} ms", (millis / float(total_frames)));
        info("Average framerate: {} FPS", total_frames / seconds);
        unloadLevel(level);

    } catch (std::exception& e) {
        error("Uncaught exception: {}", e.what());
        error("Terminating.");
    }
    PhysFS::deinit();
    logging::term();
}