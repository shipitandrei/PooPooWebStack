#pragma once

#include <SDL2/SDL.h>

#include <time.h>
#include <vector>

enum class GameState { Menu, Playing };

struct Poo {
    float x;
    float y;
    float speed;
};

class Game {
public:
    Game(SDL_Renderer* renderer);
    ~Game();

    void HandleEvent(const SDL_Event& event, bool& running);
    void Update(float deltaSeconds);
    void Render() const;

private:
    void StartNewGame();
    void ReturnToMenu();
    void OpenController(int deviceIndex);
    void CloseController();
    void SpawnPoo();
    bool PooIsCaught(const Poo& poo) const;
    float MovementInput() const;

    SDL_Renderer* renderer_;
    SDL_GameController* controller_;
    GameState state_;
    std::vector<Poo> poos_;
    float toiletX_;
    float spawnTimer_;
    unsigned int score_;
    unsigned int missed_;
    unsigned int randomState_;
};
