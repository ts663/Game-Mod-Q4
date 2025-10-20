
#include "../../idlib/precompiled.h"
#pragma hdrstop

#include "../Game_local.h"

class rvMonsterPokemonIronMaiden : public idAI {
public:

	CLASS_PROTOTYPE(rvMonsterPokemonIronMaiden);

	rvMonsterPokemonIronMaiden(void);

	void				InitSpawnArgsVariables(void);
	void				Spawn(void);
	void				PrintDets(void);
	void				GiveXP(int);
	bool				DefeatedEnemy(void);
	void				Attack1(void);
	void				Attack2(void);
	void				Save(idSaveGame* savefile) const;
	void				Restore(idRestoreGame* savefile);

	// Add some dynamic externals for debugging
	virtual void		GetDebugInfo(debugInfoProc_t proc, void* userData);

	virtual int			FilterTactical(int availableTactical);

protected:

	int					phaseTime;
	jointHandle_t		jointBansheeAttack;
	int					enemyStunTime;

	rvAIAction			actionBansheeAttack;

	virtual bool		CheckActions(void);

	void				PhaseOut(void);
	void				PhaseIn(void);

	virtual void		OnDeath(void);

private:

	// Custom actions
	bool				CheckAction_BansheeAttack(rvAIAction* action, int animNum);
	bool				PerformAction_PhaseOut(void);
	bool				PerformAction_PhaseIn(void);

	// Global States
	stateResult_t		State_Phased(const stateParms_t& parms);

	// Torso States
	stateResult_t		State_Torso_PhaseIn(const stateParms_t& parms);
	stateResult_t		State_Torso_PhaseOut(const stateParms_t& parms);
	stateResult_t		State_Torso_BansheeAttack(const stateParms_t& parms);
	stateResult_t		State_Torso_BansheeAttackDone(const stateParms_t& parms);

	// Frame commands
	stateResult_t		Frame_PhaseIn(const stateParms_t& parms);
	stateResult_t		Frame_PhaseOut(const stateParms_t& parms);
	stateResult_t		Frame_BansheeAttack(const stateParms_t& parms);
	stateResult_t		Frame_EndBansheeAttack(const stateParms_t& parms);

	idEntity* lastEnemy;

	CLASS_STATES_PROTOTYPE(rvMonsterPokemonIronMaiden);
};

CLASS_DECLARATION(idAI, rvMonsterPokemonIronMaiden)
END_CLASS

/*
================
rvMonsterPokemonIronMaiden::rvMonsterPokemonIronMaiden
================
*/
rvMonsterPokemonIronMaiden::rvMonsterPokemonIronMaiden(void) {
	enemyStunTime = 0;
	phaseTime = 0;
}

void rvMonsterPokemonIronMaiden::InitSpawnArgsVariables(void) {
	// Cache the mouth joint
	jointBansheeAttack = animator.GetJointHandle(spawnArgs.GetString("joint_bansheeAttack", "mouth_effect"));
}
/*
================
rvMonsterPokemonIronMaiden::Spawn
================
*/
void rvMonsterPokemonIronMaiden::Spawn(void) {
	// Custom actions
	actionBansheeAttack.Init(spawnArgs, "action_bansheeAttack", "Torso_BansheeAttack", AIACTIONF_ATTACK);
	team = gameLocal.GetLocalPlayer()->team;
	gameLocal.GetLocalPlayer()->activePokemon = this;

	InitSpawnArgsVariables();

	PlayEffect("fx_dress", animator.GetJointHandle(spawnArgs.GetString("joint_laser", "cog_bone")), true);

	gameLocal.Printf("Spawned pokemon iron maiden\n");
	PrintDets();
}

/*
================
rvMonsterPokemonIronMaiden::PrintDets
================
*/
void rvMonsterPokemonIronMaiden::PrintDets(void) {
	gameLocal.Printf("--------------------------------------\n");
	gameLocal.Printf("%s\n", name.c_str());
	gameLocal.Printf("XP: %d\n", xp);
	gameLocal.Printf("Level: %d\n", level);
	gameLocal.Printf("XP to level up: %d\n", xpToLevelUp);
	gameLocal.Printf("Player's active pokemon: %s\n", gameLocal.GetLocalPlayer()->activePokemon->name.c_str());
	gameLocal.Printf("--------------------------------------\n");
}

/*
================
rvMonsterPokemonIronMaiden::GiveXP
================
*/
void rvMonsterPokemonIronMaiden::GiveXP(int amount) {
	int orig = amount;
	while (amount > 0) {
		if (xp + amount >= xpToLevelUp) {
			amount -= xpToLevelUp - xp;
			xp = 0;
			level++;
			xpToLevelUp *= 1.5;
		}
		else {
			xp += amount;
			amount = 0;
		}
	}
	gameLocal.Printf("Gave %d XP\nNow level %d\n", orig, level);
}

