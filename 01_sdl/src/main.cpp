#include <iostream>
#include <SDL.h>

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

  using GLCLEAR      = void(*)(uint32_t);
  using GLCLEARCOLOR = void(*)(float,float,float,float);


  GLCLEAR      glClear      = (GLCLEAR     )SDL_GL_GetProcAddress("glClear"     );
  GLCLEARCOLOR glClearColor = (GLCLEARCOLOR)SDL_GL_GetProcAddress("glClearColor");


  bool running = true;
  while(running){//Main Loop
    SDL_Event event;
    while(SDL_PollEvent(&event)){ // Event Loop
      if(event.type == SDL_QUIT)
        running = false;
    }
    #define GL_COLOR_BUFFER_BIT     0x00004000
    glClearColor(0.0f,1.0f,0.0f,1.f);
    glClear(GL_COLOR_BUFFER_BIT);

    SDL_GL_SwapWindow(window);
  }

  SDL_GL_DeleteContext(context);

  SDL_DestroyWindow(window);

  return 0;
}
