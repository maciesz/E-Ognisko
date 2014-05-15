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
#endif