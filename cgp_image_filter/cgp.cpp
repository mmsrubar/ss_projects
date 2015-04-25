#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <assert.h>
#include <limits.h>
#include "cgp.h"

// *****************************************************************************
// TYTO GLOB. PROMENNE JSOU POUZITY PRO MOU VARIANTU PROJEKTU
#define WIN_ROWS  3
#define WIN_COLS  3
#define MATRIX_ROWS 250
#define MATRIX_COLS 250

#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))
int wins_num = pow((MATRIX_ROWS-2), 2);

unsigned char *vystupy2;  //pole vystupnich hodnot pro vyhodnocovani fce
unsigned char **gif_data;  //2D matice s hodnota pixelu obrazku
unsigned char **gif_orig;  //2D matice s hodnota pixelu originalni obrazku
unsigned char *p_gif_data;
unsigned char *p_gif_orig;
// *****************************************************************************

typedef int *chromozom;                //dynamicke pole int, velikost dana m*n*(vstupu bloku+vystupu bloku) + vystupu komb
chromozom *populace[POPULACE_MAX];   //pole ukazatelu na chromozomy jedincu populace
double fitt[POPULACE_MAX];              //fitness jedincu populace
int uzitobloku(int *p_chrom);

double bestfit;
int bestfit_idx;   //nejlepsi fittnes, index jedince v populaci
int bestblk;

int *vystupy;               //pole vystupnich hodnot pro vyhodnocovani fce
int *pouzite;               //pole, kde kazda polozka odpovida bloku a urcuje zda se jedna o pouzity blok

int param_m = PARAM_M;            //pocet sloupcu
int param_n = PARAM_N;            //pocet radku
int param_in = PARAM_IN;          //pocet vstupu komb. obvodu

int param_out = PARAM_OUT;        //pocet vystupu komb. obvodu
int param_populace = POPULACE_MAX;//pocet jedincu populace
int block_in = 2;             //pocet vstupu  jednoho bloku (neni impl pro zmenu)
int l_back = L_BACK;              // 1 (pouze predchozi sloupec)  .. param_m (maximalni mozny rozsah);

int param_fitev;  //pocet pruchodu pro ohodnoceni jednoho chromozomu, vznikne jako (pocet vstupnich dat/(pocet vstupu+pocet vystupu))

int param_generaci; //pocet kroku evoluce
int data[DATASIZE]; //trenovaci data - vstupni hodnoty + k nim prislusejici spravne vystupni hodnoty
int sizesloupec = param_n*(block_in+1); //pocet polozek ktery zabira sloupec v chromozomu
int outputidx   = param_m*sizesloupec; //index v poli chromozomu, kde zacinaji vystupy
int maxidx_out  = param_n*param_m + param_in; //max. index pouzitelny jako vstup  pro vystupy
int maxfitness  = 0; //max. hodnota fitness

int fitpop, maxfitpop; //fitness populace

typedef struct { //struktura obsahujici mozne hodnoty vstupnich poli chromozomu pro urcity sloupec
    int pocet;   //pouziva se pri generovani noveho cisla pri mutaci
    int *hodnoty;
} sl_rndval;

sl_rndval **sloupce_val;  //predpocitane mozne hodnoty vstupu pro jednotlive sloupce
#define ARRSIZE PARAM_M*PARAM_N    //velikost pole = pocet bloku

#define LOOKUPTABSIZE 256
unsigned char lookupbit_tab[LOOKUPTABSIZE]; //LookUp pro rychle zjisteni poctu nastavenych bitu na 0 v 8bit cisle

#define copy_chromozome(from,to) (chromozom *) memcpy(to, from, (outputidx + param_out)*sizeof(int));

#define FITNESS_CALLCNT (POPULACE_MAX + PARAM_GENERATIONS*POPULACE_MAX) //pocet volani funkce fitness

