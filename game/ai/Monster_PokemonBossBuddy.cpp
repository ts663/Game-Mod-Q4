
#include "../../idlib/precompiled.h"
#pragma hdrstop

#include "../Game_local.h"

//------------------------------------------------------------
class rvMonsterPokemonBossBuddy : public idAI
	//------------------------------------------------------------
{
public:

	CLASS_PROTOTYPE(rvMonsterPokemonBossBuddy);

	rvMonsterPokemonBossBuddy(void);

	void		Spawn(void);
	void		PrintDets(void);
	void		GiveXP(int);
	void		Attack1(void);
	void		Attack2(void);
	void		Attack3(void);
	void		Heal(void);
	void		InitSpawnArgsVariables(void);
	void		Save(idSaveGame* savefile) const;
	void		Restore(idRestoreGame* savefile);

	bool		CanTurn(void) const;

	void		Think(void);
	bool		Pain(idEntity* inflictor, idEntity* attacker, int damage, const idVec3& dir, int location);
	void		Damage(idEntity* inflictor, idEntity* attacker, const idVec3& dir, const char* damageDefName, const float damageScale, const int location);
	void		OnWakeUp(void);

	// Add some dynamic externals for debugging
	void		GetDebugInfo(debugInfoProc_t proc, void* userData);

protected:

	bool		CheckActions(void);
	//	void		PerformAction					( const char* stateName, int blendFrames, bool noPain );
	//	bool		PerformAction					( rvAIAction* action, bool (idAI::*condition)(rvAIAction*,int), rvAIActionTimer* timer );

	void		AdjustShieldState(bool becomeShielded);
	void		ReduceShields(int amount);

	int			mShots;
	int			mShields;
	int			mMaxShields;
	int			mLastDamageTime;
	int			mShieldsLastFor;		// read from def file, shouldn't need to be saved.

	bool		mIsShielded;
	bool		mRequestedZoneMove;
	bool		mRequestedRecharge;

	bool		waitingforattack;
	int			damagetodeal;

	//bool		mCanIdle;
	//bool		mChaseMode;

private:

	rvAIAction	mActionRocketAttack;
	rvAIAction	mActionSlashMoveAttack;
	rvAIAction	mActionLightningAttack;
	rvAIAction	mActionDarkMatterAttack;
	rvAIAction	mActionMeleeMoveAttack;
	rvAIAction	mActionMeleeAttack;

	rvScriptFuncUtility	mRequestRecharge;	// script to run when a projectile is launched
	rvScriptFuncUtility	mRequestZoneMove;	// script to run when he should move to the next zone

	stateResult_t		State_Torso_RocketAttack(const stateParms_t& parms);
	stateResult_t		State_Torso_SlashAttack(const stateParms_t& parms);
	stateResult_t		State_Torso_TurnRight90(const stateParms_t& parms);
	stateResult_t		State_Torso_TurnLeft90(const stateParms_t& parms);

	void				Event_RechargeShields(float amount);

	CLASS_STATES_PROTOTYPE(rvMonsterPokemonBossBuddy);
};

#define BOSS_BUDDY_MAX_SHIELDS 8500
//------------------------------------------------------------
// rvMonsterPokemonBossBuddy::rvMonsterPokemonBossBuddy
//------------------------------------------------------------
rvMonsterPokemonBossBuddy::rvMonsterPokemonBossBuddy(void)
{
	mMaxShields = BOSS_BUDDY_MAX_SHIELDS;
	mShields = mMaxShields;
	mLastDamageTime = 0;
	mIsShielded = false;
	mRequestedZoneMove = false;
	mRequestedRecharge = false;
	//	mCanIdle = false;
	//	mChaseMode = true;
}

void rvMonsterPokemonBossBuddy::InitSpawnArgsVariables(void)
{
	mShieldsLastFor = (int)(spawnArgs.GetFloat("mShieldsLastFor", "6") * 1000.0f);
	mMaxShields = BOSS_BUDDY_MAX_SHIELDS;
	xp = gameLocal.GetLocalPlayer()->pokemonArray.StackTop().xp;
	level = gameLocal.GetLocalPlayer()->pokemonArray.StackTop().level;
	xpToLevelUp = gameLocal.GetLocalPlayer()->pokemonArray.StackTop().xpToLevelUp;
}

