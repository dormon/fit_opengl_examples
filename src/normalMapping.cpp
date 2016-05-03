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
  Timer<float> timer;
  std::shared_ptr<CameraProjection>cameraProjection = nullptr;
  std::shared_ptr<CameraTransform >cameraTransform  = nullptr;
  std::shared_ptr<ProgramObject>program = nullptr;
  std::shared_ptr<TextureObject>diffuseTexture  = nullptr;
  std::shared_ptr<TextureObject>specularTexture = nullptr;
  std::shared_ptr<TextureObject>normalTexture   = nullptr;
  GLuint vao = 0;
  uint32_t useSpecularTexture = 0;
  uint32_t useNormalTexture = 0;
  uint32_t drawDiffuseTexture = 0;
  uint32_t drawSpecularTexture = 0;
  uint32_t drawNormalTexture = 0;
};

void init(Data*data);
void draw(Data*data);
void deinit(Data*data);
void mouseMove(SDL_Event event,Data*data);
void keyDown(SDL_Event event,Data*data);


int main(int32_t,char*[]){
  Data data;
  data.window.width = 1024;
  data.window.height = 768;
  SDLWindow window(data.window.width,data.window.height);

  init(&data);
  window.setIdle((SDLWindow::Callback)draw,&data);
  window.setEvent(SDL_MOUSEMOTION,(SDLWindow::EventCallback)mouseMove ,&data);
  window.setEvent(SDL_KEYDOWN    ,(SDLWindow::EventCallback)keyDown   ,&data);
  window.mainLoop();
  deinit(&data);

  return EXIT_SUCCESS;
}

void draw(Data*data){
  glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

  data->program->use();

  data->diffuseTexture->bind(0);
  data->specularTexture->bind(1);
  data->normalTexture->bind(2);

  data->program->setMatrix4fv("v",glm::value_ptr(data->cameraTransform->getView()));
  data->program->setMatrix4fv("p",glm::value_ptr(data->cameraProjection->getProjection()));
  data->program->set1ui("useSpecularTexture",data->useSpecularTexture);
  data->program->set1ui("useNormalTexture",data->useNormalTexture);
  data->program->set1ui("drawDiffuseTexture",data->drawDiffuseTexture);
  data->program->set1ui("drawSpecularTexture",data->drawSpecularTexture);
  data->program->set1ui("drawNormalTexture",data->drawNormalTexture);


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
      (float)data->window.width/data->window.height,0.01);

  data->program = std::make_shared<ProgramObject>(createProgram(
      compileShader(GL_VERTEX_SHADER  ,
        "#version 450\n",
        loadFile("shaders/normalMapping.vp")),
      compileShader(GL_FRAGMENT_SHADER,
        "#version 450\n",
        loadFile("shaders/lighting.vp"),
        loadFile("shaders/normalMapping.fp"))));

  data->diffuseTexture  = std::make_shared<TextureObject>(
      "textures/sponza_details_diff.png",GL_RGBA,true);
  data->specularTexture = std::make_shared<TextureObject>(
      "textures/sponza_details_spec.png",GL_RGBA,true);
  data->normalTexture = std::make_shared<TextureObject>(
      "textures/sponza_detail_ddn.png",GL_RGBA,true);

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
  if(event.key.keysym.sym == SDLK_q)data->useSpecularTexture^=1;
  if(event.key.keysym.sym == SDLK_w)data->useNormalTexture^=1;
  if(event.key.keysym.sym == SDLK_a)data->drawDiffuseTexture^=1;
  if(event.key.keysym.sym == SDLK_s)data->drawSpecularTexture^=1;
  if(event.key.keysym.sym == SDLK_d)data->drawNormalTexture^=1;

}
