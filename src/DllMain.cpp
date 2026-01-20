#define NOTIFY(str, ...) \
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_GREEN | FOREGROUND_INTENSITY); \
    printf(str, __VA_ARGS__); \
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);

enum class StateDirector // redirect to what state (initially i named this guy StateDictator XDDD)
{
	driftdash,
	bouncejump, 
	jump,
	run,
	boost,
	homing,
	preserveboost,
	doublejump,
	stomp,
	falls,
};

template<typename T>
void WriteProtected(uintptr_t address, T value) {
	DWORD oldProtect;
	VirtualProtect(reinterpret_cast<void*>(address), sizeof(T), PAGE_EXECUTE_READWRITE, &oldProtect);
	*reinterpret_cast<T*>(address) = value;
	VirtualProtect(reinterpret_cast<void*>(address), sizeof(T), oldProtect, &oldProtect);
}

void tryDamage(app::player::PlayerHsmContext* ctx, bool leaveMeAlone) {
	auto* collision = ctx->gocPlayerHsm->statePluginManager->GetPlugin<app::player::StatePluginCollision>();
	auto* BBbattle = ctx->gocPlayerBlackboard->blackboard->GetContent<app::player::BlackboardBattle>();
	auto* BBstatus = ctx->blackboardStatus;
	auto* HSMComp = ctx->playerObject->GetComponent<app::player::GOCPlayerHsm>();

	static bool isInBallState = false;
	
	bool isBoosting = BBstatus->GetStateFlag(app::player::BlackboardStatus::StateFlag::BOOST);
	
	int getID = HSMComp->GetCurrentState();
	
	if (leaveMeAlone == true) {
		collision->SetTypeAndRadius(2, 0);
		BBbattle->SetFlag020(true);
		return;
	}

	if (getID == 107 || getID == 10 || getID == 11 || getID == 9 || getID == 46) {
		isInBallState = true;
	}
	else {
		isInBallState = false;
	}

	/*if (isInBallState) {
		collision->SetTypeAndRadius(2, 1);
		BBbattle->SetFlag020(false);	
		return;
	}*/

	if (isBoosting || isInBallState) {
		collision->SetTypeAndRadius(2, 1);
		BBbattle->SetFlag020(false);
		return;
	}
	if (!isBoosting && !isInBallState)
	{
		//collision->SetTypeAndRadius(1, 0);
		collision->SetTypeAndRadius(2, 0);
		BBbattle->SetFlag020(true);
		return;
	}	
	return;
}

void DDash_bounceJump_hacking(app::player::PlayerHsmContext* ctx, bool leavemealone) {
	auto* gameManager = hh::game::GameManager::GetInstance();
	auto* lvlInfo = gameManager->GetService<app::level::LevelInfo>();
	auto isGrounded = lvlInfo->playerInformation->isGrounded;

	auto* playerCommonRFL = hh::fnd::ResourceManager::GetInstance()->GetResource<hh::fnd::ResReflectionT<app::player::PlayerParameters>>("player_common");
	auto* params = playerCommonRFL->GetData();

	csl::math::Vector4 velocity_m = ctx->gocPlayerKinematicParams->velocity;
	float vMagnitude = velocity_m.y();

	if (isGrounded == 0 && leavemealone) {
		params->whiteSpace.doubleJump.bounceSpeed = vMagnitude*.5;
		params->forwardView.doubleJump.bounceSpeed = vMagnitude*.5;
		params->sideView.doubleJump.bounceSpeed = vMagnitude*.5;
		params->boss.doubleJump.bounceSpeed = vMagnitude*.5;
	}
	if (!leavemealone) {
		params->whiteSpace.doubleJump.bounceSpeed = 25.0f;
		params->forwardView.doubleJump.bounceSpeed = 25.0f;
		params->sideView.doubleJump.bounceSpeed = 25.0f;
		params->boss.doubleJump.bounceSpeed = 25.0f;
	}

}

