#include "output.h"
#include <math.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>
#include <assert.h>

//De cate linii este nevoie pentru buffer, in afara de cele necesare pentru tren
#define NR_LINII_SUPLIMENTARE 3

//Mai adauga o linie daca trebuie printate si biletel
#ifdef PRINT_BILETE
#undef NR_LINII_SUPLIMENTARE
#define NR_LINII_SUPLIMENTARE 4
#endif

//Printeaza pe buffer
#ifdef PRINT_BILETE
#define PRINT_ON_BUFFER(where, n, howMany, what) \
	if(n > 0) { \
		howMany = snprintf(where, n, "%s", what); \
		where += howMany; \
		n -= howMany; \
	}
#endif

//Buffer folosit pentru desenare
typedef struct {
	char **screen;
	unsigned int n_linii;
	unsigned int n_coloane;
} buffer_t;

output_t *output_create(tren_t *tren, const char *locomotiva, const char *vagon, unsigned int fps)
{
	//Aloca memorie
	output_t *output = (output_t *) malloc(sizeof(output_t));

	//Incarca animatiile
	output->locomotiva = animatie_load(locomotiva);
	output->vagon      = animatie_load(vagon);

	//Seteaza variabile
	output->tren = tren;
	output->fps  = fps;

	return output;
}

//Determina numarul de locuri si indici locurilor dintr-un vagon
//TODO: Fa a.i. X-urile vecine sa fie considerate ca un singur loc
//TODO: Refactor, poate o structura care sa tina informatiile despre locuri?
unsigned int **_output_locuri_vagon(animatie_t *a_vagon, unsigned int *ret_n_locuri_vagon)
{
	//Asigura-te ca exista cel putin 1 frame in animatie
	assert(a_vagon->n_frames > 0);

	unsigned int i, j;
	unsigned n_locuri_vagon = 0;
	unsigned int **indici = NULL;			//O sa tina indicii locurilor din vagon ca o matrice n_locuri_vagon x 2

	for(i = 0; i < a_vagon->height; ++i)
		for(j = 0; j < a_vagon->width; ++j)
			//Verifica daca e un loc la pozitia (i, j)
			if(a_vagon->frames[0][i][j] == 'X') {
				//Incrementeaza contorul
				++n_locuri_vagon;

				//Aloca memorie pentru inca un rand in matrice
				indici = (unsigned int **) realloc(indici, n_locuri_vagon * sizeof(unsigned int*));
				indici[n_locuri_vagon-1] = (unsigned int *) malloc(2 * sizeof(unsigned int));

				//Salveaza indicii. Coloana 1 tine y, coloana 2 x
				indici[n_locuri_vagon-1][0] = i;
				indici[n_locuri_vagon-1][1] = j;
			}

	//Sterge X-urile din animatie
	for(i = 0; i < a_vagon->n_frames; ++i)
		for(j = 0; j < n_locuri_vagon; ++j)
			a_vagon->frames[i][indici[j][0]][indici[j][1]] = ' ';

	//Returneaza rezultatele
	*ret_n_locuri_vagon = n_locuri_vagon;
	return indici;
}

//Sterge buffer-ul si ecranul
void _output_clear_buffer(buffer_t buffer)
{
	//Sterge buffer-ul
	unsigned int i, j;
	for(i = 0; i < buffer.n_linii; ++i)
		for(j = 0; j < buffer.n_coloane; ++j)
			buffer.screen[i][j] = ' ';
}

