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

#include "g_local.h"
#include "b_local.h"
#include "g_functions.h"
#include "wp_saber.h"
#include "w_local.h"
//---------------
//	Blaster
//---------------

extern cvar_t* g_SerenityJediEngineMode;
extern qboolean G_ControlledByPlayer(const gentity_t* self);
//---------------------------------------------------------
void WP_FireBlasterMissile(gentity_t* ent, vec3_t start, vec3_t dir, const qboolean altFire)
//---------------------------------------------------------
{
	int velocity = BLASTER_VELOCITY;
	int damage = altFire ? weaponData[WP_BLASTER].altDamage : weaponData[WP_BLASTER].damage;

	if (ent && ent->client && ent->client->NPC_class == CLASS_VEHICLE)
	{
		damage *= 3;
		velocity = ATST_MAIN_VEL + ent->client->ps.speed;
	}
	else
	{
		// If an enemy is shooting at us, lower the velocity so you have a chance to evade
		if (ent->client && ent->client->ps.client_num != 0
			&& ent->client->NPC_class != CLASS_BOBAFETT
			&& ent->client->NPC_class != CLASS_MANDALORIAN
			&& ent->client->NPC_class != CLASS_JANGO
			&& ent->client->NPC_class != CLASS_JANGODUAL)
		{
			if (g_spskill->integer < 2)
			{
				velocity *= BLASTER_NPC_VEL_CUT;
			}
			else
			{
				velocity *= BLASTER_NPC_HARD_VEL_CUT;
			}
		}
	}

	WP_TraceSetStart(ent, start);
	//make sure our start point isn't on the other side of a wall

	WP_MissileTargetHint(ent, start, dir);

	gentity_t* missile = CreateMissile(start, dir, velocity, 10000, ent, altFire);

	missile->classname = "blaster_proj";
	missile->s.weapon = WP_BLASTER;

	// Do the damages
	if (ent->s.number != 0
		&& ent->client->NPC_class != CLASS_BOBAFETT
		&& ent->client->NPC_class != CLASS_MANDALORIAN
		&& ent->client->NPC_class != CLASS_JANGO
		&& ent->client->NPC_class != CLASS_JANGODUAL)
	{
		if (g_spskill->integer == 0)
		{
			damage = BLASTER_NPC_DAMAGE_EASY;
		}
		else if (g_spskill->integer == 1)
		{
			damage = BLASTER_NPC_DAMAGE_NORMAL;
		}
		else
		{
			damage = BLASTER_NPC_DAMAGE_HARD;
		}
	}

	missile->damage = damage;
	if (g_SerenityJediEngineMode->integer == 2)
	{
		missile->dflags = DAMAGE_DEATH_KNOCKBACK | DAMAGE_EXTRA_KNOCKBACK;
	}
	else
	{
		missile->dflags = DAMAGE_DEATH_KNOCKBACK;
	}
	if (altFire)
	{
		missile->methodOfDeath = MOD_BLASTER_ALT;
	}
	else
	{
		missile->methodOfDeath = MOD_BLASTER;
	}
	missile->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;

	// we don't want it to bounce forever
	missile->bounceCount = 8;
}

extern qboolean WalkCheck(const gentity_t* self);
extern qboolean PM_CrouchAnim(int anim);

//---------------------------------------------------------
void WP_FireBlaster(gentity_t* ent, const qboolean alt_fire)
//---------------------------------------------------------
{
	vec3_t dir, angs;

	vectoangles(forwardVec, angs);

	if (ent->client && ent->client->NPC_class == CLASS_VEHICLE)
	{
		//no inherent aim screw up
	}
	else if (ent->client && !(ent->client->ps.forcePowersActive & 1 << FP_SEE) || ent->client->ps.forcePowerLevel[
		FP_SEE] < FORCE_LEVEL_2)
	{
		//force sight 2+ gives perfect aim
		if (alt_fire)
		{
			// add some slop to the alt-fire direction
			if (!WalkCheck(ent) && (ent->s.number < MAX_CLIENTS || G_ControlledByPlayer(ent))) //if running aim is shit
			{
				angs[PITCH] += Q_flrand(-1.0f, 1.0f) * RUNNING_SPREAD;
				angs[YAW] += Q_flrand(-1.0f, 1.0f) * RUNNING_SPREAD;
			}
			else if (ent->client->ps.BlasterAttackChainCount >= BLASTERMISHAPLEVEL_FULL)
			{
				// add some slop to the fire direction
				angs[PITCH] += Q_flrand(-3.0f, 3.0f) * BLASTER_ALT_SPREAD;
				angs[YAW] += Q_flrand(-3.0f, 3.0f) * BLASTER_ALT_SPREAD;
			}
			else if (ent->client->ps.BlasterAttackChainCount >= BLASTERMISHAPLEVEL_HEAVY)
			{
				// add some slop to the fire direction
				angs[PITCH] += Q_flrand(-2.0f, 2.0f) * BLASTER_ALT_SPREAD;
				angs[YAW] += Q_flrand(-2.0f, 2.0f) * BLASTER_ALT_SPREAD;
			}
			else if (PM_CrouchAnim(ent->client->ps.legsAnim))
			{
				//
			}
			else
			{
				angs[PITCH] += Q_flrand(-0.5f, 0.5f) * BLASTER_ALT_SPREAD;
				angs[YAW] += Q_flrand(-0.5f, 0.5f) * BLASTER_ALT_SPREAD;
			}
		}
		else
		{
			if (ent->NPC && ent->NPC->currentAim < 5)
			{
				if (ent->client && ent->NPC &&
					(ent->client->NPC_class == CLASS_STORMTROOPER ||
						ent->client->NPC_class == CLASS_CLONETROOPER ||
						ent->client->NPC_class == CLASS_STORMCOMMANDO ||
						ent->client->NPC_class == CLASS_SWAMPTROOPER ||
						ent->client->NPC_class == CLASS_DROIDEKA ||
						ent->client->NPC_class == CLASS_SBD ||
						ent->client->NPC_class == CLASS_IMPWORKER ||
						ent->client->NPC_class == CLASS_REBEL ||
						ent->client->NPC_class == CLASS_WOOKIE ||
						ent->client->NPC_class == CLASS_BATTLEDROID))
				{
					angs[PITCH] += Q_flrand(-1.0f, 1.0f) * (BLASTER_NPC_SPREAD + (1 - ent->NPC->currentAim) * 0.25f);
					//was 0.5f
					angs[YAW] += Q_flrand(-1.0f, 1.0f) * (BLASTER_NPC_SPREAD + (1 - ent->NPC->currentAim) * 0.25f);
					//was 0.5
				}
			}
			else if (!WalkCheck(ent) && (ent->s.number < MAX_CLIENTS || G_ControlledByPlayer(ent)))
			//if running aim is shit
			{
				angs[PITCH] += Q_flrand(-1.0f, 1.0f) * RUNNING_SPREAD;
				angs[YAW] += Q_flrand(-1.0f, 1.0f) * RUNNING_SPREAD;
			}
			else if (ent->client->ps.BlasterAttackChainCount >= BLASTERMISHAPLEVEL_FULL)
			{
				// add some slop to the fire direction
				angs[PITCH] += Q_flrand(-3.0f, 3.0f) * BLASTER_MAIN_SPREAD;
				angs[YAW] += Q_flrand(-3.0f, 3.0f) * BLASTER_MAIN_SPREAD;
			}
			else if (ent->client->ps.BlasterAttackChainCount >= BLASTERMISHAPLEVEL_HEAVY)
			{
				// add some slop to the fire direction
				angs[PITCH] += Q_flrand(-2.0f, 2.0f) * BLASTER_MAIN_SPREAD;
				angs[YAW] += Q_flrand(-2.0f, 2.0f) * BLASTER_MAIN_SPREAD;
			}
			else if (PM_CrouchAnim(ent->client->ps.legsAnim))
			{
				//
			}
			else
			{
				angs[PITCH] += Q_flrand(-0.5f, 0.5f) * BLASTER_MAIN_SPREAD;
				angs[YAW] += Q_flrand(-0.5f, 0.5f) * BLASTER_MAIN_SPREAD;
			}
		}
	}

	AngleVectors(angs, dir, nullptr, nullptr);

	// FIXME: if temp_org does not have clear trace to inside the bbox, don't shoot!
	WP_FireBlasterMissile(ent, muzzle, dir, alt_fire);
}

//---------------
//	E-5 Carbine
//---------------

//---------------------------------------------------------
void WP_FireBattleDroidMissile(gentity_t* ent, vec3_t start, vec3_t dir, const qboolean altFire)
//---------------------------------------------------------
{
	int velocity = BLASTER_VELOCITY;
	int damage = altFire ? weaponData[WP_BATTLEDROID].altDamage : weaponData[WP_BATTLEDROID].damage;

	if (ent && ent->client && ent->client->NPC_class == CLASS_VEHICLE)
	{
		damage *= 3;
		velocity = ATST_MAIN_VEL + ent->client->ps.speed;
	}
	else
	{
		// If an enemy is shooting at us, lower the velocity so you have a chance to evade
		if (ent->client && ent->client->ps.client_num != 0
			&& ent->client->NPC_class != CLASS_BOBAFETT
			&& ent->client->NPC_class != CLASS_MANDALORIAN
			&& ent->client->NPC_class != CLASS_JANGO
			&& ent->client->NPC_class != CLASS_JANGODUAL)
		{
			if (g_spskill->integer < 2)
			{
				velocity *= BLASTER_NPC_VEL_CUT;
			}
			else
			{
				velocity *= BLASTER_NPC_HARD_VEL_CUT;
			}
		}
	}

	WP_TraceSetStart(ent, start);
	//make sure our start point isn't on the other side of a wall

	WP_MissileTargetHint(ent, start, dir);

	gentity_t* missile = CreateMissile(start, dir, velocity, 10000, ent, altFire);

	missile->classname = "blaster_proj";
	missile->s.weapon = WP_BATTLEDROID;

	// Do the damages
	if (ent->s.number != 0
		&& ent->client->NPC_class != CLASS_BOBAFETT
		&& ent->client->NPC_class != CLASS_MANDALORIAN
		&& ent->client->NPC_class != CLASS_JANGO
		&& ent->client->NPC_class != CLASS_JANGODUAL)
	{
		if (g_spskill->integer == 0)
		{
			damage = BLASTER_NPC_DAMAGE_EASY;
		}
		else if (g_spskill->integer == 1)
		{
			damage = BLASTER_NPC_DAMAGE_NORMAL;
		}
		else
		{
			damage = BLASTER_NPC_DAMAGE_HARD;
		}
	}

	//	if ( ent->client )
	//	{
	//		if ( ent->client->ps.powerups[PW_WEAPON_OVERCHARGE] > 0 && ent->client->ps.powerups[PW_WEAPON_OVERCHARGE] > cg.time )
	//		{
	//			// in overcharge mode, so doing double damage
	//			missile->flags |= FL_OVERCHARGED;
	//			damage *= 2;
	//		}
	//	}

	missile->damage = damage;
	if (g_SerenityJediEngineMode->integer == 2)
	{
		missile->dflags = DAMAGE_DEATH_KNOCKBACK | DAMAGE_EXTRA_KNOCKBACK;
	}
	else
	{
		missile->dflags = DAMAGE_DEATH_KNOCKBACK;
	}
	if (altFire)
	{
		missile->methodOfDeath = MOD_BLASTER_ALT;
	}
	else
	{
		missile->methodOfDeath = MOD_BLASTER;
	}
	missile->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;

	// we don't want it to bounce forever
	missile->bounceCount = 8;
}

