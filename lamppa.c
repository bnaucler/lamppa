#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <limits.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#define GRIDX 5     // grid x size
#define GRIDY 5     // grid y size
#define BLKSZ 90    // block size in pixels
#define TXFH 30
#define INITSC 15   // initial scramble level / difficulty

#define FONT "arial.ttf"
#define TCOL_R 255
#define TCOL_G 255
#define TCOL_B 255

// game settings
typedef struct Flag {
    int x,          // height
        y,          // width
        s;          // # of moves to scramble board
} Flag;

// extracts int from str
static int matoi(char* str) {

    char *ptr;

    long lret = strtol(str, &ptr, 10);

    if(str == ptr) return 0;

    if(lret > INT_MAX) lret = INT_MAX;
    else if(lret < INT_MIN) lret = INT_MIN;

    return (int)lret;
}

// grid constructor
char **mkgrid(Flag *f) {

    char **g = calloc(f->y, sizeof(char**));

    for(int i = 0; i < f->y; i++) g[i] = calloc(f->x, sizeof(char*));

    return g;
}

// grid deconstructor
void freegrid(char **g, Flag *f) {

    for(int i = 0; i < f->y; i++) free(g[i]);
    free(g);
}

// toggles individual bit
void flipbit(char **g, int x, int y, Flag *f) {

    if(x >= f->x || y >= f->y || x < 0 || y < 0) return;

    g[y][x] = g[y][x] == 0 ? 1 : 0;
}

// toggles bit and surrounding bits
void flip(char **g, int tx, int ty, Flag *f) {

    if(tx >= f->x || ty >= f->y || tx < 0 || ty < 0) return;

    for(int y = ty - 1; y < ty + 2; y++) {
        for(int x = tx - 1; x < tx + 2; x++) {
            flipbit(g, x, y, f);
        }
    }
}

// check if winning condition has been met
int wincheck(char **g, Flag *f) {

    int tot = f->x * f->y;
    int ctr = 0;

    for(int y = 0; y < f->y; y++) {
        for(int x = 0; x < f->x; x++) {
            ctr += g[y][x];
        }
    }

    return ctr == tot ? 1 : 0;
}

// resets grid to initial position
void resetgrid(char **g, Flag *f) {

    for(int y = 0; y < f->y; y++) {
        for(int x = 0; x < f->x; x++) {
             g[y][x] = 0;
        }
    }
}

// randomizes grid init position
void initgrid(char **g, Flag *f) {

    int x, y;

    for(int i = 0; i < INITSC; i++) {
      x = (rand() % f->x);
      y = (rand() % f->y);
      flip(g, x, y, f);
    }

}

// reads and processes incoming SDL2 events
int readevent(SDL_Event *event, char **g, uint16_t *ctr, char *haswon, Flag *f) {

    int mx, my;

    while (SDL_PollEvent(event)) {
        switch (event->type) {

        case SDL_MOUSEBUTTONDOWN:
            SDL_GetMouseState(&mx, &my);
            flip(g, mx / BLKSZ, my / BLKSZ, f);
            *ctr = *ctr + 1;
            *haswon = wincheck(g, f);
            break;

        case SDL_KEYDOWN:
            switch (event->key.keysym.scancode) {
                case SDL_SCANCODE_Q:
                case SDL_SCANCODE_ESCAPE:
                    return 1;

                case SDL_SCANCODE_SPACE:
                    *ctr = 0;
                    resetgrid(g, f);
                    initgrid(g, f);
                    break;

                default:
                    break;
            }
        }
    }
    return 0;
}

// draws all objects on screen
void draw(SDL_Renderer *rend, char **g, SDL_Texture *txt, SDL_Rect *r, Flag *f) {

    int txx = 10, txy = f->y * BLKSZ;
    int txw, txh;

    SDL_RenderClear(rend);

    // draw background
    SDL_SetRenderDrawColor(rend, 0, 0, 0, 255);
    SDL_RenderFillRect(rend, NULL);

    // draw grid
    for(int y = 0; y < f->y; y++) {
        for(int x = 0; x < f->x; x++) {
            r->x = x * BLKSZ;
            r->y = y * BLKSZ;
            r->w = BLKSZ - 2;
            r->h = BLKSZ - 2;
            if(g[y][x] == 1) SDL_SetRenderDrawColor(rend, 0, 50, 150, 0);
            else SDL_SetRenderDrawColor(rend, 80, 20, 20, 0);
            SDL_RenderFillRect(rend, r);
        }
    }

    // draw text
    SDL_QueryTexture(txt, NULL, NULL, &txw, &txh);
    SDL_Rect dstrect = {txx, txy, txw, txh};

    SDL_RenderCopy(rend, txt, NULL, &dstrect);
    SDL_RenderPresent(rend);
}

// update score
void updatesc(uint16_t *ctr, char *haswon, SDL_Renderer *rend, SDL_Surface **tsurf, SDL_Texture **txt,
        SDL_Color *tcol, TTF_Font *font) {

    char str[30];

    snprintf(str, 30, *haswon ? "You won! %d clicks" : "Clicks: %d", *ctr);

    *tsurf = TTF_RenderText_Solid(font, str, *tcol);
    *txt = SDL_CreateTextureFromSurface(rend, *tsurf);
}

// read command line options and set flags
void readopt(int argc, char **argv, Flag *f) {

    int optc;

    while((optc = getopt(argc, argv, "s:x:y:")) != -1) {
        switch (optc) {

            case 's':
                f->s = matoi(optarg);
                break;

            case 'x':
                f->x = matoi(optarg);
                break;

            case 'y':
                f->y = matoi(optarg);
                break;

            default:
                break;
        }
    }
}

// sets flags to default values
Flag *initflag() {

    Flag *f = calloc(1, sizeof(Flag));

    f->s = INITSC;
    f->x = GRIDX;
    f->y = GRIDY;

    return f;
}

int main(int argc, char **argv) {

    time_t t;
    srand((unsigned) time(&t));

    char haswon = 0;
    uint16_t ctr = 0;

    Flag *f = initflag();
    readopt(argc, argv, f);

    char **g = mkgrid(f);

    SDL_Rect *sr = calloc(1, sizeof(SDL_Rect));

    SDL_Init(SDL_INIT_VIDEO);
    TTF_Init();

    SDL_Event *event = calloc(1, sizeof(SDL_Event));
    TTF_Font *font = TTF_OpenFont(FONT, 25);

    SDL_Window *win = SDL_CreateWindow(argv[0],
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        f->x * BLKSZ, f->y * BLKSZ + TXFH, 0);

    SDL_Renderer *rend = SDL_CreateRenderer(win, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    SDL_SetRelativeMouseMode(SDL_FALSE);

    SDL_Color tcol = {TCOL_R, TCOL_G, TCOL_B};
    SDL_Surface *tsurf = TTF_RenderText_Solid(font, "lamppaa!", tcol);
    SDL_Texture *txt = SDL_CreateTextureFromSurface(rend, tsurf);

    initgrid(g, f);

    if(!sr || !event || !font || !win || !rend || !tsurf || !txt || !g)
        return 64;

    while(!readevent(event, g, &ctr, &haswon, f)) {
        updatesc(&ctr, &haswon, rend, &tsurf, &txt, &tcol, font);
        draw(rend, g, txt, sr, f);
    }

    // cleanup
    SDL_DestroyTexture(txt);
    SDL_FreeSurface(tsurf);
    TTF_CloseFont(font);

    free(sr);
    freegrid(g, f);
    free(f);
    TTF_Quit();
    SDL_Quit();

    return 0;
}
