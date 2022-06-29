#ifndef REPAUI_H
#define REPAUI_H

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
  enum class EventType
  {
    MOUSE_OVER = 0,
    MOUSE_OUT,
    MOUSE_MOVE,
    MOUSE_DOWN,
    MOUSE_UP
  };

  // ===========================================================================
  //                              UTILS
  // ===========================================================================
  template <typename T>
  T Clamp(T value, T min, T max)
  {
    return std::max(min, std::min(value, max));
  }

  bool IsSet(const SDL_Rect& rect)
  {
    return (rect.x != 0
         && rect.y != 0
         && rect.w != 0
         && rect.h != 0);
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
      friend class Image;
  };

  // ===========================================================================
  //                              BASE WIDGET CLASS
  // ===========================================================================
  class Element
  {
    public:
      Element(Canvas* parent,
              const SDL_Rect& transform);

      void SetEnabled(bool enabled)
      {
        _enabled = enabled;

        if (!_enabled)
        {
          _mouseEnter = false;
        }
      }

      bool IsEnabled()
      {
        return _enabled;
      }

      void SetVisible(bool visible)
      {
        _visible = visible;

        if (!_visible)
        {
          _mouseEnter = false;
        }
      }

      bool IsVisible()
      {
        return _visible;
      }

      bool IsEnabledAndVisible()
      {
        return (_enabled && _visible);
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

      const SDL_Rect& GetCornersCoordsAbsolute()
      {
        _corners.x = _transform.x;
        _corners.y = _transform.y;
        _corners.w = _transform.x + _transform.w;
        _corners.h = _transform.y + _transform.h;

        return _corners;
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
            if (IsMouseInside(evt))
            {
              if (!_mouseEnter)
              {
                _mouseEnter = true;
                RaiseEvent(EventType::MOUSE_OVER);
              }

              RaiseEvent(EventType::MOUSE_MOVE);
            }
            else
            {
              if (_mouseEnter)
              {
                _mouseEnter = false;
                RaiseEvent(EventType::MOUSE_OUT);
              }
            }
          }
          break;

          case SDL_MOUSEBUTTONDOWN:
          {
            if (IsMouseInside(evt))
            {
              RaiseEvent(EventType::MOUSE_DOWN);
            }
          }
          break;

          case SDL_MOUSEBUTTONUP:
          {
            if (IsMouseInside(evt))
            {
              RaiseEvent(EventType::MOUSE_UP);
            }
          }
          break;
        }
      }

      void ResetHandlers()
      {
        OnMouseDown = std::function<void(Element*)>();
        OnMouseUp   = std::function<void(Element*)>();
        OnMouseOver = std::function<void(Element*)>();
        OnMouseOut  = std::function<void(Element*)>();
        OnMouseMove = std::function<void(Element*)>();
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
      std::function<void(Element*)> OnMouseOver;
      std::function<void(Element*)> OnMouseOut;
      std::function<void(Element*)> OnMouseMove;

    protected:
      bool IsMouseInside(const SDL_Event& evt);

      template <typename F>
      bool CanBeCalled(const F& fn)
      {
        return (fn.target_type() != typeid(void));
      }

      template <typename CallbackIntl, typename Callback>
      void CallHandlers(const CallbackIntl& cbIntl,
                        const Callback& cb)
      {
        if (CanBeCalled(cbIntl))
        {
          cbIntl(this);
        }

        if (CanBeCalled(cb))
        {
          cb(this);
        }
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
        _onMouseDownIntl = std::function<void(Element*)>();
        _onMouseUpIntl   = std::function<void(Element*)>();
        _onMouseOverIntl = std::function<void(Element*)>();
        _onMouseOutIntl  = std::function<void(Element*)>();
        _onMouseMoveIntl = std::function<void(Element*)>();
      }

      void RaiseEvent(EventType eventType)
      {
        switch (eventType)
        {
          case EventType::MOUSE_OVER:
          {
            CallHandlers(_onMouseOverIntl, OnMouseOver);
          }
          break;

          case EventType::MOUSE_OUT:
          {
            CallHandlers(_onMouseOutIntl, OnMouseOut);
          }
          break;

          case EventType::MOUSE_DOWN:
          {
            CallHandlers(_onMouseDownIntl, OnMouseDown);
          }
          break;

          case EventType::MOUSE_UP:
          {
            CallHandlers(_onMouseUpIntl, OnMouseUp);
          }
          break;

          case EventType::MOUSE_MOVE:
          {
            CallHandlers(_onMouseMoveIntl, OnMouseMove);
          }
          break;

          default:
          {
            SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                        "Unknown event type: %i",
                        (int)eventType);
          }
          break;
        }
      }

      SDL_Rect _transform;
      SDL_Rect _localTransform;
      SDL_Rect _corners;

      SDL_Renderer* _rendRef = nullptr;

      SDL_Point _debugOutline[5];

      uint64_t _id = 0;

      bool _mouseEnter = false;

      bool _enabled     = true;
      bool _visible     = true;
      bool _showOutline = false;

      std::function<void(Element*)> _onMouseDownIntl;
      std::function<void(Element*)> _onMouseUpIntl;
      std::function<void(Element*)> _onMouseOverIntl;
      std::function<void(Element*)> _onMouseOutIntl;
      std::function<void(Element*)> _onMouseMoveIntl;

      Canvas* _owner = nullptr;

      friend class Canvas;
      friend class Manager;
  };

  // ===========================================================================
  //                              CANVAS
  // ===========================================================================
  class Canvas : public Element
  {
    public:
      Canvas(const SDL_Rect& transform,
             SDL_Renderer* rendRef)
        : Element(nullptr, transform)
      {
        _rendRef = rendRef;
      }

      void HandleEvents(const SDL_Event& evt)
      {
        //
        // Disabled canvas should not handle events at all.
        //
        if (!_enabled || !_visible)
        {
          return;
        }

        Element::HandleEvents(evt);

        switch (evt.type)
        {
          case SDL_MOUSEMOTION:
          case SDL_MOUSEBUTTONUP:
          case SDL_MOUSEBUTTONDOWN:
          {
            Element* newElement = nullptr;

            for (auto it = _elements.rbegin(); it != _elements.rend(); it++)
            {
              if (it->second->IsEnabledAndVisible()
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
                _topElement->RaiseEvent(EventType::MOUSE_OUT);
                _topElement->_mouseEnter = false;
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

      void UpdateChildTransforms()
      {
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
                   const SDL_Rect& transform)
  {
    _owner   = parent;
    _rendRef = Manager::Get()._rendRef;

    _id = Manager::Get().GetNewId();

    SetTransform(transform);
    UpdateTransform();
  }

  void Element::SetTransform(const SDL_Rect& transform)
  {
    _localTransform = transform;

    if (_owner != nullptr)
    {
      _owner->UpdateChildTransforms();
    }
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
      enum class DrawType
      {
        NORMAL = 0,
        TILED,
        SLICED
      };

      Image(Canvas* owner,
            SDL_Texture* image,
            const SDL_Rect& transform)
        : Element(owner, transform)
      {
        _image = image;

        _imageSrc.x = 0;
        _imageSrc.y = 0;

        SDL_QueryTexture(_image,
                         nullptr,
                         nullptr,
                         &_imageSrc.w,
                         &_imageSrc.h);

        SetTileRate({ 1, 1 });
      }

      void SetTileRate(const std::pair<size_t, size_t>& tileRate)
      {
        _tileRate = tileRate;

        _tileRate.first  = Clamp(_tileRate.first,  (size_t)1, (size_t)_transform.w);
        _tileRate.second = Clamp(_tileRate.second, (size_t)1, (size_t)_transform.h);

        _stepX = _transform.w / _tileRate.first;
        _stepY = _transform.h / _tileRate.second;
      }

      void SetSlicePoints(const SDL_Rect& slicePoints)
      {
        _slicePoints = slicePoints;

        _slicePoints.w = (_slicePoints.w < 0)
                         ? (_imageSrc.w + _slicePoints.w - 1)
                         : _slicePoints.w;

        _slicePoints.h = (_slicePoints.h < 0)
                         ? (_imageSrc.h + _slicePoints.h - 1)
                         : _slicePoints.h;

        _slicePoints.x = Clamp(_slicePoints.x,
                               0,
                               _imageSrc.w);
        _slicePoints.y = Clamp(_slicePoints.y,
                               0,
                               _imageSrc.h);
        _slicePoints.w = Clamp(_slicePoints.w,
                               0,
                               _imageSrc.w);
        _slicePoints.h = Clamp(_slicePoints.h,
                               0,
                               _imageSrc.h);

        if (_slicePoints.w < _slicePoints.x)
        {
          _slicePoints.w = _slicePoints.x;
        }

        if (_slicePoints.h < _slicePoints.y)
        {
          _slicePoints.h = _slicePoints.y;
        }

        //
        //  --- --- ---
        // | 1 | 2 | 3 |
        //  --- --- ---
        // | 4 | 5 | 6 |
        //  --- --- ---
        // | 7 | 8 | 9 |
        //  --- --- ---
        //

        // 1
        _slices[0] = { 0, 0, _slicePoints.x, _slicePoints.y };
        // 2
        _slices[1] = { _slicePoints.x, 0, _slicePoints.w, _slicePoints.y };
        // 3
        _slices[2] = { _slicePoints.w, 0, _imageSrc.w, _slicePoints.y };
        // 4
        _slices[3] = { 0, _slicePoints.y, _slicePoints.x, _slicePoints.h };
        // 5
        _slices[4] = { _slicePoints.x, _slicePoints.y, _slicePoints.w, _slicePoints.h };
        // 6
        _slices[5] = { _slicePoints.w, _slicePoints.y, _imageSrc.w, _slicePoints.h };
        // 7
        _slices[6] = { 0, _slicePoints.h, _slicePoints.x, _imageSrc.h };
        // 8
        _slices[7] = { _slicePoints.x, _slicePoints.h, _slicePoints.w, _imageSrc.h };
        // 9
        _slices[8] = { _slicePoints.w, _slicePoints.h, _imageSrc.w, _imageSrc.h };

        //
        // Drawing coordinates accordingly
        // assuming slice size is uniform and squared
        // thus we can use _slice[0] width and height for reference.
        //
        auto t = Transform();

        _fragments[0] = { t.x, t.y, _slices[0].w, _slices[0].h };
        _fragments[1] = { t.x + _slices[0].w, t.y, t.w - _slices[0].w * 2, _slices[0].h };
        _fragments[2] = { t.x + t.w - _slices[0].w, t.y, _slices[0].w, _slices[0].h };
        _fragments[3] = { t.x, t.y + _slices[0].h, _slices[0].w, t.h - _slices[0].h * 2 };
        _fragments[4] = { t.x + _slices[0].w, t.y + _slices[0].h, t.w - _slices[0].w * 2, t.h - _slices[0].h * 2 };
        _fragments[5] = { t.x + t.w - _slices[0].w, t.y + _slices[0].h, _slices[0].w, t.h - _slices[0].h * 2 };
        _fragments[6] = { t.x, t.y + t.h - _slices[0].h, _slices[0].w, _slices[0].h };
        _fragments[7] = { t.x + _slices[0].w, t.y + t.h - _slices[0].h, t.w - _slices[0].w * 2, _slices[0].h };
        _fragments[8] = { t.x + t.w - _slices[0].w, t.y + t.h - _slices[0].h, _slices[0].w, _slices[0].h };
      }

      void SetDrawType(DrawType drawType)
      {
        _drawType = drawType;

        if (_drawType == DrawType::SLICED
         && !IsSet(_slicePoints))
        {
          SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                      "Image #%lu: slice points not set!", _id);
        }
      }

      void DrawImpl() override
      {
        switch (_drawType)
        {
          case DrawType::NORMAL:
            DrawNormal();
            break;

          case DrawType::SLICED:
          {
            if (IsSet(_slicePoints))
            {
              DrawSliced();
            }
            else
            {
              DrawNormal();
            }
          }
          break;

          case DrawType::TILED:
            DrawTiled();
            break;
        }
      }

    private:
      void DrawNormal()
      {
        SDL_RenderCopyEx(_rendRef,
                         _image,
                         nullptr,
                         &_transform,
                         0.0,
                         nullptr,
                         SDL_FLIP_NONE);
      }

      void DrawSliced()
      {
        for (size_t i = 0; i < 9; i++)
        {
          _tmp =
          {
            _slices[i].x,
            _slices[i].y,
            _slices[i].w - _slices[i].x,
            _slices[i].h - _slices[i].y
          };

          SDL_RenderCopyEx(_rendRef,
                           _image,
                           &_tmp,
                           &_fragments[i],
                           0.0,
                           nullptr,
                           SDL_FLIP_NONE);
        }
      }

      void DrawTiled()
      {
        auto& c = GetCornersCoordsAbsolute();

        for (int x = c.x; x < c.w; x += _stepX)
        {
          for (int y = c.y; y < c.h; y += _stepY)
          {
            _tmp = { x, y, _stepX, _stepY };

            //
            // If tiling doesn't fit perfectly into image size,
            // calculate remaining clipping width and height
            // for the last step. Artifacts may occur though
            // if transform size doesn't divide wholly on tiling rate.
            //
            if (_tmp.x + _tmp.w > c.w)
            {
              _tmp.w = c.w - _tmp.x;
            }

            if (_tmp.y + _tmp.h > c.h)
            {
              _tmp.h = c.h - _tmp.y;
            }

            SDL_RenderCopyEx(_rendRef,
                             _image,
                             nullptr,
                             &_tmp,
                             0.0,
                             nullptr,
                             SDL_FLIP_NONE);
          }
        }
      }

      DrawType _drawType = DrawType::NORMAL;

      SDL_Texture* _image = nullptr;

      SDL_Rect _imageSrc;
      SDL_Rect _tmp;
      SDL_Rect _slices[9];
      SDL_Rect _fragments[9];
      SDL_Rect _slicePoints;

      std::pair<size_t, size_t> _tileRate;

      int _stepX = 1;
      int _stepY = 1;
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
    Image* img = new Image(c, image, transform);
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

        for (auto it = _canvases.rbegin(); it != _canvases.rend(); it++)
        {
          if (it->second->IsEnabledAndVisible()
           && it->second->IsMouseInside(evt))
          {
            newCanvas = it->second.get();
            break;
          }
        }

        //
        // If we moused out from canvas, we still need
        // to let it handle event for the last time
        // for OnMouseOut event
        // (also any top element of it that is clipped by the canvas).
        //
        if (_topCanvas != newCanvas)
        {
          if (_topCanvas != nullptr
           && _topCanvas->IsEnabledAndVisible())
          {
            if (_topCanvas->_topElement != nullptr)
            {
              _topCanvas->_topElement->RaiseEvent(EventType::MOUSE_OUT);
              _topCanvas->_topElement->_mouseEnter = false;

              //
              // To prevent OnMouseOut() duplicate firing
              // if we mouse outed from clipped element
              // and overed again on an empty spot on the canvas.
              //
              _topCanvas->_topElement = nullptr;
            }

            _topCanvas->RaiseEvent(EventType::MOUSE_OUT);
            _topCanvas->_mouseEnter = false;
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

#endif // REPAUI_H