//-----------------------------------------------------------------------
//Vypis chromozomu
//=======================================================================
//p_chrom ukazatel na chromozom
//-----------------------------------------------------------------------
void print_chrom(FILE *fout, chromozom p_chrom) {
  fprintf(fout, "{%d,%d, %d,%d, %d,%d,%d}", param_in, param_out, param_m, param_n, block_in, l_back, uzitobloku(p_chrom));
  for(int i=0; i<outputidx; i++) {
     if (i % 3 == 0) fprintf(fout,"([%d]",(i/3)+param_in);
     fprintf(fout,"%d", *p_chrom++);
     ((i+1) % 3 == 0) ? fprintf(fout,")") : fprintf(fout,",");
   }
  fprintf(fout,"(");
  for(int i=outputidx; i<outputidx+param_out; i++) {
     if (i > outputidx) fprintf(fout,",");
     fprintf(fout,"%d", *p_chrom++);
  }
  fprintf(fout,")");
  fprintf(fout,"\n");
}

void print_xls(FILE *xlsfil) {
  fprintf(xlsfil, "%d\t%d\t%d\t%d\t\t",param_generaci,bestfit,fitpop,bestblk);
  for (int i=0; i < param_populace;  i++)
      fprintf(xlsfil, "%d\t",fitt[i]);
  fprintf(xlsfil, "\n");
}

//-----------------------------------------------------------------------
//POCET POUZITYCH BLOKU
//=======================================================================
//p_chrom ukazatel na chromozom,jenz se ma ohodnotit
//-----------------------------------------------------------------------
int uzitobloku(chromozom p_chrom) {
    int i,j, in,idx, poc = 0;
    int *p_pom;
    memset(pouzite, 0, maxidx_out*sizeof(int));

    //oznacit jako pouzite bloky napojene na vystupy
    p_pom = p_chrom + outputidx;
    for (i=0; i < param_out; i++) {
        in = *p_pom++;
        pouzite[in] = 1;
    }

    //pruchod od vystupu ke vstupum
    p_pom = p_chrom + outputidx - 1;
    idx = maxidx_out-1;
    for (i=param_m; i > 0; i--) {
        for (j=param_n; j > 0; j--,idx--) {
            p_pom--; //fce
            if (pouzite[idx] == 1) { //pokud je blok pouzit, oznacit jako pouzite i bloky, na ktere je napojen
               in = *p_pom--; //in2
               pouzite[in] = 1;
               in = *p_pom--; //in1
               pouzite[in] = 1;
               poc++;
            } else {
               p_pom -= block_in; //posun na predchozi blok
            }
        }
    }

    return poc;
}

//-----------------------------------------------------------------------
//Fitness
//=======================================================================
// Dostane konkretni chromozom (obvod) a konkretni vstupni hodnoty a vrati
// fitness
//
//p_chrom ukazatel na chromozom,jenz se ma ohodnotit
//p_svystup ukazatel na pozadovane hodnoty vystupu
//p_vystup ukazatel na pole vstupnich a vystupnich hodnot bloku
//-----------------------------------------------------------------------
inline int fitness2(chromozom p_chrom, int *p_svystup) {
    int in1,in2,fce;
    int i,j;
    unsigned char *p_vystup = vystupy2;
    p_vystup += param_in; //posunuti az za hodnoty vstupu

    //Simulace obvodu pro dany stav vstupu
    // - simulace se provede pro kazde hradlo a vysledky se ulozi do pole vystupy
    // - hradlo provadi operaci nad dvemi 32b inty, tak jak to sekanina
    //   vysvetloval v prednaskach
    //----------------------------------------------------------------------------
    // Tyto dva cykly postupne projdou celou siti hradel a protahout tam tu
    // vstupni hodnotu a pak ve vystupy[30] budes mit tu vyslednou hodnotu
    // obvodu.
    for (i=0; i < param_m; i++) {  //vyhodnoceni funkce pro sloupec
        for (j=0; j < param_n; j++) { //vyhodnoceni funkce pro radky sloupce
            in1 = vystupy2[*p_chrom++];  // vezme se hodnota ze vstupu
            in2 = vystupy2[*p_chrom++];
            fce = *p_chrom++;

            switch (fce) {
              // FIXME: doimplementovat dalsi funkce
              // provede se hradlo a vysledek se ulozi do pole vystupy na
              // prisusny index reprezentujici vystup daneho hradla
              case 0: *p_vystup++ = 255; break;       //konstanta
              case 1: *p_vystup++ = in1; break;       //identita
              case 2: *p_vystup++ = 255-in1; break;       //inverze
              case 3: *p_vystup++ = MAX(in1, in2); break;       //max
              case 4: *p_vystup++ = MIN(in1, in2); break;       //max
              case 5: *p_vystup++ = in1 >> 1; break;       //right shift by 1
              case 6: *p_vystup++ = in1 >> 2; break;       //right shift by 22
              case 7: *p_vystup++ = (in1 + in2) % 255; break;       //add
              case 8: *p_vystup++ = (in1 > 0xFF - in2) ? 0xFF : in1 + in2; break;
              case 9: *p_vystup++ = (in1+in2) >> 1; break;
              case 10: *p_vystup++ = (in1 > 127 - in2) ? in2 : in1; break;       
              case 11: *p_vystup++ = (unsigned char) abs(in1 -in2); break;       
              case 12: *p_vystup++ = in1 | in2; break; //or
              case 13: *p_vystup++ = in1 & in2; break; //and
              case 14: *p_vystup++ = in1 ^ in2; break; //xor
              case 15: *p_vystup++ = ~(in1 & in2); break; //nand
                        /*
                       *
              case 3: *p_vystup++ = in1 ^ in2; break; //xor

              case 4: *p_vystup++ = ~in1; break;  //not in1
              case 5: *p_vystup++ = ~in2; break;  //not in2

              */
              default: ;
                 //*p_vystup++ = 0xffffffff; //log 1
            }
        }
    }

    //printf("vysledna hodnota vystupu: %d\n", vystupy2[maxidx_out-1]);
    return vystupy2[maxidx_out-1];
}