extern qboolean WalkCheck(const gentity_t* self);
extern qboolean PM_CrouchAnim(int anim);

//---------------------------------------------------------
void WP_FireBattleDroid(gentity_t* ent, const qboolean alt_fire)
//---------------------------------------------------------
{
	vec3_t dir, angs;

	vectoangles(forwardVec, angs);

	if (ent->client && ent->client->NPC_class == CLASS_VEHICLE)
	{
		//no inherent aim screw up
	}
	else if (ent->client && !(ent->client->ps.forcePowersActive & 1 << FP_SEE) || ent->client->ps.forcePowerLevel[
		FP_SEE] < FORCE_LEVEL_2)
	{
		//force sight 2+ gives perfect aim
		if (alt_fire)
		{
			// add some slop to the alt-fire direction
			if (!WalkCheck(ent) && (ent->s.number < MAX_CLIENTS || G_ControlledByPlayer(ent))) //if running aim is shit
			{
				angs[PITCH] += Q_flrand(-1.0f, 1.0f) * RUNNING_SPREAD;
				angs[YAW] += Q_flrand(-1.0f, 1.0f) * RUNNING_SPREAD;
			}
			else if (ent->client->ps.BlasterAttackChainCount >= BLASTERMISHAPLEVEL_FULL)
			{
				// add some slop to the fire direction
				angs[PITCH] += Q_flrand(-3.0f, 3.0f) * BLASTER_ALT_SPREAD;
				angs[YAW] += Q_flrand(-3.0f, 3.0f) * BLASTER_ALT_SPREAD;
			}
			else if (ent->client->ps.BlasterAttackChainCount >= BLASTERMISHAPLEVEL_HEAVY)
			{
				// add some slop to the fire direction
				angs[PITCH] += Q_flrand(-2.0f, 2.0f) * BLASTER_ALT_SPREAD;
				angs[YAW] += Q_flrand(-2.0f, 2.0f) * BLASTER_ALT_SPREAD;
			}
			else if (PM_CrouchAnim(ent->client->ps.legsAnim))
			{
				//
			}
			else
			{
				angs[PITCH] += Q_flrand(-0.5f, 0.5f) * BLASTER_ALT_SPREAD;
				angs[YAW] += Q_flrand(-0.5f, 0.5f) * BLASTER_ALT_SPREAD;
			}
		}
		else
		{
			if (ent->NPC && ent->NPC->currentAim < 5)
			{
				if (ent->client && ent->NPC &&
					(ent->client->NPC_class == CLASS_STORMTROOPER ||
						ent->client->NPC_class == CLASS_CLONETROOPER ||
						ent->client->NPC_class == CLASS_STORMCOMMANDO ||
						ent->client->NPC_class == CLASS_SWAMPTROOPER ||
						ent->client->NPC_class == CLASS_DROIDEKA ||
						ent->client->NPC_class == CLASS_SBD ||
						ent->client->NPC_class == CLASS_IMPWORKER ||
						ent->client->NPC_class == CLASS_REBEL ||
						ent->client->NPC_class == CLASS_WOOKIE ||
						ent->client->NPC_class == CLASS_BATTLEDROID))
				{
					angs[PITCH] += Q_flrand(-1.0f, 1.0f) * (BLASTER_NPC_SPREAD + (1 - ent->NPC->currentAim) * 0.25f);
					//was 0.5f
					angs[YAW] += Q_flrand(-1.0f, 1.0f) * (BLASTER_NPC_SPREAD + (1 - ent->NPC->currentAim) * 0.25f);
					//was 0.5
				}
			}
			else if (!WalkCheck(ent) && (ent->s.number < MAX_CLIENTS || G_ControlledByPlayer(ent)))
			//if running aim is shit
			{
				angs[PITCH] += Q_flrand(-1.0f, 1.0f) * RUNNING_SPREAD;
				angs[YAW] += Q_flrand(-1.0f, 1.0f) * RUNNING_SPREAD;
			}
			else if (ent->client->ps.BlasterAttackChainCount >= BLASTERMISHAPLEVEL_FULL)
			{
				// add some slop to the fire direction
				angs[PITCH] += Q_flrand(-3.0f, 3.0f) * BLASTER_MAIN_SPREAD;
				angs[YAW] += Q_flrand(-3.0f, 3.0f) * BLASTER_MAIN_SPREAD;
			}
			else if (ent->client->ps.BlasterAttackChainCount >= BLASTERMISHAPLEVEL_HEAVY)
			{
				// add some slop to the fire direction
				angs[PITCH] += Q_flrand(-2.0f, 2.0f) * BLASTER_MAIN_SPREAD;
				angs[YAW] += Q_flrand(-2.0f, 2.0f) * BLASTER_MAIN_SPREAD;
			}
			else if (PM_CrouchAnim(ent->client->ps.legsAnim))
			{
				//
			}
			else
			{
				angs[PITCH] += Q_flrand(-0.5f, 0.5f) * BLASTER_MAIN_SPREAD;
				angs[YAW] += Q_flrand(-0.5f, 0.5f) * BLASTER_MAIN_SPREAD;
			}
		}
	}

	AngleVectors(angs, dir, nullptr, nullptr);

	// FIXME: if temp_org does not have clear trace to inside the bbox, don't shoot!
	WP_FireBattleDroidMissile(ent, muzzle, dir, alt_fire);
}

//---------------
//	F-11D
//---------------

//---------------------------------------------------------
void WP_FireFirstOrderMissile(gentity_t* ent, vec3_t start, vec3_t dir, const qboolean altFire)
//---------------------------------------------------------
{
	int velocity = BLASTER_VELOCITY;
	int damage = altFire ? weaponData[WP_THEFIRSTORDER].altDamage : weaponData[WP_THEFIRSTORDER].damage;

	if (ent && ent->client && ent->client->NPC_class == CLASS_VEHICLE)
	{
		damage *= 3;
		velocity = ATST_MAIN_VEL + ent->client->ps.speed;
	}
	else
	{
		// If an enemy is shooting at us, lower the velocity so you have a chance to evade
		if (ent->client && ent->client->ps.client_num != 0
			&& ent->client->NPC_class != CLASS_BOBAFETT
			&& ent->client->NPC_class != CLASS_MANDALORIAN
			&& ent->client->NPC_class != CLASS_JANGO
			&& ent->client->NPC_class != CLASS_JANGODUAL)
		{
			if (g_spskill->integer < 2)
			{
				velocity *= BLASTER_NPC_VEL_CUT;
			}
			else
			{
				velocity *= BLASTER_NPC_HARD_VEL_CUT;
			}
		}
	}

	WP_TraceSetStart(ent, start);
	//make sure our start point isn't on the other side of a wall

	WP_MissileTargetHint(ent, start, dir);

	gentity_t* missile = CreateMissile(start, dir, velocity, 10000, ent, altFire);

	missile->classname = "blaster_proj";
	missile->s.weapon = WP_THEFIRSTORDER;

	// Do the damages
	if (ent->s.number != 0
		&& ent->client->NPC_class != CLASS_BOBAFETT
		&& ent->client->NPC_class != CLASS_MANDALORIAN
		&& ent->client->NPC_class != CLASS_JANGO
		&& ent->client->NPC_class != CLASS_JANGODUAL)
	{
		if (g_spskill->integer == 0)
		{
			damage = BLASTER_NPC_DAMAGE_EASY;
		}
		else if (g_spskill->integer == 1)
		{
			damage = BLASTER_NPC_DAMAGE_NORMAL;
		}
		else
		{
			damage = BLASTER_NPC_DAMAGE_HARD;
		}
	}

	//	if ( ent->client )
	//	{
	//		if ( ent->client->ps.powerups[PW_WEAPON_OVERCHARGE] > 0 && ent->client->ps.powerups[PW_WEAPON_OVERCHARGE] > cg.time )
	//		{
	//			// in overcharge mode, so doing double damage
	//			missile->flags |= FL_OVERCHARGED;
	//			damage *= 2;
	//		}
	//	}

	missile->damage = damage;
	if (g_SerenityJediEngineMode->integer == 2)
	{
		missile->dflags = DAMAGE_DEATH_KNOCKBACK | DAMAGE_EXTRA_KNOCKBACK;
	}
	else
	{
		missile->dflags = DAMAGE_DEATH_KNOCKBACK;
	}
	if (altFire)
	{
		missile->methodOfDeath = MOD_BLASTER_ALT;
	}
	else
	{
		missile->methodOfDeath = MOD_BLASTER;
	}
	missile->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;

	// we don't want it to bounce forever
	missile->bounceCount = 8;
}

