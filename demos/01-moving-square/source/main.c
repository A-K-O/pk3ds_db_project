// Moving Square: a minimal 3DS homebrew demo.
//
// Top screen:    a square drawn with citro2d (GPU) that you move around.
// Bottom screen: a text console showing position and controls.
//
// Controls:
//   D-Pad       move at full speed (8 directions)
//   Circle Pad  move with analog speed (any direction)
//   A           cycle square color
//   B           re-center the square
//   START       exit back to the Homebrew Launcher

#include <3ds.h>
#include <citro2d.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define TOP_SCREEN_WIDTH  400
#define TOP_SCREEN_HEIGHT 240

#define SQUARE_SIZE 32.0f
#define MAX_SPEED   4.0f // pixels per frame (the 3DS runs at ~60 fps)

// The Circle Pad reports roughly -156..156 on each axis.
#define CPAD_MAX      156.0f
#define CPAD_DEADZONE 15

static float clampf(float v, float lo, float hi)
{
	if (v < lo) return lo;
	if (v > hi) return hi;
	return v;
}

int main(void)
{
	// Initialize graphics: gfx (framebuffers), citro3d (GPU), citro2d (2D on top of citro3d).
	gfxInitDefault();
	C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
	C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
	C2D_Prepare();

	// Bottom screen is a plain text console; top screen is a GPU render target.
	consoleInit(GFX_BOTTOM, NULL);
	C3D_RenderTarget* top = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);

	const u32 background = C2D_Color32(0x1E, 0x22, 0x2E, 0xFF);
	const u32 colors[] = {
		C2D_Color32(0xE8, 0x4A, 0x4A, 0xFF), // red
		C2D_Color32(0x4A, 0x9F, 0xE8, 0xFF), // blue
		C2D_Color32(0x5C, 0xD6, 0x6B, 0xFF), // green
		C2D_Color32(0xF2, 0xC9, 0x4C, 0xFF), // yellow
	};
	const int numColors = sizeof(colors) / sizeof(colors[0]);
	int colorIndex = 0;

	float x = (TOP_SCREEN_WIDTH  - SQUARE_SIZE) / 2.0f;
	float y = (TOP_SCREEN_HEIGHT - SQUARE_SIZE) / 2.0f;

	printf("\x1b[1;1HMoving Square demo");
	printf("\x1b[3;1HD-Pad / Circle Pad: move");
	printf("\x1b[4;1HA: change color   B: re-center");
	printf("\x1b[5;1HSTART: exit");

	// aptMainLoop() returns false when the system wants the app to close.
	while (aptMainLoop())
	{
		hidScanInput();
		u32 kDown = hidKeysDown(); // pressed this frame
		u32 kHeld = hidKeysHeld(); // held down

		if (kDown & KEY_START)
			break;

		if (kDown & KEY_A)
			colorIndex = (colorIndex + 1) % numColors;

		if (kDown & KEY_B)
		{
			x = (TOP_SCREEN_WIDTH  - SQUARE_SIZE) / 2.0f;
			y = (TOP_SCREEN_HEIGHT - SQUARE_SIZE) / 2.0f;
		}

		// Build a direction vector. Screen Y grows downward.
		// KEY_DUP etc. are the D-Pad only; KEY_UP would also include the Circle Pad.
		float dx = 0.0f, dy = 0.0f;
		if (kHeld & KEY_DLEFT)  dx -= 1.0f;
		if (kHeld & KEY_DRIGHT) dx += 1.0f;
		if (kHeld & KEY_DUP)    dy -= 1.0f;
		if (kHeld & KEY_DDOWN)  dy += 1.0f;

		circlePosition cpad;
		hidCircleRead(&cpad);
		if (abs(cpad.dx) > CPAD_DEADZONE) dx += cpad.dx / CPAD_MAX;
		if (abs(cpad.dy) > CPAD_DEADZONE) dy -= cpad.dy / CPAD_MAX; // Circle Pad Y is up-positive

		// Keep diagonals from being faster than straight lines.
		float len = sqrtf(dx * dx + dy * dy);
		if (len > 1.0f)
		{
			dx /= len;
			dy /= len;
		}

		x = clampf(x + dx * MAX_SPEED, 0.0f, TOP_SCREEN_WIDTH  - SQUARE_SIZE);
		y = clampf(y + dy * MAX_SPEED, 0.0f, TOP_SCREEN_HEIGHT - SQUARE_SIZE);

		printf("\x1b[8;1HPosition:   %3d, %3d   ", (int)x, (int)y);
		printf("\x1b[9;1HCircle Pad: %4d, %4d   ", cpad.dx, cpad.dy);

		// Render the top screen.
		C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
		C2D_TargetClear(top, background);
		C2D_SceneBegin(top);
		C2D_DrawRectSolid(x, y, 0.0f, SQUARE_SIZE, SQUARE_SIZE, colors[colorIndex]);
		C3D_FrameEnd(0);
	}

	C2D_Fini();
	C3D_Fini();
	gfxExit();
	return 0;
}
