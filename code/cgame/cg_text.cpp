/*
===========================================================================
Copyright (C) 2000 - 2013, Raven Software, Inc.
Copyright (C) 2001 - 2013, Activision, Inc.
Copyright (C) 2013 - 2015, OpenJK contributors

This file is part of the OpenJK source code.

OpenJK is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, see <http://www.gnu.org/licenses/>.
===========================================================================
*/

// cg_text.c --

// this line must stay at top so the whole PCH thing works...
#include "cg_headers.h"

#include "cg_media.h"

//int precacheWav_i;	// Current high index of precacheWav array
//precacheWav_t precacheWav[MAX_PRECACHEWAV];

//int precacheText_i;	// Current high index of precacheText array
//precacheText_t precacheText[MAX_PRECACHETEXT];

extern vec4_t textcolor_caption;
extern vec4_t textcolor_center;
extern vec4_t textcolor_scroll;

// display text in a supplied box, start at top left and going down by however many pixels I feel like internally,
//	return value is NULL if all fitted, else char * of next char to continue from that didn't fit.
//
// (coords are in the usual 640x480 virtual space)...
//
// ( if you get the same char * returned as what you passed in, then none of it fitted at all (box too small) )
//
// this is execrable, and should NOT have had to've been done now, but...
//
float gfAdvanceHack = 0.0f; // MUST default to this
int giLinesOutput; // hack-city after release, only used by one function
//
const char* CG_DisplayBoxedText(const int iBoxX, const int iBoxY, const int iBoxWidth, const int iBoxHeight,
	const char* psText, const int iFontHandle, const float fScale,
	const vec4_t v4Color)
{
	giLinesOutput = 0;
	cgi_R_SetColor(v4Color);

	// Setup a reasonable vertical spacing (taiwanese & japanese need 1.5 fontheight, so use that for all)...
	//
	const int iFontHeight = cgi_R_Font_HeightPixels(iFontHandle, fScale);
	const int iFontHeightAdvance = static_cast<int>((gfAdvanceHack == 0.0f ? 1.5f : gfAdvanceHack) * static_cast<
		float>(iFontHeight));
	int iYpos = iBoxY; // start print pos

	// this could probably be simplified now, but it was converted from something else I didn't originally write,
	//	and it works anyway so wtf...
	//
	const char* psCurrentTextReadPos = psText;
	const char* psReadPosAtLineStart = psCurrentTextReadPos;
	const char* psBestLineBreakSrcPos = psCurrentTextReadPos;
	while (*psCurrentTextReadPos && iYpos + iFontHeight < iBoxY + iBoxHeight)
	{
		char sLineForDisplay[2048]; // ott

		// construct a line...
		//
		psCurrentTextReadPos = psReadPosAtLineStart;
		sLineForDisplay[0] = '\0';
		while (*psCurrentTextReadPos)
		{
			const char* psLastGood_s = psCurrentTextReadPos;

			// read letter...
			//
			qboolean bIsTrailingPunctuation;
			int iAdvanceCount;
			const unsigned int uiLetter = cgi_AnyLanguage_ReadCharFromString(
				psCurrentTextReadPos, &iAdvanceCount, &bIsTrailingPunctuation);
			psCurrentTextReadPos += iAdvanceCount;

			// concat onto string so far...
			//
			if (uiLetter == 32 && sLineForDisplay[0] == '\0')
			{
				psReadPosAtLineStart++;
				continue; // unless it's a space at the start of a line, in which case ignore it.
			}

			if (uiLetter > 255)
			{
				Q_strcat(sLineForDisplay, sizeof sLineForDisplay, va("%c%c", uiLetter >> 8, uiLetter & 0xFF));
			}
			else
			{
				Q_strcat(sLineForDisplay, sizeof sLineForDisplay, va("%c", uiLetter & 0xFF));
			}

			if (uiLetter == '\n')
			{
				// explicit new line...
				//
				sLineForDisplay[strlen(sLineForDisplay) - 1] = '\0'; // kill the CR
				psReadPosAtLineStart = psCurrentTextReadPos;
				psBestLineBreakSrcPos = psCurrentTextReadPos;
				break; // print this line
			}
			if (cgi_R_Font_StrLenPixels(sLineForDisplay, iFontHandle, fScale) >= iBoxWidth)
			{
				// reached screen edge, so cap off string at bytepos after last good position...
				//
				if (uiLetter > 255 && bIsTrailingPunctuation && !cgi_Language_UsesSpaces())
				{
					// Special case, don't consider line breaking if you're on an asian punctuation char of
					//	a language that doesn't use spaces...
					//
				}
				else
				{
					if (psBestLineBreakSrcPos == psReadPosAtLineStart)
					{
						//  aarrrggh!!!!!   we'll only get here is someone has fed in a (probably) garbage string,
						//		since it doesn't have a single space or punctuation mark right the way across one line
						//		of the screen.  So far, this has only happened in testing when I hardwired a taiwanese
						//		string into this function while the game was running in english (which should NEVER happen
						//		normally).  On the other hand I suppose it'psCurrentTextReadPos entirely possible that some taiwanese string
						//		might have no punctuation at all, so...
						//
						psBestLineBreakSrcPos = psLastGood_s; // force a break after last good letter
					}

					sLineForDisplay[psBestLineBreakSrcPos - psReadPosAtLineStart] = '\0';
					psReadPosAtLineStart = psCurrentTextReadPos = psBestLineBreakSrcPos;
					break; // print this line
				}
			}

			// record last-good linebreak pos...  (ie if we've just concat'd a punctuation point (western or asian) or space)
			//
			if (bIsTrailingPunctuation || uiLetter == ' ' || uiLetter > 255 && !cgi_Language_UsesSpaces())
			{
				psBestLineBreakSrcPos = psCurrentTextReadPos;
			}
		}

		// ... and print it...
		//
		cgi_R_Font_DrawString(iBoxX, iYpos, sLineForDisplay, v4Color, iFontHandle, -1, fScale);
		iYpos += iFontHeightAdvance;
		giLinesOutput++;

		// and echo to console in dev mode...
		//
		if (cg_developer.integer)
		{
			//			Com_Printf( "%psCurrentTextReadPos\n", sLineForDisplay );
		}
	}
	return psReadPosAtLineStart;
}

