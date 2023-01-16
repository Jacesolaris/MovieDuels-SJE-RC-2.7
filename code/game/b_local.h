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

//B_local.h
//re-added by MCG
#ifndef __B_LOCAL_H__
#define __B_LOCAL_H__

#include "g_local.h"
#include "say.h"

#include "ai.h"

#define	AI_TIMERS 0//turn on to see print-outs of AI/nav timing
//
// Navigation susbsystem
//

#define NAVF_DUCK			0x00000001
#define NAVF_JUMP			0x00000002
#define NAVF_HOLD			0x00000004
#define NAVF_SLOW			0x00000008

#define DEBUG_LEVEL_DETAIL	4
#define DEBUG_LEVEL_INFO	3
#define DEBUG_LEVEL_WARNING	2
#define DEBUG_LEVEL_ERROR	1
#define DEBUG_LEVEL_NONE	0

#define MAX_GOAL_REACHED_DIST_SQUARED	256//16 squared
#define MIN_ANGLE_ERROR 0.01f

#define MIN_ROCKET_DIST_SQUARED 16384//128*128
//
// NPC.cpp
//
// ai debug cvars
void SetNPCGlobals(gentity_t* ent);
void SaveNPCGlobals();
void RestoreNPCGlobals();
extern cvar_t* debugNPCAI; // used to print out debug info about the NPC AI
extern cvar_t* debugNPCFreeze; // set to disable NPC ai and temporarily freeze them in place
extern cvar_t* debugNPCName;
extern cvar_t* d_JediAI;
extern cvar_t* d_saberCombat;
extern cvar_t* d_combatinfo;
extern cvar_t* d_blockinfo;
extern cvar_t* d_attackinfo;
extern cvar_t* d_SaberactionInfo;
extern cvar_t* d_saberinfo;
extern cvar_t* g_DebugSaberCombat;
extern void NPC_Think(gentity_t* ent);
extern void pitch_roll_for_slope(gentity_t* forwhom, vec3_t pass_slope = nullptr, vec3_t store_angles = nullptr,
	qboolean keep_pitch = qfalse);

//NPC_reactions.cpp
extern void NPC_Pain(gentity_t* self, gentity_t* inflictor, gentity_t* attacker, vec3_t point, int damage, int mod,
	int hit_loc);
extern void NPC_Touch(gentity_t* self, gentity_t* other, trace_t* trace);
extern void NPC_Use(gentity_t* self, gentity_t* other, gentity_t* activator);
extern float NPC_GetPainChance(const gentity_t* self, int damage);

//
// NPC_misc.cpp
//
extern void Debug_Printf(const cvar_t* cv, int level, char* fmt, ...);
extern void Debug_NPCPrintf(const gentity_t* printNPC, const cvar_t* cv, int debugLevel, char* fmt, ...);

//MCG - Begin============================================================
//NPC_ai variables - shared by NPC.cpp andf the following modules
extern gentity_t* NPC;
extern gNPC_t* NPCInfo;
extern gclient_t* client;
extern usercmd_t ucmd;
extern visibility_t enemyVisibility;

//AI_Default
extern qboolean NPC_CheckInvestigate(int alert_event_num);
extern qboolean NPC_StandTrackAndShoot(gentity_t* NPC);
extern void NPC_BSIdle();
extern void NPC_BSPointShoot(qboolean shoot);
extern void NPC_BSStandGuard();
extern void NPC_BSPatrol();
extern void NPC_BSHuntAndKill();
extern void NPC_BSStandAndShoot();
extern void NPC_BSRunAndShoot();
extern void NPC_BSWait();
extern void NPC_BSDefault();

//NPC_behavior
extern void NPC_BSAdvanceFight();
extern void NPC_BSInvestigate();
extern void NPC_BSSleep();
extern void NPC_BSFollowLeader();
extern void NPC_BSJump();
extern void NPC_BSRemove();
extern void NPC_BSSearch();
extern void NPC_BSSearchStart(int home_wp, bState_t b_state);
extern void NPC_BSWander();
extern qboolean NPC_BSFlee();
extern void NPC_StartFlee(gentity_t* enemy, vec3_t danger_point, int danger_level, int flee_time_min, int flee_time_max);
extern void G_StartFlee(gentity_t* self, gentity_t* enemy, vec3_t danger_point, int danger_level, int flee_time_min,
	int flee_time_max);

