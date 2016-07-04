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
  std::shared_ptr<ProgramObject>program             = nullptr;
  std::shared_ptr<TextureObject>cubeTexture  = nullptr;

  uint32_t waterSizeX = 10;
  uint32_t waterSizeY = 10;

  GLuint sphereBuffer = 0;
  uint32_t nofSpheres;
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
  data->program->setMatrix4fv("v",glm::value_ptr(data->cameraTransform->getView()));
  data->program->setMatrix4fv("p",glm::value_ptr(data->cameraProjection->getProjection()));

  data->cubeTexture->bind(0);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER,0,data->sphereBuffer);
  data->program->set1ui("nofSpheres",data->nofSpheres);
  glBindVertexArray(data->emptyVAO);
  glDrawArrays(GL_TRIANGLE_STRIP,0,4);
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
          loadFile("shaders/fsRayTracing.vp")),
        compileShader(GL_FRAGMENT_SHADER,
          "#version 450\n",
          loadFile("shaders/fsRayTracing.fp"))));

  data->cubeTexture = std::make_shared<TextureObject>(
      /*
      "textures/volareft.tga",
      "textures/volarebk.tga",
      "textures/volareup.tga",
      "textures/volaredn.tga",
      "textures/volarelf.tga",
      "textures/volarert.tga",
      // */
//*
      "textures/cube0.png",
      "textures/cube1.png",
      "textures/cube2.png",
      "textures/cube3.png",
      "textures/cube4.png",
      "textures/cube5.png",
     // */
      GL_RGBA,true);


  glGenBuffers(1,&data->sphereBuffer);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER,data->sphereBuffer);
  struct Sphere{
    glm::vec4 positionRadius;
    glm::vec4 color;
  };
  data->nofSpheres = 30;
  Sphere*spheres = new Sphere[data->nofSpheres];

  for(uint32_t i=0;i<data->nofSpheres;++i){
    float t = (float)i/data->nofSpheres;
    float angle = glm::radians<float>(3*360.f*t);
    spheres[i].positionRadius = glm::vec4(glm::cos(angle),t*4,glm::sin(angle),glm::sin(t*10)*.2f+.3f);
    spheres[i].color          = glm::vec4(0.f,1.f,0.f,1.f);
  }

  glBufferData(GL_SHADER_STORAGE_BUFFER,sizeof(Sphere)*data->nofSpheres,spheres,GL_STATIC_DRAW);
  delete[]spheres;

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
}

void keyUp(SDL_Event event,Data*data){
  data->keyDown[event.key.keysym.sym]=false;
}

