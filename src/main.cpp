#include "app/app.h"

#include <string.h>

int main(int argc, char **argv) {
    app::AppOptions options = {};
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--control-dir") == 0 && i + 1 < argc) {
            options.control_dir = argv[++i];
        }
    }

    app::App app;
    if (!app.initialize(options)) {
        return 1;
    }
    return app.run();
}
