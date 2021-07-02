#include <unistd.h>
#include <sys/time.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <SDL2/SDL.h>
#include <math.h>

SDL_Window *wnd_sdl;
SDL_GLContext context;
volatile char exitflag = 0;
int wnd_w = 600; //ширина окна (в пикселях)
int wnd_h = 600; //высота окна (в пикселях)

double mag_energy(double phi, double alp, double p){
  return -p*cos(phi-alp) - cos(phi)*cos(phi);
}

#define NPOINTS 3600
double yval[NPOINTS+1];
double min, max;
int idxmin1, idxmin2;

void findmin2(float alp){
  double res, min;
  int ires = 0;
  if((idxmin1 > NPOINTS/4) && (idxmin1 < 3*NPOINTS/4)){
    ires = 0; min = yval[ires];
    for(int i=0; i<NPOINTS/4+2; i++){
      if(yval[i] < min){min = yval[i]; ires=i;}
    }
    for(int i=3*NPOINTS/4 - 1; i<NPOINTS; i++){
      if(yval[i] < min){min = yval[i]; ires=i;}
    }
    if((ires > NPOINTS/4) && (ires < 3*NPOINTS/4)){idxmin2=idxmin1; return;}
  }else{
    ires = NPOINTS/4-1; min = yval[ires];
    min = yval[ires];
    for(int i=NPOINTS/4; i<3*NPOINTS/4 + 2; i++){
      if(yval[i] < min){min = yval[i]; ires=i;}
    }
    if((ires < NPOINTS/4) || (ires > 3*NPOINTS/4)){idxmin2=idxmin1; return;}
  }
  idxmin2=ires;
}
void calc_points(float alp, double p){
  double x;
  idxmin1 = 0;
  min = max = mag_energy(0, alp, p);
  for(int i=0; i<NPOINTS+1; i++){
    x = i * 2*M_PI/NPOINTS;
    yval[i] = mag_energy(x, alp, p);
    if(yval[i] < min){min = yval[i]; idxmin1=i;}
    if(yval[i] > max)max = yval[i];
  }
  findmin2(alp);
}

void draw(){
  static double phi = 0;
  calc_points(phi, 1);
  double xmin = 2*M_PI*idxmin1/NPOINTS;
  double amin = 2*M_PI*idxmin2/NPOINTS;
  double dy = max - min;
  double minval = min - dy*0.2;
  dy = max - minval;
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluOrtho2D(-dy,dy, -dy,dy);
  glColor3f(0,0,0);
  glBegin(GL_LINES);
    glVertex2f(-dy, 0);
    glVertex2f(dy, 0);
    glVertex2f(0, -dy);
    glVertex2f(0, dy);
    glColor3f(1,0,0);
    glVertex2f(0,0);
    glVertex2f(dy*cos(phi), dy*sin(phi));
    glColor3f(0,0,1);
    glVertex2f(0,0);
    glVertex2f(dy*cos(amin), dy*sin(amin));
    glColor3f(0,1,0);
    glVertex2f(0,0);
    glVertex2f(dy*cos(xmin), dy*sin(xmin));
  glEnd();
  glColor3f(0,0,0);
  glBegin(GL_LINE_STRIP);
    double alp, x, y;
    for(int i=0; i<NPOINTS+1; i++){
      alp = 2*M_PI*i/NPOINTS;
      glVertex2f((yval[i]-minval)*cos(alp), (yval[i]-minval)*sin(alp));
    }
  glEnd();
  phi += 0.01;
}

int main( int argc, char *argv[]){
  wnd_sdl = SDL_CreateWindow("Hall model",SDL_WINDOWPOS_UNDEFINED,SDL_WINDOWPOS_UNDEFINED,
                            wnd_w, wnd_h, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
  context=SDL_GL_CreateContext(wnd_sdl);
  SDL_GL_MakeCurrent(wnd_sdl, context);
  
  
  glClearColor(1,1,1,0);
  glEnable(GL_BLEND); //разрешение полупрозрачности
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  SDL_Event event;
  while(!exitflag){
    while(SDL_PollEvent(&event)){
      if(event.type == SDL_QUIT){exitflag = 1;}
    }
    glClear(GL_COLOR_BUFFER_BIT);
    draw();
    glFlush();
    SDL_GL_SwapWindow(wnd_sdl);
    usleep(1000);
  }
  printf("\n");
  return 0;
}
