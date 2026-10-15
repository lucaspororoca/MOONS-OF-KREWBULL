#ifndef BGM_MANAGER_H
#define BGM_MANAGER_H

#include <SDL2/SDL_mixer.h>
#include <string>

namespace bgm_manager {

bool open();
void close();

bool playLoop(const std::string& path);
void stop();

bool isOpen();

}

#endif