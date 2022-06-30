#include <cstdio>

#include "SDL2/SDL.h"

#include "repa-ui.h"

SDL_Renderer* _renderer = nullptr;
SDL_Window* _window = nullptr;

const int kWindowWidth  = 1024;
const int kWindowHeight = 1024;

std::string message;

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
  message = "---->>>> element #" + std::to_string(sender->Id());
  SDL_Log(message.data());
}

void OutTest(RepaUI::Element* sender)
{
  message = "<<<<---- element #" + std::to_string(sender->Id());
  SDL_Log(message.data());
}

void MoveTest(RepaUI::Element* sender)
{
  message = "element #" + std::to_string(sender->Id());
  SDL_Log(message.data());
}

void DownTest(RepaUI::Element* sender)
{
  message = "\\/ element #" + std::to_string(sender->Id());
  SDL_Log(message.data());
}

void UpTest(RepaUI::Element* sender)
{
  message = "/\\ element #" + std::to_string(sender->Id());
  SDL_Log(message.data());
}

RepaUI::Canvas* canvas2 = nullptr;
RepaUI::Image* image = nullptr;
RepaUI::Image* img2 = nullptr;
RepaUI::Image* img4 = nullptr;

RepaUI::Element* elementToControl = nullptr;

int controlIndex = 3;

std::vector<RepaUI::Element*> elements;

void CreateGUI()
{
  auto grid = LoadImage("grid.bmp");
  auto screenCanvas = RepaUI::CreateCanvas({ 0, 0, kWindowWidth, kWindowHeight });
  //screenCanvas->ShowOutline(true);
  //screenCanvas->OnMouseOver = HoverTest;
  //screenCanvas->OnMouseOut  = OutTest;
  //canvas->OnMouseMove = MoveTest;

  auto imgTex = LoadImage("slice-test.bmp");
  //auto imgTex = LoadImage("slice-non-uniform.bmp");
  image = RepaUI::CreateImage(screenCanvas, { 10, 10, 32, 32 }, imgTex);
  image->SetSlicePoints({ 6, 6, 25, 25 });
  //image->SetSlicePoints({ 5, 5, 19, 20 });
  image->SetDrawType(RepaUI::Image::DrawType::SLICED);
  //image->ShowOutline(true);
  image->OnMouseOver = HoverTest;
  image->OnMouseOut  = OutTest;
  //image->OnMouseMove = MoveTest;

  auto imgWnd = LoadImage("window.bmp");
  auto imageWnd = RepaUI::CreateImage(screenCanvas, { 0, 250, 200, 200 }, imgWnd);
  imageWnd->SetSlicePoints({ 3, 3, 12, 12 });
  imageWnd->SetDrawType(RepaUI::Image::DrawType::SLICED);

  auto imgTex2 = LoadImage("checkers.bmp");
  auto img3 = RepaUI::CreateImage(screenCanvas, { 0, 50, 100, 100 }, imgTex2);
  img3->SetVisible(true);
  img3->OnMouseOver = HoverTest;
  img3->OnMouseOut  = OutTest;

  canvas2 = RepaUI::CreateCanvas({ 225, 0, 350, 350 });
  canvas2->OnMouseOver = HoverTest;
  canvas2->OnMouseOut  = OutTest;
  //canvas2->OnMouseMove = MoveTest;
  canvas2->ShowOutline(true);
  //canvas2->SetVisible(false);

  img2 = RepaUI::CreateImage(canvas2, { -50, 50, 100, 100 }, imgTex2);
  img2->ShowOutline(true);
  img2->OnMouseOver = HoverTest;
  img2->OnMouseOut  = OutTest;
  //img2->OnMouseMove = MoveTest;

  img4 = RepaUI::CreateImage(canvas2, { 180, 60, 100, 100 }, imgTex2);
  img4->ShowOutline(true);
  img4->SetDrawType(RepaUI::Image::DrawType::TILED);
  img4->SetColor({ 255, 255, 255, 128 });
  //img4->SetTileRate({ 3, 3 });
  img4->OnMouseOver = HoverTest;
  img4->OnMouseOut  = OutTest;
  img4->OnMouseDown = DownTest;
  img4->OnMouseUp   = UpTest;
  //img4->OnMouseMove = MoveTest;

  elements.push_back(image);
  elements.push_back(canvas2);
  elements.push_back(img3);
  elements.push_back(img2);
  elements.push_back(img4);

  elementToControl = elements[controlIndex];
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

  //SDL_RenderSetScale(_renderer, 4, 4);

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
            SDL_Rect t = elementToControl->Transform();
            t.x += 10;
            elementToControl->SetTransform(t);
          }

          if (evt.key.keysym.sym == SDLK_LEFT)
          {
            SDL_Rect t = elementToControl->Transform();
            t.x -= 10;
            elementToControl->SetTransform(t);
          }

          if (evt.key.keysym.sym == SDLK_DOWN)
          {
            SDL_Rect t = elementToControl->Transform();
            t.y += 10;
            elementToControl->SetTransform(t);
          }

          if (evt.key.keysym.sym == SDLK_UP)
          {
            SDL_Rect t = elementToControl->Transform();
            t.y -= 10;
            elementToControl->SetTransform(t);
          }

          if (evt.key.keysym.sym == SDLK_SPACE)
          {
            bool v = canvas2->IsVisible();
            canvas2->SetVisible(!v);
            //bool v = img2->IsVisible();
            //img2->SetVisible(!v);
          }

          if (evt.key.keysym.sym == SDLK_KP_PLUS)
          {
            static int rx = 1;
            static int ry = 1;

            rx++;
            ry++;

            img4->SetTileRate({ rx, ry });
          }

          if (evt.key.keysym.sym == SDLK_TAB)
          {
            elementToControl->ShowOutline(false);
            controlIndex++;
            controlIndex %= elements.size();
            elementToControl = elements[controlIndex];
            elementToControl->ShowOutline(true);
            SDL_Log("Active elements #%lu", elementToControl->Id());
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
