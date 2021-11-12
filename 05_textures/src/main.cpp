#include <iostream>
#include <SDL.h>

#include <geGL/geGL.h>
#include <geGL/StaticCalls.h>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <libs/stb_image/stb_image.h>

#include <bunny.hpp>

using namespace ge::gl;

#ifndef CMAKE_ROOT_DIR
#define CMAKE_ROOT_DIR "."
#endif


void error(std::string const&name,std::string const&msg){
  bool errorCmd = false;
  if(errorCmd){
    std::cerr << name << std::endl;
    std::cerr << msg << std::endl;
  }else{
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,name.c_str(),msg.c_str(),nullptr);
  }
}

std::string shaderTypeToName(GLuint type){
  if(type==GL_VERTEX_SHADER         )return "vertex"    ;
  if(type==GL_FRAGMENT_SHADER       )return "fragment"  ;
  if(type==GL_GEOMETRY_SHADER       )return "geometry"  ;
  if(type==GL_TESS_CONTROL_SHADER   )return "control"   ;
  if(type==GL_TESS_EVALUATION_SHADER)return "evaluation";
  if(type==GL_COMPUTE_SHADER        )return "compute"   ;
  return "unknown";
}

GLuint createShader(GLuint type,std::string const&src){
  GLuint vs = glCreateShader(type);
  char const*vsSrc[1]={
    src.c_str()
  };

  glShaderSource(vs,1,vsSrc,nullptr);
  glCompileShader(vs);
  int compileStatus;
  glGetShaderiv(vs,GL_COMPILE_STATUS,&compileStatus);
  if(compileStatus != GL_TRUE){
    uint32_t const msgLen = 1<<11;
    char msg[msgLen];
    glGetShaderInfoLog(vs,msgLen,nullptr,msg);

    error(shaderTypeToName(type)+" shader compilation error",msg);
  }
  return vs;
}

GLuint createProgram(std::vector<GLuint>const&shaders){
  GLuint prg = glCreateProgram();

  for(auto const&shader:shaders)
    glAttachShader(prg,shader);

  glLinkProgram(prg);
  GLint linkStatus;
  glGetProgramiv(prg,GL_LINK_STATUS,&linkStatus);
  if(linkStatus != GL_TRUE){
    uint32_t const msgLen = 1<<11;
    char msg[msgLen];
    glGetProgramInfoLog(prg,msgLen,nullptr,msg);
    error("program linking error",msg);
  }

  for(auto const&shader:shaders)
    glDeleteShader(shader);

  return prg;
}

void setVertexAttribute(
    GLuint   vao          ,
    GLuint   attribIndex  ,
    GLuint   nofComponents,
    GLenum   type         ,
    GLuint   buffer       ,
    GLintptr offset      ,
    GLsizei  stride      ){
  glVertexArrayAttribBinding(vao,attribIndex,attribIndex);
  glEnableVertexArrayAttrib(vao,attribIndex);
  glVertexArrayAttribFormat(vao,
      attribIndex  ,
      nofComponents,
      type         ,
      GL_FALSE     , //normalization
      0            );//relative offset
  glVertexArrayVertexBuffer(vao,
      attribIndex,
      buffer,
      offset,
      stride
      );
}

