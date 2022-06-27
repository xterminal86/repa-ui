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

      void Init(SDL_Renderer* rendRef)
      {
        if (_initialized)
        {
          return;
        }

        _rendRef = rendRef;

        PrepareImages();

        _initialized = true;
      }

      void HandleEvents(const SDL_Event& evt);
      void Draw(bool debugMode = false);

      // =======================================================================
      Canvas* CreateCanvas(const SDL_Rect& transform,
                           SDL_Texture* bgImage,
                           Canvas* parent);
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
        _font = LoadImage("font.bmp", 0xFF, 0x00, 0xFF);

        _btnNormal   = LoadImage("btn-normal.bmp");
        _btnPressed  = LoadImage("btn-pressed.bmp");
        _btnHover    = LoadImage("btn-hover.bmp");
        _btnDisabled = _btnPressed; //LoadImage("btn-pressed.bmp");
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

      SDL_Renderer* _rendRef = nullptr;

      SDL_Texture* _font = nullptr;

      SDL_Texture* _btnNormal   = nullptr;
      SDL_Texture* _btnPressed  = nullptr;
      SDL_Texture* _btnHover    = nullptr;
      SDL_Texture* _btnDisabled = nullptr;

      bool _initialized = false;

      const int FontW = 8;
      const int FontH = 16;

      uint64_t _globalId = 0;

      std::map<uint64_t, std::unique_ptr<Canvas>> _canvases;

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
        if (!_enabled)
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

      void Draw(bool debugMode = false)
      {
        if (_visible)
        {
          DrawImpl();

          if (debugMode)
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

      SDL_Rect _transform;
      SDL_Rect _localTransform;

      SDL_Renderer* _rendRef = nullptr;

      SDL_Point _debugOutline[5];

      uint64_t _id = 0;

      bool _mouseEnter = false;

      bool _enabled = true;
      bool _visible = true;

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
             SDL_Renderer* rendRef,
             SDL_Texture* bgImage,
             Canvas* parent = nullptr)
        : Element(parent, transform, rendRef)
      {
        _bgImage    = bgImage;
        _bgImageDst = _transform;
      }

      void HandleEvents(const SDL_Event& evt)
      {
        Element::HandleEvents(evt);

        for (auto& kvp : _elements)
        {
          kvp.second->HandleEvents(evt);
        }
      }

      void SetTransform(const SDL_Rect& transform)
      {
        Element::SetTransform(transform);
        Element::UpdateTransform();

        _bgImageDst = _transform;

        for (auto& kvp : _elements)
        {
          kvp.second->UpdateTransform();
        }
      }

      void Draw(bool debugMode)
      {
        Element::Draw(debugMode);

        Manager::Get().PushClipRect();

        SDL_RenderSetClipRect(_rendRef, &_transform);

        for (auto& kvp : _elements)
        {
          kvp.second->Draw(debugMode);
        }

        Manager::Get().PopClipRect();
      }

      void DrawImpl() override
      {
        if (_bgImage != nullptr)
        {
          SDL_RenderCopy(_rendRef,
                         _bgImage,
                         nullptr,
                         &_bgImageDst);
        }
      }

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

      SDL_Texture* _bgImage = nullptr;

      SDL_Rect _bgImageDst;

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
  Canvas* Manager::CreateCanvas(const SDL_Rect& transform,
                                SDL_Texture* bgImage,
                                Canvas* parent)
  {
    std::unique_ptr<Canvas> canvas = std::make_unique<Canvas>(transform,
                                                              _rendRef,
                                                              bgImage,
                                                              parent);
    uint64_t id = canvas->Id();
    _canvases[id] = std::move(canvas);
    return _canvases[id].get();
  }

  Image* Manager::CreateImage(Canvas* canvas,
                            const SDL_Rect& transform,
                            SDL_Texture* image)
  {
    if (canvas == nullptr)
    {
      SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Canvas is null!");
      return nullptr;
    }

    Image* img = new Image(canvas, image, transform, _rendRef);

    return static_cast<Image*>(canvas->Add(img));
  }

  // ===========================================================================
  //                             IMPLEMENTATIONS
  // ===========================================================================
  void Manager::Draw(bool debugMode)
  {
    for (auto& kvp : _canvases)
    {
      kvp.second->Draw(debugMode);
    }
  }

  void Manager::HandleEvents(const SDL_Event& evt)
  {
    switch (evt.type)
    {
      case SDL_MOUSEMOTION:
      case SDL_MOUSEBUTTONUP:
      case SDL_MOUSEBUTTONDOWN:
      {
        for (auto& kvp : _canvases)
        {
          kvp.second->HandleEvents(evt);
        }
      }
      break;
    }
  }

  // ===========================================================================
  //                              SHORTCUTS
  // ===========================================================================
  void Init(SDL_Renderer* rendRef)
  {
    Manager::Get().Init(rendRef);
  }

  void HandleEvents(const SDL_Event& evt)
  {
    Manager::Get().HandleEvents(evt);
  }

  void Draw(bool debugMode = false)
  {
    Manager::Get().Draw(debugMode);
  }

  Canvas* CreateCanvas(const SDL_Rect& transform,
                       SDL_Texture* bgImage,
                       Canvas* parent = nullptr)
  {
    return Manager::Get().CreateCanvas(transform, bgImage, parent);
  }

  Image* CreateImage(Canvas* canvas,
                   const SDL_Rect& transform,
                   SDL_Texture* image)
  {
    return Manager::Get().CreateImage(canvas, transform, image);
  }
}

#endif // S2G_H
