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

#ifndef __G_PUBLIC_H__
#define __G_PUBLIC_H__
// g_public.h -- game module information visible to server

#define	GAME_API_VERSION	10

// entity->svFlags
// the server does not know how to interpret most of the values
// in entityStates (level eType), so the game must explicitly flag
// special server behaviors
#define	SVF_NOCLIENT			0x00000001	// don't send entity to clients, even if it has effects
#define SVF_INACTIVE			0x00000002	// Can't be used when this flag is on
#define SVF_NPC					0x00000004
#define SVF_BOT					0x00000008
#define SVF_PLAYER_USABLE		0x00000010	// player can use this with the use button
#define	SVF_BROADCAST			0x00000020	// send to all connected clients
#define	SVF_PORTAL				0x00000040	// merge a second pvs at origin2 into snapshots
#define	SVF_USE_CURRENT_ORIGIN	0x00000080	// entity->currentOrigin instead of entity->s.origin
// for link position (missiles and movers)
#define SVF_TRIMODEL			0x00000100	// Use a three piece model make up like a player does
#define	SVF_OBJECTIVE			0x00000200	// Draw it's name if crosshair comes across it
#define	SVF_ANIMATING			0x00000400	// Currently animating from start_frame to end_frame
#define	SVF_NPC_PRECACHE		0x00000800	// Tell cgame to precache this spawner's NPC stuff
#define	SVF_KILLED_SELF			0x00001000	// ent killed itself in a script, so don't do ICARUS_FreeEnt on it... or else!
#define	SVF_NAVGOAL				0x00002000	// Navgoal
#define	SVF_NOPUSH				0x00004000	// Can't be pushed around
#define	SVF_ICARUS_FREEZE		0x00008000	// NPCs are frozen, ents don't execute ICARUS commands
#define SVF_PLAT				0x00010000	// A func_plat or door acting like one
#define	SVF_BBRUSH				0x00020000	// breakable brush
#define	SVF_LOCKEDENEMY			0x00040000	// keep current enemy until dead
#define SVF_IGNORE_ENEMIES		0x00080000	// Ignore all enemies
#define SVF_BEAMING				0x00100000	// Being transported
#define SVF_PLAYING_SOUND		0x00200000	// In the middle of playing a sound
#define SVF_CUSTOM_GRAVITY		0x00400000	// Use personal gravity
#define SVF_BROKEN				0x00800000	// A broken misc_model_breakable
#define SVF_NO_TELEPORT			0x01000000	// Will not be teleported
#define SVF_NONNPC_ENEMY		0x02000000	// Not a client/NPC, but can still be considered a hostile lifeform
#define SVF_SELF_ANIMATING		0x04000000	// Ent will control it's animation itself in it's think func
#define SVF_GLASS_BRUSH			0x08000000	// Ent is a glass brush
#define SVF_NO_BASIC_SOUNDS		0x10000000	// Don't load basic custom sound set
#define SVF_NO_COMBAT_SOUNDS	0x20000000	// Don't load combat custom sound set
#define SVF_NO_EXTRA_SOUNDS		0x40000000	// Don't load extra custom sound set
#define SVF_MOVER_ADJ_AREA_PORTALS	0x80000000	// For scripted movers only- must *explicitly instruct* them to affect area portals

//===============================================================

//rww - RAGDOLL_BEGIN
class CRagDollUpdateParams;
class CRagDollParams;
//rww - RAGDOLL_END

using gentity_t = struct gentity_s;
//typedef struct gclient_s gclient_t;

using SavedGameJustLoaded_e = enum
{
	eNO = 0,
	eFULL,
	eAUTO,
};

#ifndef GAME_INCLUDE

// the server needs to know enough information to handle collision and snapshot generation
template<typename TSaberInfo>
class PlayerStateBase;

using playerState_t = PlayerStateBase<saberInfo_t>;

struct gentity_s {
	entityState_t	s;				// communicated by server to clients
	playerState_t* client;
	qboolean	inuse;
	qboolean	linked;				// qfalse if not in any good cluster

