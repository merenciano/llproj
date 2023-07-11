#if 0
#ifndef __UMUGU_NODE_H__
#define __UMUGU_NODE_H__

#include "config.h"

#include <vector>

namespace umugu
{
struct WaveValue
{
	float left;
	float right;
};

class Node
{
public:
	Node();
	virtual void operator()() = 0;
	virtual void Show() = 0;
	WaveValue wave[FRAMES_PER_BUFFER];
	std::vector<Node*> in;
};

// Generators
extern float wave_table[WS_COUNT][SAMPLE_RATE];
void GenWaveTable();

class Osciloscope : public Node
{
public:
	Osciloscope();
	void operator()() override;
	void Show() override;
	
	WaveShape shape;
	int freq;
	int phase;
};

class Mixer : public Node
{
public:
	Mixer();
	void operator()() override;
	void Show() override;
};

extern Node *last;
template <typename T>
Node* CreateNode() { return new T(); }
void* Process(Node *node);
}

#endif // __UMUGU_NODE_H__
#endif