//---------------------------------------------------------
void WP_FireFirstOrder(gentity_t* ent, const qboolean alt_fire)
//---------------------------------------------------------
{
	vec3_t dir, angs;

	vectoangles(forwardVec, angs);

	if (ent->client && ent->client->NPC_class == CLASS_VEHICLE)
	{
		//no inherent aim screw up
	}
	else if (ent->client && !(ent->client->ps.forcePowersActive & 1 << FP_SEE) || ent->client->ps.forcePowerLevel[
		FP_SEE] < FORCE_LEVEL_2)
	{
		//force sight 2+ gives perfect aim
		if (alt_fire)
		{
			// add some slop to the alt-fire direction
			if (!WalkCheck(ent) && (ent->s.number < MAX_CLIENTS || G_ControlledByPlayer(ent))) //if running aim is shit
			{
				angs[PITCH] += Q_flrand(-1.0f, 1.0f) * RUNNING_SPREAD;
				angs[YAW] += Q_flrand(-1.0f, 1.0f) * RUNNING_SPREAD;
			}
			else if (ent->client->ps.BlasterAttackChainCount >= BLASTERMISHAPLEVEL_FULL)
			{
				// add some slop to the fire direction
				angs[PITCH] += Q_flrand(-3.0f, 3.0f) * BLASTER_ALT_SPREAD;
				angs[YAW] += Q_flrand(-3.0f, 3.0f) * BLASTER_ALT_SPREAD;
			}
			else if (ent->client->ps.BlasterAttackChainCount >= BLASTERMISHAPLEVEL_HEAVY)
			{
				// add some slop to the fire direction
				angs[PITCH] += Q_flrand(-2.0f, 2.0f) * BLASTER_ALT_SPREAD;
				angs[YAW] += Q_flrand(-2.0f, 2.0f) * BLASTER_ALT_SPREAD;
			}
			else if (PM_CrouchAnim(ent->client->ps.legsAnim))
			{
				//
			}
			else
			{
				angs[PITCH] += Q_flrand(-0.5f, 0.5f) * BLASTER_ALT_SPREAD;
				angs[YAW] += Q_flrand(-0.5f, 0.5f) * BLASTER_ALT_SPREAD;
			}
		}
		else
		{
			if (ent->NPC && ent->NPC->currentAim < 5)
			{
				if (ent->client && ent->NPC &&
					(ent->client->NPC_class == CLASS_STORMTROOPER ||
						ent->client->NPC_class == CLASS_CLONETROOPER ||
						ent->client->NPC_class == CLASS_STORMCOMMANDO ||
						ent->client->NPC_class == CLASS_SWAMPTROOPER ||
						ent->client->NPC_class == CLASS_DROIDEKA ||
						ent->client->NPC_class == CLASS_SBD ||
						ent->client->NPC_class == CLASS_IMPWORKER ||
						ent->client->NPC_class == CLASS_REBEL ||
						ent->client->NPC_class == CLASS_WOOKIE ||
						ent->client->NPC_class == CLASS_BATTLEDROID))
				{
					angs[PITCH] += Q_flrand(-1.0f, 1.0f) * (BLASTER_NPC_SPREAD + (1 - ent->NPC->currentAim) * 0.25f);
					//was 0.5f
					angs[YAW] += Q_flrand(-1.0f, 1.0f) * (BLASTER_NPC_SPREAD + (1 - ent->NPC->currentAim) * 0.25f);
					//was 0.5
				}
			}
			else if (!WalkCheck(ent) && (ent->s.number < MAX_CLIENTS || G_ControlledByPlayer(ent)))
			//if running aim is shit
			{
				angs[PITCH] += Q_flrand(-1.0f, 1.0f) * RUNNING_SPREAD;
				angs[YAW] += Q_flrand(-1.0f, 1.0f) * RUNNING_SPREAD;
			}
			else if (ent->client->ps.BlasterAttackChainCount >= BLASTERMISHAPLEVEL_FULL)
			{
				// add some slop to the fire direction
				angs[PITCH] += Q_flrand(-3.0f, 3.0f) * BLASTER_MAIN_SPREAD;
				angs[YAW] += Q_flrand(-3.0f, 3.0f) * BLASTER_MAIN_SPREAD;
			}
			else if (ent->client->ps.BlasterAttackChainCount >= BLASTERMISHAPLEVEL_HEAVY)
			{
				// add some slop to the fire direction
				angs[PITCH] += Q_flrand(-2.0f, 2.0f) * BLASTER_MAIN_SPREAD;
				angs[YAW] += Q_flrand(-2.0f, 2.0f) * BLASTER_MAIN_SPREAD;
			}
			else if (PM_CrouchAnim(ent->client->ps.legsAnim))
			{
				//
			}
			else
			{
				angs[PITCH] += Q_flrand(-0.5f, 0.5f) * BLASTER_MAIN_SPREAD;
				angs[YAW] += Q_flrand(-0.5f, 0.5f) * BLASTER_MAIN_SPREAD;
			}
		}
	}

	AngleVectors(angs, dir, nullptr, nullptr);

	// FIXME: if temp_org does not have clear trace to inside the bbox, don't shoot!
	WP_FireFirstOrderMissile(ent, muzzle, dir, alt_fire);
}

//---------------
//	DH-17
//---------------

//---------------------------------------------------------
void WP_FireRebelBlasterMissile(gentity_t* ent, vec3_t start, vec3_t dir, const qboolean altFire)
//---------------------------------------------------------
{
	int velocity = REBELBLASTER_VELOCITY;
	int damage = altFire ? weaponData[WP_REBELBLASTER].altDamage : weaponData[WP_REBELBLASTER].damage;

	if (ent && ent->client && ent->client->NPC_class == CLASS_VEHICLE)
	{
		damage *= 3;
		velocity = ATST_MAIN_VEL + ent->client->ps.speed;
	}
	else
	{
		// If an enemy is shooting at us, lower the velocity so you have a chance to evade
		if (ent->client && ent->client->ps.client_num != 0
			&& ent->client->NPC_class != CLASS_BOBAFETT
			&& ent->client->NPC_class != CLASS_MANDALORIAN
			&& ent->client->NPC_class != CLASS_JANGO
			&& ent->client->NPC_class != CLASS_JANGODUAL)
		{
			if (g_spskill->integer < 2)
			{
				velocity *= REBELBLASTER_NPC_VEL_CUT;
			}
			else
			{
				velocity *= REBELBLASTER_NPC_HARD_VEL_CUT;
			}
		}
	}

	WP_TraceSetStart(ent, start);
	//make sure our start point isn't on the other side of a wall

	WP_MissileTargetHint(ent, start, dir);

	gentity_t* missile = CreateMissile(start, dir, velocity, 10000, ent, altFire);

	missile->classname = "blaster_proj";
	missile->s.weapon = WP_REBELBLASTER;

	// Do the damages
	if (ent->s.number != 0
		&& ent->client->NPC_class != CLASS_BOBAFETT
		&& ent->client->NPC_class != CLASS_MANDALORIAN
		&& ent->client->NPC_class != CLASS_JANGO
		&& ent->client->NPC_class != CLASS_JANGODUAL)
	{
		if (g_spskill->integer == 0)
		{
			damage = REBELBLASTER_NPC_DAMAGE_EASY;
		}
		else if (g_spskill->integer == 1)
		{
			damage = REBELBLASTER_NPC_DAMAGE_NORMAL;
		}
		else
		{
			damage = REBELBLASTER_NPC_DAMAGE_HARD;
		}
	}

	missile->damage = damage;
	if (g_SerenityJediEngineMode->integer == 2)
	{
		missile->dflags = DAMAGE_DEATH_KNOCKBACK | DAMAGE_EXTRA_KNOCKBACK;
	}
	else
	{
		missile->dflags = DAMAGE_DEATH_KNOCKBACK;
	}
	if (altFire)
	{
		missile->methodOfDeath = MOD_REBELBLASTER_ALT;
	}
	else
	{
		missile->methodOfDeath = MOD_REBELBLASTER;
	}
	missile->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;

	// we don't want it to bounce forever
	missile->bounceCount = 8;

	if (ent->weaponModel[1] > 0)
	{
		//dual pistols, toggle the muzzle point back and forth between the two pistols each time he fires
		ent->count = ent->count ? 0 : 1;
	}
}

//---------------------------------------------------------
void WP_FireRebelBlaster(gentity_t* ent, const qboolean alt_fire)
//---------------------------------------------------------
{
	vec3_t dir, angs;

	vectoangles(forwardVec, angs);

	if (ent->client && ent->client->NPC_class == CLASS_VEHICLE)
	{
		//no inherent aim screw up
	}
	else if (ent->client && !(ent->client->ps.forcePowersActive & 1 << FP_SEE) || ent->client->ps.forcePowerLevel[
		FP_SEE] < FORCE_LEVEL_2)
	{
		//force sight 2+ gives perfect aim
		if (alt_fire)
		{
			// add some slop to the alt-fire direction
			if (!WalkCheck(ent) && (ent->s.number < MAX_CLIENTS || G_ControlledByPlayer(ent))) //if running aim is shit
			{
				angs[PITCH] += Q_flrand(-1.0f, 1.0f) * RUNNING_SPREAD;
				angs[YAW] += Q_flrand(-1.0f, 1.0f) * RUNNING_SPREAD;
			}
			else if (ent->client->ps.BlasterAttackChainCount >= BLASTERMISHAPLEVEL_FULL)
			{
				// add some slop to the fire direction
				angs[PITCH] += Q_flrand(-3.0f, 3.0f) * BLASTER_ALT_SPREAD;
				angs[YAW] += Q_flrand(-3.0f, 3.0f) * BLASTER_ALT_SPREAD;
			}
			else if (ent->client->ps.BlasterAttackChainCount >= BLASTERMISHAPLEVEL_HEAVY)
			{
				// add some slop to the fire direction
				angs[PITCH] += Q_flrand(-2.0f, 2.0f) * BLASTER_ALT_SPREAD;
				angs[YAW] += Q_flrand(-2.0f, 2.0f) * BLASTER_ALT_SPREAD;
			}
			else if (PM_CrouchAnim(ent->client->ps.legsAnim))
			{
				//
			}
			else
			{
				angs[PITCH] += Q_flrand(-0.5f, 0.5f) * BLASTER_ALT_SPREAD;
				angs[YAW] += Q_flrand(-0.5f, 0.5f) * BLASTER_ALT_SPREAD;
			}
		}
		else
		{
			if (ent->NPC && ent->NPC->currentAim < 5)
			{
				if (ent->client && ent->NPC &&
					(ent->client->NPC_class == CLASS_STORMTROOPER ||
						ent->client->NPC_class == CLASS_CLONETROOPER ||
						ent->client->NPC_class == CLASS_STORMCOMMANDO ||
						ent->client->NPC_class == CLASS_SWAMPTROOPER ||
						ent->client->NPC_class == CLASS_DROIDEKA ||
						ent->client->NPC_class == CLASS_SBD ||
						ent->client->NPC_class == CLASS_IMPWORKER ||
						ent->client->NPC_class == CLASS_REBEL ||
						ent->client->NPC_class == CLASS_WOOKIE ||
						ent->client->NPC_class == CLASS_BATTLEDROID))
				{
					angs[PITCH] += Q_flrand(-1.0f, 1.0f) * (BLASTER_NPC_SPREAD + (1 - ent->NPC->currentAim) * 0.25f);
					//was 0.5f
					angs[YAW] += Q_flrand(-1.0f, 1.0f) * (BLASTER_NPC_SPREAD + (1 - ent->NPC->currentAim) * 0.25f);
					//was 0.5
				}
			}
			else if (!WalkCheck(ent) && (ent->s.number < MAX_CLIENTS || G_ControlledByPlayer(ent)))
			//if running aim is shit
			{
				angs[PITCH] += Q_flrand(-1.0f, 1.0f) * RUNNING_SPREAD;
				angs[YAW] += Q_flrand(-1.0f, 1.0f) * RUNNING_SPREAD;
			}
			else if (ent->client->ps.BlasterAttackChainCount >= BLASTERMISHAPLEVEL_FULL)
			{
				// add some slop to the fire direction
				angs[PITCH] += Q_flrand(-3.0f, 3.0f) * BLASTER_MAIN_SPREAD;
				angs[YAW] += Q_flrand(-3.0f, 3.0f) * BLASTER_MAIN_SPREAD;
			}
			else if (ent->client->ps.BlasterAttackChainCount >= BLASTERMISHAPLEVEL_HEAVY)
			{
				// add some slop to the fire direction
				angs[PITCH] += Q_flrand(-2.0f, 2.0f) * BLASTER_MAIN_SPREAD;
				angs[YAW] += Q_flrand(-2.0f, 2.0f) * BLASTER_MAIN_SPREAD;
			}
			else if (PM_CrouchAnim(ent->client->ps.legsAnim))
			{
				//
			}
			else
			{
				angs[PITCH] += Q_flrand(-0.5f, 0.5f) * BLASTER_MAIN_SPREAD;
				angs[YAW] += Q_flrand(-0.5f, 0.5f) * BLASTER_MAIN_SPREAD;
			}
		}
	}

	AngleVectors(angs, dir, nullptr, nullptr);

	// FIXME: if temp_org does not have clear trace to inside the bbox, don't shoot!
	WP_FireRebelBlasterMissile(ent, muzzle, dir, alt_fire);
}

