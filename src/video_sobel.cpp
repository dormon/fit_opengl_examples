#include<iostream>
#include<fstream>
#include<vector>
#include<cassert>
#include<memory>
#include<SDL2/SDL.h>
#include<GL/glew.h>
#include<glm/glm.hpp>
#include<glm/gtc/matrix_transform.hpp>
#include<glm/gtc/type_ptr.hpp>
#include<glm/gtc/matrix_access.hpp>

#include<window.h>
#include<loadTextFile.h>
#include<shader.h>
#include<camera.h>
#include<timer.h>
#include<debugging.h>
#include<programObject.h>
#include<textureObject.h>
#include<videoReader.h>


struct Data{
  std::vector<std::string>args;
  std::shared_ptr<SDLWindow>window = nullptr;
  Timer<float> timer;
  std::shared_ptr<Video>video = nullptr;
  std::shared_ptr<ProgramObject>program = nullptr;
  std::shared_ptr<TextureObject>frame = nullptr;
  uint32_t frameCounter = 0;
  float    lastFrameTime = 0;
  uint32_t mode = 0;
  bool running = true;
  GLuint vao;
};

void init(Data*data);
void draw(Data*data);
void deinit(Data*data);
void keyDown(SDL_Event event,Data*data);

int main(int32_t argc,char*args[]){
  Data data;

  for(int32_t i=0;i<argc;++i)
    data.args.push_back(std::string(args[i]));

  data.window = std::make_shared<SDLWindow>();


  init(&data);
  data.window->setIdle((SDLWindow::Callback)draw,&data);
  data.window->setEvent(SDL_KEYDOWN    ,(SDLWindow::EventCallback)keyDown   ,&data);
  data.window->mainLoop();
  deinit(&data);

  return EXIT_SUCCESS;
}

void draw(Data*data){
  glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

  float  newTime = data->timer.elapsedFromStart();
  if((newTime-data->lastFrameTime)>1./data->video->getFps()){
    if(data->running){
      data->frame->setData(
          data->video->getData(),
          data->video->getWidth(),
          data->video->getHeight(),GL_RGB,GL_BGR);
      data->lastFrameTime = newTime;
    }
  }

  data->program->use();
  data->program->set2ui("windowSize",data->window->getWidth(),data->window->getHeight());
  data->program->set1ui("mode",data->mode);
  data->frame->bind(0);

  glBindVertexArray(data->vao);
  glDrawArrays(GL_TRIANGLE_STRIP,0,4);
  glBindVertexArray(0);
}

void init(Data*data){

  if(data->args.size()<2){
    std::cerr<<"expected name of video"<<std::endl;
    std::exit(0);
  }
  data->video = std::make_shared<Video>(data->args[1]);

  data->window->setSize(data->video->getWidth(),data->video->getHeight());


  glEnable(GL_DEBUG_OUTPUT);
  glDebugMessageCallback((GLDEBUGPROC)defaultDebugMessage,NULL);

  glViewport(0,0,data->video->getWidth(),data->video->getHeight());

  data->frame = std::make_shared<TextureObject>();
  data->program = std::make_shared<ProgramObject>(createProgram(
        compileShader(GL_VERTEX_SHADER  ,
          "#version 450\n",
          loadFile("shaders/video_sobel.vp")),
        compileShader(GL_FRAGMENT_SHADER,
          "#version 450\n",
          loadFile("shaders/noiseFunctions.vp"),
          loadFile("shaders/gradients.vp"),
          loadFile("shaders/video_sobel.fp"))));

  glGenVertexArrays(1,&data->vao);

  glClearColor(0,0,0,1);
}

void deinit(Data*data){
  data->program = nullptr;
  glDeleteVertexArrays(1,&data->vao);
}

void keyDown(SDL_Event event,Data*data){
  if(event.key.keysym.sym == SDLK_q)++data->mode%=3;
  if(event.key.keysym.sym == SDLK_w)data->mode=(data->mode+2)%3;
  if(event.key.keysym.sym == SDLK_SPACE)data->running^=1;
  if(event.key.keysym.sym == SDLK_RIGHT)data->video->move(1.f);
  if(event.key.keysym.sym == SDLK_LEFT)data->video->move(-1.f);
}

