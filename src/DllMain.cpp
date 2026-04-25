//////////////////////////////////////////////////////////////////////////////
csl::math::Vector4 prevVelocity{};
csl::math::Vector3 jumpEnterNormal{};
bool WantToBounce{};
bool wasI{};
bool rolling{};
csl::math::Vector2 stompInput{};
csl::math::Vector2 bounceEnterInput{};
int bounceEnterId{};
int stompDownEnterId{};
///////////////////////////////////////
float divingIdleTimer{};
bool divingIdleStatus{};
///////////////////////////////////////
float jumpTimer{};
bool jumpStatus{};
bool jumpAnim{};
bool isShortHop{};
///////////////////////////////////////
float wallJumpTimer{};
//////////////////////////////////////////////////////////////////////////////

float deadZone (0.045f); // deadzone of 15% on both X and Y axis, magnitude of vector2(0.15,0.15)

enum State
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

enum Hedgehog {
	Sonic,
	Shadow
};

static Hedgehog player{};

void SetHedgehog() {
	mINI::INIFile file("hedgehog.ini");
	mINI::INIStructure ini;
	if (!file.read(ini)) {
		FatalExit(-1);
	}
	auto& hedgehog = ini["Hedgehog"];
	bool option = hedgehog["Sonic"] == "1";
	if (option) {
		player = Sonic;
	}
	else {
		player = Shadow;
	}
}

inline void пидарас(app::player::PlayerHsmContext* ctx) {
	if (ctx->blackboardStatus->GetCombatFlag(app::player::BlackboardStatus::CombatFlag::DOUBLE_JUMP)) {
		ctx->gocPlayerHsm->ChangeState(10, 100);
	}
	else {
		ctx->gocPlayerHsm->ChangeState(13, 100);
	}
}

void damageControl(app::player::PlayerHsmContext* ctx, bool leaveMeAlone) {
	auto* pPluginCollision = ctx->gocPlayerHsm->statePluginManager->GetPlugin<app::player::StatePluginCollision>();
	auto* pBBbattle = ctx->gocPlayerBlackboard->blackboard->GetContent<app::player::BlackboardBattle>();
	
	static bool isInBallState{};

	bool isBoosting = ctx->gocPlayerBlackboard->blackboard->GetContent<app::player::BlackboardStatus>()->GetStateFlag(app::player::BlackboardStatus::StateFlag::BOOST);
	int getID = ctx->gocPlayerHsm->GetCurrentState();

	if (leaveMeAlone == true) {
		pPluginCollision->SetTypeAndRadius(2, 0);
		pBBbattle->SetFlag020(true);
		return;
	}

	if (getID == 107 || getID == 10 || getID == 9 || getID == 46) {
		isInBallState = true;
	}
	else {
		isInBallState = false;
	}

	if (isBoosting || isInBallState) {
		pPluginCollision->SetTypeAndRadius(2, 1);
		pBBbattle->SetFlag020(false);
		return;
	}
	if (getID == 11) {
		pPluginCollision->SetTypeAndRadius(1, 1);
		pBBbattle->SetFlag020(false);
		return;
	}
	if (!isBoosting && !isInBallState)
	{
		//collision->SetTypeAndRadius(1, 0);
		pPluginCollision->SetTypeAndRadius(2, 0);
		pBBbattle->SetFlag020(true);
		return;
	}
	return;
}

void rolling_air_hack(app::player::PlayerHsmContext* ctx) {
	auto* pParams = hh::fnd::ResourceManager::GetInstance()->GetResource<hh::fnd::ResReflectionT<app::player::PlayerParameters>>("player_common")->GetData();
	float vMagnitude = ctx->gocPlayerKinematicParams->velocity.y();

	if (pParams) {
		pParams->whiteSpace.doubleJump.bounceSpeed = vMagnitude*0.8f;
		pParams->forwardView.doubleJump.bounceSpeed = vMagnitude*0.8f;
		pParams->sideView.doubleJump.bounceSpeed = vMagnitude*0.8f;
		pParams->boss.doubleJump.bounceSpeed = vMagnitude*0.8f;
	}	
}

void Redirect(app::player::PlayerHsmContext* ctx, State to_what)
{
	auto* pGOCPlayerHsm = ctx->gocPlayerHsm;
	auto* plugin = ctx->gocPlayerHsm->statePluginManager->GetPlugin<app::player::StatePluginBoost>();
	app::level::PlayerInformation* playerInfo = hh::game::GameManager::GetInstance()->GetService<app::level::LevelInfo>()->playerInformation;
	// i'm sorry that i have to do this, but boostplugin addcallback is obfuscated and im stupid
	switch (to_what) 
	{
		case driftdash:
			pGOCPlayerHsm->ChangeState(107, 0);
			damageControl(ctx, false);
#if _DEBUG
			NOTIFY("DIRECTED TO DRIFTDASH");
#endif
			break;
		case bouncejump:
			pGOCPlayerHsm->ChangeState(11, 0);
			rolling_air_hack(ctx);
			damageControl(ctx, false);
#if _DEBUG
			NOTIFY("DIRECTED TO BOUNCEJUMP");
#endif	
			break;
		case jump:
			pGOCPlayerHsm->ChangeState(9,0);
			damageControl(ctx, 0);
#if _DEBUG
			NOTIFY("DIRECTED TO JUMP");
#endif	
			break;
		case run:
			pGOCPlayerHsm->ChangeState(4, 0);
			damageControl(ctx, 1);
#if _DEBUG
			NOTIFY("DIRECTED TO RUN");
#endif	
			break;
		case boost:
			if (playerInfo->currentBoostGauge > 0.0f) {
				pGOCPlayerHsm->ChangeState(4, 0);
				plugin->Boost();
				ctx->gocPlayerHsm->statePluginManager->GetPlugin<app::player::StatePluginCollision>()->SetTypeAndRadius(2, 1);
			}
#if _DEBUG
			NOTIFY("DIRECTED TO BOOST");
#endif
			break;
		case homing:
			ctx->DoHoming(0);
#if _DEBUG
			NOTIFY("HOMING SHADOW");
#endif
			break;
		case preserveboost: // for boosting inside slalom, maybe others, bad implementation if being honest
			ctx->blackboardStatus->SetStateFlag(app::player::BlackboardStatus::StateFlag::BOOST, 1);
			damageControl(ctx, false);
#if _DEBUG
			NOTIFY("KEPT BOOST");
#endif
			break;
		case doublejump:
			pGOCPlayerHsm->ChangeState(10, 0);
#if _DEBUG
			NOTIFY("DIRECTED TO DOUBLE JUMP");
#endif
			break;
		case stomp:	
			pGOCPlayerHsm->ChangeState(52, 0);
#if _DEBUG
			NOTIFY("DIRECTED TO STOMP");
#endif
			break;
		case falls:
			pGOCPlayerHsm->ChangeState(15, 0);
#if _DEBUG
			NOTIFY("Shadow shall fall");
#endif
			break;
		default:
			break;
	}
	return;
}

