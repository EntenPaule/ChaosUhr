#pragma once
#include "telemetry.h"

template<typename Draw>void weatherIcon(WeatherKind kind,uint32_t now,Draw draw){
  auto disk=[&](int cx,int cy,int r,uint32_t color){
    for(int y=0;y<9;++y)for(int x=0;x<20;++x)if((x-cx)*(x-cx)+(y-cy)*(y-cy)<=r*r)draw(x,y,color);
  };
  if(kind==WEATHER_SUN || kind==WEATHER_PARTLY){
    const int cx=kind==WEATHER_PARTLY?6:10;disk(cx,4,2,0xffc020);
    for(int d=-1;d<=1;d+=2){draw(cx+d*4,4,0xffa000);draw(cx,4+d*4,0xffa000);
      draw(cx+d*3,1,0xffa000);draw(cx+d*3,7,0xffa000);}
  }
  if(kind==WEATHER_NIGHT){
    disk(10,4,4,0xc8d8ff);disk(12,2,3,0);
  }
  if(kind==WEATHER_CLOUD || kind==WEATHER_PARTLY || kind==WEATHER_RAIN || kind==WEATHER_SNOW || kind==WEATHER_STORM){
    disk(8,3,2,0x8ca8d0);disk(12,3,3,0xa0b8d8);
    for(int y=4;y<=5;++y)for(int x=4;x<=16;++x)draw(x,y,0xa0b8d8);
  }
  if(kind==WEATHER_RAIN){
    for(int i=0;i<4;++i){const int y=6+((now/200+i)%3);draw(5+i*3,y,0x208cff);}
  }
  if(kind==WEATHER_SNOW){for(int x=5;x<=15;x+=5){draw(x,7,0xffffff);draw(x+1,8,0xffffff);}}
  if(kind==WEATHER_STORM){draw(10,5,0xffdf00);draw(9,6,0xffdf00);draw(10,7,0xffdf00);draw(9,8,0xffdf00);}
  if(kind==WEATHER_FOG || kind==WEATHER_WIND){
    for(int y=1;y<8;y+=3)for(int x=3+(y%3);x<17;++x)draw(x,y,kind==WEATHER_FOG?0x9aafba:0x60b0e0);
  }
}
