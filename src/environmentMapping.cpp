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
  std::shared_ptr<ProgramObject>environmentProgram  = nullptr;
  std::shared_ptr<ProgramObject>waterProgram        = nullptr;
  std::shared_ptr<TextureObject>diffuseTextureTop   = nullptr;
  std::shared_ptr<TextureObject>diffuseTextureSide  = nullptr;
  std::shared_ptr<TextureObject>diffuseTextureDown  = nullptr;
  std::shared_ptr<TextureObject>cubeTexture  = nullptr;

  uint32_t waterSizeX = 10;
  uint32_t waterSizeY = 10;

  GLuint vbo = 0;
  GLuint vao = 0;
  GLuint emptyVAO = 0;
  int32_t sphereSizeX = 20;
  int32_t sphereSizeY = 20;

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

  data->environmentProgram->use();
  data->environmentProgram->setMatrix4fv("v",glm::value_ptr(data->cameraTransform->getView()));
  data->environmentProgram->setMatrix4fv("p",glm::value_ptr(data->cameraProjection->getProjection()));


  data->cubeTexture->bind(0);
  glBindVertexArray(data->emptyVAO);
  glDrawArrays(GL_TRIANGLE_STRIP,0,4);
  glBindVertexArray(0);

  data->program->use();

  data->diffuseTextureTop ->bind(0);
  data->diffuseTextureSide->bind(1);
  data->diffuseTextureDown->bind(2);
  data->cubeTexture       ->bind(3);

  float frameTime = data->timer.elapsedFromLast();
  (void)frameTime;


  data->program->setMatrix4fv("v",glm::value_ptr(data->cameraTransform->getView()));
  data->program->setMatrix4fv("p",glm::value_ptr(data->cameraProjection->getProjection()));

  glBindVertexArray(data->vao);
  glDrawArrays(GL_TRIANGLES,0,data->sphereSizeX*data->sphereSizeY*6);
  glBindVertexArray(0);

  data->waterProgram->use();
  data->waterProgram->set1ui("waterSizeX",data->waterSizeX);
  data->waterProgram->set1ui("waterSizeY",data->waterSizeY);
  data->waterProgram->set1f("time",data->timer.elapsedFromStart());
  data->waterProgram->setMatrix4fv("v",glm::value_ptr(data->cameraTransform->getView()));
  data->waterProgram->setMatrix4fv("p",glm::value_ptr(data->cameraProjection->getProjection()));


  glEnable(GL_BLEND);
  data->cubeTexture->bind(0);
  glBindVertexArray(data->emptyVAO);
  glPatchParameteri(GL_PATCH_VERTICES,1);
  glDrawArrays(GL_PATCHES,0,data->waterSizeX*data->waterSizeY);
  glBindVertexArray(0);
  glDisable(GL_BLEND);
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
          loadFile("shaders/cubeMapping.vp")),
        compileShader(GL_FRAGMENT_SHADER,
          "#version 450\n",
          loadFile("shaders/lighting.vp"),
          loadFile("shaders/cubeMapping.fp"))));

  data->environmentProgram = std::make_shared<ProgramObject>(createProgram(
        compileShader(GL_VERTEX_SHADER  ,
          "#version 450\n",
          loadFile("shaders/environment.vp")),
        compileShader(GL_FRAGMENT_SHADER,
          "#version 450\n",
          loadFile("shaders/environment.fp"))));

  data->waterProgram = std::make_shared<ProgramObject>(createProgram(
        compileShader(GL_VERTEX_SHADER  ,
          "#version 450\n",
          loadFile("shaders/water.vp")),
        compileShader(GL_TESS_CONTROL_SHADER,
           "#version 450\n",
          loadFile("shaders/water.cp")),
        compileShader(GL_TESS_EVALUATION_SHADER,
           "#version 450\n",
          loadFile("shaders/noiseFunctions.vp"),
          loadFile("shaders/water.ep")),
        compileShader(GL_FRAGMENT_SHADER,
          "#version 450\n",
          loadFile("shaders/lighting.vp"),
          loadFile("shaders/water.fp"))));



  data->diffuseTextureTop  = std::make_shared<TextureObject>(
      "textures/grass.png",GL_RGBA,true);
  data->diffuseTextureSide  = std::make_shared<TextureObject>(
      "textures/dirt.jpg",GL_RGBA,true);
  data->diffuseTextureDown  = std::make_shared<TextureObject>(
      "textures/rock.jpg",GL_RGBA,true);
  data->cubeTexture = std::make_shared<TextureObject>(
      "textures/cube0.png",
      "textures/cube1.png",
      "textures/cube2.png",
      "textures/cube3.png",
      "textures/cube4.png",
      "textures/cube5.png",
      GL_RGBA,true);

  glGenBuffers(1,&data->vbo);
  glBindBuffer(GL_ARRAY_BUFFER,data->vbo);
  const uint32_t floatsPerVertex = 6;
  const uint32_t vertiesPerFace = 6;
  float*vertices = new float[data->sphereSizeX*data->sphereSizeY*vertiesPerFace*floatsPerVertex];
  for(int32_t y=0;y<data->sphereSizeY;++y){
    for(int32_t x=0;x<data->sphereSizeX;++x){
      for(uint32_t k=0;k<vertiesPerFace;++k){
        const int32_t xOffset[]={0,1,0,0,1,1};
        const int32_t yOffset[]={0,0,1,1,0,1};
        float u = (float)(x+xOffset[k])/data->sphereSizeX;
        float v = (float)(y+yOffset[k])/data->sphereSizeY;
        float xAngle = -u*glm::two_pi<float>();
        float yAngle = v*glm::pi<float>();
        uint32_t faceId = y*data->sphereSizeX+x;
        uint32_t faceVertex = faceId*vertiesPerFace+k;
        vertices[faceVertex*floatsPerVertex+0] = glm::cos(xAngle)*glm::sin(yAngle);
        vertices[faceVertex*floatsPerVertex+1] = -glm::cos(yAngle);
        vertices[faceVertex*floatsPerVertex+2] = glm::sin(xAngle)*glm::sin(yAngle);
        vertices[faceVertex*floatsPerVertex+3] = 1;
        vertices[faceVertex*floatsPerVertex+4] = u;
        vertices[faceVertex*floatsPerVertex+5] = v;
      }
    }
  }
  glBufferData(GL_ARRAY_BUFFER,sizeof(float)*data->sphereSizeX*data->sphereSizeY*vertiesPerFace*floatsPerVertex,vertices,GL_STATIC_DRAW);
  delete[]vertices;

  glGenVertexArrays(1,&data->vao);

  glBindVertexArray(data->vao);
  glBindBuffer(GL_ARRAY_BUFFER,data->vbo);

  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0,4,GL_FLOAT,GL_FALSE,sizeof(float)*floatsPerVertex,(GLvoid*)(sizeof(float)*0));

  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1,2,GL_FLOAT,GL_FALSE,sizeof(float)*floatsPerVertex,(GLvoid*)(sizeof(float)*4));

  glBindVertexArray(0);

  glGenVertexArrays(1,&data->emptyVAO);

  glClearColor(0,0,0,1);
  glDisable(GL_CULL_FACE);
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LEQUAL);
  glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

  glBlendEquation(GL_FUNC_ADD);
  glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

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

