
#include "../../idlib/precompiled.h"
#pragma hdrstop

#include "../Game_local.h"

class rvMonsterPokemonGrunt : public idAI {
public:

	CLASS_PROTOTYPE(rvMonsterPokemonGrunt);

	rvMonsterPokemonGrunt(void);

	void				Spawn(void);
	void				PrintDets(void);
	void				GiveXP(int);
	void				Attack1(void);
	void				Attack2(void);
	void				Heal(void);
	void				Think(void);
	void				Save(idSaveGame* savefile) const;
	void				Restore(idRestoreGame* savefile);

	virtual void		AdjustHealthByDamage(int damage);

protected:

	rvAIAction			actionMeleeMoveAttack;
	rvAIAction			actionChaingunAttack;

	virtual bool		CheckActions(void);

	virtual void		OnTacticalChange(aiTactical_t oldTactical);
	virtual void		OnDeath(void);

private:

	int					standingMeleeNoAttackTime;
	int					rageThreshold;

	bool				waitingforattack;
	int					damagetodeal;

	void				RageStart(void);
	void				RageStop(void);

	// Torso States
	stateResult_t		State_Torso_Enrage(const stateParms_t& parms);
	stateResult_t		State_Torso_Pain(const stateParms_t& parms);
	stateResult_t		State_Torso_LeapAttack(const stateParms_t& parms);

	CLASS_STATES_PROTOTYPE(rvMonsterPokemonGrunt);
};

CLASS_DECLARATION(idAI, rvMonsterPokemonGrunt)
END_CLASS

/*
================
rvMonsterPokemonGrunt::rvMonsterPokemonGrunt
================
*/
rvMonsterPokemonGrunt::rvMonsterPokemonGrunt(void) {
	standingMeleeNoAttackTime = 0;
}

/*
================
rvMonsterPokemonGrunt::Spawn
================
*/
void rvMonsterPokemonGrunt::Spawn(void) {
	rageThreshold = spawnArgs.GetInt("health_rageThreshold");

	// Custom actions
	actionMeleeMoveAttack.Init(spawnArgs, "action_meleeMoveAttack", NULL, AIACTIONF_ATTACK);
	actionChaingunAttack.Init(spawnArgs, "action_chaingunAttack", NULL, AIACTIONF_ATTACK);
	actionLeapAttack.Init(spawnArgs, "action_leapAttack", "Torso_LeapAttack", AIACTIONF_ATTACK);
	team = gameLocal.GetLocalPlayer()->team;
	gameLocal.GetLocalPlayer()->activePokemon = this;
	xp = gameLocal.GetLocalPlayer()->pokemonArray.StackTop().xp;
	level = gameLocal.GetLocalPlayer()->pokemonArray.StackTop().level;
	xpToLevelUp = gameLocal.GetLocalPlayer()->pokemonArray.StackTop().xpToLevelUp;

	// Enraged to start?
	if (spawnArgs.GetBool("preinject")) {
		RageStart();
	}

	double pow = 1.0;
	for (int i = 1; i < level; i++) {
		pow *= 1.5;
	}
	maxHealth *= pow;
	health = maxHealth;
	gameLocal.Printf("Spawned pokemon grunt\n");
	PrintDets();
	gameLocal.GetLocalPlayer()->pokemonArray.StackPop();
}