int main(int argc,char*argv[]){
  SDL_Init(SDL_INIT_VIDEO);//init. video

  uint32_t width = 1024;
  uint32_t height = 768;

  auto window = SDL_CreateWindow("PGRe_examples",0,0,width,height,SDL_WINDOW_OPENGL);

  unsigned version = 450;//context version
  unsigned profile = SDL_GL_CONTEXT_PROFILE_CORE;//context profile
  unsigned flags    = SDL_GL_CONTEXT_DEBUG_FLAG;//context flags
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, version/100    );
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION,(version%100)/10);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK ,profile         );
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS        ,flags           );

  auto context = SDL_GL_CreateContext(window);

  ge::gl::init();

  auto vsSrc = R".(
  #version 460
  
  layout(location=0)in vec3 pos;
  layout(location=1)in vec3 nor;

  out vec3 vColor;

  uniform float scale = 1.f;

  uniform mat4 view = mat4(1.);
  uniform mat4 proj = mat4(1.);

  void main(){
    vColor = nor;
  
    gl_Position = proj*view*vec4(pos*scale,1);
  }

  ).";

  auto fsSrc = R".(
  #version 460
  #line 78

  in vec3 vColor;
  out vec4 fColor;

  void main(){

    fColor = vec4(vColor,1);
  }

  ).";

  auto texVsSrc = R".(
  #version 460

  uniform mat4 view = mat4(1.);
  uniform mat4 proj = mat4(1.);


  out vec2 vTexCoord;
  void main(){

    vTexCoord = vec2(gl_VertexID&1,gl_VertexID>>1);
    gl_Position = proj*view*vec4(vTexCoord,0,1);

  }
  ).";

  auto texFsSrc = R".(
  #version 460

  layout(binding=0)uniform sampler2D tex;

  in vec2 vTexCoord;

  out vec4 fColor;
  void main(){
    fColor = texture(tex,vTexCoord);
  }
  ).";

  auto prg = createProgram({
      createShader(GL_VERTEX_SHADER,vsSrc),
      createShader(GL_FRAGMENT_SHADER,fsSrc),
      });

  auto texPrg = createProgram({
      createShader(GL_VERTEX_SHADER,texVsSrc),
      createShader(GL_FRAGMENT_SHADER,texFsSrc),
      });


  int texWidth;
  int texHeight;
  int texChannels;
  auto fileName = std::string(CMAKE_ROOT_DIR)+"/resources/images/pes.png";
  auto data = stbi_load(fileName.c_str(),&texWidth,&texHeight,&texChannels,0);
  if(!data)std::cerr << "image " << fileName << " could not be loaded!" << std::endl;

  GLenum internalFormat = GL_RGBA;
  GLenum format = GL_RGBA;
  if(texChannels == 4){
    internalFormat = GL_RGBA;
    format         = GL_RGBA;
  }
  if(texChannels == 3){
    internalFormat = GL_RGB;
    format         = GL_RGB;
  }

  GLuint tex;
  glCreateTextures(GL_TEXTURE_2D,1,&tex);
  glTextureParameteri(tex,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
  glTextureParameteri(tex,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
  glTextureImage2DEXT(tex,GL_TEXTURE_2D,
      0,// mipmap level
      internalFormat,//internal format
      texWidth,texHeight,
      0,//border (unused
      format,//format on the CPU
      GL_UNSIGNED_BYTE,//type on CPU
      data);
  

  stbi_image_free(data);

  GLuint scaleUniform = glGetUniformLocation(prg,"scale");


  GLuint viewUniform = glGetUniformLocation(prg,"view");
  GLuint projUniform = glGetUniformLocation(prg,"proj");


  GLuint vbo;
  glCreateBuffers(1,&vbo);
  glNamedBufferData(vbo,sizeof(bunnyVertices),bunnyVertices,GL_DYNAMIC_DRAW);

  GLuint ebo;
  glCreateBuffers(1,&ebo);
  glNamedBufferData(ebo,sizeof(bunnyIndices),bunnyIndices,GL_DYNAMIC_DRAW);


  GLuint vao;
  glCreateVertexArrays(1,&vao);
 
  setVertexAttribute(vao,0,3,GL_FLOAT,vbo,0              ,sizeof(float)*6);
  setVertexAttribute(vao,1,3,GL_FLOAT,vbo,sizeof(float)*3,sizeof(float)*6);

  glVertexArrayElementBuffer(vao,ebo);

  glEnable(GL_DEPTH_TEST);

  float scale = 1.f;

  glm::mat4 view = glm::mat4(1.f);
  glm::mat4 proj = glm::mat4(1.f);

  float angleY = 0.f;
  float angleX = 0.f;
  float distance = 2;


  float aspectRatio = (float)width / (float) height;

  float near = 0.1f;
  float far  = 1000.f;
  proj = glm::perspective(glm::half_pi<float>(),aspectRatio,near,far);

  bool running = true;
  while(running){//Main Loop
    SDL_Event event;
    while(SDL_PollEvent(&event)){ // Event Loop
      if(event.type == SDL_QUIT)
        running = false;
      if(event.type == SDL_KEYDOWN){
        if(event.key.keysym.sym == SDLK_a)scale -= .01f;
        if(event.key.keysym.sym == SDLK_d)scale += .01f;
      }
      if(event.type == SDL_MOUSEMOTION){
        if(event.motion.state == SDL_BUTTON_RMASK){
          float sensitivity = 0.1f;
          angleX += sensitivity*event.motion.yrel;
          angleY += sensitivity*event.motion.xrel;
        }
      }
    }


    auto T  = glm::translate(glm::mat4(1.f),glm::vec3(0.f,0.f,-distance));
    auto Ry = glm::rotate(glm::mat4(1.f),glm::radians(angleY),glm::vec3(0.f,1.f,0.f));
    auto Rx = glm::rotate(glm::mat4(1.f),glm::radians(angleX),glm::vec3(1.f,0.f,0.f));
    view = 
      T*Rx*Ry;

    glClearColor(0.3f,0.3f,0.3f,1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glProgramUniform1f(prg,scaleUniform,scale);

    glProgramUniformMatrix4fv(prg,viewUniform,1,GL_FALSE,glm::value_ptr(view));
    glProgramUniformMatrix4fv(prg,projUniform,1,GL_FALSE,glm::value_ptr(proj));

    glUseProgram(prg);
    glBindVertexArray(vao);
    auto nofIndices = sizeof(bunnyIndices)/sizeof(uint32_t);
    glDrawElements(GL_TRIANGLES,nofIndices,GL_UNSIGNED_INT,nullptr);

    glFrontFace(GL_CCW);
    glEnable(GL_CULL_FACE);

    glBindTextureUnit(0,tex);
    glUseProgram(texPrg);
    glProgramUniformMatrix4fv(texPrg,glGetUniformLocation(texPrg,"view"),1,GL_FALSE,glm::value_ptr(view));
    glProgramUniformMatrix4fv(texPrg,glGetUniformLocation(texPrg,"proj"),1,GL_FALSE,glm::value_ptr(proj));
    glDrawArrays(GL_TRIANGLE_STRIP,0,4);

    SDL_GL_SwapWindow(window);
  }

  glDeleteProgram(prg);

  SDL_GL_DeleteContext(context);

  SDL_DestroyWindow(window);

  return 0;
}
