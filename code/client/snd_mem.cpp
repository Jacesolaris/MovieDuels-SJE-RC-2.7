/*
===========================================================================
Copyright (C) 1999 - 2005, Id Software, Inc.
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

// snd_mem.c: sound caching
#include "../server/exe_headers.h"

#include "snd_local.h"
#include "cl_mp3.h"

#include <string>

#ifdef USE_OPENAL
// Open AL
void S_PreProcessLipSync(const sfx_t* sfx);
extern int s_UseOpenAL;
#endif
/*
===============================================================================

WAV loading

===============================================================================
*/

byte* data_p;
byte* iff_end;
byte* last_chunk;
byte* iff_data;
int 	iff_chunk_len;
extern sfx_t		s_knownSfx[];
extern	int			s_numSfx;

extern cvar_t* s_lip_threshold_1;
extern cvar_t* s_lip_threshold_2;
extern cvar_t* s_lip_threshold_3;
extern cvar_t* s_lip_threshold_4;

short GetLittleShort()
{
	short val = *data_p;
	val = static_cast<short>(val + (*(data_p + 1) << 8));
	data_p += 2;
	return val;
}

int GetLittleLong()
{
	int val = *data_p;
	val = val + (*(data_p + 1) << 8);
	val = val + (*(data_p + 2) << 16);
	val = val + (*(data_p + 3) << 24);
	data_p += 4;
	return val;
}

void FindNextChunk(const char* name)
{
	while (true)
	{
		data_p = last_chunk;

		if (data_p >= iff_end)
		{	// didn't find the chunk
			data_p = nullptr;
			return;
		}

		data_p += 4;
		iff_chunk_len = GetLittleLong();
		if (iff_chunk_len < 0)
		{
			data_p = nullptr;
			return;
		}
		data_p -= 8;
		last_chunk = data_p + 8 + (iff_chunk_len + 1 & ~1);
		if (strncmp(reinterpret_cast<char*>(data_p), name, 4) == 0)
			return;
	}
}

void FindChunk(const char* name)
{
	last_chunk = iff_data;
	FindNextChunk(name);
}

void DumpChunks()
{
	char	str[5];

	str[4] = 0;
	data_p = iff_data;
	do
	{
		memcpy(str, data_p, 4);
		data_p += 4;
		iff_chunk_len = GetLittleLong();
		Com_Printf("0x%x : %s (%d)\n", reinterpret_cast<intptr_t>(data_p - 4), str, iff_chunk_len);
		data_p += iff_chunk_len + 1 & ~1;
	} while (data_p < iff_end);
}

/*
============
GetWavinfo
============
*/
wavinfo_t GetWavinfo(const char* name, byte* wav, const int wavlength)
{
	wavinfo_t	info = {};

	if (!wav)
		return info;

	iff_data = wav;
	iff_end = wav + wavlength;

	// find "RIFF" chunk
	FindChunk("RIFF");
	if (!(data_p && strncmp(reinterpret_cast<char*>(data_p) + 8, "WAVE", 4) == 0))
	{
		Com_Printf("Missing RIFF/WAVE chunks\n");
		return info;
	}

	// get "fmt " chunk
	iff_data = data_p + 12;
	// DumpChunks ();

	FindChunk("fmt ");
	if (!data_p)
	{
		Com_Printf("Missing fmt chunk\n");
		return info;
	}
	data_p += 8;
	info.format = GetLittleShort();
	info.channels = GetLittleShort();
	info.rate = GetLittleLong();
	data_p += 4 + 2;
	info.width = GetLittleShort() / 8;

	if (info.format != 1)
	{
		Com_Printf("Microsoft PCM format only\n");
		return info;
	}

	// find data chunk
	FindChunk("data");
	if (!data_p)
	{
		Com_Printf("Missing data chunk\n");
		return info;
	}

	data_p += 4;
	const int samples = GetLittleLong() / info.width;

	if (info.samples)
	{
		if (samples < info.samples)
			Com_Error(ERR_DROP, "Sound %s has a bad loop length", name);
	}
	else
		info.samples = samples;

	info.dataofs = data_p - wav;

	return info;
}