/*
===============================================================================

CAPTION TEXT

===============================================================================
*/
void CG_CaptionTextStop()
{
	cg.captionTextTime = 0;
}

// try and get the correct StripEd text (with retry) for a given reference...
//
// returns 0 if failed, else strlen...
//
static int cg_SP_GetStringTextStringWithRetry(const char* psReference, char* psDest, const int iSizeofDest)
{
	if (psReference[0] == '#')
	{
		// then we know the striped package name is already built in, so do NOT try prepending anything else...
		//
		return cgi_SP_GetStringTextString(va("%s", psReference + 1), psDest, iSizeofDest);
	}

	for (auto& i : cgs.stripLevelName)
	{
		if (i[0]) // entry present?
		{
			const int iReturn = cgi_SP_GetStringTextString(va("%s_%s", i, psReference), psDest, iSizeofDest);
			if (iReturn)
			{
				return iReturn;
			}
		}
	}

	return 0;
}

// slightly confusingly, the char arg for this function is an audio filename of the form "path/path/filename",
//	the "filename" part of which should be the same as the StripEd reference we're looking for in the current
//	level's string package...
//
void CG_CaptionText(const char* str, const int sound)
{
	char text[8192] = { 0 };

	const float fFontScale = cgi_Language_IsAsian() ? 0.8f : 1.0f;

	const char* holds = strrchr(str, '/');
	if (!holds)
	{
#ifndef FINAL_BUILD
		Com_Printf("WARNING: CG_CaptionText given audio filename with no '/':'%s'\n", str);
#endif
		return;
	}
	int i = cg_SP_GetStringTextStringWithRetry(holds + 1, text, sizeof text);
	//ensure we found a match
	if (!i)
	{
#ifndef FINAL_BUILD
		// we only care about some sound dirs...
		if (!Q_stricmpn(str, "sound/chars/", 12))	// whichever language it is, it'll be pathed as english at this point
		{
			Com_Printf("WARNING: CG_CaptionText given invalid text key: '%s'\n", str);
		}
		else
		{
			// anything else is probably stuff we don't care about. It certainly shouldn't be speech, anyway
		}
#endif
		return;
	}

	const int fontHeight = static_cast<int>((cgi_Language_IsAsian() ? 1.4f : 1.0f) * static_cast<float>(
		cgi_R_Font_HeightPixels(
			cgs.media.qhFontMedium, fFontScale))); // taiwanese & japanese need 1.5 fontheight spacing

	cg.captionTextTime = cg.time;
	if (in_camera)
	{
		cg.captionTextY = SCREEN_HEIGHT - client_camera.bar_height_dest / 2; // ths is now a centre'd Y, not a start Y
	}
	else
	{
		//get above the hud
		cg.captionTextY = static_cast<int>(0.88f * (static_cast<float>(SCREEN_HEIGHT) - static_cast<float>(fontHeight) *
			1.5f));
		// do NOT move this, it has to fit in between the weapon HUD and the datapad update.
	}
	cg.captionTextCurrentLine = 0;

	// count the number of lines for centering
	cg.scrollTextLines = 1;

	memset(cg.captionText, 0, sizeof cg.captionText);

	// Break into individual lines
	i = 0; // this could be completely removed and replace by "cg.scrollTextLines-1", but wtf?

	auto s = reinterpret_cast<const char*>(&text);
	// tai...
	//	s="賽卓哥爾博士已經安全了，我也把所有發現報告給「商店」。很不幸地，瑞士警局有些白癡發現了一些狀況，準備在機場逮捕亞歷西•納克瑞得。他偽裝成外交使節，穿過了層層防備。現在他握有人質，並且威脅要散播病毒。根據最新的報告，納克瑞得以及他的黨羽已經完全佔據了機場。我受命來追捕納克瑞得以及救出所有人質。這並不容易。";
	// kor...
	//	s="Wp:澗顫歜檜棻 詩萼. 斜菟檜 蜓и渠煎 啻陛 澀й雖 晦渠ж啊棻.澗顫歜檜棻 詩萼. 斜菟檜 蜓и渠煎 啻陛 澀й雖 晦渠ж啊棻.";
	holds = s;

	const int iPlayingTimeMS = cgi_S_GetSampleLength(sound);
	int iLengthInChars = strlen(s);
	//cgi_R_Font_StrLenChars(s);	// strlen is also good for MBCS in this instance, since it's for timing
	if (iLengthInChars == 0)
	{
		iLengthInChars = 1;
	}
	cg.captionLetterTime = iPlayingTimeMS / iLengthInChars;

	const char* psBestLineBreakSrcPos = s;
	while (*s)
	{
		const char* psLastGood_s = s;

		// read letter...
		//
		qboolean bIsTrailingPunctuation;
		int iAdvanceCount;
		const unsigned int uiLetter = cgi_AnyLanguage_ReadCharFromString(s, &iAdvanceCount, &bIsTrailingPunctuation);
		s += iAdvanceCount;

		// concat onto string so far...
		//
		if (uiLetter == 32 && cg.captionText[i][0] == '\0')
		{
			holds++;
			continue; // unless it's a space at the start of a line, in which case ignore it.
		}

		if (uiLetter > 255)
		{
			Q_strcat(cg.captionText[i], sizeof cg.captionText[i], va("%c%c", uiLetter >> 8, uiLetter & 0xFF));
		}
		else
		{
			Q_strcat(cg.captionText[i], sizeof cg.captionText[i], va("%c", uiLetter & 0xFF));
		}

		if (uiLetter == '\n')
		{
			// explicit new line...
			//
			cg.captionText[i][strlen(cg.captionText[i]) - 1] = '\0'; // kill the CR
			i++;
			holds = s;
			psBestLineBreakSrcPos = s;
			cg.scrollTextLines++;
		}
		else if (cgi_R_Font_StrLenPixels(cg.captionText[i], cgs.media.qhFontMedium, fFontScale) >= SCREEN_WIDTH)
		{
			// reached screen edge, so cap off string at bytepos after last good position...
			//
			if (uiLetter > 255 && bIsTrailingPunctuation && !cgi_Language_UsesSpaces())
			{
				// Special case, don't consider line breaking if you're on an asian punctuation char of
				//	a language that doesn't use spaces...
				//
			}
			else
			{
				if (psBestLineBreakSrcPos == holds)
				{
					//  aarrrggh!!!!!   we'll only get here is someone has fed in a (probably) garbage string,
					//		since it doesn't have a single space or punctuation mark right the way across one line
					//		of the screen.  So far, this has only happened in testing when I hardwired a taiwanese
					//		string into this function while the game was running in english (which should NEVER happen
					//		normally).  On the other hand I suppose it's entirely possible that some taiwanese string
					//		might have no punctuation at all, so...
					//
					psBestLineBreakSrcPos = psLastGood_s; // force a break after last good letter
				}

				cg.captionText[i][psBestLineBreakSrcPos - holds] = '\0';
				holds = s = psBestLineBreakSrcPos;
				i++;
				cg.scrollTextLines++;
			}
		}

		// record last-good linebreak pos...  (ie if we've just concat'd a punctuation point (western or asian) or space)
		//
		if (bIsTrailingPunctuation || uiLetter == ' ' || uiLetter > 255 && !cgi_Language_UsesSpaces())
		{
			psBestLineBreakSrcPos = s;
		}
	}

	// calc the length of time to hold each 2 lines of text on the screen.... presumably this works?
	//
	int holdTime = strlen(cg.captionText[0]);
	if (cg.scrollTextLines > 1)
	{
		holdTime += strlen(cg.captionText[1]); // strlen is also good for MBCS in this instance, since it's for timing
	}
	cg.captionNextTextTime = cg.time + holdTime * cg.captionLetterTime;

	cg.scrollTextTime = 0; // No scrolling during captions

	//Echo to console in dev mode
	if (cg_developer.integer)
	{
		Com_Printf("%s\n", cg.captionText[0]); // ste:  was [i], but surely sentence 0 is more useful than last?
	}
}

