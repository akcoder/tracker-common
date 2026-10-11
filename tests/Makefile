CXX ?= c++
CXXFLAGS := -std=c++17 -Wall -Wextra -Werror -O1

all: test_update
	./test_update

test_update: test_update.cpp ../components/tracker_common/tracker_updlogic.h
	$(CXX) $(CXXFLAGS) -o $@ test_update.cpp

clean:
	rm -f test_update
.PHONY: all clean
