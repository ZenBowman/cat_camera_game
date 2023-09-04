#include <SDL.h>
#include <algorithm>
#include <iostream>
#if defined(WINAPI_FAMILY) && (WINAPI_FAMILY == WINAPI_FAMILY_APP)
// On UWP, we need to not have SDL_main otherwise we'll get a linker error
#define SDL_MAIN_HANDLED
#endif
#include <SDL_main.h>
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/videoio.hpp>

static bool app_quit = false;
const char *image_path = "gingertail_runwalk.bmp";

using ::cv::CHAIN_APPROX_SIMPLE;
using ::cv::Mat;
using ::cv::Point;
using ::cv::RETR_CCOMP;
using ::cv::Scalar;
using ::cv::Vec4i;
using ::cv::VideoCapture;

struct Pixel {
  unsigned char red;
  unsigned char green;
  unsigned char blue;
  unsigned char hue;
  unsigned char saturation;
  unsigned char value;
};

enum Action { NONE, MOVE_LEFT, MOVE_RIGHT };
void SDL_Fail() {
  SDL_LogError(SDL_LOG_CATEGORY_CUSTOM, "Error %s", SDL_GetError());
  exit(1);
}

Mat drawMaxCountour(Mat &src, int &maxAreaOut, Point &maxCenterOfMassOut,
                    int minArea) {
  Mat dst = Mat::zeros(src.rows, src.cols, CV_8UC3);
  std::vector<std::vector<Point>> contours;
  std::vector<Vec4i> hierarchy;

  findContours(src, contours, hierarchy, RETR_CCOMP, CHAIN_APPROX_SIMPLE);

  maxAreaOut = 0;
  int maxAreaIndex = 0;
  int currentArea;
  std::vector<int> indicesToDraw;
  std::vector<cv::Point> centerOfMasses;

  for (int i = 0; i < contours.size(); i++) {
    auto contour = contours[i];
    currentArea = contourArea(contour);

    if (currentArea > minArea) {
      indicesToDraw.push_back(i);
      cv::Point point;
      auto m = moments(contour);
      point.x = m.m10 / m.m00;
      point.y = m.m01 / m.m00;
      centerOfMasses.push_back(point);
    }

    if (currentArea > maxAreaOut) {
      // SDL_Log("Area found: %i", currentArea);
      maxAreaOut = currentArea;
      maxAreaIndex = i;
    }
  }

  if (centerOfMasses.size() == 0) {
    maxCenterOfMassOut.x = 0;
    maxCenterOfMassOut.y = 0;
  } else {
    maxCenterOfMassOut.x = 0;
    maxCenterOfMassOut.y = 0;
  }
  for (auto point : centerOfMasses) {
    maxCenterOfMassOut.x += point.x / centerOfMasses.size();
    maxCenterOfMassOut.y += point.y / centerOfMasses.size();
  }

  Scalar color = Scalar(255, 0, 0);
  for (auto i : indicesToDraw) {
    drawContours(dst, contours, i, color, 2, 8, hierarchy, 0, Point());
  }

  return dst;
}

Mat applyPixelFilter(Mat &source) {
  Mat hsvSource;
  cv::cvtColor(source, hsvSource, cv::COLOR_BGR2HSV);
  Mat newMatrix = Mat::zeros(source.rows, source.cols, CV_8UC1);
  const int rows = source.rows;
  const int cols = source.cols;

  const unsigned char *srcRowPointer;
  const unsigned char *hsvSrcRowPointer;
  unsigned char *destRowPointer;

  // Loop variables
  Pixel pixel;
  int srcColIndex;

  for (int i = 0; i < rows; i++) {
    srcRowPointer = source.ptr<uchar>(i);
    hsvSrcRowPointer = hsvSource.ptr<uchar>(i);
    destRowPointer = newMatrix.ptr<uchar>(i);
    for (int j = 0; j < cols; j++) {
      srcColIndex = j * 3;
      pixel.blue = srcRowPointer[srcColIndex];
      pixel.green = srcRowPointer[srcColIndex + 1];
      pixel.red = srcRowPointer[srcColIndex + 2];
      pixel.hue = hsvSrcRowPointer[srcColIndex];
      pixel.saturation = hsvSrcRowPointer[srcColIndex + 1];
      pixel.value = hsvSrcRowPointer[srcColIndex + 2];

      if ((pixel.green > 100) && (pixel.green > (pixel.red * 1.15)) &&
          (pixel.green > (pixel.blue * 1.15))) {
        destRowPointer[j] = 255;
      }
    }
  }

  return newMatrix;
}

// Reads the current camera frame, loads it into mutable_frame,
// and displays the camera frame in a separate window.
Action read_frame(VideoCapture &camera, Mat &mutable_frame) {
  camera.read(mutable_frame);
  Mat green_filter = applyPixelFilter(mutable_frame);
  int maxArea;
  Point maxCenterOfMass;
  Mat contours = drawMaxCountour(green_filter, maxArea, maxCenterOfMass, 10000);

  SDL_Log("Center of mass x = %i", maxCenterOfMass.x);

  imshow("Live", contours);
  if (maxCenterOfMass.x == 0) {
    return NONE;
  }
  if (maxCenterOfMass.x < 1000) {
    return MOVE_RIGHT;
  } else if (maxCenterOfMass.x > 1200) {
    return MOVE_LEFT;
  } else {
    return NONE;
  }
}

int clamp(int value, int low, int high) {
  if (value < low) {
    return low;
  } else if (value > high) {
    return high;
  }
  return value;
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
  SDL_Renderer *rend = SDL_CreateRenderer(win, NULL, render_flags);

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

    Action action = read_frame(cap, frame);
    if (action == MOVE_LEFT) {
      rect.x = clamp(rect.x - 10, 10, 800);
    } else if (action == MOVE_RIGHT) {
      rect.x = clamp(rect.x + 10, 10, 800);
    }
    SDL_RenderClear(rend);
    int res = SDL_RenderTexture(rend, tex, NULL, &rect);
    if (res != 0) {
      SDL_Log("error with rendering: %s", SDL_GetError());
    }
    SDL_RenderPresent(rend);
  }

  SDL_DestroySurface(sprite);

  SDL_DestroyTexture(tex);
  SDL_DestroyRenderer(rend);
}

int main(int argc, char *argv[]) {

  // init the library, here we make a window so we only need the Video
  // capabilities.
  if (SDL_Init(SDL_INIT_VIDEO)) {
    SDL_Fail();
  }

  // create a window
  SDL_Window *window =
      SDL_CreateWindow("Window", 1200, 1200, SDL_WINDOW_RESIZABLE);
  if (!window) {
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
    if (width != bbwidth) {
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
