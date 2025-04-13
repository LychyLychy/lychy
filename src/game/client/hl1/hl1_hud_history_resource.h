#ifndef HL1_HUD_HISTORY_RESOURCE_H
#define HL1_HUD_HISTORY_RESOURCE_H
#pragma once

class CHL1HudHistoryResource : public CHudHistoryResource
{
	DECLARE_CLASS_SIMPLE(CHL1HudHistoryResource, CHudHistoryResource);

	CHL1HudHistoryResource(const char* pElementName);

	virtual void Init(void);
	virtual void Reset(void);
	virtual bool ShouldDraw(void);
	virtual void Paint(void);

	virtual void ApplySchemeSettings(vgui::IScheme* pScheme);

	virtual void	AddToHistory(int iType, int iId, int iCount = 0);
	virtual void	AddToHistory(int iType, const char* szName, int iCount = 0);
	virtual void	AddToHistory(C_BaseCombatWeapon* weapon);
	virtual void	MsgFunc_ItemPickup(bf_read& msg);

	virtual void	CheckClearHistory(void);
	virtual void	SetHistoryGap(int iNewHistoryGap);
};

#endif //HL1_HUD_HISTORY_RESOURCE_H