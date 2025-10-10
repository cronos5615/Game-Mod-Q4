#include "../../idlib/precompiled.h"
#pragma hdrstop

#include "../Game_local.h"
#include "../Weapon.h"

class WeaponCritBow : public rvWeapon {
	CLASS_PROTOTYPE(WeaponCritBow);


	WeaponCritBow(void);
	
	virtual void		Spawn(void);
	void				Save(idSaveGame* savefile) const;
	void				Restore(idRestoreGame* savefile);
	void				PreSave(void);
	void				PostSave(void);

	protected:

		bool				UpdateAttack(void);

	private:

		int					chargeTime;
		int					chargeDelay;
		bool				fireForced;
		int					fireHeldTime;
		bool				zoomed;
		float				extra_crit_dmg = 2.00;
		float				bow_capable_mult = 0.5;
		float				charged_mult = 1.00;

		stateResult_t		State_Raise(const stateParms_t& parms);
		stateResult_t		State_Lower(const stateParms_t& parms);
		stateResult_t		State_Idle(const stateParms_t& parms);
		stateResult_t		State_Charge(const stateParms_t& parms);
		stateResult_t		State_Charged(const stateParms_t& parms);
		stateResult_t		State_Fire(const stateParms_t& parms);
		stateResult_t		State_Flashlight(const stateParms_t& parms);

		CLASS_STATES_PROTOTYPE(WeaponCritBow);
};

CLASS_DECLARATION(rvWeapon, WeaponCritBow)
END_CLASS

WeaponCritBow::WeaponCritBow(void) {
}

bool WeaponCritBow::UpdateAttack(void) {
	// Clear fire forced
	if (fireForced) {
		if (!wsfl.attack) {
			fireForced = false;
		}
		else {
			return false;
		}
	}

	// If the player is pressing the fire button and they have enough ammo for a shot
	// then start the shooting process.
	if (wsfl.attack && gameLocal.time >= nextAttackTime) {
		// Save the time which the fire button was pressed
		if (fireHeldTime == 0) {
			nextAttackTime = gameLocal.time + (fireRate * owner->PowerUpModifier(PMOD_FIRERATE));
			fireHeldTime = gameLocal.time;
		}
	}

	// If they have the charge mod and they have overcome the initial charge 
	// delay then transition to the charge state.
	if (fireHeldTime != 0) {
		if (gameLocal.time - fireHeldTime > chargeDelay) {
			SetState("Charge", 4);
			return true;
		}

		// If the fire button was let go but was pressed at one point then 
		// release the shot.
		if (!wsfl.attack) {
			idPlayer* player = gameLocal.GetLocalPlayer();
			if (player) {

				if (player->GuiActive()) {
					//make sure the player isn't looking at a gui first
					SetState("Lower", 0);
				}
				else {
					SetState("Fire", 0);
				}
			}
			return true;
		}
	}

	return false;
}

void WeaponCritBow::Spawn(void) {
	SetState("Raise", 0);

	chargeTime = SEC2MS(spawnArgs.GetFloat("chargeTime"));
	chargeDelay = SEC2MS(spawnArgs.GetFloat("chargeDelay"));

	fireHeldTime = 0;
	fireForced = false;
}

void WeaponCritBow::Save(idSaveGame* savefile) const {
	savefile->WriteInt(chargeTime);
	savefile->WriteInt(chargeDelay);
	savefile->WriteBool(fireForced);
	savefile->WriteInt(fireHeldTime);
	
}


void WeaponCritBow::Restore(idRestoreGame* savefile) {
	savefile->ReadInt(chargeTime);
	savefile->ReadInt(chargeDelay);
	savefile->ReadBool(fireForced);
	savefile->ReadInt(fireHeldTime);

}

void WeaponCritBow::PreSave(void) {
	SetState("Idle", 4);
}

void WeaponCritBow::PostSave(void) {
}

CLASS_STATES_DECLARATION(WeaponCritBow)
STATE("Raise", WeaponCritBow::State_Raise)
STATE("Lower", WeaponCritBow::State_Lower)
STATE("Idle", WeaponCritBow::State_Idle)
STATE("Charge", WeaponCritBow::State_Charge)
STATE("Charged", WeaponCritBow::State_Charged)
STATE("Fire", WeaponCritBow::State_Fire)
END_CLASS_STATES

stateResult_t WeaponCritBow::State_Raise(const stateParms_t& parms) {
	enum {
		RAISE_INIT,
		RAISE_WAIT,
	};
	switch (parms.stage) {
	case RAISE_INIT:
		SetStatus(WP_RISING);
		PlayAnim(ANIMCHANNEL_ALL, "raise", parms.blendFrames);
		return SRESULT_STAGE(RAISE_WAIT);

	case RAISE_WAIT:
		if (AnimDone(ANIMCHANNEL_ALL, 4)) {
			SetState("Idle", 4);
			return SRESULT_DONE;
		}
		if (wsfl.lowerWeapon) {
			SetState("Lower", 4);
			return SRESULT_DONE;
		}
		return SRESULT_WAIT;
	}
	return SRESULT_ERROR;
}

