#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <iostream>
#include <memory>

#include "scripts/resources/resource_archive.h"
#include "scripts/hud/bgm_manager.h"
#include "scripts/screens/screen_intro.h"
#include "scripts/screens/screen_create_character.h"
#include "scripts/screens/screen_game.h"
#include "scripts/screens/screen_load.h"
#include "scripts/screens/screen_battle.h"
#include "scripts/screens/screen_gameover.h"

int main(int argc, char* argv[]) {
    (void)argc; (void)argv;

    std::string resourceError;
    if (!resource_archive::removeDecryptedResourceCopies(
            resource_archive::executableDirectory(), resourceError)) {
        std::cerr << "Falha ao limpar copias descriptografadas dos recursos: "
                  << resourceError << std::endl;
        return -1;
    }

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "SDL_Init falhou: " << SDL_GetError() << std::endl;
        return -1;
    }
    if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
        std::cerr << "IMG_Init falhou: " << IMG_GetError() << std::endl;
        SDL_Quit();
        return -1;
    }

    bgm_manager::open();

    const int SCREEN_W = 1400;
    const int SCREEN_H = 850;

    SDL_Window* window = SDL_CreateWindow(
        "Moons of Krewbull - Build 1",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        SCREEN_W, SCREEN_H,
        SDL_WINDOW_SHOWN);

    if (!window) {
        std::cerr << "SDL_CreateWindow falhou: " << SDL_GetError() << std::endl;
        bgm_manager::close();
        IMG_Quit(); SDL_Quit();
        return -1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(
        window, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    if (!renderer) {
        std::cerr << "SDL_CreateRenderer falhou: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        bgm_manager::close();
        IMG_Quit(); SDL_Quit();
        return -1;
    }

    enum class AppState {
        INTRO, CREATE_CHARACTER, LOAD, GAME, BATTLE, GAME_OVER
    };
    AppState state = AppState::INTRO;

    std::unique_ptr<TelaIntro> intro(new TelaIntro(renderer, SCREEN_W, SCREEN_H));
    if (!intro->init()) {
        std::cerr << "Falha ao inicializar tela de intro!" << std::endl;
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        bgm_manager::close();
        IMG_Quit(); SDL_Quit();
        return -1;
    }

    std::unique_ptr<TelaCreateCharacter> createChar = nullptr;
    std::unique_ptr<TelaLoad> loadScreen = nullptr;
    std::unique_ptr<TelaGame> game = nullptr;
    std::unique_ptr<TelaBattle> battle = nullptr;
    std::unique_ptr<TelaGameOver> gameOver = nullptr;

    SDL_Event event;
    Uint32 lastTime = SDL_GetTicks();
    bool appRunning = true;

    while (appRunning) {
        Uint32 currentTime = SDL_GetTicks();
        float dt = (currentTime - lastTime) / 1000.0f;
        lastTime = currentTime;

        while (SDL_PollEvent(&event)) {
            if (state == AppState::INTRO) {
                intro->handleEvent(event);
            } else if (state == AppState::CREATE_CHARACTER && createChar) {
                createChar->handleEvent(event);
            } else if (state == AppState::LOAD && loadScreen) {
                loadScreen->handleEvent(event);
            } else if (state == AppState::BATTLE && battle) {
                battle->handleEvent(event);
            } else if (state == AppState::GAME_OVER && gameOver) {
                gameOver->handleEvent(event);
            } else if (game) {
                game->handleEvent(event);
            }
        }

        if (state == AppState::INTRO) {
            if (!intro->isRunning()) {
                appRunning = false;
                continue;
            }

            intro->update(dt);
            intro->render();

            if (intro->shouldOpenLoadScreen()) {
                intro.reset();
                loadScreen.reset(new TelaLoad(renderer, SCREEN_W, SCREEN_H));
                if (!loadScreen->init()) {
                    std::cerr << "Falha ao inicializar tela de load!"
                              << std::endl;
                    appRunning = false;
                } else {
                    state = AppState::LOAD;
                }
            } else if (intro->shouldStartGame()) {
                intro.reset();

                createChar.reset(
                    new TelaCreateCharacter(renderer, SCREEN_W, SCREEN_H));
                if (!createChar->init()) {
                    std::cerr << "Falha ao inicializar tela de criacao!"
                              << std::endl;
                    appRunning = false;
                } else {
                    state = AppState::CREATE_CHARACTER;
                }
            }
        }
        else if (state == AppState::CREATE_CHARACTER) {
            if (!createChar || !createChar->isRunning()) {
                appRunning = false;
            } else {
                createChar->update(dt);
                createChar->render();

                if (createChar->shouldReturnToIntro()) {
                    createChar.reset();
                    intro.reset(new TelaIntro(renderer, SCREEN_W, SCREEN_H));
                    if (!intro->init()) {
                        std::cerr << "Falha ao retornar para tela de intro!"
                                  << std::endl;
                        appRunning = false;
                    } else {
                        state = AppState::INTRO;
                    }
                } else if (createChar->shouldStartGame()) {
                    createChar.reset();

                    game.reset(new TelaGame(renderer, SCREEN_W, SCREEN_H));
                    if (!game->init()) {
                        std::cerr << "Falha ao inicializar tela de jogo!"
                                  << std::endl;
                        appRunning = false;
                    } else {
                        state = AppState::GAME;
                    }
                }
            }
        }
        else if (state == AppState::LOAD) {
            if (!loadScreen || !loadScreen->isRunning()) {
                appRunning = false;
            } else {
                loadScreen->update(dt);
                loadScreen->render();

                if (loadScreen->shouldReturnToIntro()) {
                    loadScreen.reset();
                    intro.reset(new TelaIntro(renderer, SCREEN_W, SCREEN_H));
                    if (!intro->init()) {
                        std::cerr << "Falha ao retornar para tela de intro!"
                                  << std::endl;
                        appRunning = false;
                    } else {
                        state = AppState::INTRO;
                    }
                } else if (loadScreen->shouldStartGame()) {
                    const std::filesystem::path selectedSave =
                        loadScreen->selectedSavePath();
                    loadScreen.reset();

                    game.reset(new TelaGame(
                        renderer, SCREEN_W, SCREEN_H, selectedSave));
                    if (!game->init()) {
                        std::cerr << "Falha ao carregar o save selecionado!"
                                  << std::endl;
                        appRunning = false;
                    } else {
                        state = AppState::GAME;
                    }
                }
            }
        }
        else if (state == AppState::GAME) {
            if (!game || !game->isRunning()) {
                appRunning = false;
            } else {
                game->update(dt);
                if (game->hasPendingBattle()) {
                    Player& player = game->getPlayer();
                    const std::vector<CombatEnemy*> enemies =
                        game->consumeBattleRequest();
                    battle.reset(new TelaBattle(
                        renderer, SCREEN_W, SCREEN_H, player, enemies));
                    if (!battle->init()) {
                        std::cerr << "Falha ao inicializar tela de batalha!"
                                  << std::endl;
                        appRunning = false;
                    } else {
                        state = AppState::BATTLE;
                    }
                } else {
                    game->render();
                }
            }
        }
        else if (state == AppState::BATTLE) {
            if (!battle || !battle->isRunning()) {
                appRunning = false;
            } else if (battle->shouldShowGameOver()) {
                Player& player = game->getPlayer();
                battle.reset();
                gameOver.reset(new TelaGameOver(
                    renderer, SCREEN_W, SCREEN_H,
                    player.characterClass, player.gender));
                if (!gameOver->init()) {
                    std::cerr << "Falha ao inicializar a tela de game over!"
                              << std::endl;
                    appRunning = false;
                } else {
                    state = AppState::GAME_OVER;
                }
            } else if (battle->shouldReturnToGame()) {
                battle.reset();
                game->resumeAfterBattle();
                state = AppState::GAME;
            } else {
                battle->update(dt);
                battle->render();
            }
        }
        else if (state == AppState::GAME_OVER) {
            if (!gameOver || !gameOver->isRunning()) {
                appRunning = false;
            } else {
                gameOver->update(dt);
                gameOver->render();
                if (gameOver->choice() == TelaGameOver::Choice::REVIVE) {
                    gameOver.reset();
                    game->resumeAfterGameOver();
                    state = AppState::GAME;
                } else if (gameOver->choice() ==
                           TelaGameOver::Choice::RETURN_TO_MENU) {
                    gameOver.reset();
                    game->resumeAfterGameOver();
                    game.reset();
                    intro.reset(new TelaIntro(renderer, SCREEN_W, SCREEN_H));
                    if (!intro->init()) {
                        std::cerr << "Falha ao retornar para tela de intro!"
                                  << std::endl;
                        appRunning = false;
                    } else {
                        state = AppState::INTRO;
                    }
                }
            }
        }

        SDL_RenderPresent(renderer);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    bgm_manager::close();
    IMG_Quit();
    SDL_Quit();
    return 0;
}