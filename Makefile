TARGET = object_detect
SRC = object_detect.cpp
CXX = g++
CXXFLAGS = -std=c++17 -O2 -Wall
OPENCV = `pkg-config --cflags --libs opencv4`

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) $(SRC) -o $(TARGET) $(OPENCV)

clean:
	rm -f $(TARGET)

.PHONY: clean

