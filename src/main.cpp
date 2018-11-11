
#include <FastNoiseSIMD.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#ifndef GL3_PROTOTYPES
#define GL3_PROTOTYPES 1
#endif
#include <GL/glew.h>
#include <SDL.h>

#include "util/helpers.h"
#include "util/logging.h"
#include "util/clock.h"
#include "graphics/shader.h"
#include "graphics/textures.h"
#include "graphics/mesh.h"

#include <physfs.hpp>

#include <functional>


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
        SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 0);
        SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 0);

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

        SDL_GameController* gameController;
        {
            std::string controllerMapping = helpers::readToString("gamecontrollerdb.txt");
            if (SDL_GameControllerAddMappingsFromRW(SDL_RWFromMem(controllerMapping.data(), controllerMapping.size()), 0) < 0) {
                fatal("Could not read gamepad mapping database.");
            } 
        }

        // auto myNoise = helpers::ptr<FastNoiseSIMD>(FastNoiseSIMD::NewFastNoiseSIMD).construct(1337);
        // auto noiseSet = helpers::ptr<float>(myNoise->GetSimplexFractalSet(0, 0, 0, 16, 16, 16));
        
        glActiveTexture(GL_TEXTURE0);
        auto tilesets = textures::loadArray(std::vector<std::string>{
            "images/tileset0.png",
            "images/tileset1.png",
            "images/tileset2.png",
        });
        info("Images loaded. Binding.");
        glBindTexture(GL_TEXTURE_2D_ARRAY, tilesets);
        on_exit_scope = [tilesets](){ glDeleteTextures(1, &tilesets); };

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

        info("Loading shader");
        auto myShader = graphics::shader::load("shaders/model.vert", "shaders/model.frag");

        graphics::mesh mesh;
        mesh.bind();
        mesh.addBuffer(std::vector<glm::vec3>{
            {-1.0f,  1.0f, 0.0f},
            {-1.0f, -1.0f, 0.0f},
            {1.0f,  1.0f, 0.0f},
            {1.0f,  1.0f, 0.0f},
            {-1.0f, -1.0f, 0.0f},
            {1.0f, -1.0f, 0.0f},
        }, true);
        mesh.addBuffer(std::vector<glm::vec3>{
            {0.0f, 0.0f, -1.0f},
            {0.0f, 0.0f, -1.0f},
            {0.0f, 0.0f, -1.0f},
            {0.0f, 0.0f, -1.0f},
            {0.0f, 0.0f, -1.0f},
            {0.0f, 0.0f, -1.0f},
        });
        mesh.addBuffer(std::vector<glm::vec2>{
            // {0.0f, 0.0f},
            // {0.0f, 0.0238f},
            // {0.0238f, 0.0f},
            // {0.0238f, 0.0f},
            // {0.0f, 0.0238f},
            // {0.0238f, 0.0238f},

            {0.0f, 0.0f},
            {0.0f, 1.0f},
            {1.0f, 0.0f},
            {1.0f, 0.0f},
            {0.0f, 1.0f},
            {1.0f, 1.0f},
        });

        glm::vec3 camera = glm::vec3(0.0f, 0.0f, 10.0f);
        glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
        glm::mat4 projection_matrix = glm::perspective(glm::radians(60.0f), 640.0f / 480.0f, 0.1f, 20.0f);

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

        Uint8 buttonStates[2][SDL_CONTROLLER_BUTTON_MAX];
        Uint8 buttonStatesPrev[SDL_CONTROLLER_BUTTON_MAX];
        float axis_values[SDL_CONTROLLER_AXIS_MAX];
        Uint8* current_button_states = buttonStates[0];
        Uint8* prev_button_states = buttonStates[1];

        do {
            trace_block("gameloop");
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
            camera.x += axis_values[SDL_CONTROLLER_AXIS_LEFTX] * 5.0f * frame_time;
            camera.z += axis_values[SDL_CONTROLLER_AXIS_LEFTY] * 5.0f * frame_time;

            glm::mat4 view = glm::lookAt(camera, glm::vec3{camera.x, camera.y, camera.z - 1.0f}, up);
            glm::mat4 model_matrix = glm::mat4(1.0);
            model_matrix = glm::translate(model_matrix, glm::vec3(0.0f, 0.0f, -2.0f)); // translate it down so it's at the center of the scene

            glClearColor(0.3f, 0.3f, 0.3f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            glEnable(GL_BLEND);
            myShader.use();
            myShader.uniform("projection").set(projection_matrix);
            myShader.uniform("view").set(view);
            myShader.uniform("model").set(model_matrix);
            myShader.uniform("texture").set(0);
            mesh.bind();
            mesh.draw();

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