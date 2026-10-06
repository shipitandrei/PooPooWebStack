#include "Config.h"
#include "Game.h"

#include <SDL2/SDL.h>

int main(int, char**) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER) != 0) return 1;

    SDL_Window* window = SDL_CreateWindow("Poo Poo on the Toilet", SDL_WINDOWPOS_CENTERED,
                                          SDL_WINDOWPOS_CENTERED, Config::kScreenWidth,
                                          Config::kScreenHeight, SDL_WINDOW_SHOWN);
    if (window == nullptr) {
        SDL_Quit();
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (renderer == nullptr) {
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    SDL_RenderSetLogicalSize(renderer, Config::kScreenWidth, Config::kScreenHeight);

    {
        // Destroy the game (and its controller handle) before SDL itself is shut down.
        Game game(renderer);
        bool running = true;
        Uint32 lastTicks = SDL_GetTicks();
        while (running) {
            SDL_Event event;
            while (SDL_PollEvent(&event)) game.HandleEvent(event, running);

            const Uint32 now = SDL_GetTicks();
            float deltaSeconds = static_cast<float>(now - lastTicks) / 1000.0f;
            lastTicks = now;
            if (deltaSeconds > 0.05f) deltaSeconds = 0.05f;  // Avoid a giant movement after a pause.
            game.Update(deltaSeconds);
            game.Render();
        }
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
