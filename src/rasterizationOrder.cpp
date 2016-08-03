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

#include<map>
#include<window.h>
#include<loadTextFile.h>
#include<shader.h>
#include<camera.h>
#include<timer.h>
#include<debugging.h>
#include<programObject.h>
#include<textureObject.h>

struct Data{
  struct{
    uint32_t width = 1024;
    uint32_t height = 1024;
  }window;
  std::map<SDL_Keycode,bool>keyDown;
  Timer<float> timer;
  std::shared_ptr<CameraProjection>cameraProjection = nullptr;
  std::shared_ptr<CameraTransform >cameraTransform  = nullptr;
  std::shared_ptr<ProgramObject>program             = nullptr;

  GLuint counterBuffer = 0;
  float percent = 1.f;
  GLuint emptyVAO = 0;
};

void init(Data*data);
void draw(Data*data);
void deinit(Data*data);
void mouseMove(SDL_Event event,Data*data);
void keyDown(SDL_Event event,Data*data);
void keyUp(SDL_Event event,Data*data);


int main(int32_t,char*[]){
  Data data;
  SDLWindow window(data.window.width,data.window.height);

  init(&data);
  window.setIdle((SDLWindow::Callback)draw,&data);
  window.setEvent(SDL_MOUSEMOTION,(SDLWindow::EventCallback)mouseMove ,&data);
  window.setEvent(SDL_KEYDOWN    ,(SDLWindow::EventCallback)keyDown   ,&data);
  window.setEvent(SDL_KEYUP      ,(SDLWindow::EventCallback)keyUp     ,&data);
  window.mainLoop();
  deinit(&data);

  return EXIT_SUCCESS;
}

void draw(Data*data){
  glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

  data->program->use();
  data->program->set2ui("windowSize",data->window.width,data->window.height);
  data->program->set1f("percent",data->percent);

  glBindBufferBase(GL_SHADER_STORAGE_BUFFER,0,data->counterBuffer);
  uint32_t zero = 0;
  glBufferSubData(GL_SHADER_STORAGE_BUFFER,0,sizeof(uint32_t),&zero);
  glFinish();
  glBindVertexArray(data->emptyVAO);
  glDrawArrays(GL_TRIANGLES,0,6);
  glBindVertexArray(0);

  float frameTime = data->timer.elapsedFromLast();
  (void)frameTime;
}

void init(Data*data){
  glEnable(GL_DEBUG_OUTPUT);
  glDebugMessageCallback((GLDEBUGPROC)defaultDebugMessage,NULL);

  FreeImage_Initialise(TRUE);
  data->cameraTransform = std::make_shared<OrbitCamera>();
  data->cameraProjection = std::make_shared<PerspectiveCamera>(
      glm::half_pi<float>(),
      (float)data->window.width/data->window.height,0.001);

  data->program = std::make_shared<ProgramObject>(createProgram(
        compileShader(GL_VERTEX_SHADER  ,
          "#version 450\n",
          loadFile("shaders/rasterizationOrder.vp")),
        compileShader(GL_FRAGMENT_SHADER,
          "#version 450\n",
          loadFile("shaders/rasterizationOrder.fp"))));


  glGenBuffers(1,&data->counterBuffer);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER,data->counterBuffer);
  glBufferData(GL_SHADER_STORAGE_BUFFER,sizeof(uint32_t),nullptr,GL_STATIC_DRAW);

  glGenVertexArrays(1,&data->emptyVAO);

  glClearColor(0,0,0,1);
  glDisable(GL_CULL_FACE);
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LEQUAL);
  glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
}

void deinit(Data*data){
  FreeImage_DeInitialise();
  glDeleteVertexArrays(1,&data->emptyVAO);
}


void mouseMove(SDL_Event event,Data*data){
  if(event.motion.state & SDL_BUTTON_LMASK){
    float sensitivity = 0.01;
    auto orbitCamera = std::dynamic_pointer_cast<OrbitCamera>(data->cameraTransform);
    if(orbitCamera){
      orbitCamera->setXAngle(orbitCamera->getXAngle() + event.motion.yrel*sensitivity);
      orbitCamera->setYAngle(orbitCamera->getYAngle() + event.motion.xrel*sensitivity);
    }
  }
  if(event.motion.state & SDL_BUTTON_RMASK){
    float step = 0.01;
    auto orbitCamera = std::dynamic_pointer_cast<OrbitCamera>(data->cameraTransform);
    if(orbitCamera){
      orbitCamera->setDistance(glm::clamp(orbitCamera->getDistance() + event.motion.yrel*step,0.1f,10.f));

    }
  }
  if(event.motion.state & SDL_BUTTON_MMASK){
    auto orbitCamera = std::dynamic_pointer_cast<OrbitCamera>(data->cameraTransform);
    orbitCamera->addXPosition(+orbitCamera->getDistance()*event.motion.xrel/data->window.width*2.);
    orbitCamera->addYPosition(-orbitCamera->getDistance()*event.motion.yrel/data->window.width*2.);
  }
}

void keyDown(SDL_Event event,Data*data){
  data->keyDown[event.key.keysym.sym]=true;
  float percentChangeSpeed = 0.0001;
  if(data->keyDown['d'])data->percent += percentChangeSpeed;
  if(data->percent>1.f)data->percent=1.f;
  if(data->keyDown['a'])data->percent -= percentChangeSpeed;
  if(data->percent<0.f)data->percent=0.f;

}

void keyUp(SDL_Event event,Data*data){
  data->keyDown[event.key.keysym.sym]=false;
}

