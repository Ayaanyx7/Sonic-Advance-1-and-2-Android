#ifdef __ANDROID__
#include <math.h>
#include <SDL.h>
#include <SDL_image.h>
#include <stdio.h>
#include <limits.h>
#include <string.h>
#include "../netlink/netlink.h" 

FILE* android_fopen_override(const char* filename, const char* mode) {
    char absolute_path[PATH_MAX];
    const char* ext_storage = SDL_AndroidGetExternalStoragePath();
    
    // Strip redundant local subdirectory indicators if specified by the engine core loops
    const char* clean_name = (strncmp(filename, "./", 2) == 0) ? filename + 2 : filename;
    
    if (ext_storage) {
        // Automatically redirects queries into: /storage/emulated/0/Android/data/[YOUR_PACKAGE]/files/
        snprintf(absolute_path, sizeof(absolute_path), "%s/%s", ext_storage, clean_name);
        return fopen(absolute_path, mode);
    }
    return fopen(filename, mode);
}
#define fopen(path, mode) android_fopen_override(path, mode)

static SDL_GameController *active_gamepad = NULL;

static SDL_Texture *touch_lr_texture = NULL;
static SDL_Texture *touch_ab_texture = NULL;
static SDL_Texture *touch_start_select_texture = NULL;
static SDL_Texture *touch_dpad_texture = NULL;
static SDL_Texture *dpadComposite = NULL;
static SDL_Texture *touch_mp_texture = NULL;

static int touch_lr_w = 0, touch_lr_h = 0;
static int touch_ab_w = 0, touch_ab_h = 0;
static int touch_ss_w = 0, touch_ss_h = 0;

static SDL_FingerID dpad_touch_finger = -1;
static SDL_FingerID a_touch_finger = -1;
static SDL_FingerID b_touch_finger = -1;
static SDL_FingerID l_touch_finger = -1;
static SDL_FingerID r_touch_finger = -1;
static SDL_FingerID start_touch_finger = -1;
static SDL_FingerID select_touch_finger = -1;
static SDL_FingerID mp_touch_finger = -1;

static SDL_Texture *LoadTouchTexture(SDL_Renderer *renderer, const char *path) { 
    SDL_RWops *rw = SDL_RWFromFile(path, "rb"); 
    if (!rw) { 
        SDL_Log("Failed to open touch asset %s: %s", path, SDL_GetError()); 
        return NULL; 
    } 

    SDL_Surface *surface = IMG_Load_RW(rw, 0); 
    SDL_RWclose(rw); 

    if (!surface) { 
        SDL_Log("Failed to load touch asset %s: %s", path, IMG_GetError()); 
        return NULL; 
    } 

    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface); 
    if (!texture) { 
        SDL_Log("Failed to create touch texture %s: %s", path, SDL_GetError()); 
    } else { 
        SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND); 
        SDL_SetTextureAlphaMod(texture, 128); 
    } 

    SDL_FreeSurface(surface); 
    return texture;
}

static SDL_Rect dstDpad;
static int dpadPixelCenterX, dpadPixelCenterY, dpadPixelRadius;
#define DPAD_DEADZONE_PX 8
static SDL_Rect dstA;
static SDL_Rect dstB;
static SDL_Rect dstL;
static SDL_Rect dstR;
static SDL_Rect dstStart;
static SDL_Rect dstSelect;
static SDL_Rect dstMp;
#endif

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#include <xinput.h>
#endif

#ifdef __PSP__
#include <pspkernel.h>
#include <pspdebug.h>
#include <pspgu.h>
#endif

#include <SDL.h>

#include "global.h"
#include "core.h"
#include "lib/agb_flash/flash_internal.h"
#include "platform/shared/dma.h"
#include "platform/shared/input.h"
#include "platform/shared/video/gpsp_renderer.h"

#if ENABLE_AUDIO
#include "platform/shared/audio/cgb_audio.h"
#endif

ALIGNED(256) uint16_t gameImage[DISPLAY_WIDTH * DISPLAY_HEIGHT];

#if ENABLE_VRAM_VIEW
uint16_t vramBuffer[VRAM_VIEW_WIDTH * VRAM_VIEW_HEIGHT];
#endif

SDL_Window *sdlWindow;
SDL_Renderer *sdlRenderer;
SDL_Texture *sdlTexture;
#if ENABLE_VRAM_VIEW
SDL_Window *vramWindow;
SDL_Renderer *vramRenderer;
SDL_Texture *vramTexture;
#endif
#define INITIAL_VIDEO_SCALE 1
unsigned int videoScale = INITIAL_VIDEO_SCALE;
unsigned int preFullscreenVideoScale = INITIAL_VIDEO_SCALE;

bool speedUp = false;
bool videoScaleChanged = false;
bool isRunning = true;
bool paused = false;
bool stepOneFrame = false;
bool headless = false;

#ifdef __PSP__
static SDL_Joystick *joystick = NULL;
static SDL_Rect pspDestRect;
#endif

double lastGameTime = 0;
double curGameTime = 0;
double fixedTimestep = 1.0 / 60.0; // 16.666667ms
double timeScale = 1.0;
double accumulator = 0.0;

static FILE *sSaveFile = NULL;

extern void AgbMain(void);
void DoSoftReset(void) {};

void ProcessSDLEvents(void);
void VDraw(SDL_Texture *texture);
void VramDraw(SDL_Texture *texture);

static void ReadSaveFile(char *path);
static void StoreSaveFile(void);
static void CloseSaveFile(void);

u16 Platform_GetKeyInput(void);

#ifdef _WIN32
void *Platform_malloc(size_t numBytes) { return HeapAlloc(GetProcessHeap(), HEAP_GENERATE_EXCEPTIONS | HEAP_ZERO_MEMORY, numBytes); }
void Platform_free(void *ptr) { HeapFree(GetProcessHeap(), 0, ptr); }
#endif

