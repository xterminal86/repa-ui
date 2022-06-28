#include <cstdio>

#include "SDL2/SDL.h"

#include "repa-ui.h"

SDL_Renderer* _renderer = nullptr;
SDL_Window* _window = nullptr;

const int kWindowWidth  = 640;
const int kWindowHeight = 480;

SDL_Texture* LoadImage(const std::string& fname)
{
  SDL_Texture* res = nullptr;
  SDL_Surface* s = SDL_LoadBMP(fname.data());
  res = SDL_CreateTextureFromSurface(_renderer, s);
  SDL_FreeSurface(s);
  return res;
}

void Draw()
{
  SDL_RenderClear(_renderer);

  RepaUI::Draw();

  SDL_RenderPresent(_renderer);
}

void HoverTest(RepaUI::Element* sender)
{
  std::string str = "---->>>> element #" + std::to_string(sender->Id());
  SDL_Log(str.data());
}

void OutTest(RepaUI::Element* sender)
{
  std::string str = "<<<<---- element #" + std::to_string(sender->Id());
  SDL_Log(str.data());
}

void MoveTest(RepaUI::Element* sender)
{
  std::string str = "element #" + std::to_string(sender->Id());
  SDL_Log(str.data());
}

RepaUI::Canvas* canvas2 = nullptr;
RepaUI::Image* img2 = nullptr;

void CreateGUI()
{
  auto grid = LoadImage("grid.bmp");
  auto canvas = RepaUI::CreateCanvas({ 0, 0, 300, 300 });
  canvas->ShowOutline(true);
  canvas->OnMouseOver = HoverTest;
  canvas->OnMouseOut  = OutTest;
  //canvas->OnMouseMove = MoveTest;

  auto img = LoadImage("checkers.bmp");
  auto image = RepaUI::CreateImage(canvas, { 10, 10, 100, 100 }, img);
  image->OnMouseOver = HoverTest;
  image->OnMouseOut  = OutTest;
  //image->OnMouseMove = MoveTest;

  auto img3 = RepaUI::CreateImage(canvas, { 50, 50, 100, 100 }, img);
  img3->OnMouseOver = HoverTest;
  img3->OnMouseOut  = OutTest;

  canvas2 = RepaUI::CreateCanvas({ 225, 0, 100, 100 });
  canvas2->OnMouseOver = HoverTest;
  canvas2->OnMouseOut  = OutTest;
  //canvas2->OnMouseMove = MoveTest;
  canvas2->ShowOutline(true);

  img2 = RepaUI::CreateImage(canvas2, { 50, 50, 100, 100 }, img);
  img2->ShowOutline(true);
  img2->OnMouseOver = HoverTest;
  img2->OnMouseOut  = OutTest;
  //img2->OnMouseMove = MoveTest;

  auto img4 = RepaUI::CreateImage(canvas2, { 60, 60, 100, 100 }, img);
  img4->ShowOutline(true);
  img4->OnMouseOver = HoverTest;
  img4->OnMouseOut  = OutTest;
  //img4->OnMouseMove = MoveTest;
}

int main(int argc, char* argv[])
{
  if (SDL_Init(SDL_INIT_VIDEO) != 0)
  {
    printf("SDL_Init Error: %s\n", SDL_GetError());
    return 1;
  }

  _window = SDL_CreateWindow("SDL2 GUI example",
                              100, 100,
                              kWindowWidth, kWindowHeight,
                              SDL_WINDOW_SHOWN);

  int drivers = SDL_GetNumRenderDrivers();

  for (int i = 0; i < drivers; i++)
  {
    SDL_RendererInfo info;
    SDL_GetRenderDriverInfo(i, &info);

    if (info.flags & SDL_RENDERER_ACCELERATED)
    {
      _renderer = SDL_CreateRenderer(_window, i, info.flags);
      if (_renderer != nullptr)
      {
        break;
      }
    }
  }

  if (_renderer == nullptr)
  {
    printf("Couldn't create renderer! %s\n", SDL_GetError());
    return 1;
  }

  RepaUI::Init(_renderer, _window);

  CreateGUI();

  SDL_SetRenderDrawColor(_renderer, 0, 0, 0, 255);

  SDL_Event evt;

  bool running = true;
  while (running)
  {
    while (SDL_PollEvent(&evt))
    {
      RepaUI::HandleEvents(evt);

      switch (evt.type)
      {
        case SDL_KEYDOWN:
        {
          if (evt.key.keysym.sym == SDLK_ESCAPE)
          {
            running = false;
          }

          if (evt.key.keysym.sym == SDLK_RIGHT)
          {
            SDL_Rect t = canvas2->Transform();
            t.x++;
            canvas2->SetTransform(t);
          }

          if (evt.key.keysym.sym == SDLK_LEFT)
          {
            SDL_Rect t = canvas2->Transform();
            t.x--;
            canvas2->SetTransform(t);
          }

          if (evt.key.keysym.sym == SDLK_DOWN)
          {
            SDL_Rect t = canvas2->Transform();
            t.y++;
            canvas2->SetTransform(t);
          }

          if (evt.key.keysym.sym == SDLK_UP)
          {
            SDL_Rect t = canvas2->Transform();
            t.y--;
            canvas2->SetTransform(t);
          }

          if (evt.key.keysym.sym == SDLK_SPACE)
          {
            bool v = canvas2->IsVisible();
            canvas2->SetVisible(!v);
            //bool v = img2->IsVisible();
            //img2->SetVisible(!v);
          }
        }
        break;
      }
    }

    Draw();
  }

  SDL_Quit();

  return 0;
}
