#ifndef _ANIMATIE_H
#define _ANIMATIE_H

#include "tren.h"

//Numarul maxim de frame-uri dintr-o animatie
#define MAX_FRAMES 15

//Animatie
typedef struct {
	char **frames[MAX_FRAMES];		//Frame-urile animatiei
	unsigned int height, width;		//Dimeniuni animatie
	unsigned int n_frames;			//Numar de frame-uri
	unsigned int cframe;			//Frame-ul curent
	unsigned int frame_time;		//Timpul dintre frame-uri
	unsigned int cframe_time;		//Timpul curent
} animatie_t;


//Incarca animatie
animatie_t *animatie_load(
	const char *filename
);

//Avanseaza animatie
void animatie_step(
	animatie_t *animatie,
	unsigned int delta		//Cat timp a trecut de la ultima apelare
);

//Scrie animatia in buffer la pozitia data
void animatie_write(
	animatie_t *animatie,
	char **buffer,
	unsigned int x,
	unsigned int y
);

//Elibereaza memoria folosita
void animatie_free(
	animatie_t *animatie
);

#endif /*_ANIMATIE_H*/
