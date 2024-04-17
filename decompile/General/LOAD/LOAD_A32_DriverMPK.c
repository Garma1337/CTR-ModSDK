#include <common.h>

void DECOMP_LOAD_DriverMPK(unsigned int param_1,int levelLOD,unsigned int param_3)
{
	int i;
	int gameMode1;
		
#ifdef USE_GPU1P
	
	for(i = 0; i < 3; i++)
	{
		// high lod CTR model
		DECOMP_LOAD_AppendQueue(param_1,LT_DRAM,
			BI_RACERMODELHI + data.characterIDs[i],
			&data.driverModel_lowLOD[i],0xfffffffe);
	}

	// Tim Trial MPK
	DECOMP_LOAD_AppendQueue(param_1,LT_DRAM,
		BI_TIMETRIALPACK + data.characterIDs[i],
		0,param_3);

	return;
		
#endif
	
	struct GameTracker* gGT = sdata->gGT;
	gameMode1 = gGT->gameMode1;
	
	int lastFileIndexMPK;

	// 3P/4P
	if(levelLOD - 3U < 2)
	{
		for(i = 0; i < 3; i++)
		{
			// low lod CTR model
			DECOMP_LOAD_AppendQueue(param_1,LT_DRAM,
				BI_RACERMODELLOW + data.characterIDs[i],
				&data.driverModel_lowLOD[i],0xfffffffe);
		}

		// load 4P MPK of fourth player
		lastFileIndexMPK = BI_4PARCADEPACK + data.characterIDs[i];
	}

// Compiles 4120 (optimized to 4064 with "else")
// This is what I want, why wont it work?
#if 0
	#ifdef USE_OXIDE
	// need oxide model for character select
	else if(gGT->levelID == MAIN_MENU_LEVEL)
	{
		lastFileIndexMPK = BI_ADVENTUREPACK + 0xf;
	}
	#endif

	// adv mpk for adventure
	else if((gameMode1 & ADVENTURE_ARENA) != 0)
	{
		lastFileIndexMPK = BI_ADVENTUREPACK + data.characterIDs[0];
	}
	
	else if(
		// adv mpk when we just need text from MPK
		((gameMode1 & (GAME_CUTSCENE | MAIN_MENU)) != 0)
		||

		// credits
		((gGT->gameMode2 & CREDITS) != 0)
	  )
	{
		// penta penguin, smallest MPK
		lastFileIndexMPK = BI_ADVENTUREPACK + 0xD;
	}
	
// Compiles 4080 (optimized to 4016 with "else")
// This is what we're stuck with
#else
	else if(
		// adv mpk when we just need text from MPK
		((gameMode1 & (GAME_CUTSCENE | ADVENTURE_ARENA | MAIN_MENU)) != 0)
		||

		// credits
		((gGT->gameMode2 & CREDITS) != 0)
	  )
	{
		lastFileIndexMPK = BI_ADVENTUREPACK + data.characterIDs[0];
	}
#endif

	else if((gameMode1 & (ADVENTURE_BOSS | RELIC_RACE | TIME_TRIAL)) != 0)
	{
		// high lod model
		DECOMP_LOAD_AppendQueue(param_1,LT_DRAM,
			BI_RACERMODELHI + data.characterIDs[0],
			&data.driverModel_lowLOD[0],0xfffffffe);

		// time trial mpk
		lastFileIndexMPK = BI_TIMETRIALPACK + data.characterIDs[1];
	}

	else if(
			// If you are in Adventure cup
			((gameMode1 & ADVENTURE_CUP) != 0) &&

			// purple gem cup
			(gGT->cup.cupID == 4)
		)
	{
		data.characterIDs[1] = 0xA;
		data.characterIDs[2] = 0x9;
		data.characterIDs[3] = 0xB;
		data.characterIDs[4] = 0x8;

		// high lod model
		DECOMP_LOAD_AppendQueue(param_1,LT_DRAM,
			BI_RACERMODELHI + data.characterIDs[0],
			&data.driverModel_lowLOD[0],0xfffffffe);

		// pack of four AIs with bosses
		lastFileIndexMPK = BI_2PARCADEPACK + 7;
	}

	// any 1P mode,
	// not adv, not time trial, not gem cup, not credits
	else if(levelLOD == 1)
	{
		DECOMP_LOAD_Robots1P(data.characterIDs[0]);

		// arcade mpk
		lastFileIndexMPK = BI_1PARCADEPACK + data.characterIDs[0];
	}

	//else if(levelLOD == 2)
	else
	{
		// med models
		for(i = 0; i < 2; i++)
		{
			// med lod CTR model
			DECOMP_LOAD_AppendQueue(param_1,LT_DRAM,
				BI_RACERMODELMED + data.characterIDs[i],
				&data.driverModel_lowLOD[i],0xfffffffe);
		}

		// this also loads MPK
		DECOMP_LOAD_Robots2P(param_1, data.characterIDs[0], data.characterIDs[1], param_3);

		return;
	}

	DECOMP_LOAD_AppendQueue(
		param_1,LT_DRAM,
		lastFileIndexMPK,
		0,param_3);

	return;
}