#include "platform.h"
#include "util.h"

#include <memory>

#include <Windows.h>

#include <WinUser.h>
#include <wtypes.h>  // HWND
#include <direct.h>
#include <io.h>
#include <fileapi.h> 
#include <handleapi.h> 
#include <shlwapi.h> 
#include <shlobj_core.h> 
#include <windowsx.h>  // GET_X_LPARAM/GET_Y_LPARAM


#define SHCNF_ACCEPT_INTERRUPTS     0x0001
#define SHCNF_ACCEPT_NON_INTERRUPTS 0x0002

#define WM_SHELLNOTIFY              ( WM_USER + 5 )
#define WM_SHELLNOTIFYRBINDIR       ( WM_USER + 6 )


extern HWND gHWND;


#if 0
struct ImgUndoManager : public IOleUndoManager
{
	HWND aHWND = 0;
	LONG nRef = 0L;

	HRESULT Open( IOleParentUndoUnit* pPUU ) override
	{
	}

	HRESULT Close( IOleParentUndoUnit* pPUU, BOOL fCommit ) override
	{
	}

	HRESULT Add( IOleUndoUnit* pUU ) override
	{
	}

	HRESULT GetOpenParentState( DWORD* pdwState ) override
	{
	}

	HRESULT DiscardFrom( IOleUndoUnit* pUU ) override
	{
	}

	HRESULT UndoTo( IOleUndoUnit* pUU ) override
	{
	}

	HRESULT RedoTo( IOleUndoUnit* pUU ) override
	{
	}

	HRESULT EnumUndoable( IEnumOleUndoUnits** ppEnum ) override
	{
	}

	HRESULT EnumRedoable( IEnumOleUndoUnits** ppEnum ) override
	{
	}

	HRESULT GetLastUndoDescription( BSTR* pBstr ) override
	{
	}

	HRESULT GetLastRedoDescription( BSTR* pBstr ) override
	{
	}

	HRESULT Enable( BOOL fEnable ) override
	{
	}

	// from IUnknown

	// this is not being called, huh
	HRESULT QueryInterface( REFIID riid, void** ppvObject ) override
	{
		printf( "QueryInterface\n" );

		if ( riid == IID_IUnknown || riid == IID_IDropTarget )
		{
			AddRef();
			*ppvObject = this;
			return S_OK;
		}
		else
		{
			*ppvObject = 0;
			return E_NOINTERFACE;
		};
	}

	ULONG AddRef() override
	{
		return ++nRef;
		// printf( "AddRef\n" );
		// return 0;
	}

	ULONG Release() override
	{
		ULONG uRet = --nRef;
		if ( uRet == 0 )
			delete this;

		return uRet;

		// printf( "Release\n" );
		// return 0;
	}
};
#endif


// ---------------------------------------------------------------------


enum UndoType : u8
{
	UndoType_Delete,
	UndoType_Rename,

	UndoType_Count
};


struct UndoData_Delete
{
	~UndoData_Delete() {}

	fs::path aPath;
};


struct UndoData_Rename
{
	~UndoData_Rename() {}

	fs::path aOriginalName;
	fs::path aNewName;
};


struct UndoOperation
{
	UndoType aUndoType;
	void*    apData;

	// union
	// {
	// 	UndoData_Delete aDelete;
	// 	UndoData_Rename aRename;
	// };
};


std::vector< UndoOperation > gUndoOperations;
size_t                       gUndoIndex;


LPSHELLFOLDER                gpDesktop       = NULL;
LPSHELLFOLDER                gpRecycleBin    = NULL;
LPITEMIDLIST                 gPidlRecycleBin = NULL;


// ---------------------------------------------------------------------


bool UndoManager_Init()
{
	HRESULT hr = S_OK;

	hr         = SHGetDesktopFolder( &gpDesktop );
	hr         = SHGetSpecialFolderLocation( gHWND, CSIDL_BITBUCKET, &gPidlRecycleBin );
	hr         = gpDesktop->BindToObject( gPidlRecycleBin, NULL, IID_IShellFolder, (LPVOID*)&gpRecycleBin );

	STRRET strRet;
	hr = gpDesktop->GetDisplayNameOf( gPidlRecycleBin, SHGDN_NORMAL, &strRet );


	return true;
}


UndoOperation* UndoManager_AddUndo()
{
	if ( gUndoOperations.size() && gUndoIndex < gUndoOperations.size() - 1 )
	{
		// clear all undo operations after this index

		return nullptr;
	}

	gUndoIndex = gUndoOperations.size()+1;
	return &gUndoOperations.emplace_back();
}


UndoOperation* UndoManager_DoUndo()
{
	if ( gUndoIndex > gUndoOperations.size() )
	{
		// uh
		return nullptr;
	}

	auto undoOp = &gUndoOperations[ gUndoIndex-1 ];
	if ( gUndoIndex > 0 )
		gUndoIndex--;

	return undoOp;
}