/*
================
ResampleSfx

resample / decimate to the current source rate
================
*/
void ResampleSfx(sfx_t* sfx, const int i_in_rate, const int i_in_width, byte* p_data)
{
	int		i_sample;

	const float f_step_scale = static_cast<float>(i_in_rate) / dma.speed;	// this is usually 0.5, 1, or 2

	// When stepscale is > 1 (we're downsampling), we really ought to run a low pass filter on the samples

	const int i_out_count = static_cast<int>(sfx->iSoundLengthInSamples / f_step_scale);
	sfx->iSoundLengthInSamples = i_out_count;

	sfx->pSoundData = reinterpret_cast<short*>(SND_malloc(sfx->iSoundLengthInSamples * 2, sfx));

	sfx->fVolRange = 0;
	unsigned int ui_sample_frac = 0;
	const unsigned int ui_frac_step = static_cast<int>(f_step_scale * 256);

	for (int i = 0; i < sfx->iSoundLengthInSamples; i++)
	{
		const int i_src_sample = ui_sample_frac >> 8;
		ui_sample_frac += ui_frac_step;
		if (i_in_width == 2) {
			i_sample = LittleShort reinterpret_cast<short*>(p_data)[i_src_sample];
		}
		else {
			i_sample = (p_data[i_src_sample] - 128) << 8;
		}

		sfx->pSoundData[i] = static_cast<short>(i_sample);

		// work out max vol for this sample...
		//
		if (i_sample < 0)
			i_sample = -i_sample;
		if (sfx->fVolRange < i_sample >> 8)
		{
			sfx->fVolRange = i_sample >> 8;
		}
	}
}

//=============================================================================

void S_LoadSound_Finalize(const wavinfo_t* info, sfx_t* sfx, byte* data)
{
	sfx->eSoundCompressionMethod = ct_16;
	sfx->iSoundLengthInSamples = info->samples;
	ResampleSfx(sfx, info->rate, info->width, data + info->dataofs);
}

// maybe I'm re-inventing the wheel, here, but I can't see any functions that already do this, so...
//
char* Filename_WithoutPath(const char* ps_filename)
{
	static char s_string[MAX_QPATH];	// !!
	const char* p = strrchr(ps_filename, '\\');

	if (!p++)
		p = ps_filename;

	strcpy(s_string, p);

	return s_string;
}

// returns (eg) "\dir\name" for "\dir\name.bmp"
//
char* Filename_WithoutExt(const char* ps_filename)
{
	static char s_string[MAX_QPATH];	// !

	strcpy(s_string, ps_filename);

	char* p = strrchr(s_string, '.');
	const char* p2 = strrchr(s_string, '\\');

	// special check, make sure the first suffix we found from the end wasn't just a directory suffix (eg on a path'd filename with no extension anyway)
	//
	if (p && (p2 == nullptr || p2 && p > p2))
		*p = 0;

	return s_string;
}

int iFilesFound;
int iFilesUpdated;
int iErrors;
qboolean qbForceRescan;
qboolean qbForceStereo;
std::string strErrors;

