#ifndef S2G_H
#define S2G_H

#include "SDL2/SDL.h"

#include <string>
#include <sstream>
#include <vector>
#include <memory>
#include <map>
#include <stack>
#include <functional>

namespace RepaUI
{
  // ===========================================================================
  //                              UTILS
  // ===========================================================================
  template <typename T>
  T Clamp(T value, T min, T max)
  {
    return std::max(min, std::min(value, max));
  }

  // ===========================================================================
  class Element;
  class Canvas;
  class Image;
  // ===========================================================================

  class Manager
  {
    public:
      static Manager& Get()
      {
        static Manager instance;
        return instance;
      }

      void Init(SDL_Renderer* rendRef, SDL_Window* windowRef)
      {
        if (_initialized)
        {
          return;
        }

        _rendRef   = rendRef;
        _windowRef = windowRef;

        SDL_GetWindowSize(_windowRef, &_windowWidth, &_windowHeight);

        CreateScreenCanvas();
        PrepareImages();

        _initialized = true;
      }

      void HandleEvents(const SDL_Event& evt);
      void Draw();

      // =======================================================================
      Canvas* CreateCanvas(const SDL_Rect& transform);
      Image* CreateImage(Canvas* canvas,
                          const SDL_Rect& transform,
                          SDL_Texture* image);
      // =======================================================================

    private:
      const uint64_t& GetNewId()
      {
        _globalId++;
        return _globalId;
      }

      void PrepareImages()
      {
        // FIXME: replace with embedded
        //_font = LoadImage("font.bmp", 0xFF, 0x00, 0xFF);

        //_btnNormal   = LoadImage("btn-normal.bmp");
        //_btnPressed  = LoadImage("btn-pressed.bmp");
        //_btnHover    = LoadImage("btn-hover.bmp");
        //_btnDisabled = _btnPressed; //LoadImage("btn-pressed.bmp");
      }

      SDL_Texture* LoadImage(const std::string& fname)
      {
        SDL_Texture* res = nullptr;
        SDL_Surface* s = SDL_LoadBMP(fname.data());
        res = SDL_CreateTextureFromSurface(_rendRef, s);
        SDL_FreeSurface(s);
        return res;
      }

      SDL_Texture* LoadImage(const std::string& fname,
                             uint8_t rMask,
                             uint8_t gMask,
                             uint8_t bMask)
      {
        SDL_Texture* res = nullptr;
        SDL_Surface* s = SDL_LoadBMP(fname.data());
        SDL_SetColorKey(s, SDL_TRUE, SDL_MapRGB(s->format, rMask, gMask, bMask));
        res = SDL_CreateTextureFromSurface(_rendRef, s);
        SDL_FreeSurface(s);
        return res;
      }

      void PushClipRect()
      {
        SDL_RenderGetClipRect(_rendRef, &_currentClipRect);
        _renderClipRects.push(_currentClipRect);
      }

      void PopClipRect()
      {
        if (!_renderClipRects.empty())
        {
          _currentClipRect = _renderClipRects.top();
          _renderClipRects.pop();

          if (_currentClipRect.x == 0
           && _currentClipRect.y == 0
           && _currentClipRect.w == 0
           && _currentClipRect.h == 0)
          {
            SDL_RenderSetClipRect(_rendRef, nullptr);
          }
          else
          {
            SDL_RenderSetClipRect(_rendRef, &_currentClipRect);
          }
        }
      }

      void CreateScreenCanvas();

      SDL_Renderer* _rendRef = nullptr;
      SDL_Window* _windowRef = nullptr;

      SDL_Texture* _font = nullptr;

      SDL_Texture* _btnNormal   = nullptr;
      SDL_Texture* _btnPressed  = nullptr;
      SDL_Texture* _btnHover    = nullptr;
      SDL_Texture* _btnDisabled = nullptr;

      bool _initialized = false;

      int _windowWidth  = 0;
      int _windowHeight = 0;

      const int FontW = 8;
      const int FontH = 16;

      uint64_t _globalId = 0;

      std::map<uint64_t, std::unique_ptr<Canvas>> _canvases;

      std::unique_ptr<Canvas> _screenCanvas;

      Canvas* _topCanvas = nullptr;

      SDL_Rect _currentClipRect;

      std::stack<SDL_Rect> _renderClipRects;

      friend class Element;
      friend class Canvas;
  };