//---------------------------------------------------------
void WP_FireRebelBlasterDuals(gentity_t* ent, const qboolean alt_fire, const qboolean secondPistol)
//---------------------------------------------------------
{
	vec3_t dir, angs;

	vectoangles(forwardVec, angs);

	if (ent->client && ent->client->NPC_class == CLASS_VEHICLE)
	{
		//no inherent aim screw up
	}
	else if (ent->client && !(ent->client->ps.forcePowersActive & 1 << FP_SEE) || ent->client->ps.forcePowerLevel[
		FP_SEE] < FORCE_LEVEL_2)
	{
		//force sight 2+ gives perfect aim
		if (alt_fire)
		{
			// add some slop to the alt-fire direction
			if (!WalkCheck(ent) && (ent->s.number < MAX_CLIENTS || G_ControlledByPlayer(ent))) //if running aim is shit
			{
				angs[PITCH] += Q_flrand(-1.0f, 1.0f) * CLONERIFLE_ALT_SPREAD;
				angs[YAW] += Q_flrand(-1.0f, 1.0f) * CLONERIFLE_ALT_SPREAD;
			}
			else if (ent->client->ps.BlasterAttackChainCount >= BLASTERMISHAPLEVEL_FULL)
			{
				// add some slop to the fire direction
				angs[PITCH] += Q_flrand(-3.0f, 3.0f) * BLASTER_ALT_SPREAD;
				angs[YAW] += Q_flrand(-3.0f, 3.0f) * BLASTER_ALT_SPREAD;
			}
			else if (ent->client->ps.BlasterAttackChainCount >= BLASTERMISHAPLEVEL_HEAVY)
			{
				// add some slop to the fire direction
				angs[PITCH] += Q_flrand(-2.0f, 2.0f) * BLASTER_ALT_SPREAD;
				angs[YAW] += Q_flrand(-2.0f, 2.0f) * BLASTER_ALT_SPREAD;
			}
			else if (PM_CrouchAnim(ent->client->ps.legsAnim))
			{
				//
			}
			else
			{
				//
			}
		}
		else
		{
			if (ent->NPC && ent->NPC->currentAim < 5)
			{
				if (ent->client && ent->NPC &&
					(ent->client->NPC_class == CLASS_STORMTROOPER ||
						ent->client->NPC_class == CLASS_CLONETROOPER ||
						ent->client->NPC_class == CLASS_STORMCOMMANDO ||
						ent->client->NPC_class == CLASS_SWAMPTROOPER ||
						ent->client->NPC_class == CLASS_DROIDEKA ||
						ent->client->NPC_class == CLASS_SBD ||
						ent->client->NPC_class == CLASS_IMPWORKER ||
						ent->client->NPC_class == CLASS_REBEL ||
						ent->client->NPC_class == CLASS_WOOKIE ||
						ent->client->NPC_class == CLASS_BATTLEDROID))
				{
					angs[PITCH] += Q_flrand(-1.0f, 1.0f) * (BLASTER_NPC_SPREAD + (1 - ent->NPC->currentAim) * 0.25f);
					//was 0.5f
					angs[YAW] += Q_flrand(-1.0f, 1.0f) * (BLASTER_NPC_SPREAD + (1 - ent->NPC->currentAim) * 0.25f);
					//was 0.5
				}
			}
			else if (!WalkCheck(ent) && (ent->s.number < MAX_CLIENTS || G_ControlledByPlayer(ent)))
			//if running aim is shit
			{
				angs[PITCH] += Q_flrand(-1.0f, 1.0f) * CLONERIFLE_MAIN_SPREAD;
				angs[YAW] += Q_flrand(-1.0f, 1.0f) * CLONERIFLE_MAIN_SPREAD;
			}
			else if (ent->client->ps.BlasterAttackChainCount >= BLASTERMISHAPLEVEL_FULL)
			{
				// add some slop to the fire direction
				angs[PITCH] += Q_flrand(-3.0f, 3.0f) * BLASTER_MAIN_SPREAD;
				angs[YAW] += Q_flrand(-3.0f, 3.0f) * BLASTER_MAIN_SPREAD;
			}
			else if (ent->client->ps.BlasterAttackChainCount >= BLASTERMISHAPLEVEL_HEAVY)
			{
				// add some slop to the fire direction
				angs[PITCH] += Q_flrand(-2.0f, 2.0f) * BLASTER_MAIN_SPREAD;
				angs[YAW] += Q_flrand(-2.0f, 2.0f) * BLASTER_MAIN_SPREAD;
			}
			else if (PM_CrouchAnim(ent->client->ps.legsAnim))
			{
				//
			}
			else
			{
				//
			}
		}
	}

	AngleVectors(angs, dir, nullptr, nullptr);

	if (secondPistol)
	{
		WP_FireRebelBlasterMissile(ent, muzzle2, dir, alt_fire);
	}
	else
	{
		WP_FireRebelBlasterMissile(ent, muzzle, dir, alt_fire);
	}
}

//---------------
//	A280
//---------------

//---------------------------------------------------------
void WP_FireRebelRifleMissile(gentity_t* ent, vec3_t start, vec3_t dir, const qboolean altFire)
//---------------------------------------------------------
{
	int velocity = REBELRIFLE_VELOCITY;
	int damage = altFire ? weaponData[WP_REBELRIFLE].altDamage : weaponData[WP_REBELRIFLE].damage;

	if (ent && ent->client && ent->client->NPC_class == CLASS_VEHICLE)
	{
		damage *= 3;
		velocity = ATST_MAIN_VEL + ent->client->ps.speed;
	}
	else
	{
		// If an enemy is shooting at us, lower the velocity so you have a chance to evade
		if (ent->client && ent->client->ps.client_num != 0
			&& ent->client->NPC_class != CLASS_BOBAFETT
			&& ent->client->NPC_class != CLASS_MANDALORIAN
			&& ent->client->NPC_class != CLASS_JANGO
			&& ent->client->NPC_class != CLASS_JANGODUAL)
		{
			if (g_spskill->integer < 2)
			{
				velocity *= REBELRIFLE_NPC_VEL_CUT;
			}
			else
			{
				velocity *= REBELRIFLE_NPC_HARD_VEL_CUT;
			}
		}
	}

	WP_TraceSetStart(ent, start);
	//make sure our start point isn't on the other side of a wall

	WP_MissileTargetHint(ent, start, dir);

	gentity_t* missile = CreateMissile(start, dir, velocity, 10000, ent, altFire);

	missile->classname = "blaster_proj";
	missile->s.weapon = WP_REBELRIFLE;

	// Do the damages
	if (ent->s.number != 0
		&& ent->client->NPC_class != CLASS_BOBAFETT
		&& ent->client->NPC_class != CLASS_MANDALORIAN
		&& ent->client->NPC_class != CLASS_JANGO
		&& ent->client->NPC_class != CLASS_JANGODUAL)
	{
		if (g_spskill->integer == 0)
		{
			damage = REBELRIFLE_NPC_DAMAGE_EASY;
		}
		else if (g_spskill->integer == 1)
		{
			damage = REBELRIFLE_NPC_DAMAGE_NORMAL;
		}
		else
		{
			damage = REBELRIFLE_NPC_DAMAGE_HARD;
		}
	}

	//	if ( ent->client )
	//	{
	//		if ( ent->client->ps.powerups[PW_WEAPON_OVERCHARGE] > 0 && ent->client->ps.powerups[PW_WEAPON_OVERCHARGE] > cg.time )
	//		{
	//			// in overcharge mode, so doing double damage
	//			missile->flags |= FL_OVERCHARGED;
	//			damage *= 2;
	//		}
	//	}

	missile->damage = damage;
	if (g_SerenityJediEngineMode->integer == 2)
	{
		missile->dflags = DAMAGE_DEATH_KNOCKBACK | DAMAGE_EXTRA_KNOCKBACK;
	}
	else
	{
		missile->dflags = DAMAGE_DEATH_KNOCKBACK;
	}
	if (altFire)
	{
		missile->methodOfDeath = MOD_REBELRIFLE_ALT;
	}
	else
	{
		missile->methodOfDeath = MOD_REBELRIFLE;
	}
	missile->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;

	// we don't want it to bounce forever
	missile->bounceCount = 8;
}

