/* LICENSE: GPLv2
 * Required libraries: OpenGL1, gtk3, SDL2
 * Compile command: gcc src/main.c -Os -Wall `pkg-config --cflags --libs gtk+-3.0` -lGL -lGLU -lm -lSDL2 -o res/prog64
 * Author: COKPOWEHEU
 */
#include <gtk/gtk.h>
#include <unistd.h>
#include <sys/time.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <SDL2/SDL.h>
#include <math.h>
#include <locale.h>

SDL_Window *wnd_sdl;
SDL_GLContext context;
volatile char exitflag = 0;
int wnd_w = 700; //ширина окна (в пикселях)
int wnd_h = 900; //высота окна (в пикселях)

int bmp_w = 450; //ширина bmp (в пикселях)
int bmp_h = 600; //высота bmp (в пикселях)
double bmp_w_m = 0.1; //ширина bmp (в метрах)

const double field_max = 1000; //максимальное внешнее поле
double Hc = 400; //коэрцитивная сила
double Haniz = 300; //поле анизотропии
double dfield = 2; //шаг анимации по полю
double H_angle_deg = 70; //угол между легкой осью и внешним полем (градусов)
double current_angle_deg = -30; //угол между легкой осью и направлением тока (градусов)
const double linewidth = 2;
char marker_flag = 1;

double clac_p(double H){
  return H/Haniz;
}
double mag_energy(double phi, double alp, double p){
  return -p*cos(phi-alp) - cos(phi)*cos(phi);
}

#define NPOINTS 3600
double yval[NPOINTS+1];
double min, max;
int idxmin1, idxmin2;
#define XPOINTS 2000
double minangle[XPOINTS][2];
#define INV_FIELD 1

