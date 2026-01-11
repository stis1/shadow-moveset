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
	stomp
};

void doWeDoDriftDashOrNah(app::player::PlayerHsmContext* ctx) {
	
}

void tryDamage(app::player::PlayerHsmContext* ctx, bool leavemealone) {
	auto* collision = ctx->gocPlayerHsm->statePluginManager->GetPlugin<app::player::StatePluginCollision>();
	auto* BBbattle = ctx->gocPlayerBlackboard->blackboard->GetContent<app::player::BlackboardBattle>();
	auto* BBstatus = ctx->blackboardStatus;
	auto* HSMComp = ctx->playerObject->GetComponent<app::player::GOCPlayerHsm>();

	static bool isInApplicableState = false;
	//static bool bomboDamage = false;
	
	bool isBoosting = BBstatus->GetStateFlag(app::player::BlackboardStatus::StateFlag::BOOST);
	
	int getID = HSMComp->GetCurrentState();
	
	if (getID == 107 || getID == 10 || getID == 11 || getID == 9 /*|| getID == 46 */ ) {
		isInApplicableState = true;
	}
	else {
		isInApplicableState = false;
	}

	if (isInApplicableState) {
		collision->SetTypeAndRadius(1, 1);
		BBbattle->SetFlag020(false);
		//bomboDamage = true;
		
	}
	if (isBoosting) {
		collision->SetTypeAndRadius(2, 1);
		BBbattle->SetFlag020(false);
		//bomboDamage = true;
		
	}
	if (!isBoosting && !isInApplicableState)
	{
		collision->SetTypeAndRadius(1, 0);
		collision->SetTypeAndRadius(2, 0);
		BBbattle->SetFlag020(true);
		//bomboDamage = false;
		
	}	
	return;
}
// 0 if damage is needed 
void Redirect(app::player::PlayerHsmContext* ctx, StateDirector to_what)
{
	auto* HSMComp = ctx->playerObject->GetComponent<app::player::GOCPlayerHsm>();
	auto* GOCSound = ctx->playerObject->GetComponent<hh::snd::GOCSound>();
	switch (to_what) 
	{
		case StateDirector::driftdash:
			HSMComp->hsm.ChangeState(107);
			tryDamage(ctx, 0);
			PRINT_ERROR(" DIRECTED TO DRIFTDASH \n");
			GOCSound->Play("sd_spin", 1);
			break;
		case StateDirector::bouncejump:
			HSMComp->hsm.ChangeState(11);
			PRINT_ERROR(" DIRECTED TO BOUNCEJUMP \n");

			break;
		case StateDirector::jump:
			HSMComp->hsm.ChangeState(9);
			PRINT_ERROR(" DIRECTED TO JUMP \n");
			break;
		case StateDirector::run:
			HSMComp->hsm.ChangeState(4);
			PRINT_ERROR(" DIRECTED TO RUN \n");
			break;
		case StateDirector::boost:
			HSMComp->hsm.ChangeState(4);
			ctx->blackboardStatus->SetStateFlag(app::player::BlackboardStatus::StateFlag::BOOST, 1);
			tryDamage(ctx, 0);
			PRINT_ERROR(" DIRECTED TO BOOST \n");
			break;
		case StateDirector::homing:
			HSMComp->hsm.ChangeState(63);
			PRINT_ERROR(" HOMING SHADOW \n");
			break;
		case StateDirector::preserveboost: // for boosting inside slalom, maybe others, bad implementation if being honest
			ctx->blackboardStatus->SetStateFlag(app::player::BlackboardStatus::StateFlag::BOOST, 1);
			tryDamage(ctx, 0);
			PRINT_ERROR(" APPLIED BOOST TO SLALOM STEP \n");
			break;
		case StateDirector::doublejump:
			HSMComp->hsm.ChangeState(10);
			PRINT_ERROR(" DIRECTED TO DOUBLE JUMP \n");
			break;
		case StateDirector::stomp:
			
			PRINT_ERROR(" unfortunately \n");
		default:
			break;
	}
	return;
}

