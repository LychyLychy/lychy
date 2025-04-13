//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
//
// Geiger.cpp
//
// implementation of CHudAmmo class
//
#include "cbase.h"
#include "hudelement.h"
#include "hud_macros.h"
#include "engine/IEngineSound.h"
#include "SoundEmitterSystem/isoundemittersystembase.h"
#include "iclientmode.h"
#include <vgui_controls/Controls.h>
#include <vgui_controls/Panel.h>
#include <vgui/ISurface.h>

using namespace vgui;

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

static const float TARGET_FRAMETIME = 1.0 / 60.0;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CHL1HudGeiger: public CHudElement, public vgui::Panel
{
	DECLARE_CLASS_SIMPLE( CHL1HudGeiger, vgui::Panel );
public:
	CHL1HudGeiger( const char *pElementName );
	void Init( void );
	void VidInit( void );
	bool ShouldDraw( void );
	virtual void	ApplySchemeSettings( vgui::IScheme *scheme );
	virtual void	Paint( void );
	void MsgFunc_Geiger(bf_read &msg);
	
private:
	int m_iGeigerRange;

};

DECLARE_HUDELEMENT( CHL1HudGeiger );
DECLARE_HUD_MESSAGE( CHL1HudGeiger, Geiger );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CHL1HudGeiger::CHL1HudGeiger( const char *pElementName ) :
	CHudElement( pElementName ), BaseClass( NULL, "HL1HudGeiger" )
{
	vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	SetHiddenBits( HIDEHUD_HEALTH );
	RegisterForRenderGroup("HL1");
}

void CHL1HudGeiger::ApplySchemeSettings( IScheme *scheme )
{
	BaseClass::ApplySchemeSettings( scheme );

	SetPaintBackgroundEnabled( false );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHL1HudGeiger::Init(void)
{
	HOOK_HUD_MESSAGE( CHL1HudGeiger, Geiger );

	m_iGeigerRange = 0;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHL1HudGeiger::VidInit(void)
{
	m_iGeigerRange = 0;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHL1HudGeiger::MsgFunc_Geiger(bf_read &msg)
{
	// update geiger data
	m_iGeigerRange = msg.ReadByte();
	m_iGeigerRange = m_iGeigerRange << 2;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CHL1HudGeiger::ShouldDraw( void )
{
	return ( CHudElement::ShouldDraw() && ( m_iGeigerRange < 1000 && m_iGeigerRange > 0 ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHL1HudGeiger::Paint()
{
	int pct;
	float flvol=0;
	bool highsound = false;

	// piecewise linear is better than continuous formula for this
	if (m_iGeigerRange > 800)
	{
		pct = 0;			//Msg ( "range > 800\n");
	}
	else if (m_iGeigerRange > 600)
	{
		pct = 2;
		flvol = 0.4;		//Msg ( "range > 600\n");
	}
	else if (m_iGeigerRange > 500)
	{
		pct = 4;
		flvol = 0.5;		//Msg ( "range > 500\n");
	}
	else if (m_iGeigerRange > 400)
	{
		pct = 8;
		flvol = 0.6;		//Msg ( "range > 400\n");
		highsound = true;
	}
	else if (m_iGeigerRange > 300)
	{
		pct = 8;
		flvol = 0.7;		//Msg ( "range > 300\n");
		highsound = true;
	}
	else if (m_iGeigerRange > 200)
	{
		pct = 28;
		flvol = 0.78;		//Msg ( "range > 200\n");
		highsound = true;
	}
	else if (m_iGeigerRange > 150)
	{
		pct = 40;
		flvol = 0.80;		//Msg ( "range > 150\n");
		highsound = true;
	}
	else if (m_iGeigerRange > 100)
	{
		pct = 60;
		flvol = 0.85;		//Msg ( "range > 100\n");
		highsound = true;
	}
	else if (m_iGeigerRange > 75)
	{
		pct = 80;
		flvol = 0.9;		//Msg ( "range > 75\n");
		//gflGeigerDelay = cl.time + GEIGERDELAY * 0.75;
		highsound = true;
	}
	else if (m_iGeigerRange > 50)
	{
		pct = 90;
		flvol = 0.95;		//Msg ( "range > 50\n");
	}
	else
	{
		pct = 95;
		flvol = 1.0;		//Msg ( "range < 50\n");
	}

	int n = random->RandomInt( 0, 64 );

	// Make geiger noise chance framerate independent
	float flMultiplier = 1 / ( gpGlobals->frametime / TARGET_FRAMETIME );

	// Constrain the multiplier
	if ( flMultiplier > 10 )
	{
		flMultiplier = 10;
	}
	else if ( flMultiplier < 0.1 )
	{
		flMultiplier = 0.1;
	}

	n *= flMultiplier;

	if ( n < pct )
	{
		char sz[256];
		if ( highsound )
		{
			strcpy( sz, "Geiger.BeepHigh" );
		}
		else
		{
			strcpy( sz, "Geiger.BeepLow" );
		}

		CSoundParameters params;

		if ( C_BaseEntity::GetParametersForSound( sz, params, NULL ) )
		{
			flvol = ( flvol * ( random->RandomInt( 0,127 ) ) / 255 ) + 0.25;

			CLocalPlayerFilter filter;

			EmitSound_t ep;
			ep.m_nChannel = params.channel;
			ep.m_pSoundName = params.soundname;
			ep.m_flVolume = flvol;
			ep.m_SoundLevel = params.soundlevel;
			ep.m_nPitch = params.pitch;

			C_BaseEntity::EmitSound( filter, SOUND_FROM_LOCAL_PLAYER, ep ); 
		}
	}
}