void CG_DrawCaptionText()
{
	if (!cg.captionTextTime)
	{
		return;
	}

	const float fFontScale = cgi_Language_IsAsian() ? 0.8f : 1.0f;

	if (cg_skippingcin.integer != 0)
	{
		cg.captionTextTime = 0;
		return;
	}

	if (cg.captionNextTextTime < cg.time)
	{
		cg.captionTextCurrentLine += 2;

		if (cg.captionTextCurrentLine >= cg.scrollTextLines)
		{
			cg.captionTextTime = 0;
			return;
		}
		int holdTime = strlen(cg.captionText[cg.captionTextCurrentLine]);
		if (cg.scrollTextLines >= cg.captionTextCurrentLine)
		{
			// ( strlen is also good for MBCS in this instance, since it's for timing -ste)
			//
			holdTime += strlen(cg.captionText[cg.captionTextCurrentLine + 1]);
		}

		cg.captionNextTextTime = cg.time + holdTime * cg.captionLetterTime; //50);
	}

	// Give a color if one wasn't given
	if (textcolor_caption[0] == 0 && textcolor_caption[1] == 0 &&
		textcolor_caption[2] == 0 && textcolor_caption[3] == 0)
	{
		VectorCopy4(colorTable[CT_WHITE], textcolor_caption);
	}

	cgi_R_SetColor(textcolor_caption);

	// Set Y of the first line (varies if only printing one line of text)
	// (this all works, please don't mess with it)
	const int fontHeight = static_cast<int>((cgi_Language_IsAsian() ? 1.4f : 1.0f) * static_cast<float>(
		cgi_R_Font_HeightPixels(
			cgs.media.qhFontMedium, fFontScale)));
	const bool bPrinting2Lines = !!cg.captionText[cg.captionTextCurrentLine + 1][0];
	int y = cg.captionTextY - static_cast<float>(fontHeight) * (bPrinting2Lines ? 1 : 0.5f);
	// captionTextY was a centered Y pos, not a top one
	y -= cgi_Language_IsAsian() ? 0 : 4;

	for (int i = cg.captionTextCurrentLine; i < cg.captionTextCurrentLine + 2; ++i)
	{
		const int w = cgi_R_Font_StrLenPixels(cg.captionText[i], cgs.media.qhFontMedium, fFontScale);
		if (w)
		{
			const int x = (SCREEN_WIDTH - w) / 2;
			cgi_R_Font_DrawString(x, y, cg.captionText[i], textcolor_caption, cgs.media.qhFontMedium, -1, fFontScale);
			y += fontHeight;
		}
	}

	cgi_R_SetColor(nullptr);
}

