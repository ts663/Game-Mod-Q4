
#include "../../idlib/precompiled.h"
#pragma hdrstop

#include "../Game_local.h"

class rvMonsterPokemonGunner : public idAI {
public:

	CLASS_PROTOTYPE(rvMonsterPokemonGunner);

	rvMonsterPokemonGunner(void);

	void				InitSpawnArgsVariables(void);
	void				Spawn(void);
	void				PrintDets(void);
	void				GiveXP(int);
	void				Attack1(void);
	void				Attack2(void);
	void				Attack3(void);
	void				Save(idSaveGame* savefile) const;
	void				Restore(idRestoreGame* savefile);

	// Add some dynamic externals for debugging
	virtual void		GetDebugInfo(debugInfoProc_t proc, void* userData);

	virtual bool		Pain(idEntity* inflictor, idEntity* attacker, int damage, const idVec3& dir, int location);

protected:

	int					shots;
	int					shotsFired;
	idStr				nailgunPrefix;
	int					nailgunMinShots;
	int					nailgunMaxShots;
	int					nextShootTime;
	int					attackRate;
	jointHandle_t		attackJoint;

	virtual void		OnStopMoving(aiMoveCommand_t oldMoveCommand);
	virtual bool		UpdateRunStatus(void);

	virtual int			FilterTactical(int availableTactical);

	virtual bool		CheckActions(void);
	virtual void		OnTacticalChange(aiTactical_t oldTactical);

private:

	// Actions
	rvAIAction			actionGrenadeAttack;
	rvAIAction			actionNailgunAttack;

	rvAIAction			actionSideStepLeft;
	rvAIAction			actionSideStepRight;
	rvAIActionTimer		actionTimerSideStep;

	bool				CheckAction_SideStepLeft(rvAIAction* action, int animNum);
	bool				CheckAction_SideStepRight(rvAIAction* action, int animNum);

	// Torso States
	stateResult_t		State_Torso_NailgunAttack(const stateParms_t& parms);
	stateResult_t		State_Torso_MovingRangedAttack(const stateParms_t& parms);

	CLASS_STATES_PROTOTYPE(rvMonsterPokemonGunner);
};

CLASS_DECLARATION(idAI, rvMonsterPokemonGunner)
END_CLASS

/*
================
rvMonsterPokemonGunner::rvMonsterPokemonGunner
================
*/
rvMonsterPokemonGunner::rvMonsterPokemonGunner() {
	nextShootTime = 0;
}


void rvMonsterPokemonGunner::InitSpawnArgsVariables(void)
{
	nailgunMinShots = spawnArgs.GetInt("action_nailgunAttack_minshots", "5");
	nailgunMaxShots = spawnArgs.GetInt("action_nailgunAttack_maxshots", "20");
	attackRate = SEC2MS(spawnArgs.GetFloat("attackRate", "0.3"));
	attackJoint = animator.GetJointHandle(spawnArgs.GetString("attackJoint", "muzzle"));
	xp = 0;
	level = 1;
	xpToLevelUp = 500;
}
/*
================
rvMonsterPokemonGunner::Spawn
================
*/
void rvMonsterPokemonGunner::Spawn(void) {
	actionGrenadeAttack.Init(spawnArgs, "action_grenadeAttack", NULL, AIACTIONF_ATTACK);
	actionNailgunAttack.Init(spawnArgs, "action_nailgunAttack", "Torso_NailgunAttack", AIACTIONF_ATTACK);
	actionSideStepLeft.Init(spawnArgs, "action_sideStepLeft", NULL, 0);
	actionSideStepRight.Init(spawnArgs, "action_sideStepRight", NULL, 0);
	actionTimerSideStep.Init(spawnArgs, "actionTimer_sideStep");
	team = gameLocal.GetLocalPlayer()->team;
	gameLocal.GetLocalPlayer()->activePokemon = this;

	InitSpawnArgsVariables();

	gameLocal.Printf("Spawned pokemon gunner\n");
	PrintDets();
}

