//  Tanvir Singh
//  Assignment 4
//  March 8, 2017

#include "pixutils.h"
#include "bmp/bmp.h"

//private methods -> make static
static pixMap* pixMap_init();
static pixMap* pixMap_copy(pixMap *p);

//plugin methods <-new for Assignment 4;
static void rotate(pixMap *p, pixMap *oldPixMap,int i, int j,void *data);
static void convolution(pixMap *p, pixMap *oldPixMap,int i, int j,void *data);
static void flipVertical(pixMap *p, pixMap *oldPixMap,int i, int j,void *data);
static void flipHorizontal(pixMap *p, pixMap *oldPixMap,int i, int j,void *data);

static pixMap* pixMap_init(){
  pixMap *p=malloc(sizeof(pixMap));
  p->pixArray_overlay=0;
  return p;
}

void pixMap_destroy (pixMap **p){
  if(!p || !*p) return;
  pixMap *this_p=*p;
  if(this_p->pixArray_overlay)
    free(this_p->pixArray_overlay);
  if(this_p->image)free(this_p->image);
  free(this_p);
  this_p=0;
}

pixMap *pixMap_read(char *filename){
  pixMap *p=pixMap_init();
  int error;
  if((error=lodepng_decode32_file(&(p->image), &(p->imageWidth), &(p->imageHeight),filename))){
    fprintf(stderr,"error %u: %s\n", error, lodepng_error_text(error));
    return 0;
  }
  p->pixArray_overlay=malloc(p->imageHeight*sizeof(rgba*));
  p->pixArray_overlay[0]=(rgba*) p->image;
  for(int i=1;i<p->imageHeight;i++){
    p->pixArray_overlay[i]=p->pixArray_overlay[i-1]+p->imageWidth;
  }
  return p;
}
int pixMap_write(pixMap *p,char *filename){
  int error=0;
  if(lodepng_encode32_file(filename, p->image, p->imageWidth, p->imageHeight)){
    fprintf(stderr,"error %u: %s\n", error, lodepng_error_text(error));
    return 1;
  }
  return 0;
}

pixMap *pixMap_copy(pixMap *p){
  pixMap *new=pixMap_init();
  *new=*p;
  new->image=malloc(new->imageHeight*new->imageWidth*sizeof(rgba));
  memcpy(new->image,p->image,p->imageHeight*p->imageWidth*sizeof(rgba));
  new->pixArray_overlay=malloc(new->imageHeight*sizeof(void*));
  new->pixArray_overlay[0]=(rgba*) new->image;
  for(int i=1;i<new->imageHeight;i++){
    new->pixArray_overlay[i]=new->pixArray_overlay[i-1]+new->imageWidth;
  }
  return new;
}


void pixMap_apply_plugin(pixMap *p,plugin *plug){
  pixMap *copy=pixMap_copy(p);
  for(int i=0;i<p->imageHeight;i++){
    for(int j=0;j<p->imageWidth;j++){
      plug->function(p,copy,i,j,plug->data);
    }
  }
  pixMap_destroy(&copy);
}

int pixMap_write_bmp16(pixMap *p,char *filename){
  BMP16map *bmp16=BMP16map_init(p->imageHeight,p->imageWidth,0,5,6,5);
  if(!bmp16) return 1;
  BMP16map_write(bmp16,filename);
  BMP16map_destroy(&bmp16);
  return 0;
}
void plugin_destroy(plugin **plug){
  if((**plug).data) free((**plug).data);
  if(*plug) free(*plug);
  *plug = 0;
}