inline int fitness(chromozom p_chrom, int *p_svystup) {
    int in1,in2,fce;
    int i,j;
    int *p_vystup = vystupy;
    p_vystup += param_in; //posunuti az za hodnoty vstupu

    //Simulace obvodu pro dany stav vstupu
    // - simulace se provede pro kazde hradlo a vysledky se ulozi do pole vystupy
    // - hradlo provadi operaci nad dvemi 32b inty, tak jak to sekanina
    //   vysvetloval v prednaskach
    //----------------------------------------------------------------------------
    // FIXME: for vsechny pixely v obrazku
    // FIXME: tady musim vytvorit tu matici hlavniho pixelu a jeho sousedu
    //nakopirovani vstupnich dat na vstupy komb. site
     
   
    // FIXME: na zacatek toho pole vystupy musi dat ty vstupni pixely

    // Tyto dva cykly postupne projdou celou siti hradel a protahout tam tu
    // vstupni hodnotu a pak ve vystupy[30] budes mit tu vyslednou hodnotu
    // obvodu.
    for (i=0; i < param_m; i++) {  //vyhodnoceni funkce pro sloupec
        for (j=0; j < param_n; j++) { //vyhodnoceni funkce pro radky sloupce
            in1 = vystupy[*p_chrom++];  // vezme se hodnota ze vstupu
            in2 = vystupy[*p_chrom++];
            fce = *p_chrom++;

            switch (fce) {
              // provede se hradlo a vysledek se ulozi do pole vystupy na
              // prisusny index reprezentujici vystup daneho hradla
              case 0: *p_vystup++ = in1; break;       //in1

              case 1: *p_vystup++ = in1 & in2; break; //and
              case 2: *p_vystup++ = in1 | in2; break; //or
              case 3: *p_vystup++ = in1 ^ in2; break; //xor

              case 4: *p_vystup++ = ~in1; break;  //not in1
              case 5: *p_vystup++ = ~in2; break;  //not in2

              case 6: *p_vystup++ = in1 & ~in2; break;
              case 7: *p_vystup++ = ~(in1 & in2); break;
              case 8: *p_vystup++ = ~(in1 | in2); break;
              default: ;
                 *p_vystup++ = 0xffffffff; //log 1
            }
        }
    }

    register int vysl;
    register int pocok = 0; //pocet shodnych bitu

    //Vyhodnoceni odezvy
    //----------------------------------------------------------------------------
    //pomoci 4 nahledu do lookup tabulky
    for (i=0; i < param_out; i++) {  
        vysl = (vystupy[*p_chrom++] ^ *p_svystup++);
        pocok += lookupbit_tab[vysl & 0xff]; //pocet 0 => pocet spravnych
        vysl = vysl >> 8;
        pocok += lookupbit_tab[vysl & 0xff];
        vysl = vysl >> 8;
        pocok += lookupbit_tab[vysl & 0xff];
        vysl = vysl >> 8;
        pocok += lookupbit_tab[vysl & 0xff];
    }

    /*
    //pomoci for cyklu
    vysl = ~(vystupy[*p_chrom++] ^ *p_svystup++); //bit 1 udava spravnou hodnotu
    for (j=0; j < 32; j++) {
        pocok += (vysl & 1);
        vysl = vysl >> 1;
    }
    */
    return pocok;
}

