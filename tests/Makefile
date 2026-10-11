CXX ?= c++
CXXFLAGS := -std=c++17 -Wall -Wextra -Werror -O1

all: test_update test_sun
	./test_update
	./test_sun

test_update: test_update.cpp ../components/tracker_common/tracker_updlogic.h
	$(CXX) $(CXXFLAGS) -o $@ test_update.cpp

clean:
	rm -f test_update test_sun
.PHONY: all clean

test_sun: test_sun.cpp ../components/tracker_common/tracker_sun.h
	$(CXX) $(CXXFLAGS) -o $@ test_sun.cpp
