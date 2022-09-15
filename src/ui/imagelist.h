#pragma once

#include <string>
#include "util.h"


enum FileSort : u8
{
	FileSort_None,
	FileSort_System,
	FileSort_AZ,
	FileSort_ZA,
	FileSort_DateModNewest,
	FileSort_DateModOldest,
	FileSort_DateCreatedNewest,
	FileSort_DateCreatedOldest,
	FileSort_SizeLargest,
	FileSort_SizeSmallest,

	FileSort_Count
};


void     ImageList_Update();
void     ImageList_Draw();
void     ImageList_Draw2();

void     ImageList_SetSortMode( FileSort sortMode );
FileSort ImageList_GetSortMode();

void     ImageList_SetPathFromFile( const fs::path& srFile );
void     ImageList_SetPath( const fs::path& srPath );
bool     ImageList_InFolder();

void     ImageList_LoadFiles();
void     ImageList_SortFiles();

bool     ImageList_LoadPrevImage();
bool     ImageList_LoadNextImage();

void     ImageList_RemoveItem( const fs::path& srFile );