UndoOperation* UndoManager_DoRedo()
{
	if ( gUndoIndex >= gUndoOperations.size() )
	{
		// can't redo
		return nullptr;
	}

	auto undoOp = &gUndoOperations[ gUndoIndex++ ];

	return undoOp;
}


// ---------------------------------------------------------------------



static BOOL ExecCommand( LPITEMIDLIST pidl, std::string_view cmd )
{
	BOOL          bReturn  = FALSE;
	LPCONTEXTMENU pCtxMenu = NULL;

	HRESULT hr = gpRecycleBin->GetUIObjectOf( gHWND, 1, (LPCITEMIDLIST*)&pidl, IID_IContextMenu, NULL, (LPVOID*)&pCtxMenu );

	if ( SUCCEEDED( hr ) )
	{
		UINT  uiID        = UINT( -1 );
		UINT  uiCommand   = 0;
		UINT  uiMenuFirst = 1;
		UINT  uiMenuLast  = 0x00007FFF;
		HMENU hmenuCtx;
		int   iMenuPos = 0;
		int   iMenuMax = 0;
		TCHAR szMenuItem[ 128 ];
		TCHAR szTrace[ 512 ];
		char  verb[ MAX_PATH ];

		hmenuCtx = CreatePopupMenu();
		hr       = pCtxMenu->QueryContextMenu( hmenuCtx, 0, uiMenuFirst, uiMenuLast, CMF_NORMAL );

		iMenuMax = GetMenuItemCount( hmenuCtx );
		wsprintf( szTrace, _T("Nb Items added to the menu %d\n\n"), iMenuMax );

		for ( iMenuPos = 0; iMenuPos < iMenuMax; iMenuPos++ )
		{
			GetMenuString( hmenuCtx, iMenuPos, szMenuItem, sizeof( szMenuItem ), MF_BYPOSITION );

			uiID = GetMenuItemID( hmenuCtx, iMenuPos );

			if ( ( uiID == -1 ) || ( uiID == 0 ) )
			{
				printf( "No Verb found for entry %d\n", uiID );
				continue;
			}

			// When we'll have found the right command, we'll be obliged to perform a
			// 'uiID - 1' else the verbs are going to be be misaligned from they're
			// real ID
			hr = pCtxMenu->GetCommandString( uiID - 1, GCS_VERBA, NULL, verb, sizeof( verb ) );
			if ( FAILED( hr ) )
			{
				verb[ 0 ] = TCHAR( '\0' );
			}
			else
			{
				if ( verb == cmd )
				{
					uiCommand = uiID - 1;
					break;
				}
			}
		}

		if ( (UINT)-1 != uiCommand )
		{
			CMINVOKECOMMANDINFO cmi;

			ZeroMemory( &cmi, sizeof( CMINVOKECOMMANDINFO ) );
			cmi.cbSize       = sizeof( CMINVOKECOMMANDINFO );
			cmi.fMask        = CMIC_MASK_FLAG_NO_UI;
			cmi.hwnd         = gHWND;
			cmi.lpParameters = NULL;
			cmi.lpDirectory  = NULL;
			cmi.lpVerb       = MAKEINTRESOURCEA( uiCommand );
			cmi.nShow        = SW_SHOWNORMAL;
			cmi.dwHotKey     = NULL;
			cmi.hIcon        = NULL;

			hr               = pCtxMenu->InvokeCommand( &cmi );

			if ( SUCCEEDED( hr ) )
			{
				bReturn = TRUE;
			}
		}
	}

	pCtxMenu->Release();

	return bReturn;
}


