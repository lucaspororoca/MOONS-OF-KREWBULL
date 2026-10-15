#include "scripts/hud/bgm_manager.h"
#include "scripts/resources/resource_archive.h"
#include <iostream>

namespace bgm_manager {

static bool audioOpen = false;
static Mix_Music* currentMusic = nullptr;
static std::string currentMusicPath;

bool open() {
    if (audioOpen) return true;

    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        std::cerr << "Mix_OpenAudio falhou: " << Mix_GetError() << std::endl;
        audioOpen = false;
        return false;
    }

    audioOpen = true;
    return true;
}

void stop() {
    if (currentMusic) {
        Mix_HaltMusic();
        Mix_FreeMusic(currentMusic);
        currentMusic = nullptr;
    }
    currentMusicPath.clear();
}

void close() {
    stop();

    if (audioOpen) {
        Mix_CloseAudio();
        audioOpen = false;
    }
}

bool isOpen() {
    return audioOpen;
}

bool playLoop(const std::string& path) {
    if (!audioOpen) {
        if (!open()) return false;
    }

    if (currentMusic && currentMusicPath == path &&
        Mix_PlayingMusic() && !Mix_PausedMusic()) {
        return true;
    }

    stop();

    currentMusic = resource_archive::loadResourceMusic(path);
    if (!currentMusic) {
        std::cerr << "Falha ao carregar BGM " << path
                  << ": " << Mix_GetError() << std::endl;
        return false;
    }

    if (Mix_PlayMusic(currentMusic, -1) < 0) {
        std::cerr << "Falha ao tocar BGM " << path
                  << ": " << Mix_GetError() << std::endl;
        Mix_FreeMusic(currentMusic);
        currentMusic = nullptr;
        return false;
    }

    currentMusicPath = path;
    return true;
}

}