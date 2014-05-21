#ifndef STRUCTURES_H
#define STRUCTURES_H

struct mixer_input {
	// Wskaźnik na dane w FIFO
	void* data;

	// Liczba dostępnych bajtów
	size_t len;

	// Wartość ustawiana przez mikser,
	// wskazująca ile bajtów należy usunąć z FIFO
	size_t consumed;
};


struct program_parametres {

	program_parametres(int argc, char** argv) 
		: _argc(argc), _argv(argv)
	{
	}

	int _argc;
	char** _argv;
};

#endif
