﻿// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2020 - TortoiseGit
// Copyright (C) 2020 - TortoiseSVN

// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software Foundation,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//
#include "stdafx.h"
#include "DarkModeHelper.h"
#include "StringUtils.h"
#include "PathUtils.h"
#include <Shlobj.h>

DarkModeHelper& DarkModeHelper::Instance()
{
	static DarkModeHelper helper;
	return helper;
}

bool DarkModeHelper::CanHaveDarkMode()
{
	return m_bCanHaveDarkMode;
}

void DarkModeHelper::AllowDarkModeForApp(BOOL allow)
{
	if (m_pAllowDarkModeForApp)
		m_pAllowDarkModeForApp(allow ? 1 : 0);
	if (m_pSetPreferredAppMode)
		m_pSetPreferredAppMode(allow ? PreferredAppMode::AllowDark : PreferredAppMode::Default);
}

void DarkModeHelper::AllowDarkModeForWindow(HWND hwnd, BOOL allow)
{
	if (m_pAllowDarkModeForWindow)
		m_pAllowDarkModeForWindow(hwnd, allow);
}

BOOL DarkModeHelper::ShouldAppsUseDarkMode()
{
	if (m_pShouldAppsUseDarkMode)
		return m_pShouldAppsUseDarkMode() & 0x01;
	return FALSE;
}

BOOL DarkModeHelper::IsDarkModeAllowedForWindow(HWND hwnd)
{
	if (m_pIsDarkModeAllowedForWindow)
		return m_pIsDarkModeAllowedForWindow(hwnd);
	return FALSE;
}

BOOL DarkModeHelper::IsDarkModeAllowedForApp()
{
	if (m_pIsDarkModeAllowedForApp)
		return m_pIsDarkModeAllowedForApp();
	return FALSE;
}

BOOL DarkModeHelper::ShouldSystemUseDarkMode()
{
	if (m_pShouldSystemUseDarkMode)
		return m_pShouldSystemUseDarkMode();
	return FALSE;
}

void DarkModeHelper::RefreshImmersiveColorPolicyState()
{
	if (m_pRefreshImmersiveColorPolicyState)
		m_pRefreshImmersiveColorPolicyState();
}

BOOL DarkModeHelper::GetIsImmersiveColorUsingHighContrast(IMMERSIVE_HC_CACHE_MODE mode)
{
	if (m_pGetIsImmersiveColorUsingHighContrast)
		return m_pGetIsImmersiveColorUsingHighContrast(mode);
	return FALSE;
}

void DarkModeHelper::FlushMenuThemes()
{
	if (m_pFlushMenuThemes)
		m_pFlushMenuThemes();
}

BOOL DarkModeHelper::SetWindowCompositionAttribute(HWND hWnd, WINDOWCOMPOSITIONATTRIBDATA* data)
{
	if (m_pSetWindowCompositionAttribute)
		return m_pSetWindowCompositionAttribute(hWnd, data);
	return FALSE;
}

 void DarkModeHelper::RefreshTitleBarThemeColor(HWND hWnd, BOOL dark)
{
	WINDOWCOMPOSITIONATTRIBDATA data = { WINDOWCOMPOSITIONATTRIB::WCA_USEDARKMODECOLORS, &dark, sizeof(dark) };
	SetWindowCompositionAttribute(hWnd, &data);
}

