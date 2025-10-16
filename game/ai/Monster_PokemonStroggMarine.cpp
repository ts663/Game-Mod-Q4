
#include "../../idlib/precompiled.h"
#pragma hdrstop

#include "../Game_local.h"


//NOTE: actually a bit of a misnomer, as all Strogg Marine types use this class now...
class rvMonsterPokemonStroggMarine : public idAI {
public:

	CLASS_PROTOTYPE(rvMonsterPokemonStroggMarine);

	rvMonsterPokemonStroggMarine(void);

	void				InitSpawnArgsVariables(void);
	void				Spawn(void);
	void				PrintDets(void);
	void				GiveXP(int);
	bool				DefeatedEnemy(void);
	void				PokemonAttackMelee(void);
	void				PokemonAttackRanged(void);
	void				Save(idSaveGame* savefile) const;
	void				Restore(idRestoreGame* savefile);

protected:

	virtual void		OnStopMoving(aiMoveCommand_t oldMoveCommand);

	virtual bool		CheckActions(void);

	int					maxShots;
	int					minShots;
	int					shots;
	int					shotsFired;

	int					fireAnimNum;
	bool				spraySideRight;
	int					sweepCount;

	bool				EnemyMovingToRight(void);

	idEntity*			lastEnemy;

private:

	void				CalculateShots(void);

	int					nextShootTime;
	int					attackRate;
	jointHandle_t		attackJoint;

	// Actions
	rvAIAction			actionStrafe;
	rvAIAction			actionCrouchRangedAttack;
	rvAIAction			actionRollAttack;
	rvAIAction			actionSprayAttack;
	rvAIAction			actionAngry;
	rvAIAction			actionReload;

	virtual bool		CheckAction_JumpBack(rvAIAction* action, int animNum);
	virtual bool		CheckAction_EvadeLeft(rvAIAction* action, int animNum);
	virtual bool		CheckAction_EvadeRight(rvAIAction* action, int animNum);
	bool				CheckAction_Strafe(rvAIAction* action, int animNum);
	virtual bool		CheckAction_RangedAttack(rvAIAction* action, int animNum);
	bool				CheckAction_CrouchRangedAttack(rvAIAction* action, int animNum);
	bool				CheckAction_RollAttack(rvAIAction* action, int animNum);
	bool				CheckAction_SprayAttack(rvAIAction* action, int animNum);
	bool				CheckAction_Angry(rvAIAction* action, int animNum);
	bool				CheckAction_Reload(rvAIAction* action, int animNum);

	stateResult_t		State_Torso_RollAttack(const stateParms_t& parms);
	stateResult_t		State_Torso_RangedAttack(const stateParms_t& parms);
	stateResult_t		State_Torso_MovingRangedAttack(const stateParms_t& parms);
	stateResult_t		State_Torso_SprayAttack(const stateParms_t& parms);

	CLASS_STATES_PROTOTYPE(rvMonsterPokemonStroggMarine);
};

CLASS_DECLARATION(idAI, rvMonsterPokemonStroggMarine)
END_CLASS

/*
================
rvMonsterPokemonStroggMarine::rvMonsterPokemonStroggMarine
================
*/
rvMonsterPokemonStroggMarine::rvMonsterPokemonStroggMarine() {
	nextShootTime = 0;
}

