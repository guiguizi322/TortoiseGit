﻿#pragma once
#include "afxwin.h"

class CBufferDC : public CPaintDC
{
	DECLARE_DYNAMIC(CBufferDC)

private:
	HDC m_hOutputDC;
	HDC m_hAttributeDC;
	HDC m_hMemoryDC;

	HBITMAP  m_hPaintBitmap;
	HBITMAP  m_hOldBitmap;

	RECT m_ClientRect;

	BOOL m_bBoundsUpdated = FALSE;

public:
	CBufferDC(CWnd* pWnd);
	~CBufferDC() override;

private:
	void Flush() const;

public:
	UINT SetBoundsRect(LPCRECT lpRectBounds, UINT flags);
	BOOL RestoreDC(int nSavedDC) override;
};