void R_CheckMP3s(const char* ps_dir)
{
	//	Com_Printf(va("Scanning Dir: %s\n",psDir));
	Com_Printf(".");	// stops useful info scrolling off screen

	int		num_sys_files, i, numdirs;

	char** dir_files = FS_ListFiles(ps_dir, "/", &numdirs);
	if (numdirs > 2)
	{
		for (i = 2; i < numdirs; i++)
		{
			char	s_dir_name[MAX_QPATH];
			sprintf(s_dir_name, "%s\\%s", ps_dir, dir_files[i]);
			R_CheckMP3s(s_dir_name);
		}
	}

	char** sys_files = FS_ListFiles(ps_dir, ".mp3", &num_sys_files);
	for (i = 0; i < num_sys_files; i++)
	{
		char	s_filename[MAX_QPATH];
		sprintf(s_filename, "%s\\%s", ps_dir, sys_files[i]);

		Com_Printf("%sFound file: %s", !i ? "\n" : "", s_filename);

		iFilesFound++;

		// read it in...
		//
		byte* pb_data = nullptr;
		const int i_size = FS_ReadFile(s_filename, reinterpret_cast<void**>(&pb_data));

		if (pb_data)
		{
			id3v1_1* p_tag;

			// do NOT check 'qbForceRescan' here as an opt, because we need to actually fill in 'pTAG' if there is one...
			//
			const qboolean qb_tag_needs_updating = /* qbForceRescan || */ !MP3_ReadSpecialTagInfo(pb_data, i_size, &p_tag) ? qtrue : qfalse;

			if (p_tag == nullptr || qb_tag_needs_updating || qbForceRescan)
			{
				Com_Printf(" ( Updating )\n");

				// I need to scan this file to get the volume...
				//
				// For EF1 I used a temp sfx_t struct, but I can't do that now with this new alloc scheme,
				//	I have to ask for it legally, so I'll keep re-using one, and restoring it's name after use.
				//	(slightly dodgy, but works ok if no-one else changes stuff)
				//
				//sfx_t SFX = {0};
				extern sfx_t* s_find_name(const char* name);
				//
				static sfx_t* p_sfx = nullptr;
				constexpr char s_reserved_sfx_entryname_for_mp3[] = "reserved_for_mp3";	// ( strlen() < MAX_QPATH )

				if (p_sfx == nullptr)	// once only
				{
					p_sfx = s_find_name(s_reserved_sfx_entryname_for_mp3);	// always returns, else ERR_FATAL
				}

				if (MP3_IsValid(s_filename, pb_data, i_size, qbForceStereo))
				{
					wavinfo_t info;

					const int i_raw_pcm_data_size = MP3_GetUnpackedSize(s_filename, pb_data, i_size, qtrue, qbForceStereo);

					if (i_raw_pcm_data_size)	// should always be true, unless file is fucked, in which case, stop this conversion process
					{
						float f_max_vol = 128;	// any old default
						int i_actual_unpacked_size = i_raw_pcm_data_size;	// default, override later if not doing music

						if (!qbForceStereo)	// no point for stereo files, which are for music and therefore no lip-sync
						{
							byte* pb_unpack_buffer = static_cast<byte*>(Z_Malloc(i_raw_pcm_data_size + 10, TAG_TEMP_WORKSPACE, qfalse));	// won't return if fails

							i_actual_unpacked_size = MP3_UnpackRawPCM(s_filename, pb_data, i_size, pb_unpack_buffer);
							if (i_actual_unpacked_size != i_raw_pcm_data_size)
							{
								Com_Error(ERR_DROP, "******* Whoah! MP3 %s unpacked to %d bytes, but size calc said %d!\n", s_filename, i_actual_unpacked_size, i_raw_pcm_data_size);
							}

							// fake up a WAV structure so I can use the other post-load sound code such as volume calc for lip-synching
							//
							MP3_FakeUpWAVInfo(s_filename, pb_data, i_size, i_actual_unpacked_size,
								// these params are all references...
								info.format, info.rate, info.width, info.channels, info.samples, info.dataofs
							);

							S_LoadSound_Finalize(&info, p_sfx, pb_unpack_buffer);	// all this just for lipsynch. Oh well.

							f_max_vol = p_sfx->fVolRange;

							// free sfx->data...
							//
							{
#ifndef INT_MIN
#define INT_MIN     (-2147483647 - 1) /* minimum (signed) int value */
#endif
								//
								p_sfx->iLastTimeUsed = INT_MIN;		// force this to be oldest sound file, therefore disposable...
								p_sfx->bInMemory = true;
								SND_FreeOldestSound();		// ... and do the disposal

								// now set our temp SFX struct back to default name so nothing else accidentally uses it...
								//
								strcpy(p_sfx->sSoundName, s_reserved_sfx_entryname_for_mp3);
								p_sfx->bDefaultSound = false;
							}

							//							OutputDebugString(va("File: \"%s\"   MaxVol %f\n",sFilename,pSFX->fVolRange));

														// other stuff...
														//
							Z_Free(pb_unpack_buffer);
						}

						// well, time to update the file now...
						//
						const fileHandle_t f = FS_FOpenFileWrite(s_filename);
						if (f)
						{
							// write the file back out, but omitting the tag if there was one...
							//
							const int i_written = FS_Write(pb_data, i_size - (p_tag ? sizeof * p_tag : 0), f);

							if (i_written)
							{
								// make up a new tag if we didn't find one in the original file...
								//
								id3v1_1 tag;
								if (!p_tag)
								{
									p_tag = &tag;
									memset(&tag, 0, sizeof tag);
									strncpy(p_tag->id, "TAG", 3);
								}

								strncpy(p_tag->title, Filename_WithoutPath(Filename_WithoutExt(s_filename)), sizeof p_tag->title);
								strncpy(p_tag->artist, "Raven Software", sizeof p_tag->artist);
								strncpy(p_tag->year, "2002", sizeof p_tag->year);
								strncpy(p_tag->comment, va("%s %g", sKEY_MAXVOL, f_max_vol), sizeof p_tag->comment);
								strncpy(p_tag->album, va("%s %d", sKEY_UNCOMP, i_actual_unpacked_size), sizeof p_tag->album);

								if (FS_Write(p_tag, sizeof * p_tag, f))	// NZ = success
								{
									iFilesUpdated++;
								}
								else
								{
									Com_Printf("*********** Failed write to file \"%s\"!\n", s_filename);
									iErrors++;
									strErrors += va("Failed to write: \"%s\"\n", s_filename);
								}
							}
							else
							{
								Com_Printf("*********** Failed write to file \"%s\"!\n", s_filename);
								iErrors++;
								strErrors += va("Failed to write: \"%s\"\n", s_filename);
							}
							FS_FCloseFile(f);
						}
						else
						{
							Com_Printf("*********** Failed to re-open for write \"%s\"!\n", s_filename);
							iErrors++;
							strErrors += va("Failed to re-open for write: \"%s\"\n", s_filename);
						}
					}
					else
					{
						Com_Error(ERR_DROP, "******* This MP3 should be deleted: \"%s\"\n", s_filename);
					}
				}
				else
				{
					Com_Printf("*********** File was not a valid MP3!: \"%s\"\n", s_filename);
					iErrors++;
					strErrors += va("Not game-legal MP3 format: \"%s\"\n", s_filename);
				}
			}
			else
			{
				Com_Printf(" ( OK )\n");
			}

			FS_FreeFile(pb_data);
		}
	}
	FS_FreeFileList(sys_files);
	FS_FreeFileList(dir_files);
}

