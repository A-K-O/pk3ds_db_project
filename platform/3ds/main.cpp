// 3DS entry point. For now it only proves the app builds and launches:
// it prints a line on the top screen and exits on START.
//
// As the app grows, this file should stay thin: set up the platform
// (graphics, input, filesystem), hand control to ui/, and shut down.

#include <3ds.h>

#include <cstdio>

int main()
{
	gfxInitDefault();
	consoleInit(GFX_TOP, nullptr);

	std::printf("3DS Pokemon Database\n\nPress START to exit.\n");

	// aptMainLoop() returns false when the system wants the app to close.
	while (aptMainLoop())
	{
		hidScanInput();
		if (hidKeysDown() & KEY_START)
			break;

		gspWaitForVBlank();
	}

	gfxExit();
	return 0;
}
