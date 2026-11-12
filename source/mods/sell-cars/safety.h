#ifndef MOON_SELL_CARS_SAFETY_H
#define MOON_SELL_CARS_SAFETY_H
/* Box presence and data presence are separate native adapter failure points. */
static inline void *moon_sale_box_data(void *box,void *(*unboxer)(void *)){
 return box&&unboxer?unboxer(box):0;
}
static inline int moon_sale_bool_data(const void *data,int *value){
 if(!data||!value)return 0;
 *value=*(const unsigned char*)data!=0;return 1;
}
/* Unavailable local identity invalidates confirmation; known peers are inert. */
enum MoonSaleActor { MOON_SALE_ACTOR_UNAVAILABLE,MOON_SALE_ACTOR_PEER,MOON_SALE_ACTOR_LOCAL };
static inline enum MoonSaleActor moon_sale_actor(int read_ok,void *local,void *callback,
                                                 void *pending,int armed){
 if(!read_ok||!local)return MOON_SALE_ACTOR_UNAVAILABLE;
 if(local!=callback)return MOON_SALE_ACTOR_PEER;
 if(armed&&pending!=callback)return MOON_SALE_ACTOR_UNAVAILABLE;
 return MOON_SALE_ACTOR_LOCAL;
}
#endif
