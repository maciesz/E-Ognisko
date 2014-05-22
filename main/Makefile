TARGETS = main

CXX = g++
CXXFLAGS = -std=c++11 -Wall --pedantic
LDFLAGS = -lboost_system -lboost_program_options -lboost_thread -lpthread

all: main clean

main: headerline_parser.o header_factory.o mixer.o client_parser.o server_parser.o
	$(CXX) -o $(CXXFLAGS) $@ $^ $(LDFLAGS)

header_factory.o: header_factory.cc header_factory.hpp
	$(CXX) -c $(CXXFLAGS) $<

mixer.o: mixer.cc mixer.hpp
	$(CXX) -c $(CXXFLAGS) $<

headerline_parser.o: headerline_parser.cc headerline_parser.hpp
	$(CXX) -c $(CXXFLAGS) $<

client_parser.o: client_parser.cc client_parser.hpp
	$(CXX) -c $(CXXFLAGS) $<

server_parser.o: server_parser.cc server_parser.hpp
	$(CXX) -c $(CXXFLAGS) $<

.PHONY: clean cleanest

clean:
	rm *.o

cleanest: clean
	rm main
