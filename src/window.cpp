#include<window.h>
#include<vector>
#include<GL/glew.h>

SDLWindow::SDLWindow(uint32_t width,uint32_t heigth){
  SDL_Init(SDL_INIT_VIDEO);//init. video
  this->_window = SDL_CreateWindow("sdl2",SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,width,heigth,
      SDL_WINDOW_OPENGL|SDL_WINDOW_SHOWN);

  unsigned profile = SDL_GL_CONTEXT_PROFILE_CORE;//context profile
  unsigned flags   = SDL_GL_CONTEXT_DEBUG_FLAG;//context flags
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK ,profile         );
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS        ,flags           );


  std::vector<uint32_t>versions={450,440,430,420,410,400,330};
  for(auto x:versions){
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, x/100    );
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION,(x%100)/10);
    this->_context = SDL_GL_CreateContext(this->_window);//create context
    if(this->_context!=nullptr)break;
  }

  glewInit();//initialisation of gl* functions
}

SDLWindow::~SDLWindow(){
  SDL_GL_DeleteContext(this->_context);
  SDL_Quit();
}

void SDLWindow::mainLoop(){
  SDL_Event event;
  bool running = true;
  while(running){
    while(SDL_PollEvent(&event)){
      if(event.type == SDL_QUIT)
        running = false;

      auto ii=this->_eventCallbacks.find(event.type);
      if(ii!=this->_eventCallbacks.end())
        if(std::get<CALLBACK>(ii->second))
          std::get<CALLBACK>(ii->second)(event,std::get<DATA>(ii->second));

    }

    if(std::get<CALLBACK>(this->_idleCallback))
      std::get<CALLBACK>(this->_idleCallback)(std::get<DATA>(this->_idleCallback));

    SDL_GL_SwapWindow(this->_window);
  }

}

void SDLWindow::setIdle(Callback fce,void*data){
  this->_idleCallback = CallbackWithData(fce,data);
}

void SDLWindow::setEvent(uint32_t type,EventCallback fce,void*data){
  this->_eventCallbacks[type] = EventCallbackWithData(fce,data);
}

void SDLWindow::setSize(uint32_t width,uint32_t heght){
  SDL_SetWindowSize(this->_window,width,heght);
}

uint32_t SDLWindow::getWidth()const{
  int size[2];
  SDL_GetWindowSize(this->_window,size+0,size+1);
  return size[0];
}

uint32_t SDLWindow::getHeight()const{
  int size[2];
  SDL_GetWindowSize(this->_window,size+0,size+1);
  return size[1];
}

