#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include <math.h>

#include <unistd.h>

#include <SDL2/SDL.h>
#include "include/lua.h"
#include "include/lualib.h"
#include "include/lauxlib.h"

#define S_WIDTH 1200
#define S_HEIGHT 900
#define FS_WIDTH 1200.0
#define FS_HEIGHT 900.0

#define STEP_DOWN 0.95
#define STEP_UP 1.05

const char* func_template = "f = function(x) return %s end";

double scale = 1;
float x_offset = 0;
float y_offset = 0;

typedef struct screen_pos {
    int x, y;
} screen_pos;

typedef struct world_pos {
    double x, y;
} world_pos; 

static inline screen_pos to_screen(double x, double y) {
    return (screen_pos) {
        .x = ((x - x_offset) * scale),
        .y = ((y - y_offset) * scale),
    };
}  

static inline world_pos to_world(int x, int y) {
    return (world_pos) {
        .x = ((x / scale) + x_offset - (S_WIDTH/2)),
        .y = ((y / scale) + y_offset - (S_HEIGHT/2)), 
    };
}

double graph_func(double x, lua_State* L) {
    lua_getglobal(L, "f");  /* function to be called */
    lua_pushnumber(L, x);   /* push 1st argument */

    if (lua_pcall(L, 1, 1, 0) != 0) {
        printf("error running function `f': %s\n",
                lua_tostring(L, -1));
        luaL_dostring(L, "f = nil");
        return 0;
    }

    if (!lua_isnumber(L, -1)) {
        puts("function `f' must return a number");
        luaL_dostring(L, "f = nil");
        return 0;
    }
    double z = lua_tonumber(L, -1);
    lua_pop(L, 1);  /* pop returned value */
    return z;   
}

void render_graph(SDL_Surface *surface, lua_State* L)
{
    if (SDL_LockSurface(surface) < 0) {
        printf("error in SDL_LockSurface: %s", SDL_GetError());
        exit(1);
    }
    memset(surface->pixels, 0, surface->pitch*S_HEIGHT);

    /* horizontal graph line */
    unsigned int set_y = to_screen(0, S_HEIGHT/2).y;
    if (set_y < S_HEIGHT) {
        for (int x = 0; x < S_WIDTH; ++x) {
            ((int*)surface->pixels)[set_y * S_WIDTH + x] = 0x737373ff;
        }
    }

    /* vertical graph line */
    unsigned int set_x = to_screen(S_WIDTH/2, 0).x;
    if (set_x < S_WIDTH) {
        for (int y = 0; y < S_HEIGHT; ++y) {
            ((int*)surface->pixels)[y * S_WIDTH + set_x] = 0x737373ff;
        }
    }

    for (int x = 0; x < S_WIDTH; ++x)
    {
        double y = graph_func(to_world(x , 0).x, L);

        //y /= scale; /* scale it */
        y = S_HEIGHT - y; /* invert it */
        y -= S_HEIGHT/2;
        y = to_screen(0, y).y;
        
        if (y >= 0 && y < FS_HEIGHT) {
            int pos = ((int)y * S_WIDTH) + x;
            ((int*)surface->pixels)[pos] = 0xffffffff;
        }
    }
    SDL_UnlockSurface(surface);
}

int main(int argc, char *argv[])
{ 
    /* don't need them */
    (void) argc; (void) argv;

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "Error, could not init SDL: %s\n", SDL_GetError());
        return EXIT_FAILURE;
    }

    SDL_Window *window = SDL_CreateWindow("graphs", 
                                    SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                                    S_WIDTH, S_HEIGHT, SDL_WINDOW_SHOWN);
    
    if (window == NULL) {
        fprintf(stderr, "SDL_CreateWindow error: %s\n", SDL_GetError());
        return EXIT_FAILURE;
    }

    SDL_Surface *surface = SDL_GetWindowSurface(window);

    if (surface == NULL) {
        SDL_DestroyWindow(window);
        fprintf(stderr, "SDL_CreateRenderer error: %s\n", SDL_GetError());
        return EXIT_FAILURE;
    }

    lua_State *L = luaL_newstate();
    luaL_openlibs(L);
    luaL_dostring(L, "f = function(x) return x*x end");

    bool quit = false;
    bool graph_okay = true;
    bool mouse_down = false;
    Sint32 mouse_x, mouse_y;
    SDL_Event e;
    while (!quit) {
        while (SDL_PollEvent(&e)) {
            switch (e.type) {
            case SDL_QUIT:
                quit = true;
                break;
            case SDL_MOUSEBUTTONDOWN:
                mouse_down = true;
                break;
            case SDL_MOUSEBUTTONUP:
                mouse_down = false;
                break;
            case SDL_MOUSEWHEEL:
                world_pos before_scale = to_world(mouse_x, mouse_y);
                if (e.wheel.y > 0) 
                    scale *= STEP_UP;  
                else
                    scale *= STEP_DOWN;
                world_pos after_scale = to_world(mouse_x, mouse_y);

                x_offset += before_scale.x - after_scale.x;
                y_offset += before_scale.y - after_scale.y;
                break;
            case SDL_MOUSEMOTION:
                mouse_x = e.motion.x;
                mouse_y = e.motion.y;
                if (mouse_down) {
                    x_offset -= e.motion.xrel / scale;
                    y_offset -= e.motion.yrel / scale;
                }
                break;
            case SDL_KEYDOWN:
                switch (e.key.keysym.scancode) 
                {
                case SDL_SCANCODE_RETURN:
                    char buf[128];
                    printf("f(x) = ");
                    scanf("%s.128", buf);
                    char func_buf[164];
                    sprintf(func_buf, func_template, buf);
                    luaL_dostring(L, func_buf);
                    graph_func(1, L);
                    lua_getglobal(L, "f");
                    if (lua_isnil(L, -1)) {
                        puts("function f isn't valid");
                        graph_okay = false;
                        break;
                    }
                    graph_okay = true;
                    break;
                default: {}
                }
                break;
            default: {}
            }
        }
        
        if (graph_okay) {
            render_graph(surface, L);
        }
        SDL_UpdateWindowSurface(window);
    }

    lua_close(L);
    SDL_DestroyWindow(window);
    SDL_Quit();
}