//---------------------------------------------------------
void WP_FireRebelRifle(gentity_t* ent, const qboolean alt_fire)
//---------------------------------------------------------
{
	vec3_t dir, angs;

	vectoangles(forwardVec, angs);

	if (ent->client && ent->client->NPC_class == CLASS_VEHICLE)
	{
		//no inherent aim screw up
	}
	else if (ent->client && !(ent->client->ps.forcePowersActive & 1 << FP_SEE) || ent->client->ps.forcePowerLevel[
		FP_SEE] < FORCE_LEVEL_2)
	{
		//force sight 2+ gives perfect aim
		if (alt_fire)
		{
			// add some slop to the alt-fire direction
			if (!WalkCheck(ent) && (ent->s.number < MAX_CLIENTS || G_ControlledByPlayer(ent))) //if running aim is shit
			{
				angs[PITCH] += Q_flrand(-1.0f, 1.0f) * RUNNING_SPREAD;
				angs[YAW] += Q_flrand(-1.0f, 1.0f) * RUNNING_SPREAD;
			}
			else if (ent->client->ps.BlasterAttackChainCount >= BLASTERMISHAPLEVEL_FULL)
			{
				// add some slop to the fire direction
				angs[PITCH] += Q_flrand(-3.0f, 3.0f) * BLASTER_ALT_SPREAD;
				angs[YAW] += Q_flrand(-3.0f, 3.0f) * BLASTER_ALT_SPREAD;
			}
			else if (ent->client->ps.BlasterAttackChainCount >= BLASTERMISHAPLEVEL_HEAVY)
			{
				// add some slop to the fire direction
				angs[PITCH] += Q_flrand(-2.0f, 2.0f) * BLASTER_ALT_SPREAD;
				angs[YAW] += Q_flrand(-2.0f, 2.0f) * BLASTER_ALT_SPREAD;
			}
			else if (PM_CrouchAnim(ent->client->ps.legsAnim))
			{
				//
			}
			else
			{
				angs[PITCH] += Q_flrand(-0.5f, 0.5f) * BLASTER_ALT_SPREAD;
				angs[YAW] += Q_flrand(-0.5f, 0.5f) * BLASTER_ALT_SPREAD;
			}
		}
		else
		{
			if (ent->NPC && ent->NPC->currentAim < 5)
			{
				if (ent->client && ent->NPC &&
					(ent->client->NPC_class == CLASS_STORMTROOPER ||
						ent->client->NPC_class == CLASS_CLONETROOPER ||
						ent->client->NPC_class == CLASS_STORMCOMMANDO ||
						ent->client->NPC_class == CLASS_SWAMPTROOPER ||
						ent->client->NPC_class == CLASS_DROIDEKA ||
						ent->client->NPC_class == CLASS_SBD ||
						ent->client->NPC_class == CLASS_IMPWORKER ||
						ent->client->NPC_class == CLASS_REBEL ||
						ent->client->NPC_class == CLASS_WOOKIE ||
						ent->client->NPC_class == CLASS_BATTLEDROID))
				{
					angs[PITCH] += Q_flrand(-1.0f, 1.0f) * (BLASTER_NPC_SPREAD + (1 - ent->NPC->currentAim) * 0.25f);
					//was 0.5f
					angs[YAW] += Q_flrand(-1.0f, 1.0f) * (BLASTER_NPC_SPREAD + (1 - ent->NPC->currentAim) * 0.25f);
					//was 0.5
				}
			}
			else if (!WalkCheck(ent) && (ent->s.number < MAX_CLIENTS || G_ControlledByPlayer(ent)))
			//if running aim is shit
			{
				angs[PITCH] += Q_flrand(-1.0f, 1.0f) * RUNNING_SPREAD;
				angs[YAW] += Q_flrand(-1.0f, 1.0f) * RUNNING_SPREAD;
			}
			else if (ent->client->ps.BlasterAttackChainCount >= BLASTERMISHAPLEVEL_FULL)
			{
				// add some slop to the fire direction
				angs[PITCH] += Q_flrand(-3.0f, 3.0f) * BLASTER_MAIN_SPREAD;
				angs[YAW] += Q_flrand(-3.0f, 3.0f) * BLASTER_MAIN_SPREAD;
			}
			else if (ent->client->ps.BlasterAttackChainCount >= BLASTERMISHAPLEVEL_HEAVY)
			{
				// add some slop to the fire direction
				angs[PITCH] += Q_flrand(-2.0f, 2.0f) * BLASTER_MAIN_SPREAD;
				angs[YAW] += Q_flrand(-2.0f, 2.0f) * BLASTER_MAIN_SPREAD;
			}
			else if (PM_CrouchAnim(ent->client->ps.legsAnim))
			{
				//
			}
			else
			{
				angs[PITCH] += Q_flrand(-0.5f, 0.5f) * BLASTER_MAIN_SPREAD;
				angs[YAW] += Q_flrand(-0.5f, 0.5f) * BLASTER_MAIN_SPREAD;
			}
		}
	}

	AngleVectors(angs, dir, nullptr, nullptr);

	// FIXME: if temp_org does not have clear trace to inside the bbox, don't shoot!
	WP_FireRebelRifleMissile(ent, muzzle, dir, alt_fire);
}

//---------------
//	Westar 34
//---------------

//---------------------------------------------------------
void WP_FireJangoPistolMissile(gentity_t* ent, vec3_t start, vec3_t dir, const qboolean altFire)
//---------------------------------------------------------
{
	int velocity = JANGO_VELOCITY;
	int damage = altFire ? weaponData[WP_JANGO].altDamage : weaponData[WP_JANGO].damage;

	if (ent && ent->client && ent->client->NPC_class == CLASS_VEHICLE)
	{
		damage *= 3;
		velocity = ATST_MAIN_VEL + ent->client->ps.speed;
	}
	else
	{
		// If an enemy is shooting at us, lower the velocity so you have a chance to evade
		if (ent->client && ent->client->ps.client_num != 0
			&& ent->client->NPC_class != CLASS_BOBAFETT
			&& ent->client->NPC_class != CLASS_MANDALORIAN
			&& ent->client->NPC_class != CLASS_JANGO
			&& ent->client->NPC_class != CLASS_JANGODUAL)
		{
			if (g_spskill->integer < 2)
			{
				velocity *= JANGO_NPC_VEL_CUT;
			}
			else
			{
				velocity *= JANGO_NPC_HARD_VEL_CUT;
			}
		}
	}

	WP_TraceSetStart(ent, start);
	//make sure our start point isn't on the other side of a wall

	WP_MissileTargetHint(ent, start, dir);

	gentity_t* missile = CreateMissile(start, dir, velocity, 10000, ent, altFire);

	missile->classname = "blaster_proj";
	missile->s.weapon = WP_JANGO;

	// Do the damages
	if (ent->s.number != 0
		&& ent->client->NPC_class != CLASS_BOBAFETT
		&& ent->client->NPC_class != CLASS_MANDALORIAN
		&& ent->client->NPC_class != CLASS_JANGO
		&& ent->client->NPC_class != CLASS_JANGODUAL)
	{
		if (g_spskill->integer == 0)
		{
			damage = JANGO_NPC_DAMAGE_EASY;
		}
		else if (g_spskill->integer == 1)
		{
			damage = JANGO_NPC_DAMAGE_NORMAL;
		}
		else
		{
			damage = JANGO_NPC_DAMAGE_HARD;
		}
	}

	missile->damage = damage;
	if (g_SerenityJediEngineMode->integer == 2)
	{
		missile->dflags = DAMAGE_DEATH_KNOCKBACK | DAMAGE_EXTRA_KNOCKBACK;
	}
	else
	{
		missile->dflags = DAMAGE_DEATH_KNOCKBACK;
	}
	if (altFire)
	{
		missile->methodOfDeath = MOD_JANGO_ALT;
	}
	else
	{
		missile->methodOfDeath = MOD_JANGO;
	}
	missile->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;

	// we don't want it to bounce forever
	missile->bounceCount = 8;

	if (ent->weaponModel[1] > 0)
	{
		//dual pistols, toggle the muzzle point back and forth between the two pistols each time he fires
		ent->count = ent->count ? 0 : 1;
	}
}

//---------------------------------------------------------
void WP_FireJangoWristMissile(gentity_t* ent, vec3_t start, vec3_t dir, const qboolean altFire)
//---------------------------------------------------------
{
	int velocity = CLONECOMMANDO_VELOCITY;
	int damage = altFire ? weaponData[WP_WRIST_BLASTER].altDamage : weaponData[WP_WRIST_BLASTER].damage;

	if (ent && ent->client && ent->client->NPC_class == CLASS_VEHICLE)
	{
		damage *= 3;
		velocity = ATST_MAIN_VEL + ent->client->ps.speed;
	}
	else
	{
		// If an enemy is shooting at us, lower the velocity so you have a chance to evade
		if (ent->client && ent->client->ps.client_num != 0
			&& ent->client->NPC_class != CLASS_BOBAFETT
			&& ent->client->NPC_class != CLASS_JANGO
			&& ent->client->NPC_class != CLASS_JANGODUAL)
		{
			if (g_spskill->integer < 2)
			{
				velocity *= CLONECOMMANDO_NPC_VEL_CUT;
			}
			else
			{
				velocity *= CLONECOMMANDO_NPC_HARD_VEL_CUT;
			}
		}
	}

	WP_TraceSetStart(ent, start);
	//make sure our start point isn't on the other side of a wall

	WP_MissileTargetHint(ent, start, dir);

	gentity_t* missile = CreateMissile(start, dir, velocity, 10000, ent, altFire);

	missile->classname = "clone_proj";
	missile->s.weapon = WP_WRIST_BLASTER;

	// Do the damages
	if (ent->s.number != 0
		&& ent->client->NPC_class != CLASS_BOBAFETT
		&& ent->client->NPC_class != CLASS_JANGO
		&& ent->client->NPC_class != CLASS_JANGODUAL)
	{
		if (g_spskill->integer == 0)
		{
			damage = CLONECOMMANDO_NPC_DAMAGE_EASY;
		}
		else if (g_spskill->integer == 1)
		{
			damage = CLONECOMMANDO_NPC_DAMAGE_NORMAL;
		}
		else
		{
			damage = CLONECOMMANDO_NPC_DAMAGE_HARD;
		}
	}

	missile->damage = damage;
	if (g_SerenityJediEngineMode->integer == 2)
	{
		missile->dflags = DAMAGE_DEATH_KNOCKBACK | DAMAGE_EXTRA_KNOCKBACK;
	}
	else
	{
		missile->dflags = DAMAGE_DEATH_KNOCKBACK;
	}
	if (altFire)
	{
		missile->methodOfDeath = MOD_CLONECOMMANDO_ALT;
	}
	else
	{
		missile->methodOfDeath = MOD_CLONECOMMANDO;
	}
	missile->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;

	// we don't want it to bounce forever
	missile->bounceCount = 8;
}

