#include <SDL.h>
#include <SDL_main.h>
#include <android/log.h>

// Replace this with the actual main loop/start function name inside your SA2 decomp code
extern int real_main(int argc, char *argv[]); 

int main(int argc, char *argv[]) {
    __android_log_print(ANDROID_LOG_INFO, "SA2_PORT", "Initializing Sonic Advance 2 via SDL2 on Android!");
    
    // Redirect execution straight into the core decompilation codebase
    return real_main(argc, argv);
}