bool driftdash_inputs(app::player::PlayerHsmContext* ctx) {
	app::level::PlayerInformation* playerInfo = hh::game::GameManager::GetInstance()->GetService<app::level::LevelInfo>()->playerInformation;
	bool isGrounded = playerInfo->isGrounded.value();
	
	auto* plugin = ctx->gocPlayerHsm->statePluginManager->GetPlugin<app::player::StatePluginBoost>();
	
	if (!isGrounded) {
		Redirect(ctx, bouncejump);
		return 1;
	}
	// player input
	auto* pGOCInput = ctx->playerObject->GetComponent<hh::game::GOCInput>()->GetInputComponent();
	 // 3 is action, 256 or 512 is whether held or tapped
	bool circleTap = pGOCInput->actionMonitors[7].state & 512;
	bool crossTap = pGOCInput->actionMonitors[0].state & 512;

	if (circleTap) {
		Redirect(ctx, run);
		return 1;
	}
	if (crossTap) {
		ctx->gocPlayerHsm->ChangeState(9, 0);
		return 1;
	}
	bool boostHeld = pGOCInput->actionMonitors[3].state & 256;
	bool boostTap = pGOCInput->actionMonitors[3].state & 512;
	if (boostHeld) // if boost button is pressed, don't give a choice to return to boost // 
	{
		ctx->blackboardStatus->SetStateFlag(app::player::BlackboardStatus::StateFlag::BOOST, 0);
		if (boostTap) 
		{
			if (playerInfo->currentBoostGauge > 0.0f) {
				ctx->gocPlayerHsm->ChangeState(4, 0);
				ctx->blackboardStatus->SetStateFlag(app::player::BlackboardStatus::StateFlag::BOOST, 1);
				ctx->gocPlayerBlackboard->blackboard->GetContent<app::player::BlackboardBattle>()->SetFlag020(0);
				ctx->gocPlayerHsm->statePluginManager->GetPlugin<app::player::StatePluginCollision>()->SetTypeAndRadius(2, 1);
				plugin->Boost();
				return 1;
			}
			else {
				return 0;
			}
		}
	}
	else // i am not using one func to delegate this, because if boostHeld is not there, then game has higher % crashes 
	{
		if (boostTap || boostHeld) 
		{
			if (playerInfo->currentBoostGauge > 0.0f) {
				ctx->gocPlayerHsm->ChangeState(4, 0);
				ctx->blackboardStatus->SetStateFlag(app::player::BlackboardStatus::StateFlag::BOOST, 1);
				ctx->gocPlayerBlackboard->blackboard->GetContent<app::player::BlackboardBattle>()->SetFlag020(0);
				ctx->gocPlayerHsm->statePluginManager->GetPlugin<app::player::StatePluginCollision>()->SetTypeAndRadius(2, 1);
				plugin->Boost();
				return 1;
			}
			else {
				return 0;
			}
		}
	}

	return 0;
}

bool bouncejump_inputs(app::player::PlayerHsmContext* ctx) {
	/*if (ctx->DoHoming(0)) {
		return 1;
	}*/
	ctx->DoHoming(0);
	auto* pGOCInput = ctx->playerObject->GetComponent<hh::game::GOCInput>()->GetInputComponent();
	float Speed = hh::game::GameManager::GetInstance()->GetService<app::level::LevelInfo>()->playerInformation->Speed.value();
	
	if (pGOCInput->actionMonitors[7].state & 512 && rolling) {
		ctx->gocPlayerHsm->ChangeState(4, 100);
		return 1;
	}
	if (pGOCInput->actionMonitors[0].state & 512) {
		пидарас(ctx);
		return 1;
	}
	if (pGOCInput->actionMonitors[7].state & 512) {
		ctx->gocPlayerHsm->ChangeState(52, 100);
		return 1;
	}
	return 0;
	//if (inputcomp->actionMonitors[7].state & 256 && Timer > 0.15f) {
	//	Redirect(ctx, StateDirector::bouncejump);
	//}
}

void drop_driftdash_transition(app::player::PlayerHsmContext* ctx, float deltaTime) {
	int getID = ctx->gocPlayerHsm->GetCurrentState();

	static float DDash_Timer = 0.0f;

	if (DDash_Timer < 0.15f && getID != 111) {
		DDash_Timer += deltaTime;
		ctx->gocPlayerHsm->ChangeState(109, 100);
		#if _DEBUG
		NOTIFY("put to drop for")
		#endif
		return;
	}

	if (DDash_Timer < 0.15f && getID == 109 || getID == 111) {
		DDash_Timer += deltaTime;
	}

	if (DDash_Timer > 0.15f) {
		Redirect(ctx, driftdash);
		DDash_Timer = 0.0f;
		return;
	}
}

void pizdecSuperBiker3D(app::player::PlayerHsmContext* ctx, float deltaTime) {
	const float bounceBreakForce = 20.0f;
	float prevX = prevVelocity.x();
	float prevZ = prevVelocity.z();
	float Y = ctx->gocPlayerKinematicParams->velocity.y();
	
	csl::math::Vector4 horVector(prevX, Y, prevZ, 0);
	if (prevX == 0 && prevZ == 0) {
		return; // discarded samples, return immediately;
	}
	#if _DEBUG
	std::cout << "PrevX infunc is " << prevX << std::endl;
	std::cout << "PrevZ infunc is " << prevZ << std::endl;
	#endif

	if (horVector.norm() < bounceBreakForce * deltaTime) {
		ctx->gocPlayerKinematicParams->velocity = csl::math::Vector4(0, 0, 0, 0);
	}
	else {
		Eigen::Vector4f horVectornormalize = (horVector / horVector.norm());
		ctx->gocPlayerKinematicParams->velocity = (horVectornormalize * (horVector.norm() - (bounceBreakForce * deltaTime)));
	}
	prevVelocity = ctx->gocPlayerKinematicParams->velocity;
}