/*
===============================================================================

SCROLL TEXT

===============================================================================

CG_ScrollText - split text up into seperate lines

 'str' arg is StripEd string reference, eg "CREDITS_RAVEN"

*/
int giScrollTextPixelWidth = SCREEN_WIDTH;

void CG_ScrollText(const char* str, const int iPixelWidth)
{
	giScrollTextPixelWidth = iPixelWidth;

	// first, ask the strlen of the final string...
	//
	int i = cgi_SP_GetStringTextString(str, nullptr, 0);

	//ensure we found a match
	if (!i)
	{
#ifndef FINAL_BUILD
		Com_Printf("WARNING: CG_ScrollText given invalid text key :'%s'\n", str);
#endif
		return;
	}
	//
	// malloc space to hold it...
	//
	const auto psText = static_cast<char*>(cgi_Z_Malloc(i + 1, TAG_TEMP_WORKSPACE));
	//
	// now get the string...
	//
	i = cgi_SP_GetStringTextString(str, psText, i + 1);
	//ensure we found a match
	if (!i)
	{
		assert(0); // should never get here now, but wtf?
		cgi_Z_Free(psText);
#ifndef FINAL_BUILD
		Com_Printf("WARNING: CG_ScrollText given invalid text key :'%s'\n", str);
#endif
		return;
	}

	cg.scrollTextTime = cg.time;
	cg.printTextY = SCREEN_HEIGHT;
	cg.scrollTextLines = 1;

	const char* s = psText;
	i = 0;
	const char* holds = s;

	const char* psBestLineBreakSrcPos = s;
	while (*s)
	{
		const char* psLastGood_s = s;

		// read letter...
		//
		qboolean bIsTrailingPunctuation;
		int iAdvanceCount;
		const unsigned int uiLetter = cgi_AnyLanguage_ReadCharFromString(s, &iAdvanceCount, &bIsTrailingPunctuation);
		s += iAdvanceCount;

		// concat onto string so far...
		//
		if (uiLetter == 32 && cg.printText[i][0] == '\0')
		{
			holds++;
			continue; // unless it's a space at the start of a line, in which case ignore it.
		}

		if (uiLetter > 255)
		{
			Q_strcat(cg.printText[i], sizeof cg.printText[i], va("%c%c", uiLetter >> 8, uiLetter & 0xFF));
		}
		else
		{
			Q_strcat(cg.printText[i], sizeof cg.printText[i], va("%c", uiLetter & 0xFF));
		}

		// record last-good linebreak pos...  (ie if we've just concat'd a punctuation point (western or asian) or space)
		//
		if (bIsTrailingPunctuation || uiLetter == ' ')
		{
			psBestLineBreakSrcPos = s;
		}

		if (uiLetter == '\n')
		{
			// explicit new line...
			//
			cg.printText[i][strlen(cg.printText[i]) - 1] = '\0'; // kill the CR
			i++;
			assert(i < static_cast<int>(sizeof cg.printText / sizeof cg.printText[0]));
			if (i >= static_cast<int>(std::size(cg.printText)))
			{
				break;
			}
			holds = s;
			cg.scrollTextLines++;
		}
		else if (cgi_R_Font_StrLenPixels(cg.printText[i], cgs.media.qhFontMedium, 1.0f) >= iPixelWidth)
		{
			// reached screen edge, so cap off string at bytepos after last good position...
			//
			if (psBestLineBreakSrcPos == holds)
			{
				//  aarrrggh!!!!!   we'll only get here is someone has fed in a (probably) garbage string,
				//		since it doesn't have a single space or punctuation mark right the way across one line
				//		of the screen.  So far, this has only happened in testing when I hardwired a taiwanese
				//		string into this function while the game was running in english (which should NEVER happen
				//		normally).  On the other hand I suppose it's entirely possible that some taiwanese string
				//		might have no punctuation at all, so...
				//
				psBestLineBreakSrcPos = psLastGood_s; // force a break after last good letter
			}

			cg.printText[i][psBestLineBreakSrcPos - holds] = '\0';
			holds = s = psBestLineBreakSrcPos;
			i++;
			assert(i < static_cast<int>(sizeof cg.printText / sizeof cg.printText[0]));
			cg.scrollTextLines++;
		}
	}

	cg.captionTextTime = 0; // No captions during scrolling
	cgi_Z_Free(psText);
}

