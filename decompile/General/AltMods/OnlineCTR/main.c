void octr_entryHook()
{
	// Until VehBirth_TeleportSelf is rewritten
	// AdvHub, replace default spawn with unused
	// start-line spawn, turn BEQ to JMP
	*(char*)0x80058072 = 0;
	
	// BOTS_Adv_AdjustDifficulty(); must be called before
	// initializing any AI, either BOTS_Driver_Init or
	// from BOTS_Driver_Convert. If AIs are wanted, then
	// put that call in OnlineInit_Drivers, but for now
	// just disable BOTS_Driver_Convert
	*(int*)0x80017318 = 0x3E00008;
	*(int*)0x8001731c = 0;
	
	// ======== Globals ============
	
	#if USE_K1 == 0
	struct OnlineCTR* octr = (struct OnlineCTR*)0x8000C000;
	#else
	octr = 0x8000C000;
	#endif
	
	// default for first LEV, before gameplay
	octr->DriverID = 0;
	octr->NumDrivers = 8;
	octr->IsBootedPS1 = 1;

	// FSM for menus
	octr->funcs[LAUNCH_ENTER_PID] = StatePS1_Launch_EnterPID;
	octr->funcs[LAUNCH_ENTER_IP] = StatePS1_Launch_EnterIP;
	octr->funcs[LAUNCH_CONNECT_FAILED] = StatePS1_Launch_ConnectFailed;
	octr->funcs[LAUNCH_FIRST_INIT] = StatePS1_Launch_FirstInit;
	octr->funcs[LOBBY_ASSIGN_ROLE] = StatePS1_Lobby_AssignRole;
	octr->funcs[LOBBY_HOST_TRACK_PICK] = StatePS1_Lobby_HostTrackPick;
	octr->funcs[LOBBY_GUEST_TRACK_WAIT] = StatePS1_Lobby_GuestTrackWait;
	octr->funcs[LOBBY_CHARACTER_PICK] = StatePS1_Lobby_CharacterPick;
	octr->funcs[LOBBY_WAIT_FOR_LOADING] = StatePS1_Lobby_WaitForLoading;
	octr->funcs[LOBBY_START_LOADING] = StatePS1_Lobby_StartLoading;
	octr->funcs[GAME_WAIT_FOR_RACE] = StatePS1_Game_WaitForRace;
	octr->funcs[GAME_START_RACE] = StatePS1_Game_StartRace;
}

void ThreadFunc()
{
	int i;
	
	#if USE_K1 == 0
	struct OnlineCTR* octr = (struct OnlineCTR*)0x8000C000;
	#endif
	
	// only disable for no$psx testing,
	// which can force in-game with 8000c000=LOBBY_START_LOADING
	#if 1
	for(i = 3; i >= 0; i--)
		octr->time[i+1] = octr->time[i];
	
	for(i = 3; i >= 0; i--)
		if(octr->time[i+1] != octr->time[i])
			break;
	
	// if client didn't update the game in 4 frames
	int boolCloseClient = 
		(i == -1) &&
		(octr->CurrState >= LAUNCH_FIRST_INIT);
		
	// if server disconnects mid-game
	// (currState < 0)
	
	if(boolCloseClient || (octr->CurrState < 0))
	{
		// reset, including CurrState
		memset(octr, 0, sizeof(struct OnlineCTR));
		
		// Draw Black Screen
		DECOMP_CTR_ErrorScreen(0,0,0);
		
		sdata->ptrActiveMenu = 0;
		octr_entryHook();
		
		// go back to empty black screen
		sdata->gGT->levelID = 0x32;
		
		// stop music, 
		// stop "most FX", let menu FX ring
		Music_Stop();
		howl_StopAudio(1,1,0);
	
		// load next level
		sdata->gGT->gameMode1 = 0x40000000;
		sdata->Loading.stage = 0;
		return;
	}
	#endif
	
	if (octr->CurrState <= LOBBY_WAIT_FOR_LOADING)
	{
		void PrintTimeStamp();
		PrintTimeStamp();
	}
	
	if (octr->CurrState >= GAME_WAIT_FOR_RACE)
	{
		void DrawOverheadNames();
		DrawOverheadNames();
	}
	
	if (octr->CurrState >= 0)
		octr->funcs[octr->CurrState]();
}

