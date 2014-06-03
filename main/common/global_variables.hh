#ifndef GLOBALS_HH
#define GLOBALS_HH

#include <climits>
#include <cstdint>
#include <string>

// Numer portu, z którego korzysta serwer(komunikacja TCP/UDP)
const std::uint16_t PORT = 15814;

// Domyślna nazwa serwera, z którym łączy się klient
const std::string SERVER_NAME = "localhost";

// Rozmiar kolejki FIFO(w bajtach) dla każdego klienta
const std::uint16_t FIFO_SIZE = 10560;

// Górna granica przejścia do stanu FILLING
const std::uint16_t FIFO_LOW_WATERMARK = 0;

// Dolna granica przejścia do stanu ACTIVE
const std::uint16_t FIFO_HIGH_WATERMARK = 10560;

// Rozmiar bufora pakietów wychodzących(w datagramach)
const std::uint16_t BUF_LEN = 10;

// Rozmiar bufora danych z wejścia u klienta(w bajtach)
const std::uint16_t CLIENT_BUFFER_LEN = 10560;//SHRT_MAX;

// Ograniczenie na limit retransmisji
const std::uint16_t RETRANSMIT_LIMIT = 10;

// Stała wpływająca na długość wiadomości z miksera
const std::uint16_t MULTIPLIER = 176;

// Czas pomiędzy kolejnymi wywołaniami miksera(w milisekundach)
const std::uint16_t TX_INTERVAL = 5;

#endif