void Redirect(app::player::PlayerHsmContext* ctx, StateDirector to_what)
{
	auto* HSMComp = ctx->playerObject->GetComponent<app::player::GOCPlayerHsm>();
	auto* GOCSound = ctx->playerObject->GetComponent<hh::snd::GOCSound>();
	switch (to_what) 
	{
		case StateDirector::driftdash:
			HSMComp->ChangeState(107, 0);
			tryDamage(ctx, false);
			NOTIFY("[Shadow Moveset]: DIRECTED TO DRIFTDASH \n");
			GOCSound->Play("sd_spin", 1);
			break;
		case StateDirector::bouncejump:
			HSMComp->ChangeState(11, 0);
			DDash_bounceJump_hacking(ctx, 1);
			tryDamage(ctx, false);
			NOTIFY("[Shadow Moveset]: DIRECTED TO BOUNCEJUMP \n");
			break;
		case StateDirector::jump:
			HSMComp->ChangeState(9,0);
			NOTIFY("[Shadow Moveset]: DIRECTED TO JUMP \n");
			break;
		case StateDirector::run:
			HSMComp->ChangeState(4, 0);
			NOTIFY("[Shadow Moveset]: DIRECTED TO RUN \n");
			break;
		case StateDirector::boost:
			HSMComp->ChangeState(4, 0);
			ctx->blackboardStatus->SetStateFlag(app::player::BlackboardStatus::StateFlag::BOOST, 1);
			tryDamage(ctx, 0);
			NOTIFY("[Shadow Moveset]: DIRECTED TO BOOST \n");
			break;
		case StateDirector::homing:
			HSMComp->ChangeState(63, 0);
			NOTIFY("[Shadow Moveset]: HOMING SHADOW \n");
			break;
		case StateDirector::preserveboost: // for boosting inside slalom, maybe others, bad implementation if being honest
			ctx->blackboardStatus->SetStateFlag(app::player::BlackboardStatus::StateFlag::BOOST, 1);
			tryDamage(ctx, false);
			NOTIFY("[Shadow Moveset]: APPLIED BOOST TO SLALOM STEP \n");
			break;
		case StateDirector::doublejump:
			HSMComp->ChangeState(10, 0);
			NOTIFY("[Shadow Moveset]: DIRECTED TO DOUBLE JUMP \n");
			break;
		case StateDirector::stomp:
			HSMComp->ChangeState(52, 0);
			NOTIFY("[Shadow Moveset]: DIRECTED TO STOMP \n");
		case StateDirector::falls:
			HSMComp->ChangeState(15, 0);
			NOTIFY("[Shadow Moveset]: Shadow shall fall \n");
		default:
			break;
	}
	return;
}

void driftdash_control(app::player::PlayerHsmContext* ctx) {
	auto* input = ctx->playerObject->GetComponent<hh::game::GOCInput>();
	auto* inputcomp = input->GetInputComponent();
	auto* gameManager = hh::game::GameManager::GetInstance();
	auto* lvlInfo = gameManager->GetService<app::level::LevelInfo>();
	auto isGrounded = lvlInfo->playerInformation->isGrounded;

	// player input
	bool boostHeld = inputcomp->actionMonitors[3].state & 256; // 3 is action, 256 or 512 is whether held or tapped
	bool boostTap = inputcomp->actionMonitors[3].state & 512;
	bool circleTap = inputcomp->actionMonitors[7].state & 512;
	bool crossTap = inputcomp->actionMonitors[0].state & 512;
	bool squareTap = inputcomp->actionMonitors[1].state & 512;

	if (isGrounded == 0) {
		Redirect(ctx, StateDirector::bouncejump);
		return;
	}

	if (squareTap && isGrounded == 0) {
		Redirect(ctx, StateDirector::homing);
		return;
	}
	if (circleTap && isGrounded == 1) {
		Redirect(ctx, StateDirector::run);
		return;
	}
	if (circleTap && isGrounded == 0) {
		Redirect(ctx, StateDirector::falls);
		return;
	}
	if (crossTap) {
		Redirect(ctx, StateDirector::jump);
		return;
	}

	if (boostHeld) // if boost button is pressed, don't give a choice to return to boost // 
	{
		ctx->blackboardStatus->SetStateFlag(app::player::BlackboardStatus::StateFlag::BOOST, 0);
		if (boostTap) 
		{
			Redirect(ctx, StateDirector::boost);
			return;
		}
	}
	else // i am not using one func to delegate this, because if boostHeld is not there, then game has higher % crashes 
	{
		if (boostTap || boostHeld) 
		{
			Redirect(ctx, StateDirector::boost);
			return;
		}
	}

	return;
}

