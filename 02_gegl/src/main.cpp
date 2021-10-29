#include <iostream>
#include <SDL.h>

#include <geGL/geGL.h>
#include <geGL/StaticCalls.h>

using namespace ge::gl;

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


int main(int argc,char*argv[]){
  SDL_Init(SDL_INIT_VIDEO);//init. video

  auto window = SDL_CreateWindow("PGRe_examples",0,0,1024,768,SDL_WINDOW_OPENGL);

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

  out vec3 vColor;
  void main(){
    if(gl_VertexID == 0)vColor = vec3(1,0,0);
    if(gl_VertexID == 1)vColor = vec3(0,1,0);
    if(gl_VertexID == 2)vColor = vec3(0,0,1);

    gl_Position = vec4(gl_VertexID&1,gl_VertexID>>1,0,1);
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

  auto prg = createProgram({
      createShader(GL_VERTEX_SHADER,vsSrc),
      createShader(GL_FRAGMENT_SHADER,fsSrc),
      });

  GLuint vao;
  glCreateVertexArrays(1,&vao);
  

  bool running = true;
  while(running){//Main Loop
    SDL_Event event;
    while(SDL_PollEvent(&event)){ // Event Loop
      if(event.type == SDL_QUIT)
        running = false;
    }

    glClearColor(0.3f,0.3f,0.3f,1.f);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(prg);
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES,0,3);

    SDL_GL_SwapWindow(window);
  }

  glDeleteProgram(prg);

  SDL_GL_DeleteContext(context);

  SDL_DestroyWindow(window);

  return 0;
}