bool UndoManager_RestoreFile( const fs::path& file )
{
	HRESULT      hr        = S_OK;
	LPITEMIDLIST pidl      = NULL;
	IEnumIDList* enumFiles = NULL;
	
	// Iterate through list
	gpRecycleBin->EnumObjects( gHWND, SHCONTF_FOLDERS | SHCONTF_NONFOLDERS | SHCONTF_INCLUDEHIDDEN, &enumFiles );

	if ( !SUCCEEDED( hr ) )
		return false;

	STRRET strRet;
	bool       foundFile = false;
	while ( enumFiles->Next( 1, &pidl, NULL ) != S_FALSE )
	{
		hr = gpRecycleBin->GetDisplayNameOf( pidl, SHGDN_NORMAL, &strRet );

		if ( !SUCCEEDED( hr ) )
			continue;

		if ( strRet.pOleStr == file )
		{
			foundFile = true;
			break;
		}
	}

	if ( !foundFile )
		return false;

#if 0
	// Remove from system clipboard history
	{
		// if ( TRUE == m_ChkRBin )
		{
			SHChangeNotifyEntry entry;
			LPITEMIDLIST        ppidl;
			// pfSHChangeNotifyRegister SHChangeNotifyRegister;

			// SHChangeNotifyRegister = (pfSHChangeNotifyRegister)GetProcAddress( m_hShell32,
			//                                                                    MAKEINTRESOURCE( 2 ) );

			if ( SHGetSpecialFolderLocation( gHWND, CSIDL_BITBUCKET, &ppidl ) != NOERROR )
			{
				printf( "GetSpecialFolder problem\n" );
			}

			entry.pidl       = ppidl;
			entry.fRecursive = TRUE;
			ULONG m_hNotifyRBin  = SHChangeNotifyRegister( gHWND,
				                                            SHCNF_ACCEPT_INTERRUPTS | SHCNF_ACCEPT_NON_INTERRUPTS,
				                                            SHCNE_ALLEVENTS,
				                                            WM_SHELLNOTIFY,  // Message that would be sent by the Shell
				                                            1,
			                                               &entry );
			if ( NULL == m_hNotifyRBin )
			{
				printf( "Warning: Change Register Failed for RecycleBin\n" );
			}
		}
		// else
		// {
		// 	pfSHChangeNotifyDeregister SHChangeNotifyDeregister = (pfSHChangeNotifyDeregister)GetProcAddress( m_hShell32,
		// 	                                                                                                  MAKEINTRESOURCE( 4 ) );
		// 
		// 	if ( NULL != SHChangeNotifyDeregister )
		// 	{
		// 		BOOL bDeregister = SHChangeNotifyDeregister( m_hNotifyRBin );
		// 	}
		// }
	}
#endif

	return ExecCommand( pidl, "undelete" );
}


// ---------------------------------------------------------------------


bool Plat_DeleteFileInternal( const fs::path& file, bool showConfirm, bool addToUndo )
{
	TCHAR Buffer[ 2048 + 4 ];

	wcsncpy_s( Buffer, 2048 + 4, file.c_str(), 2048 );
	Buffer[ wcslen( Buffer ) + 1 ] = 0;  //Double-Null-Termination

	SHFILEOPSTRUCT s;
	s.hwnd                  = gHWND;
	s.wFunc                 = FO_DELETE;
	s.pFrom                 = Buffer;
	s.pTo                   = NULL;
	s.fFlags                = FOF_ALLOWUNDO;
	s.fAnyOperationsAborted = false;
	s.hNameMappings         = NULL;
	s.lpszProgressTitle     = NULL;

	if ( !showConfirm )
		s.fFlags |= FOF_SILENT;

	int rc = SHFileOperation( &s );

	if ( rc != 0 )
	{
		wprintf( L"Failed To Delete File: %s\n", file.c_str() );
		return false;
	}

	wprintf( L"Deleted File: %s\n", file.c_str() );

	if ( !addToUndo )
		return true;

	UndoOperation* undo = UndoManager_AddUndo();

	if ( undo == nullptr )
	{
		wprintf( L"DeleteFile: Failed to add to undo stack?\n" );
	}

	UndoData_Delete* deleteData = new UndoData_Delete;
	deleteData->aPath           = file;
	undo->apData                = deleteData;

	return true;
}


bool Plat_DeleteFile( const fs::path& file, bool showConfirm )
{
	return Plat_DeleteFileInternal( file, showConfirm, true );
}


void Plat_RenameFile( const fs::path& srPath, const fs::path& srNewName )
{
}


// ---------------------------------------------------------------------


bool Plat_CanUndo()
{
	return gUndoOperations.size() && gUndoIndex > 0 && gUndoIndex <= gUndoOperations.size();
}


bool Plat_CanRedo()
{
	return gUndoOperations.size() && gUndoIndex < gUndoOperations.size();
}


bool Plat_Undo()
{
	UndoOperation* undo = UndoManager_DoUndo();

	if ( undo == nullptr )
	{
		printf( "Warning: Tried to undo but no actions to undo\n" );
		return false;
	}

	if ( undo->aUndoType == UndoType_Delete )
	{
		UndoData_Delete* deleteData = (UndoData_Delete*)undo->apData;
		if ( UndoManager_RestoreFile( deleteData->aPath ) )
			return true;
	}

	return false;
}


bool Plat_Redo()
{
	UndoOperation* undo = UndoManager_DoRedo();

	if ( undo == nullptr )
	{
		printf( "Warning: Tried to redo but no actions to redo\n" );
		return false;
	}

	if ( undo->aUndoType == UndoType_Delete )
	{
		UndoData_Delete* deleteData = (UndoData_Delete*)undo->apData;
		if ( Plat_DeleteFileInternal( deleteData->aPath, false, false ) )
			return true;
	}

	return false;
}