stateResult_t WeaponCritBow::State_Lower(const stateParms_t& parms) {
	enum {
		LOWER_INIT,
		LOWER_WAIT,
		LOWER_WAITRAISE
	};
	switch (parms.stage) {
	case LOWER_INIT:
		SetStatus(WP_LOWERING);
		PlayAnim(ANIMCHANNEL_ALL, "putaway", parms.blendFrames);
		return SRESULT_STAGE(LOWER_WAIT);

	case LOWER_WAIT:
		if (AnimDone(ANIMCHANNEL_ALL, 0)) {
			SetStatus(WP_HOLSTERED);
			return SRESULT_STAGE(LOWER_WAITRAISE);
		}
		return SRESULT_WAIT;

	case LOWER_WAITRAISE:
		if (wsfl.raiseWeapon) {
			SetState("Raise", 0);
			return SRESULT_DONE;
		}
		return SRESULT_WAIT;
	}
	return SRESULT_ERROR;
}

stateResult_t WeaponCritBow::State_Idle(const stateParms_t& parms) {
	enum {
		IDLE_INIT,
		IDLE_WAIT,
	};
	switch (parms.stage) {
	case IDLE_INIT:
		SetStatus(WP_READY);
		PlayCycle(ANIMCHANNEL_ALL, "idle", parms.blendFrames);
		return SRESULT_STAGE(IDLE_WAIT);

	case IDLE_WAIT:
		if (wsfl.lowerWeapon) {
			SetState("Lower", 4);
			return SRESULT_DONE;
		}
		
		if (UpdateAttack()) {
			return SRESULT_DONE;
		}
		return SRESULT_WAIT;
	}
	return SRESULT_ERROR;
}

stateResult_t WeaponCritBow::State_Charge(const stateParms_t& parms) {
	enum {
		CHARGE_INIT,
		CHARGE_WAIT,
	};
	switch (parms.stage) {
	case CHARGE_INIT:
		StartSound("snd_charge", SND_CHANNEL_ITEM, 0, false, NULL);
		PlayCycle(ANIMCHANNEL_ALL, "charging", parms.blendFrames);
		return SRESULT_STAGE(CHARGE_WAIT);

	case CHARGE_WAIT:
		if (gameLocal.time - fireHeldTime < chargeTime) {

			if (!wsfl.attack) {
				SetState("Fire", 0);
				return SRESULT_DONE;
			}

			return SRESULT_WAIT;
		}
		SetState("Charged", 4);
		return SRESULT_DONE;
	}
	return SRESULT_ERROR;
}
stateResult_t WeaponCritBow::State_Charged(const stateParms_t& parms) {
		enum {
			CHARGED_INIT,
			CHARGED_WAIT,
		};
		switch (parms.stage) {
		case CHARGED_INIT:

			StopSound(SND_CHANNEL_ITEM, false);
			StartSound("snd_charge_loop", SND_CHANNEL_ITEM, 0, false, NULL);
			StartSound("snd_charge_click", SND_CHANNEL_BODY, 0, false, NULL);
			return SRESULT_STAGE(CHARGED_WAIT);

		case CHARGED_WAIT:
			if (!wsfl.attack) {
				fireForced = true;
				SetState("Fire", 0);
				return SRESULT_DONE;
			}
			return SRESULT_WAIT;
		}
		return SRESULT_ERROR;
	}
stateResult_t WeaponCritBow::State_Fire(const stateParms_t& parms) {
	enum {
		FIRE_INIT,
		FIRE_WAIT,
	};
	float r = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
	bool crit = r <= owner->crit_rate ? true : false;
	float total_multiplier = 1.0;
	switch (parms.stage) {
	case FIRE_INIT:

		StopSound(SND_CHANNEL_ITEM, false);
		//don't fire if we're targeting a gui.
		idPlayer* player;
		player = gameLocal.GetLocalPlayer();

		//make sure the player isn't looking at a gui first
		if (player && player->GuiActive()) {
			fireHeldTime = 0;
			SetState("Lower", 0);
			return SRESULT_DONE;
		}

		if (player && !player->CanFire()) {
			fireHeldTime = 0;
			SetState("Idle", 4);
			return SRESULT_DONE;
		}
		
		if (player->capable == idPlayer::CAPABLE::BOW) {
			total_multiplier += bow_capable_mult;
		}
		if (crit) {
			total_multiplier += player->crit_dmg + extra_crit_dmg;
		}
		//charged attack
		if (gameLocal.time - fireHeldTime > chargeTime) {
			total_multiplier += charged_mult;
			gameLocal.Printf("Firing a charged attack with multiplier %f",total_multiplier);
			Attack(true, 1, spread, 0, total_multiplier);
			PlayEffect("fx_chargedflash", barrelJointView, false);
			PlayAnim(ANIMCHANNEL_ALL, "chargedfire", parms.blendFrames);
		}
		//normal attack
		else {
			gameLocal.Printf("Firing a normal attack with multiplier %f", total_multiplier);
			Attack(false, 1, spread, 0, total_multiplier);
			PlayEffect("fx_normalflash", barrelJointView, false);
			PlayAnim(ANIMCHANNEL_ALL, "fire", parms.blendFrames);
		}
		fireHeldTime = 0;

		return SRESULT_STAGE(FIRE_WAIT);

	case FIRE_WAIT:
		if (AnimDone(ANIMCHANNEL_ALL, 4)) {
			SetState("Idle", 4);
			return SRESULT_DONE;
		}
		if (UpdateAttack()) {
			return SRESULT_DONE;
		}
		return SRESULT_WAIT;
	}
	return SRESULT_ERROR;
}