void findmin2(float alp){
  double min;
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
  if(p < 0){alp+=M_PI; p=-p;}
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
double calc_angle(float alp, double H, char rise){
  int idx = idxmin1;
  if(rise){
    if((H > 0) && (H < Hc))idx = idxmin2;
  }else{
    if((H < 0) && (H >-Hc))idx = idxmin2;
  }
  return idx*2*M_PI/NPOINTS;
}

void draw_ring(float x, float y, float r, int N){
  float alp;
  glBegin(GL_LINE_STRIP);
  for(int i=0; i<=N; i++){
    alp = 2*M_PI*i/N;
    glVertex2f(x+r*cos(alp), y+r*sin(alp));
  }
  glEnd();
}
void draw_surface(float alp){
  glPushMatrix();
  double xmin = 2*M_PI*idxmin1/NPOINTS;
  double amin = 2*M_PI*idxmin2/NPOINTS;
  double dy = max - min;
  double minval = min - dy*0.2;
  dy = max - minval;
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluOrtho2D(-dy,dy, -dy,dy);
  glLineWidth(linewidth);
  glBegin(GL_LINES);
    glColor3f(1,0,0);
    glVertex2f(0,0);
    glVertex2f(dy*cos(alp), dy*sin(alp));
    glColor3f(0,0,1);
    glVertex2f(0,0);
    glVertex2f(dy*cos(amin), dy*sin(amin));
    glColor3f(0,1,0);
    glVertex2f(0,0);
    glVertex2f(dy*cos(xmin), dy*sin(xmin));
  glEnd();
  glColor3f(0,0,0);
  glBegin(GL_LINE_STRIP);
    double bet;
    for(int i=0; i<NPOINTS+1; i++){
      bet = 2*M_PI*i/NPOINTS;
      glVertex2f((yval[i]-minval)*cos(bet), (yval[i]-minval)*sin(bet));
    }
  glEnd();
  
  glLineWidth(1);
  
  glPopMatrix();
}
//double mag_energy(double phi, double alp, double p)
double findmin(double x1, double x2, double alp, double p){
  if(p<0){alp+=M_PI; p=-p;}
  if(x1 > x2){double temp = x1; x1=x2; x2=temp;}
  double dx = (x2-x1)/100;
  if((x2-x1) < 0.01)return (x1+x2)/2;
  double min, xmin;
  double x;
  double y;
  xmin = x = x1-dx/2; min = mag_energy(x, alp, p);
  for(x=x1; x<=x2+dx/2; x+=dx){
    y = mag_energy(x, alp, p);
    if(y<min){min=y; xmin=x;}
  }
  if((xmin < x1)||(xmin>x2))return xmin;
  return findmin(xmin-dx, xmin+dx, alp, p);
}
double findlocalmin(double alp, double p, double xmin){
  double res;
  if((xmin > M_PI/2) && (xmin < 3*M_PI/2)){
    res = findmin(-M_PI/2, M_PI/2, alp, p);
    if((res < -M_PI/2) || (res>M_PI/2))return xmin;
    if(res < 0)res+=2*M_PI;
  }else{
    res = findmin(M_PI/2, 3*M_PI/2, alp, p);
    if((res <M_PI/2)||(res>3*M_PI/2))return xmin;
  }
  return res;
}

void calc_graph(){
  int i=0;
  double alp = H_angle_deg*M_PI/180;
  char sign = 1;
  if(cos(alp)<0)sign=-1;
  double p = clac_p((i-XPOINTS/2)*field_max*2/XPOINTS);

  for(i=0; i<XPOINTS; i++){
    p = clac_p((i-XPOINTS/2)*field_max*2/XPOINTS);
    if(sign*p > 0){
      minangle[i][0] = findmin(-M_PI/2, M_PI/2, alp, p);
      if(minangle[i][0] < 0)minangle[i][0] += 2*M_PI;
    }else{
      minangle[i][0] = findmin(M_PI/2, 3*M_PI/2, alp, p);
    }
    minangle[i][1] = findlocalmin(alp, p, minangle[i][0]);
  }
}

void draw_graph(double H, char rise){
  double IAangle = current_angle_deg*M_PI/180;
  glViewport(0,wnd_w, wnd_w, wnd_h-wnd_w);

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  //gluOrtho2D(0,XPOINTS, minangle_min, minangle_max);
  gluOrtho2D(0, XPOINTS, -1.02, 1.02);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glLineWidth(linewidth);
  glColor3f(0,1,0);
  glBegin(GL_LINE_STRIP);
    for(int i=0; i<XPOINTS; i++)
      glVertex2f(i, sin(2*(IAangle -minangle[i][1])));
  glEnd();
  glColor3f(0,0,0);
  glBegin(GL_LINE_STRIP);
    for(int i=0; i<XPOINTS; i++)
      glVertex2f(i, sin(2*(IAangle -minangle[i][0])));
  glEnd();
  glColor3f(1,0,0);
  int Hci = XPOINTS/2 - XPOINTS*Hc/field_max/2;
  if(Hci > 0 && Hci < XPOINTS/2){
    glBegin(GL_LINES);
      glVertex2f(Hci, sin(2*(IAangle -minangle[Hci][0])));
      glVertex2f(Hci, sin(2*(IAangle -minangle[Hci][1])));
      glVertex2f(XPOINTS-Hci, sin(2*(IAangle -minangle[XPOINTS - Hci][0])));
      glVertex2f(XPOINTS-Hci, sin(2*(IAangle -minangle[XPOINTS - Hci][1])));
    glEnd();
  }
  glColor3f(0,0,0);
  glLineWidth(1);
  
  if(marker_flag){
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(-field_max, field_max, -1.02, 1.02);
    /*glBegin(GL_LINES);
      glVertex2f(H, -1.03);
      glVertex2f(H, 1.03);
    glEnd();
    */
    int Hci = XPOINTS/2 - XPOINTS*H/field_max/2;
    int minnum = 0;
    if((H>-Hc) && (H<Hc)){
      if(rise && (H>0))minnum = 1;
      if(!rise &&(H<0))minnum = 1;
    }
    const float size = 20;
    float dx = (size/wnd_w)*field_max;
    float dy = (size/(wnd_h-wnd_w));
    float y = sin(2*(IAangle -minangle[XPOINTS - Hci][minnum]));
    glBegin(GL_LINE_STRIP);
      glVertex2f(H-dx, y);
      glVertex2f(H, y-dy);
      glVertex2f(H+dx, y);
      glVertex2f(H, y+dy);
      glVertex2f(H-dx, y);
    glEnd();
  }
}

//Отрисовка "направления намагниченности" (стрелки компаса)
//  alp - угол
const float compass_l=0.5;
void draw_compass(float alp){
  const float w = 0.1;
  glPushMatrix();
  glRotatef(alp*180/M_PI, 0, 0, 1);
  glBegin(GL_TRIANGLES);
    glColor3f(1,0,0);
    glVertex2f(compass_l, 0);
    glVertex2f(0, w);
    glVertex2f(0, -w);
    glColor3f(0,0,1);
    glVertex2f(0, w);
    glVertex2f(0, -w);
    glVertex2f(-compass_l,0);
  glEnd();
  glPopMatrix();
}

//отрисовка "направления поля анизотропии" (горизонтальной стрелки)
//  dir_left - флаг направления (0-вправо, 1-влево)
void draw_arrow(char dir_left){
  glPushMatrix();
  if(dir_left){
    glScalef(-1, 1, 1);
  }
  
  const float fld_l = 1;
  const float arrow_w = 0.1;
  const float arrow_dl= 0.2;
  const float arrow_dw = 0.4;
  glColor4f(0, 0.5, 0.5, 0.8);
  glScalef(1, Haniz/field_max, 0);
  glBegin(GL_TRIANGLE_FAN);
    glVertex2f(-fld_l+arrow_dl, -arrow_w);
    glVertex2f(-fld_l+arrow_dl, -arrow_dw);
    glVertex2f(-fld_l, 0);
    glVertex2f(-fld_l+arrow_dl, arrow_dw);
    glVertex2f(-fld_l+arrow_dl, arrow_w);
#if INV_FIELD
    glVertex2f(+fld_l-arrow_dl, arrow_w);
    glVertex2f(+fld_l-arrow_dl, -arrow_w);
  glEnd();
  glBegin(GL_TRIANGLE_FAN);
    glVertex2f(+fld_l-arrow_dl, arrow_w);
    glVertex2f(+fld_l-arrow_dl, arrow_dw);
    glVertex2f(+fld_l, 0);
    glVertex2f(+fld_l-arrow_dl, -arrow_dw);
    glVertex2f(+fld_l-arrow_dl, -arrow_w);
#else
    glVertex2f(fld_l, arrow_w);
    glVertex2f(fld_l, -arrow_w);
#endif
  glEnd();
  glPopMatrix();
}

//отрисовка "направления поля анизотропии" в зависимости от внешних параметров
//  alp - угол между внешним полем и легкой осью
//  H - величина внешнего поля
//  rise - флаг направления изменения поля (0-уменьшение, 1-увеличение)
void draw_springs(float alp, float H, char rise){
  /*H = H*cos(alp);
  if(cos(alp) < 0)rise=!rise;
  char dir_left;
  if(rise){ //гистерезис
    if(H > Hc)dir_left = 0; else dir_left = 1;
  }else{
    if(H <-Hc)dir_left = 1; else dir_left = 0;
  }*/
  draw_arrow(0);
}

//отрисовка внешнего поля (магнита)
//  alp - угол между внешним полем и легкой осью
//  fld - величина поля (дробное число от 0.0 до 1.0)
const float fld_l = 0.7;
void draw_field(float alp, float fld){
#ifdef INV_FIELD
  if(fld > 0){
#else
  if(fld < 0){
#endif
    fld = -fld;
    alp = alp+M_PI;
  }
  const float w = 0.2;
  const float dx = 0.05;
  glPushMatrix();
  glRotatef(alp*180/M_PI, 0, 0, 1);
  glBegin(GL_TRIANGLE_FAN);
    glColor4f(1,0,0, 0.3);
    glVertex2f(-2, -w);
    glVertex2f(-fld_l-dx, -w);
    glVertex2f(-fld_l, -w+dx);
    glVertex2f(-fld_l, w-dx);
    glVertex2f(-fld_l-dx, w);
    glVertex2f(-2, w);
  glEnd();
  glBegin(GL_TRIANGLE_FAN);  
    glColor4f(0,0,1,0.3);
    glVertex2f(2, -w);
    glVertex2f(fld_l+dx, -w);
    glVertex2f(fld_l, -w+dx);
    glVertex2f(fld_l, w-dx);
    glVertex2f(fld_l+dx, w);
    glVertex2f(2, w);
  glEnd();
  const float arrow_w = 0.1;
  const float arrow_dl= 0.2;
  const float arrow_dw = 0.2;
  glScalef(1, fld, 0);
  glColor4f(0.5, 0.5, 0.5, 0.3);
  glBegin(GL_TRIANGLE_FAN);
    glVertex2f(-fld_l, 0);
    glVertex2f(-fld_l+arrow_dl, arrow_dw);
    glVertex2f(-fld_l+arrow_dl, arrow_w);
    glVertex2f(fld_l, arrow_w);
    glVertex2f(fld_l, -arrow_w);
    glVertex2f(-fld_l+arrow_dl, -arrow_w);
    glVertex2f(-fld_l+arrow_dl, -arrow_dw);
  glEnd();
  glPopMatrix();
}

//отрисовка круга
//  x, y - координаты центра
//  R - радиус
//  n - количество вершин
void draw_circle(float x, float y, float R, int n){
  float dalp = 2*M_PI/n;
  glBegin(GL_TRIANGLE_FAN);
  for(float i=0; i<2*M_PI; i+=dalp){
    glVertex2f(x+R*cos(i), y+R*sin(i));
  }
  glEnd();
}

//отрисовка холловского креста
//  alp - угол между протеканием тока и легкой осью
void draw_cross(float alp){
  const float l = 0.6;
  const float w = 0.15;
  glPushMatrix();
  glRotatef(alp*180/M_PI, 0, 0, 1);
  glBegin(GL_TRIANGLE_FAN);
    glColor4f(1, 0.84, 0, 0.7);
    glVertex2f(-w, w);
    glVertex2f(-w, l);
    glVertex2f(w, l);
    glVertex2f(w, w);
    
    glVertex2f(l, w);
    glVertex2f(l, -w);
    glVertex2f(w, -w);
    glVertex2f(w, -l);
    
    glVertex2f(-w, -l);
    glVertex2f(-w, -w);
    glVertex2f(-l, -w);
    glVertex2f(-l, w);
  glEnd();
  static float pos = 0;
  const float dist = 0.4;
  const float R = dist/10;
  const int N = 6;
  glColor4f(0, 1, 0, 0.4);
  for(float i=-l-pos; i<l-R/2; i+=dist){
    if(i<-l+R/2)continue;
    draw_circle(i, 0, R, N);
  }
  pos += l*0.01;
  if(pos > dist)pos = 0;
  glPopMatrix();
}

//отрисовка модели в целом
//  H - величина внешнего поля
//  rise - флаг направления изменения (0-уменьшение, 1-увеличение)
void draw_model(double H, char rise){
  glViewport(0,0, wnd_w, wnd_w);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluOrtho2D(-1,1, -1,1);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  double ha = H_angle_deg*M_PI/180;
  double ca = current_angle_deg*M_PI/180;
  double alp = calc_angle(ha, H, rise);
  draw_cross(ca);
  draw_field(ha, H/field_max);
  draw_compass(alp);
  draw_springs(ha, H, rise);
  draw_surface(alp);
}

//отрисовка вообще всего
void draw(){
  static double H = -field_max*1.2;
  static char rise = 0;
  double dH = dfield;
  
  calc_points(H_angle_deg*M_PI/180, clac_p(H));
  draw_model(H, rise);
  draw_graph(H, rise);
  if(rise){
    H += dH;
    if(H >= field_max){H = field_max; rise = 0;}
  }else{
    H -= dH;
    if(H <=-field_max){H =-field_max; rise = 1; calc_graph(); }
  }
}

//экспорт серии скриншотов в bmp-картинки
struct BmpHeader{ 
  uint16_t  bfType;        // смещение 0 от начала файла
  uint32_t  bfSize;        // смещение 2 от начала файла, длина 4
  uint16_t  bfReserved1;   // 0
  uint16_t  bfReserved2;   // 0
  uint32_t  bfOffBits;     // смещение 10 от начала файла, длина 4
}__attribute__((packed));

struct BmpInfo{
  uint32_t  biSize;         // размер данной структуры
  uint32_t  biWidth;        // ширина изображения
  uint32_t  biHeight;       // высота изображения
  uint16_t  biPlanes;       // количество плоскостей (???)
  uint16_t  biBitCount;     // бит на пиксель
  uint32_t  biCompression;  // сжатие (???)
  uint32_t  biSizeImage;    // размер изображения
  uint32_t  biXPelsPerMeter;// разрешение по X (пикселей на метр)
  uint32_t  biYPelsPerMeter;// разрешение по Y (пикселей на метр)
  uint32_t  biClrUsed;      // важно только для сжатых картинок
  uint32_t  biClrImportant; // важно только для сжатых картинок
}__attribute__((packed));
char save(char *filename){
  FILE *pf = fopen(filename, "wb");
  if(pf == NULL)return 0;
  uint16_t buf[wnd_w*wnd_h];
  //bmp_w_m
  struct BmpHeader header = {
    .bfType = ('M'<<8) + 'B',
    .bfSize = sizeof(struct BmpHeader)+sizeof(struct BmpInfo)+sizeof(buf),
    .bfOffBits = sizeof(struct BmpHeader)+sizeof(struct BmpInfo),
  };
  struct BmpInfo info = {
    .biSize = sizeof(struct BmpInfo),
    .biWidth = wnd_w,
    .biHeight = wnd_h,
    .biPlanes = 1,
    .biBitCount = 16,
    .biCompression = 0, //?
    .biSizeImage = sizeof(buf),
    .biXPelsPerMeter = bmp_w/bmp_w_m,
    .biYPelsPerMeter = bmp_h/bmp_w_m,
    .biClrUsed = 0, //?
    .biClrImportant = 0, //?
  };
#ifdef _WIN32
  //костыль для mingw
  #define GL_UNSIGNED_SHORT_5_6_5 0x8363
#endif
  glReadPixels(0, 0, wnd_w, wnd_h, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, &buf);
  
  fwrite(&header, sizeof(header), 1, pf);
  fwrite(&info, sizeof(info), 1, pf);
  fwrite(buf, sizeof(buf), 1, pf);
  
  fclose(pf);
  return 1;
}

///////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////
///////////////////   INTERFACE   /////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////

void realize(GtkWidget *widget){}

void OnClose(void){
  exitflag = 1;
}

//колбэк на изменение полей ввода чисел
void spined_OnChange(GtkWidget *obj, gpointer data){
  double *val = data;
  *val = gtk_spin_button_get_value(GTK_SPIN_BUTTON(obj));
  calc_graph();
}

//колбэк на изменение полей ввода углов
void angle_OnChange(GtkWidget *obj, gpointer data){
  double *val = data;
  *val = gtk_spin_button_get_value(GTK_SPIN_BUTTON(obj));
  if(*val > 360){
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(obj), *val - 360);
  }else if(*val < -360){
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(obj), *val + 360);
  }
  calc_graph();
}

