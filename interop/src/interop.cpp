#include <zmq.hpp>
#include "Spout.h"
#include <nlohmann/json.hpp>

int func()
{
	Spout sp;
	zmq::context_t c;
	return sp.GetVerticalSync();
}