#include <iostream>
extern "C" {
#include <SDL2/SDL.h>
#include <SDL2/SDL_test_images.h>
}
using namespace  std;

const int WIDTH = 960, HEIGHT = 540;
int main() {

    SDL_Surface *imageSurface = NULL;
    SDL_Surface *windowSurface = NULL;

    if (SDL_Init(SDL_INIT_EVERYTHING) < 0) {
        cout << "SDL could not initialized with error: " << SDL_GetError() << endl;
    }
    SDL_Window *window = SDL_CreateWindow("Hello SDL world!", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                          WIDTH, HEIGHT, SDL_WINDOW_ALLOW_HIGHDPI);
    if (NULL == window) {
        cout << "SDL could not create window with error: " << SDL_GetError() << endl;
    }

    windowSurface = SDL_GetWindowSurface(window);
    imageSurface = SDL_LoadBMP("little_prince.bmp");
    if (NULL == imageSurface) {
        cout << "SDL could not load image with error: " << SDL_GetError() << endl;
    }
    SDL_Event windowEvent;
    while(true) {
        if (SDL_PollEvent(&windowEvent)) {
            if (SDL_QUIT == windowEvent.type) {
                cout << "SDL quit!!" << endl;
                break;
            }
        }

        SDL_BlitSurface(imageSurface, NULL, windowSurface, NULL);
        SDL_UpdateWindowSurface(window);
    }

    imageSurface = NULL;
    windowSurface = NULL;
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}