#pragma once

#include<SDL2/SDL.h>
#include<map>
#include<tuple>

class SDLWindow{
  public:
    using Callback              = void(*)(void*);
    using EventCallback         = void(*)(SDL_Event,void*);
    SDLWindow(uint32_t width = 1024,uint32_t heigth = 768);
    ~SDLWindow();
    void mainLoop();
    void setIdle(Callback fce = nullptr,void* data = nullptr);
    void setEvent(uint32_t type,EventCallback fce = nullptr,void* data = nullptr);
    void setSize(uint32_t width,uint32_t heght);
    uint32_t getWidth()const;
    uint32_t getHeight()const;
  protected:
    SDL_Window*   _window ;
    SDL_GLContext _context;
    using CallbackWithData      = std::tuple<Callback,void*>;
    using EventCallbackWithData = std::tuple<EventCallback,void*>;
    enum CallbackParts{
      CALLBACK = 0,
      DATA     = 1,
    };
    std::map<uint32_t,EventCallbackWithData>_eventCallbacks;
    CallbackWithData _idleCallback;
};