void bounceJump_control(app::player::PlayerHsmContext* ctx, float deltaTime) {
	auto* input = ctx->playerObject->GetComponent<hh::game::GOCInput>();
	auto* inputcomp = input->GetInputComponent();
	
	static float Timer = 0.0f;
	
	Timer += deltaTime;

	if (inputcomp->actionMonitors[1].state & 512) {
		Redirect(ctx, StateDirector::homing);
	}
	if (inputcomp->actionMonitors[0].state & 512) {
		Redirect(ctx, StateDirector::doublejump);
	}
	if (inputcomp->actionMonitors[7].state & 512 && Timer > 0.15f) {
		Redirect(ctx, StateDirector::stomp);
	}
	if (inputcomp->actionMonitors[3].state & 256) {
		Redirect(ctx, StateDirector::preserveboost);
	}
}

//void spinAttackSwitcher(app::player::PlayerHsmContext* ctx, float deltaTime) {
//	auto* HSMComp = ctx->playerObject->GetComponent<app::player::GOCPlayerHsm>();
//	int getID = HSMComp->GetCurrentState();
//
//	static float SpnAttk_Timer = 0.0f;
//
//	if (SpnAttk_Timer < 0.15f) {
//		SpnAttk_Timer += deltaTime;
//	}
//	
//	if (SpnAttk_Timer > 0.15f) {
//		HSMComp->ChangeState(11, 0);
//		tryDamage(ctx, false);
//		SpnAttk_Timer = 0.0f;
//		NOTIFY("[Shadow Moveset]: DIRECTED TO BOUNCY \n");
//	}
//}

void spinAttackCalculator(app::player::PlayerHsmContext* ctx) {


	auto* playerCommonRFL = hh::fnd::ResourceManager::GetInstance()->GetResource<hh::fnd::ResReflectionT<app::player::PlayerParameters>>("player_common");
	auto* params = playerCommonRFL->GetData();

	csl::math::Vector4 velocity_m = ctx->gocPlayerKinematicParams->velocity;
	float vMagnitude = velocity_m.y();

	if (vMagnitude >= -9) {
		params->whiteSpace.spinAttack.jumpForce = 15.0f;
		params->whiteSpace.spinAttack.jumpAddForce = 15.0f;
		params->forwardView.spinAttack.jumpForce = 15.0f;
		params->forwardView.spinAttack.jumpAddForce = 15.0f;
		params->sideView.spinAttack.jumpForce = 15.0f;
		params->sideView.spinAttack.jumpAddForce = 15.0f;
		params->boss.spinAttack.jumpForce = 15.0f;
		params->boss.spinAttack.jumpAddForce = 15.0f;
	}
	else {
		params->whiteSpace.spinAttack.jumpForce = vMagnitude/-1;
		params->whiteSpace.spinAttack.jumpAddForce = vMagnitude/-1;
		params->forwardView.spinAttack.jumpForce = vMagnitude/-1;
		params->forwardView.spinAttack.jumpAddForce = vMagnitude/-1;
		params->sideView.spinAttack.jumpForce = vMagnitude/-1;
		params->sideView.spinAttack.jumpAddForce = vMagnitude/-1;
		params->boss.spinAttack.jumpForce = vMagnitude/-1;
		params->boss.spinAttack.jumpAddForce = vMagnitude/-1;
	}
}

void driftDashSwitcher(app::player::PlayerHsmContext* ctx, float deltaTime) 
{
	auto* HSMComp = ctx->playerObject->GetComponent<app::player::GOCPlayerHsm>();
	int getID = HSMComp->GetCurrentState();

	static float DDash_Timer = 0.0f;

	if (DDash_Timer < 0.15f) {
		DDash_Timer += deltaTime;
		return;
	}

	if (DDash_Timer > 0.15f) {
		Redirect(ctx, StateDirector::driftdash);	
		DDash_Timer = 0.0f;
		return;
	}
}