//---------------------------------------------------------
void WP_FireJangoDualPistolMissile(gentity_t* ent, vec3_t start, vec3_t dir, const qboolean altFire)
//---------------------------------------------------------
{
	int velocity = JANGO_VELOCITY;
	int damage = altFire ? weaponData[WP_DUAL_PISTOL].altDamage : weaponData[WP_DUAL_PISTOL].damage;

	if (ent && ent->client && ent->client->NPC_class == CLASS_VEHICLE)
	{
		damage *= 3;
		velocity = ATST_MAIN_VEL + ent->client->ps.speed;
	}
	else
	{
		// If an enemy is shooting at us, lower the velocity so you have a chance to evade
		if (ent->client && ent->client->ps.client_num != 0
			&& ent->client->NPC_class != CLASS_BOBAFETT
			&& ent->client->NPC_class != CLASS_MANDALORIAN
			&& ent->client->NPC_class != CLASS_JANGO
			&& ent->client->NPC_class != CLASS_JANGODUAL)
		{
			if (g_spskill->integer < 2)
			{
				velocity *= JANGO_NPC_VEL_CUT;
			}
			else
			{
				velocity *= JANGO_NPC_HARD_VEL_CUT;
			}
		}
	}

	WP_TraceSetStart(ent, start);
	//make sure our start point isn't on the other side of a wall

	WP_MissileTargetHint(ent, start, dir);

	gentity_t* missile = CreateMissile(start, dir, velocity, 10000, ent, altFire);

	missile->classname = "blaster_proj";

	missile->s.weapon = WP_DUAL_PISTOL;

	// Do the damages
	if (ent->s.number != 0
		&& ent->client->NPC_class != CLASS_BOBAFETT
		&& ent->client->NPC_class != CLASS_MANDALORIAN
		&& ent->client->NPC_class != CLASS_JANGO
		&& ent->client->NPC_class != CLASS_JANGODUAL)
	{
		if (g_spskill->integer == 0)
		{
			damage = JANGO_NPC_DAMAGE_EASY;
		}
		else if (g_spskill->integer == 1)
		{
			damage = JANGO_NPC_DAMAGE_NORMAL;
		}
		else
		{
			damage = JANGO_NPC_DAMAGE_HARD;
		}
	}

	missile->damage = damage;
	if (g_SerenityJediEngineMode->integer == 2)
	{
		missile->dflags = DAMAGE_DEATH_KNOCKBACK | DAMAGE_EXTRA_KNOCKBACK;
	}
	else
	{
		missile->dflags = DAMAGE_DEATH_KNOCKBACK;
	}
	if (altFire)
	{
		missile->methodOfDeath = MOD_JANGO_ALT;
	}
	else
	{
		missile->methodOfDeath = MOD_JANGO;
	}
	missile->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;

	// we don't want it to bounce forever
	missile->bounceCount = 8;

	if (ent->weaponModel[1] > 0)
	{
		//dual pistols, toggle the muzzle point back and forth between the two pistols each time he fires
		ent->count = ent->count ? 0 : 1;
	}
}

//---------------------------------------------------------
void WP_FireJangoDualPistolMissileDuals(gentity_t* ent, vec3_t start, vec3_t dir, const qboolean altFire)
//---------------------------------------------------------
{
	int velocity = JANGO_VELOCITY;
	int damage = altFire ? weaponData[WP_DUAL_PISTOL].altDamage : weaponData[WP_DUAL_PISTOL].damage;

	if (ent && ent->client && ent->client->NPC_class == CLASS_VEHICLE)
	{
		damage *= 3;
		velocity = ATST_MAIN_VEL + ent->client->ps.speed;
	}
	else
	{
		// If an enemy is shooting at us, lower the velocity so you have a chance to evade
		if (ent->client && ent->client->ps.client_num != 0
			&& ent->client->NPC_class != CLASS_BOBAFETT
			&& ent->client->NPC_class != CLASS_MANDALORIAN
			&& ent->client->NPC_class != CLASS_JANGO
			&& ent->client->NPC_class != CLASS_JANGODUAL)
		{
			if (g_spskill->integer < 2)
			{
				velocity *= JANGO_NPC_VEL_CUT;
			}
			else
			{
				velocity *= JANGO_NPC_HARD_VEL_CUT;
			}
		}
	}

	WP_TraceSetStart(ent, start);
	//make sure our start point isn't on the other side of a wall

	WP_MissileTargetHint(ent, start, dir);

	gentity_t* missile = CreateMissile(start, dir, velocity, 10000, ent, altFire);

	missile->classname = "blaster_proj";

	missile->s.weapon = WP_DUAL_PISTOL;

	// Do the damages
	if (ent->s.number != 0
		&& ent->client->NPC_class != CLASS_BOBAFETT
		&& ent->client->NPC_class != CLASS_MANDALORIAN
		&& ent->client->NPC_class != CLASS_JANGO
		&& ent->client->NPC_class != CLASS_JANGODUAL)
	{
		if (g_spskill->integer == 0)
		{
			damage = JANGO_NPC_DAMAGE_EASY;
		}
		else if (g_spskill->integer == 1)
		{
			damage = JANGO_NPC_DAMAGE_NORMAL;
		}
		else
		{
			damage = JANGO_NPC_DAMAGE_HARD;
		}
	}

	missile->damage = damage;
	if (g_SerenityJediEngineMode->integer == 2)
	{
		missile->dflags = DAMAGE_DEATH_KNOCKBACK | DAMAGE_EXTRA_KNOCKBACK;
	}
	else
	{
		missile->dflags = DAMAGE_DEATH_KNOCKBACK;
	}
	if (altFire)
	{
		missile->methodOfDeath = MOD_JANGO_ALT;
	}
	else
	{
		missile->methodOfDeath = MOD_JANGO;
	}
	missile->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;

	// we don't want it to bounce forever
	missile->bounceCount = 8;

	if (ent->weaponModel[1] > 0)
	{
		//dual pistols, toggle the muzzle point back and forth between the two pistols each time he fires
		ent->count = ent->count ? 0 : 1;
	}
}

//---------------------------------------------------------
void WP_FireJangoPistol(gentity_t* ent, const qboolean alt_fire, const qboolean secondPistol)
//---------------------------------------------------------
{
	vec3_t dir, angs;

	vectoangles(forwardVec, angs);

	if (ent->client && ent->client->NPC_class == CLASS_VEHICLE)
	{
		//no inherent aim screw up
	}
	else if (ent->client && !(ent->client->ps.forcePowersActive & 1 << FP_SEE) || ent->client->ps.forcePowerLevel[
		FP_SEE] < FORCE_LEVEL_2)
	{
		//force sight 2+ gives perfect aim
		vec3_t angles;

		vectoangles(forwardVec, angles);

		if (alt_fire)
		{
			// add some slop to the alt-fire direction
			if (!WalkCheck(ent) && (ent->s.number < MAX_CLIENTS || G_ControlledByPlayer(ent))) //if running aim is shit
			{
				angles[PITCH] += Q_flrand(-1.0f, 1.0f) * CLONERIFLE_ALT_SPREAD;
				angles[YAW] += Q_flrand(-1.0f, 1.0f) * CLONERIFLE_ALT_SPREAD;
			}
			else if (ent->client->ps.BlasterAttackChainCount >= BLASTERMISHAPLEVEL_FULL)
			{
				// add some slop to the fire direction
				angles[PITCH] += Q_flrand(-3.0f, 3.0f) * BLASTER_ALT_SPREAD;
				angles[YAW] += Q_flrand(-3.0f, 3.0f) * BLASTER_ALT_SPREAD;
			}
			else if (ent->client->ps.BlasterAttackChainCount >= BLASTERMISHAPLEVEL_HEAVY)
			{
				// add some slop to the fire direction
				angles[PITCH] += Q_flrand(-2.0f, 2.0f) * BLASTER_ALT_SPREAD;
				angles[YAW] += Q_flrand(-2.0f, 2.0f) * BLASTER_ALT_SPREAD;
			}
			else if (PM_CrouchAnim(ent->client->ps.legsAnim))
			{
				//
			}
			else
			{
				//
			}
		}
		else
		{
			if (ent->NPC && ent->NPC->currentAim < 5)
			{
				if (ent->client && ent->NPC &&
					(ent->client->NPC_class == CLASS_STORMTROOPER ||
						ent->client->NPC_class == CLASS_CLONETROOPER ||
						ent->client->NPC_class == CLASS_STORMCOMMANDO ||
						ent->client->NPC_class == CLASS_SWAMPTROOPER ||
						ent->client->NPC_class == CLASS_DROIDEKA ||
						ent->client->NPC_class == CLASS_SBD ||
						ent->client->NPC_class == CLASS_IMPWORKER ||
						ent->client->NPC_class == CLASS_REBEL ||
						ent->client->NPC_class == CLASS_WOOKIE ||
						ent->client->NPC_class == CLASS_BATTLEDROID))
				{
					angles[PITCH] += Q_flrand(-1.0f, 1.0f) * (BLASTER_NPC_SPREAD + (1 - ent->NPC->currentAim) *
						0.25f);
					//was 0.5f
					angles[YAW] += Q_flrand(-1.0f, 1.0f) * (BLASTER_NPC_SPREAD + (1 - ent->NPC->currentAim) * 0.25f);
					//was 0.5
				}
			}
			else if (!WalkCheck(ent) && (ent->s.number < MAX_CLIENTS || G_ControlledByPlayer(ent)))
			//if running aim is shit
			{
				angles[PITCH] += Q_flrand(-1.0f, 1.0f) * CLONERIFLE_MAIN_SPREAD;
				angles[YAW] += Q_flrand(-1.0f, 1.0f) * CLONERIFLE_MAIN_SPREAD;
			}
			else if (ent->client->ps.BlasterAttackChainCount >= BLASTERMISHAPLEVEL_FULL)
			{
				// add some slop to the fire direction
				angles[PITCH] += Q_flrand(-3.0f, 3.0f) * BLASTER_MAIN_SPREAD;
				angles[YAW] += Q_flrand(-3.0f, 3.0f) * BLASTER_MAIN_SPREAD;
			}
			else if (ent->client->ps.BlasterAttackChainCount >= BLASTERMISHAPLEVEL_HEAVY)
			{
				// add some slop to the fire direction
				angles[PITCH] += Q_flrand(-2.0f, 2.0f) * BLASTER_MAIN_SPREAD;
				angles[YAW] += Q_flrand(-2.0f, 2.0f) * BLASTER_MAIN_SPREAD;
			}
			else if (PM_CrouchAnim(ent->client->ps.legsAnim))
			{
				//
			}
			else
			{
				//
			}
		}

		AngleVectors(angles, forwardVec, nullptr, nullptr);
	}

	AngleVectors(angs, dir, nullptr, nullptr);

	if (secondPistol)
	{
		WP_FireJangoPistolMissile(ent, muzzle2, dir, alt_fire);
	}
	else
	{
		WP_FireJangoPistolMissile(ent, muzzle, dir, alt_fire);
	}
}

