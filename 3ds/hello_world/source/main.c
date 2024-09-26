#include <3ds.h>
#include <stdio.h>

int main(int argc, char **argv)
{
	#define RESET "\x1b[0m"
	#define RED "\x1b[31;1m"
	#define GREEN "\x1b[32;1m"
	#define YELLOW "\x1b[33;1m"
	#define BLUE "\x1b[34;1m"
	#define MAGENTA "\x1b[35;1m"
	#define CYAN "\x1b[36;1m"
	#define WHITE "\x1b[37;1m"
	gfxInitDefault();

	PrintConsole topScreen, bottomScreen;

	consoleInit(GFX_TOP, &topScreen);
	consoleInit(GFX_BOTTOM, &bottomScreen);

	consoleSelect(&topScreen);
	printf("hmmmmmmm\n");

	consoleSelect(&bottomScreen);
	printf("\x1b[20;12Heven more hmmmmm"); //12 because bototm screen has 40 rows and 40-16/2 = 24

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
