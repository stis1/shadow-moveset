#define NOTIFY(str, ...) \
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_GREEN | FOREGROUND_INTENSITY); \
    std::cout << "[Shadow Moveset]: " << str << std::endl; \
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
#include <iostream>
#include <optional>
#include <fstream>

static csl::math::Vector4 prevVelocity{};
static bool WantToBounce{};
static bool wasI{};
static bool rolling{};
static csl::math::Vector2 bounceEnterInput{};
static int bounceEnterId{};
static int stompDownEnterId{};

static float divingIdleTimer{};
static bool divingIdleStatus{};
static float jumpTimer{};
static bool jumpStatus{};
static bool jumpAnimStatus{};
static float wallJumpTimer{};
// i want kms because of these timers

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

template<typename T>
void WriteProtected(uintptr_t address, T value) {
	DWORD oldProtect;
	VirtualProtect(reinterpret_cast<void*>(address), sizeof(T), PAGE_EXECUTE_READWRITE, &oldProtect);
	*reinterpret_cast<T*>(address) = value;
	VirtualProtect(reinterpret_cast<void*>(address), sizeof(T), oldProtect, &oldProtect);
}

void Leave_LightDash(app::player::StateLightDash* self, app::player::PlayerHsmContext* ctx, int nextState);

void Leave_WallJump(app::player::StateWallJump* self, app::player::PlayerHsmContext* ctx, int nextState);

void patch_Step_StateWallJump() {
	// replacing jz with jnz iirc to avoid StateWallJump transiting into StateFall for some reason
	WriteProtected<byte>(0x01406e0bd6, 0x75);
}

void patch_Step_StateJump() {
	// replacing jbe that'd transit statejump into ball animation, with doing nothing, behavior overriden in hook
	WriteProtected<byte>(0x14A7C123A, 0xE9); // jmp
	WriteProtected<byte>(0x14A7C123B, 0x43); // where
	WriteProtected<byte>(0x14A7C123C, 0x01);
	WriteProtected<byte>(0x14A7C123D, 0x00);
	WriteProtected<byte>(0x14A7C123E, 0x00);
	WriteProtected<byte>(0x14A7C123F, 0x90); // nop
}

void patch_Leave_StateWallJump() {
	WriteProtected<void*>(0x1412631A0, &Leave_WallJump);
}

void patch_Leave_StateLightDash() {
	WriteProtected<void*>(0x14125BED8, &Leave_LightDash);
}

void Leave_WallJump(app::player::StateWallJump* self, app::player::PlayerHsmContext* ctx, int nextState) {
	wallJumpTimer = 0.0f;
	return;
}

