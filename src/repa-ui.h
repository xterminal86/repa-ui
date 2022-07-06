#ifndef REPAUI_H
#define REPAUI_H

#include "SDL2/SDL.h"

#include <algorithm>
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
  class Text;
  // ===========================================================================

  class Manager final
  {
    public:
      static Manager& Get()
      {
        static Manager instance;
        return instance;
      }

      void Init(SDL_Window* windowRef)
      {
        if (_initialized)
        {
          return;
        }

        _rendRef   = SDL_GetRenderer(windowRef);
        _windowRef = windowRef;

        SDL_GetWindowSize(_windowRef, &_windowWidth, &_windowHeight);

        _renderTexture = CreateRenderTexture(_windowWidth * 3,
                                             _windowHeight * 3);

        _renderTempTexture = CreateRenderTexture(_windowWidth,
                                                 _windowHeight);

        _renderDst =
        {
          _windowWidth,
          _windowHeight,
          _windowWidth,
          _windowHeight
        };

        CreateScreenCanvas();
        PrepareImages();
        CutFontGlyphs();

        _initialized = true;
      }

      void HandleEvents(const SDL_Event& evt);
      void Draw();

      // =======================================================================
      Canvas* CreateCanvas(const SDL_Rect& transform);
      Image* CreateImage(Canvas* canvas,
                          const SDL_Rect& transform,
                          SDL_Texture* image);
      Text* CreateText(Canvas* canvas,
                       const SDL_Point& pos,
                       const std::string& text);
      // =======================================================================

    private:
      struct GlyphInfo
      {
        int X;
        int Y;
      };

      void DrawToTexture();
      void DrawOnScreen();

      void ProcessCanvases(const SDL_Event& evt);

      const uint64_t& GetNewId()
      {
        _globalId++;
        return _globalId;
      }

      void PrepareImages()
      {
        _blankImage = LoadImageFromBase64(_pixelImageBase64);
        _font       = LoadImageFromBase64(_fontBase64, 255, 0, 255);

        _btnNormal   = LoadImageFromBase64(_btnNormalBase64);
        _btnPressed  = LoadImageFromBase64(_btnPressedBase64);
        _btnHover    = LoadImageFromBase64(_btnHoverBase64);
        _btnDisabled = _btnPressed;
      }

      void CutFontGlyphs()
      {
        int startingChar = 32;

        for (int y = 0; y < FontSheetH; y += FontH)
        {
          for (int x = 0; x < FontSheetW; x += FontW)
          {
            _fontDataByChar[startingChar] = { x, y };
            startingChar++;
          }
        }

        bool stop = false;
      }

      GlyphInfo* GetCharData(uint8_t ch)
      {
        GlyphInfo* res = nullptr;

        if (_fontDataByChar.count(ch) == 1)
        {
          res = &_fontDataByChar.at(ch);
        }
        else
        {
          res = &_fontDataByChar.at('?');
        }

        return res;
      }

      SDL_Texture* CreateRenderTexture(int w, int h)
      {
        return SDL_CreateTexture(_rendRef,
                                 SDL_PIXELFORMAT_RGBA32,
                                 SDL_TEXTUREACCESS_TARGET,
                                 w,
                                 h);
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

      SDL_Texture* LoadImageFromBase64(const std::string& base64Encoded)
      {
        SDL_Texture* res = nullptr;
        auto str = Base64_Decode(base64Encoded);
        std::vector<unsigned char> bytes;
        for (auto& c : str)
        {
            bytes.push_back(c);
        }
        SDL_RWops* data = SDL_RWFromMem(bytes.data(), bytes.size());
        SDL_Surface* s = SDL_LoadBMP_RW(data, 1);
        res = SDL_CreateTextureFromSurface(_rendRef, s);
        SDL_FreeSurface(s);
        return res;
      }

      SDL_Texture* LoadImageFromBase64(const std::string& base64Encoded,
                                       uint8_t rMask,
                                       uint8_t gMask,
                                       uint8_t bMask)
      {
        SDL_Texture* res = nullptr;
        auto str = Base64_Decode(base64Encoded);
        std::vector<unsigned char> bytes;
        for (auto& c : str)
        {
            bytes.push_back(c);
        }
        SDL_RWops* data = SDL_RWFromMem(bytes.data(), bytes.size());
        SDL_Surface* s = SDL_LoadBMP_RW(data, 1);
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

          if (!IsSet(_currentClipRect))
          {
            SDL_RenderSetClipRect(_rendRef, nullptr);
          }
          else
          {
            SDL_RenderSetClipRect(_rendRef, &_currentClipRect);
          }
        }
      }

      std::string Base64_Decode(const std::string& encoded_string)
      {
        int in_len = encoded_string.size();
        int i = 0;
        int j = 0;
        int in_ = 0;
        unsigned char char_array_4[4], char_array_3[3];
        std::string ret;

        while (in_len-- && ( encoded_string[in_] != '=') && IsBase64(encoded_string[in_]))
        {
          char_array_4[i++] = encoded_string[in_]; in_++;
          if (i ==4)
          {
            for (i = 0; i <4; i++)
            {
              char_array_4[i] = _base64Chars.find(char_array_4[i]);
            }

            char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
            char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
            char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

            for (i = 0; (i < 3); i++)
            {
              ret += char_array_3[i];
            }

            i = 0;
          }
        }

        if (i)
        {
          for (j = i; j <4; j++)
          {
            char_array_4[j] = 0;
          }

          for (j = 0; j <4; j++)
          {
            char_array_4[j] = _base64Chars.find(char_array_4[j]);
          }

          char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
          char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
          char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

          for (j = 0; (j < i - 1); j++)
          {
            ret += char_array_3[j];
          }
        }

        return ret;
      }

      bool IsBase64(unsigned char c)
      {
        auto res = std::find(_base64Chars.begin(),
                             _base64Chars.end(),
                             c);

        return (res != _base64Chars.end());
      }

      void CreateScreenCanvas();

      SDL_Renderer* _rendRef = nullptr;
      SDL_Window* _windowRef = nullptr;

      SDL_Texture* _font       = nullptr;
      SDL_Texture* _blankImage = nullptr;

      SDL_Texture* _btnNormal   = nullptr;
      SDL_Texture* _btnPressed  = nullptr;
      SDL_Texture* _btnHover    = nullptr;
      SDL_Texture* _btnDisabled = nullptr;

      SDL_Texture* _renderTexture     = nullptr;
      SDL_Texture* _renderTempTexture = nullptr;

      SDL_Color _oldRenderColor;

      SDL_Rect _renderDst;

      bool _initialized = false;

      int _windowWidth  = 0;
      int _windowHeight = 0;

      const uint8_t FontW = 8;
      const uint8_t FontH = 16;
      const uint8_t FontSheetW = 128;
      const uint8_t FontSheetH = 96;

      uint64_t _globalId = 0;

      std::map<uint64_t, std::unique_ptr<Canvas>> _canvases;
      std::map<uint8_t, GlyphInfo> _fontDataByChar;

      std::unique_ptr<Canvas> _screenCanvas;

      Canvas* _topCanvas = nullptr;

      SDL_Rect _currentClipRect;

      std::stack<SDL_Rect> _renderClipRects;

      const static std::string _base64Chars;
      const static std::string _fontBase64;
      const static std::string _pixelImageBase64;

      const static std::string _btnNormalBase64;
      const static std::string _btnPressedBase64;
      const static std::string _btnHoverBase64;

      friend class Element;
      friend class Canvas;
      friend class Image;
      friend class Text;
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
        _corners.x = _renderTransform.x;
        _corners.y = _renderTransform.y;
        _corners.w = _renderTransform.x + _renderTransform.w;
        _corners.h = _renderTransform.y + _renderTransform.h;

        return _corners;
      }

      virtual void SetTransform(const SDL_Rect& transform);

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
        _debugOutline = _transform;

        _debugOutline.x += Manager::Get()._renderDst.x;
        _debugOutline.y += Manager::Get()._renderDst.y;
      }

      void DrawOutline()
      {
        SDL_GetRenderDrawColor(_rendRef,
                              &_oldColor.r,
                              &_oldColor.g,
                              &_oldColor.b,
                              &_oldColor.a);

        if (_enabled)
        {
          SDL_SetRenderDrawColor(_rendRef, 255, 255, 255, 255);
        }
        else
        {
          SDL_SetRenderDrawColor(_rendRef, 255, 0, 0, 255);
        }

        SDL_RenderDrawRect(_rendRef, &_debugOutline);
        SDL_RenderDrawLine(_rendRef,
                           _debugOutline.x,
                           _debugOutline.y,
                           _debugOutline.x + _transform.w - 1,
                           _debugOutline.y + _transform.h - 1);
        SDL_RenderDrawLine(_rendRef,
                           _debugOutline.x,
                           _debugOutline.y + _transform.h - 1,
                           _debugOutline.x + _transform.w - 1,
                           _debugOutline.y);

        SDL_SetRenderDrawColor(_rendRef,
                               _oldColor.r,
                               _oldColor.g,
                               _oldColor.b,
                               _oldColor.a);
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
      SDL_Rect _renderTransform;
      SDL_Rect _corners;
      SDL_Rect _debugOutline;

      SDL_Renderer* _rendRef = nullptr;

      SDL_Color _oldColor;

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

      void SetTransform(const SDL_Rect& transform) override
      {
        Element::SetTransform(transform);
        Element::UpdateTransform();

        for (auto& kvp : _elements)
        {
          kvp.second->UpdateTransform();
        }
      }

    protected:
      void DrawImpl() override {}

    private:
      void Draw()
      {
        if (!_visible)
        {
          return;
        }

        for (auto& kvp : _elements)
        {
          kvp.second->Draw();
        }

        if (_showOutline)
        {
          DrawOutline();
        }
      }

      void Clear()
      {
        SDL_SetTextureColorMod(Manager::Get()._blankImage, 0, 0, 0);
        SDL_RenderCopy(_rendRef,
                       Manager::Get()._blankImage,
                       nullptr,
                       &_renderTransform);
      }

      Element* Add(Element* e)
      {
        if (e == nullptr)
        {
          SDL_LogError(SDL_LOG_CATEGORY_ERROR,
                       "Trying to add null to canvas!");
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
    UpdateTransform();
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

    _renderTransform = Manager::Get()._renderDst;

    _renderTransform.x += _transform.x;
    _renderTransform.y += _transform.y;
    _renderTransform.w = _transform.w;
    _renderTransform.h = _transform.h;

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
        _image = (image == nullptr)
                ? Manager::Get()._blankImage
                : image;

        _imageSrc.x = 0;
        _imageSrc.y = 0;

        SDL_QueryTexture(_image,
                         nullptr,
                         nullptr,
                         &_imageSrc.w,
                         &_imageSrc.h);

        SetTileRate({ 1, 1 });

        _color = { 255, 255, 255, 255 };
      }

      void SetColor(const SDL_Color& color)
      {
        _color = color;
      }

      const SDL_Color& GetColor()
      {
        return _color;
      }

      void SetBlending(bool isSet)
      {
        _blendMode = isSet
                     ? SDL_BLENDMODE_BLEND
                     : SDL_BLENDMODE_NONE;
      }

      const std::pair<size_t, size_t>& GetTileRate()
      {
        return _tileRate;
      }

      void SetTileRate(const std::pair<size_t, size_t>& tileRate)
      {
        _tileRate = tileRate;

        _tileRate.first  = Clamp<size_t>(_tileRate.first,  1, _localTransform.w);
        _tileRate.second = Clamp<size_t>(_tileRate.second, 1, _localTransform.h);

        CalculateSteps();
      }

      void SetSlicePoints(const SDL_Rect& slicePoints)
      {
        _slicePoints = slicePoints;

        _slicePoints.w = (_slicePoints.w < 0)
                         ? (_imageSrc.w + _slicePoints.w)
                         : _slicePoints.w;

        _slicePoints.h = (_slicePoints.h < 0)
                         ? (_imageSrc.h + _slicePoints.h)
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
        // | 0 | 1 | 2 |
        //  --- --- ---
        // | 3 | 4 | 5 |
        //  --- --- ---
        // | 6 | 7 | 8 |
        //  --- --- ---
        //
        //
        // Width and height must be 1 unit more for rect drawing bounds,
        // and since _slicePoints are absolute,
        // sometimes we need to add 1 for certain slices.
        //
        _slices[0] = { 0, 0, _slicePoints.x, _slicePoints.y };
        _slices[1] = { _slicePoints.x, 0, _slicePoints.w + 1, _slicePoints.y };
        _slices[2] = { _slicePoints.w + 1, 0, _imageSrc.w, _slicePoints.y };
        _slices[3] = { 0, _slicePoints.y, _slicePoints.x, _slicePoints.h + 1 };
        _slices[4] = { _slicePoints.x, _slicePoints.y, _slicePoints.w + 1, _slicePoints.h + 1 };
        _slices[5] = { _slicePoints.w + 1, _slicePoints.y, _imageSrc.w, _slicePoints.h + 1 };
        _slices[6] = { 0, _slicePoints.h + 1, _slicePoints.x, _imageSrc.h };
        _slices[7] = { _slicePoints.x, _slicePoints.h + 1, _slicePoints.w + 1, _imageSrc.h };
        _slices[8] = { _slicePoints.w + 1, _slicePoints.h + 1, _imageSrc.w, _imageSrc.h };

        for (auto& i : _slices)
        {
          _swh.push_back({ i.w - i.x - 1, i.h - i.y - 1 });
        }

        CalculateFragments();
      }

      void SetDrawType(DrawType drawType)
      {
        _drawType = drawType;
      }

    protected:
      void DrawImpl() override
      {
        //
        // Color and alpha is set on per-texture basis,
        // so if several elements share the same texture,
        // the properties for it will also be shared.
        //
        SDL_SetTextureBlendMode(_image, _blendMode);
        SDL_SetTextureColorMod(_image, _color.r, _color.g, _color.b);
        SDL_SetTextureAlphaMod(_image, _color.a);

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

          default:
            DrawNormal();
            break;
        }
      }

    private:
      void CalculateSteps()
      {
        _stepX = _localTransform.w / _tileRate.first;
        _stepY = _localTransform.h / _tileRate.second;
      }

      void CalculateFragments()
      {
        //
        // Drawing rect for screen output.
        //

        auto coords = GetCornersCoordsAbsolute();
        auto t = Transform();

        //
        // On the screen certain slices will share
        // the same width or height.
        //
        // !!!
        // FIRST PAIR IS STARTING COORDINATES,
        // SECOND - WIDTH AND HEIGHT
        // !!!
        //
        // I kinda fucked myself over
        // by misusing SDL_Rect for storing coordinates.
        //
        _fragments[0] = { coords.x, coords.y, _swh[0].first, _swh[0].second };
        _fragments[1] = { coords.x + _swh[0].first, coords.y, t.w - (_swh[0].first + _swh[2].first), _swh[0].second };
        _fragments[2] = { coords.w - _swh[2].first, coords.y, _swh[2].first, _swh[2].second };
        _fragments[3] = { coords.x, coords.y + _swh[0].second, _swh[0].first, t.h - (_swh[0].second + _swh[6].second) };
        _fragments[4] = { coords.x + _swh[0].first, coords.y + _swh[0].second, t.w - (_swh[0].first + _swh[2].first), t.h - (_swh[0].second + _swh[6].second) };
        _fragments[5] = { coords.w - _swh[2].first, coords.y + _swh[0].second, _swh[2].first, t.h - (_swh[0].second + _swh[6].second) };
        _fragments[6] = { coords.x, coords.h - _swh[6].second, _swh[0].first, _swh[6].second };
        _fragments[7] = { coords.x + _swh[0].first, coords.h - _swh[6].second, t.w - (_swh[0].first + _swh[2].first), _swh[6].second };
        _fragments[8] = { coords.w - _swh[2].first, coords.h - _swh[6].second, _swh[2].first, _swh[6].second };
      }

      void DrawNormal()
      {
        SDL_RenderCopy(_rendRef,
                        _image,
                        nullptr,
                        &_renderTransform);
      }

      void DrawSliced()
      {
        CalculateFragments();

        for (size_t i = 0; i < 9; i++)
        {
          _tempRect =
          {
            _slices[i].x,
            _slices[i].y,
            _slices[i].w - _slices[i].x,
            _slices[i].h - _slices[i].y
          };

          SDL_RenderCopy(_rendRef,
                          _image,
                          &_tempRect,
                          &_fragments[i]);
        }
      }

      void DrawTiled()
      {
        CalculateSteps();

        Manager::Get().PushClipRect();

        auto old = SDL_GetRenderTarget(_rendRef);
        SDL_SetRenderTarget(_rendRef, Manager::Get()._renderTempTexture);
        SDL_RenderClear(_rendRef);

        _tempRect = { 0, 0, _transform.w, _transform.h };

        SDL_RenderSetClipRect(_rendRef, &_tempRect);

        for (int x = 0; x < _transform.w; x += _stepX)
        {
          for (int y = 0; y < _transform.h; y += _stepY)
          {
            _tempRect = { x, y, _stepX, _stepY };

            SDL_RenderCopy(_rendRef,
                           _image,
                           nullptr,
                           &_tempRect);
          }
        }

        SDL_SetRenderTarget(_rendRef, old);

        Manager::Get().PopClipRect();

        _tempRect = { 0, 0, _transform.w, _transform.h };

        SDL_RenderCopy(_rendRef,
                       Manager::Get()._renderTempTexture,
                       &_tempRect,
                       &_renderTransform);
      }

      DrawType _drawType = DrawType::NORMAL;

      SDL_BlendMode _blendMode = SDL_BLENDMODE_NONE;

      SDL_Texture* _image = nullptr;

      SDL_Rect _imageSrc;
      SDL_Rect _tempRect;
      SDL_Rect _slices[9];
      SDL_Rect _fragments[9];
      SDL_Rect _slicePoints;

      SDL_Color _color;

      std::pair<size_t, size_t> _tileRate;
      std::vector<std::pair<int, int>> _swh;

      int _stepX = 1;
      int _stepY = 1;
  };

  // ===========================================================================
  //                               TEXT
  // ===========================================================================
  class Text : public Element
  {
    public:
      enum class Alignment
      {
        LEFT = 0,
        CENTER,
        RIGHT
      };

      Text(Canvas* owner,
           const SDL_Point& pos,
           const std::string& text)
        : Element(owner, { pos.x, pos.y, 0, 0 })
      {
        _text = text;

        _color = { 255, 255, 255, 255 };
        _scale = 1.0f;

        StoreLines();
        CalculateRenderTransform();
      }

      void SetAlignment(Alignment al)
      {
        _alignment = al;
      }

      void SetColor(const SDL_Color& c)
      {
        _color = c;
      }

      void SetScale(uint8_t scale)
      {
        _scale = scale;
        _scale = Clamp<uint8_t>(_scale, 1, 255);

        CalculateRenderTransform();
      }

      void SetText(const std::string& text)
      {
        _text = text;

        StoreLines();
        CalculateRenderTransform();
      }

      const std::string& GetText()
      {
        return _text;
      }

    protected:
      void DrawImpl() override
      {
        SDL_SetTextureColorMod(Manager::Get()._font,
                               _color.r,
                               _color.g,
                               _color.b);

        Manager::Get().PushClipRect();

        auto old = SDL_GetRenderTarget(_rendRef);
        SDL_SetRenderTarget(_rendRef, Manager::Get()._renderTempTexture);
        SDL_SetTextureBlendMode(Manager::Get()._renderTempTexture,
                                SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(_rendRef, 0, 0, 0, 0);
        SDL_RenderClear(_rendRef);

        _srcTexture = { 0, 0, _transform.w, _transform.h };

        _dstFinal =
        {
          _renderTransform.x,
          _renderTransform.y,
          _transform.w * _scale,
          _transform.h * _scale
        };

        SDL_RenderSetClipRect(_rendRef, &_srcTexture);

        DrawText();

        SDL_SetRenderTarget(_rendRef, old);

        Manager::Get().PopClipRect();

        SDL_RenderCopy(_rendRef,
                       Manager::Get()._renderTempTexture,
                       &_srcTexture,
                       &_dstFinal);
      }

    private:
      void StoreLines()
      {
        _textLines.clear();

        _textMaxStringLen = 0;

        std::stringstream ss;
        for (auto& c : _text)
        {
          if (c == '\n')
          {
            _textLines.push_back(ss.str());

            size_t l = ss.str().length();
            if (l > _textMaxStringLen)
            {
              _textMaxStringLen = l;
            }

            ss.str(std::string());
          }
          else
          {
            ss << c;
          }
        }
      }

      void CalculateRenderTransform()
      {
        auto& t = Transform();

        int w = _textMaxStringLen * Manager::Get().FontW * _scale;
        int h = _textLines.size() * Manager::Get().FontH * _scale;

        SetTransform({ t.x, t.y, w, h });
      }

      void DrawText()
      {
        int offsetX = 0;
        int offsetY = 0;

        auto& fw = Manager::Get().FontW;
        auto& fh = Manager::Get().FontH;

        for (auto& line : _textLines)
        {
          int lineLen = line.length();
          int diff = (_textMaxStringLen - lineLen) * fw;
          int middlePoint = (int)((float)diff * 0.5f);

          for (auto& c : line)
          {
            auto gi = Manager::Get().GetCharData(c);

            _glyphSrc = { gi->X, gi->Y, fw, fh };

            switch (_alignment)
            {
              case Alignment::LEFT:
              {
                _glyphDst =
                {
                  offsetX,
                  offsetY,
                  fw,
                  fh
                };
              }
              break;

              case Alignment::RIGHT:
              {
                _glyphDst =
                {
                  offsetX + diff,
                  offsetY,
                  fw,
                  fh
                };
              }
              break;

              case Alignment::CENTER:
              {
                _glyphDst =
                {
                  offsetX + middlePoint,
                  offsetY,
                  fw,
                  fh
                };
              }
              break;
            }

            SDL_RenderCopy(_rendRef,
                           Manager::Get()._font,
                          &_glyphSrc,
                          &_glyphDst);

            offsetX += fw;
          }

          offsetX = 0;
          offsetY += fh;
        }
      }

      std::string _text;

      std::vector<std::string> _textLines;

      SDL_Color _color;

      SDL_Rect _glyphSrc;
      SDL_Rect _glyphDst;
      SDL_Rect _srcTexture;
      SDL_Rect _dstFinal;

      uint8_t _scale = 1;

      size_t _textMaxStringLen = 0;

      Alignment _alignment = Alignment::LEFT;
  };

  class Button : public Element
  {
    public:
      Button(Canvas* owner,
             const SDL_Rect& transform,
             const std::string& text)
        : Element(owner, transform)
      {
        _text = Manager::Get().CreateText(owner, { transform.x, transform.y }, text);
        _text->SetAlignment(Text::Alignment::CENTER);
        _text->SetColor({ 255, 255, 255, 255 });
      }

    protected:
      void DrawImpl() override
      {
      }

    private:
      Text* _text = nullptr;
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

  Text* Manager::CreateText(Canvas* canvas,
                            const SDL_Point& pos,
                            const std::string& text)
  {
    Canvas* c = (canvas == nullptr) ? _screenCanvas.get() : canvas;
    Text* txt = new Text(c, pos, text);
    return static_cast<Text*>(c->Add(txt));
  }

  // ===========================================================================
  //                             IMPLEMENTATIONS
  // ===========================================================================
  void Manager::Draw()
  {
    SDL_GetRenderDrawColor(_rendRef,
                          &_oldRenderColor.r,
                          &_oldRenderColor.g,
                          &_oldRenderColor.b,
                          &_oldRenderColor.a);
    DrawToTexture();
    DrawOnScreen();

    SDL_SetRenderDrawColor(_rendRef,
                           _oldRenderColor.r,
                           _oldRenderColor.g,
                           _oldRenderColor.b,
                           _oldRenderColor.a);
  }

  void Manager::DrawToTexture()
  {
    auto old = SDL_GetRenderTarget(_rendRef);
    SDL_SetRenderTarget(_rendRef, _renderTexture);
    SDL_SetTextureBlendMode(_renderTempTexture,
                            SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(_rendRef, 0, 0, 0, 0);
    SDL_RenderClear(_rendRef);

    for (auto& kvp : _canvases)
    {
      SDL_RenderSetClipRect(_rendRef, &kvp.second->_renderTransform);
      kvp.second->Draw();
    }

    SDL_SetRenderTarget(_rendRef, _renderTexture);
    SDL_RenderSetClipRect(_rendRef, &_screenCanvas->_renderTransform);
    _screenCanvas->Draw();

    SDL_SetRenderTarget(_rendRef, old);
  }

  void Manager::DrawOnScreen()
  {
    auto old = SDL_GetRenderTarget(_rendRef);
    SDL_SetRenderTarget(_rendRef, nullptr);

    for (auto it = _canvases.rbegin(); it != _canvases.rend(); it++)
    {
      auto& t = it->second->_transform;

      SDL_RenderSetClipRect(_rendRef, &t);
      SDL_RenderCopy(_rendRef,
                     _renderTexture,
                     &_renderDst,
                     nullptr);
    }

    auto& t = _screenCanvas->_transform;

    SDL_RenderSetClipRect(_rendRef, &t);
    SDL_RenderCopy(_rendRef,
                   _renderTexture,
                   &_renderDst,
                   nullptr);

    SDL_RenderSetClipRect(_rendRef, nullptr);
    SDL_SetRenderTarget(_rendRef, old);
  }

  void Manager::HandleEvents(const SDL_Event& evt)
  {
    switch (evt.type)
    {
      case SDL_MOUSEMOTION:
      case SDL_MOUSEBUTTONUP:
      case SDL_MOUSEBUTTONDOWN:
      {
        _screenCanvas->HandleEvents(evt);

        if (_screenCanvas->_topElement == nullptr)
        {
          ProcessCanvases(evt);
        }
      }
      break;
    }
  }

  void Manager::ProcessCanvases(const SDL_Event& evt)
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

    if (_topCanvas != nullptr)
    {
      _topCanvas->HandleEvents(evt);
    }
  }

  // ===========================================================================
  //                              SHORTCUTS
  // ===========================================================================
  void Init(SDL_Window* screenRef)
  {
    Manager::Get().Init(screenRef);
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

  Text* CreateText(Canvas* canvas,
                   const SDL_Point& pos,
                   const std::string& text)
  {
    return Manager::Get().CreateText(canvas, pos, text);
  }

  // ===========================================================================
  //                                  BASE64
  // ===========================================================================

const std::string Manager::_base64Chars =
  "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

const std::string Manager::_pixelImageBase64 =
"Qk2OAAAAAAAAAIoAAAB8AAAAAQAAAAEAAAABABgAAAAAAAQAAAAjLgAAIy4AAAAAAAA"
"AAAAAAAD/AAD/AAD/AAAAAAAAAEJHUnMAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
"AAAAAAAAAAAAAAAAAAAAAAAAAAAAACAAAAAAAAAAAAAAAAAAAA////AA==";

const std::string Manager::_btnNormalBase64 =
"Qk0WAgAAAAAAAIoAAAB8AAAACwAAAAsAAAABABgAAAAAAIwBAAATCwAAEwsAAAAAAAAAAAAAAAD/AAD/"
"AAD/AAAAAAAAAEJHUnMAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
"AAACAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAoKCg"
"oKCgoKCgoKCgoKCgoKCgoKCgoKCgtLS0AAAAAAAAAAAA8PDwoKCgoKCgoKCgoKCgoKCgoKCgtLS0tLS0"
"AAAAAAAAAAAA8PDw8PDwoKCgoKCgoKCgoKCgtLS0tLS0tLS0AAAAAAAAAAAA8PDw8PDw8PDwyMjIyMjI"
"yMjItLS0tLS0tLS0AAAAAAAAAAAA8PDw8PDw8PDwyMjIyMjIyMjItLS0tLS0tLS0AAAAAAAAAAAA8PDw"
"8PDw8PDwyMjIyMjIyMjItLS0tLS0tLS0AAAAAAAAAAAA8PDw8PDw8PDw8PDw8PDw8PDw8PDwtLS0tLS0"
"AAAAAAAAAAAA8PDw8PDw8PDw8PDw8PDw8PDw8PDw8PDwtLS0AAAAAAAAAAAA8PDw8PDw8PDw8PDw8PDw"
"8PDw8PDw8PDw8PDwAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA";

const std::string Manager::_btnPressedBase64 =
"Qk0WAgAAAAAAAIoAAAB8AAAACwAAAAsAAAABABgAAAAAAIwBAAATCwAAEwsAAAAAAAAAAAAAAAD/AAD/"
"AAD/AAAAAAAAAEJHUnMAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
"AAACAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAoKCg"
"jIyMjIyMjIyMjIyMjIyMjIyMjIyMjIyMAAAAAAAAAAAAoKCgoKCgjIyMjIyMjIyMjIyMjIyMjIyMjIyM"
"AAAAAAAAAAAAoKCgoKCgoKCgjIyMjIyMjIyMjIyMjIyMjIyMAAAAAAAAAAAAoKCgoKCgoKCgtLS0tLS0"
"tLS0jIyMjIyMjIyMAAAAAAAAAAAAoKCgoKCgoKCgtLS0tLS0tLS0jIyMjIyMjIyMAAAAAAAAAAAAoKCg"
"oKCgoKCgtLS0tLS0tLS0jIyMjIyMjIyMAAAAAAAAAAAAoKCgoKCgoKCgoKCgoKCgoKCgoKCgjIyMjIyM"
"AAAAAAAAAAAAoKCgoKCgoKCgoKCgoKCgoKCgoKCgoKCgjIyMAAAAAAAAAAAAoKCgoKCgoKCgoKCgoKCg"
"oKCgoKCgoKCgoKCgAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA";

const std::string Manager::_btnHoverBase64 =
"Qk0WAgAAAAAAAIoAAAB8AAAACwAAAAsAAAABABgAAAAAAIwBAAATCwAAEwsAAAAAAAAAAAAAAAD/AAD/"
"AAD/AAAAAAAAAEJHUnMAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
"AAACAAAAAAAAAAAAAAAAAAAAAP//AP//AP//AP//AP//AP//AP//AP//AP//AP//AP//AAAAAP//oKCg"
"oKCgoKCgoKCgoKCgoKCgoKCgoKCgtLS0AP//AAAAAP//8PDwoKCgoKCgoKCgoKCgoKCgoKCgtLS0tLS0"
"AP//AAAAAP//8PDw8PDwoKCgoKCgoKCgoKCgtLS0tLS0tLS0AP//AAAAAP//8PDw8PDw8PDwyMjIyMjI"
"yMjItLS0tLS0tLS0AP//AAAAAP//8PDw8PDw8PDwyMjIyMjIyMjItLS0tLS0tLS0AP//AAAAAP//8PDw"
"8PDw8PDwyMjIyMjIyMjItLS0tLS0tLS0AP//AAAAAP//8PDw8PDw8PDw8PDw8PDw8PDw8PDwtLS0tLS0"
"AP//AAAAAP//8PDw8PDw8PDw8PDw8PDw8PDw8PDw8PDwtLS0AP//AAAAAP//8PDw8PDw8PDw8PDw8PDw"
"8PDw8PDw8PDw8PDwAP//AAAAAP//AP//AP//AP//AP//AP//AP//AP//AP//AP//AP//AAAA";

const std::string Manager::_fontBase64 =
"Qk2KkAAAAAAAAIoAAAB8AAAAgAAAAGAAAAABABgAAAAAAACQAAAjLgAAIy4AAAAAAAAAAAAAAAD/AAD/"
"AAD/AAAAAAAAAEJHUnMAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
"AAACAAAAAAAAAAAAAAAAAAAA/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//////////////////wD//wD/"
"/wD//wD//wD//wD//wD//////////////////wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//////////////////////wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//////////wD//wD//wD//wD//wD//wD//wD//wD//wD//////////wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/////////"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//////////wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//////////wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//////////wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/////"
"/////////////////wD//wD//wD//////////////////////wD//wD//////////////////wD//wD/"
"/wD//wD//wD//////////////////////wD//wD//wD//wD//wD//////////////wD//wD//wD/////"
"/////////wD//////////wD//wD//wD//wD//////////wD//wD//wD//wD//////////wD//wD/////"
"/////wD//////////wD//wD//wD//wD//////////wD//////////////////////////wD/////////"
"/////////////////////wD//wD//wD//wD//wD//////////////wD//wD//wD//wD//////////wD/"
"/wD//wD//wD//////////////wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//////////wD//wD//////////wD//////////wD//wD/////////"
"/wD//wD//wD//////////wD//wD//wD//wD//wD//////////wD//wD//wD//////////wD//wD//wD/"
"/////////wD//////////wD//////////wD//wD//////////wD//wD//wD//wD/////////////////"
"/wD//wD//////////////////////////////////wD//////////wD//wD//////////wD/////////"
"/wD//wD//wD//////////wD//////////wD//wD//wD//////////wD//wD//wD//wD//////////wD/"
"/wD//wD//wD//wD//wD//////////wD//wD//wD//wD//wD//wD//////////wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//////////////////////////////wD//wD//////////wD//wD/////"
"/////wD//////////wD//wD//////////wD//wD//wD//////////wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//////////wD//wD//wD//wD//////////wD//wD//wD//wD//////////wD//wD/////////"
"/wD//wD//wD//////////wD//wD//////////wD//////////wD//////////wD//////////wD//wD/"
"/////////////////wD//wD//////////wD//wD//wD//////////wD//wD//////////wD//wD//wD/"
"/wD//wD//wD//wD//wD//////////wD//wD//wD//wD//wD//wD//////////wD//wD//wD//wD//wD/"
"/wD//////////wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//////////wD//wD//wD/////"
"/////wD//wD//////////wD//wD//////////wD//////////wD//wD//////////wD//wD//wD/////"
"/////wD//wD//wD//wD//wD//wD//wD//////////////wD//wD//wD//wD//wD//////////wD//wD/"
"/wD//wD//////////wD//wD//////////wD//wD//////////wD//wD//wD//wD/////////////////"
"/wD//////////wD//////////wD//wD//wD//////////wD//wD//wD//////////wD//wD//wD/////"
"/////wD//wD//wD//////////wD//wD//wD//wD//wD//wD//wD//////////wD//wD//wD//wD//wD/"
"/wD//////////wD//wD//wD//wD//wD//wD//////////wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//////////wD//wD//wD//////////wD//wD//////////wD//wD//////////wD/////////"
"/wD//wD//////////wD//wD//wD//////////wD//wD//////////wD//wD//////////wD//wD//wD/"
"/wD//wD//wD//wD//////////wD//wD//wD//wD//////////wD//wD//////////wD//wD/////////"
"/wD//wD//wD//wD//////////////////wD//wD//wD//wD//////////wD//wD/////////////////"
"/wD//wD//////////wD//wD//wD//////////wD//wD//wD//wD//////////wD//wD//wD//wD//wD/"
"/wD//////////wD//wD//wD//wD//wD//wD//////////wD//wD//wD//wD//wD//wD//////////wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//////////wD//wD//wD//////////wD//wD/////"
"/////wD//wD//////////wD//////////wD//wD//////////wD//wD//wD//////////////wD/////"
"/////wD//////////wD//wD//wD//////////wD//wD//wD//////////wD//wD//wD//wD/////////"
"/wD//wD//////////wD//wD//////////wD//wD//wD//wD//////////////////wD//wD//wD//wD/"
"/////////wD//////////wD//wD//////////wD//////////wD//wD//wD//////////wD/////////"
"/wD//wD//////////wD//wD//wD//////////////wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//////////////wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/////"
"/////wD//////////wD//wD//////////wD//////////////wD//wD//wD//////////////wD/////"
"/////wD//////////wD//////////////wD//wD//wD//////////////////////wD//wD/////////"
"/////////////////wD//wD//////////wD//wD//////////wD//wD//////////wD//wD//wD//wD/"
"/////////////////wD//wD//wD//wD//////////////////wD//wD//wD//wD/////////////////"
"/wD//wD//wD//////////wD//////////////////////////////wD//wD//wD//wD//////////wD/"
"/wD//wD//wD//wD//wD//////////wD//wD//wD//wD//wD//wD//////////wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//////////////wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//////////wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//////////wD//wD//wD//wD//wD//wD//////////wD//wD//wD//wD//wD/"
"/wD//////////wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//////wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//////////wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//////////wD//wD//wD//wD//wD/"
"/wD//////////wD//wD//wD//wD//wD//wD//////////wD//wD//wD//////////wD/////////////"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//////wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//////////////wD//wD//wD//wD//////////wD//wD//wD//wD//////////////wD//wD/"
"/wD//wD//wD//////////////wD//////////wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/////"
"/////////////wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//////////////////wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//////////wD//wD//////////wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//////////wD//wD//////////wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/////////"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/////"
"/////wD//wD//////////wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//////////////wD/////"
"/////wD//wD//////////////////////wD//wD//wD//////////////////////wD//wD//wD/////"
"/////////wD//////////wD//wD//////////////////////wD//wD//////////////////wD//wD/"
"/wD//wD//wD//////////////////////wD//wD//////////////wD//wD//////////wD//wD//wD/"
"/////////////////wD//wD//wD//wD//wD//wD//wD//////////wD//////////////wD//wD/////"
"/////wD//wD//wD//////////////////wD//wD//////////wD//////////wD//////////wD/////"
"/////wD//wD//////////wD//wD//////////////////////wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//////////wD//wD//////////wD//wD//wD//////////wD//wD//////////wD/////////"
"/wD//wD//wD//////////wD//////////wD//wD//////////wD//wD//////////wD//wD//wD/////"
"/////wD//wD//////////wD//wD//wD//wD//wD//////////wD//wD//////////wD//wD//wD/////"
"/////wD//wD//////////wD//wD//wD//wD//////////wD//wD//wD//wD//wD//wD//wD//wD/////"
"/////wD//wD//////////wD//wD//////////wD//wD//wD//wD//////////wD//wD//wD/////////"
"/wD//////////wD//////////wD//////////wD//wD//////////wD//////////wD//wD//wD/////"
"/////wD//wD//wD//wD//wD//wD//wD//wD//wD//////////wD//wD//////////wD//wD//wD/////"
"/////wD//wD//////////wD//////////wD//wD//wD//wD//wD//wD//////////wD//wD/////////"
"/wD//wD//////////wD//wD//wD//wD//wD//wD//wD//////////wD//wD//wD//wD//wD/////////"
"/wD//wD//////////wD//wD//wD//////////wD//wD//////////wD//wD//wD//wD//////////wD/"
"/wD//wD//wD//wD//wD//wD//wD//////////wD//wD//////////wD//////////wD//wD//wD//wD/"
"/wD//////////wD//wD//wD//////////wD//////////wD//////////wD//////////wD//wD/////"
"/////wD//////////wD//wD//wD//////////wD//wD//wD//wD//wD//wD//wD//wD//wD/////////"
"/wD//wD//////////wD//wD//wD//////////wD//wD//////////wD//////////wD//wD//wD//wD/"
"/wD//wD//////////wD//wD//////////wD//wD//////////wD//wD//wD//wD//wD//wD//wD/////"
"/////wD//wD//wD//wD//wD//////////wD//wD//////////wD//wD//wD//////////wD//wD/////"
"/////wD//wD//wD//wD//////////wD//wD//wD//wD//wD//wD//wD//wD//////////wD//wD/////"
"/////////////wD//wD//wD//wD//wD//wD//////////wD//wD//wD//////////wD//////////wD/"
"/////////wD//////////wD//wD//////////wD//////////wD//wD//wD//////////wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//////////////////////wD//wD//wD//////////wD//wD/////"
"/////wD//////////wD//wD//wD//wD//wD//wD//////////wD//wD//////////wD//wD/////////"
"/////////////////////wD//wD//////////wD//wD//wD//wD//wD//////////wD//wD/////////"
"/wD//wD//wD//////////wD//wD//////////wD//wD//wD//wD//////////wD//wD//wD//wD//wD/"
"/wD//wD//wD//////////wD//wD//////////////////wD//wD//wD//wD//wD//wD//////////wD/"
"/wD//wD//////////wD//////////wD//////////wD//////////wD//wD//////////wD/////////"
"/wD//wD//wD//////////wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/////////"
"/wD//wD//wD//////////wD//////////wD//wD//////////wD//wD//wD//////////wD//wD/////"
"/////wD//////////wD//wD//////////wD//wD//wD//////////wD//////////////////wD//wD/"
"/wD//wD//////////wD//wD//////////wD//wD//wD//////////////wD//////////wD//wD//wD/"
"/wD//////////wD//wD//wD//wD//wD//wD//wD//wD//////////wD//wD//////////wD/////////"
"/wD//wD//wD//wD//wD//////////wD//wD//wD//////////////////////////////////wD/////"
"/////wD//wD//////////wD//////////wD//wD//wD//////////wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//////////////////wD//wD//wD//wD//////////////////wD//wD//wD//wD/////"
"/////////////////wD//wD//wD//wD//////////////////wD//wD//wD/////////////////////"
"/wD//wD//wD//////////wD//wD//wD//wD//wD//wD//////////////wD//////////wD//wD/////"
"/////wD//////////wD//wD//wD//wD//////////////wD//wD//wD//wD//wD//wD//wD/////////"
"/////wD//wD//////////wD//wD//////////wD//wD//wD//wD//////////wD//wD//wD/////////"
"/////wD//wD//////////wD//////////wD//////////////wD//wD//wD/////////////////////"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/////"
"/////wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/////////"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//////////wD//wD//////wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//////////wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//////////wD//wD//wD//wD//wD//wD//wD/"
"/wD//////////wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//////////wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//////////wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/////"
"/////wD//////////wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//////////wD//wD//wD/"
"/wD//wD//wD//wD//wD//////////wD//wD//wD//wD//wD//wD//wD//wD//////////wD//wD/////"
"/////wD//wD//wD//wD//wD//wD//wD//wD//////////wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//////////wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//////////////wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//////////////wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//////////////wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//////////////wD//wD//wD//wD//wD//wD//wD//wD//////////wD//wD//wD//wD//wD/"
"/wD//wD//wD//////////wD//////////////wD//wD//wD//wD//wD//wD//wD//////////////wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//////////wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//////////wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//////////////wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/////////"
"/////////////////////////wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/////////"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//////////////////wD//wD/"
"/wD//wD//wD//////////////////////wD//wD//////////////wD//wD//////////wD//wD/////"
"/////////////////wD//wD//wD//wD//////////////////wD//wD//wD/////////////////////"
"/wD//wD//wD//wD//wD//////////wD//wD//wD//wD//////////wD//wD//////////wD/////////"
"/wD//wD//wD//wD//////////wD//wD//////////////////wD//wD/////////////////////////"
"/////////wD//wD//////////////////wD//wD//wD//wD//wD//wD//wD//wD//////wD//wD//wD/"
"/////////////////wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//////////wD//wD//wD//wD//wD//////////wD//////////////////wD//wD/////"
"/////wD//wD//////////wD//////////wD//wD//wD//////////wD//wD//wD//wD//////////wD/"
"/wD//wD//////////wD//wD//wD//////////wD//wD//wD//////////////////wD//wD//wD/////"
"/////wD//wD//////////wD//////////wD//wD//wD//wD//////////wD//wD//wD//////////wD/"
"/wD//wD//////////wD//wD//wD//wD//////////wD//wD//////////wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//////////wD//wD//wD//wD//wD//////////wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//////////wD//wD//wD//wD//wD/////////"
"/wD//////wD//////////wD//wD//////////wD//wD//////////wD//////////wD//wD//wD/////"
"/////wD//wD//wD//wD//////////wD//wD//wD//////////wD//wD//wD//////////wD//wD/////"
"/////wD//wD//////////wD//////////////////////////////////wD//////////wD//wD/////"
"/////wD//wD//wD//wD//////////wD//wD//wD//////////wD//wD//wD//wD//wD//////wD//wD/"
"/////////wD//wD//wD//wD//wD//wD//wD//wD//////////////wD//wD//wD//wD//wD/////////"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/////"
"/////wD//wD//wD//wD//wD//////////wD//wD//wD//////////wD//wD//////////wD//wD/////"
"/////wD//wD//wD//wD//wD//wD//////////wD//wD//wD//wD//////////wD//wD//wD/////////"
"/wD//wD//wD//////////wD//////////wD//wD//wD//wD//////////////////wD//////////wD/"
"/////////wD//wD//////////////////wD//wD//wD//wD//wD//////////wD//wD//wD//wD/////"
"/////wD//wD//wD//wD//wD//wD//wD//////////wD//wD//wD//wD//wD//wD//wD/////////////"
"/wD//wD//wD//wD//wD//wD//////////wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//////////wD//wD//wD//wD//wD//////////wD//wD//wD/////"
"/////wD//wD//////////wD//////////wD//wD//wD//wD//wD//wD//////////wD//wD//wD//wD/"
"/wD//////////wD//wD//wD//////////wD//wD//wD//////////wD//////////wD//wD//wD//wD/"
"/////////////////wD//////////wD//////////wD//wD//wD//////////wD//wD//wD//wD//wD/"
"/wD//////////wD//wD//wD//wD//wD//////////wD//wD//wD//wD//wD//wD//////////wD//wD/"
"/wD//wD//wD//wD//////////////wD//wD//wD//wD//wD//wD//wD//////////wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/////////////////////"
"/wD//wD//////////wD//wD//wD//////////wD//wD//////////////////////wD//wD//wD//wD/"
"/////////////wD//wD//wD//wD//wD//wD//////////wD//wD//wD//////////wD//wD//wD/////"
"/////wD//////////wD//wD//wD//wD//////////////////wD//wD//wD//wD//////////wD//wD/"
"/wD//////////wD//wD//wD//wD//wD//////////////////wD//wD//wD//wD//wD//////////wD/"
"/wD//wD//wD//wD//////////wD//wD//wD//wD//wD//////////////wD//wD//wD//wD//wD//wD/"
"/wD//wD//////////wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//////////wD//wD//////////wD//////////wD//wD//wD//////////wD//wD/////"
"/////wD//wD//////////wD//wD//////////wD//wD//wD//wD//wD//wD//wD//wD//////////wD/"
"/wD//wD//////////wD//wD//wD//////////wD//////////wD//wD//wD//wD/////////////////"
"/wD//wD//wD//wD//////////wD//wD//////////////////wD//wD//wD//////////wD//wD/////"
"/////wD//wD//wD//wD//wD//////////wD//wD//wD//wD//////////wD//wD//wD//wD/////////"
"/////wD//wD//wD//wD//wD//wD//wD//wD//wD//////////wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//////////wD//wD//////////wD/////////"
"/wD//wD//wD//////////wD//wD//////////wD//wD//////////wD//////////wD//wD//wD/////"
"/////wD//////wD//wD//////////wD//wD//////////////wD//wD//wD//////////wD/////////"
"/wD//wD//wD//wD//////////////////wD//wD//wD//wD//////////wD//////////wD//wD/////"
"/////wD//////////wD//wD//wD//wD//////////////wD//wD//wD//wD//////////wD//wD//wD/"
"/////////wD//wD//wD//wD//////////wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/////////"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/////"
"/////wD//wD//////////wD//////////wD//wD//wD//////////wD//wD//////////wD//wD/////"
"/////wD//////////wD//wD//wD//////////wD//////////wD//////////wD/////////////////"
"/wD//wD//wD//////////wD//////////wD//wD//wD//wD//////////////////wD//wD//wD//wD/"
"/////////////////wD//wD//wD//wD//////////////////wD//wD//wD//wD/////////////////"
"/wD//wD//wD//wD//////////wD//wD//////////wD//wD//wD//wD//////wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//////////wD//wD//////////wD//wD//wD//////////wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//////////////////////////wD//wD//wD/////////////////////"
"/wD//wD//////////////////////////wD//wD//wD//////////////////////wD//wD/////////"
"/////////////////////////////////wD//wD//wD//////////wD//////////wD//wD//wD//wD/"
"/////////////////wD//wD//wD//wD//////////////////wD//wD//wD//wD/////////////////"
"/wD//wD//wD//wD//////////////////////////////////////////wD//wD/////////////////"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//////////////////wD//wD//wD/////"
"/////wD//////////wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//////////////wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//////wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//////////////////////wD//wD//////////wD//wD//wD//////////wD/////////"
"/////////////////wD//wD//wD//wD//////////////////wD//wD//////////////////////wD/"
"/wD//wD//////////////////////////////wD//////////////////wD//wD//wD//wD//wD//wD/"
"/////////////wD//////wD//////////wD//wD//wD//////////wD//wD//wD/////////////////"
"/wD//wD//wD//////////////////wD//wD//wD//////////////wD//wD//////////wD/////////"
"/////////////////////wD//////////wD//wD//wD//wD//////////////////wD//wD//wD/////"
"/////wD//wD//////////////////////wD//wD//////////wD//wD//wD//wD//wD//wD/////////"
"/wD//wD//wD//////////wD//wD//////////wD//wD//////////wD//wD//////////wD//wD/////"
"/////wD//wD//////////wD//////////wD//wD//wD//////////wD//wD//////////wD//wD/////"
"/////wD//wD//wD//wD//wD//wD//////////wD//wD//////////wD//////////wD//wD//wD/////"
"/////wD//wD//wD//wD//////////wD//wD//wD//////////wD//wD//////////wD//wD//wD/////"
"/////wD//wD//////////wD//wD//////////wD//wD//////////wD//////////wD//wD//wD//wD/"
"/////////////////wD//wD//wD//////////wD//////////wD//wD//wD//////////wD/////////"
"/wD//////////////wD//wD//////////wD//wD//wD//////////wD//wD//////////wD//wD/////"
"/////wD//////////wD//wD//wD//wD//////wD//wD//////////wD//wD//////////wD//wD/////"
"/////wD//wD//wD//////wD//wD//////////wD//wD//wD//wD//wD//////////wD//wD//wD/////"
"/////wD//////////wD//wD//wD//////////wD//wD//wD//wD//////////wD//wD//wD/////////"
"/wD//wD//////////wD//wD//wD//////////wD//wD//////////wD//wD//////////wD//wD//wD/"
"/////wD//////////wD//wD//wD//wD//////////////////wD//wD//wD//////////wD/////////"
"/wD//wD//wD//////////wD//////////wD//////////////////wD//////////wD//wD//wD/////"
"/////wD//wD//////////wD//wD//////////wD//////////wD//wD//wD//wD//wD//wD//wD/////"
"/////wD//wD//////////wD//wD//////////wD//wD//wD//wD//wD//wD//////////wD//wD//wD/"
"/wD//wD//////////wD//wD//wD//////////wD//////////wD//wD//wD//////////wD//wD//wD/"
"/wD//////////wD//wD//wD//////////wD//wD//////////wD//wD//wD//////////wD/////////"
"/wD//wD//wD//////////wD//wD//wD//wD//wD//////////wD//wD//wD//wD/////////////////"
"/wD//wD//wD//////////wD//////////wD//wD//wD//////////wD//////////wD/////////////"
"/////wD//////////////////////////////wD//wD//////////wD//wD//////////wD/////////"
"/wD//wD//wD//wD//wD//wD//wD//////////wD//wD//////////wD//wD//////////wD//////wD/"
"/wD//wD//wD//////////wD//////wD//wD//wD//////////wD//////////////////wD/////////"
"/wD//wD//wD//////////wD//wD//wD//wD//////////wD//wD//wD//wD//wD//wD//wD/////////"
"/wD//wD//wD//////////////////wD//wD//wD//wD//////////wD//wD//wD//wD//wD/////////"
"/wD//wD//wD//wD//////////////////wD//wD//////////////wD//////////wD//wD//wD/////"
"/////wD//////////wD//////////////////wD//////////wD//wD//wD//////////wD//wD/////"
"/////////////////wD//wD//////////wD//wD//wD//wD//wD//wD//wD//////////wD//wD/////"
"/////wD//wD//////////////////wD//wD//wD//wD//////////////////wD//wD//wD/////////"
"/wD//wD//wD//wD//wD//wD//////////////////////////////wD//wD//wD//wD//////////wD/"
"/wD//wD//wD//wD//wD//wD//////////wD//wD//wD//////////////////wD//wD//wD//wD/////"
"/////wD//wD//wD//wD//wD//////////wD//////////wD//////////////////wD/////////////"
"/////wD//////////wD//wD//wD//////////wD//////////wD//wD//wD//////////wD/////////"
"/wD//wD//wD//////////wD//wD//////////wD//wD//////////wD//////////wD//wD//wD//wD/"
"/wD//wD//wD//////////wD//wD//////////wD//wD//////////wD//////wD//wD//wD//wD/////"
"/////wD//////wD//wD//wD//////////wD//wD//wD//wD//wD//wD//////////wD//wD//wD/////"
"/////wD//wD//wD//wD//////////wD//wD//wD//wD//wD//wD//wD//////////wD//wD//wD/////"
"/////wD//////////wD//wD//wD//////////wD//wD//wD//wD//wD/////////////////////////"
"/////////////////////////////////////wD//////////wD//wD//wD//////////wD/////////"
"/wD//wD//wD//////////wD//wD//////////wD//////////wD//wD//wD//////////wD//wD/////"
"/////wD//////////wD//wD//wD//wD//////wD//wD//////////wD//wD//////////wD//wD/////"
"/////wD//wD//wD//////wD//wD//////////wD//wD//wD//////wD//////////wD//wD//wD//wD/"
"/////wD//////////wD//wD//wD//////////wD//wD//wD//wD//////////wD//wD//wD//wD//wD/"
"/wD//wD//////////wD//wD//wD//////////wD//wD//////////wD//wD//////////wD//wD//wD/"
"/wD//wD//////////////////////////////////////////////////wD//////////wD/////////"
"/wD//wD//wD//////////wD//////////wD//wD//wD//////////wD//wD//wD//////////////wD/"
"/wD//wD//wD//////////wD//wD//////////wD//wD//////////wD//wD//////////wD//wD/////"
"/////wD//////////wD//wD//wD//////////wD//wD//////////wD//wD//////////wD//wD/////"
"/////wD//wD//////////wD//wD//////////wD//////////wD//wD//wD//////////wD//wD//wD/"
"/wD//////////wD//wD//wD//wD//wD//wD//wD//////////wD//wD//wD//////////wD//wD/////"
"/////wD//wD//////////wD//wD//wD//wD//wD//////////////wD//wD/////////////////////"
"/////wD//wD//////////wD//////////wD//wD//wD//////////wD//wD/////////////////////"
"/wD//wD//wD//wD//wD//////wD//wD//wD//wD//////////////////////////wD//wD//wD//wD/"
"/////////////////wD//wD//////////////////////wD//wD//wD/////////////////////////"
"/////wD//////////////////////////////wD//wD//wD//////////////////wD//wD/////////"
"/wD//wD//wD//////////wD//wD//wD//////////////////wD//wD//wD//wD//wD/////////////"
"/////wD//////////////wD//wD//////////wD//////////////////wD//wD//wD//wD/////////"
"/wD//wD//wD//wD//////////////////wD//wD//wD//////////wD//wD/////////////////////"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//////////////////////wD//wD//wD/////"
"/////////////////////wD//////////////////////////////wD//wD/////////////////////"
"/wD//wD//wD//wD//wD//////////////////wD//wD//////////////////////wD//wD//wD/////"
"/////////////////wD//wD//wD//wD//////////wD//wD//wD//wD//wD/////////////////////"
"/wD//wD//wD//////////////////wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/////////wD//wD//wD//wD//wD//wD//wD//wD//wD//////////wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//////////wD//wD//wD//wD//wD//wD//wD//wD//////////wD//wD//wD/////////"
"/wD//wD//wD//////////wD//wD//wD//wD//////////wD//wD//wD//////////wD//wD//wD/////"
"/////wD//////////wD//wD//wD//////////wD//wD//wD//wD//wD//////////wD//wD/////////"
"/wD//wD//wD//////////wD//////////wD//wD//wD//////////wD//wD//wD//////////wD//wD/"
"/wD//wD//////////wD//wD//wD//////////wD//wD//wD//wD//wD//////////wD//wD//wD//wD/"
"/wD//////////wD//wD//wD//wD//wD//wD//////////wD//wD//wD//wD//wD//wD//wD/////////"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//////////wD//wD//wD//wD//wD//wD/"
"/wD//////////wD//wD//wD//////////wD//wD//wD//////////wD//wD//wD//wD//////////wD/"
"/wD//wD//////////wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//////////wD//wD//wD/"
"/wD//wD//////////wD//wD//wD//wD//wD//wD//wD//////////wD//////////wD//wD//wD/////"
"/////wD//wD//wD//////////wD//wD//wD//wD//////////wD//wD//wD//////////wD//wD//wD/"
"/wD//wD//wD//////////wD//wD//wD//wD//////////wD//wD//wD//wD//wD//wD//////////wD/"
"/wD//wD//wD//wD//wD//////////wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//////////wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//////////////wD//wD/////"
"/////wD//wD//wD//wD//////////wD//wD//wD//wD//////////wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//////////wD//wD//wD//wD//wD//////////wD//wD//wD//wD//wD//wD//wD/////"
"/////wD//////////wD//wD//wD//////////wD//wD//wD//////////wD//wD//wD//wD/////////"
"/wD//wD//wD//////////wD//wD//wD//wD//wD//wD//////////wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//////////wD//wD//wD//wD//wD/////"
"/////////////////////wD//wD//wD//wD//wD//////////wD//wD//wD//wD//wD//////////wD/"
"/wD//wD//////////////////wD//////////wD//wD//wD//wD//////////wD//wD//wD//wD//wD/"
"/////////wD//wD//wD//wD//wD//wD//wD//wD//wD//////////wD/////////////////////////"
"/////wD//wD//wD//wD//wD//wD//////////wD//////////wD//wD//wD//////////wD//wD//wD/"
"/wD//////////wD//wD//wD//////////wD//wD//wD//////////wD//wD//wD//wD//wD//wD/////"
"/////wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/////"
"/////wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/////"
"/////wD//wD//wD//wD//////////wD//wD//wD//////////wD//////////////////wD//wD//wD/"
"/wD//////////wD//wD//wD//wD//wD//wD//////////wD//wD//wD//wD//wD/////////////////"
"/wD//wD//////////wD//wD//////////wD//wD//////////////////////////wD//wD/////////"
"/////////////////wD//wD//wD//wD//wD//wD//////////wD//wD//wD/////////////////////"
"/wD//wD//wD//////////////////////////wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//////////wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//////////wD//wD//wD//wD//wD//////////wD//wD//wD/////////"
"/wD//wD//////////////wD//wD//wD//wD//////////wD//wD//wD//wD//wD//wD//wD/////////"
"/wD//wD//wD//wD//wD//wD//wD//////////wD//wD//////////wD//////////wD//wD/////////"
"/wD//wD//wD//wD//wD//wD//////////wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/////"
"/////wD//////////wD//wD//wD//////////wD//////////wD//wD//wD//////////wD//wD//wD/"
"/wD//////////wD//wD//wD//wD//wD//wD//////////wD//wD//wD//wD//wD//wD//////////wD/"
"/wD//wD//wD//////////////////////////wD//wD//wD//wD//////////wD//wD//wD//wD//wD/"
"/wD//wD//////////wD//wD//////////wD//wD//wD//////////wD//wD//////////////////wD/"
"/wD//wD//wD//wD//wD//wD//wD//////////wD//wD//wD//wD//wD//wD//////////wD//wD//wD/"
"/////////////////wD//wD//////////wD//wD//wD//wD//wD//wD//////////wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//////////wD//////////wD//wD//wD//////////wD/////////"
"/wD//wD//wD//////////wD//wD//wD//wD//////////wD//wD//wD//wD//wD//wD//////////wD/"
"/wD//wD//wD//wD//wD//wD//////////wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/////////wD//wD//wD//wD//////////wD//wD//wD//////////wD//////////wD//wD//wD/////"
"/////wD//wD//wD//////////////wD//wD//wD//////////wD//wD//wD//////////wD/////////"
"/wD//wD//wD//////////wD//wD//wD//wD//////////////wD//wD//////////wD//wD//wD//wD/"
"/wD//wD//wD//////////wD//wD//wD//wD//wD//////////wD//wD//wD//////////wD/////////"
"/wD//wD//wD//////////wD//////////wD//wD//wD//////////wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//////////wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//////////wD//wD//wD//wD//wD//////////wD//wD//wD/////"
"/////wD//wD//////////////////////wD//wD//wD//wD//wD//////////wD//wD//wD//wD/////"
"/////////////////wD//wD//wD//////////////////////wD//wD//wD//wD//wD//wD/////////"
"/wD//wD//////////////////////////////wD//wD//wD//////////////wD//wD//wD/////////"
"/////////////////////wD//wD//////////////////////wD//wD//wD/////////////////////"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//////////////////////wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//////////wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//////////wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//////////wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//////////wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//////////wD//////////wD//wD//wD//////////////////////wD//wD//////wD/"
"/wD//wD//wD//////////wD//wD//////////////wD//////////wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//////////wD//wD//wD//wD//////////wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//////////wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//////////wD//wD//wD//////wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//////////wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//////////wD//////////wD//wD/////////"
"/wD//wD//wD//////////wD//////////wD//wD//wD//////////wD//////////wD//wD/////////"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//////////wD//wD//wD//wD//wD/"
"/wD//////////wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//////////wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//////////wD//wD//wD//////////wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/////////"
"/////////////////////wD//////wD//wD//wD//wD//////////wD//wD//////////wD//wD//wD/"
"/wD//wD//////////wD//wD//////////wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/////////wD//wD//wD//wD//wD//wD//wD//wD//////////wD//wD//wD//////////wD//wD/////"
"/////wD//wD//wD//wD//////////wD//wD//wD//wD//wD//wD//////////wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//////////wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//////////wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//////////wD//////////wD//wD//wD//wD//wD//wD//wD/////"
"/////wD//wD//wD//////////wD//wD//wD//wD//////////wD//wD//////////wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//////////wD//wD//wD//wD//wD//wD//wD//wD/////////"
"/wD//wD//wD//wD//////////////////wD//wD//wD//wD//wD//////////wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//////////wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//////////wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//////////wD/////////"
"/wD//wD//wD//wD//wD//wD//wD//////////wD//wD//wD//wD//////////wD//wD//wD/////////"
"/wD//////////////wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//////////wD//wD/"
"/wD//wD//wD//wD//wD//wD//////////wD//wD//////////////////////////////////wD/////"
"/////////////////////wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/////////////////////"
"/////wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//////////wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//////////wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//////////wD//////////wD//wD//wD//////////////////////wD//wD//wD//wD/"
"/wD//wD//////////wD//wD//wD//////////////wD//////////wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//////////wD//wD//wD//wD//wD//wD//wD//wD//////////wD//wD//wD//wD/"
"/////////////////wD//wD//wD//wD//wD//////////wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//////////wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/////////////////"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//////////////////////////////wD/////////"
"/wD//wD//wD//wD//wD//wD//////////wD//wD//wD//////////wD//wD//wD//////////////wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//////////wD//wD//wD//wD//wD//wD/"
"/wD//wD//////////wD//wD//wD//////////wD//wD//////////wD//wD//wD//wD//////////wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//////////wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//////////////////wD//wD//wD//wD//////wD//wD//////wD//wD//wD/////"
"/////wD//////////wD//wD//////////wD//wD//wD//wD//////wD//////////wD//wD//wD//wD/"
"/////wD//wD//////////wD//////////wD//wD//wD//////////wD//wD//wD//wD//wD//wD//wD/"
"/////////wD//wD//wD//wD//wD//wD//wD//wD//////////wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/////wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//////////////////wD//wD//wD/////"
"/////wD//wD//////////wD//wD//////////wD//////////wD//wD//////////wD//wD//wD/////"
"/////wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//////////wD//////////wD//wD//wD//wD/"
"/////////wD//wD//wD//wD//wD//wD//wD//////////wD//wD//wD//wD//wD//wD//////////wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//////////wD//wD//wD//wD//////////wD//wD//////////wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//////////////////////wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/////////////wD//wD//wD//wD//wD//////////wD//wD//wD//wD//wD//wD//wD//wD/////////"
"/wD//wD//wD//wD//////////wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//////////wD//wD/////"
"/////wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//////////wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//////////wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//////////wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/"
"/wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD//wD/";
}

#endif // REPAUI_H