	int			svFlags;			// SVF_NOCLIENT, SVF_BROADCAST, etc

	qboolean	bmodel;				// if false, assume an explicit mins / maxs bounding box
	// only set by gi.SetBrushModel
	vec3_t		mins, maxs;
	int			contents;			// CONTENTS_TRIGGER, CONTENTS_SOLID, CONTENTS_BODY, etc
	// a non-solid entity should set to 0

	vec3_t		absmin, absmax;		// derived from mins/maxs and origin + rotation

	// currentOrigin will be used for all collision detection and world linking.
	// it will not necessarily be the same as the trajectory evaluation for the current
	// time, because each entity must be moved one at a time after time is advanced
	// to avoid simultanious collision issues
	vec3_t		currentOrigin;
	vec3_t		currentAngles;

	gentity_t* owner;				// objects never interact with their owners, to
	// prevent player missiles from immediately
	// colliding with their owner
/*
Ghoul2 Insert Start
*/
// this marker thing of Jake's is used for memcpy() length calcs, so don't put any ordinary fields (like above)
//	below this point or they won't work, and will mess up all sorts of stuff.
//
	CGhoul2Info_v	ghoul2;
	/*
	Ghoul2 Insert End
	*/
	// the game dll can add anything it wants after
	// this point in the structure
};

#endif		// GAME_INCLUDE

//===============================================================

