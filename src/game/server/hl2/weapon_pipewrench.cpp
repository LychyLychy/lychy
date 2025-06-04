#include "cbase.h"
#include "basehlcombatweapon_shared.h"
#include "basebludgeonweapon.h"
#include "ai_basenpc.h"
#include "npcevent.h"
#include "in_buttons.h"

#define	WRENCH_RANGE	75.0f
#define	WRENCH_REFIRE	0.4f

class CWeaponWrench : public CBaseHLBludgeonWeapon
{
	DECLARE_CLASS(CWeaponWrench, CBaseHLBludgeonWeapon);
	DECLARE_SERVERCLASS();
	DECLARE_ACTTABLE();
	DECLARE_DATADESC();

	float		GetRange(void) OVERRIDE { return	WRENCH_RANGE; }
	float		GetFireRate(void) OVERRIDE { return	WRENCH_REFIRE; }

	void		Spawn(void) OVERRIDE;
	void		AddViewKick(void);
	float		GetDamageForActivity(Activity hitActivity) OVERRIDE;

	virtual int WeaponMeleeAttack1Condition(float flDot, float flDist);

	// Animation event
	virtual void Operator_HandleAnimEvent(animevent_t* pEvent, CBaseCombatCharacter* pOperator);

private:
	// Animation event handlers
	void HandleAnimEventMeleeHit(animevent_t* pEvent, CBaseCombatCharacter* pOperator);

	void PrimaryAttack(void) OVERRIDE;
	void SecondaryAttack(void) OVERRIDE;
	void ItemPostFrame(void) OVERRIDE;

	float GetChargeTime(void) const;
	void SetChargeTime(float chargeTime);
	void ResetChargeTime(void);
	bool IsCharged(void) const;
	float GetChargeTimeSentinel(void) const;

	float m_flChargeTime;
};

ConVar    sk_plr_dmg_wrench("sk_plr_dmg_wrench", "0");
ConVar    sk_npc_dmg_wrench("sk_npc_dmg_wrench", "0");

ConVar	  sk_pipewrench_fullcharge_time("sk_pipewrench_fullcharge_time", "0", 0, "time to max damage");
ConVar	  sk_pipewrench_fullcharge_min_time("sk_pipewrench_fullcharge_min_time", "0", 0, "Minimum time needed to super swing so we dont spam");
ConVar	  sk_pipewrench_fullcharge_damage("sk_pipewrench_fullcharge_damage", "0", 0, "max damage, should be greater than sk_plr_dmg_wrench");

LINK_ENTITY_TO_CLASS(weapon_pipewrench, CWeaponWrench);
PRECACHE_WEAPON_REGISTER(weapon_pipewrench);

acttable_t CWeaponWrench::m_acttable[] =
{
	{ ACT_MELEE_ATTACK1,	ACT_MELEE_ATTACK_SWING, true },
	{ ACT_IDLE,				ACT_IDLE_ANGRY_MELEE,	false },
	{ ACT_IDLE_ANGRY,		ACT_IDLE_ANGRY_MELEE,	false },
};

IMPLEMENT_ACTTABLE(CWeaponWrench);

IMPLEMENT_SERVERCLASS_ST(CWeaponWrench, DT_WeaponWrench)
END_SEND_TABLE();

BEGIN_DATADESC(CWeaponWrench)
	DEFINE_FIELD(m_flChargeTime, FIELD_TIME),
END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose: Spawn the weapon
//-----------------------------------------------------------------------------
void CWeaponWrench::Spawn(void)
{
	ResetChargeTime();
	BaseClass::Spawn();
}

float CWeaponWrench::GetDamageForActivity(Activity hitActivity)
{
	if ((GetOwner() != NULL) && (GetOwner()->IsPlayer()))
	{
		if (IsCharged() && hitActivity == ACT_VM_HITCENTER2)
		{
			return Lerp(RemapValClamped(gpGlobals->curtime - GetChargeTime(), 0, sk_pipewrench_fullcharge_time.GetFloat(), 0, 1), sk_plr_dmg_wrench.GetFloat(), sk_pipewrench_fullcharge_damage.GetFloat());
		}
		return sk_plr_dmg_wrench.GetFloat();
	}
	return sk_npc_dmg_wrench.GetFloat();
}

//-----------------------------------------------------------------------------
// Purpose: Add in a view kick for this weapon
//-----------------------------------------------------------------------------
void CWeaponWrench::AddViewKick(void)
{
	CBasePlayer* pPlayer = ToBasePlayer(GetOwner());

	if (pPlayer == NULL)
		return;

	QAngle punchAng;

	punchAng.x = random->RandomFloat(1.0f, 2.0f);
	punchAng.y = random->RandomFloat(-2.0f, -1.0f);
	punchAng.z = 0.0f;

	pPlayer->ViewPunch(punchAng);
}


//-----------------------------------------------------------------------------
// Attempt to lead the target (needed because citizens can't hit manhacks with the wrench!)
//-----------------------------------------------------------------------------
ConVar sk_wrench_lead_time("sk_wrench_lead_time", "0.9");