//-----------------------------------------------------------------------
//OHODNOCENI CELE POPULACE
// vstup_komb je to pole tech binarnich cisel pro ktere mas pocitat tu paritu
//=======================================================================
inline void ohodnoceni(int vstup_komb[], int minidx, int maxidx, int ignoreidx) {
    int fit;
    
    for (int l=0; l < param_fitev; l++) {

        memcpy(vystupy, vstup_komb, param_in*sizeof(int));
        vstup_komb += param_in;

        // Ohodnoceni vsech jedincu (obvodu) v populaci
        //simulace obvodu vsech jedincu populace pro dane vstupy
        //
        for (int i=minidx; i < maxidx; i++) {
            if (i == ignoreidx) continue;
            fit = fitness((int *) populace[i], vstup_komb);
            (l==0) ? fitt[i] = fit : fitt[i] += fit;
        }
        vstup_komb += param_out; //posun na dalsi vstupni kombinace
    }
}

unsigned char **getNextWindow(unsigned char **matrix, int *i, int *j)
{
  int k;
  // pozice aktualni stredovy pixel se kterym pracuji
  static int row = 1;
  static int col = 1;

  if (row == 0 && col == 0) {
    // nastav je zpet na pocatecni hodnoty aby dalsi volani funkce ohodnoceni2
    // mohlo pokracovat zase od zacatku
    row = 1;
    col = 1;
    return NULL;
  }

  unsigned char **window = (unsigned char **)malloc(sizeof(unsigned char *) * WIN_ROWS);
  if (window == NULL) {
    perror("malloc");
    return NULL;
  }
  for (k = 0; k < WIN_ROWS; k++) {
    window[k] = (unsigned char *)malloc(sizeof(unsigned char) * WIN_COLS);
    if (window[k] == NULL) {
      perror("malloc");
      return NULL;
    }
  }

  window[0][0] = matrix[row-1][col-1];
  window[0][1] = matrix[row-1][col];
  window[0][2] = matrix[row-1][col+1];

  window[1][0] = matrix[row][col-1];
  window[1][1] = matrix[row][col];    // stredovy pixel
  window[1][2] = matrix[row][col+1];

  window[2][0] = matrix[row+1][col-1];
  window[2][1] = matrix[row+1][col];
  window[2][2] = matrix[row+1][col+1];

  if (col+1 != MATRIX_COLS-1) {
    col++;  // jeste se muzu posunout o jedno doprava
  } else if (col+1 == MATRIX_COLS-1 && row+1 != MATRIX_ROWS-1) {
    col = 1;
    row++;  // doprava uz nemuzu ale muzu se posunout na dalsi radek
  } else if (col+1 == MATRIX_COLS-1 && row+1 == MATRIX_ROWS-1) {
    // uz se nemzu dale posunout
    col = 0;
    row = 0;
  } else {
    printf("dostal jsem se nakm uplne mimo\n");
    assert(false);
  }

  *i = row;
  *j = col;
  return window;
}