//---------------------------------------------------------
void WP_FireWristPistol(gentity_t* ent, const qboolean alt_fire)
//---------------------------------------------------------
{
	vec3_t dir, angs;

	vectoangles(forwardVec, angs);

	if (ent->client && ent->client->NPC_class == CLASS_VEHICLE)
	{
		//no inherent aim screw up
	}
	else if (ent->client && !(ent->client->ps.forcePowersActive & 1 << FP_SEE) || ent->client->ps.forcePowerLevel[
		FP_SEE] < FORCE_LEVEL_2)
	{
		//force sight 2+ gives perfect aim
		vec3_t angles;

		vectoangles(forwardVec, angles);

		if (alt_fire)
		{
			// add some slop to the alt-fire direction
			if (!WalkCheck(ent) && (ent->s.number < MAX_CLIENTS || G_ControlledByPlayer(ent))) //if running aim is shit
			{
				angles[PITCH] += Q_flrand(-1.0f, 1.0f) * CLONERIFLE_ALT_SPREAD;
				angles[YAW] += Q_flrand(-1.0f, 1.0f) * CLONERIFLE_ALT_SPREAD;
			}
			else if (ent->client->ps.BlasterAttackChainCount >= BLASTERMISHAPLEVEL_FULL)
			{
				// add some slop to the fire direction
				angles[PITCH] += Q_flrand(-3.0f, 3.0f) * BLASTER_ALT_SPREAD;
				angles[YAW] += Q_flrand(-3.0f, 3.0f) * BLASTER_ALT_SPREAD;
			}
			else if (ent->client->ps.BlasterAttackChainCount >= BLASTERMISHAPLEVEL_HEAVY)
			{
				// add some slop to the fire direction
				angles[PITCH] += Q_flrand(-2.0f, 2.0f) * BLASTER_ALT_SPREAD;
				angles[YAW] += Q_flrand(-2.0f, 2.0f) * BLASTER_ALT_SPREAD;
			}
			else if (PM_CrouchAnim(ent->client->ps.legsAnim))
			{
				//
			}
			else
			{
				//
			}
		}
		else
		{
			if (ent->NPC && ent->NPC->currentAim < 5)
			{
				if (ent->client && ent->NPC &&
					(ent->client->NPC_class == CLASS_STORMTROOPER ||
						ent->client->NPC_class == CLASS_CLONETROOPER ||
						ent->client->NPC_class == CLASS_STORMCOMMANDO ||
						ent->client->NPC_class == CLASS_SWAMPTROOPER ||
						ent->client->NPC_class == CLASS_DROIDEKA ||
						ent->client->NPC_class == CLASS_SBD ||
						ent->client->NPC_class == CLASS_IMPWORKER ||
						ent->client->NPC_class == CLASS_REBEL ||
						ent->client->NPC_class == CLASS_WOOKIE ||
						ent->client->NPC_class == CLASS_BATTLEDROID))
				{
					angles[PITCH] += Q_flrand(-1.0f, 1.0f) * (BLASTER_NPC_SPREAD + (1 - ent->NPC->currentAim) *
						0.25f);
					//was 0.5f
					angles[YAW] += Q_flrand(-1.0f, 1.0f) * (BLASTER_NPC_SPREAD + (1 - ent->NPC->currentAim) * 0.25f);
					//was 0.5
				}
			}
			else if (!WalkCheck(ent) && (ent->s.number < MAX_CLIENTS || G_ControlledByPlayer(ent)))
			//if running aim is shit
			{
				angles[PITCH] += Q_flrand(-1.0f, 1.0f) * CLONERIFLE_MAIN_SPREAD;
				angles[YAW] += Q_flrand(-1.0f, 1.0f) * CLONERIFLE_MAIN_SPREAD;
			}
			else if (ent->client->ps.BlasterAttackChainCount >= BLASTERMISHAPLEVEL_FULL)
			{
				// add some slop to the fire direction
				angles[PITCH] += Q_flrand(-3.0f, 3.0f) * BLASTER_MAIN_SPREAD;
				angles[YAW] += Q_flrand(-3.0f, 3.0f) * BLASTER_MAIN_SPREAD;
			}
			else if (ent->client->ps.BlasterAttackChainCount >= BLASTERMISHAPLEVEL_HEAVY)
			{
				// add some slop to the fire direction
				angles[PITCH] += Q_flrand(-2.0f, 2.0f) * BLASTER_MAIN_SPREAD;
				angles[YAW] += Q_flrand(-2.0f, 2.0f) * BLASTER_MAIN_SPREAD;
			}
			else if (PM_CrouchAnim(ent->client->ps.legsAnim))
			{
				//
			}
			else
			{
				//
			}
		}

		AngleVectors(angles, forwardVec, nullptr, nullptr);
	}

	AngleVectors(angs, dir, nullptr, nullptr);

	WP_FireJangoWristMissile(ent, muzzle, dir, alt_fire);
}

//---------------------------------------------------------
void WP_FireJangoDualPistol(gentity_t* ent, const qboolean alt_fire)
//---------------------------------------------------------
{
	vec3_t dir, angs;

	vectoangles(forwardVec, angs);

	if (ent->client && ent->client->NPC_class == CLASS_VEHICLE)
	{
		//no inherent aim screw up
	}
	else if (ent->client && !(ent->client->ps.forcePowersActive & 1 << FP_SEE) || ent->client->ps.forcePowerLevel[
		FP_SEE] < FORCE_LEVEL_2)
	{
		//force sight 2+ gives perfect aim
		vec3_t angles;

		vectoangles(forwardVec, angles);

		if (alt_fire)
		{
			// add some slop to the alt-fire direction
			if (!WalkCheck(ent) && (ent->s.number < MAX_CLIENTS || G_ControlledByPlayer(ent))) //if running aim is shit
			{
				angles[PITCH] += Q_flrand(-1.0f, 1.0f) * CLONERIFLE_ALT_SPREAD;
				angles[YAW] += Q_flrand(-1.0f, 1.0f) * CLONERIFLE_ALT_SPREAD;
			}
			else if (ent->client->ps.BlasterAttackChainCount >= BLASTERMISHAPLEVEL_FULL)
			{
				// add some slop to the fire direction
				angles[PITCH] += Q_flrand(-3.0f, 3.0f) * BLASTER_ALT_SPREAD;
				angles[YAW] += Q_flrand(-3.0f, 3.0f) * BLASTER_ALT_SPREAD;
			}
			else if (ent->client->ps.BlasterAttackChainCount >= BLASTERMISHAPLEVEL_HEAVY)
			{
				// add some slop to the fire direction
				angles[PITCH] += Q_flrand(-2.0f, 2.0f) * BLASTER_ALT_SPREAD;
				angles[YAW] += Q_flrand(-2.0f, 2.0f) * BLASTER_ALT_SPREAD;
			}
			else if (PM_CrouchAnim(ent->client->ps.legsAnim))
			{
				//
			}
			else
			{
				//
			}
		}
		else
		{
			if (ent->NPC && ent->NPC->currentAim < 5)
			{
				if (ent->client && ent->NPC &&
					(ent->client->NPC_class == CLASS_STORMTROOPER ||
						ent->client->NPC_class == CLASS_CLONETROOPER ||
						ent->client->NPC_class == CLASS_STORMCOMMANDO ||
						ent->client->NPC_class == CLASS_SWAMPTROOPER ||
						ent->client->NPC_class == CLASS_DROIDEKA ||
						ent->client->NPC_class == CLASS_SBD ||
						ent->client->NPC_class == CLASS_IMPWORKER ||
						ent->client->NPC_class == CLASS_REBEL ||
						ent->client->NPC_class == CLASS_WOOKIE ||
						ent->client->NPC_class == CLASS_BATTLEDROID))
				{
					angles[PITCH] += Q_flrand(-1.0f, 1.0f) * (BLASTER_NPC_SPREAD + (1 - ent->NPC->currentAim) *
						0.25f);
					//was 0.5f
					angles[YAW] += Q_flrand(-1.0f, 1.0f) * (BLASTER_NPC_SPREAD + (1 - ent->NPC->currentAim) * 0.25f);
					//was 0.5
				}
			}
			else if (!WalkCheck(ent) && (ent->s.number < MAX_CLIENTS || G_ControlledByPlayer(ent)))
			//if running aim is shit
			{
				angles[PITCH] += Q_flrand(-1.0f, 1.0f) * CLONERIFLE_MAIN_SPREAD;
				angles[YAW] += Q_flrand(-1.0f, 1.0f) * CLONERIFLE_MAIN_SPREAD;
			}
			else if (ent->client->ps.BlasterAttackChainCount >= BLASTERMISHAPLEVEL_FULL)
			{
				// add some slop to the fire direction
				angles[PITCH] += Q_flrand(-3.0f, 3.0f) * BLASTER_MAIN_SPREAD;
				angles[YAW] += Q_flrand(-3.0f, 3.0f) * BLASTER_MAIN_SPREAD;
			}
			else if (ent->client->ps.BlasterAttackChainCount >= BLASTERMISHAPLEVEL_HEAVY)
			{
				// add some slop to the fire direction
				angles[PITCH] += Q_flrand(-2.0f, 2.0f) * BLASTER_MAIN_SPREAD;
				angles[YAW] += Q_flrand(-2.0f, 2.0f) * BLASTER_MAIN_SPREAD;
			}
			else if (PM_CrouchAnim(ent->client->ps.legsAnim))
			{
				//
			}
			else
			{
				//
			}
		}

		AngleVectors(angles, forwardVec, nullptr, nullptr);
	}

	AngleVectors(angs, dir, nullptr, nullptr);

	WP_FireJangoDualPistolMissile(ent, muzzle, dir, alt_fire);
}

