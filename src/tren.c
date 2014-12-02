#include "tren.h"
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>

//Afiseaza un mesaj de eroare si apleaza exit
#define ERROR(detalii) \
	fprintf(stderr, "Eroare - %s", detalii); \
	fprintf(stderr, "(linia: %d, fisierul: \"%s\").\n", __LINE__, __FILE__); \
	exit(-1);

//Thread-ul trenului
void *_thread_tren(void *tren_ptr);

//Cate ms sunt intr-o ora
#define MS_H 500

//Cu cat accelereaza trenul
#define ACCELERATIE 2

//TODO: micsoreaza latime cache cu 1?
tren_t *tren_create(unsigned int n_locuri, unsigned int n_statii,
		unsigned int *distante, unsigned int max_speed)
{
	//Aloca memorie
	tren_t *tren = (tren_t *) malloc(sizeof(tren_t));
	if(!tren) {
		ERROR("Alocare memorie")
	}

	//Initializeaza variabile
	tren->n_locuri = n_locuri;
	tren->n_statii = n_statii;
	tren->cstatie  = 0;
	tren->distante = (unsigned int *) malloc( (n_statii-1) * sizeof(unsigned int));
	tren->dist_next= distante[0];
	tren->mspeed   = max_speed;
	tren->cspeed   = 0;
	tren->bilete   = NULL;
	tren->last     = NULL;

	//Verifica alocare distante
	if(!tren->distante) {
		ERROR("Alocare memorie")
	}

	//Initializeaza mutex
	if(pthread_mutex_init(&tren->m_tren, NULL)) {
		ERROR("Initializare mutex")
	}

	//Creaza cache locuri
	tren->cache_locuri = (om_t **) malloc(n_locuri * sizeof(om_t *));
	if(!tren->cache_locuri) {
		ERROR("Alocare memorie")
	}
	unsigned int i, j;
	for(i = 0; i < n_locuri; ++i) {
		//Aloca memorie pentru rand
		tren->cache_locuri[i] = (om_t *) malloc(n_statii * sizeof(om_t));

		//Verifica alocarea
		if(!tren->cache_locuri[i]) {
			ERROR("Alocare memorie")
		}

		//Initializeaza cu 0(indicand ca locul e liber)
		for(j = 0; j < n_statii; ++j)
			tren->cache_locuri[i][j] = 0;
	}

	//Copiaza distantele
	for(i = 0; i < n_statii-1; ++i)
		tren->distante[i] = distante[i];

	return tren;
}

#define MAX_STATII 20
tren_t *tren_load(const char *filename)
{
	//Deschide fisierul
	FILE *file = fopen(filename, "r");
	if(!file) {
		ERROR("Fisier tren invalid")
	}

	//Citeste variabilele
	unsigned int n_locuri;				fscanf(file, "%u", &n_locuri);
	unsigned int n_statii;				fscanf(file, "%u", &n_statii);
	assert(n_statii < MAX_STATII);

	//Citeste distante
	unsigned int distante[MAX_STATII-1], i;
	for(i = 0; i < n_statii-1; ++i)
		fscanf(file, "%u", &distante[i]);

	//Citeste viteza maxima
	unsigned int viteza;				fscanf(file, "%u", &viteza);

	return tren_create(n_locuri, n_statii, distante, viteza);
}

bilet_t *cumpara_bilet(tren_t *tren, om_t om, statie_t sursa, statie_t destinatie)
{
	//Verifica ordinea statiilor
	if(sursa >= destinatie)
		return NULL;

	//Creaza biletul
	bilet_t *bilet = (bilet_t *) malloc(sizeof(bilet_t));
	bilet->om = om;
	bilet->sursa = sursa;
	bilet->destinatie = destinatie;
	bilet->loc = -1;
	bilet->next = NULL;

	//Incuie mutex
	pthread_mutex_lock(&tren->m_tren);

	//Verifica daca nu s-a trecut inca de statie
	if(tren->cstatie <= sursa) {						//TODO: modifica aici cand modifici marime cache
		int i, j;

		//Cauta un loc liber
		for(i = 0; i < tren->n_locuri; ++i) {
			//Vezi daca locul e liber pe tot parcursul drumului
			int valid = 1;
			for(j = sursa; j < destinatie; ++j)
				if(tren->cache_locuri[i][j] != 0) {		//Daca locul e ocupat
					valid = 0;
					break;
				}

			//Daca s-a gasit un loc
			if(valid) {
				//Seteaza locul
				bilet->loc = i;

				//Rezerva locul
				for(j = sursa; j < destinatie; ++j)
					tren->cache_locuri[i][j] = om;

				//Adauga biletul
				if(tren->bilete == NULL)
					tren->bilete = tren->last = bilet;
				else {
					tren->last->next = bilet;
					tren->last = bilet;
				}

				//Opreste cautarea
				break;
			}
		}
	}

	//Descuie mutex
	pthread_mutex_unlock(&tren->m_tren);

	//Returneaza biletul daca s-a gasit un loc liber
	if(bilet->loc != -1) {
		return bilet;
	} else {
		free(bilet);
		return NULL;
	}
}

