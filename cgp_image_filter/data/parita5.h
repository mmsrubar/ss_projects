#define POPIS "# doplni na sudou paritu-5b  5 vstupu, 1 vystup %i i4,i3,i2,i1,i0 %o out \n"
//Pocet vstupu a vystupu
#define PARAM_IN 2        //pocet vstupu komb. obvodu
#define PARAM_OUT 1       //pocet vystupu komb. obvodu
//Inicializace dat. pole
#define init_data(a) \
  a[0]=0xffff0000;\
  a[1]=0xff00ff00;\
  a[2]=0xf0f0f0f0;\
  a[3]=0xcccccccc;\
  a[4]=0xaaaaaaaa;\
  a[5]=0x96696996;
//Pocet prvku pole
#define DATASIZE 6