// draws using [textcolor_scroll]...
//
constexpr auto SCROLL_LPM = 1 / 50.0; // 1 line per 50 ms;
void CG_DrawScrollText()
{
	const int fontHeight = static_cast<int>(1.5f * static_cast<float>(cgi_R_Font_HeightPixels(
		cgs.media.qhFontMedium, 1.0f)));
	// taiwanese & japanese need 1.5 fontheight spacing

	if (!cg.scrollTextTime)
	{
		return;
	}

	cgi_R_SetColor(textcolor_scroll);

	int y = cg.printTextY - (cg.time - cg.scrollTextTime) * SCROLL_LPM;

	//	cgi_R_Font_DrawString(320, 200, va("Scrolltext printing @ %d",y), colorTable[CT_LTGOLD1], cgs.media.qhFontMedium, -1, 1.0f);

	// See if text has finished scrolling off screen
	if (y + cg.scrollTextLines * fontHeight < 1)
	{
		cg.scrollTextTime = 0;
		return;
	}

	for (int i = 0; i < cg.scrollTextLines; ++i)
	{
		// Is this line off top of screen?
		if (y + (i + 1) * fontHeight < 1)
		{
			y += fontHeight;
			continue;
		}
		// or past bottom of screen?
		if (y > SCREEN_HEIGHT)
		{
			break;
		}

		//		w = cgi_R_Font_StrLenPixels(cg.printText[i], cgs.media.qhFontMedium, 1.0f);
		//		if (w)
		{
			const int x = (SCREEN_WIDTH - giScrollTextPixelWidth) / 2;
			cgi_R_Font_DrawString(x, y, cg.printText[i], textcolor_scroll, cgs.media.qhFontMedium, -1, 1.0f);
			y += fontHeight;
		}
	}

	cgi_R_SetColor(nullptr);
}