//колбэк на нажатие клавиши 'Save'
void sbtn_click(GtkButton *button, gpointer user_data){
  double H = -field_max;
  char rise = 1;
  double dH = 50;
  const char *str = gtk_entry_get_text((GtkEntry*)user_data);
  size_t namelen = strlen(str);
  char *fname = malloc(sizeof(char) * (namelen + strlen("10000.bmp")));
  strcpy(fname, str);
  int bkp_w, bkp_h;
  
  SDL_GetWindowSize(wnd_sdl, &bkp_w, &bkp_h);
  wnd_w = bmp_w; wnd_h = bmp_h;
  SDL_SetWindowSize(wnd_sdl, wnd_w, wnd_h);

  int i = 0;
  while(1){
    glClear(GL_COLOR_BUFFER_BIT);
    calc_points(H_angle_deg*M_PI/180, clac_p(H));
    draw_model(H, rise);
    draw_graph(H, rise);
    if(rise){
      H += dH;
      if(H >= field_max){H = field_max; rise = 0;}
    }else{
      H -= dH;
      if(H<-field_max)break;
    }
    glFlush();
    i++;
    sprintf(&fname[namelen], "%04d.bmp", i);
    if( !save(fname) ){
      fprintf(stderr, "Can not write file [%s]\n", fname);
      i = -1;
      break;
    }
    printf("[%s] saved\n", fname);
  }
  
  wnd_w = bkp_w; wnd_h = bkp_h;
  SDL_SetWindowSize(wnd_sdl, wnd_w, wnd_h);
  
  free(fname);
#ifndef _WIN32
  //кажется, imagemagick и ffmpeg под винду устанавливают редко
  if(i >= 0){
    printf("Now you can run 'convert %s*.bmp -fuzz 10%% -layers Optimize res.gif' to convert this into res.gif\n", str);
    printf("Or 'ffmpeg -loop 1 -t 60 -framerate 7 -i %s%%4d.bmp res.mov' to convert into res.mov\n", str);
  }
#endif
}