//
// functions provided by the main engine
//
/*
Ghoul2 Insert Start
*/
class CMiniHeap;
/*
Ghoul2 Insert End
*/
using game_import_t = struct
{
	//============== general Quake services ==================

	// print message on the local console
	void (*Printf)(const char* fmt, ...);

	// Write a camera ref_tag to cameras.map
	void (*WriteCam)(const char* text);
	void (*FlushCamFile)();

	// abort the game
	NORETURN_PTR void (*Error)(int, const char* fmt, ...);

	// get current time for profiling reasons
	// this should NOT be used for any game related tasks,
	// because it is not journaled
	int (*Milliseconds)();

	// console variable interaction
	cvar_t* (*cvar)(const char* var_name, const char* value, int flags);
	void (*cvar_set)(const char* var_name, const char* value);
	int (*Cvar_VariableIntegerValue)(const char* var_name);
	void (*Cvar_VariableStringBuffer)(const char* var_name, char* buffer, int bufsize);

	// ClientCommand and ServerCommand parameter access
	int (*argc)();
	char* (*argv)(int n);

	int (*FS_FOpenFile)(const char* qpath, fileHandle_t* file, fsMode_t mode);
	int (*FS_Read)(void* buffer, int len, fileHandle_t f);
	int (*FS_Write)(const void* buffer, int len, fileHandle_t f);
	void (*FS_FCloseFile)(fileHandle_t f);
	long (*FS_ReadFile)(const char* name, void** buf);
	void (*FS_FreeFile)(void* buf);
	int (*FS_GetFileList)(const char* path, const char* extension, char* listbuf, int bufsize);

	// Savegame handling
	//
	ojk::ISavedGame* saved_game;

	// add commands to the console as if they were typed in
	// for map changing, etc
	void (*SendConsoleCommand)(const char* text);

	//=========== server specific functionality =============

	// kick a client off the server with a message
	void (*DropClient)(int client_num, const char* reason);

	// reliably sends a command string to be interpreted by the given
	// client.  If client_num is -1, it will be sent to all clients
	void (*SendServerCommand)(int client_num, const char* fmt, ...);

	// config strings hold all the index strings, and various other information
	// that is reliably communicated to all clients
	// All of the current configstrings are sent to clients when
	// they connect, and changes are sent to all connected clients.
	// All confgstrings are cleared at each level start.
	void (*SetConfigstring)(int num, const char* string);
	void (*GetConfigstring)(int num, char* buffer, int buffer_size);

	// userinfo strings are maintained by the server system, so they
	// are persistant across level loads, while all other game visible
	// data is completely reset
	void (*GetUserinfo)(int num, char* buffer, int buffer_size);
	void (*SetUserinfo)(int num, const char* buffer);

	// the serverinfo info string has all the cvars visible to server browsers
	void (*GetServerinfo)(char* buffer, int buffer_size);

	// sets mins and maxs based on the brushmodel name
	void (*SetBrushModel)(gentity_t* ent, const char* name);

	// collision detection against all linked entities
	void (*trace)(trace_t* results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end,
		int pass_entity_num, int contentmask, EG2_Collision e_g2_trace_type, int use_lod);

	// point contents against all linked entities
	int (*pointcontents)(const vec3_t point, int pass_entity_num);
	// what contents are on the map?
	int (*totalMapContents)();

	qboolean(*inPVS)(const vec3_t p1, const vec3_t p2);
	qboolean(*inPVSIgnorePortals)(const vec3_t p1, const vec3_t p2);
	void (*AdjustAreaPortalState)(gentity_t* ent, qboolean open);
	qboolean(*AreasConnected)(int area1, int area2);

	// an entity will never be sent to a client or used for collision
	// if it is not passed to linkentity.  If the size, position, or
	// solidity changes, it must be relinked.
	void (*linkentity)(gentity_t* ent);
	void (*unlinkentity)(gentity_t* ent); // call before removing an interactive entity

	// EntitiesInBox will return brush models based on their bounding box,
	// so exact determination must still be done with EntityContact
	int (*EntitiesInBox)(const vec3_t mins, const vec3_t maxs, gentity_t** list, int maxcount);

	// perform an exact check against inline brush models of non-square shape
	qboolean(*EntityContact)(const vec3_t mins, const vec3_t maxs, const gentity_t* ent);

	// sound volume values
	int* VoiceVolume;

	// dynamic memory allocator for things that need to be freed
	void* (*Malloc)(int iSize, memtag_t eTag, qboolean bZeroIt); // see qcommon/tags.h for choices
	int (*Free)(void* buf);
	qboolean(*bIsFromZone)(const void* buf, memtag_t eTag); // see qcommon/tags.h for choices

	/*
	Ghoul2 Insert Start
	*/
	qhandle_t(*G2API_PrecacheGhoul2Model)(const char* fileName);

	int (*G2API_InitGhoul2Model)(CGhoul2Info_v& ghoul2, const char* fileName, int model_index, qhandle_t customSkin,
		qhandle_t customShader, int modelFlags, int lodBias);
	qboolean(*G2API_SetSkin)(CGhoul2Info* ghl_info, qhandle_t customSkin, qhandle_t renderSkin);
	qboolean(*G2API_SetBoneAnim)(CGhoul2Info* ghl_info, const char* bone_name, int start_frame, int end_frame,
		int flags, float anim_speed, int current_time, float setFrame, int blend_time);
	qboolean(*G2API_SetBoneAngles)(CGhoul2Info* ghl_info, const char* bone_name, const vec3_t angles,
		int flags, Eorientations up, Eorientations right, Eorientations forward,
		qhandle_t* model_list, int blend_time, int blendStart);
	qboolean(*G2API_SetBoneAnglesOffset)(CGhoul2Info* ghl_info, const char* bone_name, const vec3_t angles, int flags,
		Eorientations up, Eorientations left, Eorientations forward,
		qhandle_t* model_list,
		int blend_time, int acurrent_time, const vec3_t offset);
	qboolean(*G2API_SetBoneAnglesIndex)(CGhoul2Info* ghl_info, int index, const vec3_t angles, int flags,
		Eorientations yaw, Eorientations pitch, Eorientations roll,
		qhandle_t* model_list, int blend_time, int current_time);
	qboolean(*G2API_SetBoneAnglesMatrix)(CGhoul2Info* ghl_info, const char* bone_name, const mdxaBone_t& matrix,
		int flags,
		qhandle_t* model_list, int blend_time, int current_time);
	void (*G2API_CopyGhoul2Instance)(CGhoul2Info_v& ghoul2From, CGhoul2Info_v& ghoul2To, int model_index);
	qboolean(*G2API_SetBoneAnimIndex)(CGhoul2Info* ghl_info, int index, int start_frame, int end_frame, int flags,
		float anim_speed, int current_time, float setFrame, int blend_time);

	qboolean(*G2API_SetLodBias)(CGhoul2Info* ghl_info, int lodBias);
	qboolean(*G2API_SetShader)(CGhoul2Info* ghl_info, qhandle_t customShader);
	qboolean(*G2API_RemoveGhoul2Model)(CGhoul2Info_v& ghl_info, int model_index);
	qboolean(*G2API_SetSurfaceOnOff)(CGhoul2Info* ghl_info, const char* surface_name, int flags);
	qboolean(*G2API_SetRootSurface)(CGhoul2Info_v& ghl_info, int model_index, const char* surface_name);
	qboolean(*G2API_RemoveSurface)(CGhoul2Info* ghl_info, int index);
	int (*G2API_AddSurface)(CGhoul2Info* ghl_info, int surface_number, int polyNumber, float BarycentricI,
		float BarycentricJ, int lod);
	qboolean(*G2API_GetBoneAnim)(CGhoul2Info* ghl_info, const char* bone_name, int current_time, float* current_frame,
		int* start_frame, int* end_frame, int* flags, float* anim_speed, int* model_list);
	qboolean(*G2API_GetBoneAnimIndex)(CGhoul2Info* ghl_info, int i_bone_index, int current_time, float* current_frame,
		int* start_frame, int* end_frame, int* flags, float* anim_speed, int* model_list);
	qboolean(*G2API_GetAnimRange)(CGhoul2Info* ghl_info, const char* bone_name, int* start_frame, int* end_frame);
	qboolean(*G2API_GetAnimRangeIndex)(CGhoul2Info* ghl_info, int bone_index, int* start_frame, int* end_frame);

	qboolean(*G2API_PauseBoneAnim)(CGhoul2Info* ghl_info, const char* bone_name, int current_time);
	qboolean(*G2API_PauseBoneAnimIndex)(CGhoul2Info* ghl_info, int bone_index, int current_time);
	qboolean(*G2API_IsPaused)(CGhoul2Info* ghl_info, const char* bone_name);
	qboolean(*G2API_StopBoneAnim)(CGhoul2Info* ghl_info, const char* bone_name);
	qboolean(*G2API_StopBoneAngles)(CGhoul2Info* ghl_info, const char* bone_name);
	qboolean(*G2API_RemoveBone)(CGhoul2Info* ghl_info, const char* bone_name);
	qboolean(*G2API_RemoveBolt)(CGhoul2Info* ghl_info, int index);
	int (*G2API_AddBolt)(CGhoul2Info* ghl_info, const char* bone_name);
	int (*G2API_AddBoltSurfNum)(CGhoul2Info* ghl_info, int surf_index);
	qboolean(*G2API_AttachG2Model)(CGhoul2Info* ghl_info, CGhoul2Info* ghlInfoTo, int toBoltIndex, int toModel);
	qboolean(*G2API_DetachG2Model)(CGhoul2Info* ghl_info);
	qboolean(*G2API_AttachEnt)(int* bolt_info, CGhoul2Info* ghlInfoTo, int toBoltIndex, int ent_num, int toModelNum);
	void (*G2API_DetachEnt)(int* bolt_info);

	qboolean(*G2API_GetBoltMatrix)(CGhoul2Info_v& ghoul2, int model_index, int bolt_index, mdxaBone_t* matrix,
		const vec3_t angles, const vec3_t position, int frame_num, qhandle_t* model_list,
		const vec3_t scale);

	void (*G2API_ListSurfaces)(CGhoul2Info* ghl_info);
	void (*G2API_ListBones)(CGhoul2Info* ghl_info, int frame);
	qboolean(*G2API_HaveWeGhoul2Models)(CGhoul2Info_v& ghoul2);
	qboolean(*G2API_SetGhoul2ModelFlags)(CGhoul2Info* ghl_info, int flags);
	int (*G2API_GetGhoul2ModelFlags)(CGhoul2Info* ghl_info);

	qboolean(*G2API_GetAnimFileName)(CGhoul2Info* ghl_info, char** filename);
	void (*G2API_CollisionDetect)(CCollisionRecord* collRecMap, CGhoul2Info_v& ghoul2, const vec3_t angles,
		const vec3_t position,
		int frameNumber, int ent_num, vec3_t rayStart, vec3_t rayEnd, vec3_t scale,
		CMiniHeap* g2_vert_space,
		EG2_Collision e_g2_trace_type, int use_lod, float fRadius);
	void (*G2API_GiveMeVectorFromMatrix)(mdxaBone_t& bolt_matrix, Eorientations flags, vec3_t& vec);
	void (*G2API_CleanGhoul2Models)(CGhoul2Info_v& ghoul2);
	IGhoul2InfoArray& (*TheGhoul2InfoArray)();
	int (*G2API_GetParentSurface)(CGhoul2Info* ghl_info, int index);
	int (*G2API_GetSurfaceIndex)(CGhoul2Info* ghl_info, const char* surface_name);
	char* (*G2API_GetSurfaceName)(CGhoul2Info* ghl_info, int surf_number);
	char* (*G2API_GetGLAName)(CGhoul2Info* ghl_info);
	qboolean(*G2API_SetNewOrigin)(CGhoul2Info* ghl_info, int bolt_index);
	int (*G2API_GetBoneIndex)(CGhoul2Info* ghl_info, const char* bone_name, qboolean bAddIfNotFound);
	qboolean(*G2API_StopBoneAnglesIndex)(CGhoul2Info* ghl_info, int index);
	qboolean(*G2API_StopBoneAnimIndex)(CGhoul2Info* ghl_info, int index);
	qboolean(*G2API_SetBoneAnglesMatrixIndex)(CGhoul2Info* ghl_info, int index, const mdxaBone_t& matrix,
		int flags, qhandle_t* model_list, int blend_time, int current_time);
	qboolean(*G2API_SetAnimIndex)(CGhoul2Info* ghl_info, int index);
	int (*G2API_GetAnimIndex)(CGhoul2Info* ghl_info);
	void (*G2API_SaveGhoul2Models)(CGhoul2Info_v& ghoul2);
	void (*G2API_LoadGhoul2Models)(CGhoul2Info_v& ghoul2, char* buffer);
	void (*G2API_LoadSaveCodeDestructGhoul2Info)(CGhoul2Info_v& ghoul2);
	char* (*G2API_GetAnimFileNameIndex)(qhandle_t model_index);
	char* (*G2API_GetAnimFileInternalNameIndex)(qhandle_t model_index);
	int (*G2API_GetSurfaceRenderStatus)(CGhoul2Info* ghl_info, const char* surface_name);

	//rww - RAGDOLL_BEGIN
	void (*G2API_SetRagDoll)(CGhoul2Info_v& ghoul2, CRagDollParams* parms);
	void (*G2API_AnimateG2Models)(CGhoul2Info_v& ghoul2, int acurrent_time, CRagDollUpdateParams* params);

	qboolean(*G2API_RagPCJConstraint)(CGhoul2Info_v& ghoul2, const char* bone_name, vec3_t min, vec3_t max);
	qboolean(*G2API_RagPCJGradientSpeed)(CGhoul2Info_v& ghoul2, const char* bone_name, float speed);
	qboolean(*G2API_RagEffectorGoal)(CGhoul2Info_v& ghoul2, const char* bone_name, vec3_t pos);
	qboolean(*G2API_GetRagBonePos)(CGhoul2Info_v& ghoul2, const char* bone_name, vec3_t pos, vec3_t entAngles,
		vec3_t entPos, vec3_t entScale);
	qboolean(*G2API_RagEffectorKick)(CGhoul2Info_v& ghoul2, const char* bone_name, vec3_t velocity);
	qboolean(*G2API_RagForceSolve)(CGhoul2Info_v& ghoul2, qboolean force);

	qboolean(*G2API_SetBoneIKState)(CGhoul2Info_v& ghoul2, int time, const char* bone_name, int ikState,
		sharedSetBoneIKStateParams_t* params);
	qboolean(*G2API_IKMove)(CGhoul2Info_v& ghoul2, int time, sharedIKMoveParams_t* params);
	//rww - RAGDOLL_END

	void (*G2API_AddSkinGore)(CGhoul2Info_v& ghoul2, SSkinGoreData& gore);
	void (*G2API_ClearSkinGore)(CGhoul2Info_v& ghoul2);

	void (*RMG_Init)(int terrainID);

	int (*CM_RegisterTerrain)(const char* info);

	const char* (*SetActiveSubBSP)(int index);

	int (*RE_RegisterSkin)(const char* name);
	int (*RE_GetAnimationCFG)(const char* psCFGFilename, char* psDest, int iDestSize);

	bool (*WE_GetWindVector)(vec3_t windVector, vec3_t atpoint);
	bool (*WE_GetWindGusting)(vec3_t atpoint);
	bool (*WE_IsOutside)(vec3_t pos);
	float (*WE_IsOutsideCausingPain)(vec3_t pos);
	float (*WE_GetChanceOfSaberFizz)();
	bool (*WE_IsShaking)(vec3_t pos);
	void (*WE_AddWeatherZone)(vec3_t mins, vec3_t maxs);
	bool (*WE_SetTempGlobalFogColor)(vec3_t color);

	/*
	Ghoul2 Insert End
	*/
};

