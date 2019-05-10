#include <iostream>

#include <SDL.h>
#include <SDL2/SDL_image.h>

using namespace  std;

const int WIDTH = 960, HEIGHT = 540;
int main() {

    if (SDL_Init(SDL_INIT_EVERYTHING) < 0) {
        cout << "SDL could not initialized with error: " << SDL_GetError() << endl;
        return -1;
    }
    SDL_Window *window = SDL_CreateWindow("Hello SDL world!", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                                          WIDTH, HEIGHT, SDL_WINDOW_ALLOW_HIGHDPI);
    if (NULL == window) {
        cout << "SDL could not create window with error: " << SDL_GetError() << endl;
        return -1;
    }

    if (!(IMG_Init(IMG_INIT_JPG) & IMG_INIT_JPG)) {
        cout << "SDL_image could not init with error: " << IMG_GetError() << endl;
        return -1;
    }

    FILE* pFile = fopen("little_prince_yv12_960x540.yuv", "rb");
    if (pFile == NULL) {
        cerr << "little_prince_yv21_960x540.yuv open failed" << endl;
    }
    unsigned char *m_yuv_data;
    int frameSize = HEIGHT * WIDTH * 12 / 8;
    m_yuv_data = (unsigned char*)malloc(frameSize * sizeof(unsigned char));
    fread(m_yuv_data, 1, frameSize, pFile);
    fclose(pFile);

    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, 0);
    SDL_Texture *texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING, WIDTH, HEIGHT);
    SDL_Rect    rect;
    rect.x = 0;
    rect.y = 0;
    rect.w = WIDTH;
    rect.h = HEIGHT;
    if (texture != NULL) {
        SDL_Event windowEvent;
        while (true) {
            if (SDL_PollEvent(&windowEvent)) {
                if (SDL_QUIT == windowEvent.type) {
                    cout << "SDL quit!!" << endl;
                    break;
                }
            }
            SDL_UpdateTexture(texture, NULL, m_yuv_data, WIDTH);
            SDL_RenderClear(renderer);
            SDL_RenderCopy(renderer, texture, NULL, NULL);
            SDL_RenderPresent(renderer);
        }

        SDL_DestroyWindow(window);
        SDL_Quit();
        return 0;
    }
}