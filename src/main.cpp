#include <iostream>
#include <FastNoiseSIMD.h>
#include <glm/glm.hpp>

#ifndef GL3_PROTOTYPES
#define GL3_PROTOTYPES 1
#endif
#include <GL/glew.h>
#include <SDL.h>

#include "logging.h"
#include "shader.h"

#include <functional>

struct [[nodiscard]] exit_scope_obj {
    template <typename Lambda>
    exit_scope_obj(Lambda& f) : func(f) {}
    template <typename Lambda>
    exit_scope_obj(Lambda&& f) : func(std::move(f)) {}
    ~exit_scope_obj() {func();}
private:
    std::function<void()> func;
};
#define CONCAT_IDENTIFIERS_(a,b) a ## b
#define ON_SCOPE_EXIT_(name,num) exit_scope_obj CONCAT_IDENTIFIERS_(name, num)
#define on_exit_scope ON_SCOPE_EXIT_(exit_scope_obj_, __LINE__)

template <typename T, typename Ctor, typename Dtor>
struct unique_maker_obj {
    explicit unique_maker_obj (Ctor ctor, Dtor dtor) : ctor(ctor), dtor(dtor) {}

    template <typename... Args>
    std::unique_ptr<T, Dtor> make (Args... args) {
        return std::unique_ptr<T, Dtor>(ctor(args...), dtor);
    }
private:
    Ctor ctor;
    Dtor dtor;
};

template <typename T, typename Ctor>
unique_maker_obj<T, Ctor, void(*)(T*)> unique (Ctor ctor) {
    return unique_maker_obj<T, Ctor, void(*)(T*)>(ctor, [](T* p){ info("deleted"); delete p; });
}
template <typename T, typename Ctor, typename Dtor>
unique_maker_obj<T, Ctor, void(*)(T*)> unique (Ctor ctor, Dtor dtor) {
    return unique_maker_obj<T, Ctor, void(*)(T*)>(ctor, dtor);
}
template <typename T>
std::unique_ptr<T> unique (T* raw) {
    return std::unique_ptr<T>{raw};
}


int main (int argc, char* argv[])
{
    logging::init();
    try {
        if (SDL_Init(SDL_INIT_VIDEO) < 0)
        {
            fatal("Failed to initialise SDL");
        }
        on_exit_scope = [](){ info("SDL_Quit"); SDL_Quit(); };
        
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
        auto window = unique<SDL_Window>(SDL_CreateWindow, SDL_DestroyWindow).make(
                    "BloodFarm",
                    SDL_WINDOWPOS_CENTERED,
                    SDL_WINDOWPOS_CENTERED,
                    640,
                    480,
                    SDL_WINDOW_OPENGL);
        
        SDL_GLContext context = SDL_GL_CreateContext(window.get());
        on_exit_scope = [&context](){ SDL_GL_DeleteContext(context); };

        // Load OpenGL 3+ functions
        glewExperimental = GL_TRUE;
        glewInit();

        auto myNoise = unique<FastNoiseSIMD>(FastNoiseSIMD::NewFastNoiseSIMD).make(1337);
        auto noiseSet = unique<float>(myNoise->GetSimplexFractalSet(0, 0, 0, 16, 16, 16));

        shader::shader myShader = shader::load("data/shaders/model.vert", "data/shaders/model.frag");

        info("Ready");
        SDL_Event event;
        bool running = true;
        // Run the main processing loop
        do {
            // Gather and dispatch input
            while (SDL_PollEvent(&event))
            {
                if (event.type == SDL_QUIT) {
                    running = false;
                }
            }
            SDL_GL_SwapWindow(window.get());
        } while (running);

    } catch (std::exception& e) {
        error("Uncaught exception: {}", e.what());
        error("Terminating.");
    }
}