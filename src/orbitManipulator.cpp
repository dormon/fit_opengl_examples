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
  GLint mvpUniform = -1;
  glm::mat4 mvp;
};

void init(Data*data);
void draw(Data*data);
void deinit(Data*data);
void mouseMove(SDL_Event event,Data*data);


int main(int32_t,char*[]){
  Data data;
  data.window.width = 1024;
  data.window.height = 768;
  SDLWindow window(data.window.width,data.window.height);

  init(&data);
  window.setIdle((SDLWindow::Callback)draw,&data);
  window.setEvent(SDL_MOUSEMOTION,(SDLWindow::EventCallback)mouseMove ,&data);
  window.mainLoop();
  deinit(&data);

  return EXIT_SUCCESS;
}

void draw(Data*data){
  glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

  glUseProgram(data->program);

  glUniformMatrix4fv(data->mvpUniform,1,GL_FALSE,glm::value_ptr(data->mvp));

  glBindVertexArray(data->vao);
  glDrawArrays(GL_TRIANGLES,0,3);
  glBindVertexArray(0);
}

void init(Data*data){
  data->cameraTransform = std::make_shared<OrbitCamera>();
  data->cameraProjection = std::make_shared<PerspectiveCamera>(
      glm::half_pi<float>(),
      (float)data->window.width/data->window.height);

  data->program = createProgram(
      compileShader(GL_VERTEX_SHADER  ,loadFile("shaders/orbitManipulator.vp")),
      compileShader(GL_FRAGMENT_SHADER,loadFile("shaders/orbitManipulator.fp")));

  data->mvpUniform = glGetUniformLocation(data->program,"mvp");

  glGenBuffers(1,&data->vbo);
  glBindBuffer(GL_ARRAY_BUFFER,data->vbo);
  float vertices[]={0,0,0,1, 1,0,0,1, 0,1,0,1};
  glBufferData(GL_ARRAY_BUFFER,sizeof(vertices),vertices,GL_STATIC_DRAW);

  glGenVertexArrays(1,&data->vao);

  glBindVertexArray(data->vao);
  glBindBuffer(GL_ARRAY_BUFFER,data->vbo);

  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0,4,GL_FLOAT,GL_FALSE,sizeof(float)*4,0);

  glBindVertexArray(0);

  glClearColor(0,0,0,1);
  glDisable(GL_CULL_FACE);
}

void deinit(Data*data){
  glDeleteProgram(data->program);
  glDeleteVertexArrays(1,&data->vao);
}


void mouseMove(SDL_Event event,Data*data){
  bool recompute = false;
  if(event.motion.state & SDL_BUTTON_LMASK){
    float sensitivity = 0.01;
    auto orbitCamera = std::dynamic_pointer_cast<OrbitCamera>(data->cameraTransform);
    if(orbitCamera){
      orbitCamera->setXAngle(orbitCamera->getXAngle() + event.motion.yrel*sensitivity);
      orbitCamera->setYAngle(orbitCamera->getYAngle() + event.motion.xrel*sensitivity);
      recompute = true;
    }
  }
  if(event.motion.state & SDL_BUTTON_RMASK){
    float step = 0.1;
    auto orbitCamera = std::dynamic_pointer_cast<OrbitCamera>(data->cameraTransform);
    if(orbitCamera){
      orbitCamera->setDistance(orbitCamera->getDistance() + event.motion.yrel*step);
      recompute = true;
    }
    recompute = true;
  }
  if(recompute)
    data->mvp = data->cameraProjection->getProjection()*data->cameraTransform->getView();
}