  // ===========================================================================
  //                              BASE WIDGET CLASS
  // ===========================================================================
  class Element
  {
    public:
      Element(Canvas* parent,
              const SDL_Rect& transform,
              SDL_Renderer* rendRef);

      void SetEnabled(bool enabled)
      {
        _enabled = enabled;
      }

      bool IsEnabled()
      {
        return _enabled;
      }

      void SetVisible(bool visible)
      {
        _visible = visible;
      }

      bool IsVisible()
      {
        return _visible;
      }

      void ShowOutline(bool value)
      {
        _showOutline = value;
      }

      const uint64_t& Id()
      {
        return _id;
      }

      const SDL_Rect& Transform()
      {
        return (_owner == nullptr) ? _transform : _localTransform;
      }

      void SetTransform(const SDL_Rect& transform);

      void HandleEvents(const SDL_Event& evt)
      {
        if (!_enabled || !_visible)
        {
          return;
        }

        switch (evt.type)
        {
          case SDL_MOUSEMOTION:
          {
            if (!_mouseEnter && IsMouseInside(evt))
            {
              _mouseEnter = true;

              if (CanBeCalled(_onMouseHoverIntl))
              {
                _onMouseHoverIntl(this);
              }

              if (CanBeCalled(OnMouseHover))
              {
                OnMouseHover(this);
              }
            }
            else if (_mouseEnter && !IsMouseInside(evt))
            {
              _mouseEnter = false;

              if (CanBeCalled(_onMouseOutIntl))
              {
                _onMouseOutIntl(this);
              }

              if (CanBeCalled(OnMouseOut))
              {
                OnMouseOut(this);
              }
            }
          }
          break;

          case SDL_MOUSEBUTTONDOWN:
          {
            if (IsMouseInside(evt))
            {
              if (CanBeCalled(_onMouseDownIntl))
              {
                _onMouseDownIntl(this);
              }

              if (CanBeCalled(OnMouseDown))
              {
                OnMouseDown(this);
              }
            }
          }
          break;

          case SDL_MOUSEBUTTONUP:
          {
            if (IsMouseInside(evt))
            {
              if (CanBeCalled(_onMouseUpIntl))
              {
                _onMouseUpIntl(this);
              }

              if (CanBeCalled(OnMouseUp))
              {
                OnMouseUp(this);
              }
            }
          }
          break;
        }
      }

      void ResetHandlers()
      {
        OnMouseDown  = std::function<void(Element*)>();
        OnMouseUp    = std::function<void(Element*)>();
        OnMouseHover = std::function<void(Element*)>();
        OnMouseOut   = std::function<void(Element*)>();
      }

      void Draw()
      {
        if (_visible)
        {
          DrawImpl();

          if (_showOutline)
          {
            DrawOutline();
          }
        }
      }

      std::function<void(Element*)> OnMouseDown;
      std::function<void(Element*)> OnMouseUp;
      std::function<void(Element*)> OnMouseHover;
      std::function<void(Element*)> OnMouseOut;

    protected:
      bool IsMouseInside(const SDL_Event& evt);

      template <typename F>
      bool CanBeCalled(const F& fn)
      {
        return (fn.target_type() != typeid(void));
      }

      void UpdateTransform();

      virtual void DrawImpl() = 0;

      void SetOutline()
      {
        _debugOutline[0] = { _transform.x, _transform.y };
        _debugOutline[1] = { _transform.x + _transform.w, _transform.y };
        _debugOutline[2] = { _transform.x + _transform.w, _transform.y + _transform.h };
        _debugOutline[3] = { _transform.x, _transform.y + _transform.h };
        _debugOutline[4] = { _transform.x, _transform.y };
      }

      void DrawOutline()
      {
        uint8_t r, g, b, a;
        SDL_GetRenderDrawColor(_rendRef, &r, &g, &b, &a);

        SDL_SetRenderDrawColor(_rendRef, 255, 255, 255, 255);
        SDL_RenderDrawLines(_rendRef, _debugOutline, 5);
        SDL_RenderDrawLine(_rendRef,
                           _transform.x,
                           _transform.y,
                           _transform.x + _transform.w,
                           _transform.y + _transform.h);
        SDL_RenderDrawLine(_rendRef,
                           _transform.x + _transform.w,
                           _transform.y,
                           _transform.x,
                           _transform.y + _transform.h);
        SDL_SetRenderDrawColor(_rendRef, r, g, b, a);
      }

