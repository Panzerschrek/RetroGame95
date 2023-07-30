#include "Host.hpp"
#include <SDL.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>

namespace
{

std::unique_ptr<Host> host;

void MainLoop()
{
	if(host == nullptr)
		return;

	if(host->Loop())
	{
		emscripten_cancel_main_loop();
		host= nullptr;
	}
}

extern "C" int main(int argc, char *argv[])
{
	(void) argc;
	(void) argv;

	host= std::make_unique<Host>();
	emscripten_set_main_loop(MainLoop, 0, 1);

	return 0;
}

} // namespace

#else

extern "C" int main(int argc, char *argv[])
{
	(void) argc;
	(void) argv;

	Host host;
	while(!host.Loop()){}
	return 0;
}

extern "C" void vermischung_main()
{
	Host host;
	while(!host.Loop()){}
}

#endif