// this runs after the end of MainInit_FinalizeInit,
// which is also after the end of camera initialization
void octr_initHook()
{
	#if USE_K1 == 0
	struct OnlineCTR* octr = (struct OnlineCTR*)0x8000C000;
	#endif
	
	// small stack pool, pause thread (those threads can't pause)
	PROC_BirthWithObject(0x310, ThreadFunc, 0, 0);
	
	sdata->lngStrings[0x17D] = "OnlineCTR";
	sdata->lngStrings[0x4e] = "OnlineCTR";
	
	struct GameTracker* gGT = sdata->gGT;
	if(gGT->levelID <= TURBO_TRACK)
	{
		DECOMP_CAM_StartOfRace(&gGT->cameraDC[0]);
		gGT->gameMode1 |= START_OF_RACE;
		gGT->hudFlags &= ~(1);
	}
}

// replace MainInit_Drivers
void OnlineInit_Drivers(struct GameTracker* gGT)
{	
	int i;
	int bitFlag;
	struct Driver* dr;

	#if USE_K1 == 0
	struct OnlineCTR* octr = (struct OnlineCTR*)0x8000C000;
	#endif

	for(i = 0; i < 8; i++)
	{
		gGT->drivers[i] = 0;
		sdata->kartSpawnOrderArray[i] = i;
	}
	
	// if first boot
	if(gGT->levelID == CREDITS_POLAR)
	{
		gGT->drivers[0] = VehBirth_Player(0);
		return;
	}
	
	if(DECOMP_LOAD_IsOpen_RacingOrBattle())
		DECOMP_RB_MinePool_Init();
	
	// 8 spawn positions
	bitFlag = 7;
	
	// override for battle maps
	if(
		(gGT->levelID > TURBO_TRACK) &&
		(gGT->levelID < GEM_STONE_VALLEY)
	  )
	{
		// 4 spawn positions
		bitFlag = 3;
	}
	
	// All clients must spawn drivers in the same order,
	// so that pointers can be sent over network
	
	for(i = 0; i < octr->DriverID; i++)
	{
		// init, save, fakeID, teleport, realID
		dr = DECOMP_VehBirth_Player(i+1);
		gGT->drivers[i+1] = dr;
		dr->driverID = i&bitFlag;
		VehBirth_TeleportSelf(dr,3,0);
		dr->driverID = i+1;
		
		#ifdef USE_60FPS
		// needed cause VehBirth_TeleportSelf
		// has not been rewritten yet for decomp
		dr->instSelf->animFrame = FPS_DOUBLE(10);
		#endif
	}
	
	// init, save, fakeID, teleport, realID
	dr = DECOMP_VehBirth_Player(0);
	gGT->drivers[0] = dr;
	dr->driverID = i&bitFlag;
	VehBirth_TeleportSelf(dr,3,0);
	dr->driverID = 0;
	
	#ifdef USE_60FPS
	// needed cause VehBirth_TeleportSelf
	// has not been rewritten yet for decomp
	dr->instSelf->animFrame = FPS_DOUBLE(10);
	#endif
	
	for(i = i+1; i < octr->NumDrivers; i++)
	{
		// init, save, fakeID, teleport, realID
		dr = DECOMP_VehBirth_Player(i);
		gGT->drivers[i] = dr;
		dr->driverID = i&bitFlag;
		VehBirth_TeleportSelf(dr,3,0);
		dr->driverID = i;
		
		#ifdef USE_60FPS
		// needed cause VehBirth_TeleportSelf
		// has not been rewritten yet for decomp
		dr->instSelf->animFrame = FPS_DOUBLE(10);
		#endif
	}
	
	octr->CurrState = GAME_WAIT_FOR_RACE;
}