void Bouncinggg(std::string arg, float Multiplier, bool directionChanged) {
	auto* pParams = hh::fnd::ResourceManager::GetInstance()->GetResource<hh::fnd::ResReflectionT<app::player::PlayerParameters>>("player_common")->GetData();
	
	float Speed = hh::game::GameManager::GetInstance()->GetService<app::level::LevelInfo>()->playerInformation->Speed.value();
	static float SampleSpeed;

	if (arg == "null") {
		SampleSpeed = 5;
		return;
	}
	if (arg == "sample") {
		SampleSpeed = Speed;
		return;
	}
	if (arg == "apply") {
		SampleSpeed = SampleSpeed * Multiplier;
		pParams->whiteSpace.jumpSpeed.limitUpSpeed= SampleSpeed;
		pParams->forwardView.jumpSpeed.limitUpSpeed = SampleSpeed;
		pParams->sideView.jumpSpeed.limitUpSpeed = SampleSpeed;
		pParams->boss.jumpSpeed.limitUpSpeed = SampleSpeed;
		if (directionChanged) {
			pParams->whiteSpace.jumpSpeed.minStickSpeed = 0.1f;
			pParams->forwardView.jumpSpeed.minStickSpeed = 0.1f;
			pParams->sideView.jumpSpeed.minStickSpeed = 0.1f;
			pParams->boss.jumpSpeed.minStickSpeed = 0.1f;
		}
		else {
			pParams->whiteSpace.jumpSpeed.minStickSpeed = SampleSpeed;
			pParams->forwardView.jumpSpeed.minStickSpeed = SampleSpeed;
			pParams->sideView.jumpSpeed.minStickSpeed = SampleSpeed;
			pParams->boss.jumpSpeed.minStickSpeed = SampleSpeed;
		}
		if (Multiplier < 0.09) {
			pParams->whiteSpace.jumpSpeed.brake = 75;
			pParams->forwardView.jumpSpeed.brake = 75;
			pParams->sideView.jumpSpeed.brake = 75;
			pParams->boss.jumpSpeed.brake = 75;
			return;
		}
		if (Multiplier < 0.9) {
			pParams->whiteSpace.jumpSpeed.brake = 20 / Multiplier;
			pParams->forwardView.jumpSpeed.brake = 20 / Multiplier;
			pParams->sideView.jumpSpeed.brake = 3 / Multiplier;
			pParams->boss.jumpSpeed.brake = 5 / Multiplier;
			return;
		}
		return;
	}
}

HOOK(void, __fastcall, InitPlayer, 0x14060DBC0, hh::game::GameObject* self) {
	originalInitPlayer(self);

	//self->GetComponent<app::player::GOCPlayerBlackboard>()->blackboard->GetContent<app::player::BlackboardStatus>()->SetCombatFlag(app::player::BlackboardStatus::CombatFlag::SLALOM_STEP,1);
	//rfl_idiotism::player_common_tweak();
	rfl_idiotism::jump_speed_func("backup");
	rfl_idiotism::bounceRFL("backup");
	auto dword28 = self->GetComponent<app::player::GOCPlayerBlackboard>()->blackboard->GetContent<app::player::BlackboardStatus>()->dword28;
	std::cout << "DWORD28, probably skin id: " << dword28 << std::endl; // 3 if movie Shadow
	// reset
	prevVelocity = csl::math::Vector4(0,0,0,0);
	jumpEnterNormal = csl::math::Vector3(0, 0, 0);
	WantToBounce = 0;
	wasI = 0;
	rolling = 0;
	stompInput = csl::math::Vector2(0, 0);
	bounceEnterInput = csl::math::Vector2(0, 0);
	bounceEnterId = 0;
	stompDownEnterId = 0;
	divingIdleTimer = 0.0f;
	divingIdleStatus = 0;
	jumpTimer = 0;
	jumpStatus = 0;
	jumpAnim = 0;
	wallJumpTimer = 0;

	return;
}

HOOK(void, __fastcall, Enter_Sliding, 0x1406CBB70, app::player::StateSliding* self, app::player::PlayerHsmContext* ctx, int previousState) {
	//app::level::PlayerInformation* playerInfo = hh::game::GameManager::GetInstance()->GetService<app::level::LevelInfo>()->playerInformation;
	//csl::math::Vector2 leftStick = playerInfo->leftStickInput.value();
	//float speed = playerInfo->Speed.value();
	///*bool circleDown = ctx->playerObject->GetComponent<hh::game::GOCInput>()->GetInputComponent()->actionMonitors[7].state & 256;*/
	//if (speed > 17 && std::fabs(leftStick.x()) > 0.4f && std::fabs(leftStick.y() < 0.8f)) {
	//	ctx->gocPlayerHsm->hsm.ChangeState(106, 0);
	//	return;
	//}	
	if (previousState == 20) {
		ctx->gocPlayerHsm->hsm.ChangeState(111, 0);
		#if _DEBUG
		NOTIFY("Rolling from squat")
		#endif
	}

	if (previousState != 20) {
		ctx->gocPlayerHsm->hsm.ChangeState(107, 0);
		damageControl(ctx, false);
		#if _DEBUG
		NOTIFY("Rolling...")
		#endif
	}
}

HOOK(bool, __fastcall, Step_SpinAttack, 0x1406CB3E0, app::player::StateSpinAttack* self, app::player::PlayerHsmContext* ctx, float deltaTime) {
	rfl_idiotism::bounceRFL("restore");
	ctx->gocPlayerHsm->ChangeState(11, 0);
	return 1;
}

HOOK(bool, __fastcall, Step_DropDash, 0x1406AFFB0, app::player::StateDropDash* self, app::player::PlayerHsmContext* ctx, float deltaTime) {
	drop_driftdash_transition(ctx, deltaTime);
	return originalStep_DropDash(self, ctx, deltaTime);
}

HOOK(bool, __fastcall, Step_SpinDash, 0x1406CB790, app::player::StateSpinDash* self, app::player::PlayerHsmContext* ctx, float deltaTime) {
	drop_driftdash_transition(ctx, deltaTime); // for rolling from squat
	return originalStep_SpinDash(self, ctx, deltaTime);
}

