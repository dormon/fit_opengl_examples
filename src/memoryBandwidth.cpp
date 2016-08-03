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
#include<ValuePrinter.h>

struct Data{
  struct{
    uint32_t width = 1024;
    uint32_t height = 1024;
  }window;
  Timer<float> timer;
  std::shared_ptr<ProgramObject>program = nullptr;

  GLuint buffer = 0;
  GLuint query = 0;
};

void init(Data*data);
void draw(Data*data);
void deinit(Data*data);


int main(int32_t,char*[]){
  Data data;
  SDLWindow window(data.window.width,data.window.height);

  init(&data);
  window.setIdle((SDLWindow::Callback)draw,&data);
  window.mainLoop();
  deinit(&data);

  return EXIT_SUCCESS;
}

const size_t FLOATS_PER_THREAD = 32;
const size_t COMPUTE_UNITS = 20;
const size_t WAVEFRONT_SIZE = 64;
const size_t WAVEFRONTS_PER_COMPUTE_UNIT = 2048;
const size_t NOF_THREADS = WAVEFRONT_SIZE*COMPUTE_UNITS*WAVEFRONTS_PER_COMPUTE_UNIT;
const size_t READBUFFER_SIZE_IN_FLOATS = NOF_THREADS*FLOATS_PER_THREAD;
const size_t WORKGROUP_SIZE = 64;
const size_t NOF_WORKGROUPS = NOF_THREADS/WORKGROUP_SIZE;
const size_t BYTES_PER_FLOAT = 4;
const size_t GIGA = 1024*1024*1024;
const size_t NANOSECONDS_IN_SECOND = 1000000000;
const size_t ITERATIONS = 50;

void draw(Data*data){
  glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

  data->program->use();

  glBindBufferBase(GL_SHADER_STORAGE_BUFFER,0,data->buffer);
  glFinish();
  glBeginQuery(GL_TIME_ELAPSED,data->query);
  for(size_t i=0;i<ITERATIONS;++i)
    glDispatchCompute(NOF_WORKGROUPS,1,1);
  glEndQuery(GL_TIME_ELAPSED);
  GLint64 elapsedTime;
  glGetQueryObjecti64v(data->query,GL_QUERY_RESULT,&elapsedTime);
  glFinish();


  double throughput=
    (double)ITERATIONS*
    (double)BYTES_PER_FLOAT*
    (double)READBUFFER_SIZE_IN_FLOATS/
    (double)(elapsedTime)*
    (double)(NANOSECONDS_IN_SECOND);
  std::cout<<throughput/GIGA<<" GB/second"<<std::endl;

}

void init(Data*data){
  glEnable(GL_DEBUG_OUTPUT);
  glDebugMessageCallback((GLDEBUGPROC)defaultDebugMessage,NULL);

  data->program = std::make_shared<ProgramObject>(createProgram(
        compileShader(GL_COMPUTE_SHADER,
          "#version 450\n",
          std::string("#define WORKGROUP_SIZE ")+value2str(WORKGROUP_SIZE)+"\n",
          std::string("#define FLOATS_PER_THREAD ")+value2str(FLOATS_PER_THREAD)+"\n",
          loadFile("shaders/memoryBandwidth.comp"))));


  glGenBuffers(1,&data->buffer);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER,data->buffer);
  glBufferData(GL_SHADER_STORAGE_BUFFER,READBUFFER_SIZE_IN_FLOATS*sizeof(float),nullptr,GL_DYNAMIC_COPY);

  glGenQueries(1,&data->query);

  glClearColor(0,0,0,1);
  glDisable(GL_CULL_FACE);
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LEQUAL);
  glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
}

void deinit(Data*data){
  glDeleteBuffers(1,&data->buffer);
  glDeleteQueries(1,&data->query);
}