//------------------------------------------------------------
// rvMonsterPokemonBossBuddy::Spawn
//------------------------------------------------------------
void rvMonsterPokemonBossBuddy::Spawn(void)
{
	mActionRocketAttack.Init(spawnArgs, "action_rocketAttack", NULL, AIACTIONF_ATTACK);
	mActionLightningAttack.Init(spawnArgs, "action_lightningAttack", NULL, AIACTIONF_ATTACK);
	mActionDarkMatterAttack.Init(spawnArgs, "action_dmgAttack", NULL, AIACTIONF_ATTACK);
	mActionMeleeMoveAttack.Init(spawnArgs, "action_meleeMoveAttack", NULL, AIACTIONF_ATTACK);
	mActionSlashMoveAttack.Init(spawnArgs, "action_slashMoveAttack", NULL, AIACTIONF_ATTACK);
	mActionMeleeAttack.Init(spawnArgs, "action_meleeAttack", NULL, AIACTIONF_ATTACK);
	team = gameLocal.GetLocalPlayer()->team;
	gameLocal.GetLocalPlayer()->activePokemon = this;

	InitSpawnArgsVariables();
	mShields = mMaxShields;

	const char* func;
	if (spawnArgs.GetString("requestRecharge", "", &func))
	{
		mRequestRecharge.Init(func);
	}
	if (spawnArgs.GetString("requestZoneMove", "", &func))
	{
		mRequestZoneMove.Init(func);
	}

	HideSurface("models/monsters/bossbuddy/forcefield");

	double pow = 1.0;
	for (int i = 1; i < level; i++) {
		pow *= 1.5;
	}
	maxHealth *= pow;
	health = maxHealth;
	gameLocal.Printf("Spawned pokemon boss buddy\n");
	PrintDets();
	gameLocal.GetLocalPlayer()->pokemonArray.StackPop();
}

