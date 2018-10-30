#include <iostream>
#include <FastNoiseSIMD.h>
#include <glm/glm.hpp>

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


int main (int argc, char* argv[])
{
    logging::init();
    info("setting up physicsfs");
    setupPhysFS(argv[0]);
    info("setting up SDL");
    try {
        if (SDL_Init(SDL_INIT_VIDEO) < 0)
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

        glViewport(0, 0, int(640), int(480));

        // Load OpenGL 3+ functions
        glewExperimental = GL_TRUE;
        glewInit();

        auto myNoise = helpers::ptr<FastNoiseSIMD>(FastNoiseSIMD::NewFastNoiseSIMD).construct(1337);
        auto noiseSet = helpers::ptr<float>(myNoise->GetSimplexFractalSet(0, 0, 0, 16, 16, 16));
        
        glActiveTexture(GL_TEXTURE0+5);
        auto tilesets = textures::loadArray(std::vector<std::string>{
            "images/tileset0.png",
            "images/tileset1.png",
            "images/tileset2.png",
        });
        info("Images loaded. Binding.");
        glBindTexture(GL_TEXTURE_2D_ARRAY, tilesets);
        on_exit_scope = [tilesets](){ glDeleteTextures(1, &tilesets); };

        info("Loading shader");
        graphics::shader_t myShader = shaders::load("shaders/model.vert", "shaders/model.frag");

        graphics::mesh mesh;
        mesh.bind();
        mesh.addBuffer(std::vector<glm::vec3>{
            {-1.0f,  1.0f, 0.0f},
            {-1.0f, -1.0f, 0.0f},
            {1.0f,  1.0f, 0.0f},
            {1.0f, -1.0f, 0.0f}
        }, true);

        glm::vec3 camera = glm::vec3(0.0f, 0.0f, 10.0f);
        glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);

        info("Ready");
        SDL_Event event;
        bool running = true;
        // Initialise timekeeping
        DeltaTime_t frame_time = 0;
        ElapsedTime_t time_since_start = 0L;
        auto start_time = Clock::now();
        auto previous_time = start_time;
        auto current_time = start_time;
        // Run the main processing loop
        do {
            trace_block("gameloop");
            // Gather and dispatch input
            while (SDL_PollEvent(&event))
            {
                if (event.type == SDL_QUIT) {
                    running = false;
                }
                if (event.type == SDL_KEYDOWN)
                {
                    switch (event.key.keysym.sym)
                    {
                    case SDLK_ESCAPE:
                        running = false;
                        break;
                    default:
                        break;
                    };
                }
            }
            glm::mat4 view = glm::lookAt(camera, glm::vec3{camera.x, camera.y, camera.z - 1.0f}, up);

            myShader.use();
            mesh.bind();
            mesh.draw();
            SDL_GL_SwapWindow(window.get());
            // Update timekeeping
            previous_time = current_time;
            current_time = Clock::now();
            frame_time = std::chrono::duration_cast<DeltaTime>(current_time - previous_time).count();
            if (frame_time > 1.0f) {
                // If frame took over a second, assume debugger breakpoint
                previous_time = current_time;
            } else {
                time_since_start += std::chrono::duration_cast<ElapsedTime>(current_time - previous_time).count();
            }
        } while (running);

    } catch (std::exception& e) {
        error("Uncaught exception: {}", e.what());
        error("Terminating.");
    }
    PhysFS::deinit();
    logging::term();
}