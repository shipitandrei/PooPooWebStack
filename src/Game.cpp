#include "Game.h"

#include "Config.h"

#include <algorithm>
#include <cmath>
#include <string>

namespace {

struct Glyph {
    char character;
    unsigned char rows[7];
};

// Five-pixel-wide block lettering keeps this prototype independent of font files.
constexpr Glyph kGlyphs[] = {
    {'0', {14, 17, 19, 21, 25, 17, 14}}, {'1', {4, 12, 4, 4, 4, 4, 14}},
    {'2', {14, 17, 1, 2, 4, 8, 31}}, {'3', {30, 1, 1, 14, 1, 1, 30}},
    {'4', {2, 6, 10, 18, 31, 2, 2}}, {'5', {31, 16, 16, 30, 1, 1, 30}},
    {'6', {14, 16, 16, 30, 17, 17, 14}}, {'7', {31, 1, 2, 4, 8, 8, 8}},
    {'8', {14, 17, 17, 14, 17, 17, 14}}, {'9', {14, 17, 17, 15, 1, 1, 14}},
    {'A', {14, 17, 17, 31, 17, 17, 17}}, {'C', {14, 17, 16, 16, 16, 17, 14}},
    {'D', {30, 17, 17, 17, 17, 17, 30}}, {'E', {31, 16, 16, 30, 16, 16, 31}},
    {'G', {14, 17, 16, 23, 17, 17, 14}}, {'H', {17, 17, 17, 31, 17, 17, 17}},
    {'I', {31, 4, 4, 4, 4, 4, 31}}, {'L', {16, 16, 16, 16, 16, 16, 31}},
    {'M', {17, 27, 21, 21, 17, 17, 17}}, {'N', {17, 25, 21, 19, 17, 17, 17}},
    {'O', {14, 17, 17, 17, 17, 17, 14}}, {'P', {30, 17, 17, 30, 16, 16, 16}},
    {'R', {30, 17, 17, 30, 20, 18, 17}}, {'S', {15, 16, 16, 14, 1, 1, 30}},
    {'T', {31, 4, 4, 4, 4, 4, 4}}, {'U', {17, 17, 17, 17, 17, 17, 14}},
    {'X', {17, 17, 10, 4, 10, 17, 17}}, {':', {0, 4, 4, 0, 4, 4, 0}},
    {'?', {14, 17, 1, 2, 4, 0, 4}}, {' ', {0, 0, 0, 0, 0, 0, 0}},
};

const Glyph& FindGlyph(char character) {
    for (const Glyph& glyph : kGlyphs) {
        if (glyph.character == character) return glyph;
    }
    return kGlyphs[sizeof(kGlyphs) / sizeof(kGlyphs[0]) - 2];
}

void FillRect(SDL_Renderer* renderer, int x, int y, int width, int height) {
    const SDL_Rect rect = {x, y, width, height};
    SDL_RenderFillRect(renderer, &rect);
}

int TextWidth(const std::string& text, int scale) {
    return static_cast<int>(text.size()) * 6 * scale - scale;
}

void DrawText(SDL_Renderer* renderer, const std::string& text, int x, int y, int scale,
              SDL_Color color) {
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    int cursor = x;
    for (char character : text) {
        const Glyph& glyph = FindGlyph(character);
        for (int row = 0; row < 7; ++row) {
            for (int column = 0; column < 5; ++column) {
                if ((glyph.rows[row] & (1 << (4 - column))) != 0) {
                    FillRect(renderer, cursor + column * scale, y + row * scale, scale, scale);
                }
            }
        }
        cursor += 6 * scale;
    }
}

void DrawCenteredText(SDL_Renderer* renderer, const std::string& text, int y, int scale,
                      SDL_Color color) {
    DrawText(renderer, text, (Config::kScreenWidth - TextWidth(text, scale)) / 2, y, scale, color);
}

void DrawPoo(SDL_Renderer* renderer, const Poo& poo) {
    const int x = static_cast<int>(poo.x);
    const int y = static_cast<int>(poo.y);
    SDL_SetRenderDrawColor(renderer, 103, 57, 31, 255);
    FillRect(renderer, x + 8, y + 22, 22, 10);
    FillRect(renderer, x + 5, y + 15, 25, 10);
    FillRect(renderer, x + 10, y + 8, 16, 10);
    FillRect(renderer, x + 15, y + 3, 7, 7);
    SDL_SetRenderDrawColor(renderer, 132, 80, 42, 255);
    FillRect(renderer, x + 12, y + 11, 10, 3);
}

void DrawToilet(SDL_Renderer* renderer, float toiletX) {
    const int x = static_cast<int>(toiletX);
    const int y = static_cast<int>(Config::kToiletY);
    SDL_SetRenderDrawColor(renderer, 223, 240, 244, 255);
    FillRect(renderer, x + 18, y + 40, 154, 48);  // bowl
    FillRect(renderer, x + 58, y + 83, 75, 33);   // pedestal
    FillRect(renderer, x + 30, y + 11, 130, 32);  // tank
    SDL_SetRenderDrawColor(renderer, 37, 75, 94, 255);
    FillRect(renderer, x + 36, y + 45, 118, 16);  // catch opening
    SDL_SetRenderDrawColor(renderer, 150, 200, 214, 255);
    FillRect(renderer, x + 35, y + 14, 120, 5);
}

}  // namespace

