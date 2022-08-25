#pragma once

struct ImageData;


struct ImageDrawInfo
{
	double aX;
	double aY;
	int aWidth;
	int aHeight;
};


bool Render_Init();
void Render_Shutdown();

void Render_NewFrame();
void Render_Draw();

bool Render_LoadImage( ImageData* spData );
void Render_FreeImage( ImageData* spData );
void Render_DrawImage( ImageData* spData, const ImageDrawInfo& srDrawInfo );

void Render_GetWindowSize( int& srWidth, int& srHeight );