/*
===============================================================================

CENTER PRINTING

===============================================================================
*/

/*
==============
CG_CenterPrint

Called for important messages that should stay in the center of the screen
for a few moments
==============
*/
void CG_CenterPrint(const char* str, const int y)
{
	// Find text to match the str given
	if (*str == '@')
	{
		const int i = cgi_SP_GetStringTextString(str + 1, cg.centerPrint, sizeof cg.centerPrint);

		if (!i)
		{
			Com_Printf(S_COLOR_RED"CG_CenterPrint: cannot find reference '%s' in StringPackage!\n", str);
			Q_strncpyz(cg.centerPrint, str, sizeof cg.centerPrint);
		}
	}
	else
	{
		Q_strncpyz(cg.centerPrint, str, sizeof cg.centerPrint);
	}

	cg.centerPrintTime = cg.time;
	cg.centerPrintY = y;

	// count the number of lines for centering
	cg.centerPrintLines = 1;
	char* s = cg.centerPrint;
	while (*s)
	{
		if (*s == '\n')
			cg.centerPrintLines++;
		s++;
	}
}

/*
===================
CG_DrawCenterString
===================
*/
void CG_DrawCenterString()
{
	if (!cg.centerPrintTime)
	{
		return;
	}

	const float* color = CG_FadeColor(cg.centerPrintTime, 1000 * 3);
	if (!color)
	{
		return;
	}

	if (textcolor_center[0] == 0 && textcolor_center[1] == 0 &&
		textcolor_center[2] == 0 && textcolor_center[3] == 0)
	{
		VectorCopy4(colorTable[CT_WHITE], textcolor_center);
	}

	char* start = cg.centerPrint;

	const int fontHeight = cgi_R_Font_HeightPixels(cgs.media.qhFontMedium, 1.0f);
	int y = cg.centerPrintY - cg.centerPrintLines * fontHeight / 2;

	while (true)
	{
		char linebuffer[1024];

		// this is kind of unpleasant when dealing with MBCS, but...
		//
		const char* psString = start;
		int iOutIndex = 0;
		for (unsigned int l = 0; l < sizeof linebuffer - 1; l++)
		{
			int i_advance_count;
			const unsigned int ui_letter = cgi_AnyLanguage_ReadCharFromString(psString, &i_advance_count);
			psString += i_advance_count;
			if (!ui_letter || ui_letter == '\n')
			{
				break;
			}
			if (ui_letter > 255)
			{
				linebuffer[iOutIndex++] = ui_letter >> 8;
				linebuffer[iOutIndex++] = ui_letter & 0xFF;
			}
			else
			{
				linebuffer[iOutIndex++] = ui_letter & 0xFF;
			}
		}
		linebuffer[iOutIndex++] = '\0';

		const int w = cgi_R_Font_StrLenPixels(linebuffer, cgs.media.qhFontMedium, 1.0f);

		const int x = (SCREEN_WIDTH - w) / 2;

		cgi_R_Font_DrawString(x, y, linebuffer, textcolor_center, cgs.media.qhFontMedium, -1, 1.0f);

		y += fontHeight;

		while (*start && *start != '\n')
		{
			start++;
		}
		if (!*start)
		{
			break;
		}
		start++;
	}
}