void jumpSpeed_func(std::string arg) {
	auto* playerCommonRFL = hh::fnd::ResourceManager::GetInstance()->GetResource<hh::fnd::ResReflectionT<app::player::PlayerParameters>>("player_common");
	if (playerCommonRFL) {
		auto* params = playerCommonRFL->GetData();

		struct jumpSpeedParams {
			float maxStickBrake;
			float minStickBrake;
			float brake;
			float accel;
			float limitUpSpeed;
			float StickSpeed;
			float bounceSpeed;
		};

		static jumpSpeedParams WS_B{
			0, 0, 0, 0, 0, 0
		};
		static jumpSpeedParams FV_B{
			0, 0, 0, 0, 0, 0
		};
		static jumpSpeedParams SV_B{
			0, 0, 0, 0, 0, 0
		};
		static jumpSpeedParams BS_B{
			0, 0, 0, 0, 0, 0
		};

		static jumpSpeedParams WS{
			0.0f, 0.0f, 0.0f, 1500.0f, 1000.0f, 15
		};
		static jumpSpeedParams FV{
			0.0f, 0.0f, 0.0f, 15.0f, 1000.0f, 15
		};
		static jumpSpeedParams SV{
			0.0f, 0.0f, 0.0f, 15.0f, 1000.0f, 10
		};
		static jumpSpeedParams BS{
			0.0f, 0.0f, 0.0f, 15.0f, 1000.0f, 15
		};

		if (arg == "backup") {
			// whiteSpace
			WS_B.maxStickBrake = params->whiteSpace.jumpSpeed.maxStickBrake;
			WS_B.minStickBrake = params->whiteSpace.jumpSpeed.minStickBrake;
			WS_B.brake = params->whiteSpace.jumpSpeed.brake;
			WS_B.accel = params->whiteSpace.jumpSpeed.accel;
			WS_B.limitUpSpeed = params->whiteSpace.jumpSpeed.limitUpSpeed;
			WS_B.StickSpeed = params->whiteSpace.jumpSpeed.minStickSpeed;
			// forwardView
			FV_B.maxStickBrake = params->forwardView.jumpSpeed.maxStickBrake;
			FV_B.minStickBrake = params->forwardView.jumpSpeed.minStickBrake;
			FV_B.brake = params->forwardView.jumpSpeed.brake;
			FV_B.accel = params->forwardView.jumpSpeed.accel;
			FV_B.limitUpSpeed = params->forwardView.jumpSpeed.limitUpSpeed;
			FV_B.StickSpeed = params->forwardView.jumpSpeed.minStickSpeed;
			// sideView
			SV_B.maxStickBrake = params->sideView.jumpSpeed.maxStickBrake;
			SV_B.minStickBrake = params->sideView.jumpSpeed.minStickBrake;
			SV_B.brake = params->sideView.jumpSpeed.brake;
			SV_B.accel = params->sideView.jumpSpeed.accel;
			SV_B.limitUpSpeed = params->sideView.jumpSpeed.limitUpSpeed;
			SV_B.StickSpeed = params->sideView.jumpSpeed.minStickSpeed;
			// boss
			BS_B.maxStickBrake = params->boss.jumpSpeed.maxStickBrake;
			BS_B.minStickBrake = params->boss.jumpSpeed.minStickBrake;
			BS_B.brake = params->boss.jumpSpeed.brake;
			BS_B.accel = params->boss.jumpSpeed.accel;
			BS_B.limitUpSpeed = params->boss.jumpSpeed.limitUpSpeed;
			BS_B.StickSpeed = params->boss.jumpSpeed.minStickSpeed;
			NOTIFY("[Shadow Moveset]: Backed up jumpSpeed params")
			return;
		}
		if (arg == "apply") {
			// whiteSpace
			params->whiteSpace.jumpSpeed.maxStickBrake = WS.maxStickBrake;
			params->whiteSpace.jumpSpeed.minStickBrake = WS.minStickBrake;
			params->whiteSpace.jumpSpeed.brake = WS.brake;
			params->whiteSpace.jumpSpeed.accel = WS.accel;
			params->whiteSpace.jumpSpeed.limitUpSpeed = WS.limitUpSpeed;
			params->whiteSpace.jumpSpeed.minStickSpeed = WS.StickSpeed;
			// forwardView
			params->forwardView.jumpSpeed.maxStickBrake = FV.maxStickBrake;
			params->forwardView.jumpSpeed.minStickBrake = FV.minStickBrake;
			params->forwardView.jumpSpeed.brake = FV.brake;
			params->forwardView.jumpSpeed.accel = FV.accel;
			params->forwardView.jumpSpeed.limitUpSpeed = FV.limitUpSpeed;
			params->forwardView.jumpSpeed.minStickSpeed = FV.StickSpeed;
			// sideView
			params->sideView.jumpSpeed.maxStickBrake = SV.maxStickBrake;
			params->sideView.jumpSpeed.minStickBrake = SV.minStickBrake;
			params->sideView.jumpSpeed.brake = SV.brake;
			params->sideView.jumpSpeed.accel = SV.accel;
			params->sideView.jumpSpeed.limitUpSpeed = SV.limitUpSpeed;
			params->sideView.jumpSpeed.minStickSpeed = SV.StickSpeed;
			// boss
			params->boss.jumpSpeed.maxStickBrake = BS.maxStickBrake;
			params->boss.jumpSpeed.minStickBrake = BS.minStickBrake;
			params->boss.jumpSpeed.brake = BS.brake;
			params->boss.jumpSpeed.accel = BS.accel;
			params->boss.jumpSpeed.limitUpSpeed = BS.limitUpSpeed;
			params->boss.jumpSpeed.minStickSpeed = BS.StickSpeed;
			NOTIFY("[Shadow Moveset]: Applied jumpSpeed params \n")
			return;
		}
		if (arg == "restore") {
			// whiteSpace
			params->whiteSpace.jumpSpeed.maxStickBrake = WS_B.maxStickBrake;
			params->whiteSpace.jumpSpeed.minStickBrake = WS_B.minStickBrake;
			params->whiteSpace.jumpSpeed.brake = WS_B.brake;
			params->whiteSpace.jumpSpeed.accel = WS_B.accel;
			params->whiteSpace.jumpSpeed.limitUpSpeed = WS_B.limitUpSpeed;
			params->whiteSpace.jumpSpeed.minStickSpeed = WS_B.StickSpeed;
			// forwardView
			params->forwardView.jumpSpeed.maxStickBrake = FV_B.maxStickBrake;
			params->forwardView.jumpSpeed.minStickBrake = FV_B.minStickBrake;
			params->forwardView.jumpSpeed.brake = FV_B.brake;
			params->forwardView.jumpSpeed.accel = FV_B.accel;
			params->forwardView.jumpSpeed.limitUpSpeed = FV_B.limitUpSpeed;
			params->forwardView.jumpSpeed.minStickSpeed = FV_B.StickSpeed;
			// sideView
			params->sideView.jumpSpeed.maxStickBrake = SV_B.maxStickBrake;
			params->sideView.jumpSpeed.minStickBrake = SV_B.minStickBrake;
			params->sideView.jumpSpeed.brake = SV_B.brake;
			params->sideView.jumpSpeed.accel = SV_B.accel;
			params->sideView.jumpSpeed.limitUpSpeed = SV_B.limitUpSpeed;
			params->sideView.jumpSpeed.minStickSpeed = SV_B.StickSpeed;
			// boss
			params->boss.jumpSpeed.maxStickBrake = BS_B.maxStickBrake;
			params->boss.jumpSpeed.minStickBrake = BS_B.minStickBrake;
			params->boss.jumpSpeed.brake = BS_B.brake;
			params->boss.jumpSpeed.accel = BS_B.accel;
			params->boss.jumpSpeed.limitUpSpeed = BS_B.limitUpSpeed;
			params->boss.jumpSpeed.minStickSpeed = BS_B.StickSpeed;
			NOTIFY("[Shadow Moveset]: Restored jumpSpeed params \n")
			return;
		}
		return;
	}
	return;
}