//Printeaza informatii despre tren, maxim NR_LINII_SUPLIMENTARE linii
void _output_info_tren(buffer_t buffer, tren_t *tren, int offset)
{
	//Statia
	if(tren->cspeed == 0)
		snprintf(buffer.screen[offset], buffer.n_coloane, "Statia: trenul asteapta in statia %u", tren->cstatie);
	else
		snprintf(buffer.screen[offset], buffer.n_coloane, "Statia: urmeaza statia %u(%u km).", tren->cstatie+1, tren->dist_next);

	//Viteza
	snprintf(buffer.screen[offset+1], buffer.n_coloane, "Viteaza: %u km/h", tren->cspeed);

	//Legenda
	snprintf(buffer.screen[offset+2], buffer.n_coloane, "L - Liber, Numar - Statie urcare.");

#ifdef PRINT_BILETE				//TODO: refactor
	//Printare pe buffer
	char *where = buffer.screen[offset+3];
	int n = buffer.n_coloane;
	unsigned int howMany, statie;
	char format_statie[10];

	//Determinare numar bilete
	const bilet_t *it;
	unsigned int *nrBileteStatii = malloc(tren->n_statii * sizeof(unsigned int));
	for(statie = 0; statie < tren->n_statii; ++statie)
		nrBileteStatii[statie] = 0;

	//Printeaza mesaj informare
	PRINT_ON_BUFFER(where, n, howMany, "S(bilete): ");

	//Calculeaza numarul de bilete din fiecare statie
	for(it = tren->bilete; it != NULL; it = it->next)
		++nrBileteStatii[it->sursa];

	//Calculeaza cate bilete s-au vandut in fiecare statie si printeaza in buffer
	for(statie = 0; statie < tren->n_statii; ++statie) {
		//Printeaza pe buffer
		snprintf(format_statie, 10, "%u(%u) ", statie, nrBileteStatii[statie]);
		PRINT_ON_BUFFER(where, n, howMany, format_statie)
	}

	//Elibereaza memoria
	free(nrBileteStatii);
#endif
}

//Sterge ecranul si deseneaza bufferul pe ecran
void _output_print_buffer(buffer_t buffer)
{
	//Sterge ecranul
	system("clear");

	//Deseneaza buffer-ul pe ecran
	int i, j;
	for(i = 0; i < buffer.n_linii; ++i) {
		for(j = 0; j < buffer.n_coloane; ++j)
			printf("%c", buffer.screen[i][j]);
		printf("\n");
	}

	fflush(stdout);
}

//Deseneaza pe buffer si afiseaza-l.
//Returneaza daca s-a ajuns la ultima statie sau nu.
int _output_draw(buffer_t buffer, tren_t *tren,
				  animatie_t *a_locomotiva, animatie_t *a_vagon,
				  int n_vagoane, unsigned int n_locuri_vagon, unsigned int **indici,
				  double *procent_viteza)
{
	//Sterge buffer-ul si ecranul
	_output_clear_buffer(buffer);

	//Deseneaza locomotiva
	animatie_write(a_locomotiva, buffer.screen, 0, 0);

	//Deseneaza vagoanele
	const int y = a_locomotiva->height - a_vagon->height;
	int i, j;
	for(i = 0; i < n_vagoane; ++i) {
		const int x = a_locomotiva->width + i * a_vagon->width;
		animatie_write(a_vagon, buffer.screen, x, y);
	}

	//ZONA CRITICA
	pthread_mutex_lock(&tren->m_tren);

	//Calculeaza procentul vitezei(cat % din viteaza maxima reprezinta viteza curenta)
	*procent_viteza = (double) tren->cspeed / (double) tren->mspeed;

	//Indica daca s-a ajuns la ultima statie sau nu
	int ultima_statie = tren->cstatie == (tren->n_statii - 1);

	//Deseneaza locurile
	for(i = 0; i < tren->n_locuri; ++i) {
		//Determina vagonul in care se afla locul
		int vagon = i / n_locuri_vagon;

		//Determina locului din vagon
		int indice_loc = i % n_locuri_vagon;

		//Caracterul folosit pentru a afisa stadiul locului
		char c;
		if(tren->cache_locuri[i][tren->cstatie] == 0)
			c = 'L';
		else {
			//TODO: vezi _output_locuri_vagon
			statie_t statie = 0;

			const bilet_t *it;
			for(it = tren->bilete; it->next != NULL; it = it->next) {
				//Daca e pentru acelasi loc
				if(it->loc == i) {
					if(it->sursa <= tren->cstatie && tren->cstatie < it->destinatie) {
						statie = it->sursa;
						break;
					}
				}
			}

			c = '0' + statie%10;
		}

		//Pune caracterul in buffer
		int offset_x = a_locomotiva->width + vagon*a_vagon->width;
		buffer.screen[y + indici[indice_loc][0]][offset_x + indici[indice_loc][1]] = c;

		/*//Caracterul folosit pentru a afisa stadiul locului(L == Liber, O == Ocupat)
		char c = (tren->cache_locuri[i][tren->cstatie] == 0) ? 'L' : 'O';

		//Pune caracterul in buffer
		int offset_x = a_locomotiva->width + vagon*a_vagon->width;
		buffer.screen[y + indici[indice_loc][0]][offset_x + indici[indice_loc][1]] = c;*/
	}

	//Printeaza informatii despre tren
	_output_info_tren(buffer, tren, a_locomotiva->height);

	//END ZONA CRITICA
	pthread_mutex_unlock(&tren->m_tren);

	//Afiseaza buffer-ul pe ecran
	_output_print_buffer(buffer);

	//Returneaza
	return ultima_statie;
}