/*
================
rvMonsterPokemonIronMaiden::DefeatedEnemy
================
*/
bool rvMonsterPokemonIronMaiden::DefeatedEnemy(void) {
	if (lastEnemy) {
		if (lastEnemy->health <= 0) {
			lastEnemy = NULL;
			return true;
		}
	}
	return false;
}

/*
================
rvMonsterPokemonIronMaiden::Attack1
================
*/
void rvMonsterPokemonIronMaiden::Attack1(void) {
	PlayAnim(ANIMCHANNEL_LEGS, "melee_attack1", 4);
}

/*
================
rvMonsterPokemonIronMaiden::Attack2
================
*/
void rvMonsterPokemonIronMaiden::Attack2(void) {
	/*if (level < 2) {
		return;
	}*/
	PlayAnim(ANIMCHANNEL_LEGS, "range_attack1", 4);
}

/*
================
rvMonsterPokemonIronMaiden::Save
================
*/
void rvMonsterPokemonIronMaiden::Save(idSaveGame* savefile) const {
	savefile->WriteInt(phaseTime);
	savefile->WriteInt(enemyStunTime);

	actionBansheeAttack.Save(savefile);
}

/*
================
rvMonsterPokemonIronMaiden::Restore
================
*/
void rvMonsterPokemonIronMaiden::Restore(idRestoreGame* savefile) {
	savefile->ReadInt(phaseTime);
	savefile->ReadInt(enemyStunTime);

	actionBansheeAttack.Restore(savefile);

	InitSpawnArgsVariables();
}

/*
================
rvMonsterPokemonIronMaiden::FilterTactical
================
*/
int rvMonsterPokemonIronMaiden::FilterTactical(int availableTactical) {
	// When hidden the iron maiden only uses ranged tactical
	if (fl.hidden) {
		availableTactical &= (AITACTICAL_RANGED_BIT | AITACTICAL_TURRET_BIT);
	}
	return idAI::FilterTactical(availableTactical);
}

/*
================
rvMonsterPokemonIronMaiden::CheckAction_BansheeAttack
================
*/
bool rvMonsterPokemonIronMaiden::CheckAction_BansheeAttack(rvAIAction* action, int animNum) {
	return CheckAction_RangedAttack(action, animNum);
}

/*
================
rvMonsterPokemonIronMaiden::PerformAction_PhaseIn
================
*/
bool rvMonsterPokemonIronMaiden::PerformAction_PhaseIn(void) {
	if (!phaseTime) {
		return false;
	}

	// Must be out for at least 3 seconds 
	if (gameLocal.time - phaseTime < 3000) {
		return false;
	}

	// Phase in after 10 seconds or our movement is done
	if (gameLocal.time - phaseTime > 10000 || move.fl.done) {
		// Make sure we arent in something
		if (CanBecomeSolid()) {
			PerformAction("Torso_PhaseIn", 4, true);
			return true;
		}
	}

	return false;
}

/*
================
rvMonsterPokemonIronMaiden::PerformAction_PhaseOut
================
*/
bool rvMonsterPokemonIronMaiden::PerformAction_PhaseOut(void) {
	// Little randomization 
	if (gameLocal.random.RandomFloat() > 0.5f) {
		return false;
	}
	if (!enemyStunTime || gameLocal.time - enemyStunTime > 1500) {
		return false;
	}

	PerformAction("Torso_PhaseOut", 4, true);
	return true;
}

/*
================
rvMonsterPokemonIronMaiden::CheckActions
================
*/
bool rvMonsterPokemonIronMaiden::CheckActions(void) {
	// When phased the only available action is phase in
	if (phaseTime) {
		if (PerformAction_PhaseIn()) {
			return true;
		}
		return false;
	}

	if (PerformAction(&actionBansheeAttack, (checkAction_t)&rvMonsterPokemonIronMaiden::CheckAction_BansheeAttack, &actionTimerRangedAttack)) {
		return true;
	}

	if (PerformAction_PhaseOut()) {
		return true;
	}

	return idAI::CheckActions();
}

/*
================
rvMonsterPokemonIronMaiden::PhaseOut
================
*/
void rvMonsterPokemonIronMaiden::PhaseOut(void) {
	if (phaseTime) {
		return;
	}

	rvClientEffect* effect;
	effect = PlayEffect("fx_phase", animator.GetJointHandle("cog_bone"));
	if (effect) {
		effect->Unbind();
	}

	Hide();

	move.fl.allowHiddenMove = true;

	ProcessEvent(&EV_BecomeNonSolid);

	StopMove(MOVE_STATUS_DONE);
	SetState("State_Phased");

	// Move away from here, to anywhere
	MoveOutOfRange(this, 500.0f, 150.0f);

	SetShaderParm(5, MS2SEC(gameLocal.time));

	phaseTime = gameLocal.time;
}

