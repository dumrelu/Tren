#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "tren.h"
#include "animatie.h"
#include "output.h"

//Cate bilete sa se cumpere la fiecare ghiseu
int NR_BILETE;

//Timp minim de dormit(ms)
const unsigned int TIMP_MINIM = 500;

//FPS pentru output
#define FPS 15

typedef struct {
	statie_t statia;		//Statia in care se afla ghiseul
	tren_t *tren;			//Trenul
	unsigned int seed;		//Pentru rand_r
	om_t generator;			//De unde se incepe sa se genereze oameni
	unsigned int max_sleep;	//Timpul maxim care se poate dormi(ms)
} argument_ghiseu_t;

void *ghiseu_thread(void *arg)
{
	//Converteste argument
	argument_ghiseu_t *argument = (argument_ghiseu_t *) arg;

	//Cumpara bilete
	int i;
	for(i = 0; i < NR_BILETE && !trecut_de(argument->tren, argument->statia); ++i) {
		//Destinatie aleatoare
		statie_t destinatie = argument->statia;
		destinatie += rand_r(&argument->seed)%(argument->tren->n_statii - 1 - argument->statia);
		destinatie += 1;

		//Cumpara bilet
		if(!cumpara_bilet(argument->tren, argument->generator++, argument->statia, destinatie))
			--i;

		//Dormi un timp aleator
		unsigned int timp_aleator = argument->statia * TIMP_ASTEPTAT_STATIE * 100 + argument->max_sleep/2 +
									TIMP_MINIM * (argument->statia+1) +
									rand_r(&argument->seed)%argument->max_sleep;
		timp_aleator *= 1000;	//Converteste in ms in micros
		usleep(timp_aleator);
	}

	pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
	//Verifica ca s-au dat destule argumente
	if(argc != 5) {
		fprintf(stderr, "Numar de argumente invalid.\n");
		fprintf(stderr, "Mod folosire: %s fisier_tren animatie_locomotiva animatie_vagon output\n", argv[0]);
		exit(-1);
	}

	//Seed pentru random-ul global
	srand(time(NULL));

	//Indicii pentru argumente
	const unsigned int TREN       = 1;
	const unsigned int LOCOMOTIVA = 2;
	const unsigned int VAGON      = 3;
	const unsigned int OUTPUT     = 4;

	//Incarca trenul
	tren_t *tren = tren_load(argv[TREN]);
	NR_BILETE = 0.65*tren->n_locuri;

	//Creaza output
	output_t *output = output_create(tren, argv[LOCOMOTIVA], argv[VAGON], FPS);

	//Creaza argumentele pentru ghiseuri
	argument_ghiseu_t *argumente = malloc((tren->n_statii-1) * sizeof(argument_ghiseu_t));
	unsigned int i, sum_dist = 0;
	for(i = 0; i < (tren->n_statii-1); sum_dist += tren->distante[i], ++i) {
		argumente[i].statia    = i;
		argumente[i].tren      = tren;
		argumente[i].seed      = rand();
		argumente[i].generator = i * NR_BILETE + 1;
		//TODO: o formula buna pentru timp
		argumente[i].max_sleep = sum_dist*100/tren->mspeed+1;
		//argumente[i].max_sleep = i*TIMP_ASTEPTAT_STATIE*100 + 1;
	}

	//Porneste trenul
	tren_start(tren);

	//Porneste thread-urile
	pthread_t *threads = malloc((tren->n_statii-1) * sizeof(pthread_t) );
	for(i = 0; i < (tren->n_statii-1); ++i)
		pthread_create(&threads[i], NULL, ghiseu_thread, &argumente[i]);

	//Ruleaza output-ul(pe main thread)
	output_run(output);

	//Join la thread-uri
	for(i = 0; i < (tren->n_statii-1); ++i)
		pthread_join(threads[i], NULL);

	//Scrie output-ul
	FILE *file = fopen(argv[OUTPUT], "w");
	tren_log(tren, file);
	fflush(file);
	fclose(file);

	//Elibereaza tren si output
	tren_free(tren);
	output_free(output);

	//Elibereaza argumente si threads
	free(argumente);
	free(threads);

	return EXIT_SUCCESS;
}