Game::Game(SDL_Renderer* renderer)
    : renderer_(renderer), controller_(nullptr), state_(GameState::Menu),
      toiletX_((Config::kScreenWidth - Config::kToiletWidth) / 2.0f), spawnTimer_(0.0f),
      score_(0), missed_(0), randomState_(0xC0FFEEu) {
    for (int i = 0; i < SDL_NumJoysticks(); ++i) {
        if (SDL_IsGameController(i)) {
            OpenController(i);
            break;
        }
    }
}

Game::~Game() { CloseController(); }

void Game::OpenController(int deviceIndex) {
    if (controller_ == nullptr && SDL_IsGameController(deviceIndex)) {
        controller_ = SDL_GameControllerOpen(deviceIndex);
    }
}

void Game::CloseController() {
    if (controller_ != nullptr) {
        SDL_GameControllerClose(controller_);
        controller_ = nullptr;
    }
}

void Game::HandleEvent(const SDL_Event& event, bool& running) {
    if (event.type == SDL_QUIT) {
        running = false;
    } else if (event.type == SDL_CONTROLLERDEVICEADDED) {
        OpenController(event.cdevice.which);
    } else if (event.type == SDL_CONTROLLERDEVICEREMOVED && controller_ != nullptr &&
               SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(controller_)) == event.cdevice.which) {
        CloseController();
    } else if (event.type == SDL_CONTROLLERBUTTONDOWN) {
        if (event.cbutton.button == SDL_CONTROLLER_BUTTON_A && state_ == GameState::Menu) {
            StartNewGame();  // DualShock 4 Cross maps to SDL's A button.
        } else if (event.cbutton.button == SDL_CONTROLLER_BUTTON_START && state_ == GameState::Playing) {
            ReturnToMenu();  // DualShock 4 Options maps to SDL's Start button.
        }
    } else if (event.type == SDL_KEYDOWN) {
        // Keyboard fallbacks make desktop/container testing convenient; the PS4 uses the controller above.
        if ((event.key.keysym.sym == SDLK_RETURN || event.key.keysym.sym == SDLK_SPACE) &&
            state_ == GameState::Menu) {
            StartNewGame();
        } else if (event.key.keysym.sym == SDLK_ESCAPE && state_ == GameState::Playing) {
            ReturnToMenu();
        }
    }
}

void Game::StartNewGame() {
    state_ = GameState::Playing;
    poos_.clear();
    toiletX_ = (Config::kScreenWidth - Config::kToiletWidth) / 2.0f;
    spawnTimer_ = 0.45f;
    score_ = 0;
    missed_ = 0;
}

void Game::ReturnToMenu() {
    state_ = GameState::Menu;
    poos_.clear();
}