HOOK(bool, __fastcall, Step_DriftDash, 0x1406AFB70, app::player::StateDriftDash* self, app::player::PlayerHsmContext* ctx, float deltaTime) {
	//bool OutOfControl = hh::game::GameManager::GetInstance()->GetService<app::level::LevelInfo>()->playerInformation->outOfControl_ns.value();
	//if (OutOfControl) {
	//	ctx->gocPlayerHsm->ChangeState(4, 0);
	//	return 1;
	//}
	damageControl(ctx, false);
	if (driftdash_inputs(ctx)) {
		return 1;
	}
	app::player::PlayerParameters* params = hh::fnd::ResourceManager::GetInstance()->GetResource<hh::fnd::ResReflectionT<app::player::PlayerParameters>>("player_common")->GetData();
	float vertVelocity = ctx->gocPlayerKinematicParams->velocity.y();
	float speed = hh::game::GameManager::GetInstance()->GetService<app::level::LevelInfo>()->playerInformation->Speed.value();

	if (speed > 72.5f) {
		ctx->blackboardStatus->SetStateFlag(app::player::BlackboardStatus::StateFlag::BOOST, 1);
	}
	if (vertVelocity < -1.0f) {
		if (speed > 90) {
			params->forwardView.driftDash.brake = 3.0f;
			params->whiteSpace.driftDash.brake = 3.0f;
			params->sideView.driftDash.brake = 3.0f;
			params->boss.driftDash.brake = 3.0f;
			return originalStep_DriftDash(self, ctx, deltaTime);
		}
		else {
			params->forwardView.driftDash.brake = vertVelocity * 0.5f;
			params->whiteSpace.driftDash.brake = vertVelocity;
			params->sideView.driftDash.brake = vertVelocity * 0.5f;
			params->boss.driftDash.brake = vertVelocity * 0.5f;
			return originalStep_DriftDash(self, ctx, deltaTime);
		}
	}
	if (vertVelocity > -1.0f && vertVelocity < 8.0f) {
		params->forwardView.driftDash.brake = 3.0f;
		params->whiteSpace.driftDash.brake = 3.0f;
		params->sideView.driftDash.brake = 3.0f;
		params->boss.driftDash.brake = 3.0f;
		return originalStep_DriftDash(self, ctx, deltaTime);
	}
	if (vertVelocity > 8.1f) {
		params->forwardView.driftDash.brake = vertVelocity * 0.5f;
		params->whiteSpace.driftDash.brake = vertVelocity * 0.5f;
		params->sideView.driftDash.brake = vertVelocity * 0.5f;
		params->boss.driftDash.brake = vertVelocity * 0.5f;
		return originalStep_DriftDash(self, ctx, deltaTime);
	}
	else {
		return originalStep_DriftDash(self, ctx, deltaTime);
	}
}

HOOK(void, __fastcall, Leave_DriftDash, 0x1406AF8D0, app::player::StateDriftDash* self, app::player::PlayerHsmContext* ctx, int nextState) {
	damageControl(ctx, true);
	originalLeave_DriftDash(self, ctx, nextState);
	ctx->playerObject->GetComponent<app::player::GOCPlayerCollider>()->GetPlayerCollision()->qword2C = 0.5f;
	
}

HOOK(bool, __fastcall, Step_Diving, 0x1406AD600, app::player::StateDiving* self, app::player::PlayerHsmContext* ctx, float deltaTime) {
	float currInput = hh::game::GameManager::GetInstance()->GetService<app::level::LevelInfo>()->playerInformation->leftStickInput.value().norm();
	if (currInput < deadZone) {
		if (divingIdleTimer < 5.0f && !divingIdleStatus) {
			divingIdleTimer += deltaTime;
			return originalStep_Diving(self, ctx, deltaTime);
		}
		if (divingIdleStatus) {
			divingIdleTimer += deltaTime;
			return originalStep_Diving(self, ctx, deltaTime);
		}
		if (divingIdleTimer > 2.0f && divingIdleStatus) {
			ctx->playerObject->GetComponent<hh::anim::GOCAnimator>()->ChangeState("SPIN"); // what am i even doing?
			divingIdleStatus = false;
			divingIdleTimer = 0.0f;
			return originalStep_Diving(self, ctx, deltaTime);
		}
		if (divingIdleTimer >= 5.0f && !divingIdleStatus) {
			ctx->playerObject->GetComponent<hh::anim::GOCAnimator>()->ChangeState("DIVE_IDLE_START");
			divingIdleStatus = true;
			divingIdleTimer = 0.0f;
			return originalStep_Diving(self, ctx, deltaTime);
		}
	}
	else {
		divingIdleStatus = false;
		divingIdleTimer = 0;
		return originalStep_Diving(self, ctx, deltaTime);
	}
}

HOOK(void, __fastcall, Leave_Diving, 0x1406ACEB0, app::player::StateDiving* self, app::player::PlayerHsmContext* ctx, int nextState) {
	divingIdleTimer = 0;
	divingIdleStatus = false;
	originalLeave_Diving(self, ctx, nextState);
}

HOOK(void, __fastcall, Enter_StompingDown, 0x14a85c590, app::player::StateStompingDown* self, app::player::PlayerHsmContext* ctx, int previousState) {
	float atitude = hh::game::GameManager::GetInstance()->GetService<app::level::LevelInfo>()->playerInformation->altitude.value();
	csl::math::Vector4 currVelocity = ctx->gocPlayerKinematicParams->velocity;
	float currX = currVelocity.x();
	float currZ = currVelocity.z();
	prevVelocity = csl::math::Vector4(currX, 0, currZ, 0);
	stompInput = hh::game::GameManager::GetInstance()->GetService<app::level::LevelInfo>()->playerInformation->leftStickInput.value();
#if _DEBUG
	NOTIFY("sample velocity on StompingDown");
	std::cout << "PrevX Enter is " << currX << std::endl;
	std::cout << "PrevZ Enter is " << currZ << std::endl;
#endif
	stompDownEnterId = previousState;
	Bouncinggg("sample", 1, 0);
	originalEnter_StompingDown(self, ctx, previousState);
	if (previousState == 24) {
		ctx->StopEffects(); // don't ask me about this
	}
	ctx->ChangeState("SPINJUMP");
	wasI = false;
}

HOOK(bool, __fastcall, Step_StompingDown, 0x1406d1760, app::player::StateStompingDown* self, app::player::PlayerHsmContext* ctx, float deltaTime) {
	bool crossTap = ctx->playerObject->GetComponent<hh::game::GOCInput>()->GetInputComponent()->actionMonitors[0].state & 512;
	
	if (crossTap) {
		ctx->gocPlayerHsm->ChangeState(10, 0);
		return 1;
	}
	csl::math::Vector2 currInput = hh::game::GameManager::GetInstance()->GetService<app::level::LevelInfo>()->playerInformation->leftStickInput.value();
	bool circleHold = ctx->playerObject->GetComponent<hh::game::GOCInput>()->GetInputComponent()->actionMonitors[7].state & 256;
	bool sideView = hh::game::GameManager::GetInstance()->GetService<app::level::LevelInfo>()->playerInformation->sideView_ns.value();
	bool OutOfControl = ctx->blackboardStatus->GetWorldFlag(app::player::BlackboardStatus::WorldFlag::OUT_OF_CONTROL);
	float yVel = ctx->gocPlayerKinematicParams->velocity.y();
	
	//float altitudeNow = hh::game::GameManager::GetInstance()->GetService<app::level::LevelInfo>()->playerInformation->altitude.value();
	//static float altitudeOneFrameAgo = 0;
	// 
	//if (altitudeOneFrameAgo > 0) {
	//	if (altitudeNow < fallToRollAtitude && altitudeNow != altitudeOneFrameAgo) {
	//		ctx->gocPlayerHsm->hsm.ChangeState(111, 0);
	//	}
	//}
	//if (altitudeOneFrameAgo != altitudeNow) {
	//	altitudeOneFrameAgo = altitudeNow;
	//}
	// 
	// i wanted Shadow to transit into rolling if he is too close to the ground
	// but, if there is no collision underneath him, game spits out the same altitude each frame
	// and in this case this code breaks...
	
	csl::math::Vector4 zero(0, yVel, 0, 0);
	
	if (!OutOfControl) {
		if (currInput.norm() > deadZone && circleHold) {
			pizdecSuperBiker3D(ctx, deltaTime);
			WantToBounce = true;
			return originalStep_StompingDown(self, ctx, deltaTime);
		}
		if (currInput.norm() > deadZone && !circleHold && !sideView) {
			pizdecSuperBiker3D(ctx, deltaTime);
			WantToBounce = false;
			return originalStep_StompingDown(self, ctx, deltaTime);
		}
		if (currInput.norm() < deadZone || (sideView && !circleHold)) {
			if (!wasI) { // do the things once
				prevVelocity = csl::math::Vector4(0, yVel, 0, 0); // Discard sample, to prevent 2 above cases running again
				ctx->gocPlayerKinematicParams->velocity = csl::math::Vector4(0, yVel, 0, 0); // Zero out horVelocity
				wasI = true;
				WantToBounce = false;
				ctx->StopEffects();
			}
		}
	}
	return originalStep_StompingDown(self, ctx, deltaTime);
}

