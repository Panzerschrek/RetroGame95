#include "Host.hpp"

extern "C" int main(const int argc, const char* argv[])
{
	(void) argc;
	(void) argv;

	Host host;
	while(!host.Loop()){}
	return 0;
}
