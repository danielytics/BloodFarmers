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
#include "graphics/renderer.h"

#include "graphics/generators/surfaces.h"

#include "ecs/systems/sprite_render.h"
#include "ecs/systems/sprite_animation.h"
#include "ecs/systems/physics_simulation.h"

#include "services/locator.h"
#include "services/core/resources.h"
#include "services/core/renderer.h"
#include "services/core/physics.h"

#include "physics/engine.h"

struct BufferAllocator : public services::Resources::Allocator {
    void allocate (std::size_t bytes) {
        memory = reinterpret_cast<intptr_t>(std::malloc(bytes));
        top = 0;
    }
    void deallocate () {
        std::free(reinterpret_cast<void*>(memory));
    }

    void* request (std::size_t alignment, std::size_t size, std::size_t count) {
        intptr_t buffer = reinterpret_cast<intptr_t>(memory) + top;
        void* retval = reinterpret_cast<void*>(buffer);
        for (auto index = 0; index < count; ++index) {
            resources::MemoryBuffer* membuf = reinterpret_cast<resources::MemoryBuffer*>(buffer);
            intptr_t buffer_start = helpers::align(buffer + sizeof(resources::MemoryBuffer), alignment);
            const_cast<std::size_t&>(membuf->capacity) = size;
            const_cast<void*&>(membuf->data) = reinterpret_cast<void*>(buffer_start);
            membuf->count = 0;
            std::size_t used_bytes = (buffer_start - reinterpret_cast<intptr_t>(membuf)) + size;
            top += used_bytes;
            buffer += used_bytes;
        }
        return retval;
    }

    void release (void* buffer) {

    }
private:
    std::size_t top;
    intptr_t memory;
};

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

void setupTypes ()
{
    auto& resources = services::locator::resources::ref();
    
    // Register allocators
    resources.registerAllocator("buffer-allocator"_hs, new BufferAllocator());

    // Register types
    resources.registerType<services::Resources::InvalidType>("invalid-type"_hs);
    resources.registerType<graphics::Sprite>("sprite"_hs);
}

void setupBuffers (const std::string& config_file)
{
    auto& resources = services::locator::resources::ref();
    try {
        std::istringstream iss;
        iss.str(helpers::readToString(config_file));
        cpptoml::parser parser{iss};
        std::shared_ptr<cpptoml::table> config = parser.parse();
        auto tarr = config->get_table_array("buffer-pool");
        for (const auto& table : *tarr) {
            auto id = table->get_as<std::string>("id");
            auto lifecycle = table->get_as<std::string>("lifecycle");
            auto type = table->get_as<std::string>("type");
            auto requests = table->get_as<std::string>("requests");
            auto buffers = table->get_as<int64_t>("buffers");
            auto buffer = table->get_table("buffer");
            auto alignment = buffer->get_as<int64_t>("alignment");
            auto size = buffer->get_as<int64_t>("size");
            auto units = buffer->get_as<std::string>("units");

            std::uint32_t num_bytes = *size;
            std::uint32_t element_size = resources.sizeOf(entt::hashed_string{type->data()});
            if (*units == "b") {
                // No-op
            } else if (*units == "kb") {
                num_bytes *= 1024;
            } else if (*units == "mb") {
                num_bytes *= 1024 * 1024; 
            } else if (*units == "elements") {
                num_bytes = num_bytes * element_size;
            }
            
            // Register the buffer
            resources.registerResource({
                entt::hashed_string{id->data()},        // id
                entt::hashed_string{lifecycle->data()}, // lifecycle
                entt::hashed_string{requests->data()},  // request_type
                "buffer-allocator"_hs,                  // allocator
                entt::hashed_string{type->data()},      // contained_type
                std::uint32_t(*alignment),              // buffer alignment
                num_bytes,                              // size
                std::uint32_t(*buffers),                // num_buffers
            });
        }
    }
    catch (const cpptoml::parse_exception& e) {
        fatal("Parsing failed: {}", e.what());
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

template <typename... Args> inline void null_fn (Args&&...) {}
template <typename... Args>
void initServices (Args&&... args) {
    auto init = [](auto item){item->init(); return 0;};
    null_fn(init(std::forward<Args>(args))...);
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

        // Load OpenGL 3+ functions
        glewExperimental = GL_TRUE;
        glewInit();

        SDL_GameController* gameController;
        {
            std::string controllerMapping = helpers::readToString("gamecontrollerdb.txt");
            if (SDL_GameControllerAddMappingsFromRW(SDL_RWFromMem(controllerMapping.data(), controllerMapping.size()), 0) < 0) {
                fatal("Could not read gamepad mapping database.");
            } 
        }

        // auto myNoise = helpers::ptr<FastNoiseSIMD>(FastNoiseSIMD::NewFastNoiseSIMD).construct(1337);
        // auto noiseSet = helpers::ptr<float>(myNoise->GetSimplexFractalSet(0, 0, 0, 16, 16, 16));

        info("Creating resources service");
        services::locator::resources::set<services::Resources>();
        setupTypes();
        setupBuffers("buffers.toml");
        services::locator::resources::ref().init("static"_hs);
        
        info("Creating rendeding service");
        auto renderer = std::make_shared<graphics::Renderer>();
        services::locator::renderer::set(std::shared_ptr<services::Renderer>(renderer));
        services::locator::camera::set<services::Camera>();

        info("Creating physics service");
        auto physicsEngine = std::make_shared<physics::Engine>();
        services::locator::physics::set(std::shared_ptr<services::Physics>(physicsEngine));

        info("Initialising services");
        initServices(physicsEngine, renderer);

        info("Setting renderer config");
        services::locator::config<"renderer.field-of-view"_hs, float>(60.0f);
        services::locator::config<"renderer.near-distance"_hs, float>(0.1f);
        services::locator::config<"renderer.far-distance"_hs, float>(100.0f);
        services::locator::config<"renderer.width"_hs, float>(640.0f);
        services::locator::config<"renderer.height"_hs, float>(480.0f);
        renderer->windowChanged();

        info("Initialising game systems");
        ecs::registry_type registry;
        auto physics_simulation_system = new ecs::systems::physics_simulation;
        auto sprite_animation_system = new ecs::systems::sprite_animation;
        auto sprite_render_system = new ecs::systems::sprite_render;
        auto systems = std::vector<ecs::system*>{
            physics_simulation_system,
            sprite_animation_system,
            sprite_render_system,
        };

        info("Loading level");
        // auto level = loadLevel(imagesets, "maps/level.toml");

        info("Generating entities");
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

            if (current_button_states[SDL_CONTROLLER_BUTTON_RIGHTSHOULDER] && !prev_button_states[SDL_CONTROLLER_BUTTON_RIGHTSHOULDER]) {
                auto entity = registry.create();
                registry.assign<ecs::components::position>(entity, glm::vec3(0, 0, 0));
                registry.assign<ecs::components::sprite>(entity, 0.f);
                registry.assign<ecs::components::physics_body>(entity);
            }

            physicsEngine->stepSimulation(frame_time);

            sprite_animation_system->setTime(time_since_start);

            for (auto system : systems) {
                system->run(registry);
            }

            renderer->render();

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
        // unloadLevel(level);

    } catch (std::exception& e) {
        error("Uncaught exception: {}", e.what());
        error("Terminating.");
    }
    PhysFS::deinit();
    logging::term();
}