inline void ohodnoceni2(int vstup_komb[], int minidx, int maxidx, int ignoreidx) {
  unsigned char v;
  unsigned char **p_win;
  int i, j;
  int i_index, j_index;
  int sum[POPULACE_MAX] = {};
  //int sum[POPULACE_MAX] = {[POPULACE_MAX-1] = 0};

  // FIXME: param_fitev rika kolikrat je treba projit obvodem, abych dostal
  // fitness danoho obvodu, takže to pro me bude pocet pixelu obrazku, ale
  // pouze tech ktere maji sousedy, takze ne ty na hranach!
  //for (int l=0; l < wins_num; l++) {

    while ((p_win = getNextWindow(gif_data, &i_index, &j_index)) != NULL) {
      // nakopiruj hodnoty z okna na vstupy filtru (jakoby vystupy z neceho co
      // dodava obrazek)
      //memcpy(vystupy2, p_win, param_in*sizeof(unsigned char));
      vystupy2[0] = p_win[0][0];
      vystupy2[1] = p_win[0][1];
      vystupy2[2] = p_win[0][2];
      vystupy2[3] = p_win[1][0];
      vystupy2[4] = p_win[1][1];
      vystupy2[5] = p_win[1][2];
      vystupy2[6] = p_win[2][0];
      vystupy2[7] = p_win[2][1];
      vystupy2[8] = p_win[2][2];

      for (int k = 0; k < WIN_ROWS; k++) {
        free(p_win[k]);
      }
      free(p_win);

    //vstup_komb += param_in;

      // Ohodnoceni vsech jedincu (obvodu) v populaci
      //simulace obvodu vsech jedincu populace pro dane vstupy
      //
      // FIXME: prvni pixel poslu vsem obvodum, pak druhy, pak treti ...
      // takze tady jeste nedostanu fitness celeho obvodu, ale pouze
      // aktualniho okna pixelu pro dany obvod
      for (int i=0; i < maxidx; i++) {
          if (i == ignoreidx) {
            // vypocet fitness se pro nejspeiho v dalsich generaci uplne
            // preskoci a proto tam hned nastavim tu nejvetsi hodnotu 
            sum[i] = INT_MAX;   
            continue;
          }
          
          // FIXME: fit bude fitness pro dane okno pixelu a ted tu hodnotu
          // musim odecist od hodnoty puvodniho pixelu a potom ji sectu s
          // predchozi - tak mi vznikne suma pro vsechny pixely
          // v(i,j) - orig_pixel(i,j)
          v = fitness2((int *) populace[i], vstup_komb);

          // najednou pocitam sumu pro kazdeho jedince
          sum[i] += abs((int)v - (int)gif_orig[i_index][j_index]);
          //(l==0) ? fitt[i] = fit : fitt[i] += fit;
      }

    }

    for (int i=0; i < maxidx; i++) {
      fitt[i] = (1.0/wins_num) * sum[i];
      //printf("chromozom[%d] ma fitness = %f\n", i, fitt[i]);
    }
      //vstup_komb += param_out; //posun na dalsi vstupni kombinace

  // FIXME: ted mam udelanou sumu pro fitnes u vsech obvodu a ted to jeste
  // musim vynasobit tou konstantou a dostanu tak konecnou hodnotu fitness pro
  // dany obvod.
}

//-----------------------------------------------------------------------
//MUTACE
//=======================================================================
//p_chrom ukazatel na chromozom, jenz se ma zmutovat
//-----------------------------------------------------------------------
inline void mutace(chromozom p_chrom) {
    int rnd;
    int genu = (rand()%MUTACE_MAX) + 1;     //pocet genu, ktere se budou mutovat
    int index_hodnoty;

      for (int j = 0; j < genu; j++) {
        int i = rand() % (outputidx + param_out); //vyber indexu v chromozomu pro mutaci
        int sloupec = (int) (i / sizesloupec);
        rnd = rand();
        if (i < outputidx) { //mutace bloku
           if ((i % 3) < 2) { //mutace vstupu
              p_chrom[i] = sloupce_val[sloupec]->hodnoty[(rnd % (sloupce_val[sloupec]->pocet))];
           } else { //mutace fce
              p_chrom[i] = rnd % FUNCTIONS;
           }
        } else { //mutace vystupu
           p_chrom[i] = rnd % maxidx_out;
        }
    }
}