DarkModeHelper::DarkModeHelper()
{
	INITCOMMONCONTROLSEX used = {
		sizeof(INITCOMMONCONTROLSEX),
		ICC_STANDARD_CLASSES | ICC_BAR_CLASSES | ICC_COOL_CLASSES
	};
	InitCommonControlsEx(&used);

	m_bCanHaveDarkMode = false;
	long micro = 0;
	{
		auto version = CPathUtils::GetVersionFromFile(L"uxtheme.dll");
		std::vector<std::wstring> tokens;
		stringtok(tokens, version, false, L".");
		if (tokens.size() == 4)
		{
			auto major = std::stol(tokens[0]);
			auto minor = std::stol(tokens[1]);
			micro = std::stol(tokens[2]);

			// the windows 10 update 1809 has the version
			// number as 10.0.17763.1
			if (major > 10)
				m_bCanHaveDarkMode = true;
			else if (major == 10)
			{
				if (minor > 0)
					m_bCanHaveDarkMode = true;
				else if (micro > 17762)
					m_bCanHaveDarkMode = true;
			}
		}
	}

	m_hUxthemeLib = LoadLibrary(L"uxtheme.dll");
	if (m_hUxthemeLib && m_bCanHaveDarkMode)
	{
		// Note: these functions are undocumented! Which meas I shouldn't even use them.
		// So, since MS decided to keep this new feature to themselves, I have to use
		// undocumented functions to adjust.
		// Let's just hope they change their minds and document these functions one day...

		// first try with the names, just in case MS decides to properly export these functions
		if (micro < 18362)
			m_pAllowDarkModeForApp = reinterpret_cast<AllowDarkModeForAppFPN>(GetProcAddress(m_hUxthemeLib, "AllowDarkModeForApp"));
		else
			m_pSetPreferredAppMode = reinterpret_cast<SetPreferredAppModeFPN>(GetProcAddress(m_hUxthemeLib, "SetPreferredAppMode"));
		m_pAllowDarkModeForWindow = reinterpret_cast<AllowDarkModeForWindowFPN>(GetProcAddress(m_hUxthemeLib, "AllowDarkModeForWindow"));
		m_pShouldAppsUseDarkMode = reinterpret_cast<ShouldAppsUseDarkModeFPN>(GetProcAddress(m_hUxthemeLib, "ShouldAppsUseDarkMode"));
		m_pIsDarkModeAllowedForWindow = reinterpret_cast<IsDarkModeAllowedForWindowFPN>(GetProcAddress(m_hUxthemeLib, "IsDarkModeAllowedForWindow"));
		m_pIsDarkModeAllowedForApp = reinterpret_cast<IsDarkModeAllowedForAppFPN>(GetProcAddress(m_hUxthemeLib, "IsDarkModeAllowedForApp"));
		m_pShouldSystemUseDarkMode = reinterpret_cast<ShouldSystemUseDarkModeFPN>(GetProcAddress(m_hUxthemeLib, "ShouldSystemUseDarkMode"));
		m_pRefreshImmersiveColorPolicyState = reinterpret_cast<RefreshImmersiveColorPolicyStateFN>(GetProcAddress(m_hUxthemeLib, "RefreshImmersiveColorPolicyState"));
		m_pGetIsImmersiveColorUsingHighContrast = reinterpret_cast<GetIsImmersiveColorUsingHighContrastFN>(GetProcAddress(m_hUxthemeLib, "GetIsImmersiveColorUsingHighContrast"));
		m_pFlushMenuThemes = reinterpret_cast<FlushMenuThemesFN>(GetProcAddress(m_hUxthemeLib, "FlushMenuThemes"));
		m_pSetWindowCompositionAttribute = reinterpret_cast<SetWindowCompositionAttributeFPN>(GetProcAddress(GetModuleHandleW(L"user32.dll"), "SetWindowCompositionAttribute"));
		if (!m_pAllowDarkModeForApp && micro < 18362)
			m_pAllowDarkModeForApp = reinterpret_cast<AllowDarkModeForAppFPN>(GetProcAddress(m_hUxthemeLib, MAKEINTRESOURCEA(135)));
		if (!m_pSetPreferredAppMode && micro >= 18362)
			m_pSetPreferredAppMode = reinterpret_cast<SetPreferredAppModeFPN>(GetProcAddress(m_hUxthemeLib, MAKEINTRESOURCEA(135)));
		if (!m_pAllowDarkModeForWindow)
			m_pAllowDarkModeForWindow = reinterpret_cast<AllowDarkModeForWindowFPN>(GetProcAddress(m_hUxthemeLib, MAKEINTRESOURCEA(133)));
		if (!m_pShouldAppsUseDarkMode)
			m_pShouldAppsUseDarkMode = reinterpret_cast<ShouldAppsUseDarkModeFPN>(GetProcAddress(m_hUxthemeLib, MAKEINTRESOURCEA(132)));
		if (!m_pIsDarkModeAllowedForWindow)
			m_pIsDarkModeAllowedForWindow = reinterpret_cast<IsDarkModeAllowedForWindowFPN>(GetProcAddress(m_hUxthemeLib, MAKEINTRESOURCEA(137)));
		if (!m_pIsDarkModeAllowedForApp)
			m_pIsDarkModeAllowedForApp = reinterpret_cast<IsDarkModeAllowedForAppFPN>(GetProcAddress(m_hUxthemeLib, MAKEINTRESOURCEA(139)));
		if (!m_pShouldSystemUseDarkMode)
			m_pShouldSystemUseDarkMode = reinterpret_cast<ShouldSystemUseDarkModeFPN>(GetProcAddress(m_hUxthemeLib, MAKEINTRESOURCEA(138)));
		if (!m_pRefreshImmersiveColorPolicyState)
			m_pRefreshImmersiveColorPolicyState = reinterpret_cast<RefreshImmersiveColorPolicyStateFN>(GetProcAddress(m_hUxthemeLib, MAKEINTRESOURCEA(104)));
		if (!m_pGetIsImmersiveColorUsingHighContrast)
			m_pGetIsImmersiveColorUsingHighContrast = reinterpret_cast<GetIsImmersiveColorUsingHighContrastFN>(GetProcAddress(m_hUxthemeLib, MAKEINTRESOURCEA(106)));
		if (!m_pFlushMenuThemes)
			m_pFlushMenuThemes = reinterpret_cast<FlushMenuThemesFN>(GetProcAddress(m_hUxthemeLib, MAKEINTRESOURCEA(136)));
	}
}

DarkModeHelper::~DarkModeHelper()
{
	FreeLibrary(m_hUxthemeLib);
}
