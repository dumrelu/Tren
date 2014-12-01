#ifndef OUTPUT_H_
#define OUTPUT_H_

#include "animatie.h"

//Daca sa se printeza o lista cu biletele vandute
#define PRINT_BILETE

//Structura pentru output
typedef struct {
	tren_t *tren;				//Pointer catre tren
	animatie_t *locomotiva;		//Animatie locomotiva
	animatie_t *vagon;			//Animatie pentru un vagon
	unsigned int fps;			//Frames per second
} output_t;

//Creaza output
output_t *output_create(
	tren_t *tren,
	const char *locomotiva,
	const char *vagon,
	unsigned int fps
);

//Ruleaza pana cand se ajunge la ultima statie
void output_run(
	output_t *output
);

//Elibereaza memoria
void output_free(
	output_t *output
);

#endif /* OUTPUT_H_ */