// this console-function is for development purposes, and makes sure that sound/*.mp3 /s have tags in them
//	specifying stuff like their max volume (and uncompressed size) etc...
//
void S_MP3_CalcVols_f()
{
	char s_start_dir[MAX_QPATH] = { "sound" };
	constexpr char s_usage[] = "Usage: mp3_calcvols [-rescan] <startdir>\ne.g. mp3_calcvols sound/chars";

	if (Cmd_Argc() == 1 || Cmd_Argc() > 4)	// 3 optional arguments
	{
		Com_Printf(s_usage);
		return;
	}

	S_StopAllSounds();

	qbForceRescan = qfalse;
	qbForceStereo = qfalse;
	iFilesFound = 0;
	iFilesUpdated = 0;
	iErrors = 0;
	strErrors = "";

	for (int i = 1; i < Cmd_Argc(); i++)
	{
		if (Cmd_Argv(i)[0] == '-')
		{
			if (!Q_stricmp(Cmd_Argv(i), "-rescan"))
			{
				qbForceRescan = qtrue;
			}
			else
				if (!Q_stricmp(Cmd_Argv(i), "-stereo"))
				{
					qbForceStereo = qtrue;
				}
				else
				{
					// unknown switch...
					//
					Com_Printf(s_usage);
					return;
				}
			continue;
		}
		strcpy(s_start_dir, Cmd_Argv(i));
	}

	Com_Printf(va("Starting Scan for Updates in Dir: %s\n", s_start_dir));
	R_CheckMP3s(s_start_dir);

	Com_Printf("\n%d files found/scanned, %d files updated      ( %d errors total)\n", iFilesFound, iFilesUpdated, iErrors);

	if (iErrors)
	{
		Com_Printf("\nBad Files:\n%s\n", strErrors.c_str());
	}
}