#ifdef __PSP__
PSP_MODULE_INFO("SonicAdvance2", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);
PSP_HEAP_SIZE_KB(-1024);

unsigned int sce_newlib_stack_size = 512 * 1024;

extern bool isRunning;

int exitCallback(int arg1, int arg2, void *common)
{
    (void)arg1;
    (void)arg2;
    (void)common;
    isRunning = false;
    return 0;
}

int callbackThread(SceSize args, void *argp)
{
    (void)args;
    (void)argp;
    int cbid = sceKernelCreateCallback("Exit Callback", exitCallback, NULL);
    sceKernelRegisterExitCallback(cbid);
    sceKernelSleepThreadCB();
    return 0;
}

int setupPspCallbacks(void)
{
    int thid = sceKernelCreateThread("update_thread", callbackThread, 0x11, 0xFA0, 0, 0);
    if (thid >= 0) {
        sceKernelStartThread(thid, 0, 0);
    }
    return thid;
}
#endif

#ifdef __ANDROID__
static u16 keys;
#endif

int main(int argc, char **argv)
{
#ifdef __PSP__
    setupPspCallbacks();
#endif

    const char *headlessEnv = getenv("HEADLESS");

    if (headlessEnv && strcmp(headlessEnv, "true") == 0) {
        headless = true;
    }

    const char *parentEnv = getenv("SIO_PARENT");

    if (parentEnv && strcmp(parentEnv, "true") == 0) {
        SIO_MULTI_CNT->id = 0;
        SIO_MULTI_CNT->si = 1;
        SIO_MULTI_CNT->sd = 1;
        SIO_MULTI_CNT->enable = false;
    }

    // Open an output console on Windows
#if (defined _WIN32) && (DEBUG != 0)
    AllocConsole();
    AttachConsole(GetCurrentProcessId());
    freopen("CON", "w", stdout);
#endif

    ReadSaveFile("sa2.sav");

    // Prevent the multiplayer screen from being drawn ( see core.c:EngineInit() )
    REG_RCNT = 0x8000;
    REG_KEYINPUT = 0x3FF;

    if (headless) {
#if ENABLE_AUDIO
        // Required or it makes an infinite loop
        cgb_audio_init(48000);
#endif
        AgbMain();
        return 1;
    }

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER) < 0) {
    fprintf(stderr, "SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
    return 1;
}

#ifdef __ANDROID__
    SDL_SetHint(SDL_HINT_ORIENTATIONS, "LandscapeLeft LandscapeRight");
#endif
    
#ifdef __PSP__
    if (SDL_NumJoysticks() > 0) {
        joystick = SDL_JoystickOpen(0);
    }
#endif

#ifdef TITLE_BAR
    const char *title = STR(TITLE_BAR);
#else
    const char *title = "SAT-R sa2";
#endif

#ifdef __PSP__
    sdlWindow = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 480, 272, SDL_WINDOW_SHOWN);
#else
    sdlWindow = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, DISPLAY_WIDTH * videoScale,
                                 DISPLAY_HEIGHT * videoScale, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
#endif
    if (sdlWindow == NULL) {
        fprintf(stderr, "Window could not be created! SDL_Error: %s\n", SDL_GetError());
        return 1;
    }