//
// functions exported by the game subsystem
//
using game_export_t = struct
{
	int apiversion;

	// init and shutdown will be called every single level
	// levelTime will be near zero, while globalTime will be a large number
	// that can be used to track spectator entry times across restarts
	void (*Init)(const char* mapname, const char* spawntarget, int checkSum, const char* entstring,
		int levelTime, int randomSeed, int globalTime, SavedGameJustLoaded_e e_saved_game_just_loaded,
		qboolean qbLoadTransition);
	void (*Shutdown)();

	// ReadLevel is called after the default map information has been
	// loaded with SpawnEntities
	void (*WriteLevel)(qboolean qbAutosave);
	void (*ReadLevel)(qboolean qbAutosave, qboolean qbLoadTransition);
	qboolean(*GameAllowedToSaveHere)();

	// return NULL if the client is allowed to connect, otherwise return
	// a text string with the reason for denial
	char* (*ClientConnect)(int client_num, qboolean firstTime, SavedGameJustLoaded_e e_saved_game_just_loaded);

	void (*ClientBegin)(int client_num, const usercmd_t* cmd, SavedGameJustLoaded_e e_saved_game_just_loaded);
	void (*ClientUserinfoChanged)(int client_num);
	void (*ClientDisconnect)(int client_num);
	void (*ClientCommand)(int client_num);
	void (*ClientThink)(int client_num, usercmd_t* cmd);

	void (*RunFrame)(int levelTime);
	void (*ConnectNavs)(const char* mapname, int checkSum);

	// ConsoleCommand will be called when a command has been issued
	// that is not recognized as a builtin function.
	// The game can issue gi.argc() / gi.argv() commands to get the command
	// and parameters.  Return qfalse if the game doesn't recognize it as a command.
	qboolean(*ConsoleCommand)();

	void (*GameSpawnRMGEntity)(const char* s);
	//
	// global variables shared between game and server
	//

	// The gentities array is allocated in the game dll so it
	// can vary in size from one game to another.
	//
	// The size will be fixed when ge->Init() is called
	// the server can't just use pointer arithmetic on gentities, because the
	// server's sizeof(struct gentity_s) doesn't equal gentitySize
	struct gentity_s* gentities;
	int gentitySize;
	int num_entities; // current number, <= MAX_GENTITIES
};

game_export_t* GetGameApi(game_import_t* import);

#endif//#ifndef __G_PUBLIC_H__
