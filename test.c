#include <nSystem.h>

static int estaciones[4];
static int jugadores[8];
static int jugados[8][8];
static nMonitor mutex;
static int cont, en_curso, max_par, suma;
static int paralelo= TRUE;
static int msgs= TRUE;
int prox; // Indice del proximo partido.

struct { int x, y; } emparejamientos[4*7]= {
{0,4}, {1,5}, {2,6}, {3,7}, 
{0,1}, {2,4}, {3,5}, {7,6}, 
{0,2}, {3,1}, {7,4}, {6,5},
{0,3}, {7,2}, {6,1}, {5,4},
{0,7}, {6,3}, {5,2}, {4,1},
{0,6}, {5,7}, {4,3}, {1,2},
{0,5}, {4,6}, {1,7}, {2,3} };

static int referencia[5]= {0, 1280, 640, 500, 450};
static int tiempo_inicio[4*7];

typedef int Duraciones[8];
void lolcito(int i, int j, int m);

//Funciones implementadas
void torneo(int N);
int torneo_aux(int N);
int estacion(int m,int N);
int lolcito_aux(int x, int y, int m);

static int duraciones[8][8]= {
  /*       0   1   2   3   4   5   6   7  */
  /* 0 */  0,  10,  50,  20,  50,  60,  100, 30,
  /* 1 */  0,   0,  40,  70,  60,  30,  10,  50,
  /* 2 */  0,   0,   0,  20,  70,  80,  20,  90,
  /* 3 */  0,   0,   0,   0,  40,  30,  60,  50,
  /* 4 */  0,   0,   0,   0,   0,  70,  10,  40,
  /* 5 */  0,   0,   0,   0,   0,   0,  90,  10,
  /* 6 */  0,   0,   0,   0,   0,   0,   0,  20,
  /* 7 */  0,   0,   0,   0,   0,   0,   0,   0
};

static void ini_torneo(int N) {
  int i, j, m;
  cont= en_curso= max_par= suma= 0;
  for (j= 0; j<8; j++) {
    jugadores[j]= FALSE;
    for (i=0; i<8; i++)
      jugados[i][j]= FALSE;
  }
  for (m= 0; m<N; m++)
    estaciones[m]= FALSE;
}

void valida_secuencia();

int nMain(int argc, int argv) {
  int N, tiempo_inicial;
  mutex= nMakeMonitor(1);
  msgs= TRUE;
  for (N= 1; N<=4; N++) {
    nPrintf("\n-------------------------------------------------\n");
    nPrintf("Torneo con %d estaciones\n\n", N);
    ini_torneo(N);
    tiempo_inicial= nGetTime();

    torneo(N);

    valida_secuencia();
    if (cont!=28)
      nFatalError("nMain", "no se jugaron todas las partidos para N=%d\n",
                  N);
    nPrintf("Resultados para N=%d\n", N);
    nPrintf("maximo paralelismo: %d\n", max_par);
    if (max_par!=N) {
      nPrintf("*** advertencia *** el paralelismo es inferior al esperado\n");
      paralelo= FALSE;
    }
    nPrintf("paralelismo acumulado: %d\n", suma);
    nPrintf("tiempo transcurrido: %d milisegs\n", nGetTime()-tiempo_inicial);
    nPrintf("tiempo de referencia: %d milisegs\n", referencia[N]);
  }

  nPrintf("Test de robustez en modo non preemptive durante 10 segundos.\n");
  msgs= FALSE;
  nSetTimeSlice(1);

  {
    int tiempo_actual= tiempo_inicial= nGetTime();
    int tiempo_msg= tiempo_actual+1000;
    int iter= 1;
    tiempo_inicial= tiempo_actual;
    while (tiempo_actual-tiempo_inicial<10000) {
      if (tiempo_actual>tiempo_msg) {
        nPrintf("%d ", iter++);
        tiempo_msg= tiempo_inicial+1000*iter;
      }
      for (N= 1; N<=4; N++) {
        ini_torneo(N);
        torneo(N);
        if (cont!=28)
          nFatalError("nMain", "no se jugaron todas las partidas para N=%d\n",N);
      }
      tiempo_actual= nGetTime();
    }
  }
  nPrintf("10\nTest de robustez finalizado con exito.\n");

  if (!paralelo) {
    nPrintf("*** Advertencia ***:\n%s\n%s\n%s\n%s\n",
    "En uno de los tests no se alcanzo el paralelismo esperado.",
    "Intente correr los tests en un computador sin carga adicional.",
    "Si el problema persiste, revise su tarea, pues no cumple con el",
    "requisito de paralelismo.");
  }
  return 0;
}

