#pragma once

// Register Drag and Drop support for a window
extern bool DragDrop_Register( HWND hwnd );

extern bool UndoManager_Init();
extern void UndoManager_Shutdown();

extern bool FolderMonitor_Init();
extern void FolderMonitor_Shutdown();

extern HWND gHWND;

