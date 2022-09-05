#include "Host.hpp"
#include <SDL.h>

extern "C" int main(int argc, char *argv[])
{
	(void) argc;
	(void) argv;

	Host host;
	while(!host.Loop()){}
	return 0;
}
