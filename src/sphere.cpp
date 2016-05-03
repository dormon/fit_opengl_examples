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

struct Data{
  struct{
    uint32_t width = 1024;
    uint32_t height = 768;
  }window;
  std::shared_ptr<CameraProjection>cameraProjection = nullptr;
  std::shared_ptr<CameraTransform >cameraTransform  = nullptr;
  GLuint program = 0;
  GLuint vbo = 0;
  GLuint vao = 0;
  GLint vUniform = -1;
  GLint pUniform = -1;
  GLint lightUniform = -1;

  int32_t sphereSizeX = 20;
  int32_t sphereSizeY = 20;
  GLint sphereSizeXUniform = -1;
  GLint sphereSizeYUniform = -1;

  bool useGouraudNormal = false;
  bool usePhongLighting = false;
  bool computeLightInFS = false;

  GLint useGouraudNormalUniform = -1;
  GLint usePhongLightingUniform = -1;
  GLint computeLightInFSUniform = -1;
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

  glUseProgram(data->program);

  glUniformMatrix4fv(data->vUniform,1,GL_FALSE,glm::value_ptr(data->cameraTransform->getView()));
  glUniformMatrix4fv(data->pUniform,1,GL_FALSE,glm::value_ptr(data->cameraProjection->getProjection()));

  glUniform4f(data->lightUniform,10,10,10,1);

  glUniform1ui(data->sphereSizeXUniform,data->sphereSizeX);
  glUniform1ui(data->sphereSizeYUniform,data->sphereSizeY);

  glUniform1i(data->useGouraudNormalUniform,data->useGouraudNormal);
  glUniform1i(data->usePhongLightingUniform,data->usePhongLighting);
  glUniform1i(data->computeLightInFSUniform,data->computeLightInFS);




  glBindVertexArray(data->vao);
  glDrawArrays(GL_TRIANGLES,0,data->sphereSizeX*data->sphereSizeY*6);
  glBindVertexArray(0);
}

void init(Data*data){
  data->cameraTransform = std::make_shared<OrbitCamera>();
  data->cameraProjection = std::make_shared<PerspectiveCamera>(
      glm::half_pi<float>(),
      (float)data->window.width/data->window.height);

  data->program = createProgram(
      compileShader(GL_VERTEX_SHADER  ,
        "#version 450\n",
        loadFile("shaders/lighting.vp"),
        loadFile("shaders/sphere.vp")),
      compileShader(GL_FRAGMENT_SHADER,
        "#version 450\n",
        loadFile("shaders/lighting.vp"),
        loadFile("shaders/sphere.fp")));

  data->vUniform     = glGetUniformLocation(data->program,"v"    );
  data->pUniform     = glGetUniformLocation(data->program,"p"    );
  data->lightUniform = glGetUniformLocation(data->program,"light");

  data->sphereSizeXUniform = glGetUniformLocation(data->program,"sphereSizeX");
  data->sphereSizeYUniform = glGetUniformLocation(data->program,"sphereSizeY");

  data->useGouraudNormalUniform = glGetUniformLocation(data->program,"useGouraudNormal");
  data->usePhongLightingUniform = glGetUniformLocation(data->program,"usePhongLighting");
  data->computeLightInFSUniform = glGetUniformLocation(data->program,"computeLightInFS");


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
  glDeleteProgram(data->program);
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