// !!! big func, don't open it !!! 
void player_common_tweak() {
	auto* playerCommonRFL = hh::fnd::ResourceManager::GetInstance()->GetResource<hh::fnd::ResReflectionT<app::player::PlayerParameters>>("player_common");
	auto* params = playerCommonRFL->GetData();
	//whiteSpace
	params->whiteSpace.spinAttack.limitSpeedMax = 100.0f;
	params->whiteSpace.spinAttack.brakeForce = 0.0f;
	params->whiteSpace.driftDash.brake = 0.0f;
	params->whiteSpace.driftDash.checkDashSpeed = 15.0f;
	params->whiteSpace.driftDash.checkDashTime = 0.45f;
	// forwardView
	params->forwardView.spinAttack.limitSpeedMax = 80.0f;
	params->forwardView.spinAttack.brakeForce = 0.0f;
	params->forwardView.driftDash.brake = 0.0f;
	params->forwardView.driftDash.checkDashSpeed = 15.0f;
	params->forwardView.driftDash.checkDashTime = 0.45f;
	// sideView
	params->sideView.spinAttack.limitSpeedMax = 80.0f;
	params->sideView.spinAttack.brakeForce = 0.0f;
	params->sideView.driftDash.brake = 0.0f;
	params->sideView.driftDash.checkDashSpeed = 15.0f;
	params->sideView.driftDash.checkDashTime = 0.45f;
	// boss
	params->boss.spinAttack.limitSpeedMax = 80.0f;
	params->boss.spinAttack.brakeForce = 0.0f;
	params->boss.driftDash.brake = 0.0f;
	params->boss.driftDash.checkDashSpeed = 15.0f;
	params->boss.driftDash.checkDashTime = 0.45f;
	// ... okay maybe not that big rn, but soon it'd be BIG 
	// or not..
	// not used for bundled in Digital Circus
}