/*
================
rvMonsterPokemonGrunt::PrintDets
================
*/
void rvMonsterPokemonGrunt::PrintDets(void) {
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
rvMonsterPokemonGrunt::GiveXP
================
*/
void rvMonsterPokemonGrunt::GiveXP(int amount) {
	int orig = amount;
	while (amount > 0) {
		if (xp + amount >= xpToLevelUp) {
			amount -= xpToLevelUp - xp;
			xp = 0;
			level++;
			xpToLevelUp *= 1.5;
			maxHealth *= 1.5;
			health = maxHealth;
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
rvMonsterPokemonGrunt::Attack1
================
*/
void rvMonsterPokemonGrunt::Attack1(void) {
	idAI* enemy = gameLocal.GetLocalPlayer()->activeEnemy;
	if (!enemy) {
		return;
	}
	if (!aifl.turn) {
		return;
	}
	TurnToward(enemy->GetEyePosition());
	PlayAnim(ANIMCHANNEL_LEGS, "melee_attack1", 4);
	waitingforattack = true;
	double pow = 1.0;
	for (int i = 1; i < level; i++) {
		pow *= 1.5;
	}
	damagetodeal = 20 * pow;
	damagetodeal += damagetodeal * amplify;
	if (!secondTurn) {
		aifl.turn = 0;
	}
}

/*
================
rvMonsterPokemonGrunt::Attack2
================
*/
void rvMonsterPokemonGrunt::Attack2(void) {
	idAI* enemy = gameLocal.GetLocalPlayer()->activeEnemy;
	if (!enemy) {
		return;
	}
	if (!aifl.turn) {
		return;
	}
	if (level < 2) {
		return;
	}
	TurnToward(enemy->GetEyePosition());
	PlayAnim(ANIMCHANNEL_LEGS, "range_attack", 4);
	waitingforattack = true;
	double pow = 1.0;
	for (int i = 1; i < level; i++) {
		pow *= 1.5;
	}
	damagetodeal = 30 * pow;
	damagetodeal += damagetodeal * amplify;
	if (!secondTurn) {
		aifl.turn = 0;
	}
}

/*
================
rvMonsterPokemonGrunt::Heal
================
*/
void rvMonsterPokemonGrunt::Heal(void) {
	idAI* enemy = gameLocal.GetLocalPlayer()->activeEnemy;
	if (!enemy) {
		return;
	}
	if (!aifl.turn) {
		return;
	}
	health += maxHealth / 2;
	if (health > maxHealth) {
		health = maxHealth;
	}
	if (!secondTurn) {
		aifl.turn = 0;
	}
	int attack = rand() % 2;
	if (!attack) {
		enemy->Attack1();
	} else {
		enemy->Attack2();
	}
}

/*
================
rvMonsterPokemonGrunt::Think
================
*/
void rvMonsterPokemonGrunt::Think(void) {
	idAI::Think();

	if (waitingforattack && AnimDone(ANIMCHANNEL_LEGS, 0)) {
		waitingforattack = false;
		damagetodeal += damagetodeal * amplify;
		idAI* enemy = gameLocal.GetLocalPlayer()->activeEnemy;
		if (enemy) {
			enemy->health -= damagetodeal;
			if (enemy->health <= 0) {
				enemy->aifl.defeated = 1;
				health = maxHealth;
				gameLocal.GetLocalPlayer()->pfl.combat = false;
				GiveXP(enemy->defeatXp);
				int randItem = rand() % 5;
				if (randItem == 0) {
					gameLocal.GetLocalPlayer()->powerHerbs++;
				}
				else if (randItem == 1) {
					gameLocal.GetLocalPlayer()->shields++;
				}
				else if (randItem == 2) {
					gameLocal.GetLocalPlayer()->blackBelts++;
				}
				else if (randItem == 3) {
					gameLocal.GetLocalPlayer()->lifeOrbs++;
				}
				else {
					gameLocal.GetLocalPlayer()->rareCandies++;
				}
			} else if (!secondTurn) {
				int attack = rand() % 2;
				if (!attack) {
					enemy->Attack1();
				}
				else {
					enemy->Attack2();
				}
			}
			secondTurn = 0;
		}
	}
	if (!waitingforattack && AnimDone(ANIMCHANNEL_LEGS, 0)) {
		PlayAnim(ANIMCHANNEL_LEGS, "idle", 4);
	}
}

/*
================
rvMonsterPokemonGrunt::Save
================
*/
void rvMonsterPokemonGrunt::Save(idSaveGame* savefile) const {
	actionMeleeMoveAttack.Save(savefile);
	actionChaingunAttack.Save(savefile);

	savefile->WriteInt(rageThreshold);
	savefile->WriteInt(standingMeleeNoAttackTime);
}

/*
================
rvMonsterPokemonGrunt::Restore
================
*/
void rvMonsterPokemonGrunt::Restore(idRestoreGame* savefile) {
	actionMeleeMoveAttack.Restore(savefile);
	actionChaingunAttack.Restore(savefile);

	savefile->ReadInt(rageThreshold);
	savefile->ReadInt(standingMeleeNoAttackTime);
}

/*
================
rvMonsterPokemonGrunt::RageStart
================
*/
void rvMonsterPokemonGrunt::RageStart(void) {
	SetShaderParm(6, 1);

	// Disable non-rage actions
	actionEvadeLeft.fl.disabled = true;
	actionEvadeRight.fl.disabled = true;

	// Speed up animations
	animator.SetPlaybackRate(1.25f);

	// Disable pain
	pain.threshold = 0;

	// Start over with health when enraged
	health = spawnArgs.GetInt("health");

	// No more going to rage
	rageThreshold = 0;
}

/*
================
rvMonsterPokemonGrunt::RageStop
================
*/
void rvMonsterPokemonGrunt::RageStop(void) {
	SetShaderParm(6, 0);
}

/*
================
rvMonsterPokemonGrunt::CheckActions
================
*/
bool rvMonsterPokemonGrunt::CheckActions(void) {
	// If our health is below the rage threshold then enrage
	if (health < rageThreshold) {
		PerformAction("Torso_Enrage", 4, true);
		return true;
	}

	// Moving melee attack?
	if (PerformAction(&actionMeleeMoveAttack, (checkAction_t)&idAI::CheckAction_MeleeAttack, NULL)) {
		return true;
	}

	// Default actions
	if (CheckPainActions()) {
		return true;
	}

	if (PerformAction(&actionEvadeLeft, (checkAction_t)&idAI::CheckAction_EvadeLeft, &actionTimerEvade) ||
		PerformAction(&actionEvadeRight, (checkAction_t)&idAI::CheckAction_EvadeRight, &actionTimerEvade) ||
		PerformAction(&actionJumpBack, (checkAction_t)&idAI::CheckAction_JumpBack, &actionTimerEvade) ||
		PerformAction(&actionLeapAttack, (checkAction_t)&idAI::CheckAction_LeapAttack)) {
		return true;
	}
	else if (PerformAction(&actionMeleeAttack, (checkAction_t)&idAI::CheckAction_MeleeAttack)) {
		standingMeleeNoAttackTime = 0;
		return true;
	}
	else {
		if (actionMeleeAttack.status != rvAIAction::STATUS_FAIL_TIMER
			&& actionMeleeAttack.status != rvAIAction::STATUS_FAIL_EXTERNALTIMER
			&& actionMeleeAttack.status != rvAIAction::STATUS_FAIL_CHANCE)
		{//melee attack fail for any reason other than timer?
			if (combat.tacticalCurrent == AITACTICAL_MELEE && !move.fl.moving)
			{//special case: we're in tactical melee and we're close enough to think we've reached the enemy, but he's just out of melee range!
				if (!standingMeleeNoAttackTime)
				{
					standingMeleeNoAttackTime = gameLocal.GetTime();
				}
				else if (standingMeleeNoAttackTime + 2500 < gameLocal.GetTime())
				{//we've been standing still and not attacking for at least 2.5 seconds, fall back to ranged attack
					//allow ranged attack
					actionRangedAttack.fl.disabled = false;
				}
			}
		}
		if (PerformAction(&actionRangedAttack, (checkAction_t)&idAI::CheckAction_RangedAttack, &actionTimerRangedAttack)) {
			return true;
		}
	}
	return false;
}

/*
================
rvMonsterPokemonGrunt::OnDeath
================
*/
void rvMonsterPokemonGrunt::OnDeath(void) {
	RageStop();
	return idAI::OnDeath();
}

/*
================
rvMonsterPokemonGrunt::OnTacticalChange

Enable/Disable the ranged attack based on whether the grunt needs it
================
*/
void rvMonsterPokemonGrunt::OnTacticalChange(aiTactical_t oldTactical) {
	switch (combat.tacticalCurrent) {
	case AITACTICAL_MELEE:
		actionRangedAttack.fl.disabled = true;
		break;

	default:
		actionRangedAttack.fl.disabled = false;
		break;
	}
}

/*
=====================
rvMonsterPokemonGrunt::AdjustHealthByDamage
=====================
*/
void rvMonsterPokemonGrunt::AdjustHealthByDamage(int damage) {
	// Take less damage during enrage process 
	if (rageThreshold && health < rageThreshold) {
		health -= (damage * 0.25f);
		return;
	}
	return idAI::AdjustHealthByDamage(damage);
}

/*
===============================================================================

	States

===============================================================================
*/

CLASS_STATES_DECLARATION(rvMonsterPokemonGrunt)
STATE("Torso_Enrage", rvMonsterPokemonGrunt::State_Torso_Enrage)
STATE("Torso_Pain", rvMonsterPokemonGrunt::State_Torso_Pain)
STATE("Torso_LeapAttack", rvMonsterPokemonGrunt::State_Torso_LeapAttack)
END_CLASS_STATES

/*
================
rvMonsterPokemonGrunt::State_Torso_Pain
================
*/
stateResult_t rvMonsterPokemonGrunt::State_Torso_Pain(const stateParms_t& parms) {
	// Stop streaming pain if its time to get angry
	if (pain.loopEndTime && health < rageThreshold) {
		pain.loopEndTime = 0;
	}
	return idAI::State_Torso_Pain(parms);
}

/*
================
rvMonsterPokemonGrunt::State_Torso_Enrage
================
*/
stateResult_t rvMonsterPokemonGrunt::State_Torso_Enrage(const stateParms_t& parms) {
	enum {
		STAGE_ANIM,
		STAGE_ANIM_WAIT,
	};
	switch (parms.stage) {
	case STAGE_ANIM:
		DisableAnimState(ANIMCHANNEL_LEGS);
		PlayAnim(ANIMCHANNEL_TORSO, "anger", parms.blendFrames);
		return SRESULT_STAGE(STAGE_ANIM_WAIT);

	case STAGE_ANIM_WAIT:
		if (AnimDone(ANIMCHANNEL_TORSO, 4)) {
			RageStart();
			return SRESULT_DONE;
		}
		return SRESULT_WAIT;
	}
	return SRESULT_ERROR;
}


/*
================
rvMonsterPokemonGrunt::State_Torso_LeapAttack
================
*/
stateResult_t rvMonsterPokemonGrunt::State_Torso_LeapAttack(const stateParms_t& parms) {
	enum {
		STAGE_ANIM,
		STAGE_ANIM_WAIT,
	};
	switch (parms.stage) {
	case STAGE_ANIM:
		DisableAnimState(ANIMCHANNEL_LEGS);
		lastAttackTime = 0;
		// Play the action animation
		PlayAnim(ANIMCHANNEL_TORSO, animator.GetAnim(actionAnimNum)->FullName(), parms.blendFrames);
		return SRESULT_STAGE(STAGE_ANIM_WAIT);

	case STAGE_ANIM_WAIT:
		if (AnimDone(ANIMCHANNEL_TORSO, parms.blendFrames)) {
			// If we missed our leap attack get angry
			if (!lastAttackTime && rageThreshold) {
				PostAnimState(ANIMCHANNEL_TORSO, "Torso_Enrage", parms.blendFrames);
			}
			return SRESULT_DONE;
		}
		return SRESULT_WAIT;
	}
	return SRESULT_ERROR;
}