// adjust filename for foreign languages and WAV/MP3 issues.
//
// returns qfalse if failed to load, else fills in *p_data
//
extern	cvar_t* com_buildScript;
static qboolean S_LoadSound_FileLoadAndNameAdjuster(char* ps_filename, byte** p_data, int* pi_size, const int i_name_strlen)
{
	char* psVoice = strstr(ps_filename, "chars");
	if (psVoice)
	{
		// cache foreign voices...
		//
		if (com_buildScript->integer)
		{
			fileHandle_t h_file;
			//German
			strncpy(psVoice, "chr_d", 5);	// same number of letters as "chars"
			FS_FOpenFileRead(ps_filename, &h_file, qfalse);		//cache the wav
			if (!h_file)
			{
				strcpy(&ps_filename[i_name_strlen - 3], "mp3");		//not there try mp3
				FS_FOpenFileRead(ps_filename, &h_file, qfalse);	//cache the mp3
			}
			if (h_file)
			{
				FS_FCloseFile(h_file);
			}
			strcpy(&ps_filename[i_name_strlen - 3], "wav");	//put it back to wav

			//French
			strncpy(psVoice, "chr_f", 5);	// same number of letters as "chars"
			FS_FOpenFileRead(ps_filename, &h_file, qfalse);		//cache the wav
			if (!h_file)
			{
				strcpy(&ps_filename[i_name_strlen - 3], "mp3");		//not there try mp3
				FS_FOpenFileRead(ps_filename, &h_file, qfalse);	//cache the mp3
			}
			if (h_file)
			{
				FS_FCloseFile(h_file);
			}
			strcpy(&ps_filename[i_name_strlen - 3], "wav");	//put it back to wav

			//czech
			strncpy(psVoice, "chr_cz", 5);	// same number of letters as "chars"
			FS_FOpenFileRead(ps_filename, &h_file, qfalse);		//cache the wav
			if (!h_file)
			{
				strcpy(&ps_filename[i_name_strlen - 3], "mp3");		//not there try mp3
				FS_FOpenFileRead(ps_filename, &h_file, qfalse);	//cache the mp3
			}
			if (h_file)
			{
				FS_FCloseFile(h_file);
			}
			strcpy(&ps_filename[i_name_strlen - 3], "wav");	//put it back to wav

			//Spanish
			strncpy(psVoice, "chr_e", 5);	// same number of letters as "chars"
			FS_FOpenFileRead(ps_filename, &h_file, qfalse);		//cache the wav
			if (!h_file)
			{
				strcpy(&ps_filename[i_name_strlen - 3], "mp3");		//not there try mp3
				FS_FOpenFileRead(ps_filename, &h_file, qfalse);	//cache the mp3
			}
			if (h_file)
			{
				FS_FCloseFile(h_file);
			}
			strcpy(&ps_filename[i_name_strlen - 3], "wav");	//put it back to wav

			strncpy(psVoice, "chars", 5);	//put it back to chars
		}

		// account for foreign voices...
		//
		extern cvar_t* s_language;
		if (s_language && Q_stricmp("DEUTSCH", s_language->string) == 0)
		{
			strncpy(psVoice, "chr_d", 5);	// same number of letters as "chars"
		}
		else if (s_language && Q_stricmp("FRANCAIS", s_language->string) == 0)
		{
			strncpy(psVoice, "chr_f", 5);	// same number of letters as "chars"
		}
		else if (s_language && Q_stricmp("ESPANOL", s_language->string) == 0)
		{
			strncpy(psVoice, "chr_e", 5);	// same number of letters as "chars"
		}
		else if (s_language && Q_stricmp("CZECH", s_language->string) == 0)
		{
			strncpy(psVoice, "chr_cz", 5);	// same number of letters as "chars"
		}
		else
		{
			psVoice = nullptr;	// use this ptr as a flag as to whether or not we substituted with a foreign version
		}
	}

	*pi_size = FS_ReadFile(ps_filename, reinterpret_cast<void**>(p_data));	// try WAV
	if (!*p_data) {
		ps_filename[i_name_strlen - 3] = 'm';
		ps_filename[i_name_strlen - 2] = 'p';
		ps_filename[i_name_strlen - 1] = '3';
		*pi_size = FS_ReadFile(ps_filename, reinterpret_cast<void**>(p_data));	// try MP3

		if (!*p_data)
		{
			//hmmm, not found, ok, maybe we were trying a foreign noise ("arghhhhh.mp3" that doesn't matter?) but it
			// was missing?   Can't tell really, since both types are now in sound/chars. Oh well, fall back to English for now...

			if (psVoice)	// were we trying to load foreign?
			{
				// yep, so fallback to re-try the english...
				//
#ifndef FINAL_BUILD
				Com_Printf(S_COLOR_YELLOW "Foreign file missing: \"%s\"! (using English...)\n", ps_filename);
#endif

				strncpy(psVoice, "chars", 5);

				ps_filename[i_name_strlen - 3] = 'w';
				ps_filename[i_name_strlen - 2] = 'a';
				ps_filename[i_name_strlen - 1] = 'v';
				*pi_size = FS_ReadFile(ps_filename, reinterpret_cast<void**>(p_data));	// try English WAV
				if (!*p_data)
				{
					ps_filename[i_name_strlen - 3] = 'm';
					ps_filename[i_name_strlen - 2] = 'p';
					ps_filename[i_name_strlen - 1] = '3';
					*pi_size = FS_ReadFile(ps_filename, reinterpret_cast<void**>(p_data));	// try English MP3
				}
			}

			if (!*p_data)
			{
				return qfalse;	// sod it, give up...
			}
		}
	}

	return qtrue;
}

