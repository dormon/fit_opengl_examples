#include<iostream>
#include<fstream>
#include<vector>
#include<cassert>
#include<SDL2/SDL.h>
#include<GL/glew.h>
#include<glm/glm.hpp>

#include<window.h>
#include<loadTextFile.h>
#include<shader.h>

struct Data{
  GLuint program;
  GLuint vbo;
  GLuint vao;
};

void init(Data*data);
void draw(Data*data);
void deinit(Data*data);

int main(int32_t,char*[]){
  SDLWindow window;
  Data data;

  init(&data);
  window.setIdle((SDLWindow::Callback)draw,&data);
  window.mainLoop();
  deinit(&data);

  return EXIT_SUCCESS;
}

void draw(Data*data){
  glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

  glUseProgram(data->program);

  glBindVertexArray(data->vao);
  glDrawArrays(GL_TRIANGLES,0,3);
  glBindVertexArray(0);
}

void init(Data*data){
  data->program = createProgram(
      compileShader(GL_VERTEX_SHADER  ,loadFile("shaders/triangle.vp")),
      compileShader(GL_FRAGMENT_SHADER,loadFile("shaders/triangle.fp")));

  glGenBuffers(1,&data->vbo);
  glBindBuffer(GL_ARRAY_BUFFER,data->vbo);
  float vertices[]={0,0,1,0,0,1};
  glBufferData(GL_ARRAY_BUFFER,sizeof(vertices),vertices,GL_STATIC_DRAW);

  glGenVertexArrays(1,&data->vao);

  glBindVertexArray(data->vao);
  glBindBuffer(GL_ARRAY_BUFFER,data->vbo);

  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,sizeof(float)*2,0);

  glBindVertexArray(0);

  glClearColor(0,0,0,1);
}

void deinit(Data*data){
  glDeleteProgram(data->program);
  glDeleteVertexArrays(1,&data->vao);
}


