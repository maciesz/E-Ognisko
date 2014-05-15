#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <stdint.h>


// Numer portu, z którego korzysta serwer(komunikacja TCP/UDP)
uint16_t PORT = 15814;

// Domyślna nazwa serwera, z którym łączy się klient
char* SERVER_NAME = "localhost";

// Rozmiar kolejki FIFO(w bajtach) dla każdego klienta
uint16_t FIFO_SIZE = 10560;

// Górna granica przejścia do stanu FILLING
uint16_t FIFO_LOW_WATERMARK = 0;

// Dolna granica przejścia do stanu ACTIVE
uint16_t FIFO_HIGH_WATERMARK = 10560;

// Rozmiar bufora pakietów wychodzących(w datagramach)
uint16_t BUF_LEN = 10;

// Ograniczenie na limit retransmisji
uint16_t RETRANSMIT_LIMIT = 10;

// Czas pomiędzy kolejnymi wywołaniami miksera(w milisekundach)
uint16_t TX_INTERVAL = 5;

#endif