int main( int argc, char *argv[]){
  wnd_sdl = SDL_CreateWindow("Hall model",SDL_WINDOWPOS_UNDEFINED,SDL_WINDOWPOS_UNDEFINED,
                            wnd_w, wnd_h, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
  context=SDL_GL_CreateContext(wnd_sdl);
  SDL_GL_MakeCurrent(wnd_sdl, context);
  
  char *loc = strdup(setlocale(LC_ALL, NULL));
  GtkWidget *wnd_gtk, *fixed;  
  gtk_init(&argc, &argv);
  setlocale(LC_ALL, loc);
  free(loc);
  
  wnd_gtk = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(wnd_gtk), "Hall control");
  gtk_window_set_default_size(GTK_WINDOW(wnd_gtk), 400, 600);
  gtk_window_move(GTK_WINDOW(wnd_gtk), 0, 0);
  
  
  fixed = gtk_fixed_new();
  gtk_container_add(GTK_CONTAINER(wnd_gtk), fixed);
  
  GtkWidget *aslabel = gtk_label_new("Animation speed:");
  gtk_fixed_put(GTK_FIXED(fixed), aslabel, 0, 0);
  GtkWidget *animspeed = gtk_spin_button_new_with_range(0, 500, 0.5);
  gtk_spin_button_set_update_policy(GTK_SPIN_BUTTON(animspeed), GTK_UPDATE_IF_VALID);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(animspeed), dfield);
  g_signal_connect(G_OBJECT(animspeed), "value-changed", G_CALLBACK(spined_OnChange), &dfield);
  gtk_fixed_put(GTK_FIXED(fixed), animspeed, 200, 0);
  
  GtkWidget *laalabel = gtk_label_new("Light axis angle");
  gtk_fixed_put(GTK_FIXED(fixed), laalabel, 0, 40);
  GtkWidget *ha = gtk_spin_button_new_with_range(-720, 720, 0.5);
  gtk_spin_button_set_update_policy(GTK_SPIN_BUTTON(ha), GTK_UPDATE_IF_VALID);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(ha), H_angle_deg);
  g_signal_connect(G_OBJECT(ha), "value-changed", G_CALLBACK(angle_OnChange), &H_angle_deg);
  gtk_fixed_put(GTK_FIXED(fixed), ha, 200, 40);
  
  GtkWidget *calabel = gtk_label_new("Current angle");
  gtk_fixed_put(GTK_FIXED(fixed), calabel, 0, 80);
  GtkWidget *ca = gtk_spin_button_new_with_range(-720, 720, 0.5);
  gtk_spin_button_set_update_policy(GTK_SPIN_BUTTON(ca), GTK_UPDATE_IF_VALID);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(ca), current_angle_deg);
  g_signal_connect(G_OBJECT(ca), "value-changed", G_CALLBACK(angle_OnChange), &current_angle_deg);
  gtk_fixed_put(GTK_FIXED(fixed), ca, 200, 80);
  
  GtkWidget *hclabel = gtk_label_new("Coercive field");
  gtk_fixed_put(GTK_FIXED(fixed), hclabel, 0, 120);
  GtkWidget *hc = gtk_spin_button_new_with_range(0, 10000, 1);
  gtk_spin_button_set_update_policy(GTK_SPIN_BUTTON(hc), GTK_UPDATE_IF_VALID);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(hc), Hc);
  g_signal_connect(G_OBJECT(hc), "value-changed", G_CALLBACK(spined_OnChange), &Hc);
  gtk_fixed_put(GTK_FIXED(fixed), hc, 200, 120);
  
  GtkWidget *anlabel = gtk_label_new("Anisotropy field");
  gtk_fixed_put(GTK_FIXED(fixed), anlabel, 0, 160);
  GtkWidget *an = gtk_spin_button_new_with_range(0, 10000, 1);
  gtk_spin_button_set_update_policy(GTK_SPIN_BUTTON(an), GTK_UPDATE_IF_VALID);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(an), Haniz);
  g_signal_connect(G_OBJECT(an), "value-changed", G_CALLBACK(spined_OnChange), &Haniz);
  gtk_fixed_put(GTK_FIXED(fixed), an, 200, 160);
  
  GtkWidget *bmpname = gtk_entry_new();
  gtk_entry_set_text((GtkEntry*)(bmpname), "bmp/a");
#ifdef _WIN32
  gtk_entry_set_text((GtkEntry*)(bmpname), "../bmp/a");
#endif
  gtk_fixed_put(GTK_FIXED(fixed), bmpname, 0, 200);
  GtkWidget *sbtn = gtk_button_new_with_label("Save");
  g_signal_connect(G_OBJECT(sbtn), "clicked", G_CALLBACK(sbtn_click), bmpname);
  gtk_fixed_put(GTK_FIXED(fixed), sbtn, 200, 200);

  
  gtk_widget_show_all(wnd_gtk);
  g_signal_connect(G_OBJECT(wnd_gtk), "destroy", G_CALLBACK(OnClose), NULL);
  
  
  glClearColor(1,1,1,0);
  glEnable(GL_BLEND); //разрешение полупрозрачности
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  SDL_Event event;
  while(!exitflag){
    while(gtk_events_pending())gtk_main_iteration_do(0);
    while(SDL_PollEvent(&event)){
      if(event.type == SDL_QUIT){exitflag = 1;}
    }
    glClear(GL_COLOR_BUFFER_BIT);
    draw();
    glFlush();
    SDL_GL_SwapWindow(wnd_sdl);
    //usleep(10000);
  }
  printf("\n");
  return 0;
}
