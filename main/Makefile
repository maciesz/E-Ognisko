CXX=g++
CXXFLAGS=-std=c++11 -Wall --pedantic
BOOST_LIBS=-lboost_system -lboost_program_options -lboost_thread -lpthread

SOURCES=$(shell find . -name "*.cc")
#OBJECT_DIR=bin
OBJECTS=$(SOURCES:%.cc=%.o)
#$(addprefix $(OBJECT_DIR)/, $(patsubst %.cc, %.o, $(wildcard *.cc)))
#$(SOURCES:%.cc=%.o))
#HEADERS=$(SOURCES:%.cc=%.hpp)

TARGET=client_prog

.PHONY: all clean

all: $(TARGET)
# $(OBJECT_DIR)

#$(OBJECT_DIR):
#	mkdir $(OBJECT_DIR)

#$(OBJECT_DIR)/%.o: %.cc
#	$(CXX) $(CXXFLAGS) -c $< -o $@

$(TARGET): $(OBJECTS)
	$(CXX) $(BOOST_LIBS) $^ -o $@

%.o: %.cc
	$(CXX) $(CXXFLAGS) -c $< -o $@

#all: $(SOURCES) $(TARGET)

#$(TARGET): $(OBJECTS)
#	$(CXX) main.cc $(CXXFLAGS) $^ -o $@ $(BOOST_LIBS)

.PHONY: clean cleanest

clean:
#rm -rf $(OBJECT_DIR)
	find . -name "*.o" -type f -delete

cleanest: clean
	rm -f $(TARGET)