//-----------------------------------------------------------------------
// MAIN
//-----------------------------------------------------------------------
int main(int argc, char* argv[])
{
    using namespace std;

    FILE *xlsfil, *f;
    string logfname, logfname2;
    int rnd, fitn, blk;
    int *vstup_komb; //ukazatel na vstupni data
    bool log;
    int run_succ = 0;
    int i, j, c;
    int parentidx;
    
    if (argc < 3) {
      printf("Usage: %s noise_raw_gif orig_raw_gif\n", argv[0]);
      return EXIT_FAILURE;
    }

    logfname = "log";
    if ((argc == 2) && (argv[1] != "")) 
       logfname = string(argv[1]);
    
    vystupy = new int [maxidx_out+param_out];
    vystupy2 = new unsigned char [maxidx_out+param_out];
    pouzite = new int [maxidx_out];

    // input file of the filter
    gif_data = new unsigned char*[MATRIX_ROWS];
    for (i = 0; i < MATRIX_ROWS; ++i) {
      gif_data[i] = new unsigned char[MATRIX_COLS];
    }
    p_gif_data = *gif_data;


    gif_orig = new unsigned char*[MATRIX_ROWS];
    for (i = 0; i < MATRIX_ROWS; ++i) {
      gif_orig[i] = new unsigned char[MATRIX_COLS];
    }
    p_gif_orig = *gif_orig;

    //init_data(data); //inicializace dat

    f = fopen(argv[1], "rb");
    for (i = 0; (c = fgetc(f)) != EOF; i++) {
      //printf("byte[%d]: 0.%x\n", i, c);
      *(p_gif_data+i) = (unsigned char) c;
    }

    /* vypis raw souboru, ktery pujde na vstup filtru jako matice */
    /*
    for (i = 0; i < MATRIX_ROWS; i++) {
      printf("row%d: ", i);
      for (j = 0; j < MATRIX_COLS; j++) {
        printf("%d ", gif_data[i][j]);
      }
      putchar('\n');
    }
    */


    // read the original file
    f = fopen(argv[2], "rb");
    for (i = 0; (c = fgetc(f)) != EOF; i++) {
      //printf("read byte[%d]: 0.%x\n", i, c);
      *(p_gif_orig+i) = (unsigned char) c;
    }
 
    srand((unsigned) time(NULL)); //inicializace pseudonahodneho generatoru

    /* DATASIZE = celkovy pocet bitu jak pro vstup tak pro vystup (pro 5b
     * paritu to bude 6) */
    // FIXME: musim projit kazdy pixel toho obrazku, krome okraju
    param_fitev = DATASIZE / (param_in+param_out); //Spocitani poctu pruchodu pro ohodnoceni
    // FIXME: nejlepsi fitness bude mit hodnotu 0

    maxfitness = 0; // nejlepsi fitness ktere mohu dosahnout

    //maxfitness = param_fitev*param_out*32;         //Vypocet max. fitness
    printf("maxfitnes: %d\n", maxfitness);
    printf("param_fitev: %d\n", param_fitev);
    
    for (int i=0; i < param_populace; i++) //alokace pameti pro chromozomy populace
        populace[i] = new chromozom [outputidx + param_out];
    
    //---------------------------------------------------------------------------
    // Vytvoreni LOOKUP tabulky pro rychle zjisteni poctu nenulovych bitu v bytu
    //---------------------------------------------------------------------------
    for (int i = 0; i < LOOKUPTABSIZE; i++) {
        int poc1 = 0;
        int zi = ~i;
        for (int j=0; j < 8; j++) {
            poc1 += (zi & 1);
            zi = zi >> 1;
        }
        lookupbit_tab[i] = poc1;
    }

    //-----------------------------------------------------------------------
    //Priprava pole moznych hodnot vstupu pro sloupec podle l-back a ostatnich parametru
    //-----------------------------------------------------------------------
    sloupce_val = new sl_rndval *[param_m];
    for (int i=0; i < param_m; i++) {
        sloupce_val[i] = new sl_rndval;

        int minidx = param_n*(i-l_back) + param_in;
        if (minidx < param_in) minidx = param_in; //vystupy bloku zacinaji od param_in do param_in+m*n
        int maxidx = i*param_n + param_in;

        sloupce_val[i]->pocet = param_in + maxidx - minidx;
        sloupce_val[i]->hodnoty = new int [sloupce_val[i]->pocet];

        int j=0;
        for (int k=0; k < param_in; k++,j++) //vlozeni indexu vstupu komb. obvodu
            sloupce_val[i]->hodnoty[j] = k;
        for (int k=minidx; k < maxidx; k++,j++) //vlozeni indexu moznych vstupu ze sousednich bloku vlevo
            sloupce_val[i]->hodnoty[j] = k;
    }

    //-----------------------------------------------------------------------
    printf("LogName: %s  l-back: %d  popsize:%d\n", logfname.c_str(), l_back, param_populace);
    //-----------------------------------------------------------------------

    for (int run=0; run < PARAM_RUNS; run++) {
        time_t t;
        struct tm *tl;
        char fn[100];
    
        t = time(NULL);
        tl = localtime(&t);
    
        sprintf(fn, "_%d", run);
        logfname2 = logfname + string(fn);
        strcpy(fn, logfname2.c_str()); strcat(fn,".xls");
        xlsfil = fopen(fn,"wb");
        if (!xlsfil) {
           printf("Can't create file %s!\n",fn);
           return -1;
        }
        //Hlavicka do log. souboru
        fprintf(xlsfil, "Generation\tBestfitness\tPop. fitness\t#Blocks\t\t");
        for (int i=0; i < param_populace;  i++) fprintf(xlsfil,"chrom #%d\t",i);
        fprintf(xlsfil, "\n");
    
        printf("----------------------------------------------------------------\n");
        printf("Run: %d \t\t %s", run, asctime(tl));
        printf("----------------------------------------------------------------\n");
    
        //-----------------------------------------------------------------------
        //Vytvoreni pocatecni populace
        //-----------------------------------------------------------------------
        chromozom p_chrom;
        int sloupec;
        for (int i=0; i < param_populace; i++) {
            p_chrom = (chromozom) populace[i];
            // pro kazde hladlo na radku
            for (int j=0; j < param_m*param_n; j++) {
                sloupec = (int)(j / param_n);
                // vstup 1
                *p_chrom++ = sloupce_val[sloupec]->hodnoty[(rand() % (sloupce_val[sloupec]->pocet))];
                // vstup 2
                *p_chrom++ = sloupce_val[sloupec]->hodnoty[(rand() % (sloupce_val[sloupec]->pocet))];
                // funkce
                rnd = rand() % FUNCTIONS;
                *p_chrom++ = rnd;
            }
            for (int j=outputidx; j < outputidx+param_out; j++)  //napojeni vystupu
                //*p_chrom++ = rand() % maxidx_out;
                *p_chrom = rand() % maxidx_out;
                *p_chrom++;
        }
        /*
        printf("pocatecni populace ma tyto choromozomy: \n");
        for (int i = 0; i < param_populace; i++) {
          printf("%d: ", i);
          print_chrom(stdout, (chromozom) populace[i]);
        }
          */

        //-----------------------------------------------------------------------
        //Ohodnoceni pocatecni populace
        //-----------------------------------------------------------------------
        //bestfit = 0; 
        bestfit = INT_MAX;   // na zacatku nastavim bestfitness na tu
                                // nejhorsi fitness a cim mensi fitness najdu time leps
        bestfit_idx = -1;
        ohodnoceni2(data, 0, param_populace, -1);
        for (int i=0; i < param_populace; i++) { //nalezeni nejlepsiho jedince
            if (fitt[i] < bestfit) {  // FIXME: <= ??? byla tam > ...
               bestfit = fitt[i];
               bestfit_idx = i;
            }
        }

        if (bestfit_idx != -1) {
          printf("nasel jsem obvod s novou lepsi fitness: %f\n", bestfit);
        }
    
        //bestfit_idx ukazuje na nejlepsi reseni v ramci pole jedincu "populace"
        //bestfit obsahuje fitness hodnotu prvku s indexem bestfit_idx

        if (bestfit_idx == -1) 
           return 0;
    
        //-----------------------------------------------------------------------
        // EVOLUCE
        //-----------------------------------------------------------------------
        param_generaci = 0;
        maxfitpop = 0;
        while (param_generaci++ < PARAM_GENERATIONS) {

            if (param_generaci % 100 == 0) {
              printf("gen: %d\n", param_generaci); fflush(stdout);
            }
            //printf("generace: %d\n", param_generaci);
            //-----------------------------------------------------------------------
            //Periodicky vypis chromozomu populace
            //-----------------------------------------------------------------------
            #ifdef PERIODIC_LOG
            if (param_generaci % PERIODICLOGG == 0) {
               printf("Generation: %d\n",param_generaci);
               for(int j=0; j<param_populace; j++) {
                  printf("{%d, %d}",fitt[j],uzitobloku((int *)populace[j]));
                  print_chrom(stdout,(chromozom)populace[j]);
               }
            }
            #endif

            //-----------------------------------------------------------------------
            //mutace nejlepsiho jedince populace (na param_populace mutantu)
            //-----------------------------------------------------------------------
            for (int i=0, midx = 0; i < param_populace;  i++, midx++) {
                if (bestfit_idx == i) continue;

                /*
                printf("Mutace:\n");
                printf("puvodni: ");
                print_chrom(stdout, (chromozom)populace[midx]);
                */

                p_chrom = (int *) copy_chromozome(populace[bestfit_idx],populace[midx]);
                mutace(p_chrom);

                /*
                printf("zmutovany: ");
                print_chrom(stdout, p_chrom);
                */
            }

            //-----------------------------------------------------------------------
            //ohodnoceni populace
            //-----------------------------------------------------------------------
            ohodnoceni2(data, 0, param_populace, bestfit_idx);
            parentidx = bestfit_idx;
            fitpop = 0;
            log = false;
            for (int i=0; i < param_populace; i++) { 
                fitpop += fitt[i];
                
                if (i == parentidx) continue; //preskocime rodice

                if (fitt[i] == maxfitness) {
                   //optimalizace na poc. bloku obvodu

                   blk = uzitobloku((chromozom) populace[i]);
                   if (blk <= bestblk) {

                      if (blk < bestblk) {
                         printf("Generation:%d\t\tbestblk b:%d\n",param_generaci,blk);
                         log = true;
                      }

                      bestfit_idx = i;
                      bestfit = fitt[i];
                      bestblk = blk;
                   }
                } else if (fitt[i] <= bestfit) {  // FIXME: tady bylo predtim >=, ale my hledame mensi
                   //nalezen lepsi nebo stejne dobry jedinec jako byl jeho rodic

                   if (fitt[i] < bestfit) {   // FIXME: tady bylo >
                      printf("Generation:%d\t\tFittness: %f/%d\n",param_generaci,fitt[i],maxfitness); fflush(stdout);
                      log = true;
                   }

                   bestfit_idx = i;
                   bestfit = fitt[i];
                   bestblk = ARRSIZE;
                }
            }
    
            //-----------------------------------------------------------------------
            // Vypis fitness populace do xls souboru pri zmene fitness populace/poctu bloku
            //-----------------------------------------------------------------------
            if ((fitpop > maxfitpop) || (log)) {
               print_xls(xlsfil);

               maxfitpop = fitpop;
               log = false;
            }
        }
        //-----------------------------------------------------------------------
        // Konec evoluce
        //-----------------------------------------------------------------------
        print_xls(xlsfil);
        fclose(xlsfil);
        printf("Best chromosome fitness: %f/%d\n",bestfit,maxfitness);
        printf("Best chromosome: ");
        print_chrom(stdout, (chromozom)populace[bestfit_idx]);
    
        if (bestfit == maxfitness) {
            strcpy(fn, logfname2.c_str()); strcat(fn,".chr");
            FILE *chrfil = fopen(fn,"wb");
            fprintf(chrfil, POPIS);
            print_chrom(chrfil, (chromozom)populace[bestfit_idx]);
            fclose(chrfil);
        }

        if (bestfit == maxfitness) 
           run_succ++; 
    } //runs
    for (int i=param_populace-1; i >= 0; i--)
        delete[] populace[i];
    printf("Successful runs: %d/%d (%5.1f%%)",run_succ, PARAM_RUNS, 100*run_succ/(float)PARAM_RUNS);
    return 0;
}