//NPC_combat
extern int ChooseBestWeapon();
extern void NPC_ChangeWeapon(int new_weapon);
extern void WeaponThink();
extern qboolean HaveWeapon(int weapon);
extern qboolean CanShoot(const gentity_t* ent, const gentity_t* shooter);
extern void NPC_CheckPossibleEnemy(gentity_t* other, visibility_t vis);
extern gentity_t* NPC_PickEnemy(const gentity_t* closest_to, int enemy_team, qboolean check_vis, qboolean find_players_first,
	qboolean find_closest);
extern gentity_t* NPC_CheckEnemy(qboolean find_new, qboolean too_far_ok, qboolean set_enemy = qtrue);
extern qboolean NPC_CheckAttack(float scale);
extern qboolean NPC_CheckDefend(float scale);
extern qboolean NPC_CheckCanAttack(float attack_scale);
extern int NPC_AttackDebounceForWeapon();
extern qboolean EntIsGlass(const gentity_t* check);
extern qboolean ShotThroughGlass(trace_t* tr, const gentity_t* target, vec3_t spot, int mask);
extern void G_ClearEnemy(gentity_t* self);
extern void G_SetEnemy(gentity_t* self, gentity_t* enemy);
extern gentity_t* NPC_PickAlly(qboolean facing_each_other, float range, qboolean ignore_group, qboolean moving_only);
extern void NPC_LostEnemyDecideChase();
extern float NPC_MaxDistSquaredForWeapon();
extern qboolean NPC_EvaluateShot(int hit);
extern int NPC_ShotEntity(const gentity_t* ent, vec3_t impact_pos = nullptr);
extern void npc_check_speak(gentity_t* speaker_npc);

//NPC_formation
extern qboolean NPC_SlideMoveToGoal();
extern float NPC_FindClosestTeammate(gentity_t* self);
extern void NPC_CalcClosestFormationSpot(gentity_t* self);
//extern void G_MaintainFormations (gentity_t *self);
extern void NPC_BSFormation();
extern void NPC_CreateFormation(gentity_t* self);
extern void NPC_DropFormation(gentity_t* self);
extern void NPC_ReorderFormation(gentity_t* self);
extern void NPC_InsertIntoFormation(gentity_t* self);
extern void NPC_DeleteFromFormation(gentity_t* self);

#define COLLISION_RADIUS 32
#define NUM_POSITIONS 30

//NPC spawnflags
#define SFB_RIFLEMAN	2
#define SFB_PHASER		4

#define	SFB_CINEMATIC	32
#define	SFB_NOTSOLID	64
#define	SFB_STARTINSOLID 128

#define SFB_TROOPERAI	(1<<9)

//NPC_goal
extern void SetGoal(gentity_t* goal, float rating);
extern void NPC_SetGoal(gentity_t* goal, float rating);
extern void NPC_ClearGoal();
extern void NPC_ReachedGoal();
extern qboolean ReachedGoal(gentity_t* goal);
extern gentity_t* UpdateGoal();
extern qboolean NPC_MoveToGoal(qboolean tryStraight);

//NPC_move
qboolean NPC_Jumping();
qboolean NPC_JumpBackingUp();

qboolean NPC_TryJump(gentity_t* goal, float max_xy_dist = 0.0f, float max_z_diff = 0.0f);
qboolean NPC_TryJump(const vec3_t& pos, float max_xy_dist = 0.0f, float max_z_diff = 0.0f);

//NPC_reactions

//NPC_senses
#define	ALERT_CLEAR_TIME	200
#define CHECK_PVS		1
#define CHECK_360		2
#define CHECK_FOV		4
#define CHECK_SHOOT		8
#define CHECK_VISRANGE	16
extern qboolean CanSee(const gentity_t* ent);
extern qboolean InFOV(const gentity_t* ent, const gentity_t* from, int hFOV, int vFOV);
extern qboolean InFOV(const vec3_t origin, const gentity_t* from, int hFOV, int vFOV);
extern qboolean InFOV(const vec3_t spot, vec3_t from, vec3_t fromAngles, int hFOV, int vFOV);
extern visibility_t NPC_CheckVisibility(const gentity_t* ent, int flags);
extern qboolean InVisrange(const gentity_t* ent);