      void ResetHandlersIntl()
      {
        _onMouseDownIntl  = std::function<void(Element*)>();
        _onMouseUpIntl    = std::function<void(Element*)>();
        _onMouseHoverIntl = std::function<void(Element*)>();
        _onMouseOutIntl   = std::function<void(Element*)>();
      }

      SDL_Rect _transform;
      SDL_Rect _localTransform;

      SDL_Renderer* _rendRef = nullptr;

      SDL_Point _debugOutline[5];

      uint64_t _id = 0;

      bool _mouseEnter = false;

      bool _enabled     = true;
      bool _visible     = true;
      bool _showOutline = false;

      std::function<void(Element*)> _onMouseDownIntl;
      std::function<void(Element*)> _onMouseUpIntl;
      std::function<void(Element*)> _onMouseHoverIntl;
      std::function<void(Element*)> _onMouseOutIntl;

      Canvas* _owner = nullptr;

      friend class Canvas;
  };

  // ===========================================================================
  //                              CANVAS
  // ===========================================================================
  class Canvas : public Element
  {
    public:
      Canvas(const SDL_Rect& transform,
             SDL_Renderer* rendRef)
        : Element(nullptr, transform, rendRef) {}

      void HandleEvents(const SDL_Event& evt)
      {
        Element::HandleEvents(evt);

        switch (evt.type)
        {
          case SDL_MOUSEMOTION:
          case SDL_MOUSEBUTTONUP:
          case SDL_MOUSEBUTTONDOWN:
          {
            Element* newElement = nullptr;

            for (int i = _elements.size() - 1; i >= 0; i--)
            {
              auto it = _elements.begin();
              std::advance(it, i);
              if (it->second->IsVisible()
               && it->second->IsEnabled()
               && it->second->IsMouseInside(evt))
              {
                newElement = it->second.get();
                break;
              }
            }

            if (_topElement != newElement)
            {
              if (_topElement != nullptr)
              {
                _topElement->HandleEvents(evt);
              }

              _topElement = newElement;
            }
          }
          break;
        }

        if (_topElement != nullptr)
        {
          _topElement->HandleEvents(evt);
        }
      }

      void SetTransform(const SDL_Rect& transform)
      {
        Element::SetTransform(transform);
        Element::UpdateTransform();

        for (auto& kvp : _elements)
        {
          kvp.second->UpdateTransform();
        }
      }

      void Draw()
      {
        if (!_visible)
        {
          return;
        }

        Element::Draw();

        Manager::Get().PushClipRect();

        SDL_RenderSetClipRect(_rendRef, &_transform);

        for (auto& kvp : _elements)
        {
          kvp.second->Draw();
        }

        Manager::Get().PopClipRect();
      }

      void DrawImpl() override {}

    private:
      Element* Add(Element* e)
      {
        if (e == nullptr)
        {
          SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Trying to add null to canvas!");
          return nullptr;
        }

        uint64_t id = e->Id();
        _elements[id].reset(e);

        return _elements[id].get();
      }

      bool IsOutsideCanvas(const SDL_Rect& transform)
      {
        bool cond = ((transform.x + transform.w) < _transform.x)
                 || ((transform.y + transform.h) < _transform.y)
                 || (transform.x > (_transform.x + _transform.w))
                 || (transform.y > (_transform.y + _transform.h));

        return cond;
      }

      std::map<uint64_t, std::unique_ptr<Element>> _elements;

      Element* _topElement = nullptr;

      friend class Manager;
  };

  // ===========================================================================
  //                             DEFINITIONS
  // ===========================================================================
  Element::Element(Canvas* parent,
                   const SDL_Rect& transform,
                   SDL_Renderer* rendRef)
  {
    _owner   = parent;
    _rendRef = rendRef;

    _id = Manager::Get().GetNewId();

    SetTransform(transform);
    UpdateTransform();
  }

  void Element::SetTransform(const SDL_Rect& transform)
  {
    _localTransform = transform;
  }

  void Element::UpdateTransform()
  {
    if (_owner == nullptr)
    {
      _transform = _localTransform;
    }
    else
    {
      auto& pt = _owner->Transform();

      _transform.x = _localTransform.x + pt.x;
      _transform.y = _localTransform.y + pt.y;
      _transform.w = _localTransform.w;
      _transform.h = _localTransform.h;
    }

    SetOutline();
  }