void rvMonsterPokemonStroggMarine::InitSpawnArgsVariables(void)
{
	maxShots = spawnArgs.GetInt("maxShots", "1");
	minShots = spawnArgs.GetInt("minShots", "1");
	attackRate = SEC2MS(spawnArgs.GetFloat("attackRate", "0.2"));
	attackJoint = animator.GetJointHandle(spawnArgs.GetString("attackJoint", "muzzle"));
	xp = 0;
	level = 1;
	xpToLevelUp = 500;
	//aifl.disableLook = true;
}
/*
================
rvMonsterPokemonStroggMarine::Spawn
================
*/
void rvMonsterPokemonStroggMarine::Spawn(void) {
	actionStrafe.Init(spawnArgs, "action_strafe", NULL, 0);
	actionCrouchRangedAttack.Init(spawnArgs, "action_crouchRangedAttack", NULL, AIACTIONF_ATTACK);
	actionRollAttack.Init(spawnArgs, "action_rollAttack", NULL, AIACTIONF_ATTACK);
	actionSprayAttack.Init(spawnArgs, "action_sprayAttack", "Torso_SprayAttack", AIACTIONF_ATTACK);
	actionAngry.Init(spawnArgs, "action_angry", NULL, 0);
	actionReload.Init(spawnArgs, "action_reload", NULL, 0);
	team = gameLocal.GetLocalPlayer()->team;

	InitSpawnArgsVariables();

	shots = 0;
	shotsFired = 0;
	gameLocal.Printf("Spawned pokemon strogg\n");
	PrintDets();
	const idDict* attackDict;
	attackDict = gameLocal.FindEntityDefDict(spawnArgs.GetString("def_attack_melee_left", false));
	/*AttackMelee("melee_left", attackDict);
	gameLocal.Printf("attacked\n");*/
}

/*
================
rvMonsterPokemonStroggMarine::PrintDets
================
*/
void rvMonsterPokemonStroggMarine::PrintDets(void) {
	gameLocal.Printf("--------------------------------------\n");
	gameLocal.Printf("%s\n", name.c_str());
	gameLocal.Printf("XP: %d\n", xp);
	gameLocal.Printf("Level: %d\n", level);
	gameLocal.Printf("XP to level up: %d\n", xpToLevelUp);
	gameLocal.Printf("--------------------------------------\n");
}

