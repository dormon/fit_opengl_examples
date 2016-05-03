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
#include<programObject.h>

struct Data{
  struct{
    uint32_t width = 1024;
    uint32_t height = 768;
  }window;
  Timer<float> timer;
  std::shared_ptr<CameraProjection>cameraProjection = nullptr;
  std::shared_ptr<CameraTransform >cameraTransform  = nullptr;
  std::shared_ptr<ProgramObject>program = nullptr;
  GLuint vbo = 0;
  GLuint vao = 0;

  int32_t sphereSizeX = 20;
  int32_t sphereSizeY = 20;

  bool useGouraudNormal = true;
  bool usePhongLighting = true;
  bool computeLightInFS = true;
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

  data->program->setMatrix4fv("v",glm::value_ptr(data->cameraTransform->getView()));
  data->program->setMatrix4fv("p",glm::value_ptr(data->cameraProjection->getProjection()));

  data->program->set4f("light",10,10,10,1);

  data->program->set1ui("sphereSizeX",data->sphereSizeX);
  data->program->set1ui("sphereSizeY",data->sphereSizeY);

  data->program->set1i("useGouraudNormal",data->useGouraudNormal);
  data->program->set1i("usePhongLighting",data->usePhongLighting);
  data->program->set1i("computeLightInFS",data->computeLightInFS);

  data->program->set1f("time",data->timer.elapsedFromStart());


  glBindVertexArray(data->vao);
  glDrawArrays(GL_TRIANGLES,0,data->sphereSizeX*data->sphereSizeY*6);
  glBindVertexArray(0);
}

void init(Data*data){
  data->cameraTransform = std::make_shared<OrbitCamera>();
  data->cameraProjection = std::make_shared<PerspectiveCamera>(
      glm::half_pi<float>(),
      (float)data->window.width/data->window.height);

  data->program = std::make_shared<ProgramObject>(createProgram(
      compileShader(GL_VERTEX_SHADER  ,
        "#version 450\n",
        loadFile("shaders/lighting.vp"),
        loadFile("shaders/noiseFunctions.vp"),
        loadFile("shaders/noiseSphere.vp")),
      compileShader(GL_FRAGMENT_SHADER,
        "#version 450\n",
        loadFile("shaders/lighting.vp"),
        loadFile("shaders/noiseFunctions.vp"),
        loadFile("shaders/noiseSphere.fp"))));

  glGenBuffers(1,&data->vbo);
  glBindBuffer(GL_ARRAY_BUFFER,data->vbo);
  float*vertices = new float[data->sphereSizeX*data->sphereSizeY*6*4];
  for(int32_t y=0;y<data->sphereSizeY;++y){
    for(int32_t x=0;x<data->sphereSizeX;++x){
      for(int32_t k=0;k<6;++k){
        const int32_t xOffset[]={0,1,0,0,1,1};
        const int32_t yOffset[]={0,0,1,1,0,1};
        float xAngle = (float)(x+xOffset[k])/data->sphereSizeX*glm::two_pi<float>();
        float yAngle = (float)(y+yOffset[k])/data->sphereSizeY*glm::pi<float>();
        vertices[((y*data->sphereSizeX+x)*6+k)*4+0] = glm::cos(xAngle)*glm::sin(yAngle);
        vertices[((y*data->sphereSizeX+x)*6+k)*4+1] = -glm::cos(yAngle);
        vertices[((y*data->sphereSizeX+x)*6+k)*4+2] = glm::sin(xAngle)*glm::sin(yAngle);
        vertices[((y*data->sphereSizeX+x)*6+k)*4+3] = 1;
      }
    }
  }
  glBufferData(GL_ARRAY_BUFFER,sizeof(float)*data->sphereSizeX*data->sphereSizeY*6*4,vertices,GL_STATIC_DRAW);
  delete[]vertices;

  glGenVertexArrays(1,&data->vao);

  glBindVertexArray(data->vao);
  glBindBuffer(GL_ARRAY_BUFFER,data->vbo);

  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0,4,GL_FLOAT,GL_FALSE,sizeof(float)*4,0);

  glBindVertexArray(0);

  glClearColor(0,0,0,1);
  glDisable(GL_CULL_FACE);
  glEnable(GL_DEPTH_TEST);
}

void deinit(Data*data){
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
    float step = 0.1;
    auto orbitCamera = std::dynamic_pointer_cast<OrbitCamera>(data->cameraTransform);
    if(orbitCamera){
      orbitCamera->setDistance(orbitCamera->getDistance() + event.motion.yrel*step);
    }
  }
}

void keyDown(SDL_Event event,Data*data){
  if(event.key.keysym.sym == SDLK_q)data->useGouraudNormal^=1;
  if(event.key.keysym.sym == SDLK_w)data->usePhongLighting^=1;
  if(event.key.keysym.sym == SDLK_e)data->computeLightInFS^=1;
  if(event.key.keysym.sym == SDLK_r){
    data->useGouraudNormal = 0;
    data->usePhongLighting = 0;
    data->computeLightInFS = 0;
  }
}