HOOK(void, __fastcall, Leave_StompingDown, 0x14A85DF40, app::player::StateStompingDown* self, app::player::PlayerHsmContext* ctx, int nextState) {
	if ( (nextState != 11) || (nextState != 63) || (nextState != 9) || (nextState != 10) || (nextState != 24) ) {
		ctx->StopEffects();
	}

	return originalLeave_StompingDown(self, ctx, nextState);
}

HOOK(void, __fastcall, Enter_StompingLand, 0x1406D0430, app::player::StateStompingLand* self, app::player::PlayerHsmContext* ctx, int previousState) {
	ctx->gocPlayerKinematicParams->velocity = csl::math::Vector4(0, 0, 0, 0);
	originalEnter_StompingLand(self, ctx, previousState);
	if (ctx->playerObject->GetComponent<hh::game::GOCInput>()->GetInputComponent()->actionMonitors[7].state & 256 && WantToBounce) {	
		ctx->gocPlayerHsm->hsm.ChangeState(11);
	}

	return;
}

HOOK(bool, __fastcall, Step_StompingLand, 0x1406d1d30, app::player::StateStompingLand* self, app::player::PlayerHsmContext* ctx, float deltaTime) {
	auto* pGOCInput = ctx->playerObject->GetComponent<hh::game::GOCInput>()->GetInputComponent();
	
	if (pGOCInput->actionMonitors[7].state & 256 && WantToBounce) {
		originalStep_StompingLand(self, ctx, deltaTime);
		ctx->gocPlayerHsm->hsm.ChangeState(11);
		return 1;
	}
	else {
		originalStep_StompingLand(self, ctx, deltaTime);
	}
	return 0;
}

HOOK(bool, __fastcall, Step_GrindDoubleJump, 0x1406BEAB0, app::player::StateGrindDoubleJump* self, app::player::PlayerHsmContext* ctx, float deltaTime) {
	damageControl(ctx, 0);
	return originalStep_GrindDoubleJump(self, ctx, deltaTime);
}

HOOK(bool, __fastcall, ProcessMsg_GrindDoubleJump, 0x1406BE330, app::player::StateGrindDoubleJump* self, app::player::PlayerHsmContext* ctx, const hh::fnd::Message* message) {
	if (message->ID == hh::fnd::MessageID::EightFourNineSeven) {
		ctx->gocPlayerHsm->ChangeState(11, 0);
		return 1;
	}
	//if (message->ID == hh::fnd::MessageID::EightThreeThreeSix) {
	//	ctx->gocPlayerHsm->ChangeState(30, 0); // evil way, wanted to patch stateID, but it didn't work, blep
	//	//ctx->gocPlayerHsm->ChangeState(30, 0);
	//	return 1;
	//}
	else {
		return originalProcessMsg_GrindDoubleJump(self, ctx, message);
	}
}

HOOK(void, __fastcall, Enter_BounceJump, 0x1406C06E0, app::player::StateBounceJump* self, app::player::PlayerHsmContext* ctx, int previousState) {
	app::level::PlayerInformation* playerInfo = hh::game::GameManager::GetInstance()->GetService<app::level::LevelInfo>()->playerInformation;
	csl::math::Vector2 currentInput = playerInfo->leftStickInput.value();
	bool sideView = playerInfo->sideView_ns.value();

	bounceEnterInput = currentInput;
	bounceEnterId = previousState;
	#if _DEBUG
	std::cout << "Bounce previous State " << previousState << std::endl;
	#endif
	if (currentInput.norm() < deadZone) {
		Bouncinggg("null", 1, 0);
	}
	if (previousState == 107) {
		rolling = true;
	}
	if (previousState != 107) {
		rolling = false;
	}
	if (previousState == 55) {
		if (((stompInput.x() * currentInput.x()) < 0) ) 
		{
			Bouncinggg("apply", 1, 0);
		}
		else {
			Bouncinggg("apply", 1, 0); // if bouncing, then we hack to retain velocity
		}
#if _DEBUG
		NOTIFY("hacking bounce speed");
#endif
		originalEnter_BounceJump(self, ctx, previousState);
	}
	if (previousState != 55) {
		rfl_idiotism::jump_speed_func("apply");
		originalEnter_BounceJump(self, ctx, previousState);
#if _DEBUG	
		NOTIFY("or.. not");
#endif
	}
}

HOOK(bool, __fastcall, Step_BounceJump, 0x1406C1C80, app::player::StateBounceJump* self, app::player::PlayerHsmContext* ctx, float deltaTime) {
	bouncejump_inputs(ctx);
	csl::math::Vector2 currInput = hh::game::GameManager::GetInstance()->GetService<app::level::LevelInfo>()->playerInformation->leftStickInput.value();
	float horVelocity = hh::game::GameManager::GetInstance()->GetService<app::level::LevelInfo>()->playerInformation->Speed.value();
	float totalVelocity = ctx->gocPlayerKinematicParams->velocity.norm();
	if ((horVelocity > 80.0f || totalVelocity > 85.0f) && bounceEnterId == 107 ){
		ctx->blackboardStatus->SetStateFlag(app::player::BlackboardStatus::StateFlag::BOOST, 1);
	}
	else {
		ctx->blackboardStatus->SetStateFlag(app::player::BlackboardStatus::StateFlag::BOOST, 0);
	}
	if (!WantToBounce) {
		rfl_idiotism::jump_speed_func("apply");
	}
	if (bounceEnterId == 55)
	{
		float mult = currInput.norm() / bounceEnterInput.norm();
		if (mult >= 0.05 && currInput.norm() < bounceEnterInput.norm()) {
			Bouncinggg("apply", mult, 0);
		}
		if (mult < 0.05) {
			rfl_idiotism::jump_speed_func("apply");
		}
	}
	damageControl(ctx, 0);

	return originalStep_BounceJump(self,ctx,deltaTime);
}

