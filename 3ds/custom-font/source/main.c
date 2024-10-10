#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <3ds.h>
#include <citro2d.h>

C2D_TextBuf g_staticBuf;
C2D_Text g_staticText[3];
C2D_Font font[5];

static void sceneInit(void)
{
	g_staticBuf  = C2D_TextBufNew(4096); // support up to 4096 glyphs in the buffer
	font[1] = C2D_FontLoad("romfs:/Hurmit.bcfnt");
	font[2] = C2D_FontLoad("romfs:/JetBrains.bcfnt");
	font[3] = C2D_FontLoad("romfs:/Gohu.bcfnt");
	font[4] = C2D_FontLoad("romfs:/ComicSans.bcfnt");

	C2D_TextFontParse(&g_staticText[0], font[3], g_staticBuf, "we got da");
	C2D_TextFontParse(&g_staticText[1], font[4], g_staticBuf, "comic sans");
	C2D_TextFontParse(&g_staticText[2], font[1], g_staticBuf, "on da 3ds");

	C2D_TextOptimize(&g_staticText[2]);
}

static void sceneRender(float size)
{
	// Draw static text strings
	//float text2PosX = 400.0f - 16.0f - g_staticText[2].width*0.75f; // right-justify
	u32 color0 = C2D_Color32(0xFF, 0x00, 0x00, 0xFF);
	u32 color1 = C2D_Color32(0x00, 0xFF, 0x00, 0xFF);
	u32 color2 = C2D_Color32(0x00, 0x00, 0xFF, 0xFF);
	u32 color3 = C2D_Color32(0xFF, 0xFF, 0xFF, 0xFF);
	C2D_DrawText(&g_staticText[0], C2D_WithColor | C2D_AtBaseline, 020.0f, 040.0f, 0.5f, size, size, color0);
	C2D_DrawText(&g_staticText[1], C2D_WithColor | C2D_AtBaseline, 120.0f, 140.0f, 0.5f, size, size, color1);
	C2D_DrawText(&g_staticText[2], C2D_WithColor | C2D_AtBaseline, 220.0f, 240.0f, 0.5f, size, size, color3);
	C2D_DrawText(&g_staticText[2], C2D_WithColor | C2D_AtBaseline, 220.0f, 240.0f, 0.5f, size, size, color2);
}

static void sceneExit(void)
{
	// Delete the text buffers
	C2D_TextBufDelete(g_staticBuf);
	C2D_FontFree(font[0]);
	C2D_FontFree(font[1]);
	C2D_FontFree(font[2]);
}

int main()
{
	romfsInit();
	cfguInit(); // Allow C2D_FontLoadSystem to work
	// Initialize the libs
	gfxInitDefault();
	C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
	C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
	C2D_Prepare();

	// Create screen
	C3D_RenderTarget* top = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);

	// Initialize the scene
	sceneInit();

	float size = 1.5f;

	// Main loop
	while (aptMainLoop())
	{
		hidScanInput();

		// Respond to user input
		u32 kDown = hidKeysDown();
		if (kDown & KEY_START)
			break; // break in order to return to hbmenu

		// Render the scene
		C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
		C2D_TargetClear(top, C2D_Color32(0x00, 0x00, 0x00, 0xFF));
		C2D_SceneBegin(top);
		sceneRender(size);
		C3D_FrameEnd(0);
	}

	// Deinitialize the scene
	sceneExit();

	// Deinitialize the libs
	C2D_Fini();
	C3D_Fini();
	romfsExit();
	cfguExit();
	gfxExit();
	return 0;
}