HOOK(uint64_t, __fastcall, GameModeTitleInit, 0x1401990E0, app::game::GameMode* self) {
	auto res = originalGameModeTitleInit(self);
	player_common_tweak();
	jumpSpeed_func("backup");
	return res;
}

HOOK(void, __fastcall, InitPlayer, 0x14060DBC0, hh::game::GameObject* self) {
	originalInitPlayer(self);
	
	auto* BBcomp = self->GetComponent<app::player::GOCPlayerBlackboard>();
	auto* BBstatus = BBcomp->blackboard->GetContent<app::player::BlackboardStatus>();

	if (BBstatus) {
		BBstatus->SetCombatFlag(app::player::BlackboardStatus::CombatFlag::SLALOM_STEP, 1);
		NOTIFY("[Shadow Moveset]: Enabled Slalom Step \n")
		return;
	}
	else {
		NOTIFY("[Shadow Moveset]: Failed to enable Slalom step \n")
		return;
	}
	return;

}

HOOK(bool, __fastcall, SpinAttackStep, 0x1406CB3E0, app::player::StateSpinAttack* self, app::player::PlayerHsmContext* ctx, float deltaTime) {
	auto* HSMComp = ctx->playerObject->GetComponent<app::player::GOCPlayerHsm>();
	DDash_bounceJump_hacking(ctx, 0);
	HSMComp->ChangeState(11, 0);
	return originalSpinAttackStep(self, ctx, deltaTime);
}


HOOK(bool, __fastcall, StompingBounceStep, 0x1406D1620, app::player::StateStompingBounce* self, app::player::PlayerHsmContext* ctx, float deltaTime) {
	Redirect(ctx, StateDirector::bouncejump);
	return originalStompingBounceStep(self, ctx, deltaTime);
}

HOOK(bool, __fastcall, SlidingStep, 0x1406CC600, app::player::StateSliding* self, app::player::PlayerHsmContext* ctx, float deltaTime) {
	driftDashSwitcher(ctx, deltaTime);
	return originalSlidingStep(self, ctx, deltaTime);
}

HOOK(bool, __fastcall, DriftDashStep, 0x1406AFB70, app::player::StateDriftDash* self, app::player::PlayerHsmContext* ctx, float deltaTime) {
	tryDamage(ctx, false);
	driftdash_control(ctx);
	return originalDriftDashStep(self, ctx, deltaTime);
}

HOOK(bool, __fastcall, SlalomStep, 0x1406C9390, app::player::StateSlalomStep * self, app::player::PlayerHsmContext * ctx, float deltaTime) {
	auto* input = ctx->playerObject->GetComponent<hh::game::GOCInput>();
	auto* inputcomp = input->GetInputComponent();
	if (inputcomp->actionMonitors[3].state & 256) {
		/*ctx->blackboardStatus->SetStateFlag(app::player::BlackboardStatus::StateFlag::BOOST, 1);
		tryDamage(ctx);*/
		Redirect(ctx, StateDirector::preserveboost);
	}
	return originalSlalomStep(self, ctx, deltaTime);
}

