#pragma once

#include <filesystem>

extern bool gRunning;

// ---------------------------------------------------------------------
// Undo System

enum UndoType : unsigned char
{
	UndoType_Delete,
	UndoType_Rename,

	UndoType_Count
};

struct UndoData_Delete
{
	~UndoData_Delete() {}

	std::filesystem::path aPath;
};

struct UndoData_Rename
{
	~UndoData_Rename() {}

	std::filesystem::path aOriginalName;
	std::filesystem::path aNewName;
};

struct UndoOperation
{
	UndoType aUndoType;
	void*    apData;
};

// ---------------------------------------------------------------------

void           Main_WindowDraw();
void           Main_ShouldDrawWindow( bool draw = true );

UndoOperation* UndoSys_AddUndo();
UndoOperation* UndoSys_DoUndo();
UndoOperation* UndoSys_DoRedo();

bool           UndoSys_CanUndo();
bool           UndoSys_CanRedo();
bool           UndoSys_Undo();
bool           UndoSys_Redo();