  bool Element::IsMouseInside(const SDL_Event& evt)
  {
    bool insideClipRect = true;

    if (_owner != nullptr)
    {
      auto& ot = _owner->Transform();

      insideClipRect = (evt.motion.x >= ot.x
                     && evt.motion.x <= ot.x + ot.w
                     && evt.motion.x >= ot.y
                     && evt.motion.y <= ot.y + ot.h);
    }

    bool insideTransform = (evt.motion.x >= _transform.x
                         && evt.motion.x <= _transform.x + _transform.w
                         && evt.motion.y >= _transform.y
                         && evt.motion.y <= _transform.y + _transform.h);

    return (insideClipRect && insideTransform);
  }

  void Manager::CreateScreenCanvas()
  {
    _screenCanvas = std::make_unique<Canvas>(SDL_Rect{ 0, 0, _windowWidth, _windowHeight }, _rendRef);
    _screenCanvas->ResetHandlersIntl();
  }

  // ===========================================================================
  //                              IMAGE
  // ===========================================================================
  class Image : public Element
  {
    public:
      Image(Canvas* owner,
            SDL_Texture* image,
            const SDL_Rect& transform,
            SDL_Renderer* rendRef)
        : Element(owner, transform, rendRef)
      {
        _image = image;

        _src.x = 0;
        _src.y = 0;

        SDL_QueryTexture(_image,
                         nullptr,
                         nullptr,
                         &_src.w,
                         &_src.h);
      }

      void DrawImpl() override
      {
        SDL_RenderCopyEx(_rendRef,
                         _image,
                         &_src,
                         &_transform,
                         0.0,
                         nullptr,
                         SDL_FLIP_NONE);
      }

    private:
      SDL_Texture* _image = nullptr;

      SDL_Rect _src;
  };

  // ===========================================================================
  //                           GUI ELEMENTS CREATION
  // ===========================================================================
  Canvas* Manager::CreateCanvas(const SDL_Rect& transform)
  {
    std::unique_ptr<Canvas> canvas = std::make_unique<Canvas>(transform,
                                                              _rendRef);
    uint64_t id = canvas->Id();
    _canvases[id] = std::move(canvas);
    return _canvases[id].get();
  }

  Image* Manager::CreateImage(Canvas* canvas,
                            const SDL_Rect& transform,
                            SDL_Texture* image)
  {
    Canvas* c = (canvas == nullptr) ? _screenCanvas.get() : canvas;
    Image* img = new Image(c, image, transform, _rendRef);
    return static_cast<Image*>(c->Add(img));
  }

  // ===========================================================================
  //                             IMPLEMENTATIONS
  // ===========================================================================
  void Manager::Draw()
  {
    for (auto& kvp : _canvases)
    {
      kvp.second->Draw();
    }

    _screenCanvas->Draw();
  }

  void Manager::HandleEvents(const SDL_Event& evt)
  {
    switch (evt.type)
    {
      case SDL_MOUSEMOTION:
      case SDL_MOUSEBUTTONUP:
      case SDL_MOUSEBUTTONDOWN:
      {
        Canvas* newCanvas = nullptr;

        for (int i = _canvases.size() - 1; i >= 0; i--)
        {
          auto it = _canvases.begin();
          std::advance(it, i);
          if (it->second->IsVisible()
           && it->second->IsEnabled()
           && it->second->IsMouseInside(evt))
          {
            newCanvas = it->second.get();
            break;
          }
        }

        //
        // If we moused out from canvas, we still need
        // to let it handle event for the last time
        // for OnMouseOut event.
        //
        if (_topCanvas != newCanvas)
        {
          if (_topCanvas != nullptr)
          {
            _topCanvas->HandleEvents(evt);
          }

          _topCanvas = newCanvas;
        }
      }
      break;
    }

    if (_topCanvas != nullptr)
    {
      _topCanvas->HandleEvents(evt);
    }
    else
    {
      _screenCanvas->HandleEvents(evt);
    }
  }

  // ===========================================================================
  //                              SHORTCUTS
  // ===========================================================================
  void Init(SDL_Renderer* rendRef, SDL_Window* screenRef)
  {
    Manager::Get().Init(rendRef, screenRef);
  }

  void HandleEvents(const SDL_Event& evt)
  {
    Manager::Get().HandleEvents(evt);
  }

  void Draw()
  {
    Manager::Get().Draw();
  }

  Canvas* CreateCanvas(const SDL_Rect& transform)
  {
    return Manager::Get().CreateCanvas(transform);
  }

  Image* CreateImage(Canvas* canvas,
                   const SDL_Rect& transform,
                   SDL_Texture* image)
  {
    return Manager::Get().CreateImage(canvas, transform, image);
  }
}

#endif // S2G_H