/*
================
rvMonsterPokemonIronMaiden::PhaseIn
================
*/
void rvMonsterPokemonIronMaiden::PhaseIn(void) {
	if (!phaseTime) {
		return;
	}

	rvClientEffect* effect;
	effect = PlayEffect("fx_phase", animator.GetJointHandle("cog_bone"));
	if (effect) {
		effect->Unbind();
	}

	if (enemy.ent) {
		TurnToward(enemy.lastKnownPosition);
	}

	ProcessEvent(&AI_BecomeSolid);

	Show();

	//	PlayEffect ( "fx_dress", animator.GetJointHandle ( "cog_bone" ), true );

	phaseTime = 0;
	// Wait for the action that started the phase in to finish, then go back to combat
	SetState("Wait_Action");
	PostState("State_Combat");

	SetShaderParm(5, MS2SEC(gameLocal.time));
}

/*
================
rvMonsterPokemonIronMaiden::OnDeath
================
*/
void rvMonsterPokemonIronMaiden::OnDeath(void) {
	StopSound(SND_CHANNEL_ITEM, false);

	// Stop looping effects
	StopEffect("fx_banshee");
	StopEffect("fx_dress");

	idAI::OnDeath();
}

/*
================
rvMonsterPokemonIronMaiden::GetDebugInfo
================
*/
void rvMonsterPokemonIronMaiden::GetDebugInfo(debugInfoProc_t proc, void* userData) {
	// Base class first
	idAI::GetDebugInfo(proc, userData);

	proc("idAI", "action_BansheeAttack", aiActionStatusString[actionBansheeAttack.status], userData);
}

/*
===============================================================================

	States

===============================================================================
*/

CLASS_STATES_DECLARATION(rvMonsterPokemonIronMaiden)
STATE("State_Phased", rvMonsterPokemonIronMaiden::State_Phased)

STATE("Torso_PhaseOut", rvMonsterPokemonIronMaiden::State_Torso_PhaseOut)
STATE("Torso_PhaseIn", rvMonsterPokemonIronMaiden::State_Torso_PhaseIn)
STATE("Torso_BansheeAttack", rvMonsterPokemonIronMaiden::State_Torso_BansheeAttack)
STATE("Torso_BansheeAttackDone", rvMonsterPokemonIronMaiden::State_Torso_BansheeAttackDone)

STATE("Frame_PhaseIn", rvMonsterPokemonIronMaiden::Frame_PhaseIn)
STATE("Frame_PhaseOut", rvMonsterPokemonIronMaiden::Frame_PhaseOut)
STATE("Frame_BansheeAttack", rvMonsterPokemonIronMaiden::Frame_BansheeAttack)
STATE("Frame_EndBansheeAttack", rvMonsterPokemonIronMaiden::Frame_EndBansheeAttack)
END_CLASS_STATES

/*
================
rvMonsterPokemonIronMaiden::State_Phased
================
*/
stateResult_t rvMonsterPokemonIronMaiden::State_Phased(const stateParms_t& parms) {
	// If done moving and cant become solid here, move again
	if (move.fl.done) {
		if (!CanBecomeSolid()) {
			MoveOutOfRange(this, 300.0f, 150.0f);
		}
	}

	// Keep the enemy status up to date
	if (!enemy.ent) {
		CheckForEnemy(true);
	}

	// Make sure we keep facing our enemy
	if (enemy.ent) {
		TurnToward(enemy.lastKnownPosition);
	}

	// Make sure we are checking actions if we have no tactical move
	UpdateAction();

	return SRESULT_WAIT;
}

/*
================
rvMonsterPokemonIronMaiden::State_Torso_PhaseIn
================
*/
stateResult_t rvMonsterPokemonIronMaiden::State_Torso_PhaseIn(const stateParms_t& parms) {
	enum {
		STAGE_ANIM,
		STAGE_ANIM_WAIT,
		STAGE_PHASE,
	};
	switch (parms.stage) {
	case STAGE_ANIM:
		if (enemy.ent) {
			TurnToward(enemy.lastKnownPosition);
		}
		DisableAnimState(ANIMCHANNEL_LEGS);
		PlayAnim(ANIMCHANNEL_TORSO, "phase_in", parms.blendFrames);
		return SRESULT_STAGE(STAGE_ANIM_WAIT);

	case STAGE_ANIM_WAIT:
		if (AnimDone(ANIMCHANNEL_TORSO, parms.blendFrames)) {
			return SRESULT_DONE;
		}
		return SRESULT_WAIT;
	}
	return SRESULT_ERROR;
}