// returns qtrue if this dir is allowed to keep loaded MP3s, else qfalse if they should be WAV'd instead...
//
// note that this is passed the original, un-language'd name
//
// (I was going to remove this, but on kejim_post I hit an assert because someone had got an ambient sound when the
//	perimter fence goes online that was an MP3, then it tried to get added as looping. Presumably it sounded ok or
//	they'd have noticed, but we therefore need to stop other levels using those. "sound/ambience" I can check for,
//	but doors etc could be anything. Sigh...)
//
constexpr auto SOUND_CHARS_DIR = "sound/chars/";
constexpr auto SOUND_CHARS_DIR_LENGTH = 12; // strlen( SOUND_CHARS_DIR );
static qboolean S_LoadSound_DirIsAllowedToKeepMP3s(const char* ps_filename)
{
	if (Q_stricmpn(ps_filename, SOUND_CHARS_DIR, SOUND_CHARS_DIR_LENGTH) == 0)
		return qtrue;	// found a dir that's allowed to keep MP3s

	return qfalse;
}

/*
==============
S_LoadSound

The filename may be different than sfx->name in the case
of a forced fallback of a player specific sound	(or of a wav/mp3 substitution now -Ste)
==============
*/
qboolean gbInsideLoadSound = qfalse;
static qboolean S_LoadSound_Actual(sfx_t* sfx)
{
	byte* data;
	wavinfo_t	info;
	int		size;
	char	s_load_name[MAX_QPATH];

	int		len = strlen(sfx->sSoundName);
	if (len < 5)
	{
		return qfalse;
	}

	// player specific sounds are never directly loaded...
	//
	if (sfx->sSoundName[0] == '*') {
		return qfalse;
	}
	// make up a local filename to try wav/mp3 substitutes...
	//
	Q_strncpyz(s_load_name, sfx->sSoundName, sizeof s_load_name);
	Q_strlwr(s_load_name);
	//
	// Ensure name has an extension (which it must have, but you never know), and get ptr to it...
	//
	const char* ps_ext = &s_load_name[strlen(s_load_name) - 4];
	if (*ps_ext != '.')
	{
		//Com_Printf( "WARNING: soundname '%s' does not have 3-letter extension\n",sLoadName);
		COM_DefaultExtension(s_load_name, sizeof s_load_name, ".wav");	// so psExt below is always valid
		ps_ext = &s_load_name[strlen(s_load_name) - 4];
		len = strlen(s_load_name);
	}

	if (!S_LoadSound_FileLoadAndNameAdjuster(s_load_name, &data, &size, len))
	{
		return qfalse;
	}

	SND_TouchSFX(sfx);
	//=========
	if (Q_stricmpn(ps_ext, ".mp3", 4) == 0)
	{
		// load MP3 file instead...
		//
		if (MP3_IsValid(s_load_name, data, size, qfalse))
		{
			const int i_raw_pcm_data_size = MP3_GetUnpackedSize(s_load_name, data, size, qfalse, qfalse);

			if (S_LoadSound_DirIsAllowedToKeepMP3s(sfx->sSoundName)	// NOT sLoadName, this uses original un-languaged name
				&&
				MP3Stream_InitFromFile(sfx, data, size, s_load_name, i_raw_pcm_data_size + 2304 /* + 1 MP3 frame size, jic */, qfalse)
				)
			{
				//				Com_DPrintf("(Keeping file \"%s\" as MP3)\n",sLoadName);

#ifdef USE_OPENAL
				if (s_UseOpenAL)
				{
					// Create space for lipsync data (4 lip sync values per streaming AL buffer)
					if (strstr(sfx->sSoundName, "chars"))
						sfx->lipSyncData = static_cast<char*>(Z_Malloc(16, TAG_SND_RAWDATA, qfalse));
					else
						sfx->lipSyncData = nullptr;
				}
#endif
			}
			else
			{
				// small file, not worth keeping as MP3 since it would increase in size (with MP3 header etc)...
				//
				Com_DPrintf("S_LoadSound: Unpacking MP3 file \"%s\" to wav.\n", s_load_name);
				//
				// unpack and convert into WAV...
				//
				{
					{
						byte* pb_unpack_buffer = static_cast<byte*>(Z_Malloc(i_raw_pcm_data_size + 10 + 2304 /* <g> */, TAG_TEMP_WORKSPACE, qfalse));
						const int i_result_bytes = MP3_UnpackRawPCM(s_load_name, data, size, pb_unpack_buffer, qfalse);

						if (i_result_bytes != i_raw_pcm_data_size) {
							Com_Printf(S_COLOR_YELLOW"**** MP3 %s final unpack size %d different to previous value %d\n", s_load_name, i_result_bytes, i_raw_pcm_data_size);
							//assert (iResultBytes == iRawPCMDataSize);
						}

						// fake up a WAV structure so I can use the other post-load sound code such as volume calc for lip-synching
						//
						// (this is a bit crap really, but it lets me drop through into existing code)...
						//
						MP3_FakeUpWAVInfo(s_load_name, data, size, i_result_bytes,
							// these params are all references...
							info.format, info.rate, info.width, info.channels, info.samples, info.dataofs,
							qfalse
						);

						S_LoadSound_Finalize(&info, sfx, pb_unpack_buffer);

#ifdef Q3_BIG_ENDIAN
						// the MP3 decoder returns the samples in the correct endianness, but ResampleSfx byteswaps them,
						// so we have to swap them again...
						sfx->fVolRange = 0;

						for (int i = 0; i < sfx->iSoundLengthInSamples; i++)
						{
							sfx->pSoundData[i] = LittleShort(sfx->pSoundData[i]);
							// C++11 defines double abs(short) which is not what we want here,
							// because double >> int is not defined. Force interpretation as int
							if (sfx->fVolRange < (abs(static_cast<int>(sfx->pSoundData[i])) >> 8))
							{
								sfx->fVolRange = abs(static_cast<int>(sfx->pSoundData[i])) >> 8;
							}
						}
#endif

						// Open AL
#ifdef USE_OPENAL
						if (s_UseOpenAL)
						{
							if (strstr(sfx->sSoundName, "chars"))
							{
								sfx->lipSyncData = static_cast<char*>(Z_Malloc(sfx->iSoundLengthInSamples / 1000 + 1, TAG_SND_RAWDATA, qfalse));
								S_PreProcessLipSync(sfx);
							}
							else
								sfx->lipSyncData = nullptr;

							// Clear Open AL Error state
							alGetError();

							// Generate AL Buffer
							ALuint buffer;
							alGenBuffers(1, &buffer);
							if (alGetError() == AL_NO_ERROR)
							{
								// Copy audio data to AL Buffer
								alBufferData(buffer, AL_FORMAT_MONO16, sfx->pSoundData, sfx->iSoundLengthInSamples * 2, 22050);
								if (alGetError() == AL_NO_ERROR)
								{
									sfx->Buffer = buffer;
									Z_Free(sfx->pSoundData);
									sfx->pSoundData = nullptr;
								}
							}
						}
#endif

						Z_Free(pb_unpack_buffer);
					}
				}
			}
		}
		else
		{
			// MP3_IsValid() will already have printed any errors via Com_Printf at this point...
			//
			FS_FreeFile(data);
			return qfalse;
		}
	}
	else
	{
		// loading a WAV, presumably...

//=========

		info = GetWavinfo(s_load_name, data, size);
		if (info.channels != 1) {
			Com_Printf("%s is a stereo wav file\n", s_load_name);
			FS_FreeFile(data);
			return qfalse;
		}

		/*		if ( info.width == 1 ) {
					Com_Printf(S_COLOR_YELLOW "WARNING: %s is a 8 bit wav file\n", sLoadName);
				}

				if ( info.rate != 22050 ) {
					Com_Printf(S_COLOR_YELLOW "WARNING: %s is not a 22kHz wav file\n", sLoadName);
				}
		*/
		const auto samples = static_cast<short*>(Z_Malloc(info.samples * sizeof(short) * 2, TAG_TEMP_WORKSPACE, qfalse));

		sfx->eSoundCompressionMethod = ct_16;
		sfx->iSoundLengthInSamples = info.samples;
		sfx->pSoundData = nullptr;
		ResampleSfx(sfx, info.rate, info.width, data + info.dataofs);

		// Open AL
#ifdef USE_OPENAL
		if (s_UseOpenAL)
		{
			if (strstr(sfx->sSoundName, "chars") || strstr(sfx->sSoundName, "CHARS"))
			{
				sfx->lipSyncData = static_cast<char*>(Z_Malloc(sfx->iSoundLengthInSamples / 1000 + 1, TAG_SND_RAWDATA, qfalse));
				S_PreProcessLipSync(sfx);
			}
			else
				sfx->lipSyncData = nullptr;

			// Clear Open AL Error State
			alGetError();

			// Generate AL Buffer
			ALuint buffer;
			alGenBuffers(1, &buffer);
			if (alGetError() == AL_NO_ERROR)
			{
				// Copy audio data to AL Buffer
				alBufferData(buffer, AL_FORMAT_MONO16, sfx->pSoundData, sfx->iSoundLengthInSamples * 2, 22050);
				if (alGetError() == AL_NO_ERROR)
				{
					// Store AL Buffer in sfx struct, and release sample data
					sfx->Buffer = buffer;
					Z_Free(sfx->pSoundData);
					sfx->pSoundData = nullptr;
				}
			}
		}
#endif

		Z_Free(samples);
	}

	FS_FreeFile(data);

	return qtrue;
}

