#include "app/App.h"

#include <SDL3/SDL_main.h>

int main(int, char **) {
    App app;
    if (!app.initialize()) {
        return 1;
    }
    return app.run();
}
