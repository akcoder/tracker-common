CXX ?= c++
CXXFLAGS := -std=c++17 -Wall -Wextra -Werror -O1

all: test_update test_sun test_png test_boot
	./test_update
	./test_sun
	./test_png
	./test_boot

test_update: test_update.cpp ../components/tracker_common/tracker_updlogic.h
	$(CXX) $(CXXFLAGS) -o $@ test_update.cpp

clean:
	rm -f test_update test_sun test_png test_boot
.PHONY: all clean

test_sun: test_sun.cpp ../components/tracker_common/tracker_sun.h
	$(CXX) $(CXXFLAGS) -o $@ test_sun.cpp

test_png: test_png.cpp fixture_png.h ../components/tracker_common/tracker_png.h
	$(CXX) $(CXXFLAGS) -o $@ test_png.cpp -lz

test_boot: test_boot.cpp ../components/tracker_common/tracker_boot.h
	$(CXX) $(CXXFLAGS) -o $@ test_boot.cpp