void OnlineEndOfRace()
{
	struct Driver* d = sdata->gGT->drivers[0];
	
	// if "you" are still racing, do nothing
	if((d->actionsFlagSet & 0x2000000) == 0)
		return;
	
	// if "you" finished race,
	DECOMP_DecalFont_DrawLine("FINISHED!", 0x100, 206, FONT_SMALL, JUSTIFY_CENTER|ORANGE);
}

void Online_OtherFX_RecycleNew(
		u_int* soundID_Count, u_int newSoundID, u_int modifyFlags)
{
	struct Driver* d = sdata->gGT->drivers[0];
	
	if(
		((int)soundID_Count != (int)&d->driverAudioPtrs[0]) &&
		((int)soundID_Count != (int)&d->driverAudioPtrs[1]) &&
		((int)soundID_Count != (int)&d->driverAudioPtrs[2]) &&
		((int)soundID_Count != (int)&d->driverAudioPtrs[3])
	  )
	{
		// For now, mute everyone who is not Player1.
		// In the future, modify Vol/LR
		return;
	}
	
	int local = *soundID_Count;
	
    if (
		// if this sound is already playing
		(local != 0) &&

        // if soundID doesn't match new ID
        ((local & 0xffff) != newSoundID)
	   )
    {
        OtherFX_Stop1(local);
        
		*soundID_Count = 0;
		local = 0;
    }

    if (newSoundID != -1)
    {
        // if this is a new sound
        if (local == 0)
        {
            *soundID_Count = 
				OtherFX_Play_LowLevel(newSoundID & 0xffff, 0, modifyFlags);
        }
        // if not a new sound,
        // modification of old sound
        else
        {
            OtherFX_Modify(local, modifyFlags);
        }
    }
}

struct MyData
{
	short World_posX;
	short World_posY;
	short World_posZ;
	short World_posW;
	
	short Screen_posX;
	short Screen_posY;
	
	int Screen_posZ;
};

void RunPerspOnDriver(struct Driver* d)
{
	struct MyData* ptrDest = (struct MyData*)0x1f800108;
	ptrDest->World_posX = d->posCurr[0] >> 8;
	ptrDest->World_posY = (d->posCurr[1] >> 8) + 0x19;
	ptrDest->World_posZ = d->posCurr[2] >> 8;
	ptrDest->World_posW = 0;
	
	gte_ldv0(&ptrDest->World_posX);

	gte_rtps();
	gte_stsxy(&ptrDest->Screen_posX);
	gte_stsz(&ptrDest->Screen_posZ);
}

void DrawOverheadNames()
{
	int i;
	MATRIX* m;

	struct GameTracker* gGT = sdata->gGT;
	struct MyData* ptrDest = (struct MyData*)0x1f800108;
	
	#if USE_K1 == 0
	struct OnlineCTR* octr = (struct OnlineCTR*)0x8000C000;
	#endif

	// pushBuffer offset 0x28
	m = &sdata->gGT->pushBuffer[0].matrix_ViewProj;
    gte_SetRotMatrix(m);
    gte_SetTransMatrix(m);
	
	RunPerspOnDriver(gGT->drivers[0]);

	int p1z = ptrDest->Screen_posZ;

	for(i = 1; i < octr->NumDrivers; i++)
	{
		RunPerspOnDriver(gGT->drivers[i]);
		
		if(ptrDest->Screen_posZ < (p1z))
			continue;
		
		if(ptrDest->Screen_posZ > (p1z*5))
			continue;

		DecalFont_DrawLine(
			&octr->nameBuffer[i * 0xC],
			ptrDest->Screen_posX, 
			ptrDest->Screen_posY-0x4, 
			FONT_SMALL, (JUSTIFY_CENTER | ORANGE));
	}
}