float Game::MovementInput() const {
    float movement = 0.0f;
    const Uint8* keyboard = SDL_GetKeyboardState(nullptr);
    if (keyboard[SDL_SCANCODE_LEFT]) movement -= 1.0f;
    if (keyboard[SDL_SCANCODE_RIGHT]) movement += 1.0f;
    if (controller_ != nullptr) {
        const Sint16 axis = SDL_GameControllerGetAxis(controller_, SDL_CONTROLLER_AXIS_LEFTX);
        if (std::abs(axis) > 7000) movement += static_cast<float>(axis) / 32767.0f;
        if (SDL_GameControllerGetButton(controller_, SDL_CONTROLLER_BUTTON_DPAD_LEFT)) movement -= 1.0f;
        if (SDL_GameControllerGetButton(controller_, SDL_CONTROLLER_BUTTON_DPAD_RIGHT)) movement += 1.0f;
    }
    return std::max(-1.0f, std::min(1.0f, movement));
}

void Game::SpawnPoo() {
    // A tiny deterministic PRNG avoids extra platform dependencies and keeps gameplay repeatable.
    randomState_ = randomState_ * 1664525u + 1013904223u;
    const float unit = static_cast<float>((randomState_ >> 8) & 0x00FFFFFFu) / 16777215.0f;
    const float maxX = Config::kScreenWidth - Config::kPooSize;
    poos_.push_back({unit * maxX, -Config::kPooSize, Config::kInitialPooSpeed + score_ * Config::kPooSpeedPerPoint});
}

bool Game::PooIsCaught(const Poo& poo) const {
    const float catchLeft = toiletX_ + 36.0f;
    const float catchRight = toiletX_ + Config::kToiletWidth - 36.0f;
    const float catchTop = Config::kToiletY + 40.0f;
    const float catchBottom = Config::kToiletY + 61.0f;
    const float pooCenterX = poo.x + Config::kPooSize * 0.5f;
    const float pooBottom = poo.y + Config::kPooSize;
    return pooCenterX >= catchLeft && pooCenterX <= catchRight && pooBottom >= catchTop && poo.y <= catchBottom;
}

void Game::Update(float deltaSeconds) {
    if (state_ != GameState::Playing) return;

    toiletX_ += MovementInput() * Config::kToiletSpeed * deltaSeconds;
    toiletX_ = std::max(0.0f, std::min(Config::kScreenWidth - Config::kToiletWidth, toiletX_));

    spawnTimer_ -= deltaSeconds;
    const float interval = std::max(Config::kMinimumSpawnInterval,
                                    Config::kInitialSpawnInterval - score_ * Config::kSpawnReductionPerPoint);
    while (spawnTimer_ <= 0.0f) {
        SpawnPoo();
        spawnTimer_ += interval;
    }

    for (auto it = poos_.begin(); it != poos_.end();) {
        it->y += it->speed * deltaSeconds;
        if (PooIsCaught(*it)) {
            ++score_;
            it = poos_.erase(it);
        } else if (it->y > Config::kScreenHeight) {
            ++missed_;
            it = poos_.erase(it);
        } else {
            ++it;
        }
    }
}

void Game::Render() const {
    SDL_SetRenderDrawColor(renderer_, 112, 190, 224, 255);
    SDL_RenderClear(renderer_);

    // Simple bathroom wall/floor placeholder, intentionally kept separate from gameplay data.
    SDL_SetRenderDrawColor(renderer_, 228, 244, 247, 255);
    FillRect(renderer_, 0, 0, Config::kScreenWidth, 520);
    SDL_SetRenderDrawColor(renderer_, 181, 215, 222, 255);
    FillRect(renderer_, 0, 520, Config::kScreenWidth, Config::kScreenHeight - 520);

    if (state_ == GameState::Menu) {
        DrawCenteredText(renderer_, "POO POO ON THE TOILET", 185, 8, {70, 42, 25, 255});
        DrawCenteredText(renderer_, "PRESS X TO START", 390, 5, {24, 50, 65, 255});
        DrawCenteredText(renderer_, "CATCH THE POO", 450, 3, {24, 50, 65, 255});
    } else {
        DrawText(renderer_, "POOS: " + std::to_string(score_), 36, 34, 4, {32, 55, 68, 255});
        DrawText(renderer_, "MISSED: " + std::to_string(missed_), 36, 75, 4, {32, 55, 68, 255});
        DrawText(renderer_, "OPTIONS: MENU", 908, 35, 3, {32, 55, 68, 255});
        for (const Poo& poo : poos_) DrawPoo(renderer_, poo);
        DrawToilet(renderer_, toiletX_);
    }

    SDL_RenderPresent(renderer_);
}
