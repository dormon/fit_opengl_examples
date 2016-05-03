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

struct Data{
  struct{
    uint32_t width = 1024;
    uint32_t height = 768;
  }window;
  std::map<SDL_Keycode,bool>keyDown;
  Timer<float> timer;
  std::shared_ptr<CameraProjection>cameraProjection = nullptr;
  std::shared_ptr<CameraTransform >cameraTransform  = nullptr;
  std::shared_ptr<ProgramObject>program = nullptr;
  std::shared_ptr<TextureObject>diffuseTexture  = nullptr;
  GLuint vao = 0;
  glm::vec2 textureScale = glm::vec2(1.0f,1.0f);
  uint32_t useMagLINEAR = 0;
  uint32_t useMipmap = 0;
  uint32_t useBetweenLINEAR = 0;
  uint32_t useMinLINEAR = 0;
  uint32_t textureWrapModeS = 0;
  uint32_t textureWrapModeT = 0;
};

void init(Data*data);
void draw(Data*data);
void deinit(Data*data);
void mouseMove(SDL_Event event,Data*data);
void keyDown(SDL_Event event,Data*data);
void keyUp(SDL_Event event,Data*data);


int main(int32_t,char*[]){
  Data data;
  data.window.width = 1024;
  data.window.height = 768;
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

  data->diffuseTexture->bind(0);
  if(data->useMagLINEAR)
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR );
  else
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);

  if(data->useMipmap){
    if(data->useMinLINEAR){
      if(data->useBetweenLINEAR)
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR  );
      else
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_NEAREST );
    }else{
      if(data->useBetweenLINEAR)
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST_MIPMAP_LINEAR );
      else
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST_MIPMAP_NEAREST);
    }
  }else{
    if(data->useMinLINEAR)
      glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR );
    else
      glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
  }

  switch(data->textureWrapModeS){
    case 0:glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);break;
    case 1:glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);break;
    case 2:glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_MIRRORED_REPEAT);break;
  }
  switch(data->textureWrapModeT){
    case 0:glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);break;
    case 1:glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);break;
    case 2:glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_MIRRORED_REPEAT);break;
  }

  float frameTime = data->timer.elapsedFromLast();
  if(data->keyDown[SDLK_i])data->textureScale.y*=glm::pow(2.0f,frameTime);
  if(data->keyDown[SDLK_k])data->textureScale.y/=glm::pow(2.0f,frameTime);
  if(data->keyDown[SDLK_l])data->textureScale.x*=glm::pow(2.0f,frameTime);
  if(data->keyDown[SDLK_j])data->textureScale.x/=glm::pow(2.0f,frameTime);
  if(data->textureScale.x<0.125)data->textureScale.x=0.125;
  if(data->textureScale.y<0.125)data->textureScale.y=0.125;
  if(data->textureScale.x>16)data->textureScale.x=16;
  if(data->textureScale.y>16)data->textureScale.y=16;

  data->program->set2fv("textureScale",glm::value_ptr(data->textureScale));

  data->program->setMatrix4fv("v",glm::value_ptr(data->cameraTransform->getView()));
  data->program->setMatrix4fv("p",glm::value_ptr(data->cameraProjection->getProjection()));

  glBindVertexArray(data->vao);
  glDrawArrays(GL_TRIANGLES,0,6);
  glBindVertexArray(0);
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
          loadFile("shaders/wrapping.vp")),
        compileShader(GL_FRAGMENT_SHADER,
          "#version 450\n",
          loadFile("shaders/lighting.vp"),
          loadFile("shaders/wrapping.fp"))));

  data->diffuseTexture  = std::make_shared<TextureObject>(
      "textures/sponza_details_diff.png",GL_RGBA,true);

  glGenVertexArrays(1,&data->vao);

  glClearColor(0,0,0,1);
  glDisable(GL_CULL_FACE);
  glEnable(GL_DEPTH_TEST);

}

void deinit(Data*data){
  FreeImage_DeInitialise();
  glDeleteVertexArrays(1,&data->vao);
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
      orbitCamera->setDistance(orbitCamera->getDistance() + event.motion.yrel*step);
    }
  }
  if(event.motion.state & SDL_BUTTON_MMASK){
    auto orbitCamera = std::dynamic_pointer_cast<OrbitCamera>(data->cameraTransform);
    orbitCamera->addXPosition(+orbitCamera->getDistance()*event.motion.xrel/data->window.width*2.);
    orbitCamera->addYPosition(-orbitCamera->getDistance()*event.motion.yrel/data->window.width*2.);
  }
}

void keyDown(SDL_Event event,Data*data){
  if(event.key.keysym.sym == SDLK_q)data->useMagLINEAR^=1;
  if(event.key.keysym.sym == SDLK_a)data->useMinLINEAR^=1;
  if(event.key.keysym.sym == SDLK_s)data->useMipmap^=1;
  if(event.key.keysym.sym == SDLK_d)data->useBetweenLINEAR^=1;
  if(event.key.keysym.sym == SDLK_y)++data->textureWrapModeS%=3;
  if(event.key.keysym.sym == SDLK_x)++data->textureWrapModeT%=3;
  data->keyDown[event.key.keysym.sym]=true;
}

void keyUp(SDL_Event event,Data*data){
  data->keyDown[event.key.keysym.sym]=false;
}