/*
================
rvMonsterPokemonGunner::PrintDets
================
*/
void rvMonsterPokemonGunner::PrintDets(void) {
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
rvMonsterPokemonGunner::GiveXP
================
*/
void rvMonsterPokemonGunner::GiveXP(int amount) {
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
rvMonsterPokemonGunner::Attack1
================
*/
void rvMonsterPokemonGunner::Attack1(void) {
	PlayAnim(ANIMCHANNEL_LEGS, "melee_attack1", 4);
}

/*
================
rvMonsterPokemonGunner::Attack2
================
*/
void rvMonsterPokemonGunner::Attack2(void) {
	/*if (level < 2) {
		return;
	}*/
	PlayAnim(ANIMCHANNEL_LEGS, "range_attack1", 4);
}

/*
================
rvMonsterPokemonGunner::Attack3
================
*/
void rvMonsterPokemonGunner::Attack3(void) {
	/*if (level < 3) {
		return;
	}*/
	PlayAnim(ANIMCHANNEL_LEGS, "grenade_attack", 4);
}

/*
================
rvMonsterPokemonGunner::Save
================
*/
void rvMonsterPokemonGunner::Save(idSaveGame* savefile) const {
	actionGrenadeAttack.Save(savefile);
	actionNailgunAttack.Save(savefile);
	actionSideStepLeft.Save(savefile);
	actionSideStepRight.Save(savefile);
	actionTimerSideStep.Save(savefile);

	savefile->WriteInt(shots);
	savefile->WriteInt(shotsFired);
	savefile->WriteString(nailgunPrefix);
	savefile->WriteInt(nextShootTime);
}

/*
================
rvMonsterPokemonGunner::Restore
================
*/
void rvMonsterPokemonGunner::Restore(idRestoreGame* savefile) {
	actionGrenadeAttack.Restore(savefile);
	actionNailgunAttack.Restore(savefile);
	actionSideStepLeft.Restore(savefile);
	actionSideStepRight.Restore(savefile);
	actionTimerSideStep.Restore(savefile);

	savefile->ReadInt(shots);
	savefile->ReadInt(shotsFired);
	savefile->ReadString(nailgunPrefix);
	savefile->ReadInt(nextShootTime);

	InitSpawnArgsVariables();
}

/*
============
rvMonsterPokemonGunner::OnStopMoving
============
*/
void rvMonsterPokemonGunner::OnStopMoving(aiMoveCommand_t oldMoveCommand) {
	//MCG - once you get to your position, attack immediately (no pause)
	//FIXME: Restrict this some?  Not after animmoves?  Not if move was short?  Only in certain tactical states?
	if (GetEnemy())
	{
		if (combat.tacticalCurrent == AITACTICAL_HIDE)
		{//hiding
		}
		else if (combat.tacticalCurrent == AITACTICAL_MELEE || enemy.range <= combat.meleeRange)
		{//in melee state or in melee range
			actionMeleeAttack.timer.Clear(actionTime);
		}
		else if ((!actionNailgunAttack.timer.IsDone(actionTime) || !actionTimerRangedAttack.IsDone(actionTime))
			&& (!actionGrenadeAttack.timer.IsDone(actionTime) || !actionTimerSpecialAttack.IsDone(actionTime)))
		{//no attack is ready
			//Ready at least one of them
			if (gameLocal.random.RandomInt(3))
			{
				actionNailgunAttack.timer.Clear(actionTime);
				actionTimerRangedAttack.Clear(actionTime);
			}
			else
			{
				actionGrenadeAttack.timer.Clear(actionTime);
				actionTimerSpecialAttack.Clear(actionTime);
			}
		}
	}
}

/*
================
rvMonsterPokemonGunner::UpdateRunStatus
================
*/
bool rvMonsterPokemonGunner::UpdateRunStatus(void) {
	move.fl.idealRunning = false;

	return move.fl.running != move.fl.idealRunning;
}

/*
================
rvMonsterPokemonGunner::FilterTactical
================
*/
int rvMonsterPokemonGunner::FilterTactical(int availableTactical) {
	if (!move.fl.moving && enemy.range > combat.meleeRange)
	{//keep moving!
		if ((!actionNailgunAttack.timer.IsDone(actionTime + 500) || !actionTimerRangedAttack.IsDone(actionTime + 500))
			&& (!actionGrenadeAttack.timer.IsDone(actionTime + 500) || !actionTimerSpecialAttack.IsDone(actionTime + 500)))
		{//won't be attacking in the next 1 second
			combat.tacticalUpdateTime = 0;
			availableTactical |= (AITACTICAL_MELEE_BIT);
			if (!gameLocal.random.RandomInt(2))
			{
				availableTactical &= ~(AITACTICAL_RANGED_BITS);
			}
		}
	}

	return idAI::FilterTactical(availableTactical);
}

/*
================
rvMonsterPokemonGunner::OnTacticalChange

Enable/Disable the ranged attack based on whether the grunt needs it
================
*/
void rvMonsterPokemonGunner::OnTacticalChange(aiTactical_t oldTactical) {
	switch (combat.tacticalCurrent) {
	case AITACTICAL_MELEE:
		//walk for at least 2 seconds (default update time of 500 is too short)
		combat.tacticalUpdateTime = gameLocal.GetTime() + 2000 + gameLocal.random.RandomInt(1000);
		break;
	}
}

/*
================
rvMonsterPokemonGunner::CheckAction_SideStepLeft
================
*/
bool rvMonsterPokemonGunner::CheckAction_SideStepLeft(rvAIAction* action, int animNum) {
	if (animNum == -1) {
		return false;
	}
	idVec3 moveVec;
	TestAnimMove(animNum, NULL, &moveVec);
	//NOTE: should we care if we can't walk all the way to the left?
	int attAnimNum = -1;
	if (actionNailgunAttack.anims.Num()) {
		// Pick a random animation from the list
		attAnimNum = GetAnim(ANIMCHANNEL_TORSO, actionNailgunAttack.anims[gameLocal.random.RandomInt(actionNailgunAttack.anims.Num())]);
	}
	if (attAnimNum != -1 && !CanHitEnemyFromAnim(attAnimNum, moveVec)) {
		return false;
	}
	return true;
}

/*
================
rvMonsterPokemonGunner::CheckAction_SideStepRight
================
*/
bool rvMonsterPokemonGunner::CheckAction_SideStepRight(rvAIAction* action, int animNum) {
	if (animNum == -1) {
		return false;
	}
	idVec3 moveVec;
	TestAnimMove(animNum, NULL, &moveVec);
	//NOTE: should we care if we can't walk all the way to the right?
	int attAnimNum = -1;
	if (actionNailgunAttack.anims.Num()) {
		// Pick a random animation from the list
		attAnimNum = GetAnim(ANIMCHANNEL_TORSO, actionNailgunAttack.anims[gameLocal.random.RandomInt(actionNailgunAttack.anims.Num())]);
	}
	if (attAnimNum != -1 && !CanHitEnemyFromAnim(attAnimNum, moveVec)) {
		return false;
	}
	return true;
}

/*
================
rvMonsterPokemonGunner::CheckActions
================
*/
bool rvMonsterPokemonGunner::CheckActions(void) {
	// Fire a grenade?
	if (PerformAction(&actionGrenadeAttack, (checkAction_t)&idAI::CheckAction_RangedAttack, &actionTimerSpecialAttack) ||
		PerformAction(&actionNailgunAttack, (checkAction_t)&idAI::CheckAction_RangedAttack, &actionTimerRangedAttack)) {
		return true;
	}

	bool action = idAI::CheckActions();
	if (!action) {
		//try a strafe
		if (GetEnemy() && enemy.fl.visible && gameLocal.GetTime() - lastAttackTime > actionTimerRangedAttack.GetRate() + 1000) {
			//we can see our enemy but haven't been able to shoot him in a while...
			if (PerformAction(&actionSideStepLeft, (checkAction_t)&rvMonsterPokemonGunner::CheckAction_SideStepLeft, &actionTimerSideStep)
				|| PerformAction(&actionSideStepRight, (checkAction_t)&rvMonsterPokemonGunner::CheckAction_SideStepRight, &actionTimerSideStep)) {
				return true;
			}
		}
	}
	return action;
}

/*
=====================
rvMonsterPokemonGunner::GetDebugInfo
=====================
*/
void rvMonsterPokemonGunner::GetDebugInfo(debugInfoProc_t proc, void* userData) {
	// Base class first
	idAI::GetDebugInfo(proc, userData);

	proc("idAI", "action_grenadeAttack", aiActionStatusString[actionGrenadeAttack.status], userData);
	proc("idAI", "action_nailgunAttack", aiActionStatusString[actionNailgunAttack.status], userData);
	proc("idAI", "actionSideStepLeft", aiActionStatusString[actionSideStepLeft.status], userData);
	proc("idAI", "actionSideStepRight", aiActionStatusString[actionSideStepRight.status], userData);
}

bool rvMonsterPokemonGunner::Pain(idEntity* inflictor, idEntity* attacker, int damage, const idVec3& dir, int location) {
	actionTimerRangedAttack.Clear(actionTime);
	actionNailgunAttack.timer.Clear(actionTime);
	return (idAI::Pain(inflictor, attacker, damage, dir, location));
}
/*
===============================================================================

	States

===============================================================================
*/

CLASS_STATES_DECLARATION(rvMonsterPokemonGunner)
STATE("Torso_NailgunAttack", rvMonsterPokemonGunner::State_Torso_NailgunAttack)
STATE("Torso_MovingRangedAttack", rvMonsterPokemonGunner::State_Torso_MovingRangedAttack)
END_CLASS_STATES

/*
================
rvMonsterPokemonGunner::State_Torso_MovingRangedAttack
================
*/
stateResult_t rvMonsterPokemonGunner::State_Torso_MovingRangedAttack(const stateParms_t& parms) {
	enum {
		STAGE_INIT,
		STAGE_SHOOT,
		STAGE_SHOOT_WAIT,
	};
	switch (parms.stage) {
	case STAGE_INIT:
		shots = (gameLocal.random.RandomInt(nailgunMaxShots - nailgunMinShots) + nailgunMinShots) * combat.aggressiveScale;
		shotsFired = 0;
		return SRESULT_STAGE(STAGE_SHOOT);

	case STAGE_SHOOT:
		shots--;
		shotsFired++;
		nextShootTime = gameLocal.GetTime() + attackRate;
		if (attackJoint != INVALID_JOINT) {
			Attack("nail", attackJoint, GetEnemy());
			PlayEffect("fx_nail_flash", attackJoint);
		}
		StartSound("snd_nailgun_fire", SND_CHANNEL_WEAPON, 0, false, 0);
		/*
		switch ( move.currentDirection )
		{
		case MOVEDIR_RIGHT:
			PlayAnim ( ANIMCHANNEL_TORSO, "range_attack_torso_right", 0 );
			break;
		case MOVEDIR_LEFT:
			PlayAnim ( ANIMCHANNEL_TORSO, "range_attack_torso_left", 0 );
			break;
		case MOVEDIR_BACKWARD:
			PlayAnim ( ANIMCHANNEL_TORSO, "range_attack_torso_back", 0 );
			break;
		default:
			PlayAnim ( ANIMCHANNEL_TORSO, "range_attack_torso", 0 );
			break;
		}
		*/
		return SRESULT_STAGE(STAGE_SHOOT_WAIT);

	case STAGE_SHOOT_WAIT:
		// When the shoot animation is done either play another shot animation
		// or finish up with post_shooting
		if (gameLocal.GetTime() >= nextShootTime) {
			if (shots <= 0 || (!enemy.fl.inFov && shotsFired >= nailgunMinShots) || !move.fl.moving) {
				return SRESULT_DONE;
			}
			return SRESULT_STAGE(STAGE_SHOOT);
		}
		return SRESULT_WAIT;
	}
	return SRESULT_ERROR;
}


/*
================
rvMonsterPokemonGunner::State_Torso_NailgunAttack
================
*/
stateResult_t rvMonsterPokemonGunner::State_Torso_NailgunAttack(const stateParms_t& parms) {
	static const char* nailgunAnims[] = { "nailgun_short", "nailgun_long" };
	enum {
		STAGE_INIT,
		STAGE_WAITSTART,
		STAGE_LOOP,
		STAGE_WAITLOOP,
		STAGE_WAITEND
	};
	switch (parms.stage) {
	case STAGE_INIT:
		// If moving switch to the moving ranged attack (torso only)
		if (move.fl.moving && !actionNailgunAttack.fl.overrideLegs && FacingIdeal() && !gameLocal.random.RandomInt(1)) {
			PostAnimState(ANIMCHANNEL_TORSO, "Torso_MovingRangedAttack", parms.blendFrames);
			return SRESULT_DONE;
		}

		shots = (gameLocal.random.RandomInt(nailgunMaxShots - nailgunMinShots) + nailgunMinShots) * combat.aggressiveScale;
		DisableAnimState(ANIMCHANNEL_LEGS);
		shotsFired = 0;
		nailgunPrefix = nailgunAnims[shots % 2];
		if (!CanHitEnemyFromAnim(GetAnim(ANIMCHANNEL_TORSO, va("%s_loop", nailgunPrefix.c_str()))))
		{//this is hacky, but we really need to test the attack anim first since they're so different
			//can't hit with this one, just use the other one...
			nailgunPrefix = nailgunAnims[(shots + 1) % 2];
		}
		PlayAnim(ANIMCHANNEL_TORSO, va("%s_start", nailgunPrefix.c_str()), parms.blendFrames);
		return SRESULT_STAGE(STAGE_WAITSTART);

	case STAGE_WAITSTART:
		if (AnimDone(ANIMCHANNEL_TORSO, 0)) {
			return SRESULT_STAGE(STAGE_LOOP);
		}
		return SRESULT_WAIT;

	case STAGE_LOOP:
		PlayAnim(ANIMCHANNEL_TORSO, va("%s_loop", nailgunPrefix.c_str()), 0);
		shotsFired++;
		return SRESULT_STAGE(STAGE_WAITLOOP);

	case STAGE_WAITLOOP:
		if (AnimDone(ANIMCHANNEL_TORSO, 0)) {
			if (--shots <= 0 || (shotsFired >= nailgunMinShots && !enemy.fl.inFov) || aifl.damage) {
				PlayAnim(ANIMCHANNEL_TORSO, va("%s_end", nailgunPrefix.c_str()), 0);
				return SRESULT_STAGE(STAGE_WAITEND);
			}
			return SRESULT_STAGE(STAGE_LOOP);
		}
		return SRESULT_WAIT;

	case STAGE_WAITEND:
		if (AnimDone(ANIMCHANNEL_TORSO, 4)) {
			return SRESULT_DONE;
		}
		return SRESULT_WAIT;
	}
	return SRESULT_ERROR;
}
