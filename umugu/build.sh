#!/bin/bash
gcc sisi.cpp -l:libportaudio.a -lrt -lasound -ljack -lm -pthread -o sisi