HOOK(void, __fastcall, Leave_BounceJump, 0x1406C15F0, app::player::StateBounceJump* self, app::player::PlayerHsmContext* ctx, int nextState) {
	rfl_idiotism::jump_speed_func("restore");
	rfl_idiotism::bounceRFL("restore");
	damageControl(ctx, 1);
	originalLeave_BounceJump(self, ctx, nextState);
}

HOOK(void, __fastcall, Enter_Jump, 0x1406C0AB0, app::player::StateJump* self, app::player::PlayerHsmContext* ctx, int previousState) {
	app::level::PlayerInformation* playerInfo = hh::game::GameManager::GetInstance()->GetService<app::level::LevelInfo>()->playerInformation;
	float speed = playerInfo->Speed.value();
	jumpEnterNormal = playerInfo->groundNormal.value();
	//static bool jumpStartLR{};
	jumpTimer = 0.0f;
	jumpStatus = false;
	jumpAnim = false;
	// can someone 
	if (speed < 17) {
		WriteProtected<DWORD>(0x1406c0d3c, 0x00BA0620); // JUMP_UP (low speed)
		//WriteProtected<byte>(0x1406c0d3c, 0x20);
		//WriteProtected<byte>(0x1406c0d3d, 0x06);
		//WriteProtected<byte>(0x1406c0d3e, 0xBA);
	}
	else {
		WriteProtected<DWORD>(0x1406c0d3c, 0x00B95CB0); // JUMP_START (high speed)
		//WriteProtected<byte>(0x1406c0d3c, 0xB0);
		//WriteProtected<byte>(0x1406c0d3d, 0x5C);
		//WriteProtected<byte>(0x1406c0d3e, 0xB9);
	}
	ctx->playerObject->GetComponent<app::player::GOCPlayerVisual>()->SetAnimationVariableFloat("SPEED_RATIO", 0.45f);
	originalEnter_Jump(self, ctx, previousState);
	return;
}

HOOK(bool, __fastcall, Step_Jump, 0x14a7c1140, app::player::StateJump* self, app::player::PlayerHsmContext* ctx, float deltaTime) {
	// context
	app::level::PlayerInformation* playerInfo = hh::game::GameManager::GetInstance()->GetService<app::level::LevelInfo>()->playerInformation;
	
	float altitude = playerInfo->altitude.value();
	float speed = playerInfo->Speed.value();
	bool dWings = playerInfo->dWings.value();
	bool boost = ctx->blackboardStatus->GetStateFlag(app::player::BlackboardStatus::StateFlag::BOOST);
	float boostMin = ctx->playerObject->GetComponent<app::player::GOCPlayerParameter>()->GetBoostParameters().startSpeed;
	// input
	hh::game::InputComponent* inputComp = ctx->playerObject->GetComponent<hh::game::GOCInput>()->GetInputComponent();
	bool crossHold = inputComp->actionMonitors[0].state & 256;
	bool crossTap = inputComp->actionMonitors[0].state & 512;


	jumpTimer += deltaTime;
	if (crossTap)
	{
		пидарас(ctx);
		return 1;
	}
	// player jump decision
	if (jumpTimer < 0.12f)
	{
		if (!crossHold) {
			isShortHop = 1;  // tap = short hop
		}
		else {
			isShortHop = 0; // hold = full jump
		}
	}
	if (boost)
	{
		ctx->gocPlayerBlackboard->blackboard->GetContent<app::player::BlackboardBattle>()->SetFlag020(1); // Shadow gets damage (bullets and etc)
		ctx->gocPlayerHsm->statePluginManager->GetPlugin<app::player::StatePluginCollision>()->SetTypeAndRadius(1, 1); // Shadow deal damage 
	}
	if (!boost && isShortHop && jumpAnim) {
		ctx->gocPlayerBlackboard->blackboard->GetContent<app::player::BlackboardBattle>()->SetFlag020(1); // Shadow gets damage(bullets and etc)
		ctx->gocPlayerHsm->statePluginManager->GetPlugin<app::player::StatePluginCollision>()->SetTypeAndRadius(1, 0); // Shadow doesn't deal damage
	}
	if (!jumpAnim && isShortHop && jumpTimer > 0.5f) {
		ctx->playerObject->GetComponent<hh::anim::GOCAnimator>()->ChangeState("FALL");
		*(uint8_t*)((char*)self + 0x40) = 1u;
		return originalStep_Jump(self, ctx, deltaTime);
	}
	//shortHop
	if (isShortHop)
	{
		// high-speed shorthop
		if (!jumpAnim && speed > boostMin && !dWings && jumpTimer < 0.12f) {
			ctx->playerObject->GetComponent<hh::anim::GOCAnimator>()->ChangeState("AVOID_FRONT_AIR");
			jumpAnim = true; // do the thing once
		}
		if (altitude >= 1.25f && !jumpStatus) {
			csl::math::Vector3 velocity3 = ConvertTo3(ctx->gocPlayerKinematicParams->velocity);
			jumpEnterNormal.normalize();
			velocity3 -= jumpEnterNormal * velocity3.dot(jumpEnterNormal);
			velocity3 += jumpEnterNormal * 2.5f;
			ctx->gocPlayerKinematicParams->velocity = csl::math::Vector4(velocity3.x(), velocity3.y(), velocity3.z(), 0);
			jumpStatus = true; // reset it once
		}
		return originalStep_Jump(self, ctx, deltaTime);
	}
	// fullJump
	if (!isShortHop)
	{
		if (!jumpAnim && !dWings && jumpTimer >= 0.12f) {
			// We Spin after 0.12 seconds
			ctx->ChangeState("SPINJUMP");
			ctx->playerObject->GetComponent<app::player::GOCPlayerVisual>()->SetAnimationState("SPINJUMP", 0xFE);
			ctx->gocPlayerHsm->statePluginManager->GetPlugin<app::player::StatePluginCollision>()->SetTypeAndRadius(1, 1);
			*(uint8_t*)((char*)self + 0xF4) = 1u;
			jumpAnim = true;
		}
	}
	return originalStep_Jump(self, ctx, deltaTime);
}