/*
================
rvMonsterPokemonStroggMarine::GiveXP
================
*/
void rvMonsterPokemonStroggMarine::GiveXP(int amount) {
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
rvMonsterPokemonStroggMarine::DefeatedEnemy
================
*/
bool rvMonsterPokemonStroggMarine::DefeatedEnemy(void) {
	if (lastEnemy) {
		if (lastEnemy->health <= 0) {
			return true;
		}
	}
	return false;
}

/*
================
rvMonsterPokemonStroggMarine::PokemonAttackMelee
================
*/
void rvMonsterPokemonStroggMarine::PokemonAttackMelee(void) {
	idEntity* potential = FindEnemy(!IsBehindCover(), 0, 0.0f);
	if (!potential) {
		return;
	}
	enemy.ent = potential;
	lastEnemy = enemy.ent;
	gameLocal.Printf("Current enemy: %s\n", enemy.ent.GetEntity()->name.c_str());
	combat.fl.ignoreEnemies = false;
	PerformAction(&actionMeleeAttack, (checkAction_t)&idAI::CheckAction_MeleeAttack);
}

/*
================
rvMonsterPokemonStroggMarine::PokemonAttackRanged
================
*/
void rvMonsterPokemonStroggMarine::PokemonAttackRanged(void) {
	/*if (level < 2) {
		return;
	}*/
	idEntity* potential = FindEnemy(!IsBehindCover(), 0, 0.0f);
	if (!potential) {
		return;
	}
	enemy.ent = potential;
	lastEnemy = enemy.ent;
	gameLocal.Printf("Current enemy: %s\n", enemy.ent.GetEntity()->name.c_str());
	combat.fl.ignoreEnemies = false;
	PerformAction(&actionRangedAttack, (checkAction_t)&idAI::CheckAction_RangedAttack, &actionTimerRangedAttack);
	if (DefeatedEnemy()) {
		lastEnemy = NULL;
		GiveXP(250);
	}
}

/*
================
rvMonsterPokemonStroggMarine::Save
================
*/
void rvMonsterPokemonStroggMarine::Save(idSaveGame* savefile) const {
	actionStrafe.Save(savefile);
	actionCrouchRangedAttack.Save(savefile);
	actionRollAttack.Save(savefile);
	actionSprayAttack.Save(savefile);
	actionAngry.Save(savefile);
	actionReload.Save(savefile);

	savefile->WriteInt(shots);
	savefile->WriteInt(shotsFired);

	savefile->WriteInt(fireAnimNum);
	savefile->WriteBool(spraySideRight);
	savefile->WriteInt(sweepCount);

	savefile->WriteInt(nextShootTime);
}

/*
================
rvMonsterPokemonStroggMarine::Restore
================
*/
void rvMonsterPokemonStroggMarine::Restore(idRestoreGame* savefile) {
	actionStrafe.Restore(savefile);
	actionCrouchRangedAttack.Restore(savefile);
	actionRollAttack.Restore(savefile);
	actionSprayAttack.Restore(savefile);
	actionAngry.Restore(savefile);
	actionReload.Restore(savefile);

	savefile->ReadInt(shots);
	savefile->ReadInt(shotsFired);

	savefile->ReadInt(fireAnimNum);
	savefile->ReadBool(spraySideRight);
	savefile->ReadInt(sweepCount);

	savefile->ReadInt(nextShootTime);

	InitSpawnArgsVariables();
}

/*
============
rvMonsterPokemonStroggMarine::OnStopMoving
============
*/
void rvMonsterPokemonStroggMarine::OnStopMoving(aiMoveCommand_t oldMoveCommand) {
	//MCG - once you get to your position, attack immediately (no pause)
	//FIXME: Restrict this some?  Not after animmoves?  Not if move was short?  Only in certain tactical states?
	if (GetEnemy())
	{
		if (combat.tacticalCurrent == AITACTICAL_HIDE)
		{
		}
		else if (combat.tacticalCurrent == AITACTICAL_MELEE)
		{
			actionMeleeAttack.timer.Clear(actionTime);
		}
		else
		{
			actionRangedAttack.timer.Clear(actionTime);
			actionTimerRangedAttack.Clear(actionTime);
			actionCrouchRangedAttack.timer.Clear(actionTime);
			actionRollAttack.timer.Clear(actionTime);
			actionSprayAttack.timer.Clear(actionTime);
		}
	}
}

/*
================
rvMonsterPokemonStroggMarine::CheckAction_JumpBack
================
*/
bool rvMonsterPokemonStroggMarine::CheckAction_JumpBack(rvAIAction* action, int animNum) {
	// Jump back after taking damage
	if (!aifl.damage && gameLocal.time - pain.lastTakenTime > 1500) {
		return false;
	}

	// enemy must be in front to jump backwards
	if (!enemy.ent || !enemy.fl.inFov || !enemy.fl.visible) {
		return false;
	}

	// Can we actually move backwards?
	if (!TestAnimMove(animNum)) {
		return false;
	}
	return true;
}

/*
================
rvMonsterPokemonStroggMarine::CheckAction_EvadeLeft
================
*/
bool rvMonsterPokemonStroggMarine::CheckAction_EvadeLeft(rvAIAction* action, int animNum) {
	if (gameLocal.time - pain.lastTakenTime > 1500) {
		if (combat.shotAtAngle >= 0 || gameLocal.time - combat.shotAtTime > 100) {
			return false;
		}
	}
	// TODO: dont evade unless it was coming from directly in front of us
	if (!TestAnimMove(animNum)) {
		return false;
	}
	return true;
}

/*
================
rvMonsterPokemonStroggMarine::CheckAction_EvadeRight
================
*/
bool rvMonsterPokemonStroggMarine::CheckAction_EvadeRight(rvAIAction* action, int animNum) {
	if (gameLocal.time - pain.lastTakenTime > 1500) {
		if (combat.shotAtAngle < 0 || gameLocal.time - combat.shotAtTime > 100) {
			return false;
		}
	}
	// TODO: Dont eveade unless it was coming from directly in front of us
	if (!TestAnimMove(animNum)) {
		return false;
	}
	return true;
}

/*
================
rvMonsterPokemonStroggMarine::CheckAction_Strafe
================
*/
bool rvMonsterPokemonStroggMarine::CheckAction_Strafe(rvAIAction* action, int animNum) {
	if (!enemy.fl.visible) {
		return false;
	}

	if (!enemy.fl.inFov) {
		return false;
	}

	if (!move.fl.done) {
		return false;
	}

	if (!TestAnimMove(animNum)) {
		//well, at least try a new attack position
		if (combat.tacticalCurrent == AITACTICAL_RANGED) {
			combat.tacticalUpdateTime = 0;
		}
		return false;
	}
	return true;
}

/*
================
rvMonsterPokemonStroggMarine::CheckAction_RangedAttack
================
*/
bool rvMonsterPokemonStroggMarine::CheckAction_RangedAttack(rvAIAction* action, int animNum) {

	if (!enemy.ent || !enemy.fl.inFov) {
		return false;
	}
	if (spawnArgs.GetBool("rangeAttackChanceInverse")
		&& enemy.range - action->minRange > gameLocal.random.RandomFloat() * (action->maxRange - action->minRange)) {
		//the father away you are, the more likely you are to not attack
		return false;
	}
	if (spawnArgs.GetBool("rangeAttackChance")
		&& enemy.range - action->minRange < gameLocal.random.RandomFloat() * (action->maxRange - action->minRange)) {
		//the closer you are, the more likely you are to not attack
		return false;
	}
	return idAI::CheckAction_RangedAttack(action, animNum);
}

/*
================
rvMonsterPokemonStroggMarine::CheckAction_CrouchRangedAttack
================
*/
bool rvMonsterPokemonStroggMarine::CheckAction_CrouchRangedAttack(rvAIAction* action, int animNum)
{
	if (!enemy.ent || !enemy.fl.inFov) {
		return false;
	}
	if (!IsEnemyRecentlyVisible() || enemy.ent->DistanceTo(enemy.lastKnownPosition) > 128.0f) {
		return false;
	}
	if (spawnArgs.GetBool("rangeAttackChanceInverse")
		&& enemy.range - action->minRange > gameLocal.random.RandomFloat() * (action->maxRange - action->minRange)) {
		//the father away you are, the more likely you are to not attack
		return false;
	}
	if (spawnArgs.GetBool("rangeAttackChance")
		&& enemy.range - action->minRange < gameLocal.random.RandomFloat() * (action->maxRange - action->minRange)) {
		//the closer you are, the more likely you are to not attack
		return false;
	}
	if (animNum != -1 && !CanHitEnemyFromAnim(animNum)) {
		return false;
	}
	return true;
}

/*
================
rvMonsterPokemonStroggMarine::CheckAction_RollAttack
================
*/
bool rvMonsterPokemonStroggMarine::CheckAction_RollAttack(rvAIAction* action, int animNum)
{
	if (!enemy.ent || !enemy.fl.inFov || !enemy.fl.visible) {
		return false;
	}
	if (!TestAnimMove(animNum)) {
		return false;
	}
	return true;
}

/*
================
rvMonsterPokemonStroggMarine::CheckAction_SprayAttack
================
*/
bool rvMonsterPokemonStroggMarine::CheckAction_SprayAttack(rvAIAction* action, int animNum)
{
	if (!enemy.ent || !enemy.fl.inFov) {
		return false;
	}
	if (!IsEnemyRecentlyVisible() || enemy.ent->DistanceTo(enemy.lastKnownPosition) > 128.0f) {
		return false;
	}
	if (GetEnemy()->GetPhysics()->GetLinearVelocity().Compare(vec3_origin))
	{//not moving
		return false;
	}
	return true;
}

/*
================
rvMonsterPokemonStroggMarine::CheckAction_Angry
================
*/
bool rvMonsterPokemonStroggMarine::CheckAction_Angry(rvAIAction* action, int animNum)
{
	if (!enemy.ent || !enemy.fl.inFov || !enemy.fl.visible) {
		return false;
	}
	return true;
}

/*
================
rvMonsterPokemonStroggMarine::CheckAction_Reload
================
*/
bool rvMonsterPokemonStroggMarine::CheckAction_Reload(rvAIAction* action, int animNum) {
	if (!enemy.ent || !enemy.fl.inFov || !enemy.fl.visible) {
		return false;
	}
	return true;
}

/*
================
rvMonsterPokemonStroggMarine::CheckActions
================
*/
bool rvMonsterPokemonStroggMarine::CheckActions(void) {

	if (idAI::CheckActions())
	{
		return true;
	}
	if (PerformAction(&actionCrouchRangedAttack, (checkAction_t)&rvMonsterPokemonStroggMarine::CheckAction_CrouchRangedAttack, &actionTimerRangedAttack) ||
		PerformAction(&actionRollAttack, (checkAction_t)&rvMonsterPokemonStroggMarine::CheckAction_RollAttack, &actionTimerRangedAttack) ||
		PerformAction(&actionSprayAttack, (checkAction_t)&rvMonsterPokemonStroggMarine::CheckAction_SprayAttack, &actionTimerRangedAttack) ||
		PerformAction(&actionStrafe, (checkAction_t)&rvMonsterPokemonStroggMarine::CheckAction_Strafe) ||
		PerformAction(&actionAngry, (checkAction_t)&rvMonsterPokemonStroggMarine::CheckAction_Angry) ||
		PerformAction(&actionReload, (checkAction_t)&rvMonsterPokemonStroggMarine::CheckAction_Reload)) {
		return true;
	}
	return false;
}

/*
===============================================================================

	States

===============================================================================
*/

CLASS_STATES_DECLARATION(rvMonsterPokemonStroggMarine)
STATE("Torso_RollAttack", rvMonsterPokemonStroggMarine::State_Torso_RollAttack)
STATE("Torso_RangedAttack", rvMonsterPokemonStroggMarine::State_Torso_RangedAttack)
STATE("Torso_MovingRangedAttack", rvMonsterPokemonStroggMarine::State_Torso_MovingRangedAttack)
STATE("Torso_SprayAttack", rvMonsterPokemonStroggMarine::State_Torso_SprayAttack)
END_CLASS_STATES

/*
================
rvMonsterPokemonStroggMarine::State_Torso_RollAttack
================
*/
stateResult_t rvMonsterPokemonStroggMarine::State_Torso_RollAttack(const stateParms_t& parms) {
	enum {
		TORSO_ROLLATTACK_ROLL,
		TORSO_ROLLATTACK_FACE,
		TORSO_ROLLATTACK_FIRE,
		TORSO_ROLLATTACK_FINISH
	};

	TurnToward(enemy.lastKnownPosition);

	switch (parms.stage) {
		// Start the roll attack animation
	case TORSO_ROLLATTACK_ROLL:
		// Full body animations
		DisableAnimState(ANIMCHANNEL_LEGS);

		// Play the roll
		PlayAnim(ANIMCHANNEL_TORSO, "dive_turn", parms.blendFrames);
		move.fl.noTurn = false;
		//FaceEnemy();
		return SRESULT_STAGE(TORSO_ROLLATTACK_FACE);

		// Wait for roll animation to finish
	case TORSO_ROLLATTACK_FACE:
		if (AnimDone(ANIMCHANNEL_LEGS, 6)) {
			return SRESULT_STAGE(TORSO_ROLLATTACK_FIRE);
		}
		return SRESULT_WAIT;

		// Play fire animation
	case TORSO_ROLLATTACK_FIRE:
		if (!enemy.ent || !enemy.fl.visible)
		{//whoops! rolled out of LOS
			return SRESULT_DONE;
		}
		if (enemy.fl.inFov)
		{
			PlayAnim(ANIMCHANNEL_TORSO, "shotgun_range_attack", parms.blendFrames);
			return SRESULT_STAGE(TORSO_ROLLATTACK_FINISH);
		}
		return SRESULT_WAIT;

		// Wait for fire animation to finish
	case TORSO_ROLLATTACK_FINISH:
		if (AnimDone(ANIMCHANNEL_TORSO, 2)) {
			return SRESULT_DONE;
		}
		return SRESULT_WAIT;
	}
	return SRESULT_ERROR;
}


/*
================
rvMonsterPokemonStroggMarine::CalculateShots
================
*/
void rvMonsterPokemonStroggMarine::CalculateShots(void) {
	// Random number of shots ( scale by aggression range)
	shots = (minShots + gameLocal.random.RandomInt(maxShots - minShots + 1)) * combat.aggressiveScale;

	// Update the firing animation playback rate
	/*
	int	animNum;
	animNum = GetAnim( ANIMCHANNEL_TORSO, fireAnim );
	if ( animNum != 0 ) {
		const idAnim* anim = GetAnimator()->GetAnim ( animNum );
		if ( anim ) {
			GetAnimator()->SetPlaybackRate ( animNum, ((float)anim->Length() * combat.aggressiveScale) / fireRate );
		}
	}
	*/
}

/*
================
rvMonsterPokemonStroggMarine::State_Torso_RangedAttack
================
*/
stateResult_t rvMonsterPokemonStroggMarine::State_Torso_RangedAttack(const stateParms_t& parms) {
	enum {
		STAGE_START,
		STAGE_START_WAIT,
		STAGE_SHOOT,
		STAGE_SHOOT_WAIT,
		STAGE_END,
		STAGE_END_WAIT,
	};
	//TurnToward(enemy.lastKnownPosition);
	switch (parms.stage) {
	case STAGE_START:
		// If moving switch to the moving ranged attack (torso only)
		if (move.fl.moving && move.fl.running && !actionRangedAttack.fl.overrideLegs && FacingIdeal()) {
			PostAnimState(ANIMCHANNEL_TORSO, "Torso_MovingRangedAttack", parms.blendFrames);
			return SRESULT_DONE;
		}

		// Full body animations						
		DisableAnimState(ANIMCHANNEL_LEGS);

		fireAnimNum = gameLocal.random.RandomInt(2) + 1;
		CalculateShots();
		shotsFired = 0;

		// Attack lead in animation?
		if (HasAnim(ANIMCHANNEL_TORSO, va("range_attack%d_start", fireAnimNum), true)) {
			PlayAnim(ANIMCHANNEL_TORSO, va("range_attack%d_start", fireAnimNum), parms.blendFrames);
			return SRESULT_STAGE(STAGE_START_WAIT);
		}

		return SRESULT_STAGE(STAGE_SHOOT);

	case STAGE_START_WAIT:
		// When the pre shooting animation is done head over to shooting
		if (AnimDone(ANIMCHANNEL_TORSO, 0)) {
			return SRESULT_STAGE(STAGE_SHOOT);
		}
		return SRESULT_WAIT;

	case STAGE_SHOOT:
		PlayAnim(ANIMCHANNEL_TORSO, va("range_attack%d_loop", fireAnimNum), 0);
		shots--;
		shotsFired++;
		return SRESULT_STAGE(STAGE_SHOOT_WAIT);

	case STAGE_SHOOT_WAIT:
		// When the shoot animation is done either play another shot animation
		// or finish up with post_shooting
		if (AnimDone(ANIMCHANNEL_TORSO, 0)) {
			if (shots <= 0) {
				return SRESULT_STAGE(STAGE_END);
			}
			// If our enemy is no longer in our fov we can stop shooting
			if (!enemy.fl.inFov && shotsFired >= minShots) {
				return SRESULT_STAGE(STAGE_END);
			}
			return SRESULT_STAGE(STAGE_SHOOT);
		}
		return SRESULT_WAIT;

	case STAGE_END:
		// Attack lead in animation?
		if (HasAnim(ANIMCHANNEL_TORSO, va("range_attack%d_end", fireAnimNum), true)) {
			PlayAnim(ANIMCHANNEL_TORSO, va("range_attack%d_end", fireAnimNum), parms.blendFrames);
			return SRESULT_STAGE(STAGE_END_WAIT);
		}
		return SRESULT_DONE;

	case STAGE_END_WAIT:
		if (AnimDone(ANIMCHANNEL_TORSO, parms.blendFrames)) {
			return SRESULT_DONE;
		}
		return SRESULT_WAIT;
	}
	return SRESULT_ERROR;
}

/*
================
rvMonsterPokemonStroggMarine::State_Torso_MovingRangedAttack
================
*/
stateResult_t rvMonsterPokemonStroggMarine::State_Torso_MovingRangedAttack(const stateParms_t& parms) {
	enum {
		STAGE_INIT,
		STAGE_SHOOT,
		STAGE_SHOOT_WAIT,
	};
	switch (parms.stage) {
	case STAGE_INIT:
		CalculateShots();
		shotsFired = 0;
		return SRESULT_STAGE(STAGE_SHOOT);

	case STAGE_SHOOT:
		shots--;
		shotsFired++;
		nextShootTime = gameLocal.GetTime() + attackRate;
		if (attackJoint != INVALID_JOINT) {
			Attack("base", attackJoint, GetEnemy());
			PlayEffect("fx_blaster_muzzleflash", attackJoint);
		}
		StartSound("snd_weapon_fire", SND_CHANNEL_WEAPON, 0, false, 0);
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
			if (shots <= 0 || (!enemy.fl.inFov && shotsFired >= minShots) || !move.fl.running || !move.fl.moving) {
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
rvMonsterPokemonStroggMarine::EnemyMovingToRight
================
*/
bool rvMonsterPokemonStroggMarine::EnemyMovingToRight(void)
{
	if (!GetEnemy())
	{
		return false;
	}
	//use their movement direction
	idVec3 dir = GetEnemy()->GetPhysics()->GetLinearVelocity();
	//flatten
	dir.z = 0;
	dir.Normalize();

	idVec3 fwd = viewAxis[0];
	idVec3 lt = viewAxis[1];

	float dot = 0.0f;
	dot = DotProduct(dir, lt);
	if (dot > 0)
	{
		return false;
	}
	else
	{
		return true;
	}
}

/*
================
rvMonsterPokemonStroggMarine::State_Torso_SprayAttack
================
*/
stateResult_t rvMonsterPokemonStroggMarine::State_Torso_SprayAttack(const stateParms_t& parms) {
	enum {
		STAGE_START,
		STAGE_SWEEP,
		STAGE_END,
		STAGE_FINISH
	};
	switch (parms.stage) {
	case STAGE_START:
		DisableAnimState(ANIMCHANNEL_LEGS);
		spraySideRight = EnemyMovingToRight();
		sweepCount = 0;
		if (spraySideRight)
		{
			PlayAnim(ANIMCHANNEL_TORSO, "sprayright_start", 0);
		}
		else
		{
			PlayAnim(ANIMCHANNEL_TORSO, "sprayleft_start", 0);
		}
		return SRESULT_STAGE(STAGE_SWEEP);

	case STAGE_SWEEP:
		if (AnimDone(ANIMCHANNEL_TORSO, 0)) {
			sweepCount++;
			if (spraySideRight)
			{
				PlayAnim(ANIMCHANNEL_TORSO, "sprayright_sweep", 0);
			}
			else
			{
				PlayAnim(ANIMCHANNEL_TORSO, "sprayleft_sweep", 0);
			}
			return SRESULT_STAGE(STAGE_END);
		}
		return SRESULT_WAIT;

	case STAGE_END:
		if (AnimDone(ANIMCHANNEL_TORSO, 0)) {
			bool curEnemyMovingRight = EnemyMovingToRight();
			if (sweepCount < 3
				&& (!gameLocal.random.RandomInt(2)
					|| (spraySideRight && !curEnemyMovingRight)
					|| (!spraySideRight && curEnemyMovingRight)))
			{
				spraySideRight = !spraySideRight;
				return SRESULT_STAGE(STAGE_SWEEP);
			}
			else
			{
				if (spraySideRight)
				{
					PlayAnim(ANIMCHANNEL_TORSO, "sprayright_end", 0);
				}
				else
				{
					PlayAnim(ANIMCHANNEL_TORSO, "sprayleft_end", 0);
				}
				return SRESULT_STAGE(STAGE_FINISH);
			}
		}
		return SRESULT_WAIT;

	case STAGE_FINISH:
		if (AnimDone(ANIMCHANNEL_TORSO, 0)) {
			return SRESULT_DONE;
		}
		return SRESULT_WAIT;
	}
	return SRESULT_ERROR;
}