/*
================
rvMonsterPokemonBossBuddy::PrintDets
================
*/
void rvMonsterPokemonBossBuddy::PrintDets(void) {
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
rvMonsterPokemonBossBuddy::GiveXP
================
*/
void rvMonsterPokemonBossBuddy::GiveXP(int amount) {
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
rvMonsterPokemonBossBuddy::Attack1
================
*/
void rvMonsterPokemonBossBuddy::Attack1(void) {
	idAI* enemy = gameLocal.GetLocalPlayer()->activeEnemy;
	if (!enemy) {
		return;
	}
	if (!aifl.turn) {
		return;
	}
	TurnToward(enemy->GetEyePosition());
	PlayAnim(ANIMCHANNEL_LEGS, "melee_attack", 4);
	waitingforattack = true;
	double pow = 1.0;
	for (int i = 1; i < level; i++) {
		pow *= 1.5;
	}
	damagetodeal = 50 * pow;
	damagetodeal += damagetodeal * amplify;
	aifl.turn = 0;
}

/*
================
rvMonsterPokemonBossBuddy::Attack2
================
*/
void rvMonsterPokemonBossBuddy::Attack2(void) {
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
	PlayAnim(ANIMCHANNEL_LEGS, "attack_rocket", 4);
	waitingforattack = true;
	double pow = 1.0;
	for (int i = 1; i < level; i++) {
		pow *= 1.5;
	}
	damagetodeal = 75 * pow;
	damagetodeal += damagetodeal * amplify;
	if (!secondTurn) {
		aifl.turn = 0;
	}
}

/*
================
rvMonsterPokemonBossBuddy::Attack3
================
*/
void rvMonsterPokemonBossBuddy::Attack3(void) {
	idAI* enemy = gameLocal.GetLocalPlayer()->activeEnemy;
	if (!enemy) {
		return;
	}
	if (!aifl.turn) {
		return;
	}
	if (level < 3) {
		return;
	}
	TurnToward(enemy->GetEyePosition());
	PlayAnim(ANIMCHANNEL_LEGS, "attack_lightning", 4);
	waitingforattack = true;
	double pow = 1.0;
	for (int i = 1; i < level; i++) {
		pow *= 1.5;
	}
	damagetodeal = 100 * pow;
	damagetodeal += damagetodeal * amplify;
	if (!secondTurn) {
		aifl.turn = 0;
	}
}

/*
================
rvMonsterPokemonBossBuddy::Heal
================
*/
void rvMonsterPokemonBossBuddy::Heal(void) {
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

//------------------------------------------------------------
// rvMonsterPokemonBossBuddy::Save
//------------------------------------------------------------
void rvMonsterPokemonBossBuddy::Save(idSaveGame* savefile) const
{
	savefile->WriteInt(mShots);
	savefile->WriteInt(mShields);
	savefile->WriteInt(mMaxShields);
	savefile->WriteInt(mLastDamageTime);
	savefile->WriteBool(mIsShielded);
	savefile->WriteBool(mRequestedZoneMove);
	savefile->WriteBool(mRequestedRecharge);

	mActionRocketAttack.Save(savefile);
	mActionLightningAttack.Save(savefile);
	mActionDarkMatterAttack.Save(savefile);
	mActionMeleeMoveAttack.Save(savefile);
	mActionMeleeAttack.Save(savefile);
	mActionSlashMoveAttack.Save(savefile);

	mRequestRecharge.Save(savefile);
	mRequestZoneMove.Save(savefile);
}

//------------------------------------------------------------
// rvMonsterPokemonBossBuddy::Restore
//------------------------------------------------------------
void rvMonsterPokemonBossBuddy::Restore(idRestoreGame* savefile)
{
	savefile->ReadInt(mShots);
	savefile->ReadInt(mShields);
	savefile->ReadInt(mMaxShields);
	savefile->ReadInt(mLastDamageTime);
	savefile->ReadBool(mIsShielded);
	savefile->ReadBool(mRequestedZoneMove);
	savefile->ReadBool(mRequestedRecharge);

	mActionRocketAttack.Restore(savefile);
	mActionLightningAttack.Restore(savefile);
	mActionDarkMatterAttack.Restore(savefile);
	mActionMeleeMoveAttack.Restore(savefile);
	mActionMeleeAttack.Restore(savefile);
	mActionSlashMoveAttack.Restore(savefile);

	mRequestRecharge.Restore(savefile);
	mRequestZoneMove.Restore(savefile);

	InitSpawnArgsVariables();
}

//------------------------------------------------------------
// rvMonsterBerserker::GetDebugInfo
//------------------------------------------------------------
void rvMonsterPokemonBossBuddy::GetDebugInfo(debugInfoProc_t proc, void* userData)
{
	// Base class first
	idAI::GetDebugInfo(proc, userData);

	proc("idAI", "action_darkMatterAttack", aiActionStatusString[mActionDarkMatterAttack.status], userData);
	proc("idAI", "action_rocketAttack", aiActionStatusString[mActionRocketAttack.status], userData);
	proc("idAI", "action_meleeMoveAttack", aiActionStatusString[mActionMeleeMoveAttack.status], userData);
	proc("idAI", "action_lightningAttack", aiActionStatusString[mActionLightningAttack.status], userData);
}

//--------------------------------------------------------------
// Custom Script Events
//--------------------------------------------------------------
const idEventDef EV_RechargeShields("rechargeShields", "f", 'f');

CLASS_DECLARATION(idAI, rvMonsterPokemonBossBuddy)
EVENT(EV_RechargeShields, rvMonsterPokemonBossBuddy::Event_RechargeShields)
END_CLASS

//------------------------------------------------------------
// rvMonsterPokemonBossBuddy::Event_RechargeShields
//------------------------------------------------------------
void rvMonsterPokemonBossBuddy::Event_RechargeShields(float amount)
{
	mShields += (int)amount;

	if (mShields >= mMaxShields)
	{
		// charge is done
		mShields = mMaxShields;
		idThread::ReturnInt(0);

		// reset request states
		mRequestedRecharge = false;
		mRequestedZoneMove = false;

		// shield warning no longer neede for now
		gameLocal.GetLocalPlayer()->hud->HandleNamedEvent("hideBossShieldWarn");
	}
	else
	{
		// still charging
		idThread::ReturnInt(1);
	}
}

//------------------------------------------------------------
// rvMonsterPokemonBossBuddy::ReduceShields
//------------------------------------------------------------
void rvMonsterPokemonBossBuddy::ReduceShields(int amount)
{
	mShields -= amount;

	// if no mShields left... or the last time we took damage was more than 8 seconds ago
	if (mShields <= 0 || (mLastDamageTime + 8000) < gameLocal.time)
	{
		//....remove the shielding
		AdjustShieldState(false);
	}

	if (mShields < 1000)
	{
		if (!mRequestedRecharge)
		{
			// entering a dangerous state!  Get to the recharge station, fast!
			gameLocal.GetLocalPlayer()->hud->HandleNamedEvent("showBossShieldWarn");
			ExecScriptFunction(mRequestRecharge);
			mRequestedRecharge = true;
		}
	}
	else if (mShields < 4000)
	{
		if (!mRequestedZoneMove)
		{
			// Getting low, so move him close to the next zone so he can be ready to recharge 
			ExecScriptFunction(mRequestZoneMove);
			mRequestedZoneMove = true;
		}
	}
}

//------------------------------------------------------------
// rvMonsterPokemonBossBuddy::AdjustShieldState
//------------------------------------------------------------
void rvMonsterPokemonBossBuddy::AdjustShieldState(bool becomeShielded)
{
	// only do the work for adjusting the state when it doesn't match our current state
	if (!mIsShielded && becomeShielded)
	{
		// Activate Shields!
		ShowSurface("models/monsters/bossbuddy/forcefield");
		StartSound("snd_enable_shields", SND_CHANNEL_ANY, 0, false, NULL);
		gameLocal.GetLocalPlayer()->hud->HandleNamedEvent("showBossShieldBar");
	}
	else if (mIsShielded && !becomeShielded)
	{
		// Deactivate Shields!
		HideSurface("models/monsters/bossbuddy/forcefield");
		StartSound("snd_disable_shields", SND_CHANNEL_ANY, 0, false, NULL);
		//		gameLocal.GetLocalPlayer()->hud->HandleNamedEvent( "hideBossShieldBar" );
	}
	mIsShielded = becomeShielded;
}

//------------------------------------------------------------
// rvMonsterPokemonBossBuddy::Damage
//------------------------------------------------------------
void rvMonsterPokemonBossBuddy::Think()
{
	if (!fl.hidden && !fl.isDormant && (thinkFlags & TH_THINK) && !aifl.dead)
	{
		// run simple shielding logic when we have them active
		if (mIsShielded)
		{
			ReduceShields(1);

			// if they are on but we haven't taken damage in x seconds, turn them off to conserve on shields
			if ((mLastDamageTime + mShieldsLastFor) < gameLocal.time)
			{
				AdjustShieldState(false);
			}
		}

		// update shield bar
		idUserInterface* hud = gameLocal.GetLocalPlayer()->hud;
		if (hud)
		{
			float percent = ((float)mShields / mMaxShields);

			hud->SetStateFloat("boss_shield_percent", percent);
			hud->HandleNamedEvent("updateBossShield");
		}
	}

	if (move.obstacle.GetEntity())
	{
		PerformAction(&mActionSlashMoveAttack, (checkAction_t)&idAI::CheckAction_MeleeAttack, &actionTimerSpecialAttack);
	}

	idAI::Think();

	if (waitingforattack && AnimDone(ANIMCHANNEL_LEGS, 0)) {
		waitingforattack = false;
		amplify = 0;
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
//------------------------------------------------------------
// rvMonsterPokemonBossBuddy::PerformAction
//------------------------------------------------------------
void rvMonsterPokemonBossBuddy::PerformAction( const char* stateName, int blendFrames, bool noPain )
{
	// Allow movement in actions
	move.fl.allowAnimMove = true;

	if ( mChaseMode )
	{
		return;
	}

	// Start the action
	SetAnimState( ANIMCHANNEL_TORSO, stateName, blendFrames );

	// Always call finish action when the action is done, it will clear the action flag
	aifl.action = true;
	PostAnimState( ANIMCHANNEL_TORSO, "Torso_FinishAction", 0, 0, SFLAG_ONCLEAR );

	// Go back to idle when done-- sometimes.
	if ( mCanIdle )
	{
		PostAnimState( ANIMCHANNEL_TORSO, "Torso_Idle", blendFrames );
	}

	// Main state will wait until action is finished before continuing
	InterruptState( "Wait_ActionNoPain" );
	OnStartAction( );
}

//------------------------------------------------------------
// rvMonsterPokemonBossBuddy::PerformAction
//------------------------------------------------------------
bool rvMonsterPokemonBossBuddy::PerformAction( rvAIAction* action, bool (idAI::*condition)(rvAIAction*,int), rvAIActionTimer* timer )
{
	if ( mChaseMode )
	{
		return false;
	}

	return idAI::PerformAction( action, condition ,timer );
}
*/
//------------------------------------------------------------
// rvMonsterPokemonBossBuddy::Damage
//------------------------------------------------------------
void rvMonsterPokemonBossBuddy::Damage(idEntity* inflictor, idEntity* attacker, const idVec3& dir,
	const char* damageDefName, const float damageScale, const int location)
{
	// get damage amount so we can decay the shields and check for ignoreShields
	const idDict* damageDef = gameLocal.FindEntityDefDict(damageDefName, false);
	if (!damageDef)
	{
		gameLocal.Error("Unknown damageDef '%s'\n", damageDefName);
	}

	// NOTE: there is a damage def for the electrocution that is marked 'ignoreShields'.. when present on the damage def,
	//	we don't run shielding logic
	bool directDamage = damageDef->GetBool("ignoreShields");

	int loc = location;
	if (directDamage)
	{
		// Lame, I know, but hack the location
		loc = INVALID_JOINT;
	}
	else if (attacker == this)
	{
		// can't damage self
		return;
	}

	float scale = 1;

	// Shields will activate for a set amount of time when damage is being taken
	mLastDamageTime = gameLocal.time;

	// if shields are active, we should try to 'eat' them before directing damage to the BB
	if (mIsShielded && !directDamage)
	{
		// BB is resistant to any kind of splash damage when the shields are up
		if (loc <= INVALID_JOINT)
		{
			// damage must have been done by splash damage
			return;
		}

		int	damage = damageDef->GetInt("damage");
		ReduceShields(damage * 8);

		// Shielding dramatically reduces actual damage done to BB
		scale = 0.1f;
	}
	else if (mShields > 0) // not currently shielded...does he have shields to use?
	{
		// Yep, so turn them on
		AdjustShieldState(true);
	}

	idAI::Damage(inflictor, attacker, dir, damageDefName, damageScale * scale, loc);
}

//------------------------------------------------------------
// rvMonsterPokemonBossBuddy::Pain
//------------------------------------------------------------
bool rvMonsterPokemonBossBuddy::Pain(idEntity* inflictor, idEntity* attacker, int damage, const idVec3& dir, int location)
{
	// immune to small damage.  Is this safe to do?
	if (damage > 5)
	{
		rvClientCrawlEffect* effect;
		effect = new rvClientCrawlEffect(gameLocal.GetEffect(spawnArgs, "fx_shieldcrawl"), this, SEC2MS(spawnArgs.GetFloat("shieldCrawlTime", ".2")));
		effect->Play(gameLocal.time, false);

		return idAI::Pain(inflictor, attacker, damage, dir, location);
	}
	return false;
}

//------------------------------------------------------------
// rvMonsterPokemonBossBuddy::CheckActions
//------------------------------------------------------------
bool rvMonsterPokemonBossBuddy::CheckActions(void)
{
	// If not moving, try turning in place
/*	if ( !move.fl.moving && gameLocal.time > combat.investigateTime )
	{
		float turnYaw = idMath::AngleNormalize180( move.ideal_yaw - move.current_yaw );
		if ( turnYaw > lookMax[YAW] * 0.75f )
		{
			PerformAction( "Torso_TurnRight90", 4, true );
			return true;
		} else if ( turnYaw < -lookMax[YAW] * 0.75f )
		{
			PerformAction( "Torso_TurnLeft90", 4, true );
			return true;
		}
	}
*/
	if (PerformAction(&mActionMeleeMoveAttack, (checkAction_t)&idAI::CheckAction_MeleeAttack, NULL) ||
		PerformAction(&mActionSlashMoveAttack, (checkAction_t)&idAI::CheckAction_MeleeAttack, &actionTimerSpecialAttack))
	{
		return true;
	}

	if (PerformAction(&mActionDarkMatterAttack, (checkAction_t)&idAI::CheckAction_RangedAttack, &actionTimerRangedAttack) ||
		PerformAction(&mActionRocketAttack, (checkAction_t)&idAI::CheckAction_RangedAttack, &actionTimerRangedAttack) ||
		PerformAction(&mActionLightningAttack, (checkAction_t)&idAI::CheckAction_RangedAttack, &actionTimerRangedAttack))
	{
		return true;
	}

	return idAI::CheckActions();
}

//------------------------------------------------------------
// rvMonsterPokemonBossBuddy::CanTurn
//------------------------------------------------------------
bool rvMonsterPokemonBossBuddy::CanTurn(void) const
{
	/* 	if ( !idAI::CanTurn ( ) ) {
			return false;
		}
		return move.anim_turn_angles != 0.0f || move.fl.moving;
	*/
	return idAI::CanTurn();
}

//------------------------------------------------------------
// rvMonsterPokemonBossBuddy::OnWakeUp
//------------------------------------------------------------
void rvMonsterPokemonBossBuddy::OnWakeUp(void)
{
	mActionDarkMatterAttack.timer.Reset(actionTime, mActionDarkMatterAttack.diversity);
	mActionRocketAttack.timer.Reset(actionTime, mActionDarkMatterAttack.diversity);
	idAI::OnWakeUp();
}

//------------------------------------------------------------
//	States 
//------------------------------------------------------------

CLASS_STATES_DECLARATION(rvMonsterPokemonBossBuddy)
STATE("Torso_RocketAttack", rvMonsterPokemonBossBuddy::State_Torso_RocketAttack)
STATE("Torso_SlashAttack", rvMonsterPokemonBossBuddy::State_Torso_SlashAttack)
STATE("Torso_TurnRight90", rvMonsterPokemonBossBuddy::State_Torso_TurnRight90)
STATE("Torso_TurnLeft90", rvMonsterPokemonBossBuddy::State_Torso_TurnLeft90)
END_CLASS_STATES

//------------------------------------------------------------
// rvMonsterPokemonBossBuddy::State_Torso_RocketAttack
//------------------------------------------------------------
stateResult_t rvMonsterPokemonBossBuddy::State_Torso_RocketAttack(const stateParms_t& parms)
{
	enum
	{
		STAGE_INIT,
		STAGE_WAITSTART,
		STAGE_LOOP,
		STAGE_WAITLOOP,
		STAGE_WAITEND
	};
	switch (parms.stage)
	{
	case STAGE_INIT:
		DisableAnimState(ANIMCHANNEL_LEGS);
		PlayAnim(ANIMCHANNEL_TORSO, "attack_rocket2start", parms.blendFrames);
		mShots = (gameLocal.random.RandomInt(3) + 2) * combat.aggressiveScale;
		return SRESULT_STAGE(STAGE_WAITSTART);

	case STAGE_WAITSTART:
		if (AnimDone(ANIMCHANNEL_TORSO, 0))
		{
			return SRESULT_STAGE(STAGE_LOOP);
		}
		return SRESULT_WAIT;

	case STAGE_LOOP:
		PlayAnim(ANIMCHANNEL_TORSO, "attack_rocket2loop2", 0);
		return SRESULT_STAGE(STAGE_WAITLOOP);

	case STAGE_WAITLOOP:
		if (AnimDone(ANIMCHANNEL_TORSO, 0))
		{
			if (--mShots <= 0 ||										// exhausted mShots? .. or
				(!IsEnemyVisible() && rvRandom::irand(0, 10) >= 8) ||	// ... player is no longer visible .. or
				(enemy.ent && DistanceTo(enemy.ent) < 256)) 		// ... player is so close, we prolly want to do a melee attack
			{
				PlayAnim(ANIMCHANNEL_TORSO, "attack_rocket2end", 0);
				return SRESULT_STAGE(STAGE_WAITEND);
			}
			return SRESULT_STAGE(STAGE_LOOP);
		}
		return SRESULT_WAIT;

	case STAGE_WAITEND:
		if (AnimDone(ANIMCHANNEL_TORSO, 4))
		{
			return SRESULT_DONE;
		}
		return SRESULT_WAIT;
	}
	return SRESULT_ERROR;
}

//------------------------------------------------------------
// rvMonsterPokemonBossBuddy::State_Torso_SlashAttack
//------------------------------------------------------------
stateResult_t rvMonsterPokemonBossBuddy::State_Torso_SlashAttack(const stateParms_t& parms)
{
	enum
	{
		STAGE_INIT,
		STAGE_WAIT_FIRST_SWIPE,
		STAGE_WAIT_FINISH
	};
	switch (parms.stage)
	{
	case STAGE_INIT:
		DisableAnimState(ANIMCHANNEL_LEGS);
		PlayAnim(ANIMCHANNEL_TORSO, "melee_move_attack", parms.blendFrames);
		return SRESULT_STAGE(STAGE_WAIT_FIRST_SWIPE);

	case STAGE_WAIT_FIRST_SWIPE:
		if (AnimDone(ANIMCHANNEL_TORSO, 0))
		{
			PlayAnim(ANIMCHANNEL_TORSO, "melee_move_attack", parms.blendFrames);
			return SRESULT_STAGE(STAGE_WAIT_FINISH);
		}
		return SRESULT_WAIT;

	case STAGE_WAIT_FINISH:
		if (AnimDone(ANIMCHANNEL_TORSO, 0))
		{
			return SRESULT_DONE;
		}
		return SRESULT_WAIT;
	}
	return SRESULT_ERROR;
}

//------------------------------------------------------------
// rvMonsterPokemonBossBuddy::State_Torso_TurnRight90
//------------------------------------------------------------
stateResult_t rvMonsterPokemonBossBuddy::State_Torso_TurnRight90(const stateParms_t& parms)
{
	enum
	{
		STAGE_INIT,
		STAGE_WAIT
	};
	switch (parms.stage)
	{
	case STAGE_INIT:
		DisableAnimState(ANIMCHANNEL_LEGS);
		PlayAnim(ANIMCHANNEL_TORSO, "turn_right", parms.blendFrames);
		AnimTurn(90.0f, true);
		return SRESULT_STAGE(STAGE_WAIT);

	case STAGE_WAIT:
		if (move.fl.moving || AnimDone(ANIMCHANNEL_TORSO, 0))
		{
			AnimTurn(0, true);
			combat.investigateTime = gameLocal.time + 250;
			return SRESULT_DONE;
		}
		return SRESULT_WAIT;
	}
	return SRESULT_ERROR;
}

//------------------------------------------------------------
// rvMonsterPokemonBossBuddy::State_Torso_TurnLeft90
//------------------------------------------------------------
stateResult_t rvMonsterPokemonBossBuddy::State_Torso_TurnLeft90(const stateParms_t& parms)
{
	enum
	{
		STAGE_INIT,
		STAGE_WAIT
	};
	switch (parms.stage)
	{
	case STAGE_INIT:
		DisableAnimState(ANIMCHANNEL_LEGS);
		PlayAnim(ANIMCHANNEL_TORSO, "turn_left", parms.blendFrames);
		AnimTurn(90.0f, true);
		return SRESULT_STAGE(STAGE_WAIT);

	case STAGE_WAIT:
		if (move.fl.moving || AnimDone(ANIMCHANNEL_TORSO, 0))
		{
			AnimTurn(0, true);
			combat.investigateTime = gameLocal.time + 250;
			return SRESULT_DONE;
		}
		return SRESULT_WAIT;
	}
	return SRESULT_ERROR;
}
