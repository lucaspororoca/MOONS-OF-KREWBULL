#pragma once

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <SDL2/SDL_ttf.h>

#include <algorithm>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <limits>
#include <string>
#include <utility>
#include <vector>

namespace resource_archive {

namespace detail {

constexpr char xorKey[] = "MoonsOfKrewbull2024SecretKey!";
constexpr std::size_t xorKeySize = sizeof(xorKey) - 1;

inline std::vector<std::filesystem::path> resourceCandidates(
    const std::string& resourcePath) {
    const std::filesystem::path requested(resourcePath);
    std::filesystem::path relative = requested;
    if (!requested.is_absolute() && requested.begin() != requested.end() &&
        *requested.begin() == "resources") {
        relative.clear();
        auto component = requested.begin();
        ++component;
        for (; component != requested.end(); ++component) {
            relative /= *component;
        }
    }

    std::vector<std::filesystem::path> candidates;
    if (requested.is_absolute()) {
        candidates.push_back(requested);
        return candidates;
    }

    if (char* basePath = SDL_GetBasePath()) {
        const std::filesystem::path executableDirectory(basePath);
        SDL_free(basePath);
        candidates.push_back(executableDirectory / relative);
        candidates.push_back(executableDirectory / requested);
    }

    const std::filesystem::path workingDirectory = std::filesystem::current_path();
    candidates.push_back(workingDirectory / requested);
    if (relative != requested) {
        candidates.push_back(workingDirectory / relative);
    }

    return candidates;
}

struct MemoryStream {
    std::vector<Uint8> bytes;
    std::size_t position = 0;
};

inline Sint64 SDLCALL streamSize(SDL_RWops* context) {
    const auto* stream = static_cast<const MemoryStream*>(context->hidden.unknown.data1);
    return static_cast<Sint64>(stream->bytes.size());
}

inline Sint64 SDLCALL streamSeek(SDL_RWops* context, Sint64 offset, int whence) {
    auto* stream = static_cast<MemoryStream*>(context->hidden.unknown.data1);
    Sint64 base = 0;
    if (whence == RW_SEEK_CUR) {
        base = static_cast<Sint64>(stream->position);
    } else if (whence == RW_SEEK_END) {
        base = static_cast<Sint64>(stream->bytes.size());
    } else if (whence != RW_SEEK_SET) {
        return SDL_SetError("Invalid resource stream seek mode");
    }

    if (offset < -base ||
        (offset > 0 && base > std::numeric_limits<Sint64>::max() - offset)) {
        return SDL_SetError("Resource stream seek is out of range");
    }

    const Sint64 target = base + offset;
    if (target < 0 || static_cast<Uint64>(target) > stream->bytes.size()) {
        return SDL_SetError("Resource stream seek is out of range");
    }

    stream->position = static_cast<std::size_t>(target);
    return target;
}

inline std::size_t SDLCALL streamRead(SDL_RWops* context, void* destination,
                                     std::size_t objectSize, std::size_t maxObjects) {
    auto* stream = static_cast<MemoryStream*>(context->hidden.unknown.data1);
    if (objectSize == 0 || maxObjects == 0) {
        return 0;
    }

    const std::size_t available = stream->bytes.size() - stream->position;
    const std::size_t objects = std::min(maxObjects, available / objectSize);
    const std::size_t bytesToCopy = objects * objectSize;
    if (bytesToCopy > 0) {
        std::memcpy(destination, stream->bytes.data() + stream->position, bytesToCopy);
        stream->position += bytesToCopy;
    }
    return objects;
}

inline std::size_t SDLCALL streamWrite(SDL_RWops*, const void*, std::size_t,
                                      std::size_t) {
    SDL_SetError("Resource streams are read-only");
    return 0;
}

inline int SDLCALL streamClose(SDL_RWops* context) {
    delete static_cast<MemoryStream*>(context->hidden.unknown.data1);
    SDL_FreeRW(context);
    return 0;
}

} // namespace detail

inline std::filesystem::path executableDirectory() {
    if (char* basePath = SDL_GetBasePath()) {
        const std::filesystem::path result(basePath);
        SDL_free(basePath);
        return result;
    }
    return std::filesystem::current_path();
}

inline bool readResourceBytes(const std::string& resourcePath,
                              std::vector<Uint8>& bytes,
                              std::string& errorMessage) {
    std::ifstream input;
    bool encrypted = false;
    std::filesystem::path openedPath;
    const auto candidates = detail::resourceCandidates(resourcePath);
    for (const auto& candidate : candidates) {
        std::filesystem::path pakPath(candidate);
        pakPath += ".pak";
        input.open(pakPath, std::ios::binary);
        if (input) {
            encrypted = true;
            openedPath = pakPath;
            break;
        }
        input.clear();
    }

    if (!input) {
        for (const auto& candidate : candidates) {
            input.open(candidate, std::ios::binary);
            if (input) {
                openedPath = candidate;
                break;
            }
            input.clear();
        }
    }
    if (!input) {
        errorMessage = "Nao foi possivel abrir o recurso: " + resourcePath;
        return false;
    }

    bytes.assign(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
    if (input.bad()) {
        errorMessage = "Erro ao ler o recurso: " + openedPath.string();
        return false;
    }

    if (bytes.size() > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
        errorMessage = "Recurso excede o limite de tamanho suportado: " + resourcePath;
        return false;
    }

    if (encrypted) {
        for (std::size_t i = 0; i < bytes.size(); ++i) {
            bytes[i] ^= static_cast<Uint8>(detail::xorKey[i % detail::xorKeySize]);
        }
    }

    errorMessage.clear();
    return true;
}

inline SDL_RWops* openResourceRWops(const std::string& resourcePath) {
    auto* stream = new detail::MemoryStream;
    std::string errorMessage;
    if (!readResourceBytes(resourcePath, stream->bytes, errorMessage)) {
        delete stream;
        SDL_SetError("%s", errorMessage.c_str());
        return nullptr;
    }

    SDL_RWops* rwops = SDL_AllocRW();
    if (!rwops) {
        delete stream;
        return nullptr;
    }

    rwops->size = detail::streamSize;
    rwops->seek = detail::streamSeek;
    rwops->read = detail::streamRead;
    rwops->write = detail::streamWrite;
    rwops->close = detail::streamClose;
    rwops->type = SDL_RWOPS_UNKNOWN;
    rwops->hidden.unknown.data1 = stream;
    rwops->hidden.unknown.data2 = nullptr;
    return rwops;
}

inline SDL_Surface* loadResourceImage(const std::string& resourcePath) {
    SDL_RWops* rwops = openResourceRWops(resourcePath);
    if (!rwops) {
        return nullptr;
    }
    return IMG_Load_RW(rwops, 1);
}

inline Mix_Music* loadResourceMusic(const std::string& resourcePath) {
    SDL_RWops* rwops = openResourceRWops(resourcePath);
    if (!rwops) {
        return nullptr;
    }
    return Mix_LoadMUS_RW(rwops, 1);
}

inline TTF_Font* openResourceFont(const std::string& resourcePath, int pointSize) {
    SDL_RWops* rwops = openResourceRWops(resourcePath);
    if (!rwops) {
        return nullptr;
    }
    return TTF_OpenFontRW(rwops, 1, pointSize);
}

inline bool readResourceText(const std::string& resourcePath,
                             std::string& contents,
                             std::string& errorMessage) {
    std::vector<Uint8> bytes;
    if (!readResourceBytes(resourcePath, bytes, errorMessage)) {
        return false;
    }
    contents.assign(bytes.begin(), bytes.end());
    return true;
}

inline bool removeDecryptedResourceCopies(const std::filesystem::path& resourceRoot,
                                          std::string& errorMessage) {
    std::error_code error;
    std::filesystem::recursive_directory_iterator current(resourceRoot, error);
    const std::filesystem::recursive_directory_iterator end;
    if (error) {
        errorMessage = "Nao foi possivel listar os recursos: " + error.message();
        return false;
    }

    std::vector<std::filesystem::path> plaintextCopies;
    for (; current != end; current.increment(error)) {
        if (error) {
            errorMessage = "Erro ao percorrer os recursos: " + error.message();
            return false;
        }
        if (!current->is_regular_file(error)) {
            if (error) {
                errorMessage = "Nao foi possivel verificar um recurso: " + error.message();
                return false;
            }
            continue;
        }

        const std::filesystem::path pakPath = current->path();
        if (pakPath.extension() == ".pak") {
            std::filesystem::path plaintextPath = pakPath;
            plaintextPath.replace_extension();
            plaintextCopies.push_back(std::move(plaintextPath));
        }
    }
    if (error) {
        errorMessage = "Erro ao percorrer os recursos: " + error.message();
        return false;
    }

    for (const auto& plaintextPath : plaintextCopies) {
        error.clear();
        if (std::filesystem::exists(plaintextPath, error) &&
            !std::filesystem::remove(plaintextPath, error)) {
            if (!error) {
                errorMessage = "Nao foi possivel remover a copia descriptografada: " +
                               plaintextPath.string();
            } else {
                errorMessage = "Erro ao remover a copia descriptografada: " + error.message();
            }
            return false;
        }
        if (error) {
            errorMessage = "Erro ao verificar a copia descriptografada: " + error.message();
            return false;
        }
    }

    errorMessage.clear();
    return true;
}

} // namespace resource_archive