void Leave_LightDash(app::player::StateLightDash* self, app::player::PlayerHsmContext* ctx, int nextState) {
	// Thank You Ash and Zor
	ctx->gocPlayerBlackboard->blackboard->GetContent<app::player::BlackboardBattle>()->SetFlag020(1);
	ctx->gocPlayerHsm->statePluginManager->GetPlugin<app::player::StatePluginCollision>()->SetTypeAndRadius(2, 0);
	return;
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
	auto* pGOCPlayerHsm = ctx->playerObject->GetComponent<app::player::GOCPlayerHsm>();
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
			pGOCPlayerHsm->ChangeState(4, 0);
			ctx->blackboardStatus->SetStateFlag(app::player::BlackboardStatus::StateFlag::BOOST, 1);
			damageControl(ctx, 0);
#if _DEBUG
			NOTIFY("DIRECTED TO BOOST");
#endif
			break;
		case homing:
			pGOCPlayerHsm->ChangeState(63, 0);
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

void driftdash_inputs(app::player::PlayerHsmContext* ctx) {
	bool isGrounded = hh::game::GameManager::GetInstance()->GetService<app::level::LevelInfo>()->playerInformation->isGrounded.value();
	if (!isGrounded) {
		Redirect(ctx, bouncejump);
		return;
	}
	// player input
	auto* pGOCInput = ctx->playerObject->GetComponent<hh::game::GOCInput>()->GetInputComponent();
	 // 3 is action, 256 or 512 is whether held or tapped
	bool circleTap = pGOCInput->actionMonitors[7].state & 512;
	bool crossTap = pGOCInput->actionMonitors[0].state & 512;

	if (circleTap) {
		Redirect(ctx, run);
		return;
	}
	if (crossTap) {
		Redirect(ctx, jump);
		return;
	}
	bool boostHeld = pGOCInput->actionMonitors[3].state & 256;
	bool boostTap = pGOCInput->actionMonitors[3].state & 512;
	if (boostHeld) // if boost button is pressed, don't give a choice to return to boost // 
	{
		ctx->blackboardStatus->SetStateFlag(app::player::BlackboardStatus::StateFlag::BOOST, 0);
		if (boostTap) 
		{
			Redirect(ctx, boost);
			return;
		}
	}
	else // i am not using one func to delegate this, because if boostHeld is not there, then game has higher % crashes 
	{
		if (boostTap || boostHeld) 
		{
			Redirect(ctx, boost);
			return;
		}
	}

	return;
}

void bouncejump_inputs(app::player::PlayerHsmContext* ctx) {
	if (ctx->DoHoming(0)) {
		return;
	}
	
	auto* pGOCInput = ctx->playerObject->GetComponent<hh::game::GOCInput>()->GetInputComponent();
	float Speed = hh::game::GameManager::GetInstance()->GetService<app::level::LevelInfo>()->playerInformation->Speed.value();
	
	if (pGOCInput->actionMonitors[7].state & 512 && rolling) {
		ctx->gocPlayerHsm->ChangeState(4, 0);
		return;
	}
	if (pGOCInput->actionMonitors[0].state & 512) {
		ctx->gocPlayerHsm->ChangeState(10, 0);
		return;
	}
	if (pGOCInput->actionMonitors[7].state & 512) {
		ctx->gocPlayerHsm->ChangeState(52, 0);
		return;
	}
	if (pGOCInput->actionMonitors[3].state & 256 && Speed > 45) {
		ctx->blackboardStatus->SetStateFlag(app::player::BlackboardStatus::StateFlag::BOOST, 1);
		damageControl(ctx, false);
		return;
	}
	//if (inputcomp->actionMonitors[7].state & 256 && Timer > 0.15f) {
	//	Redirect(ctx, StateDirector::bouncejump);
	//}
}

void drop_driftdash_transition(app::player::PlayerHsmContext* ctx, float deltaTime) {
	int getID = ctx->gocPlayerHsm->GetCurrentState();

	static float DDash_Timer = 0.0f;

	if (DDash_Timer < 0.15f && getID != 111) {
		DDash_Timer += deltaTime;
		ctx->gocPlayerHsm->ChangeState(109, 0);
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
	float prevX = prevVelocity.x();
	float prevZ = prevVelocity.z();
	if (prevX == 0 && prevZ == 0) {
		return; // discarded samples, return immediately;
	}

	float Y = ctx->gocPlayerKinematicParams->velocity.y();
	const float bounceBreakForce = 20.0f;

	csl::math::Vector4 horVector(prevX, Y, prevZ, 0);
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
		pParams->whiteSpace.jumpSpeed.limitUpSpeed = SampleSpeed;
		pParams->forwardView.jumpSpeed.limitUpSpeed = SampleSpeed;
		pParams->sideView.jumpSpeed.limitUpSpeed = SampleSpeed;
		pParams->boss.jumpSpeed.limitUpSpeed = SampleSpeed;
		if (directionChanged) {
			pParams->whiteSpace.jumpSpeed.minStickSpeed = SampleSpeed*0.5f;
			pParams->forwardView.jumpSpeed.minStickSpeed = SampleSpeed*0.5f;
			pParams->sideView.jumpSpeed.minStickSpeed = SampleSpeed*0.5f;
			pParams->boss.jumpSpeed.minStickSpeed = SampleSpeed*0.5f;
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
			pParams->sideView.jumpSpeed.brake = 20;
			pParams->boss.jumpSpeed.brake = 50;
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
	
	auto* pBBstatus = self->GetComponent<app::player::GOCPlayerBlackboard>()->blackboard->GetContent<app::player::BlackboardStatus>();
	//rfl_idiotism::player_common_tweak();
	rfl_idiotism::jump_speed_func("backup");
	rfl_idiotism::bounceRFL("backup");
}

HOOK(void, __fastcall, Enter_Sliding, 0x1406CBB70, app::player::StateSliding* self, app::player::PlayerHsmContext* ctx, int previousState) {
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
	driftdash_inputs(ctx);
	auto* params = hh::fnd::ResourceManager::GetInstance()->GetResource<hh::fnd::ResReflectionT<app::player::PlayerParameters>>("player_common")->GetData();
	float vertVelocity = ctx->gocPlayerKinematicParams->velocity.y();
	float speed = hh::game::GameManager::GetInstance()->GetService<app::level::LevelInfo>()->playerInformation->Speed.value();

	if (speed > 72.5f) {
		ctx->blackboardStatus->SetStateFlag(app::player::BlackboardStatus::StateFlag::BOOST, 1);
	}
	if (vertVelocity < -1.0f) {
		params->forwardView.driftDash.brake = vertVelocity * 0.5f;
		params->whiteSpace.driftDash.brake = vertVelocity;
		params->sideView.driftDash.brake = vertVelocity * 0.5f;
		params->boss.driftDash.brake = vertVelocity * 0.5f;
		return originalStep_DriftDash(self, ctx, deltaTime);
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
			ctx->playerObject->GetComponent<hh::anim::GOCAnimator>()->ChangeState("SPIN");
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
		return 0;
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
	if (nextState != 11 || nextState != 63 || nextState != 9 || nextState != 10 || nextState != 24) {
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
	}
	else {
		originalStep_StompingLand(self, ctx, deltaTime);
	}
	return 1;
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
	csl::math::Vector2 currentInput = hh::game::GameManager::GetInstance()->GetService<app::level::LevelInfo>()->playerInformation->leftStickInput.value();
	
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
	if (previousState == 55 /*&& currentInput.norm() > deadZone.norm()*/) {
		Bouncinggg("apply", 1, 0); // if bouncing, then we hack to retain velocity
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
	csl::math::Vector2 currInput = hh::game::GameManager::GetInstance()->GetService<app::level::LevelInfo>()->playerInformation->leftStickInput.value();
	float speed = hh::game::GameManager::GetInstance()->GetService<app::level::LevelInfo>()->playerInformation->Speed.value();
	float speed2 = ctx->gocPlayerKinematicParams->velocity.norm();
	if ((speed > 80.0f || speed2 > 85.0f) && bounceEnterId== 107 ){
		ctx->blackboardStatus->SetStateFlag(app::player::BlackboardStatus::StateFlag::BOOST, 1);
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
	bouncejump_inputs(ctx);
	originalStep_BounceJump(self, ctx, deltaTime);
	damageControl(ctx, 0);

	return 1;
}

HOOK(void, __fastcall, Leave_BounceJump, 0x1406C15F0, app::player::StateBounceJump* self, app::player::PlayerHsmContext* ctx, int nextState) {
	rfl_idiotism::jump_speed_func("restore");
	rfl_idiotism::bounceRFL("restore");
	damageControl(ctx, 1);
	originalLeave_BounceJump(self, ctx, nextState);
}

HOOK(void, __fastcall, Enter_Jump, 0x1406C0AB0, app::player::StateJump* self, app::player::PlayerHsmContext* ctx, int previousState) {
	float speed = hh::game::GameManager::GetInstance()->GetService<app::level::LevelInfo>()->playerInformation->Speed.value();
	jumpTimer = 0.0f;
	jumpStatus = false;
	jumpAnimStatus = false;
	// can someone 
	if (speed < 17) {
		WriteProtected<byte>(0x1406c0d3c, 0x20); // JUMP_UP
		WriteProtected<byte>(0x1406c0d3d, 0x06);
		WriteProtected<byte>(0x1406c0d3e, 0xBA);
	}
	else {
		WriteProtected<byte>(0x1406c0d3c, 0xB0); // JUMP_START
		WriteProtected<byte>(0x1406c0d3d, 0x5C);
		WriteProtected<byte>(0x1406c0d3e, 0xB9);
	}
	originalEnter_Jump(self, ctx, previousState);
	return;
}

HOOK(bool, __fastcall, Step_Jump, 0x14a7c1140, app::player::StateJump* self, app::player::PlayerHsmContext* ctx, float deltaTime) {
	bool crossHold = ctx->playerObject->GetComponent<hh::game::GOCInput>()->GetInputComponent()->actionMonitors[0].state & 256;
	bool crossTap = ctx->playerObject->GetComponent<hh::game::GOCInput>()->GetInputComponent()->actionMonitors[0].state & 512;
	bool boost = ctx->blackboardStatus->GetStateFlag(app::player::BlackboardStatus::StateFlag::BOOST);
	float altitude = hh::game::GameManager::GetInstance()->GetService<app::level::LevelInfo>()->playerInformation->altitude.value();
	jumpTimer += deltaTime;	
	if (!jumpAnimStatus) {
		ctx->gocPlayerBlackboard->blackboard->GetContent<app::player::BlackboardBattle>()->SetFlag020(1);
	}
	if (boost) {
		ctx->gocPlayerBlackboard->blackboard->GetContent<app::player::BlackboardBattle>()->SetFlag020(1);
		ctx->gocPlayerHsm->statePluginManager->GetPlugin<app::player::StatePluginCollision>()->SetTypeAndRadius(1, 1);
	}
	//if (!jumpAnimStatus && jumpTimer >= 0.125f) {
	//	ctx->ChangeState("SPINJUMP");
	//	ctx->playerObject->GetComponent<app::player::GOCPlayerVisual>()->SetAnimationState("SPINJUMP", 0xFE);
	//	ctx->gocPlayerHsm->statePluginManager->GetPlugin<app::player::StatePluginCollision>()->SetTypeAndRadius(1, 1);
	//	jumpAnimStatus = true;
	//}
	if (jumpStatus) {
		return originalStep_Jump(self, ctx, deltaTime);
	}
	if (crossTap) {
		ctx->gocPlayerHsm->ChangeState(11, 0);
		return 1;
	}
	if (!crossHold) {
		if (altitude < 0.25f && !jumpStatus) {
			return originalStep_Jump(self, ctx, deltaTime);
		}
		float speed = hh::game::GameManager::GetInstance()->GetService<app::level::LevelInfo>()->playerInformation->Speed.value();
		if (speed > 45 && !jumpAnimStatus) {
			//ctx->ChangeState("SPINJUMP");
			//ctx->playerObject->GetComponent<app::player::GOCPlayerVisual>()->SetAnimationState("QUICK_TURN", 0xFE);
			ctx->playerObject->GetComponent<hh::anim::GOCAnimator>()->ChangeState("QUICK_TURN");
			ctx->gocPlayerHsm->statePluginManager->GetPlugin<app::player::StatePluginCollision>()->SetTypeAndRadius(1, 1);
			jumpAnimStatus = true;
		}
		if (jumpTimer >= 0.085f && !jumpAnimStatus) {
			ctx->ChangeState("SPINJUMP");
			ctx->playerObject->GetComponent<app::player::GOCPlayerVisual>()->SetAnimationState("SPINJUMP", 0xFE);
			ctx->gocPlayerHsm->statePluginManager->GetPlugin<app::player::StatePluginCollision>()->SetTypeAndRadius(1, 1);
			jumpAnimStatus = true;
		}
		if (altitude > 0.25f && !jumpStatus) {
			csl::math::Vector4 velocity = ctx->gocPlayerKinematicParams->velocity;
			ctx->gocPlayerKinematicParams->velocity = csl::math::Vector4(velocity.x(), 5.0f, velocity.z(), 0);
			jumpStatus = true;
		}
		return originalStep_Jump(self, ctx, deltaTime);
	}
	if (jumpTimer >= 0.125f && !jumpAnimStatus) {
		jumpStatus = true;
		ctx->ChangeState("SPINJUMP");
		ctx->playerObject->GetComponent<app::player::GOCPlayerVisual>()->SetAnimationState("SPINJUMP", 0xFE);
		ctx->gocPlayerHsm->statePluginManager->GetPlugin<app::player::StatePluginCollision>()->SetTypeAndRadius(1, 1);
		jumpAnimStatus = true;
		return originalStep_Jump(self, ctx, deltaTime);
	}
	return originalStep_Jump(self, ctx, deltaTime);
	//float preActionTime = ctx->playerObject->GetComponent<app::player::GOCPlayerParameter>()->GetJumpParameters().preActionTime;
	//if (!crossHold && !jumpStatus) {
	//	if (altitude < 0.25f && jumpTimer < 1.0f) {
	//		return originalStep_Jump(self, ctx, deltaTime);
	//	}
	//	if (altitude > 0.25f && jumpTimer < 0.075f) {
	//		// nullify vertical speed pre-Frontiers style
	//		csl::math::Vector4 velocity = ctx->gocPlayerKinematicParams->velocity;
	//		ctx->gocPlayerKinematicParams->velocity = csl::math::Vector4(velocity.x(), 5.0f, velocity.z(), 0);
	//		jumpStatus = true;
	//		return originalStep_Jump(self, ctx, deltaTime);
	//	}
	//	if (jumpTimer >= 0.75f) {
	//		ctx->ChangeState("SPINJUMP");
	//		ctx->playerObject->GetComponent<app::player::GOCPlayerVisual>()->SetAnimationState("SPINJUMP", 0xFE);
	//		ctx->gocPlayerHsm->statePluginManager->GetPlugin<app::player::StatePluginCollision>()->SetTypeAndRadius(1, 1);
	//		jumpStatus = true;
	//	}
	//	return originalStep_Jump(self, ctx, deltaTime);
	//}
	//if (jumpTimer >= preActionTime && !jumpStatus) {
	//	jumpStatus = true;
	//	ctx->ChangeState("SPINJUMP");
	//	ctx->playerObject->GetComponent<app::player::GOCPlayerVisual>()->SetAnimationState("SPINJUMP", 0xFE);
	//	ctx->gocPlayerHsm->statePluginManager->GetPlugin<app::player::StatePluginCollision>()->SetTypeAndRadius(1, 1);
	//	return originalStep_Jump(self, ctx, deltaTime);
	//}
	//else {
	//	return originalStep_Jump(self, ctx, deltaTime);
	//}
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
	if (previousState == 11 && bounceEnterId == 107) {
		ctx->gocPlayerHsm->hsm.ChangeState(107);
		return;
	}
	bool r2Hold = ctx->playerObject->GetComponent<hh::game::GOCInput>()->GetInputComponent()->actionMonitors[3].state & 256;
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
		ctx->gocPlayerHsm->ChangeState(10, 0);
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

BOOL WINAPI DllMain(_In_ HINSTANCE hInstance, _In_ DWORD reason, _In_ LPVOID reserved)
{
	switch (reason)
	{
	case DLL_PROCESS_ATTACH:
		patch_Step_StateWallJump();
		patch_Step_StateJump();
		patch_Leave_StateWallJump();
		patch_Leave_StateLightDash();
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
		INSTALL_HOOK(Step_AirBoost);
		INSTALL_HOOK(Enter_Run);
		INSTALL_HOOK(Enter_LightDash);
		INSTALL_HOOK(Step_WallJump);
		
		break;
	case DLL_PROCESS_DETACH:
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
		break;
	}

	return TRUE;
}