int CWeaponWrench::WeaponMeleeAttack1Condition(float flDot, float flDist)
{
	// Attempt to lead the target (needed because citizens can't hit manhacks with the wrench!)
	CAI_BaseNPC* pNPC = GetOwner()->MyNPCPointer();
	CBaseEntity* pEnemy = pNPC->GetEnemy();
	if (!pEnemy)
		return COND_NONE;

	Vector vecVelocity;
	vecVelocity = pEnemy->GetSmoothedVelocity();

	// Project where the enemy will be in a little while
	float dt = sk_wrench_lead_time.GetFloat();
	dt += random->RandomFloat(-0.3f, 0.2f);
	if (dt < 0.0f)
		dt = 0.0f;

	Vector vecExtrapolatedPos;
	VectorMA(pEnemy->WorldSpaceCenter(), dt, vecVelocity, vecExtrapolatedPos);

	Vector vecDelta;
	VectorSubtract(vecExtrapolatedPos, pNPC->WorldSpaceCenter(), vecDelta);

	if (fabs(vecDelta.z) > 70)
	{
		return COND_TOO_FAR_TO_ATTACK;
	}

	Vector vecForward = pNPC->BodyDirection2D();
	vecDelta.z = 0.0f;
	float flExtrapolatedDist = Vector2DNormalize(vecDelta.AsVector2D());
	if ((flDist > 64) && (flExtrapolatedDist > 64))
	{
		return COND_TOO_FAR_TO_ATTACK;
	}

	float flExtrapolatedDot = DotProduct2D(vecDelta.AsVector2D(), vecForward.AsVector2D());
	if ((flDot < 0.7) && (flExtrapolatedDot < 0.7))
	{
		return COND_NOT_FACING_ATTACK;
	}

	return COND_CAN_MELEE_ATTACK1;
}


//-----------------------------------------------------------------------------
// Animation event handlers
//-----------------------------------------------------------------------------
void CWeaponWrench::HandleAnimEventMeleeHit(animevent_t* pEvent, CBaseCombatCharacter* pOperator)
{
	// Trace up or down based on where the enemy is...
	// But only if we're basically facing that direction
	Vector vecDirection;
	AngleVectors(GetAbsAngles(), &vecDirection);

	CBaseEntity* pEnemy = pOperator->MyNPCPointer() ? pOperator->MyNPCPointer()->GetEnemy() : NULL;
	if (pEnemy)
	{
		Vector vecDelta;
		VectorSubtract(pEnemy->WorldSpaceCenter(), pOperator->Weapon_ShootPosition(), vecDelta);
		VectorNormalize(vecDelta);

		Vector2D vecDelta2D = vecDelta.AsVector2D();
		Vector2DNormalize(vecDelta2D);
		if (DotProduct2D(vecDelta2D, vecDirection.AsVector2D()) > 0.8f)
		{
			vecDirection = vecDelta;
		}
	}

	Vector vecEnd;
	VectorMA(pOperator->Weapon_ShootPosition(), 50, vecDirection, vecEnd);
	CBaseEntity* pHurt = pOperator->CheckTraceHullAttack(pOperator->Weapon_ShootPosition(), vecEnd,
		Vector(-16, -16, -16), Vector(36, 36, 36), sk_npc_dmg_wrench.GetFloat(), DMG_CLUB, 0.75);

	// did I hit someone?
	if (pHurt)
	{
		// play sound
		WeaponSound(MELEE_HIT);

		// Fake a trace impact, so the effects work out like a player's crowbaw
		trace_t traceHit;
		UTIL_TraceLine(pOperator->Weapon_ShootPosition(), pHurt->GetAbsOrigin(), MASK_SHOT_HULL, pOperator, COLLISION_GROUP_NONE, &traceHit);
		ImpactEffect(traceHit);
	}
	else
	{
		WeaponSound(MELEE_MISS);
	}
}


//-----------------------------------------------------------------------------
// Animation event
//-----------------------------------------------------------------------------
void CWeaponWrench::Operator_HandleAnimEvent(animevent_t* pEvent, CBaseCombatCharacter* pOperator)
{
	switch (pEvent->event)
	{
	case EVENT_WEAPON_MELEE_HIT:
		HandleAnimEventMeleeHit(pEvent, pOperator);
		break;

	default:
		BaseClass::Operator_HandleAnimEvent(pEvent, pOperator);
		break;
	}
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CWeaponWrench::SecondaryAttack(void)
{
	if (!IsCharged())
	{
		SetChargeTime(gpGlobals->curtime);
		SendWeaponAnim(ACT_VM_PULLBACK);
	}
	else if(GetActivity() == ACT_VM_PULLBACK && IsViewModelSequenceFinished())
	{
		SendWeaponAnim(ACT_VM_PULLBACK_HIGH);
	}
}

//------------------------------------------------------------------------------
// Purpose : Update weapon
//------------------------------------------------------------------------------
void CWeaponWrench::ItemPostFrame(void)
{
	CBasePlayer* pPlayer = ToBasePlayer(GetOwner());

	if (IsCharged() && gpGlobals->curtime - GetChargeTime() < sk_pipewrench_fullcharge_min_time.GetFloat())
	{
		pPlayer->m_nButtons |= IN_ATTACK2;
		pPlayer->m_afButtonReleased &= ~IN_ATTACK2;
	}
	else if (IsCharged() && pPlayer && pPlayer->m_afButtonReleased & IN_ATTACK2)
	{
		BaseClass::SecondaryAttack();
		ResetChargeTime();
	}

	BaseClass::ItemPostFrame();
}

float CWeaponWrench::GetChargeTime(void) const
{
	return m_flChargeTime;
}

void CWeaponWrench::SetChargeTime(float chargeTime)
{
	m_flChargeTime = chargeTime;
}

void CWeaponWrench::ResetChargeTime(void)
{
	SetChargeTime(GetChargeTimeSentinel());
}

bool CWeaponWrench::IsCharged(void) const 
{
	return GetChargeTime() != GetChargeTimeSentinel();
}

float CWeaponWrench::GetChargeTimeSentinel(void) const
{
	return FLT_MAX;
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CWeaponWrench::PrimaryAttack()
{
	if (IsCharged())
		return;

	BaseClass::PrimaryAttack();
}