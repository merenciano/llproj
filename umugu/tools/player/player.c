#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define UMUGU_IMPLEMENTATION
#define UMUGU_IO_PORTAUDIO
#include "../../umugu.h"

umugu_scene scene;

static void *LoadScene(const char *filename)
{
	FILE *f = fopen(filename, "r");

	fseek(f, 0, SEEK_END);
	size_t size = ftell(f);
	fseek(f, 0, SEEK_SET);

	void *buffer = malloc(size);
	fread(buffer, size, 1, f);
	fclose(f);
	return buffer;
}

static void StartStream()
{
	umugu_init(&scene, NULL);
	umugu_start_stream();
}

int main(int argc, char **argv)
{
	void *scene_data = LoadScene(argv[1]);
	umugu_deserialize_scene(&scene, scene_data);
	StartStream();
	sleep(5);
	free(scene_data);
	return 0;
}
