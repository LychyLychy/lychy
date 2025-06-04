#include "cbase.h"
#include "basehlcombatweapon.h"

class CWeaponSniperrifleOp4 : public CHLMachineGun
{
	DECLARE_CLASS(CWeaponSniperrifleOp4, CHLMachineGun);
	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();

private:
	bool m_bNeedsCock;
	bool NeedsCock(void) const;
	void SetNeedsCock(bool needs);
	void Cock(void);

public:
	void ItemPostFrame(void) OVERRIDE;
	void PrimaryAttack(void) OVERRIDE;
	void SecondaryAttack(void) OVERRIDE;
	bool Reload(void) OVERRIDE;

	float GetFireRate(void) OVERRIDE;
	void WeaponIdle(void) OVERRIDE;

	void ToggleZoom(void);
	bool Holster(CBaseCombatWeapon* pSwitchingTo) OVERRIDE;

	Activity GetPrimaryAttackActivity(void) OVERRIDE;
};

LINK_ENTITY_TO_CLASS(weapon_sniperrifle, CWeaponSniperrifleOp4);
PRECACHE_WEAPON_REGISTER(weapon_sniperrifle);

IMPLEMENT_SERVERCLASS_ST(CWeaponSniperrifleOp4, DT_WeaponSniperrifleOp4)
END_SEND_TABLE()

BEGIN_DATADESC(CWeaponSniperrifleOp4)
	DEFINE_FIELD(m_bNeedsCock, FIELD_BOOLEAN),
END_DATADESC()


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponSniperrifleOp4::ItemPostFrame(void)
{
	if (NeedsCock())
	{
		Cock();
	}
	return BaseClass::ItemPostFrame();
}

bool CWeaponSniperrifleOp4::NeedsCock(void) const
{
	return m_bNeedsCock;
}

void CWeaponSniperrifleOp4::SetNeedsCock(bool needs)
{
	m_bNeedsCock = needs;
}

void CWeaponSniperrifleOp4::Cock(void)
{
	CBaseCombatCharacter* pOwner = GetOwner();

	if (!pOwner)
		return;

	SendWeaponAnim(ACT_VM_RELOAD_FINISH);
	pOwner->SetNextAttack(gpGlobals->curtime + SequenceDuration());
	m_flNextPrimaryAttack = gpGlobals->curtime + SequenceDuration();
	SetNeedsCock(false);
}

//-----------------------------------------------------------------------------
// Purpose: 
//
//
//-----------------------------------------------------------------------------
void CWeaponSniperrifleOp4::PrimaryAttack(void)
{
	BaseClass::PrimaryAttack();
	
	CBaseCombatCharacter* pOwner = GetOwner();
	if (pOwner)
		pOwner->SetNextAttack(gpGlobals->curtime + 2.0f);
	m_flNextPrimaryAttack = gpGlobals->curtime + 2.0f;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CWeaponSniperrifleOp4::Reload(void)
{
	CBasePlayer* pPlayer = ToBasePlayer(GetOwner());

	if (pPlayer && pPlayer->GetFOV() != pPlayer->GetDefaultFOV())
		SecondaryAttack();

	if (Clip1() == 0)
	{
		SetNeedsCock(true);
		return DefaultReload(GetMaxClip1(), GetMaxClip2(), ACT_VM_RELOAD_EMPTY);
	}
	return BaseClass::Reload();
}

//=========================================================
Activity CWeaponSniperrifleOp4::GetPrimaryAttackActivity(void)
{
	if (Clip1() == 0)
		return ACT_VM_PRIMARYATTACK_EMPTY;

	return BaseClass::GetPrimaryAttackActivity();
}

float CWeaponSniperrifleOp4::GetFireRate(void)
{
	return 1.0f;
}

//=========================================================
void CWeaponSniperrifleOp4::WeaponIdle(void)
{
	//Idle again if we've finished
	if (HasWeaponIdleTimeElapsed() && m_iClip1 == 0)
		SendWeaponAnim(ACT_VM_IDLE_EMPTY);
	else
		BaseClass::WeaponIdle();
}
//-----------------------------------------------------------------------------
// Purpose: 
//
//
//-----------------------------------------------------------------------------
void CWeaponSniperrifleOp4::SecondaryAttack(void)
{
	WeaponSound(SPECIAL1);
	ToggleZoom();

	m_flNextSecondaryAttack = gpGlobals->curtime + 0.5f;
}

void CWeaponSniperrifleOp4::ToggleZoom(void)
{
	CBasePlayer* pPlayer = ToBasePlayer(GetOwner());

	if (!pPlayer)
		return;

	if (pPlayer->GetFOV() == pPlayer->GetDefaultFOV())
		pPlayer->SetFOV(this, 18);
	else
		pPlayer->SetFOV(this, 0);
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CWeaponSniperrifleOp4::Holster(CBaseCombatWeapon* pSwitchingTo)
{
	CBasePlayer* pPlayer = ToBasePlayer(GetOwner());

	if (pPlayer && pPlayer->GetFOV() != pPlayer->GetDefaultFOV())
		SecondaryAttack();

	return BaseClass::Holster(pSwitchingTo);
}