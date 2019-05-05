#include <iostream>
extern "C" {
#include <SDL2/SDL.h>
}
using namespace  std;

const int WIDTH = 400, HEIGHT = 400;
int main() {
    if (SDL_Init(SDL_INIT_EVERYTHING) < 0) {
        cout << "SDL could not initialized with error: " << SDL_GetError() << endl;
    }

    SDL_Window *window = SDL_CreateWindow("Hello SDL world!", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                          WIDTH, HEIGHT, SDL_WINDOW_ALLOW_HIGHDPI);
    if (NULL == window) {
        cout << "SDL could not create window with error: " << SDL_GetError() << endl;
    }

    SDL_Event windowEvent;
    while(true) {
        if (SDL_PollEvent(&windowEvent)) {
            if (SDL_QUIT == windowEvent.type) {
                cout << "SDL quit!!" << endl;
                break;
            }
        }
    }
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}