/*
================
rvMonsterPokemonIronMaiden::State_Torso_PhaseOut
================
*/
stateResult_t rvMonsterPokemonIronMaiden::State_Torso_PhaseOut(const stateParms_t& parms) {
	enum {
		STAGE_ANIM,
		STAGE_ANIM_WAIT,
	};
	switch (parms.stage) {
	case STAGE_ANIM:
		DisableAnimState(ANIMCHANNEL_LEGS);
		PlayAnim(ANIMCHANNEL_TORSO, "phase_out", parms.blendFrames);
		return SRESULT_STAGE(STAGE_ANIM_WAIT);

	case STAGE_ANIM_WAIT:
		if (AnimDone(ANIMCHANNEL_TORSO, parms.blendFrames)) {
			return SRESULT_DONE;
		}
		return SRESULT_WAIT;
	}
	return SRESULT_ERROR;
}

/*
================
rvMonsterPokemonIronMaiden::State_Torso_BansheeAttack
================
*/
stateResult_t rvMonsterPokemonIronMaiden::State_Torso_BansheeAttack(const stateParms_t& parms) {
	enum {
		STAGE_ATTACK,
		STAGE_ATTACK_WAIT,
	};
	switch (parms.stage) {
	case STAGE_ATTACK:
		PlayAnim(ANIMCHANNEL_TORSO, "banshee", parms.blendFrames);
		PostAnimState(ANIMCHANNEL_TORSO, "Torso_BansheeAttackDone", 0, 0, SFLAG_ONCLEAR);
		return SRESULT_STAGE(STAGE_ATTACK_WAIT);

	case STAGE_ATTACK_WAIT:
		if (enemy.ent) {
			TurnToward(enemy.lastKnownPosition);
		}
		if (AnimDone(ANIMCHANNEL_TORSO, parms.blendFrames)) {
			return SRESULT_DONE;
		}
		return SRESULT_WAIT;
	}
	return SRESULT_ERROR;
}


/*
================
rvMonsterPokemonIronMaiden::State_Torso_BansheeAttackDone

To ensure the movement is enabled after the banshee attack and that the effect is stopped this state
will be posted to be run after the banshe attack finishes.
================
*/
stateResult_t rvMonsterPokemonIronMaiden::State_Torso_BansheeAttackDone(const stateParms_t& parms) {
	Frame_EndBansheeAttack(parms);
	return SRESULT_DONE;
}

/*
================
rvMonsterPokemonIronMaiden::Frame_PhaseIn
================
*/
stateResult_t rvMonsterPokemonIronMaiden::Frame_PhaseIn(const stateParms_t& parms) {
	PhaseIn();
	return SRESULT_OK;
}


/*
================
rvMonsterPokemonIronMaiden::Frame_PhaseOut
================
*/
stateResult_t rvMonsterPokemonIronMaiden::Frame_PhaseOut(const stateParms_t& parms) {
	PhaseOut();
	return SRESULT_OK;
}

/*
================
rvMonsterPokemonIronMaiden::Frame_BansheeAttack
================
*/
stateResult_t rvMonsterPokemonIronMaiden::Frame_BansheeAttack(const stateParms_t& parms) {
	idVec3		origin;
	idMat3		axis;
	idEntity* entities[1024];
	int			count;
	int			i;

	// Get mouth origin
	GetJointWorldTransform(jointBansheeAttack, gameLocal.time, origin, axis);

	// Find all entities within the banshee attacks radius 
	count = gameLocal.EntitiesWithinRadius(origin, actionBansheeAttack.maxRange, entities, 1024);
	for (i = 0; i < count; i++) {
		idEntity* ent = entities[i];
		if (!ent || ent == this) {
			continue;
		}

		// Must be an actor that takes damage to be affected		
		if (!ent->fl.takedamage || !ent->IsType(idActor::GetClassType())) {
			continue;
		}

		// Has to be within fov
		if (!CheckFOV(ent->GetEyePosition())) {
			continue;
		}

		// Do some damage		
		idVec3 dir;
		dir = origin = ent->GetEyePosition();
		dir.NormalizeFast();
		ent->Damage(this, this, dir, spawnArgs.GetString("def_banshee_damage"), 1.0f, 0);

		// Cache the last time we stunned our own enemy for the phase out
		if (ent == enemy.ent) {
			enemyStunTime = gameLocal.time;
		}
	}

	return SRESULT_OK;
}

/*
================
rvMonsterPokemonIronMaiden::Frame_EndBansheeAttack
================
*/
stateResult_t rvMonsterPokemonIronMaiden::Frame_EndBansheeAttack(const stateParms_t& parms) {
	StopEffect("fx_banshee");
	return SRESULT_OK;
}