HOOK(void, __fastcall, Leave_Jump, 0x1406c2b70, app::player::StateJump* self, app::player::PlayerHsmContext* ctx, int nextState) {
	jumpTimer = 0.0f;
	jumpStatus = false;
	jumpAnim = false;
	isShortHop = false;
	originalLeave_Jump(self, ctx, nextState);
}

HOOK(bool, __fastcall, Step_AirBoost, 0x14A670E80, app::player::StateAirBoost* self, app::player::PlayerHsmContext* ctx, float deltaTime) {
	if (ctx->DoHoming(0)) {
		return 1;
	}
	if (ctx->playerObject->GetComponent<hh::game::GOCInput>()->GetInputComponent()->actionMonitors[0].state & 512) {
		ctx->gocPlayerHsm->ChangeState(10, 0);
		return 1;
	}
	return originalStep_AirBoost(self, ctx, deltaTime);
	//if (ctx->playerObject->GetComponent<hh::game::GOCInput>()->GetInputComponent()->actionMonitors[7].state & 512) {
	//	ctx->ChangeState("SPINJUMP");
	//	return 1;
	//}
}

HOOK(void, __fastcall, Enter_Run, 0x1406C7000, app::player::StateRun* self, app::player::PlayerHsmContext* ctx, int previousState) {
	#if _DEBUG
	std::cout << "previous and bounceEnterID: " << previousState << " " << bounceEnterId << std::endl;
	#endif
	bool r2Hold = ctx->playerObject->GetComponent<hh::game::GOCInput>()->GetInputComponent()->actionMonitors[3].state & 256;
	auto* pParams = hh::fnd::ResourceManager::GetInstance()->GetResource<hh::fnd::ResReflectionT<heur::rfl::StandardCameraConfig>>("standard_camera_ws")->GetData();
	pParams->common.azimuthSensitivity = 10;
	if (previousState == 11 && bounceEnterId == 107) {
		ctx->gocPlayerHsm->hsm.ChangeState(107);
		return;
	}
	
	if (previousState == 9 && isShortHop && jumpAnim) {
		originalEnter_Run(self, ctx, previousState);
		ctx->playerObject->GetComponent<hh::anim::GOCAnimator>()->ChangeState("SPRING_LANDING"); // hmm
		return;
	}
	if (previousState == 24 && r2Hold) {
		ctx->blackboardStatus->SetStateFlag(app::player::BlackboardStatus::StateFlag::BOOST, 1);
	} 
	return originalEnter_Run(self, ctx, previousState);
}

HOOK(void, __fastcall, Enter_LightDash, 0x1406c2f80, app::player::StateLightDash* self, app::player::PlayerHsmContext* ctx, int previousState) {
	originalEnter_LightDash(self, ctx, previousState);
	ctx->gocPlayerHsm->statePluginManager->GetPlugin<app::player::StatePluginCollision>()->SetTypeAndRadius(2, 1);
	return;
}

HOOK(bool, __fastcall, Step_WallJump, 0x1406E0BC0, app::player::StateWallJump* self, app::player::PlayerHsmContext* ctx, float deltaTime) {
	wallJumpTimer += deltaTime;
	if (wallJumpTimer > 3.5f) {
		ctx->gocPlayerHsm->ChangeState(15, 0);
		return 1;
	}
	if (ctx->playerObject->GetComponent<hh::game::GOCInput>()->GetInputComponent()->actionMonitors[0].state & 512 && wallJumpTimer >= 0.25f) {
		пидарас(ctx);
		return 1;
	}
	if (ctx->playerObject->GetComponent<hh::game::GOCInput>()->GetInputComponent()->actionMonitors[3].state & 512) {
		ctx->gocPlayerHsm->ChangeState(24, 0);
		return 1;
	}
	else {
		return originalStep_WallJump(self, ctx, deltaTime);
	}
}

HOOK(void, __fastcall, BindMaps, 0x140A2FEC0, hh::game::GameManager* gameManager, hh::hid::InputMapSettings* inputSettings) {
	inputSettings->BindActionMapping("PlayerLightDash", 0x40004u);
	originalBindMaps(gameManager, inputSettings);
}

HOOK(void, __fastcall, bindAction, 0x140c0db60, hh::hid::InputMapSettings* self, const char* action, uint32_t inputID, signed int unk1) {
	originalbindAction(self ,action, inputID, unk1);
	printf(action);
	PRINT_INTEGER(inputID);
}

inline const char* decideHomingFinishAnim(Hedgehog player ,char counter) {
	switch (player) 
	{
		case Sonic: 
		{
			switch (counter) {
				case 0: return "ATTACK_BOUNCE";
				case 1: return "JUMP_TRICK_D0";
				case 2: return "JUMP_TRICK_L0";
				case 3: return "JUMP_TRICK_R0";
				case 4: return "JUMP_TRICK_D1";
				case 5: return "JUMP_TRICK_L1";
				case 6: return "JUMP_TRICK_R1";
				case 7: return "JUMP_TRICK_D2";
				case 8: return "JUMP_TRICK_L2";
				case 9: return "JUMP_TRICK_R2";
				default: return "ATTACK_BOUNCE";
			}
		}
		case Shadow: 
		{
			switch (counter) {
			    case 0: return "BUMP_JUMP_START";
				case 1: return "BUMP_JUMP_RETURN_L";
				case 2: return "BUMP_JUMP_START";
				case 3: return "ATTACK_BOUNCE";
				case 4: return "BUMP_JUMP_RETURN_D";
				case 5: return "BUMP_JUMP_RETURN_R";
				case 6: return "BUMP_JUMP_RETURN_U";
				default: return "ATTACK_BOUNCE";
			}
		}
		default: return "ATTACK_BOUNCE";
	}
}

HOOK(void, __fastcall, Enter_HomingFinished, 0x140697820, app::player::StateHomingFinished* self, app::player::PlayerHsmContext* ctx, int previousState) {
	originalEnter_HomingFinished(self, ctx, previousState);
	static char counter{};
	if (previousState == 120 || (previousState - 124 <= 4)) {
		if (ctx->blackboardStatus->dword28 == 3) {
			ctx->playerObject->GetComponent<app::player::GOCPlayerVisual>()->SetAnimationStateWithoutTransition("ATTACK_BOUNCE");
			*(uint8_t*)((char*)self + 0xE6) = 1;
			return;
		} // if movie Shadow
		switch (player) {
		case Sonic:
			ctx->playerObject->GetComponent<app::player::GOCPlayerVisual>()->SetAnimationVariableFloat("SPEED_RATIO", 0.75f);
			if (counter == 9) {
				counter = 0;
			}
			break;
		case Shadow:
			ctx->playerObject->GetComponent<app::player::GOCPlayerVisual>()->SetAnimationVariableFloat("SPEED_RATIO", 1.20f);
			if (counter == 6) {
				counter = 0;
			}
			break;
		}
		ctx->playerObject->GetComponent<app::player::GOCPlayerVisual>()->SetAnimationStateWithoutTransition(decideHomingFinishAnim(player, counter));
		/*ctx->playerObject->GetComponent<app::player::GOCPlayerVisual>()->SetAnimationStateWithoutTransition("JUMP_TRICK_D2");*/
		counter += 1;
		*(uint8_t*)((char*)self + 0xE6) = 1;
		return;
		
	}
	else {
		ctx->playerObject->GetComponent<app::player::GOCPlayerVisual>()->SetAnimationStateWithoutTransition("ATTACK_BOUNCE");
	}
}

