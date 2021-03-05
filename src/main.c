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
double Hc = 40; //коэрцитивная сила
double Haniz = 100; //поле анизотропии
double dfield = 2; //шаг анимации по полю
double H_angle_deg = 70; //угол между легкой осью и внешним полем (градусов)
double current_angle_deg = -30; //угол между легкой осью и направлением тока (градусов)

//Расчет угла между вектором намагниченности и легкой осью
//  alp - угол между внешним полем и легкой осью
//  H - величина внешнего поля
//  rise - направление поля (1-нарастает в положительную сторону, 0-в отрицательную), для получения гистерезиса
double calc_angle(float alp, double H, char rise){
  double Hx, Hy;
  Hx = H*cos(alp);
  Hy = H*sin(alp);
  char rise_flag = rise;
  if(cos(alp)<0)rise_flag = !rise_flag; //если знак поля меняется из-за угла, надо проигнорировать
  if(rise_flag){
    if(Hx > Hc)Hx += Haniz; else Hx -= Haniz;
  }else{
    if(Hx <-Hc)Hx -= Haniz; else Hx += Haniz;
  }
  double bet = atan2(Hy, Hx);
  return bet;
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

//отрисовка "направления поля анизотропии" в зависимости от внешних параметров
//  alp - угол между внешним полем и легкой осью
//  H - величина внешнего поля
//  rise - флаг направления изменения поля (0-уменьшение, 1-увеличение)
void draw_springs(float alp, float H, char rise){
  H = H*cos(alp);
  if(cos(alp) < 0)rise=!rise;
  char dir_left;
  if(rise){ //гистерезис
    if(H > Hc)dir_left = 0; else dir_left = 1;
  }else{
    if(H <-Hc)dir_left = 1; else dir_left = 0;
  }
  draw_arrow(dir_left);
}

//отрисовка внешнего поля (магнита)
//  alp - угол между внешним полем и легкой осью
//  fld - величина поля (дробное число от 0.0 до 1.0)
const float fld_l = 0.7;
void draw_field(float alp, float fld){
  if(fld < 0){
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
}

//отрисовка графиков
//  H - величина внешнего поля
//  rise - флаг направления изменения (0-уменьшение, 1-увеличение)
void draw_graph(double H, char rise){
  const float df = field_max/1000;
  glViewport(0,wnd_w, wnd_w, wnd_h-wnd_w);
  double alpha = H_angle_deg*M_PI/180;
  double IAangle = current_angle_deg*M_PI/180;
  double beta = calc_angle(alpha, H, rise);
  /*glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluOrtho2D(-field_max,field_max, -M_PI, M_PI);
  glColor3f(0,1,0);
  glBegin(GL_LINE_STRIP);
  for(float f = -field_max; f<3*field_max; f+= df){
    float fr = f;
    char rise = 1;
    if(f > field_max){fr = 2*field_max - f; rise = 0;}
    double bet = calc_angle(alpha, fr, rise);
    glVertex2f(fr, bet);
  }
  glEnd();*/
  
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluOrtho2D(-field_max,field_max, -1.05, 1.05);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glColor3f(0,0,0);
  glBegin(GL_LINE_STRIP);
  for(float f = -field_max; f<3*field_max; f+= df){
    float fr = f;
    char rise = 1;
    if(f > field_max){fr = 2*field_max - f; rise = 0;}
    double bet = calc_angle(alpha, fr, rise);
    //bet += IAangle;
    bet = IAangle - bet;
    bet = sin(2*bet);
    glVertex2f(fr, bet);
  }
  glEnd();
  const float size_abs = 10;
  float w = size_abs*2*field_max/wnd_w;
  float h = size_abs*2.1/(wnd_h-wnd_w);
  float sinbeta = sin(2*(IAangle -  beta));
  beta *= 1.05/M_PI;
  glColor4f(0,0,0,0.5);
  glBegin(GL_LINE_STRIP);
    glVertex2f(H, -1.1);
    glVertex2f(H, 1.1);
  glEnd();
  glColor4f(0,0,0,1);
  glBegin(GL_LINE_STRIP);
    glVertex2f(H, sinbeta+h);
    glVertex2f(H+w, sinbeta);
    glVertex2f(H, sinbeta-h);
    glVertex2f(H-w, sinbeta);
    glVertex2f(H, sinbeta+h);
  glEnd();
  /*glColor4f(0,1,0,1);
  glBegin(GL_LINE_STRIP);
    glVertex2f(H, beta+h);
    glVertex2f(H+w, beta);
    glVertex2f(H, beta-h);
    glVertex2f(H-w, beta);
    glVertex2f(H, beta+h);
  glEnd();*/
}

//отрисовка вообще всего
void draw(){
  static double H = -field_max;
  static char rise = 1;
  double dH = dfield;
  double Hslow = Hc*2;
  const double kslow = 0.3;
  double Hp = fabs(H);
  if( Hp < Hslow ){
    if( Hp < Hc ){
      dH = kslow;
    }else{
      dH = (Hp-Hc)/(Hslow-Hc)*(1-kslow)+kslow;
    }
    dH *= dfield;
  }
  draw_model(H, rise);
  draw_graph(H, rise);
  if(rise){
    H += dH;
    if(H >= field_max){H = field_max; rise = 0;}
  }else{
    H -= dH;
    if(H <=-field_max){H =-field_max; rise = 1;}
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
void save(char *filename){
  FILE *pf = fopen(filename, "wb");
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
  
  glReadPixels(0, 0, wnd_w, wnd_h, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, &buf);
  
  fwrite(&header, sizeof(header), 1, pf);
  fwrite(&info, sizeof(info), 1, pf);
  fwrite(buf, sizeof(buf), 1, pf);
  
  fclose(pf);
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
}

//колбэк на нажатие клавиши 'Save'
void sbtn_click(GtkButton *button, gpointer user_data){
  double H = -field_max;
  char rise = 1;
  double dH = 50;
  double dfield = 50;
  double Hslow = Hc*2;
  double Hp;
  const double kslow = 0.3;
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
    draw_model(H, rise);
    draw_graph(H, rise);
    Hp = fabs(H);
    if( Hp < Hslow ){
      if( Hp < Hc ){
        dH = kslow;
      }else{
        dH = (Hp-Hc)/(Hslow-Hc)*(1-kslow)+kslow;
      }
      dH *= dfield;
    }
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
    save(fname);
    printf("[%s] saved\n", fname);
  }
  
  wnd_w = bkp_w; wnd_h = bkp_h;
  SDL_SetWindowSize(wnd_sdl, wnd_w, wnd_h);
  
  free(fname);
  printf("Now you can run 'convert %s*.bmp -fuzz 10%% -layers Optimize res.gif' to convert this into res.gif\n", str);
  printf("Or 'ffmpeg -loop 1 -t 60 -framerate 7 -i %s%%4d.bmp res.mov' to convert into res.mov\n", str);
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
  
  GtkWidget *anlabel = gtk_label_new("Anizotropy field");
  gtk_fixed_put(GTK_FIXED(fixed), anlabel, 0, 160);
  GtkWidget *an = gtk_spin_button_new_with_range(0, 10000, 1);
  gtk_spin_button_set_update_policy(GTK_SPIN_BUTTON(an), GTK_UPDATE_IF_VALID);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(an), Haniz);
  g_signal_connect(G_OBJECT(an), "value-changed", G_CALLBACK(spined_OnChange), &Haniz);
  gtk_fixed_put(GTK_FIXED(fixed), an, 200, 160);
  
  GtkWidget *bmpname = gtk_entry_new();
  gtk_entry_set_text((GtkEntry*)(bmpname), "bmp/a");
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
    usleep(10000);
  }
  printf("\n");
  return 0;
}