int trecut_de(tren_t *tren, statie_t statie)
{
	pthread_mutex_lock(&tren->m_tren);
	int ret = tren->cstatie > statie;
	if(tren->cstatie == statie)
		ret = tren->cspeed != 0;
	pthread_mutex_unlock(&tren->m_tren);
	return ret;
}

void tren_start(tren_t *tren)
{
	pthread_create(&tren->thread, NULL, _thread_tren, (void*) tren);
}

void tren_wait(tren_t *tren)
{
	pthread_join(tren->thread, NULL);
}

void tren_log(const tren_t *tren, FILE *file)
{
	//Scrie cache-ul
	fprintf(file, "Vizualizare calatorie:\n======================\n");
	unsigned int i, j;
	fprintf(file, "   Statii: ");
	for(i = 0; i < tren->n_statii; ++i)
		fprintf(file, "%4u ", i);
	fprintf(file, "\n");

	for(i = 0; i < tren->n_locuri; ++i) {
		fprintf(file, "Locul #%2d: ", i);
		for(j = 0; j < tren->n_statii; ++j)
			fprintf(file, "%4d ", tren->cache_locuri[i][j]);
		fprintf(file, "\n");
	}
	fprintf(file, "\n\n");

	//Printeaza biletele cumparate
	fprintf(file, "Bilete cumparate:\n=================\n");
	const bilet_t *it;
	for(it = tren->bilete, i = 0; it != NULL ; it = it->next, ++i)
		fprintf(file, "Bilet #%2u: Om - %4u, Sursa - %2u, Destinatie - %2u, Loc - %2d.\n", i, it->om,
				it->sursa, it->destinatie, it->loc);
}

void tren_free(tren_t *tren)
{
	//Distruge mutex
	pthread_mutex_destroy(&tren->m_tren);

	//Elibereaza distantele
	free(tren->distante);

	//Elibereaza cache
	unsigned int i;
	for(i = 0; i < tren->n_locuri; ++i)
		free(tren->cache_locuri[i]);
	free(tren->cache_locuri);

	//Elibereaza lista de bilete
	while(tren->bilete) {
		tren->last = tren->bilete;
		tren->bilete = tren->bilete->next;
		free(tren->last);
	}

	//Elibereaza structura
	free(tren);
}

void *_thread_tren(void *tren_ptr)
{
	//Converteste argument
	tren_t *tren = (tren_t *) tren_ptr;

	//Cat timp nu s-a ajuns la ultima statie
	while(tren->cstatie < tren->n_statii-1) {

		//Asteapta in statie
		if(tren->cspeed == 0) {
#ifdef DEBUG_TREN
			printf("Astept in statia %u.\n", tren->cstatie);
#endif
			sleep(TIMP_ASTEPTAT_STATIE);		//Asteapta mai mult in prima statie
		}

		pthread_mutex_lock(&tren->m_tren);

		//Accelereaza
		if(tren->cspeed < tren->mspeed) {
			tren->cspeed = (tren->cspeed + ACCELERATIE) <= tren->mspeed ? tren->cspeed + ACCELERATIE : tren->mspeed;
		}

		//Parcurge distanta
		tren->dist_next = (tren->dist_next < tren->cspeed) ? 0 : tren->dist_next - tren->cspeed;

		//Daca s-a ajuns la statie
		if(tren->dist_next == 0) {
			++tren->cstatie;
			tren->cspeed = 0;
			tren->dist_next = (tren->cstatie < tren->n_statii-1) ? tren->distante[tren->cstatie] : 0;
		}

		pthread_mutex_unlock(&tren->m_tren);

#ifdef DEBUG_TREN
		printf("Tren:\n\tViteza: %u/%u\n\tStatie: %u/%u\n\tDistanta pana la urm: %u\n", tren->cspeed, tren->mspeed, tren->cstatie, tren->n_statii-1, tren->dist_next);
#endif

		if(tren->cspeed)
			usleep(MS_H * 1000);
	}

	pthread_exit(NULL);
}
