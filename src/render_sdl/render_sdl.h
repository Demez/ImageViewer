#pragma once

#include <vector>
#include <string>

struct ImageInfo;


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

bool Render_LoadImage( ImageInfo* spInfo, std::vector< char >& srData );
void Render_FreeImage( ImageInfo* spInfo );
void Render_DrawImage( ImageInfo* spInfo, const ImageDrawInfo& srDrawInfo );

void Render_GetWindowSize( int& srWidth, int& srHeight );
void Render_SetWindowTitle( const std::wstring& srTitle );

