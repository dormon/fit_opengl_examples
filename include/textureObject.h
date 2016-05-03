#pragma once

#include<GL/glew.h>
#include<iostream>
#include<FreeImage.h>
#include<vector>
#include<cstring>

class TextureData2D{
  public:
    GLenum format           ;
    GLenum type             ;
    std::vector<uint8_t>data;
    GLsizei width;
    GLsizei height;
    TextureData2D(std::string fileName){
      FREE_IMAGE_FORMAT fif = FIF_UNKNOWN;
      FIBITMAP *dib(0);
      BYTE* bits(0);

      fif = FreeImage_GetFileType(fileName.c_str(), 0);
      if(fif == FIF_UNKNOWN) 
        fif = FreeImage_GetFIFFromFilename(fileName.c_str());
      if(fif == FIF_UNKNOWN)
        return;
      if(FreeImage_FIFSupportsReading(fif))
        dib = FreeImage_Load(fif, fileName.c_str());
      if(!dib)
        return;

      this->width  = FreeImage_GetWidth(dib);
      this->height = FreeImage_GetHeight(dib);

      auto bpp = FreeImage_GetBPP(dib);
      this->format = bpp == 32?GL_BGRA:GL_BGR;

      this->type = GL_UNSIGNED_BYTE;
      bits       = FreeImage_GetBits(dib);

      this->data.resize(this->width*this->height*bpp/8);
      std::memcpy(this->data.data(),bits,this->width*this->height*bpp/8);

      FreeImage_Unload(dib);
    }

};

class TextureObject{
  protected:
    GLenum _target = GL_TEXTURE_2D;
    GLuint _id = 0;
  public:
    TextureObject(){
      this->_target = GL_TEXTURE_2D;
      glGenTextures(1,&this->_id);
      glBindTexture(this->_target,this->_id);
    }
    void setData(void const*data,GLsizei width,GLsizei height,GLenum internalFormat,GLenum format){
      glBindTexture(this->_target,this->_id);
      glTexImage2D(this->_target,0,internalFormat,width,height,0,format,GL_UNSIGNED_BYTE,data);
      glTexParameteri(this->_target,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
      glTexParameteri(this->_target,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    };
    TextureObject(std::string fileName,GLenum internalFormat,bool buildMipmaps){
      TextureData2D imgData(fileName);

      this->_target = GL_TEXTURE_2D;
      glGenTextures(1,&this->_id);
      glBindTexture(this->_target,this->_id);
      glTexImage2D(this->_target,0,internalFormat,imgData.width,imgData.height,0,imgData.format,imgData.type,imgData.data.data());
      if(buildMipmaps){
        glTexParameteri(this->_target,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(this->_target,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
        glGenerateMipmap(this->_target);
      }else{
        glTexParameteri(this->_target,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
        glTexParameteri(this->_target,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
      }

    }
    TextureObject(
        std::string pxFileName,
        std::string nxFileName,
        std::string pyFileName,
        std::string nyFileName,
        std::string pzFileName,
        std::string nzFileName,
        GLenum internalFormat,bool buildMipmaps){
      this->_target = GL_TEXTURE_CUBE_MAP;

      TextureData2D imgData[6]={
        TextureData2D(pxFileName),
        TextureData2D(nxFileName),
        TextureData2D(pyFileName),
        TextureData2D(nyFileName),
        TextureData2D(pzFileName),
        TextureData2D(nzFileName),
      };

      GLenum faceTarget[6]={
        GL_TEXTURE_CUBE_MAP_POSITIVE_X,
        GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
        GL_TEXTURE_CUBE_MAP_POSITIVE_Y,
        GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
        GL_TEXTURE_CUBE_MAP_POSITIVE_Z,
        GL_TEXTURE_CUBE_MAP_NEGATIVE_Z,
      };

      glGenTextures(1,&this->_id);
      glBindTexture(this->_target,this->_id);
      for(uint32_t f=0;f<6;++f){
        glTexImage2D(
            faceTarget[f],
            0,
            internalFormat,
            imgData[f].width,
            imgData[f].height,
            0,
            imgData[f].format,
            imgData[f].type,imgData[f].data.data());
      }
      if(buildMipmaps){
        glTexParameteri(this->_target,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(this->_target,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
        glGenerateMipmap(this->_target);
      }else{
        glTexParameteri(this->_target,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
        glTexParameteri(this->_target,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
      }
    }

    ~TextureObject(){
    }
    GLuint getId()const{return this->_id;}
    GLenum getTarget()const{return this->_target;}
    void bind(size_t unit)const{
      glActiveTexture(GL_TEXTURE0+unit);
      glBindTexture(this->_target,this->_id);
    }
};