//NPC_spawn
extern void NPC_Spawn(gentity_t* self);

//NPC_stats
extern int NPC_ReactionTime();
extern qboolean NPC_ParseParms(const char* npc_name, gentity_t* npc);
extern void NPC_LoadParms();

//NPC_utils
extern int teamNumbers[TEAM_NUM_TEAMS];
extern int teamStrength[TEAM_NUM_TEAMS];
extern int teamCounter[TEAM_NUM_TEAMS];
extern void CalcEntitySpot(const gentity_t* ent, spot_t spot, vec3_t point);
extern qboolean NPC_UpdateAngles(qboolean doPitch, qboolean doYaw);
extern void NPC_UpdateShootAngles(vec3_t angles, qboolean doPitch, qboolean doYaw);
extern qboolean NPC_UpdateFiringAngles(qboolean doPitch, qboolean doYaw);
extern void SetTeamNumbers();
extern qboolean G_ActivateBehavior(gentity_t* self, int bset);
extern void NPC_AimWiggle(vec3_t enemy_org);
extern void NPC_SetLookTarget(const gentity_t* self, int ent_num, int clearTime);

//other modules
extern void CalcMuzzlePoint(gentity_t* ent, vec3_t forward_vec, vec3_t muzzle_point, float lead_in);

//g_combat
extern void ExplodeDeath(gentity_t* self);
extern void ExplodeDeath_Wait(gentity_t* self, gentity_t* inflictor, gentity_t* attacker, int damage, int meansOfDeath,
	int d_flags, int hit_loc);
extern void GoExplodeDeath(gentity_t* self, gentity_t* other, gentity_t* activator);
extern float IdealDistance();

//g_client
extern qboolean SpotWouldTelefrag(const gentity_t* spot, team_t checkteam);

//g_squad
extern void NPC_SetSayState(gentity_t* self, gentity_t* to, int saying);

//g_utils
extern qboolean G_CheckInSolid(gentity_t* self, qboolean fix);
extern qboolean infront(const gentity_t* from, const gentity_t* to);

//MCG - End============================================================

// NPC.cpp
extern void NPC_SetAnim(gentity_t* ent, int set_anim_parts, int anim, int set_anim_flags,
	int i_blend = SETANIM_BLEND_DEFAULT);
extern qboolean NPC_EnemyTooFar(const gentity_t* enemy, float dist, qboolean to_shoot);

// ==================================================================

inline qboolean NPC_ClearLOS(const vec3_t start, const vec3_t end)
{
	return G_ClearLOS(NPC, start, end);
}

inline qboolean NPC_ClearLOS(const vec3_t end)
{
	return G_ClearLOS(NPC, end);
}

inline qboolean NPC_ClearLOS(const gentity_t* ent)
{
	return G_ClearLOS(NPC, ent);
}

inline qboolean NPC_ClearLOS(const vec3_t start, const gentity_t* ent)
{
	return G_ClearLOS(NPC, start, ent);
}

inline qboolean NPC_ClearLOS(const gentity_t* ent, const vec3_t end)
{
	return G_ClearLOS(NPC, ent, end);
}

extern qboolean NPC_ClearShot(const gentity_t* ent);

extern int NPC_FindCombatPoint(const vec3_t position, const vec3_t avoid_position, vec3_t dest_position, int flags,
	float avoid_dist, int ignore_point = -1);
extern int NPC_FindCombatPointRetry(const vec3_t position,
	const vec3_t avoid_position,
	vec3_t dest_position,
	int* cp_flags,
	float avoid_dist,
	int ignore_point);

extern qboolean NPC_ReserveCombatPoint(int combat_point_id);
extern qboolean NPC_FreeCombatPoint(int combat_point_id, qboolean failed = qfalse);
extern qboolean NPC_SetCombatPoint(int combat_point_id);

