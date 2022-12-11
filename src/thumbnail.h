#pragma once

struct ImageInfo;

void       Thumbnail_LoadFolder( const std::filesystem::path& srDir );
void       Thumbnail_CreateTextures();
void       Thumbnail_Shutdown();
ImageInfo* Thumbnail_GetThumbnail( const std::filesystem::path& srFile );

void       Thumbnail_Update();
void       Thumbnail_Draw();