#if ENABLE_VRAM_VIEW
    int mainWindowX;
    int mainWindowWidth;
    SDL_GetWindowPosition(sdlWindow, &mainWindowX, NULL);
    SDL_GetWindowSize(sdlWindow, &mainWindowWidth, NULL);
    int vramWindowX = mainWindowX + mainWindowWidth;
    u16 vramWindowWidth = VRAM_VIEW_WIDTH;
    u16 vramWindowHeight = VRAM_VIEW_HEIGHT;
    vramWindow = SDL_CreateWindow("VRAM View", vramWindowX, SDL_WINDOWPOS_CENTERED, vramWindowWidth, vramWindowHeight,
                                  SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    if (vramWindow == NULL) {
        fprintf(stderr, "VRAM Window could not be created! SDL_Error: %s\n", SDL_GetError());
        return 1;
    }
#endif

#ifdef __PSP__
    sdlRenderer = SDL_CreateRenderer(sdlWindow, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (sdlRenderer == NULL)
        sdlRenderer = SDL_CreateRenderer(sdlWindow, -1, SDL_RENDERER_ACCELERATED);
    if (sdlRenderer == NULL)
        sdlRenderer = SDL_CreateRenderer(sdlWindow, -1, 0);
#else
    sdlRenderer = SDL_CreateRenderer(sdlWindow, -1, SDL_RENDERER_PRESENTVSYNC);
#endif
    if (sdlRenderer == NULL) {
        fprintf(stderr, "Renderer could not be created! SDL_Error: %s\n", SDL_GetError());
        return 1;
    }
    
#if ENABLE_VRAM_VIEW
    vramRenderer = SDL_CreateRenderer(vramWindow, -1, SDL_RENDERER_PRESENTVSYNC);
    if (vramRenderer == NULL) {
        fprintf(stderr, "VRAM Renderer could not be created! SDL_Error: %s\n", SDL_GetError());
        return 1;
    }
#endif

    SDL_SetRenderDrawColor(sdlRenderer, 0, 0, 0, 255);
    SDL_RenderClear(sdlRenderer);
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");
#ifdef __PSP__
    // SDL_RenderSetLogicalSize is broken on PSP, stretch to fill manually
    pspDestRect = (SDL_Rect) { 0, 0, GU_SCR_WIDTH, GU_SCR_HEIGHT };
#else
    SDL_RenderSetLogicalSize(sdlRenderer, DISPLAY_WIDTH, DISPLAY_HEIGHT);
#endif
#if ENABLE_VRAM_VIEW
    SDL_SetRenderDrawColor(vramRenderer, 0, 0, 0, 255);
    SDL_RenderClear(vramRenderer);
    SDL_RenderSetLogicalSize(vramRenderer, vramWindowWidth, vramWindowHeight);
#endif

    sdlTexture = SDL_CreateTexture(sdlRenderer, SDL_PIXELFORMAT_ABGR1555, SDL_TEXTUREACCESS_STREAMING, DISPLAY_WIDTH, DISPLAY_HEIGHT);
    if (sdlTexture == NULL) {
        fprintf(stderr, "Texture could not be created! SDL_Error: %s\n", SDL_GetError());
        return 1;
    }

#ifdef __ANDROID__
    if ((IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG) != IMG_INIT_PNG) {
        SDL_Log("SDL_image PNG initialization failed: %s", IMG_GetError());
        return 1; // Works perfectly here because main returns an int!
    }
        
    touch_lr_texture = LoadTouchTexture(sdlRenderer, "touch/L & R buttons.png");
    touch_ab_texture = LoadTouchTexture(sdlRenderer, "touch/A & B buttons.png");
    touch_start_select_texture = LoadTouchTexture(sdlRenderer, "touch/start and select.png");
    touch_dpad_texture = LoadTouchTexture(sdlRenderer, "touch/Dpad stuff.png");
    touch_mp_texture = LoadTouchTexture(sdlRenderer, "touch/Multiplayer button.png");

dpadComposite = SDL_CreateTexture(sdlRenderer, SDL_PIXELFORMAT_RGBA8888,
                                    SDL_TEXTUREACCESS_TARGET, 64, 64);
SDL_SetTextureBlendMode(dpadComposite, SDL_BLENDMODE_BLEND);

    if (touch_lr_texture) SDL_QueryTexture(touch_lr_texture, NULL, NULL, &touch_lr_w, &touch_lr_h);
    if (touch_ab_texture) SDL_QueryTexture(touch_ab_texture, NULL, NULL, &touch_ab_w, &touch_ab_h);
    if (touch_start_select_texture) SDL_QueryTexture(touch_start_select_texture, NULL, NULL, &touch_ss_w, &touch_ss_h);

    dstDpad = (SDL_Rect){ 20, DISPLAY_HEIGHT - 100, 80, 80 };
    dpadPixelCenterX = dstDpad.x + dstDpad.w / 2;
    dpadPixelCenterY = dstDpad.y + dstDpad.h / 2;
    dpadPixelRadius  = dstDpad.w / 2;  

    dstMp = (SDL_Rect){ (DISPLAY_WIDTH / 2) - 16, DISPLAY_HEIGHT - 40, 32, 32 };
#endif
    
#if ENABLE_VRAM_VIEW
    vramTexture = SDL_CreateTexture(vramRenderer, SDL_PIXELFORMAT_ABGR1555, SDL_TEXTUREACCESS_STREAMING, vramWindowWidth, vramWindowHeight);
    if (vramTexture == NULL) {
        fprintf(stderr, "Texture could not be created! SDL_Error: %s\n", SDL_GetError());
        return 1;
    }
#endif

#if ENABLE_AUDIO
    SDL_AudioSpec want;

    SDL_memset(&want, 0, sizeof(want)); /* or SDL_zero(want) */
    want.freq = 48000;
    want.format = AUDIO_S16;
    want.channels = 2;
    want.samples = (want.freq / 60);
    cgb_audio_init(want.freq);

    if (SDL_OpenAudio(&want, 0) < 0) {
        SDL_Log("Failed to open audio: %s", SDL_GetError());
    } else {
        if (want.format != AUDIO_S16) /* we let this one thing change. */
            SDL_Log("We didn't get S16 audio format.");
        SDL_PauseAudio(0);
    }
#endif

    VDraw(sdlTexture);
#if ENABLE_VRAM_VIEW
    VramDraw(vramTexture);
#endif
    AgbMain();

    return 0;
}

bool newFrameRequested = FALSE;

// called every gba frame. we process sdl events and render as many times
// as vsync needs, then return when a new game frame is needed.
void VBlankIntrWait(void)
{
#define HANDLE_VBLANK_INTRS()                                                                                                              \
    ({                                                                                                                                     \
        REG_DISPSTAT |= INTR_FLAG_VBLANK;                                                                                                  \
        RunDMAs(DMA_VBLANK);                                                                                                               \
        if (REG_DISPSTAT & DISPSTAT_VBLANK_INTR)                                                                                           \
            gIntrTable[INTR_INDEX_VBLANK]();                                                                                               \
        REG_DISPSTAT &= ~INTR_FLAG_VBLANK;                                                                                                 \
    })

    if (headless) {
        REG_VCOUNT = DISPLAY_HEIGHT + 1;
        HANDLE_VBLANK_INTRS();
        return;
    }

    bool frameAvailable = TRUE;
    bool frameDrawn = false;

    while (isRunning) {
#ifndef __PSP__
        ProcessSDLEvents();
#endif

        if (!paused || stepOneFrame) {
            double dt = fixedTimestep / timeScale; // TODO: Fix speedup

            // don't accumulate time if we already requested a new frame
            // this frame cycle (emulates threaded sdl behavior)
            if (!newFrameRequested) {
                double deltaTime = 0;

                curGameTime = SDL_GetPerformanceCounter();
                if (stepOneFrame) {
                    deltaTime = dt;
                } else {
                    deltaTime = (double)((curGameTime - lastGameTime) / (double)SDL_GetPerformanceFrequency());
                    if (deltaTime > (dt * 5))
                        deltaTime = dt * 5;
                }
                lastGameTime = curGameTime;

                accumulator += deltaTime;
            } else {
                newFrameRequested = FALSE;
            }

            while (accumulator >= dt) {
                REG_KEYINPUT = KEYS_MASK ^ Platform_GetKeyInput();
                if (frameAvailable) {
                    VDraw(sdlTexture);
                    frameAvailable = FALSE;
                    frameDrawn = true;

                    HANDLE_VBLANK_INTRS();

                    accumulator -= dt;
                } else {
                    newFrameRequested = TRUE;
                    return;
                }
            }

            if (paused && stepOneFrame) {
                stepOneFrame = false;
            }
        }

        // present
#ifdef __PSP__
        // manual blit since SDL_RenderSetLogicalSize doesn't work on psp
        SDL_RenderCopy(sdlRenderer, sdlTexture, NULL, &pspDestRect);
        SDL_RenderPresent(sdlRenderer);
#else
        SDL_RenderClear(sdlRenderer);
        SDL_RenderCopy(sdlRenderer, sdlTexture, NULL, NULL);


#if ENABLE_VRAM_VIEW
        VramDraw(vramTexture);
        SDL_RenderClear(vramRenderer);
        SDL_RenderCopy(vramRenderer, vramTexture, NULL, NULL);
#endif
        
        if (videoScaleChanged) {
            SDL_SetWindowSize(sdlWindow, DISPLAY_WIDTH * videoScale, DISPLAY_HEIGHT * videoScale);
            videoScaleChanged = false;
        }
#ifdef __ANDROID__
#include "multi_sio.h"
#include "game/sa2/multiplayer/multipak_connection.h"
#include "global.h"
#include "game/sa2/options_screen.h"
#include "game/sa2/save.h"
#include "game/shared/stage/stage.h"
#include "game/sa2/multiplayer/mode_select.h"

if (NetLink_ConsumePendingMultiplayerStart())
{
    gGameMode = GAME_MODE_MULTI_PLAYER;
    ApplyGameStageSettings();
    if (LOADED_SAVE->playerName[0] != PLAYER_NAME_END_CHAR) {
        CreateMultiplayerModeSelectScreen();
    } else {
        CreateNewProfileNameScreen(NEW_PROFILE_NAME_MULTIPLAYER);
    }
}
        
SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest");

        #define LR_GRID_W 688

        int lrW = LR_GRID_W / 2;
        int lrRowSplit = 223;
        
        SDL_Rect srcL = { (l_touch_finger != -1) ? lrW : 0, 0,          lrW, lrRowSplit };
        SDL_Rect srcR = { (r_touch_finger != -1) ? lrW : 0, lrRowSplit, lrW, touch_lr_h - lrRowSplit };
        

        dstL = (SDL_Rect){ 10, 10, 80, 48 };
        dstR = (SDL_Rect){ DISPLAY_WIDTH - 90, 10, 80, 48 };

        SDL_RenderCopy(sdlRenderer, touch_lr_texture, &srcL, &dstL);
        SDL_RenderCopy(sdlRenderer, touch_lr_texture, &srcR, &dstR);
        
        int menuW = touch_ss_w / 2;
        
        SDL_Rect srcStart  = { 0, 0, menuW, touch_ss_h };         
        SDL_Rect srcSelect = { touch_ss_w - menuW, 0, menuW, touch_ss_h };

        dstSelect = (SDL_Rect){ (DISPLAY_WIDTH / 2) - 45, 12, 32, 32 };
        dstStart  = (SDL_Rect){ (DISPLAY_WIDTH / 2) + 13, 12, 32, 32 };

        SDL_RenderCopy(sdlRenderer, touch_start_select_texture, &srcStart, &dstStart);
        SDL_RenderCopy(sdlRenderer, touch_start_select_texture, &srcSelect, &dstSelect);

        int btnW = touch_ab_w / 4; 

        int frameA = (a_touch_finger != -1) ? 1 : 0;
        int frameB = (b_touch_finger != -1) ? 3 : 2;

        SDL_Rect srcA = { frameA * btnW, 0, btnW, touch_ab_h };
        SDL_Rect srcB = { frameB * btnW, 0, btnW, touch_ab_h };

        SDL_Rect dstA = { DISPLAY_WIDTH - 145 - (frameA == 1 ? 2 : 0), DISPLAY_HEIGHT - 80, 60, 60 };
        SDL_Rect dstB = { DISPLAY_WIDTH - 75  - (frameB == 3 ? 2 : 0), DISPLAY_HEIGHT - 80, 60, 60 };

        SDL_RenderCopy(sdlRenderer, touch_ab_texture, &srcA, &dstA);
        SDL_RenderCopy(sdlRenderer, touch_ab_texture, &srcB, &dstB);

float dpadScaleX = dstDpad.w / 64.0f;
float dpadScaleY = dstDpad.h / 64.0f;

int x0 = dstDpad.x;
int x1 = dstDpad.x + (int)(26 * dpadScaleX + 0.5f);
int x2 = dstDpad.x + (int)(38 * dpadScaleX + 0.5f);
int x3 = dstDpad.x + dstDpad.w;

int y0 = dstDpad.y;
int y1 = dstDpad.y + (int)(25 * dpadScaleY + 0.5f);
int y2 = dstDpad.y + (int)(38 * dpadScaleY + 0.5f);
int y3 = dstDpad.y + dstDpad.h;

SDL_Rect dstArmUp     = { x1, y0, x2 - x1, y1 - y0 };
SDL_Rect dstArmDown   = { x1, y2, x2 - x1, y3 - y2 };
SDL_Rect dstArmLeft   = { x0, y1, x1 - x0, y2 - y1 };
SDL_Rect dstArmRight  = { x2, y1, x3 - x2, y2 - y1 };
SDL_Rect dstArmCenter = { x1, y1, x2 - x1, y2 - y1 };

SDL_Rect armUp     = { 26, 0,  12, 25 };
SDL_Rect armDown   = { 26, 38, 12, 26 };
SDL_Rect armLeft   = { 0,  25, 26, 13 };
SDL_Rect armRight  = { 38, 25, 26, 13 };
SDL_Rect armCenter = { 26, 25, 12, 13 };

int dpadIdleOX    = 81;
int dpadPressedOX = 154;
int dpadCellOY    = 8;

SDL_SetRenderTarget(sdlRenderer, dpadComposite);
SDL_SetRenderDrawColor(sdlRenderer, 0, 0, 0, 0);
SDL_RenderClear(sdlRenderer);

SDL_Rect srcDpadBase = { 8, 8, 64, 64 };
SDL_Rect dstDpadBaseLocal = { 0, 0, 64, 64 };
SDL_RenderCopy(sdlRenderer, touch_dpad_texture, &srcDpadBase, &dstDpadBaseLocal);

#define COMPOSITE_DPAD_ARM(armRect, held) \
    do { \
        SDL_Rect srcArm = { (held ? dpadPressedOX : dpadIdleOX) + (armRect).x, \
                             dpadCellOY + (armRect).y, \
                             (armRect).w, (armRect).h }; \
        SDL_RenderCopy(sdlRenderer, touch_dpad_texture, &srcArm, &(armRect)); \
    } while (0)

COMPOSITE_DPAD_ARM(armUp,    (keys & DPAD_UP)    != 0);
COMPOSITE_DPAD_ARM(armDown,  (keys & DPAD_DOWN)  != 0);
COMPOSITE_DPAD_ARM(armLeft,  (keys & DPAD_LEFT)  != 0);
COMPOSITE_DPAD_ARM(armRight, (keys & DPAD_RIGHT) != 0);
COMPOSITE_DPAD_ARM(armCenter, false);

#undef COMPOSITE_DPAD_ARM

SDL_SetRenderTarget(sdlRenderer, NULL);
SDL_RenderCopy(sdlRenderer, dpadComposite, NULL, &dstDpad);

SDL_Rect srcMp = { 0, 0, 16, 16 };
if (mp_touch_finger != -1)
    SDL_SetTextureColorMod(touch_mp_texture, 180, 180, 180);
else
    SDL_SetTextureColorMod(touch_mp_texture, 255, 255, 255);
SDL_RenderCopy(sdlRenderer, touch_mp_texture, &srcMp, &dstMp);
#endif
        SDL_RenderPresent(sdlRenderer);
#if ENABLE_VRAM_VIEW
        SDL_RenderPresent(vramRenderer);
#endif
#endif
    }

    CloseSaveFile();

    SDL_DestroyWindow(sdlWindow);
    SDL_Quit();
#ifdef __PSP__
    sceKernelExitGame();
#endif
    exit(0);
#undef HANDLE_VBLANK_INTRS
}

static void ReadSaveFile(char *path)
{
    // Check whether the saveFile exists, and create it if not
    sSaveFile = fopen(path, "r+b");
    if (sSaveFile == NULL) {
        sSaveFile = fopen(path, "w+b");
    }

    fseek(sSaveFile, 0, SEEK_END);
    int fileSize = ftell(sSaveFile);
    fseek(sSaveFile, 0, SEEK_SET);

    // Only read as many bytes as fit inside the buffer
    // or as many bytes as are in the file
    int bytesToRead = (fileSize < sizeof(FLASH_BASE)) ? fileSize : sizeof(FLASH_BASE);

    int bytesRead = fread(FLASH_BASE, 1, bytesToRead, sSaveFile);

    // Fill the buffer if the savefile was just created or smaller than the buffer itself
    for (int i = bytesRead; i < sizeof(FLASH_BASE); i++) {
        FLASH_BASE[i] = 0xFF;
    }
}

static void StoreSaveFile()
{
    if (sSaveFile != NULL) {
        fseek(sSaveFile, 0, SEEK_SET);
        fwrite(FLASH_BASE, 1, sizeof(FLASH_BASE), sSaveFile);
    }
}

void Platform_StoreSaveFile(void) { StoreSaveFile(); }

static void CloseSaveFile()
{
    if (sSaveFile != NULL) {
        fclose(sSaveFile);
    }
}

static u16 keys;

// Key mappings
#define KEY_A_BUTTON      SDLK_c
#define KEY_B_BUTTON      SDLK_x
#define KEY_START_BUTTON  SDLK_RETURN
#define KEY_SELECT_BUTTON SDLK_BACKSLASH
#define KEY_L_BUTTON      SDLK_s
#define KEY_R_BUTTON      SDLK_d
#define KEY_DPAD_UP       SDLK_UP
#define KEY_DPAD_DOWN     SDLK_DOWN
#define KEY_DPAD_LEFT     SDLK_LEFT
#define KEY_DPAD_RIGHT    SDLK_RIGHT

#define HANDLE_KEYUP(key)                                                                                                                  \
    case KEY_##key:                                                                                                                        \
        keys &= ~key;                                                                                                                      \
        break;

#define HANDLE_KEYDOWN(key)                                                                                                                \
    case KEY_##key:                                                                                                                        \
        keys |= key;                                                                                                                       \
        break;

#ifdef __PSP__
#define BTN_TRIANGLE 0
#define BTN_CIRCLE   1
#define BTN_CROSS    2
#define BTN_SQUARE   3
#define BTN_LTRIGGER 4
#define BTN_RTRIGGER 5
#define BTN_DOWN     6
#define BTN_LEFT     7
#define BTN_UP       8
#define BTN_RIGHT    9
#define BTN_SELECT   10
#define BTN_START    11

static u16 PollJoystickButtons(void)
{
    u16 newKeys = 0;
    if (joystick == NULL)
        return newKeys;

    SDL_JoystickUpdate();

    if (SDL_JoystickGetButton(joystick, BTN_CROSS))
        newKeys |= A_BUTTON;
    if (SDL_JoystickGetButton(joystick, BTN_CIRCLE))
        newKeys |= B_BUTTON;
    if (SDL_JoystickGetButton(joystick, BTN_SQUARE))
        newKeys |= B_BUTTON; // Square also B
    if (SDL_JoystickGetButton(joystick, BTN_START))
        newKeys |= START_BUTTON;
    if (SDL_JoystickGetButton(joystick, BTN_SELECT))
        newKeys |= SELECT_BUTTON;
    if (SDL_JoystickGetButton(joystick, BTN_LTRIGGER))
        newKeys |= L_BUTTON;
    if (SDL_JoystickGetButton(joystick, BTN_RTRIGGER))
        newKeys |= R_BUTTON;
    if (SDL_JoystickGetButton(joystick, BTN_UP))
        newKeys |= DPAD_UP;
    if (SDL_JoystickGetButton(joystick, BTN_DOWN))
        newKeys |= DPAD_DOWN;
    if (SDL_JoystickGetButton(joystick, BTN_LEFT))
        newKeys |= DPAD_LEFT;
    if (SDL_JoystickGetButton(joystick, BTN_RIGHT))
        newKeys |= DPAD_RIGHT;

    return newKeys;
}

#endif

u32 fullScreenFlags = 0;
static SDL_DisplayMode sdlDispMode = { 0 };

void Platform_QueueAudio(const s16 *data, uint32_t bytesCount)
{
    if (headless) {
        return;
    }
    // Reset the audio buffer if we are 10 frames out of sync
    // If this happens it suggests there was some OS level lag
    // in playing audio. The queue length should remain stable at < 10 otherwise
    if (SDL_GetQueuedAudioSize(1) > (bytesCount * 10)) {
        SDL_ClearQueuedAudio(1);
    }

    SDL_QueueAudio(1, data, bytesCount);
    // printf("Queueing %d\n, QueueSize %d\n", bytesCount, SDL_GetQueuedAudioSize(1));
}
#if defined(__ANDROID__)

static int dpadPixelCenterX, dpadPixelCenterY, dpadPixelRadius;

static bool IsInsideDpad(int px, int py)
{
    int dx = px - dpadPixelCenterX;
    int dy = py - dpadPixelCenterY;
    return (dx * dx + dy * dy) <= (dpadPixelRadius * dpadPixelRadius);
}

static u16 ComputeDpadKeys(int px, int py)
{
    int dx = px - dpadPixelCenterX;
    int dy = py - dpadPixelCenterY;
    float dist = sqrtf((float)(dx * dx + dy * dy));

    if (dist < DPAD_DEADZONE_PX)
        return 0;

    float angle = atan2f((float)-dy, (float)dx) * 180.0f / (float)M_PI;
    if (angle < 0) angle += 360.0f;

    if (angle >= 337.5f || angle < 22.5f)   return DPAD_RIGHT;
    if (angle < 67.5f)                       return DPAD_RIGHT | DPAD_UP;
    if (angle < 112.5f)                      return DPAD_UP;
    if (angle < 157.5f)                      return DPAD_UP | DPAD_LEFT;
    if (angle < 202.5f)                      return DPAD_LEFT;
    if (angle < 247.5f)                      return DPAD_LEFT | DPAD_DOWN;
    if (angle < 292.5f)                      return DPAD_DOWN;
    return DPAD_DOWN | DPAD_RIGHT;
}
#endif

void ProcessSDLEvents(void)
{
    SDL_Event event;
    
    while (SDL_PollEvent(&event)) {
        SDL_Keycode keyCode = event.key.keysym.sym;
        Uint16 keyMod = event.key.keysym.mod;

        switch (event.type) {
            
#if defined(__ANDROID__)
            
            case SDL_CONTROLLERDEVICEADDED:
                if (!active_gamepad) {
                    active_gamepad = SDL_GameControllerOpen(event.cdevice.which);
                }
                break;

            case SDL_CONTROLLERDEVICEREMOVED:
                if (active_gamepad) {
                    SDL_GameControllerClose(active_gamepad);
                    active_gamepad = NULL;
                }
                break;

            case SDL_CONTROLLERBUTTONDOWN:
                switch (event.cbutton.button) {
                    case SDL_CONTROLLER_BUTTON_A:             keys |= A_BUTTON; break;
                    case SDL_CONTROLLER_BUTTON_B:             keys |= B_BUTTON; break;
                    case SDL_CONTROLLER_BUTTON_START:         keys |= START_BUTTON; break;
                    case SDL_CONTROLLER_BUTTON_BACK:          keys |= SELECT_BUTTON; break;
                    case SDL_CONTROLLER_BUTTON_LEFTSHOULDER:  keys |= L_BUTTON; break;
                    case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER: keys |= R_BUTTON; break;
                    case SDL_CONTROLLER_BUTTON_DPAD_UP:       keys |= DPAD_UP; break;
                    case SDL_CONTROLLER_BUTTON_DPAD_DOWN:     keys |= DPAD_DOWN; break;
                    case SDL_CONTROLLER_BUTTON_DPAD_LEFT:     keys |= DPAD_LEFT; break;
                    case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:    keys |= DPAD_RIGHT; break;
                }
                break;

            case SDL_CONTROLLERBUTTONUP:
                switch (event.cbutton.button) {
                    case SDL_CONTROLLER_BUTTON_A:             keys &= ~A_BUTTON; break;
                    case SDL_CONTROLLER_BUTTON_B:             keys &= ~B_BUTTON; break;
                    case SDL_CONTROLLER_BUTTON_START:         keys &= ~START_BUTTON; break;
                    case SDL_CONTROLLER_BUTTON_BACK:          keys &= ~SELECT_BUTTON; break;
                    case SDL_CONTROLLER_BUTTON_LEFTSHOULDER:  keys &= ~L_BUTTON; break;
                    case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER: keys &= ~R_BUTTON; break;
                    case SDL_CONTROLLER_BUTTON_DPAD_UP:       keys &= ~DPAD_UP; break;
                    case SDL_CONTROLLER_BUTTON_DPAD_DOWN:     keys &= ~DPAD_DOWN; break;
                    case SDL_CONTROLLER_BUTTON_DPAD_LEFT:     keys &= ~DPAD_LEFT; break;
                    case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:    keys &= ~DPAD_RIGHT; break;
                }
                break;

            case SDL_CONTROLLERAXISMOTION:
{
    const int DEADZONE = 8000;

    switch (event.caxis.axis)
    {
        case SDL_CONTROLLER_AXIS_LEFTX:
            keys &= ~(DPAD_LEFT | DPAD_RIGHT);

            if (event.caxis.value < -DEADZONE)
                keys |= DPAD_LEFT;
            else if (event.caxis.value > DEADZONE)
                keys |= DPAD_RIGHT;
            break;

        case SDL_CONTROLLER_AXIS_LEFTY:
            keys &= ~(DPAD_UP | DPAD_DOWN);

            if (event.caxis.value < -DEADZONE)
                keys |= DPAD_UP;
            else if (event.caxis.value > DEADZONE)
                keys |= DPAD_DOWN;
            break;
    }
    break;
}
case SDL_FINGERDOWN:
{
    float x = event.tfinger.x;
    float y = event.tfinger.y;
    SDL_FingerID finger = event.tfinger.fingerId;

    int px = (int)(x * DISPLAY_WIDTH);
    int py = (int)(y * DISPLAY_HEIGHT);

    /* D-Pad */
    if (IsInsideDpad(px, py))
    {
        if (dpad_touch_finger == -1)
        {
    dpad_touch_finger = finger;
    keys |= ComputeDpadKeys(px, py);
        }
    }

    /* L button */
    else if (x < 0.20f && y < 0.25f)
    {
        if (l_touch_finger == -1)
        {
            l_touch_finger = finger;
            keys |= L_BUTTON;
        }
    }

    /* R button */
    else if (x > 0.80f && y < 0.25f)
    {
        if (r_touch_finger == -1)
        {
            r_touch_finger = finger;
            keys |= R_BUTTON;
        }
    }

    /* Select */
    else if (x > 0.40f && x < 0.55f && y < 0.15f)
    {
        if (select_touch_finger == -1)
        {
            select_touch_finger = finger;
            keys |= SELECT_BUTTON;
        }
    }

    /* Start */
    else if (x > 0.55f && x < 0.70f && y < 0.15f)
    {
        if (start_touch_finger == -1)
        {
            start_touch_finger = finger;
            keys |= START_BUTTON;
        }
    }

    /* A button */
    else if (x > 0.65f && x < 0.82f && y > 0.60f && y < 0.85f)
    {
        if (a_touch_finger == -1)
        {
            a_touch_finger = finger;
            keys |= A_BUTTON;
        }
    }

    /* B button */
    else if (x > 0.82f && y > 0.60f && y < 0.85f)
    {
        if (b_touch_finger == -1)
        {
            b_touch_finger = finger;
            keys |= B_BUTTON;
        }
    }

    else if (x >= dstMp.x / (float)DISPLAY_WIDTH && x <= (dstMp.x + dstMp.w) / (float)DISPLAY_WIDTH &&
         y >= dstMp.y / (float)DISPLAY_HEIGHT && y <= (dstMp.y + dstMp.h) / (float)DISPLAY_HEIGHT)
{
    if (mp_touch_finger == -1)
        mp_touch_finger = finger;
}
}
break;

case SDL_FINGERMOTION:
{
    float x = event.tfinger.x;
    float y = event.tfinger.y;
    SDL_FingerID finger = event.tfinger.fingerId;

    int px = (int)(x * DISPLAY_WIDTH);
    int py = (int)(y * DISPLAY_HEIGHT);

    if (finger == dpad_touch_finger)
    {
        keys &= ~(DPAD_UP | DPAD_DOWN | DPAD_LEFT | DPAD_RIGHT);

        if (IsInsideDpad(px, py))
            keys |= ComputeDpadKeys(px, py);
        else
            dpad_touch_finger = -1;
    }
}
break;
            
case SDL_FINGERUP:
{
    SDL_FingerID finger = event.tfinger.fingerId;

    if (dpad_touch_finger == finger)
    {
        dpad_touch_finger = -1;
        keys &= ~(DPAD_UP | DPAD_DOWN | DPAD_LEFT | DPAD_RIGHT);
    }

    if (a_touch_finger == finger)
    {
        a_touch_finger = -1;
        keys &= ~A_BUTTON;
    }

    if (b_touch_finger == finger)
    {
        b_touch_finger = -1;
        keys &= ~B_BUTTON;
    }

    if (l_touch_finger == finger)
    {
        l_touch_finger = -1;
        keys &= ~L_BUTTON;
    }

    if (r_touch_finger == finger)
    {
        r_touch_finger = -1;
        keys &= ~R_BUTTON;
    }

    if (start_touch_finger == finger)
    {
        start_touch_finger = -1;
        keys &= ~START_BUTTON;
    }

    if (select_touch_finger == finger)
    {
        select_touch_finger = -1;
        keys &= ~SELECT_BUTTON;
    }

    if (mp_touch_finger == finger)
    {
        float x = event.tfinger.x;
        float y = event.tfinger.y;
        bool stillInside = (x >= dstMp.x / (float)DISPLAY_WIDTH && x <= (dstMp.x + dstMp.w) / (float)DISPLAY_WIDTH &&
                             y >= dstMp.y / (float)DISPLAY_HEIGHT && y <= (dstMp.y + dstMp.h) / (float)DISPLAY_HEIGHT);
        mp_touch_finger = -1;
        if (stillInside)
        {
            OpenMultiplayerMenu();
        }
    }
}
break;
#endif
            case SDL_QUIT:
                isRunning = false;
                break;
            case SDL_KEYUP:
                switch (event.key.keysym.sym) {
                    HANDLE_KEYUP(A_BUTTON)
                    HANDLE_KEYUP(B_BUTTON)
                    HANDLE_KEYUP(START_BUTTON)
                    HANDLE_KEYUP(SELECT_BUTTON)
                    HANDLE_KEYUP(L_BUTTON)
                    HANDLE_KEYUP(R_BUTTON)
                    HANDLE_KEYUP(DPAD_UP)
                    HANDLE_KEYUP(DPAD_DOWN)
                    HANDLE_KEYUP(DPAD_LEFT)
                    HANDLE_KEYUP(DPAD_RIGHT)
                    case SDLK_SPACE:
                        if (speedUp) {
                            speedUp = false;
                            timeScale = 1.0;
                            SDL_ClearQueuedAudio(1);
                            SDL_PauseAudio(0);
                        }
                        break;
                }
                break;
            case SDL_KEYDOWN:
                if (keyCode == SDLK_RETURN && (keyMod & KMOD_ALT)) {
                    fullScreenFlags ^= SDL_WINDOW_FULLSCREEN_DESKTOP;
                    if (fullScreenFlags & SDL_WINDOW_FULLSCREEN_DESKTOP) {
                        SDL_GetWindowDisplayMode(sdlWindow, &sdlDispMode);
                        preFullscreenVideoScale = videoScale;
                    } else {
                        SDL_SetWindowDisplayMode(sdlWindow, &sdlDispMode);
                        videoScale = preFullscreenVideoScale;
                    }
                    SDL_SetWindowFullscreen(sdlWindow, fullScreenFlags);

                    SDL_SetWindowSize(sdlWindow, DISPLAY_WIDTH * videoScale, DISPLAY_HEIGHT * videoScale);
                    videoScaleChanged = FALSE;
                } else
                    switch (event.key.keysym.sym) {
                        HANDLE_KEYDOWN(A_BUTTON)
                        HANDLE_KEYDOWN(B_BUTTON)
                        HANDLE_KEYDOWN(START_BUTTON)
                        HANDLE_KEYDOWN(SELECT_BUTTON)
                        HANDLE_KEYDOWN(L_BUTTON)
                        HANDLE_KEYDOWN(R_BUTTON)
                        HANDLE_KEYDOWN(DPAD_UP)
                        HANDLE_KEYDOWN(DPAD_DOWN)
                        HANDLE_KEYDOWN(DPAD_LEFT)
                        HANDLE_KEYDOWN(DPAD_RIGHT)
                        case SDLK_r:
                            if (event.key.keysym.mod & (KMOD_LCTRL | KMOD_RCTRL)) {
                                DoSoftReset();
                            }
                            break;
                        case SDLK_p:
                            if (event.key.keysym.mod & (KMOD_LCTRL | KMOD_RCTRL)) {
                                paused = !paused;
                            }
                            break;
                        case SDLK_SPACE:
                            if (!speedUp) {
                                speedUp = true;
                                timeScale = SPEEDUP_SCALE;
                                SDL_PauseAudio(1);
                            }
                            break;
                        case SDLK_F10:
                            paused = true;
                            stepOneFrame = true;
                            break;
                    }
                break;
            case SDL_WINDOWEVENT:
                if (event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                    unsigned int w = event.window.data1;
                    unsigned int h = event.window.data2;

                    videoScale = 0;
                    if (w / DISPLAY_WIDTH > videoScale)
                        videoScale = w / DISPLAY_WIDTH;
                    if (h / DISPLAY_HEIGHT > videoScale)
                        videoScale = h / DISPLAY_HEIGHT;
                    if (videoScale < 1)
                        videoScale = 1;

                    videoScaleChanged = true;
                }
                break;
        }
    }
}

u16 Platform_GetKeyInput(void)
{
#ifdef _WIN32
    SharedKeys gamepadKeys = GetXInputKeys();

    speedUp = (gamepadKeys & KEY_SPEEDUP) ? true : false;

    if (speedUp) {
        timeScale = SPEEDUP_SCALE;
        SDL_PauseAudio(1);
    } else {
        timeScale = 1.0f;
        SDL_PauseAudio(0);
    }

    return (gamepadKeys != 0) ? gamepadKeys : keys;
#endif

#ifdef __PSP__
    return keys | PollJoystickButtons();
#endif

    return keys;
}

#if ENABLE_VRAM_VIEW
void VramDraw(SDL_Texture *texture)
{
    memset(vramBuffer, 0, sizeof(vramBuffer));
    gpsp_draw_vram_view(vramBuffer);
    SDL_UpdateTexture(texture, NULL, vramBuffer, VRAM_VIEW_WIDTH * sizeof(Uint16));
}
#endif

void VDraw(SDL_Texture *texture)
{
    gpsp_draw_frame(gameImage);
    SDL_UpdateTexture(texture, NULL, gameImage, DISPLAY_WIDTH * sizeof(Uint16));
    REG_VCOUNT = DISPLAY_HEIGHT + 1; // prep for being in VBlank period
}