void output_run(output_t *output)
{
	//Extrage trenul si animatiile
	tren_t *tren             = output->tren;
	animatie_t *a_locomotiva = output->locomotiva;
	animatie_t *a_vagon      = output->vagon;

	//Determina numarul de locuri pe vagon si pozitiile acestora
	unsigned int n_locuri_vagon, **indici;
	indici = _output_locuri_vagon(a_vagon, &n_locuri_vagon);

	//Informatii constante
	assert(a_locomotiva->height > a_vagon->height);		//Presupun ca locomotiva e mai intalta decat un vagon
	const unsigned int NR_LINII   = a_locomotiva->height + NR_LINII_SUPLIMENTARE;					//Nr de linii pentru buffer
	const unsigned int NR_VAGOANE = ceil((double) tren->n_locuri / (double) n_locuri_vagon);		//Nr de vagoane necesare pentru tren
	const unsigned int NR_COLOANE = a_locomotiva->width + NR_VAGOANE*a_vagon->width;				//Nr de coloane pentru buffer

	//Aloca buffer-ul folosit pentru desenare
	buffer_t buffer = { NULL, NR_LINII, NR_COLOANE };
	buffer.screen = (char **) malloc(NR_LINII * sizeof(char*));
	int i;
	for(i = 0; i < NR_LINII; ++i)
		buffer.screen[i] = (char *) malloc(NR_COLOANE * sizeof(char));

	//Folosite pentru a masura delay-ul provocat de desenat
	struct timeval stop, start;

	//Valorile returnate de _output_draw()
	int ultima_statie = 0;	//Indica daca s-a ajuns la ultima statie
	double procent_viteza;	//Procentul vitezei curente a trenului(cat % reprezinta viteza viteza curenta din viteza maxima)

	//Loop-ul pentru desenat
	while(!ultima_statie) {	//Cat timp nu s-a ajuns la ultima statie
		//Porneste cronometrul
		gettimeofday(&start, NULL);

		//Deseneaza
		ultima_statie = _output_draw(buffer, tren, a_locomotiva, a_vagon, NR_VAGOANE, n_locuri_vagon, indici, &procent_viteza);

		//Opreste cronometrul si determina delay-ul
		gettimeofday(&stop, NULL);
		unsigned int delay = stop.tv_usec - start.tv_usec;

		//Determina, in functie de fps, cat ar trebui sa se "doarma" apoi scade delay-ul din acel timp
		unsigned int sleep_time = 1000000 / output->fps;
		sleep_time = (delay < sleep_time) ? sleep_time - delay : 0;

		//Avanseaza animatiile
		unsigned int delta = (sleep_time + delay) * procent_viteza;		//timpul de dormit scalat cu procentul vitezei
		animatie_step(a_locomotiva, delta);
		animatie_step(a_vagon, delta);

		//Dormi
		usleep(sleep_time);
	}

	//Elibereaza buffer-ul
	for(i = 0; i < NR_LINII; ++i)
		free(buffer.screen[i]);
	free(buffer.screen);

	//Elibereaza indicii
	for(i = 0; i < n_locuri_vagon; ++i)
		free(indici[i]);
	free(indici);
}

void output_free(output_t *output)
{
	animatie_free(output->locomotiva);
	animatie_free(output->vagon);
	free(output);
}
