#include <zmq.hpp>
#include "Spout.h"
#include "rapidjson/writer.h"

int func()
{
	Spout sp;
	zmq::context_t c;
	return sp.GetVerticalSync();
}