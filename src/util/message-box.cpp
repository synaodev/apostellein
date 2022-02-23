#include <SDL2/SDL_messagebox.h>
#include <apostellein/konst.hpp>

#include "./message-box.hpp"

void message_box::error(const char* message) {
    SDL_ShowSimpleMessageBox(
        SDL_MESSAGEBOX_ERROR,
        konst::APPLICATION,
        message,
        nullptr
    );
}

void message_box::error(const std::string& message) {
    message_box::error(message.c_str());
}