void Spin_control(app::player::PlayerHsmContext* ctx, float deltaTime) {
	//check buttons
	//if (RT), then boost
	//if (B), then run
	//if (A), then jump
	auto* input = ctx->playerObject->GetComponent<hh::game::GOCInput>();
	auto* inputcomp = input->GetInputComponent();
	
	// player input
	bool boostHeld = inputcomp->actionMonitors[3].state & 256; // 3 is action, 256 or 512 is whether held or tapped
	bool boostTap = inputcomp->actionMonitors[3].state & 512;
	bool circleTap = inputcomp->actionMonitors[7].state & 512;
	bool crossTap = inputcomp->actionMonitors[0].state & 512;

	if (inputcomp->actionMonitors[1].state & 512) {
		Redirect(ctx, StateDirector::homing);
	}

	if (boostHeld) // if boost button is pressed, don't give a choice to return to boost // 
	{
		ctx->blackboardStatus->SetStateFlag(app::player::BlackboardStatus::StateFlag::BOOST, 0);
		if (boostTap) {
			Redirect(ctx, StateDirector::boost);
		}
		if (circleTap) {
			Redirect(ctx, StateDirector::run);
		}
		if (crossTap) {
			Redirect(ctx, StateDirector::jump);
		}
	}
	else // i am not using one func to delegate this, because if boostHeld is not there, then game has higher % crashes 
	{
		if (boostTap || boostHeld) {
			Redirect(ctx, StateDirector::boost);
		}
		if (circleTap) {
			Redirect(ctx, StateDirector::run);
		}
		if (crossTap) {
			Redirect(ctx, StateDirector::jump);
		}
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

void slalomEnable(app::player::PlayerHsmContext* ctx) {
	ctx->blackboardStatus->SetCombatFlag(app::player::BlackboardStatus::CombatFlag::SLALOM_STEP, 1);
} 

HOOK(uint64_t, __fastcall, GameModeTitleInit, 0x1401990E0, app::game::GameMode* self) {
	player_common_tweak();
	return originalGameModeTitleInit(self);
}

HOOK(bool, __fastcall, SpinAttackStep, 0x1406CB3E0, app::player::StateSpinAttack* self, app::player::PlayerHsmContext* ctx, float deltaTime) {
	
	Redirect(ctx, StateDirector::bouncejump);
	
	return originalSpinAttackStep(self, ctx, deltaTime);
}

HOOK(bool, __fastcall, StompingBounceStep, 0x1406D1620, app::player::StateStompingBounce* self, app::player::PlayerHsmContext* ctx, float deltaTime) {
	Redirect(ctx, StateDirector::bouncejump);
	return originalStompingBounceStep(self, ctx, deltaTime);
}

HOOK(void, __fastcall, SlidingEnter, 0x1406CBB70, app::player::PlayerStateBase* self, app::player::PlayerHsmContext* ctx, int nextState) {
	Redirect(ctx, StateDirector::driftdash);
}

//HOOK(bool, __fastcall, DropDashStep, 0x1406AFFB0, app::player::StateDropDash* self, app::player::PlayerHsmContext* ctx, float deltaTime) {
//	/*Spin_control(ctx);*/
//	/*Redirect(ctx, StateDirector::driftdash);*/ //so many crashes 
//	return originalDropDashStep(self, ctx, deltaTime);
//}

HOOK(bool, __fastcall, DriftDashStep, 0x1406AFB70, app::player::StateDriftDash* self, app::player::PlayerHsmContext* ctx, float deltaTime) {
	slalomEnable(ctx);
	tryDamage(ctx, 0);
	Spin_control(ctx, deltaTime);
	
	return originalDriftDashStep(self, ctx, deltaTime);
}

HOOK(bool, __fastcall, FallStep, 0x1406B82C0, app::player::StateStand* self, app::player::PlayerHsmContext* ctx, float deltaTime) {
	slalomEnable(ctx);
	return originalFallStep(self, ctx, deltaTime);
}

HOOK(bool, __fastcall, StandStep, 0x1406CE800, app::player::StateStand* self, app::player::PlayerHsmContext* ctx, float deltaTime) {
	slalomEnable(ctx);
	return originalStandStep(self, ctx, deltaTime);
}

HOOK(bool, __fastcall, RunStep, 0x1406C7760, app::player::StateRun* self, app::player::PlayerHsmContext* ctx, float deltaTime) {
	slalomEnable(ctx);
	tryDamage(ctx, 1);
	return originalRunStep(self, ctx, deltaTime);
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
	auto* input = ctx->playerObject->GetComponent<hh::game::GOCInput>();
	auto* inputcomp = input->GetInputComponent();
	
	if (inputcomp->actionMonitors[1].state & 512) {
		Redirect(ctx, StateDirector::homing);
	}
	if (inputcomp->actionMonitors[0].state & 512) {
		Redirect(ctx, StateDirector::doublejump);
	}
	if (inputcomp->actionMonitors[7].state & 512) {
		Redirect(ctx, StateDirector::stomp);
	}
	if (inputcomp->actionMonitors[3].state & 256) {
		Redirect(ctx, StateDirector::preserveboost);
	}
	tryDamage(ctx, 0);
	return originalBounceJumpStep(self, ctx, deltaTime);
}

//HOOK(bool, __fastcall, JumpStep, 0x14A7C1140, app::player::StateJump* self, app::player::PlayerHsmContext* ctx, float deltaTime) {
//	tryDamage(ctx, 0);
//	return originalJumpStep(self, ctx, deltaTime);
//}

//HOOK(bool, __fastcall, DoubleJumpStep, 0x14A7ADE20, app::player::StateDoubleJump* self, app::player::PlayerHsmContext* ctx, float deltaTime) {
//	tryDamage(ctx, 0);
//	return originalDoubleJumpStep(self, ctx, deltaTime);
//}

//HOOK(bool, __fastcall, GrindDoubleJumpStep, 0x1406BEAB0, app::player::StateGrindDoubleJump* self, app::player::PlayerHsmContext* ctx, float deltaTime) {
//	tryDamage(ctx, 0);
//	return originalGrindDoubleJumpStep(self, ctx, deltaTime);
//}

//HOOK(bool, __fastcall, HomingAttackStep, 0x14A632A90, app::player::StateHomingAttack* self, app::player::PlayerHsmContext* ctx, float deltaTime) {
//	tryDamage(ctx, 0);
//	return originalHomingAttackStep(self, ctx, deltaTime);
//}

HOOK(bool, __fastcall, StateDriftDashLeave, 0x1406AF8D0, app::player::StateDriftDash* self, app::player::PlayerHsmContext* ctx, int nextState) {
	auto* GOCSound = ctx->playerObject->GetComponent<hh::snd::GOCSound>();
	originalStateDriftDashLeave(self, ctx, nextState);
	tryDamage(ctx, 1);
	GOCSound->StopAll(1);
	return originalStateDriftDashLeave(self, ctx, nextState);
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
		INSTALL_HOOK(SpinAttackStep);
		INSTALL_HOOK(StompingBounceStep);
		INSTALL_HOOK(SlidingEnter);
		//INSTALL_HOOK(DropDashStep);
		INSTALL_HOOK(DriftDashStep);
		INSTALL_HOOK(FallStep);
		INSTALL_HOOK(StandStep);
		INSTALL_HOOK(SlalomStep);
		INSTALL_HOOK(BounceJumpStep);
		INSTALL_HOOK(RunStep);
		//INSTALL_HOOK(JumpStep);
		//INSTALL_HOOK(DoubleJumpStep);
		//INSTALL_HOOK(GrindDoubleJumpStep);
		//INSTALL_HOOK(HomingAttackStep);
		INSTALL_HOOK(StateDriftDashLeave);
		break;
	case DLL_PROCESS_DETACH:
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
		break;
	}

	return TRUE;
}