HOOK(bool, __fastcall, Step_HomingFinished, 0x1406985B0, app::player::StateHomingFinished* self, app::player::PlayerHsmContext* ctx, float deltaTime) {
	uint8_t flag = *((uint8_t*)self + 229);
	printf("flag: %u\n", flag);
	return originalStep_HomingFinished(self, ctx, deltaTime);
}

HOOK(void, __fastcall, Enter_Drift, 0x1, app::player::StateDrift* self, app::player::PlayerHsmContext* ctx, int previousState) {

}

HOOK(bool, __fastcall, Step_Drift, 0x1406AF920, app::player::StateDrift* self, app::player::PlayerHsmContext* ctx, float deltaTime) {
	static float timer{};
	auto* pParams = hh::fnd::ResourceManager::GetInstance()->GetResource<hh::fnd::ResReflectionT<heur::rfl::StandardCameraConfig>>("standard_camera_ws")->GetData();
	pParams->common.azimuthSensitivity = 500;
	if (ctx->blackboardStatus->GetWorldFlag(app::player::BlackboardStatus::WorldFlag::OUT_OF_CONTROL)) {
		ctx->gocPlayerHsm->ChangeState(4, 0);
		timer = 0;
		return 1;
	}
	
	app::level::PlayerInformation* playerInfo = hh::game::GameManager::GetInstance()->GetService<app::level::LevelInfo>()->playerInformation;
	float speed = playerInfo->Speed.value();
	hh::game::InputComponent* inputComp = ctx->playerObject->GetComponent<hh::game::GOCInput>()->GetInputComponent();
	bool circleDown = inputComp->actionMonitors[7].state & 256;
	bool r2Hold = inputComp->actionMonitors[3].state & 256;
	bool r2Tap = inputComp->actionMonitors[3].state & 512;
	bool crossTap = inputComp->actionMonitors[0].state & 512;
	
	if (originalStep_Drift(self, ctx, deltaTime)) {
		timer = 0;
		return 1;
	}
	if (abs(playerInfo->leftStickInput.value().x()) > 0.075f) {
		float value = speed / 7.5f;
		if (value > 5.5f) {
			value = 5.5f;
		}
		ctx->playerObject->GetComponent<app::player::GOCPlayerVisual>()->SetAnimationVariableFloat("RUN_SPEED", value );
	}
	else{
		ctx->playerObject->GetComponent<app::player::GOCPlayerVisual>()->SetAnimationVariableFloat("RUN_SPEED", 1);
	}
	if (!playerInfo->isGrounded.value()) {
		ctx->gocPlayerHsm->ChangeState(15, 0);
		timer = 0;
		return 1;
	}
	if (!circleDown) {
		ctx->gocPlayerHsm->ChangeState(4, 10);
		timer = 0;
		return 1;
	}
	if (r2Tap) {
		/*ctx->gocPlayerBlackboard->blackboard->GetContent<app::player::BlackboardStatus>()->SetStateFlag(app::player::BlackboardStatus::StateFlag::BOOST, 1);*/
		//ctx->gocPlayerHsm->statePluginManager->GetPlugin<app::player::StatePluginBoost>()->Boost();
	}
	if (r2Hold) {
		ctx->gocPlayerBlackboard->blackboard->GetContent<app::player::BlackboardStatus>()->SetStateFlag(app::player::BlackboardStatus::StateFlag::BOOST, 1);
	}
	if (crossTap) {
		ctx->gocPlayerHsm->ChangeState(9, 0);
		return 1;
	}
}

HOOK(bool, __fastcall, ProcessMsg_Jump, 0x1406C1AD0, app::player::StateJump* self, app::player::PlayerHsmContext* ctx, const hh::fnd::Message* message) {
	//PRINT_INTEGER(static_cast<uint32_t>(message->ID));
	return originalProcessMsg_Jump(self, ctx, message);
}

BOOL WINAPI DllMain(_In_ HINSTANCE hInstance, _In_ DWORD reason, _In_ LPVOID reserved)
{
	switch (reason)
	{
	case DLL_PROCESS_ATTACH:
		Patches::Patch();
		SetHedgehog();

		INSTALL_HOOK(InitPlayer);

		INSTALL_HOOK(Enter_Sliding);
		INSTALL_HOOK(Step_SpinAttack);
		INSTALL_HOOK(Step_DropDash);
		INSTALL_HOOK(Step_SpinDash);
		INSTALL_HOOK(Step_DriftDash);
		INSTALL_HOOK(Leave_DriftDash);
		INSTALL_HOOK(Step_Diving);
		INSTALL_HOOK(Leave_Diving);
		INSTALL_HOOK(Enter_StompingDown);
		INSTALL_HOOK(Step_StompingDown);
		INSTALL_HOOK(Leave_StompingDown);
		INSTALL_HOOK(Enter_StompingLand);
		INSTALL_HOOK(Step_GrindDoubleJump);	
		INSTALL_HOOK(ProcessMsg_GrindDoubleJump);
		INSTALL_HOOK(Enter_BounceJump);
		INSTALL_HOOK(Step_BounceJump);
		INSTALL_HOOK(Leave_BounceJump);
		INSTALL_HOOK(Enter_Jump);
		INSTALL_HOOK(Step_Jump);
		//INSTALL_HOOK(ProcessMsg_Jump);
		INSTALL_HOOK(Leave_Jump);
		INSTALL_HOOK(Step_AirBoost);
		INSTALL_HOOK(Enter_Run);
		INSTALL_HOOK(Enter_LightDash);
		INSTALL_HOOK(Step_WallJump);
		INSTALL_HOOK(BindMaps);
		INSTALL_HOOK(bindAction);
		INSTALL_HOOK(Enter_HomingFinished);
		//INSTALL_HOOK(Step_HomingFinished);
		INSTALL_HOOK(Step_Drift);
		
		break;
	case DLL_PROCESS_DETACH:
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
		break;
	}

	return TRUE;
}
