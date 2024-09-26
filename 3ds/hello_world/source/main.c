#include <3ds.h>
#include <stdio.h>

int main(int argc, char **argv)
{
	#define RESET "\x1b[0m"
	#define RED "\x1b[31m"
	#define GREEN "\x1b[32m"
	#define YELLOW "\x1b[33m"
	#define BLUE "\x1b[34m"
	#define MAGENTA "\x1b[35m"
	#define CYAN "\x1b[36m"
	#define WHITE "\x1b[37m"
	gfxInitDefault();

	PrintConsole topScreen, bottomScreen;

	consoleInit(GFX_TOP, &topScreen);
	consoleInit(GFX_BOTTOM, &bottomScreen);

	consoleSelect(&topScreen);
	printf("hmmmmmmm\n");

	consoleSelect(&bottomScreen);
	printf(WHITE "even more hmmmmm");

	consoleSelect(&topScreen);
	printf(RED "blood...\n");
	printf(GREEN "green\n");
	printf(YELLOW "yellow\n");
	printf(BLUE "blue\n");
	printf(MAGENTA "magenta\n");
	printf(CYAN "cyan\n");

	while (aptMainLoop())
	{
		hidScanInput();
		u32 kDown = hidKeysDown();
		if (kDown & KEY_START) break; //close
		gfxFlushBuffers();
		gfxSwapBuffers();
		gspWaitForVBlank();
	}

	gfxExit();

	return 0;
}