//---------------------------------------------------------
void WP_FireJangoFPPistolDuals(gentity_t* ent, const qboolean alt_fire, const qboolean secondPistol)
//---------------------------------------------------------
{
	vec3_t dir, angs;

	vectoangles(forwardVec, angs);

	if (ent->client && ent->client->NPC_class == CLASS_VEHICLE)
	{
		//no inherent aim screw up
	}
	else if (ent->client && !(ent->client->ps.forcePowersActive & 1 << FP_SEE) || ent->client->ps.forcePowerLevel[
		FP_SEE] < FORCE_LEVEL_2)
	{
		//force sight 2+ gives perfect aim
		vec3_t angles;

		vectoangles(forwardVec, angles);

		if (alt_fire)
		{
			// add some slop to the alt-fire direction
			if (!WalkCheck(ent) && (ent->s.number < MAX_CLIENTS || G_ControlledByPlayer(ent))) //if running aim is shit
			{
				angles[PITCH] += Q_flrand(-1.0f, 1.0f) * CLONERIFLE_ALT_SPREAD;
				angles[YAW] += Q_flrand(-1.0f, 1.0f) * CLONERIFLE_ALT_SPREAD;
			}
			else if (ent->client->ps.BlasterAttackChainCount >= BLASTERMISHAPLEVEL_FULL)
			{
				// add some slop to the fire direction
				angles[PITCH] += Q_flrand(-3.0f, 3.0f) * BLASTER_ALT_SPREAD;
				angles[YAW] += Q_flrand(-3.0f, 3.0f) * BLASTER_ALT_SPREAD;
			}
			else if (ent->client->ps.BlasterAttackChainCount >= BLASTERMISHAPLEVEL_HEAVY)
			{
				// add some slop to the fire direction
				angles[PITCH] += Q_flrand(-2.0f, 2.0f) * BLASTER_ALT_SPREAD;
				angles[YAW] += Q_flrand(-2.0f, 2.0f) * BLASTER_ALT_SPREAD;
			}
			else if (PM_CrouchAnim(ent->client->ps.legsAnim))
			{
				//
			}
			else
			{
				//
			}
		}
		else
		{
			if (ent->NPC && ent->NPC->currentAim < 5)
			{
				if (ent->client && ent->NPC &&
					(ent->client->NPC_class == CLASS_STORMTROOPER ||
						ent->client->NPC_class == CLASS_CLONETROOPER ||
						ent->client->NPC_class == CLASS_STORMCOMMANDO ||
						ent->client->NPC_class == CLASS_SWAMPTROOPER ||
						ent->client->NPC_class == CLASS_DROIDEKA ||
						ent->client->NPC_class == CLASS_SBD ||
						ent->client->NPC_class == CLASS_IMPWORKER ||
						ent->client->NPC_class == CLASS_REBEL ||
						ent->client->NPC_class == CLASS_WOOKIE ||
						ent->client->NPC_class == CLASS_BATTLEDROID))
				{
					angles[PITCH] += Q_flrand(-1.0f, 1.0f) * (BLASTER_NPC_SPREAD + (1 - ent->NPC->currentAim) *
						0.25f);
					//was 0.5f
					angles[YAW] += Q_flrand(-1.0f, 1.0f) * (BLASTER_NPC_SPREAD + (1 - ent->NPC->currentAim) * 0.25f);
					//was 0.5
				}
			}
			else if (!WalkCheck(ent) && (ent->s.number < MAX_CLIENTS || G_ControlledByPlayer(ent)))
			//if running aim is shit
			{
				angles[PITCH] += Q_flrand(-1.0f, 1.0f) * CLONERIFLE_MAIN_SPREAD;
				angles[YAW] += Q_flrand(-1.0f, 1.0f) * CLONERIFLE_MAIN_SPREAD;
			}
			else if (ent->client->ps.BlasterAttackChainCount >= BLASTERMISHAPLEVEL_FULL)
			{
				// add some slop to the fire direction
				angles[PITCH] += Q_flrand(-3.0f, 3.0f) * BLASTER_MAIN_SPREAD;
				angles[YAW] += Q_flrand(-3.0f, 3.0f) * BLASTER_MAIN_SPREAD;
			}
			else if (ent->client->ps.BlasterAttackChainCount >= BLASTERMISHAPLEVEL_HEAVY)
			{
				// add some slop to the fire direction
				angles[PITCH] += Q_flrand(-2.0f, 2.0f) * BLASTER_MAIN_SPREAD;
				angles[YAW] += Q_flrand(-2.0f, 2.0f) * BLASTER_MAIN_SPREAD;
			}
			else if (PM_CrouchAnim(ent->client->ps.legsAnim))
			{
				//
			}
			else
			{
				//
			}
		}

		AngleVectors(angles, forwardVec, nullptr, nullptr);
	}

	AngleVectors(angs, dir, nullptr, nullptr);

	if (secondPistol)
	{
		WP_FireJangoDualPistolMissileDuals(ent, muzzle2, dir, alt_fire);
	}
	else
	{
		WP_FireJangoDualPistolMissileDuals(ent, muzzle, dir, alt_fire);
	}
}

//---------------
//	EE-3 Carbine Rifle
//---------------

//---------------------------------------------------------
void WP_FireBobaRifleMissile(gentity_t* ent, vec3_t start, vec3_t dir, const qboolean altFire)
//---------------------------------------------------------
{
	int velocity = BOBA_VELOCITY;
	int damage = altFire ? weaponData[WP_BOBA].altDamage : weaponData[WP_BOBA].damage;

	if (altFire)
	{
		velocity = Q_irand(1500, 3000);
	}

	if (ent && ent->client && ent->client->NPC_class == CLASS_VEHICLE)
	{
		damage *= 3;
		velocity = ATST_MAIN_VEL + ent->client->ps.speed;
	}
	else
	{
		// If an enemy is shooting at us, lower the velocity so you have a chance to evade
		if (ent->client && ent->client->ps.client_num != 0
			&& ent->client->NPC_class != CLASS_BOBAFETT
			&& ent->client->NPC_class != CLASS_MANDALORIAN
			&& ent->client->NPC_class != CLASS_JANGO
			&& ent->client->NPC_class != CLASS_JANGODUAL)
		{
			if (g_spskill->integer < 2)
			{
				velocity *= BOBA_NPC_VEL_CUT;
			}
			else
			{
				velocity *= BOBA_NPC_HARD_VEL_CUT;
			}
		}
	}

	WP_TraceSetStart(ent, start);
	//make sure our start point isn't on the other side of a wall

	WP_MissileTargetHint(ent, start, dir);

	gentity_t* missile = CreateMissile(start, dir, velocity, 10000, ent, altFire);

	missile->classname = "blaster_proj";
	missile->s.weapon = WP_BOBA;

	// Do the damages
	if (ent->s.number != 0
		&& ent->client->NPC_class != CLASS_BOBAFETT
		&& ent->client->NPC_class != CLASS_MANDALORIAN
		&& ent->client->NPC_class != CLASS_JANGO
		&& ent->client->NPC_class != CLASS_JANGODUAL)
	{
		if (g_spskill->integer == 0)
		{
			damage = BOBA_NPC_DAMAGE_EASY;
		}
		else if (g_spskill->integer == 1)
		{
			damage = BOBA_NPC_DAMAGE_NORMAL;
		}
		else
		{
			damage = BOBA_NPC_DAMAGE_HARD;
		}
	}

	missile->damage = damage;
	if (g_SerenityJediEngineMode->integer == 2)
	{
		missile->dflags = DAMAGE_DEATH_KNOCKBACK | DAMAGE_EXTRA_KNOCKBACK;
	}
	else
	{
		missile->dflags = DAMAGE_DEATH_KNOCKBACK;
	}

	/*	if (altFire)
		{
			missile->methodOfDeath = MOD_BOBA_ALT;
		}
		else*/
	//	{
	missile->methodOfDeath = MOD_BOBA;
	//	}

	missile->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;

	// we don't want it to bounce forever
	missile->bounceCount = 8;
}

//---------------------------------------------------------
void WP_FireBobaRifle(gentity_t* ent, const qboolean alt_fire)
//---------------------------------------------------------
{
	vec3_t dir, angs;

	vectoangles(forwardVec, angs);

	if (ent->client && ent->client->NPC_class == CLASS_VEHICLE)
	{
		//no inherent aim screw up
	}
	else if (ent->client && !(ent->client->ps.forcePowersActive & 1 << FP_SEE) || ent->client->ps.forcePowerLevel[
		FP_SEE] < FORCE_LEVEL_2)
	{
		//force sight 2+ gives perfect aim
		if (alt_fire)
		{
			// add some slop to the alt-fire direction
			if (!WalkCheck(ent) && (ent->s.number < MAX_CLIENTS || G_ControlledByPlayer(ent))) //if running aim is shit
			{
				angs[PITCH] += Q_flrand(-1.0f, 1.0f) * RUNNING_SPREAD;
				angs[YAW] += Q_flrand(-1.0f, 1.0f) * RUNNING_SPREAD;
			}
			else if (ent->client->ps.BlasterAttackChainCount >= BLASTERMISHAPLEVEL_FULL)
			{
				// add some slop to the fire direction
				angs[PITCH] += Q_flrand(-3.0f, 3.0f) * BLASTER_ALT_SPREAD;
				angs[YAW] += Q_flrand(-3.0f, 3.0f) * BLASTER_ALT_SPREAD;
			}
			else if (ent->client->ps.BlasterAttackChainCount >= BLASTERMISHAPLEVEL_HEAVY)
			{
				// add some slop to the fire direction
				angs[PITCH] += Q_flrand(-2.0f, 2.0f) * BLASTER_ALT_SPREAD;
				angs[YAW] += Q_flrand(-2.0f, 2.0f) * BLASTER_ALT_SPREAD;
			}
			else if (PM_CrouchAnim(ent->client->ps.legsAnim))
			{
				//
			}
			else
			{
				angs[PITCH] += Q_flrand(-0.5f, 0.5f) * BLASTER_ALT_SPREAD;
				angs[YAW] += Q_flrand(-0.5f, 0.5f) * BLASTER_ALT_SPREAD;
			}
		}
		else
		{
			if (ent->NPC && ent->NPC->currentAim < 5)
			{
				if (ent->client && ent->NPC &&
					(ent->client->NPC_class == CLASS_STORMTROOPER ||
						ent->client->NPC_class == CLASS_CLONETROOPER ||
						ent->client->NPC_class == CLASS_STORMCOMMANDO ||
						ent->client->NPC_class == CLASS_SWAMPTROOPER ||
						ent->client->NPC_class == CLASS_DROIDEKA ||
						ent->client->NPC_class == CLASS_SBD ||
						ent->client->NPC_class == CLASS_IMPWORKER ||
						ent->client->NPC_class == CLASS_REBEL ||
						ent->client->NPC_class == CLASS_WOOKIE ||
						ent->client->NPC_class == CLASS_BATTLEDROID))
				{
					angs[PITCH] += Q_flrand(-1.0f, 1.0f) * (BLASTER_NPC_SPREAD + (1 - ent->NPC->currentAim) * 0.25f);
					//was 0.5f
					angs[YAW] += Q_flrand(-1.0f, 1.0f) * (BLASTER_NPC_SPREAD + (1 - ent->NPC->currentAim) * 0.25f);
					//was 0.5
				}
			}
			else if (!WalkCheck(ent) && (ent->s.number < MAX_CLIENTS || G_ControlledByPlayer(ent)))
			//if running aim is shit
			{
				angs[PITCH] += Q_flrand(-1.0f, 1.0f) * RUNNING_SPREAD;
				angs[YAW] += Q_flrand(-1.0f, 1.0f) * RUNNING_SPREAD;
			}
			else if (ent->client->ps.BlasterAttackChainCount >= BLASTERMISHAPLEVEL_FULL)
			{
				// add some slop to the fire direction
				angs[PITCH] += Q_flrand(-3.0f, 3.0f) * BLASTER_MAIN_SPREAD;
				angs[YAW] += Q_flrand(-3.0f, 3.0f) * BLASTER_MAIN_SPREAD;
			}
			else if (ent->client->ps.BlasterAttackChainCount >= BLASTERMISHAPLEVEL_HEAVY)
			{
				// add some slop to the fire direction
				angs[PITCH] += Q_flrand(-2.0f, 2.0f) * BLASTER_MAIN_SPREAD;
				angs[YAW] += Q_flrand(-2.0f, 2.0f) * BLASTER_MAIN_SPREAD;
			}
			else if (PM_CrouchAnim(ent->client->ps.legsAnim))
			{
				//
			}
			else
			{
				angs[PITCH] += Q_flrand(-0.5f, 0.5f) * BLASTER_MAIN_SPREAD;
				angs[YAW] += Q_flrand(-0.5f, 0.5f) * BLASTER_MAIN_SPREAD;
			}
		}
	}

	AngleVectors(angs, dir, nullptr, nullptr);

	// FIXME: if temp_org does not have clear trace to inside the bbox, don't shoot!
	WP_FireBobaRifleMissile(ent, muzzle, dir, alt_fire);
}