HOOK(bool, __fastcall, BounceJumpStep, 0x1406C1C80, app::player::StateBounceJump* self, app::player::PlayerHsmContext* ctx, float deltaTime) {
	bounceJump_control(ctx, deltaTime);
	
	tryDamage(ctx, 0);
	
	return originalBounceJumpStep(self, ctx, deltaTime);
}

HOOK(bool, __fastcall, GrindDoubleJumpStep, 0x1406BEAB0, app::player::StateGrindDoubleJump* self, app::player::PlayerHsmContext* ctx, float deltaTime) {
	tryDamage(ctx, 0);
	return originalGrindDoubleJumpStep(self, ctx, deltaTime);
}

HOOK(void, __fastcall, StateDriftDashLeave, 0x1406AF8D0, app::player::StateDriftDash* self, app::player::PlayerHsmContext* ctx, int nextState) {
	auto* GOCSound = ctx->playerObject->GetComponent<hh::snd::GOCSound>();
	tryDamage(ctx, true);
	GOCSound->StopAll(1);
	originalStateDriftDashLeave(self, ctx, nextState);
}

//HOOK(void, __fastcall, SpinAttackEnter, 0x1406CB150,app::player::StateSpinAttack* self, app::player::PlayerHsmContext* ctx, int nextState) {
//	originalSpinAttackEnter(self, ctx, nextState);
//	auto* HSMComp = ctx->playerObject->GetComponent<app::player::GOCPlayerHsm>();
//	HSMComp->ChangeState(11, 0);
//}


HOOK(void, __fastcall, StateBounceJumpEnter, 0x1406C06E0, app::player::StateBounceJump* self, app::player::PlayerHsmContext* ctx, int previousState) {
	jumpSpeed_func("apply");
	originalStateBounceJumpEnter(self, ctx, previousState);
}

HOOK(void, __fastcall, StateBounceJumpLeave, 0x1406C15F0, app::player::StateBounceJump* self, app::player::PlayerHsmContext* ctx, int nextState) {
	jumpSpeed_func("restore");
	DDash_bounceJump_hacking(ctx, 0);
	originalStateBounceJumpLeave(self, ctx, nextState);
}

//HOOK(void, __fastcall, BindMaps, 0x140A2FEC0, hh::game::GameManager* gameManager, hh::hid::InputMapSettings* inputSettings) {
//	inputSettings->BindActionMapping("PlayerLightDash", 0x2001du);
//	originalBindMaps(gameManager, inputSettings);
//}
// 
//0x140684B80 for ascending Shadow XDDD
//0x1406985B0 is right one
//got the wrong address and Shadow ascended
BOOL WINAPI DllMain(_In_ HINSTANCE hInstance, _In_ DWORD reason, _In_ LPVOID reserved)
{
	switch (reason)
	{
	case DLL_PROCESS_ATTACH:
		INSTALL_HOOK(GameModeTitleInit);
		INSTALL_HOOK(InitPlayer);
		
		INSTALL_HOOK(SpinAttackStep);
		INSTALL_HOOK(SlidingStep);
		INSTALL_HOOK(DriftDashStep);
		INSTALL_HOOK(SlalomStep);
		INSTALL_HOOK(BounceJumpStep);		
		INSTALL_HOOK(GrindDoubleJumpStep);
		INSTALL_HOOK(StateDriftDashLeave);

		INSTALL_HOOK(StateBounceJumpEnter);
		INSTALL_HOOK(StateBounceJumpLeave);

		//INSTALL_HOOK(StompingBounceStep);
		//INSTALL_HOOK(DropDashStep);
		//INSTALL_HOOK(FallStep);
		//INSTALL_HOOK(StandStep);
		//INSTALL_HOOK(RunStep);
		//INSTALL_HOOK(JumpStep);
		//INSTALL_HOOK(DoubleJumpStep);
		break;
	case DLL_PROCESS_DETACH:
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
		break;
	}

	return TRUE;
}