// wrapper function for above so I can guarantee that we don't attempt any audio-dumping during this call because
//	of a z_malloc() fail recovery...
//
qboolean S_LoadSound(sfx_t* sfx)
{
	gbInsideLoadSound = qtrue;	// !!!!!!!!!!!!!

	const qboolean b_return = S_LoadSound_Actual(sfx);

	gbInsideLoadSound = qfalse;	// !!!!!!!!!!!!!

	return b_return;
}

#ifdef USE_OPENAL
/*
	Precalculate the lipsync values for the whole sample
*/
void S_PreProcessLipSync(const sfx_t* sfx)
{
	int i;
	int sample;
	int sample_total = 0;

	int j = 0;
	for (i = 0; i < sfx->iSoundLengthInSamples; i += 100)
	{
		sample = LittleShort sfx->pSoundData[i];

		sample = sample >> 8;
		sample_total += sample * sample;
		if ((i + 100) % 1000 == 0)
		{
			sample_total /= 10;

			if (sample_total < sfx->fVolRange * s_lip_threshold_1->value)
			{
				// tell the scripts that are relying on this that we are still going, but actually silent right now.
				sample = -1;
			}
			else if (sample_total < sfx->fVolRange * s_lip_threshold_2->value)
				sample = 1;
			else if (sample_total < sfx->fVolRange * s_lip_threshold_3->value)
				sample = 2;
			else if (sample_total < sfx->fVolRange * s_lip_threshold_4->value)
				sample = 3;
			else
				sample = 4;

			sfx->lipSyncData[j] = sample;
			j++;

			sample_total = 0;
		}
	}

	if (i % 1000 == 0)
		return;

	i -= 100;
	i = i % 1000;
	i = i / 100;
	// Process last < 1000 samples
	if (i != 0)
		sample_total /= i;
	else
		sample_total = 0;

	if (sample_total < sfx->fVolRange * s_lip_threshold_1->value)
	{
		// tell the scripts that are relying on this that we are still going, but actually silent right now.
		sample = -1;
	}
	else if (sample_total < sfx->fVolRange * s_lip_threshold_2->value)
		sample = 1;
	else if (sample_total < sfx->fVolRange * s_lip_threshold_3->value)
		sample = 2;
	else if (sample_total < sfx->fVolRange * s_lip_threshold_4->value)
		sample = 3;
	else
		sample = 4;

	sfx->lipSyncData[j] = sample;
}
#endif