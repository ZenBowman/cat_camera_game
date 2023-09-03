#include <iostream>
#include <SDL.h>
#if defined(WINAPI_FAMILY) && (WINAPI_FAMILY == WINAPI_FAMILY_APP)
//On UWP, we need to not have SDL_main otherwise we'll get a linker error
#define SDL_MAIN_HANDLED
#endif
#include <SDL_main.h>
#include <opencv2/core.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/highgui.hpp>

void SDL_Fail(){
    SDL_LogError(SDL_LOG_CATEGORY_CUSTOM, "Error %s", SDL_GetError());
    exit(1);
}

static bool app_quit = false;
const char* image_path = "Snowy.bmp";

using ::cv::Mat;
using ::cv::VideoCapture;

// Reads the current camera frame, loads it into mutable_frame,
// and displays the camera frame in a separate window.
void read_frame(VideoCapture &camera, Mat &mutable_frame) {
  camera.read(mutable_frame);
  imshow("Live", mutable_frame);
}

void main_loop(SDL_Window *win) {
  Mat frame;
  // Initialize camera
  VideoCapture cap;
  cap.open(0);
  if (!cap.isOpened()) {
    SDL_Log("ERROR! Unable to open camera\n");
    return;
  }

  // Setup basic renderer.
  Uint32 render_flags = SDL_RENDERER_ACCELERATED;
  SDL_Renderer* rend = SDL_CreateRenderer(win, NULL, render_flags);
  
  SDL_RWops *file = SDL_RWFromFile(image_path, "rb");
  SDL_Surface *sprite = SDL_LoadBMP_RW(file, SDL_TRUE);
  if (!sprite) {
    SDL_Log("Failed to load image: %s", SDL_GetError());
  }
  SDL_Texture *tex = SDL_CreateTextureFromSurface(rend, sprite);
  
  SDL_FRect rect;
  rect.x = 500;
  rect.y = 500;
  rect.w = 100;
  rect.h = 100;
  
  SDL_Event event;
  while (app_quit == false) {
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_EVENT_QUIT) {
	app_quit = true;
	break;
      } else if (event.type == SDL_EVENT_KEY_DOWN) {
	
      }
    }
    read_frame(cap, frame);
    SDL_RenderClear(rend);
    int res = SDL_RenderTexture(rend, tex, NULL, &rect);
    if (res !=0) {
      SDL_Log("error with rendering: %s", SDL_GetError());
    }
    SDL_RenderPresent(rend);
  }
  
  SDL_DestroySurface(sprite);
  
  SDL_DestroyTexture(tex);
  SDL_DestroyRenderer(rend);
}


int main(int argc, char* argv[]){

  // init the library, here we make a window so we only need the Video capabilities.
  if (SDL_Init(SDL_INIT_VIDEO)){
    SDL_Fail();
  }

  // create a window
  SDL_Window* window = SDL_CreateWindow("Window", 1200, 1200, SDL_WINDOW_RESIZABLE);
  if (!window){
    SDL_Fail();
  }

  // print some information about the window
  SDL_ShowWindow(window);
  {
    int width, height, bbwidth, bbheight;
    SDL_GetWindowSize(window, &width, &height);
    SDL_GetWindowSizeInPixels(window, &bbwidth, &bbheight);
    SDL_Log("Window size: %ix%i", width, height);
    SDL_Log("Backbuffer size: %ix%i", bbwidth, bbheight);
    if (width != bbwidth){
      SDL_Log("This is a highdpi environment.");
    }
  }

  SDL_Log("Application started successfully!");

  while (!app_quit) {
    main_loop(window);
  }

  // cleanup everything at the end
  SDL_DestroyWindow(window);
  SDL_Quit();
  SDL_Log("Application quit successfully!");
  return 0;
}
