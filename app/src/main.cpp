#include "app.h"
#include "utils/clipboard.h"
#include <SDL3/SDL.h>
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_opengl3.h>
#include <stdio.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

static SDL_Window* window = nullptr;
static SDL_GLContext gl_context = nullptr;
static App app;
static bool running = true;

void init_sdl_and_imgui() {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        return;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);
    window = SDL_CreateWindow("PlotDraw", 1280, 720, window_flags);
    if (!window) {
        SDL_Log("SDL_CreateWindow failed: %s", SDL_GetError());
        return;
    }

    gl_context = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, gl_context);
    SDL_GL_SetSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui::StyleColorsDark();

    ImGui_ImplSDL3_InitForOpenGL(window, gl_context);
#ifdef __EMSCRIPTEN__
    ImGui_ImplOpenGL3_Init("#version 300 es");
#else
    ImGui_ImplOpenGL3_Init("#version 330");
#endif

    // After the backend init, not before: ImGui_ImplSDL3_InitForOpenGL points these at
    // SDL_Set/GetClipboardText, which on Emscripten is an in-process buffer that never reaches
    // the browser. Overriding here is what makes Ctrl+C and Ctrl+V cross the app boundary.
    // These are the ImGuiPlatformIO hooks, not the same-named ImGuiIO fields -- those are the
    // obsolete pair and setting them compiles but does nothing.
    ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();
    platform_io.Platform_SetClipboardTextFn = [](ImGuiContext*, const char* text) {
        clipboard_set_text(text ? text : "");
    };
    platform_io.Platform_GetClipboardTextFn = [](ImGuiContext*) {
        return clipboard_get_text();
    };

    float dpi_scale = SDL_GetDisplayContentScale(SDL_GetDisplayForWindow(window));
    app.init();
}

void main_loop() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        ImGui_ImplSDL3_ProcessEvent(&event);
        if (event.type == SDL_EVENT_QUIT) {
            running = false;
        }
    }

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    app.update();
    app.render();

    ImGui::Render();
    SDL_GL_MakeCurrent(window, gl_context);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    SDL_GL_SwapWindow(window);
}

void cleanup() {
    app.shutdown();
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
    SDL_GL_DestroyContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

#ifdef __EMSCRIPTEN__
void emscripten_loop() {
    main_loop();
}
#endif

int main(int argc, char* argv[]) {
    init_sdl_and_imgui();

#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop(emscripten_loop, 0, 1);
#else
    while (running) {
        main_loop();
    }
#endif

    cleanup();
    return 0;
}
