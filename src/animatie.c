#include "animatie.h"
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

animatie_t *animatie_load(const char *filename)
{
	//Deschide fisierul
	FILE *file = fopen(filename, "r");
	if(!file) {
		fprintf(stderr, "Eroare, fisierul de animatie \"%s\" nu exista.\n", filename);
		exit(-1);
	}

	//Aloca memorie
	animatie_t *animatie = (animatie_t *) malloc(sizeof(animatie_t));

	//Initializare
	fscanf(file, "%u", &animatie->height);
	fscanf(file, "%u", &animatie->width);
	fscanf(file, "%u", &animatie->n_frames);
	fscanf(file, "%u", &animatie->frame_time);	animatie->frame_time *= 1000;
	animatie->cframe_time = 0;
	animatie->cframe = 0;

	//Verifica ca nu s-a depasit MAX_FRAMES
	assert(animatie->n_frames < MAX_FRAMES);

	//Citeste frame-uri
	unsigned int frame, i, j;
	for(frame = 0; frame < animatie->n_frames; ++frame) {
		animatie->frames[frame] = malloc(animatie->height * sizeof(char*));
		for(i = 0; i < animatie->height; ++i)
			animatie->frames[frame][i] = malloc(animatie->width * sizeof(char));

		for(i = 0; i < animatie->height; ++i)
			for(j = 0; j < animatie->width; ++j) {
				char c = fgetc(file);

				if(c != '\n')
					animatie->frames[frame][i][j] = c;
				else
					--j;
			}
	}

	//Inchide fisierul
	fclose(file);

	return animatie;
}

void animatie_step(animatie_t *animatie, unsigned int delta)
{
	animatie->cframe_time += delta;
	if(animatie->cframe_time >= animatie->frame_time) {
		int avansare = (animatie->cframe_time / (animatie->frame_time+1));
		animatie->cframe = (animatie->cframe + avansare) % animatie->n_frames;
		animatie->cframe_time = 0;
	}
}

void animatie_write(animatie_t *animatie, char **buffer, unsigned int x, unsigned int y)
{
	unsigned int i, j;
	for(i = 0; i < animatie->height; ++i)
		for(j = 0; j < animatie->width; ++j)
			buffer[y+i][x+j] = animatie->frames[animatie->cframe][i][j];
}

void animatie_free(animatie_t *animatie)
{
	unsigned int frame, i;
	for(frame = 0; frame < animatie->n_frames; ++frame) {
		for(i = 0; i < animatie->height; ++i)
			free(animatie->frames[frame][i]);
		free(animatie->frames[frame]);
	}

	free(animatie);
}