plugin *plugin_parse(char *argv[] ,int *iptr){
  //malloc new plugin
  plugin *new=malloc(sizeof(plugin));
  new->function=0;
  new->data=0;
  
  int i=*iptr;
  if(!strcmp(argv[i]+2,"rotate")){
    //code goes here
    new->function = &rotate;
    new->data = malloc(2*sizeof(float));
    float theta = atof(argv[i+1]);
    memcpy(new->data, &theta, sizeof(float));
    ((float *) new->data)[0] = sin(degreesToRadians(-theta));
    ((float *) new->data)[1] = cos(degreesToRadians(-theta));
    
    *iptr=i+2;
    return new;
  }
  if(!strcmp(argv[i]+2,"convolution")){
    //code goes here
    new->function = &convolution;
    new->data = (int*) malloc(sizeof(int) * 9);
    int *kernel = new->data;
    for (int j = 0; j < 9; j++) {
      *kernel = atoi(argv[i+1+j]);
      kernel++;
    }
    
    *iptr=i+10;
    return new;
  }
  if(!strcmp(argv[i]+2,"flipHorizontal")){
    new->function = &flipHorizontal;
    
    *iptr=i+1;
    return new;
  }
  if(!strcmp(argv[i]+2,"flipVertical")){
    new->function = &flipVertical;
    
    *iptr=i+1;
    return new;
  }
  return(0);
}

//define plugin functions

static void rotate(pixMap *p, pixMap *oldPixMap,int i, int j,void *data){
  float *sc=(float*) data;
  const float ox=p->imageWidth/2.0f;
  const float oy=p->imageHeight/2.0f;
  const float s=sc[0];
  const float c=sc[1];
  const int y=i;
  const int x=j;
  float rotx = c*(x-ox) - s * (oy-y) + ox;
  float roty = -(s*(x-ox) + c * (oy-y) - oy);
  int rotj=rotx+.5;
  int roti=roty+.5;
  if(roti >=0 && roti < oldPixMap->imageHeight && rotj >=0 && rotj < oldPixMap->imageWidth){
    memcpy(p->pixArray_overlay[y]+x,oldPixMap->pixArray_overlay[roti]+rotj,sizeof(rgba));
  }
  else{
    memset(p->pixArray_overlay[y]+x,0,sizeof(rgba));
  }
}

static void convolution(pixMap *p, pixMap *oldPixMap,int i, int j,void *data){
  int *kern = (int*) data;
  int width = oldPixMap->imageWidth;
  int height = oldPixMap->imageHeight;
  int kernel[3][3] = {
    {(int) kern[0], (int) kern[1], (int) kern[2]} ,
    {(int) kern[3], (int) kern[4], (int) kern[5]} ,
    {(int) kern[6], (int) kern[7], (int) kern[8]}
  };
  int sum = 0;
  for (int i = 0; i < 9; i++) {
    sum += kern[i];
  }
  if (sum == 0) {
    sum = 1;
  }
  int r = 0;
  int g = 0;
  int b = 0;
  for (int k = 0; k < 3; k++) {
    for (int x = 0; x < 3; x++) {
      int row = i + k - 1;
      int col = j + x - 1;
      if (row < 0) {
        row = 0;
      } if (col < 0) {
        col = 0;
      } if (row >= height) {
        row = height - 1;
      } if (col >= width) {
        col = width - 1;
      }
      
      int kernelNum = kernel[k][x];
      r += (int) oldPixMap->pixArray_overlay[row][col].r * kernelNum / sum;
      g += (int) oldPixMap->pixArray_overlay[row][col].g * kernelNum / sum;
      b += (int) oldPixMap->pixArray_overlay[row][col].b * kernelNum / sum;
    }
  }
  
  if (r < 0) {
    r = 0;
  } if (r > 255) {
    r = 255;
  } if (g < 0) {
    g = 0;
  } if (g > 255) {
    g = 255;
  } if (b < 0) {
    b = 0;
  } if (b > 255) {
    b = 255;
  }
  
  p->pixArray_overlay[i][j].r = (unsigned char) r;
  p->pixArray_overlay[i][j].g = (unsigned char) g;
  p->pixArray_overlay[i][j].b = (unsigned char) b;

}


static void flipVertical(pixMap *p, pixMap *oldPixMap,int i, int j,void *data){
  //reverse the pixels vertically
  p->pixArray_overlay[i][j] = oldPixMap->pixArray_overlay[oldPixMap->imageHeight - 1 - i][j];
  
}
static void flipHorizontal(pixMap *p, pixMap *oldPixMap,int i, int j,void *data){
  //reverse the pixels horizontally - can be done in one line
  p->pixArray_overlay[i][j] = oldPixMap->pixArray_overlay[i][oldPixMap->imageWidth - 1 - j];
}
