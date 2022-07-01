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

RepaUI::Element* elementToControl = nullptr;

int controlIndex = 0;

std::vector<RepaUI::Element*> elements;

void CreateGUI()
{
  auto slices   = LoadImage("slice-test.bmp");
  auto checkers = LoadImage("checkers.bmp");
  auto slicesNu = LoadImage("slice-nu.bmp");

  auto canvas1 = RepaUI::CreateCanvas({ 0, 0, 300, 300 });
  auto img1 = RepaUI::CreateImage(canvas1, { 0, 0, 100, 100 }, slices);
  img1->OnMouseOver = HoverTest;
  img1->OnMouseOut  = OutTest;

  auto img2 = RepaUI::CreateImage(canvas1, { 0, 110, 100, 100 }, checkers);
  img2->SetDrawType(RepaUI::Image::DrawType::TILED);
  img2->OnMouseOver = HoverTest;
  img2->OnMouseOut  = OutTest;

  auto img3 = RepaUI::CreateImage(canvas1, { 110, 0, 100, 100 }, slices);
  img3->SetDrawType(RepaUI::Image::DrawType::SLICED);
  img3->SetSlicePoints({ 6, 6, 25, 25 });
  img3->OnMouseOver = HoverTest;
  img3->OnMouseOut  = OutTest;

  auto img4 = RepaUI::CreateImage(canvas1, { 110, 110, 100, 100 }, slicesNu);
  img4->SetDrawType(RepaUI::Image::DrawType::SLICED);
  img4->SetSlicePoints({ 5, 5, 19, 20 });
  img4->OnMouseOver = HoverTest;
  img4->OnMouseOut  = OutTest;

  elements.push_back(canvas1);
  elements.push_back(img1);
  elements.push_back(img2);
  elements.push_back(img3);
  elements.push_back(img4);

  elementToControl = elements.back();
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

  RepaUI::Init(_window);

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
            bool v = elementToControl->IsVisible();
            elementToControl->SetVisible(!v);
          }

          if (evt.key.keysym.sym == SDLK_KP_PLUS)
          {
            RepaUI::Image* i = static_cast<RepaUI::Image*>(elementToControl);

            auto tr = i->GetTileRate();

            tr.first++;
            tr.second++;

            i->SetTileRate(tr);
          }

          if (evt.key.keysym.sym == SDLK_KP_MINUS)
          {
            RepaUI::Image* i = static_cast<RepaUI::Image*>(elementToControl);

            auto tr = i->GetTileRate();

            tr.first--;
            tr.second--;

            i->SetTileRate(tr);
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

          if (evt.key.keysym.sym == SDLK_o)
          {
            static bool val = true;
            elementToControl->ShowOutline(val);
            val = !val;
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