#define	CP_ANY			0			//No flags
#define	CP_COVER		0x00000001	//The enemy cannot currently shoot this position
#define CP_CLEAR		0x00000002	//This cover point has a clear shot to the enemy
#define CP_FLEE			0x00000004	//This cover point is marked as a flee point
#define CP_DUCK			0x00000008	//This cover point is marked as a duck point
#define CP_NEAREST		0x00000010	//Find the nearest combat point
#define CP_AVOID_ENEMY	0x00000020	//Avoid our enemy
#define CP_INVESTIGATE	0x00000040	//A special point worth enemy investigation if searching
#define	CP_SQUAD		0x00000080	//Squad path
#define	CP_AVOID		0x00000100	//Avoid supplied position
#define	CP_APPROACH_ENEMY 0x00000200	//Try to get closer to enemy
#define	CP_CLOSEST		0x00000400	//Take the closest combatPoint to the enemy that's available
#define	CP_FLANK		0x00000800	//Pick a combatPoint behind the enemy
#define	CP_HAS_ROUTE	0x00001000	//Pick a combatPoint that we have a route to
#define	CP_SNIPE		0x00002000	//Pick a combatPoint that is marked as a sniper spot
#define	CP_SAFE			0x00004000	//Pick a combatPoint that is not have dangerTime
#define	CP_HORZ_DIST_COLL 0x00008000	//Collect combat points within *horizontal* dist
#define	CP_NO_PVS		0x00010000	//A combat point out of the PVS of enemy pos
#define	CP_RETREAT	 	0x00020000	//Try to get farther from enemy
#define CP_SHORTEST_PATH 0x0004000	//Shortest path from me to combat point to enemy
#define CP_TRYFAR		0x00080000

#define CPF_NONE		0
#define	CPF_DUCK		0x00000001
#define	CPF_FLEE		0x00000002
#define	CPF_INVESTIGATE	0x00000004
#define	CPF_SQUAD		0x00000008
#define	CPF_LEAN		0x00000010
#define	CPF_SNIPE		0x00000020

#define	MAX_COMBAT_POINT_CHECK	32

extern qboolean NPC_ValidEnemy(const gentity_t* ent);
extern qboolean NPC_CheckEnemyExt(qboolean checkAlerts = qfalse);
extern qboolean NPC_FindPlayer();
extern qboolean NPC_CheckCanAttackExt();

extern int NPC_CheckAlertEvents(qboolean checkSight, qboolean checkSound, int ignoreAlert = -1,
	qboolean mustHaveOwner = qfalse, int minAlertLevel = AEL_MINOR,
	qboolean onGroundOnly = qfalse);
extern qboolean NPC_CheckForDanger(int alert_event);
extern void G_AlertTeam(const gentity_t* victim, gentity_t* attacker, float radius, float sound_dist);

extern int NPC_FindSquadPoint(vec3_t position);

extern void ClearPlayerAlertEvents();

extern qboolean G_BoundsOverlap(const vec3_t mins1, const vec3_t maxs1, const vec3_t mins2, const vec3_t maxs2);

extern void NPC_SetMoveGoal(const gentity_t* ent, vec3_t point, int radius, qboolean isNavGoal = qfalse,
	int combatPoint = -1,
	gentity_t* targetEnt = nullptr);

extern void NPC_ApplyWeaponFireDelay();

//NPC_FaceXXX suite
extern qboolean NPC_FacePosition(vec3_t position, qboolean doPitch = qtrue);
extern qboolean NPC_FaceEntity(const gentity_t* ent, qboolean doPitch = qtrue);
extern qboolean NPC_FaceEnemy(qboolean doPitch = qtrue);

//Skill level cvar
extern cvar_t* g_spskill;

#define	NIF_NONE		0x00000000
#define	NIF_FAILED		0x00000001	//failed to find a way to the goal
#define	NIF_MACRO_NAV	0x00000002	//using macro navigation
#define	NIF_COLLISION	0x00000004	//resolving collision with an entity
#define NIF_BLOCKED		0x00000008	//blocked from moving

/*
-------------------------
struct navInfo_s
-------------------------
*/

using navInfo_t = struct navInfo_s
{
	gentity_t* blocker;
	vec3_t direction;
	vec3_t pathDirection;
	float distance;
	trace_t trace;
	int flags;
};

extern void NAV_GetLastMove(navInfo_t& info);

#endif
