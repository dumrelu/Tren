#ifndef TREN_H_
#define TREN_H_

#include <pthread.h>
#include <stdio.h>

//Debug pentru tren
//#define DEBUG_TREN

//Cate secunde se asteapta in statie
#define TIMP_ASTEPTAT_STATIE 3

//Indetificator om
typedef unsigned int om_t;

//Identificator statie
typedef unsigned int statie_t;

//Locul din tren
typedef int loc_t;

//Biletul reprezinta o calatorie
typedef struct _bilet_t {
	om_t om;					//Omul care a cumparat biletul
	statie_t sursa;				//De unde pleaca
	statie_t destinatie;		//Unde vrea sa ajunga
	loc_t loc;					//Locul din tren
	struct _bilet_t *next;		//Folosit pentru a implementa o lista
} bilet_t;

//Trenul
typedef struct {
	om_t **cache_locuri;		//Folosit pentru cautarea unui loc liber
	unsigned int n_locuri;		//Cate locuri sunt valabile in tren
	unsigned int n_statii;		//Cate statii sunt pe ruta trenului
	unsigned int cstatie;		//Statie curenta

	unsigned int *distante;		//Distantele dintre statii(km)
	unsigned int dist_next;		//Distanta pana la urmatoarea statie

	unsigned int mspeed;		//Viteza maxima cu care poate circula trenul(km/h)
	unsigned int cspeed;		//Viteza curenta cu care merge trenul(km/h)

	bilet_t *bilete;			//Biletele vandute
	bilet_t *last;				//Ultimul bilet vandut

	pthread_t thread;			//Thread-ul pe care o sa ruleze trenul
	pthread_mutex_t m_tren;		//Protejeaza datele trenului
} tren_t;


//Creaza trenul
tren_t *tren_create(
	unsigned int n_locuri,
	unsigned int n_statii,
	unsigned int *distante,
	unsigned int max_speed
);

//Citeste trenul dintr-un fisier
tren_t *tren_load(
	const char *filename
);

//Cumpara un bilet. Returneaza un pointer catre un bilet(care nu trebuie
//eliberat!) sau NULL daca nu e niciun loc liber sau a depasit statia.
bilet_t *cumpara_bilet(
	tren_t *tren,
	om_t om,
	statie_t sursa,
	statie_t destinatie
);

//Verifica daca s-a trecut de o statie
int trecut_de(
	tren_t *tren,
	statie_t statie
);

//Ruleaza trenul pe un nou thread
void tren_start(
	tren_t *tren
);

//Asteapta pana se termina thread-ul trenului
void tren_wait(
	tren_t *tren
);

//Scrie log-ul intr-un fisier
void tren_log(
	const tren_t *tren,
	FILE *file
);

//Elibereaza memoria folosita de tren
void tren_free(
	tren_t *tren
);

#endif /* TREN_H_ */
