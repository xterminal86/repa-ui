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

std::vector<RepaUI::Element*> elements;

int controlIndex = 0;

RepaUI::Element* elementToControl = nullptr;

const std::string TestString =
1+ R"(
This is line one
This is line two
This is line three
)";

void CreateGUI()
{
  auto gridImg     = LoadImage("grid.bmp");
  auto sliceImg    = LoadImage("slice-test-big.bmp");
  //auto sliceImg    = LoadImage("slice-test.bmp");
  auto wndImg      = LoadImage("window.bmp");
  auto checkersImg = LoadImage("checkers.bmp");
  auto btnImg      = LoadImage("button.bmp");

  auto canvas = RepaUI::CreateCanvas({ 0, 0, 500, 500 });

  auto canvasBg = RepaUI::CreateImage(canvas, { 0, 0, 500, 500 }, nullptr);
  canvasBg->SetColor({ 32, 32, 32, 255 });

  auto img1 = RepaUI::CreateImage(canvas, { 0, 0, 100, 100 }, sliceImg);
  img1->OnMouseOver = HoverTest;
  img1->OnMouseOut  = OutTest;
  img1->SetDrawType(RepaUI::Image::DrawType::NORMAL);

  auto img2 = RepaUI::CreateImage(canvas, { 150, 0, 100, 100 }, checkersImg);
  img2->OnMouseOver = HoverTest;
  img2->OnMouseOut  = OutTest;
  img2->SetDrawType(RepaUI::Image::DrawType::TILED);

  auto img3 = RepaUI::CreateImage(canvas, { 0, 300, 300, 300 }, sliceImg);
  img3->OnMouseOver = HoverTest;
  img3->OnMouseOut  = OutTest;
  //img3->SetSlicePoints({ 7, 7, 24, 24 });
  img3->SetSlicePoints({ 70, 70, 249, 249 });
  img3->SetDrawType(RepaUI::Image::DrawType::SLICED);

  auto canvas3 = RepaUI::CreateCanvas({ 400, 100, 500, 500 });
  canvasBg = RepaUI::CreateImage(canvas3, { 0, 0, 500, 500 }, nullptr);
  canvasBg->SetColor({ 32, 0, 0, 255 });

  auto img4 = RepaUI::CreateImage(canvas3, { 50, 50, 100, 100 }, btnImg);
  img4->OnMouseOver = HoverTest;
  img4->OnMouseOut  = OutTest;
  img4->SetSlicePoints({ 3, 3, 12, 12 });
  img4->SetDrawType(RepaUI::Image::DrawType::SLICED);

  auto canvas2 = RepaUI::CreateCanvas({ 100, 100, 500, 500 });
  canvasBg = RepaUI::CreateImage(canvas2, { 0, 0, 500, 500 }, nullptr);
  canvasBg->SetColor({ 0, 32, 0, 255 });

  auto img5 = RepaUI::CreateImage(canvas2, { 0, 0, 100, 100 }, sliceImg);
  img5->OnMouseOver = HoverTest;
  img5->OnMouseOut  = OutTest;

  auto img6 = RepaUI::CreateImage(nullptr, { 550, 400, 50, 50 }, nullptr);
  img6->OnMouseOver = HoverTest;
  img6->OnMouseOut  = OutTest;
  img6->SetColor({ 64, 64, 64, 255 });

  auto txt = RepaUI::CreateText(canvas2, { 100, 100, 200, 50 }, TestString);

  elements.push_back(canvas);
  elements.push_back(canvas2);
  elements.push_back(canvas3);
  elements.push_back(img1);
  elements.push_back(img2);
  elements.push_back(img3);
  elements.push_back(img4);
  elements.push_back(img5);
  elements.push_back(img6);
  elements.push_back(txt);

  elementToControl = elements[controlIndex];
  elementToControl->ShowOutline(true);
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

  RepaUI::Init(_window);

  CreateGUI();

  SDL_SetRenderDrawColor(_renderer, 64, 0, 64, 255);

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

          if (evt.key.keysym.sym == SDLK_a)
          {
            auto t = elementToControl->Transform();
            t.w -= 10;
            elementToControl->SetTransform(t);
          }

          if (evt.key.keysym.sym == SDLK_d)
          {
            auto t = elementToControl->Transform();
            t.w += 10;
            elementToControl->SetTransform(t);
          }

          if (evt.key.keysym.sym == SDLK_w)
          {
            auto t = elementToControl->Transform();
            t.h -= 10;
            elementToControl->SetTransform(t);
          }

          if (evt.key.keysym.sym == SDLK_s)
          {
            auto t = elementToControl->Transform();
            t.h += 10;
            elementToControl->SetTransform(t);
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
            RepaUI::Image* i = dynamic_cast<RepaUI::Image*>(elementToControl);
            if (i != nullptr)
            {
              auto tr = i->GetTileRate();
              tr.first++;
              tr.second++;
              i->SetTileRate(tr);
            }
          }

          if (evt.key.keysym.sym == SDLK_KP_MINUS)
          {
            RepaUI::Image* i = dynamic_cast<RepaUI::Image*>(elementToControl);
            if (i != nullptr)
            {
              auto tr = i->GetTileRate();
              tr.first--;
              tr.second--;
              i->SetTileRate(tr);
            }
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
