#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>
#include <unistd.h>

#include <libtcc.h>
#include <SDL2/SDL.h>

#define S_WIDTH 1200
#define S_HEIGHT 900
#define FS_WIDTH 1200.0
#define FS_HEIGHT 900.0

#define STEP_DOWN 0.875
#define STEP_UP 1.125

int clamp(int x, int min, int max)
{
    if (x < min)
        return min;
    if (x > max)
        return max;
    return x;
}

double (*graph_func)(const double x);

/* default */
double f(const double x)
{
    return x;
}

const char *graph_func_template =
    "#include <tcclib.h>"
    "double graph_func(const double x){"
    "return %s;"
    "}";

double scale = 1;
double x_offset = 0;
double y_offset = 0;

static inline int to_screen_x(double x)
{
    return (x - x_offset) * scale;
}
static inline double to_world_x(float x)
{
    return (x / scale) + x_offset - (S_WIDTH / 2);
}
static inline int to_screen_y(double y)
{
    return (y - y_offset) * scale;
}
static inline double to_world_y(float y)
{
    return (y / scale) + y_offset - (S_WIDTH / 2);
}

void render_graph(SDL_Surface *surface)
{
    if (SDL_LockSurface(surface) < 0)
    {
        printf("error in SDL_LockSurface: %s", SDL_GetError());
        exit(EXIT_FAILURE);
    }
    memset(surface->pixels, 0, surface->pitch * S_HEIGHT);

    unsigned int *pixels = surface->pixels;

    /* horizontal graph line */
    unsigned int set_y = to_screen_y(S_HEIGHT / 2);
    if (set_y < S_HEIGHT)
    {
        for (int x = 0; x < S_WIDTH; ++x)
        {
            pixels[set_y * S_WIDTH + x] = 0x737373ff;
        }
    }

    /* vertical graph line */
    unsigned int set_x = to_screen_x(S_WIDTH / 2);
    if (set_x < S_WIDTH)
    {
        for (int y = 0; y < S_HEIGHT; ++y)
        {
            pixels[y * S_WIDTH + set_x] = 0x737373ff;
        }
    }


    // double last_y = S_HEIGHT - graph_func(to_world_x(0)) - S_HEIGHT / 2;

    for (double x = 0; x < S_WIDTH; x += 0.005)
    {
        double world_y = graph_func(to_world_x(x));
        world_y = S_HEIGHT - world_y;
        world_y -= S_HEIGHT / 2;

        //int y1 = to_screen_y(last_y);
        int y2 = to_screen_y(world_y);

        if (y2 < 0 || y2 >= S_HEIGHT) continue;
        pixels[y2 * S_WIDTH + (int)x] = 0xffffffff;

        // y1 = clamp(y1, 0, S_HEIGHT - 1);
        // y2 = clamp(y2, 0, S_HEIGHT - 1);

        // int step = (y2 < y1) ? -1 : 1;
        // int y = y1;
        // for (int i = 0; i < abs(y1 - y2) + 1; ++i)
        // {
        //     unsigned int pos = y * S_WIDTH + (unsigned int)x;
        //     pixels[pos] = 0xffffffff;
        //     y += step;
        // }

        // last_y = world_y;
    }
    
    SDL_UnlockSurface(surface);
}

int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    if (SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        fprintf(stderr, "Error, could not init SDL: %s\n", SDL_GetError());
        return EXIT_FAILURE;
    }

    SDL_Window *window = SDL_CreateWindow("graphs",
                                          SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                                          S_WIDTH, S_HEIGHT, SDL_WINDOW_SHOWN);

    if (window == NULL)
    {
        fprintf(stderr, "SDL_CreateWindow error: %s\n", SDL_GetError());
        return EXIT_FAILURE;
    }

    SDL_Surface *surface = SDL_GetWindowSurface(window);

    if (surface == NULL)
    {
        SDL_DestroyWindow(window);
        fprintf(stderr, "SDL_CreateRenderer error: %s\n", SDL_GetError());
        return EXIT_FAILURE;
    }

    /* linear function by default */
    graph_func = &f;

    TCCState *s = tcc_new();
    if (!s)
    {
        printf("Can't create a TCC context\n");
        return 1;
    }
    tcc_set_output_type(s, TCC_OUTPUT_MEMORY);

    bool quit = false;
    bool mouse_down = false;
    Sint32 mouse_x, mouse_y;
    SDL_Event e;
    while (!quit)
    {
        while (SDL_PollEvent(&e))
        {
            switch (e.type)
            {
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
                double x_before_scale = to_world_x(mouse_x);
                double y_before_scale = to_world_y(mouse_y);

                if (e.wheel.y > 0)
                    scale *= STEP_UP;
                else
                    scale *= STEP_DOWN;

                double x_after_scale = to_world_x(mouse_x);
                double y_after_scale = to_world_y(mouse_y);

                x_offset += x_before_scale - x_after_scale;
                y_offset += y_before_scale - y_after_scale;
                break;
            case SDL_MOUSEMOTION:
                mouse_x = e.motion.x;
                mouse_y = e.motion.y;
                if (mouse_down)
                {
                    x_offset -= e.motion.xrel / scale;
                    y_offset -= e.motion.yrel / scale;
                }
                break;
            case SDL_KEYDOWN:
                switch (e.key.keysym.scancode)
                {
                case SDL_SCANCODE_RETURN:
                    char scan_buf[256];
                    char func_buf[256];

                    fflush(stdin);
                    printf("f(x) = ");
                    scanf("%s", scan_buf);

                    sprintf(func_buf, graph_func_template, scan_buf);

                    tcc_delete(s);
                    s = tcc_new();
                    if (!s)
                    {
                        printf("Can't create a TCC context\n");
                        return 1;
                    }
                    tcc_set_output_type(s, TCC_OUTPUT_MEMORY);

                    if (tcc_compile_string(s, func_buf) > 0)
                    {
                        printf("Compilation error.\n");
                        break;
                    }

                    tcc_relocate(s, TCC_RELOCATE_AUTO);

                    graph_func = tcc_get_symbol(s, "graph_func");

                    break;
                default:
                }
                break;
            default:
            }
        }

        render_graph(surface);
        SDL_UpdateWindowSurface(window);
    }

    tcc_delete(s);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return EXIT_SUCCESS;
}