#include "common.h"
#include "utils/utils.h"
#include "kz.h"
#include "utils/simplecmds.h"

#include "checkpoint/kz_checkpoint.h"
#include "jumpstats/kz_jumpstats.h"
#include "quiet/kz_quiet.h"
#include "mode/kz_mode.h"
#include "style/kz_style.h"
#include "noclip/kz_noclip.h"
#include "hud/kz_hud.h"
#include "spec/kz_spec.h"
#include "timer/kz_timer.h"
#include "tip/kz_tip.h"

internal SCMD_CALLBACK(Command_KzHidelegs)
{
	KZPlayer *player = g_pKZPlayerManager->ToPlayer(controller);
	player->ToggleHideLegs();
	player->PrintChat(true, false, "{grey}Player legs are now %s.", player->HidingLegs() ? "hidden" : "shown");
	return MRES_SUPERCEDE;
}

internal SCMD_CALLBACK(Command_KzHide)
{
	KZPlayer *player = g_pKZPlayerManager->ToPlayer(controller);
	player->quietService->ToggleHide();
	player->PrintChat(true, false, "{grey}You are now %s other players.", player->quietService->hideOtherPlayers ? "hiding" : "showing");
	return MRES_SUPERCEDE;
}

internal SCMD_CALLBACK(Command_KzRestart)
{
	KZPlayer *player = g_pKZPlayerManager->ToPlayer(controller);

	player->timerService->OnTeleportToStart();
	if (player->GetPawn()->IsAlive())
	{
		// Fix players spawning 500u under spawn positions.
		if (player->noclipService->IsNoclipping())
		{
			player->noclipService->DisableNoclip();
			player->noclipService->HandleNoclip();
		}
		player->GetPawn()->Respawn();
	}
	else
	{
		controller->Respawn();
	}
	if (player->checkpointService->HasCustomStartPosition())
	{
		player->checkpointService->TpToStartPosition();
	}
	return MRES_SUPERCEDE;
}

internal SCMD_CALLBACK(Command_KzHideWeapon)
{
	KZPlayer *player = g_pKZPlayerManager->ToPlayer(controller);
	player->quietService->ToggleHideWeapon();
	player->PrintChat(true, false, "{grey}You are now %s your weapons.", player->quietService->ShouldHideWeapon() ? "hiding" : "showing");
	return MRES_SUPERCEDE;
}

internal SCMD_CALLBACK(Command_JoinTeam)
{
	KZ::misc::JoinTeam(g_pKZPlayerManager->ToPlayer(controller), atoi(args->Arg(1)), false);
	return MRES_SUPERCEDE;
}

// TODO: move command registration to the service class?
void KZ::misc::RegisterCommands()
{
	scmd::RegisterCmd("kz_hidelegs", Command_KzHidelegs, "Hide your legs in first person.");
	scmd::RegisterCmd("kz_hide", Command_KzHide, "Hide other players.");
	scmd::RegisterCmd("kz_restart", Command_KzRestart, "Restart.");
	scmd::RegisterCmd("kz_r", Command_KzRestart, "Restart.");
	scmd::RegisterCmd("kz_hideweapon", Command_KzHideWeapon, "Hide weapon viewmodel.");
	scmd::RegisterCmd("jointeam", Command_JoinTeam, "Jointeam interceptor", true);
	// TODO: Fullupdate spectators on spec_mode/spec_next/spec_player/spec_prev
	KZCheckpointService::RegisterCommands();
	KZJumpstatsService::RegisterCommands();
	KZTimerService::RegisterCommands();
	KZNoclipService::RegisterCommands();
	KZHUDService::RegisterCommands();
	KZ::mode::RegisterCommands();
	KZ::style::RegisterCommands();
}

void KZ::misc::OnClientActive(CPlayerSlot slot)
{
	KZPlayer *player = g_pKZPlayerManager->ToPlayer(slot);
	player->Reset();
}

void KZ::misc::JoinTeam(KZPlayer *player, int newTeam, bool restorePos)
{
	int currentTeam = player->GetController()->GetTeam();

	// Don't use CS_TEAM_NONE
	if (newTeam == CS_TEAM_NONE)
	{
		newTeam = CS_TEAM_SPECTATOR;
	}

	if (newTeam == CS_TEAM_SPECTATOR && currentTeam != CS_TEAM_SPECTATOR)
	{
		if (currentTeam != CS_TEAM_NONE)
		{
			player->specService->SavePosition();
		}

		if (!player->timerService->GetPaused() && !player->timerService->CanPause())
		{
			player->timerService->TimerStop();
		}
		player->GetController()->ChangeTeam(CS_TEAM_SPECTATOR);
		player->quietService->SendFullUpdate();
		// TODO: put spectators of this player to freecam, and send them full updates
	}
	else if (newTeam == CS_TEAM_CT && currentTeam != CS_TEAM_CT || newTeam == CS_TEAM_T && currentTeam != CS_TEAM_T)
	{
		player->GetPawn()->CommitSuicide(false, true);
		player->GetController()->SwitchTeam(newTeam);
		player->GetController()->Respawn();
		if (restorePos && player->specService->HasSavedPosition())
		{
			player->specService->LoadPosition();
		}
		else
		{
			player->timerService->TimerStop();
			// Just joining a team alone can put you into weird invalid spawns.
			// Need to teleport the player to a valid one.
			Vector spawnOrigin;
			QAngle spawnAngles;
			utils::FindValidSpawn(spawnOrigin, spawnAngles);
			player->GetPawn()->Teleport(&spawnOrigin, &spawnAngles, &vec3_origin);
		}
		player->specService->ResetSavedPosition();
	}
}
