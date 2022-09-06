#include "platform.h"
#include "util.h"
#include "ui/imageview.h"
#include "formats/imageloader.h"

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

#include "drag_drop.h"


ULONG gDropFormats[] = {
	CF_HDROP,  // file drop
};

ULONG gDropFormatCount = ARR_SIZE( gDropFormats );


struct ImgDropTarget : public IDropTarget
{
	HWND aHWND = 0;
	LONG nRef = 0L;

	bool                    aDropSupported = false;
	std::vector< fs::path > aDropFiles;

	bool IsValidClipboardType( IDataObject* pDataObj, FORMATETC& fmtetc )
	{
		ULONG     lFmt;
		for ( lFmt = 0; lFmt < gDropFormatCount; lFmt++ )
		{
			fmtetc.cfFormat = gDropFormats[ lFmt ];
			if ( pDataObj->QueryGetData( &fmtetc ) == S_OK )
				return true;
		}

		return false;
	}

	bool GetFilesFromDataObject( FORMATETC& fmtetc, IDataObject* pDataObj )
	{
		STGMEDIUM pmedium;
		HRESULT   ret = pDataObj->GetData( &fmtetc, &pmedium );

		if ( ret != S_OK )
			return false;

		DROPFILES* dropfiles = (DROPFILES*)GlobalLock( pmedium.hGlobal );
		HDROP      drop      = (HDROP)dropfiles;

		auto       fileCount = DragQueryFileW( drop, 0xFFFFFFFF, NULL, NULL );

		for ( UINT i = 0; i < fileCount; i++ )
		{
			TCHAR filepath[ MAX_PATH ];
			DragQueryFileW( drop, i, filepath, ARR_SIZE( filepath ) );
			// NOTE: maybe check if we can load this file here?
			// some callback function or ImageLoader_SupportsImage()?

			// TODO: DO ASYNC TO NOT LOCK UP FILE EXPLORER !!!!!!!!
			if ( ImageLoader_SupportsImage( filepath ) )
			{
				aDropFiles.push_back( filepath );
			}
		}

		GlobalUnlock( pmedium.hGlobal );

		ReleaseStgMedium( &pmedium );

		return true;
	}

	HRESULT DragEnter( IDataObject* pDataObj, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect ) override
	{
		aDropFiles.clear();

		// needed?
		// FORMATETC formats;
		// pDataObj->EnumFormatEtc( DATADIR_GET, formats );

		FORMATETC fmtetc = { CF_TEXT, 0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
		bool      valid  = IsValidClipboardType( pDataObj, fmtetc );
		if ( !valid )
			return S_FALSE;

		if ( !GetFilesFromDataObject( fmtetc, pDataObj ) )
			return S_FALSE;

		if ( aDropFiles.empty() )
		{
			aDropSupported = false;
			*pdwEffect = DROPEFFECT_SCROLL;
			// return S_FALSE;
			return S_OK;
		}
		else
		{
			aDropSupported = true;
			*pdwEffect = DROPEFFECT_LINK;
			return S_OK;
		}
	}

	HRESULT DragOver( DWORD grfKeyState, POINTL pt, DWORD* pdwEffect ) override
	{
		if ( aDropSupported )
		{
			*pdwEffect = DROPEFFECT_LINK;
		}
		else
		{
			*pdwEffect = DROPEFFECT_SCROLL;
		}

		return S_OK;
	}

	HRESULT DragLeave() override
	{
		return S_OK;
	}

	HRESULT Drop( IDataObject* pDataObj, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect ) override
	{
		FORMATETC fmtetc = { CF_TEXT, 0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
		bool      valid  = IsValidClipboardType( pDataObj, fmtetc );
		if ( !valid )
			return S_FALSE;

		if ( !GetFilesFromDataObject( fmtetc, pDataObj ) )
			return S_FALSE;

		// TODO: make async/non-blocking
		ImageView_SetImage( aDropFiles[ 0 ] );

		// SetFocus( aHWND );

		return S_OK;
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


std::vector< ImgDropTarget > gTargets;


// ---------------------------------------------------------------------


bool DragDrop_Init()
{
	// init OLE
	return OleInitialize( NULL ) == S_OK;
}


bool DragDrop_Register( HWND hwnd )
{
	auto& target = gTargets.emplace_back();
	target.aHWND = hwnd;

	if ( RegisterDragDrop( hwnd, &target ) != S_OK )
	{
		return false;
	}

	return true;
}