void lolcito(int i, int j, int m) {
  int posicion;
  nEnter(mutex);
  if (i<0 || i>=8)
    nFatalError("lolcito", "numero de jugador %d fuera de rango\n", i);
  if (j<0 || j>=8)
    nFatalError("lolcito", "numero de jugador %d fuera de rango\n", j);
  if (i==j)
    nFatalError("lolcito", "jugador %d se enfrenta consigo mismo\n", i);
  if (jugadores[i])
    nFatalError("lolcito", "jugador %d ya esta jugando\n", i);
  if (jugadores[j])
    nFatalError("lolcito", "jugador %d ya esta jugando\n", j);
  if (estaciones[m])
    nFatalError("lolcito", "estacion %d ya esta ocupada\n", m);
  if (jugados[i][j])
    nFatalError("lolcito", "partido entre %d y %d ya se jugo\n", i, j);
  /* Anotar tiempo de inicio */
  {
    int ronda_prev;
    int ctime= nGetTime();
    for (posicion= 0; posicion<28; posicion++)
      if (emparejamientos[posicion].x==i && emparejamientos[posicion].y==j ||
          emparejamientos[posicion].x==j && emparejamientos[posicion].y==i)
        break;
    if (posicion>=28)
      nFatalError("lolcito", "no se encontro partida (%d,%d)\n", i, j);
    tiempo_inicio[posicion]= ctime;
  }

  if (msgs)
    nPrintf("enfrentando %d con %d en estacion %d\n", i, j, m);
  jugadores[i]= jugadores[j]= estaciones[m]= TRUE;
  jugados[i][j]= jugados[j][i]= TRUE;
  en_curso++;
  if (en_curso>max_par)
    max_par= en_curso;
  cont++;
  suma+= en_curso;
  nExit(mutex);
  if (msgs) {
    if (i<j)
      nSleep(duraciones[i][j]);
    else
      nSleep(duraciones[j][i]);
  }
  else
    nSleep(0);
  nEnter(mutex);
  jugadores[i]= jugadores[j]= estaciones[m]= FALSE;
  if (msgs)
    nPrintf("finaliza partida entre %d y %d en estacion %d\n", i, j, m);
  en_curso--;
  nNotifyAll(mutex);
  nExit(mutex);
}

void valida_secuencia() {
  int ronda, k;
  int tiempo_ult= tiempo_inicio[0];
  /* validar primera ronda */
  for (k= 1; k<4; k++) {
    if (tiempo_inicio[k]-tiempo_inicio[k-1]<-2)
      nFatalError("valida_secuencia", "partida %d comenzo antes que %d\n",
                  k, k-1);
    if (tiempo_inicio[k]>tiempo_ult)
      tiempo_ult= tiempo_inicio[k];
  }
  /* validar rondas restantes */
  for (ronda= 1; ronda<4; ronda++) {
    int prev_ult= tiempo_ult;
    int prox_ronda;

    k= ronda*4;
    prox_ronda= k+4;
    tiempo_ult= 0;
    while (k<prox_ronda) {
      if (tiempo_inicio[k]-prev_ult<-2)
        nFatalError("valida_secuencia", "partida %d comenzo antes que "
          "uno de la ronda previa\n", k);
      if (tiempo_inicio[k]>tiempo_ult)
        tiempo_ult= tiempo_inicio[k];
      k++;
    }
  }
}

int player[8];
nMonitor mon;

void torneo(int N){
  
  nSetTaskName("Torneo");
  
  mon = nMakeMonitor();
  prox = 0;
  
  //nEnter(mon);
  int m;

  for(m=0; m<8; m++){
    player[m]= 0;
  }

  nTask estacion1[N];
  nEnter(mon);
  for(m=0; m<N; m++){
    estacion1[m]=nEmitTask(estacion,m,N);
  }
  nNotifyAll(mon);
  nExit(mon);

  for(m=0; m<N; m++){
    if (estacion1[m]!=NULL){
      nWaitTask(estacion1[m]);
    }
  }
  nDestroyMonitor(mon);
}

int torneo_aux(int N){
  nSetTaskName("torneo_aux");
  //nEnter(mon);
  int m;

  for(m=0; m<8; m++){
    player[m]= 0;
  }

  nTask estacion1[N];
  nEnter(mon);
  for(m=0; m<N; m++){
    estacion1[m]=nEmitTask(estacion,m,N);
  }
  nNotifyAll(mon);
  nExit(mon);

  for(m=0; m<N; m++){
    if (estacion1[m]!=NULL){
      nWaitTask(estacion1[m]);
    }
  }
  return 0;
}


int estacion(int m, int N){
  nSetTaskName("estacion");
  nEnter(mon);
  while(prox< 4*7){
    int x = emparejamientos[prox].x;
    int y = emparejamientos[prox].y;
    prox++;
    while(player[x]==1 || player[y]==1){
      nWait(mon);
    }
    nTask lol_aux= nEmitTask(lolcito_aux,x,y,m);
    player[x]= 1;
    player[y]= 1;
    
    while(player[x]==1 || player[y]==1){
      nWait(mon);
    }
    if (lol_aux!=NULL){
      nWaitTask(lol_aux);
    }
  }
  nNotifyAll(mon);
  nExit(mon);
  return 0;
}

int lolcito_aux(int x, int y, int m){
  nSetTaskName("lolcito_aux");
  lolcito(x,y,m);
  nEnter(mon);
  player[x]= 0;
  player[y]= 0;
  nNotifyAll(mon);
  nExit(mon);
  return 0;
}

