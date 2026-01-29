#define NOTIFY(str, ...) \
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_GREEN | FOREGROUND_INTENSITY); \
    std::cout << "[Shadow Moveset]: " << str << "\n" << std::endl; \
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
#include <iostream>
#include <optional>

static Eigen::Vector4f prevVelocity;

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

	if (getID == 107 || getID == 10 || getID == 9 || getID == 46) {
		isInBallState = true;
	}
	else {
		isInBallState = false;
	}

	if (isBoosting || isInBallState) {
		collision->SetTypeAndRadius(2, 1);
		BBbattle->SetFlag020(false);
		return;
	}
	if (getID == 11) {
		collision->SetTypeAndRadius(1, 1);
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

void bounceRFL(std::string arg) {
	auto* playerCommonRFL = hh::fnd::ResourceManager::GetInstance()->GetResource<hh::fnd::ResReflectionT<app::player::PlayerParameters>>("player_common");
	auto* params = playerCommonRFL->GetData();

	static float WS_B = 0;
	static float FV_B = 0;
	static float SV_B = 0;
	static float BS_B = 0;

	if (arg == "backup") {
		WS_B = params->whiteSpace.doubleJump.bounceSpeed;
		FV_B = params->forwardView.doubleJump.bounceSpeed;
		SV_B = params->sideView.doubleJump.bounceSpeed;
		BS_B = params->boss.doubleJump.bounceSpeed;
	}
	if (arg == "restore") {
		params->whiteSpace.doubleJump.bounceSpeed = WS_B;
		params->forwardView.doubleJump.bounceSpeed = FV_B;
		params->sideView.doubleJump.bounceSpeed = SV_B;
		params->boss.doubleJump.bounceSpeed = BS_B;
	}
}

void rolling_air_hack(app::player::PlayerHsmContext* ctx) {
	auto* playerCommonRFL = hh::fnd::ResourceManager::GetInstance()->GetResource<hh::fnd::ResReflectionT<app::player::PlayerParameters>>("player_common");
	auto* params = playerCommonRFL->GetData();

	csl::math::Vector4 velocity_m = ctx->gocPlayerKinematicParams->velocity;
	float vMagnitude = velocity_m.y();

	if (params) {
		params->whiteSpace.doubleJump.bounceSpeed = vMagnitude*0.5f;
		params->forwardView.doubleJump.bounceSpeed = vMagnitude*0.5f;
		params->sideView.doubleJump.bounceSpeed = vMagnitude*0.5f;
		params->boss.doubleJump.bounceSpeed = vMagnitude*0.5f;
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
			NOTIFY("DIRECTED TO DRIFTDASH");
			break;
		case StateDirector::bouncejump:
			HSMComp->ChangeState(11, 0);
			rolling_air_hack(ctx);
			tryDamage(ctx, false);
			NOTIFY("DIRECTED TO BOUNCEJUMP");
			break;
		case StateDirector::jump:
			HSMComp->ChangeState(9,0);
			NOTIFY("DIRECTED TO JUMP");
			break;
		case StateDirector::run:
			HSMComp->ChangeState(4, 0);
			NOTIFY("DIRECTED TO RUN");
			break;
		case StateDirector::boost:
			HSMComp->ChangeState(4, 0);
			ctx->blackboardStatus->SetStateFlag(app::player::BlackboardStatus::StateFlag::BOOST, 1);
			tryDamage(ctx, 0);
			NOTIFY("DIRECTED TO BOOST");
			break;
		case StateDirector::homing:
			HSMComp->ChangeState(63, 0);
			NOTIFY("HOMING SHADOW");
			break;
		case StateDirector::preserveboost: // for boosting inside slalom, maybe others, bad implementation if being honest
			ctx->blackboardStatus->SetStateFlag(app::player::BlackboardStatus::StateFlag::BOOST, 1);
			tryDamage(ctx, false);
			NOTIFY("KEPT BOOST");
			break;
		case StateDirector::doublejump:
			HSMComp->ChangeState(10, 0);
			NOTIFY("DIRECTED TO DOUBLE JUMP");
			break;
		case StateDirector::stomp:
			HSMComp->ChangeState(52, 0);
			NOTIFY("DIRECTED TO STOMP");
			break;
		case StateDirector::falls:
			HSMComp->ChangeState(15, 0);
			NOTIFY("Shadow shall fall");
			break;
		default:
			break;
	}
	return;
}

void driftdash_inputs(app::player::PlayerHsmContext* ctx) {
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
	if (circleTap && isGrounded == 1) {
		Redirect(ctx, StateDirector::run);
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

void bouncejump_inputs(app::player::PlayerHsmContext* ctx) {
	auto* input = ctx->playerObject->GetComponent<hh::game::GOCInput>();
	auto* inputcomp = input->GetInputComponent();
	auto* HSMComp = ctx->playerObject->GetComponent<app::player::GOCPlayerHsm>();

	if (inputcomp->actionMonitors[1].state & 512) {
		Redirect(ctx, StateDirector::homing);
	}
	if (inputcomp->actionMonitors[0].state & 512) {
		Redirect(ctx, StateDirector::doublejump);
	}
	if (inputcomp->actionMonitors[7].state & 512) {
		HSMComp->ChangeState(52, 0);
	}
	if (inputcomp->actionMonitors[3].state & 256) {
		Redirect(ctx, StateDirector::preserveboost);
	}
	//if (inputcomp->actionMonitors[7].state & 256 && Timer > 0.15f) {
	//	Redirect(ctx, StateDirector::bouncejump);
	//}
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

void drop_driftdash_transition(app::player::PlayerHsmContext* ctx, float deltaTime)
{
	auto* HSMComp = ctx->playerObject->GetComponent<app::player::GOCPlayerHsm>();
	int getID = HSMComp->GetCurrentState();
	auto* gameManager = hh::game::GameManager::GetInstance();
	auto* lvlInfo = gameManager->GetService<app::level::LevelInfo>();

	static float DDash_Timer = 0.0f;

	if (DDash_Timer < 0.15f && getID != 111) {
		DDash_Timer += deltaTime;
		HSMComp->ChangeState(109, 0);
		NOTIFY("put to drop for")
		return;
	}

	if (DDash_Timer < 0.15f && getID == 109 || getID == 111) {
		DDash_Timer += deltaTime;
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
	params->whiteSpace.driftDash.brake = 3.0f;
	params->whiteSpace.driftDash.checkDashSpeed = 15.0f;
	params->whiteSpace.driftDash.checkDashTime = 0.45f;
	params->whiteSpace.driftDash.endSteeringSpeed = 120.0f;
	params->whiteSpace.driftDash.maxSpeed = 100.0f;
	params->whiteSpace.driftDash.outOfControlSpeed = 80.0f;
	params->whiteSpace.spindash.minSpeed = 20.0f;
	// forwardView
	params->forwardView.spinAttack.limitSpeedMax = 80.0f;
	params->forwardView.spinAttack.brakeForce = 0.0f;
	params->forwardView.driftDash.brake = 3.0f;
	params->forwardView.driftDash.checkDashSpeed = 15.0f;
	params->forwardView.driftDash.checkDashTime = 0.45f;
	params->forwardView.driftDash.endSteeringSpeed = 120.0f;
	params->forwardView.driftDash.maxSpeed = 100.0f;
	params->forwardView.driftDash.outOfControlSpeed = 80.0f;
	params->forwardView.spindash.minSpeed = 20.0f;
	// sideView
	params->sideView.spinAttack.limitSpeedMax = 80.0f;
	params->sideView.spinAttack.brakeForce = 0.0f;
	params->sideView.driftDash.brake = 3.0f;
	params->sideView.driftDash.checkDashSpeed = 15.0f;
	params->sideView.driftDash.checkDashTime = 0.45f;
	params->sideView.driftDash.endSteeringSpeed = 120.0f;
	params->sideView.driftDash.maxSpeed = 100.0f;
	params->sideView.driftDash.outOfControlSpeed = 80.0f;
	params->sideView.spindash.minSpeed = 20.0f;
	// boss
	params->boss.spinAttack.limitSpeedMax = 80.0f;
	params->boss.spinAttack.brakeForce = 0.0f;
	params->boss.driftDash.brake = 3.0f;
	params->boss.driftDash.checkDashSpeed = 15.0f;
	params->boss.driftDash.checkDashTime = 0.45f;
	params->boss.driftDash.endSteeringSpeed = 120.0f;
	params->boss.driftDash.maxSpeed = 100.0f;
	params->boss.driftDash.outOfControlSpeed = 80.0f;
	params->boss.spindash.minSpeed = 20.0f;
	NOTIFY("APPLIED HARDCODE \n")
	// ... okay maybe not that big rn, but soon it'd be BIG 
	// or not..
	// should not be used for bundled dll in modpack
}

void pizdecSuperBiker3D(app::player::PlayerHsmContext* ctx, float deltaTime) {

	const float stompStopForce = 15.0f;

	csl::math::Vector4 Velocity = ctx->gocPlayerKinematicParams->velocity;
	float Y = Velocity.y();
	float prevX = prevVelocity.x();
	float prevZ = prevVelocity.z();
	std::cout << "PrevX infunc is " << prevX << "\n" << std::endl;
	std::cout << "PrevZ infunc is " << prevZ << "\n" << std::endl;
	Eigen::Vector4f idkVector(prevX, 0, prevZ, 0);
	Eigen::Vector4f idkVector1(prevX, 0, prevZ, 0);

	if (idkVector.norm() < stompStopForce * deltaTime) {
		Velocity = Eigen::Vector4f(0, 0, 0, 0);
	}
	else {
		Eigen::Vector4f idkVectornormalize = idkVector / idkVector.norm();
		ctx->gocPlayerKinematicParams->velocity = (idkVectornormalize * (idkVector.norm() - (stompStopForce * deltaTime)));
		Velocity.y() = Y;
	}
	prevVelocity = Velocity;

	std::cout << "Velocity X " << Velocity.x() << "\n" << std::endl;
	std::cout << "Velocity Y " << Velocity.y() << "\n" << std::endl;
	std::cout << "Velocity Z " << Velocity.z() << "\n" << std::endl;
}

void Bouncinggg(std::string arg) {
	auto* gameManager = hh::game::GameManager::GetInstance();
	auto* lvlInfo = gameManager->GetService<app::level::LevelInfo>();
	auto horSpeed = lvlInfo->playerInformation->horizontalSpeed;

	auto* playerCommonRFL = hh::fnd::ResourceManager::GetInstance()->GetResource<hh::fnd::ResReflectionT<app::player::PlayerParameters>>("player_common");
	auto* params = playerCommonRFL->GetData();
	static float SampleHorSpeed = 0;

	if (arg == "sample") {
		if (horSpeed.has_value()) {
			SampleHorSpeed = horSpeed.value();
		}
		return;
	}
	if (arg == "apply") {
		params->whiteSpace.jumpSpeed.minStickSpeed = SampleHorSpeed;
		params->forwardView.jumpSpeed.minStickSpeed = SampleHorSpeed;
		params->sideView.jumpSpeed.minStickSpeed = SampleHorSpeed;
		params->boss.jumpSpeed.minStickSpeed = SampleHorSpeed;
		return;
	}
}

// mainly rfl
HOOK(uint64_t, __fastcall, GameModeTitleInit, 0x1401990E0, app::game::GameMode* self) {
	auto res = originalGameModeTitleInit(self);
	NOTIFY("IN MENU");
	return res;
}

HOOK(void, __fastcall, InitPlayer, 0x14060DBC0, hh::game::GameObject* self) {
	originalInitPlayer(self);
	
	auto* BBcomp = self->GetComponent<app::player::GOCPlayerBlackboard>();
	auto* BBstatus = BBcomp->blackboard->GetContent<app::player::BlackboardStatus>();
	
	//player_common_tweak();
	jumpSpeed_func("backup");
	bounceRFL("backup");

	if (BBstatus) {
		BBstatus->SetCombatFlag(app::player::BlackboardStatus::CombatFlag::SLALOM_STEP, 1);
		NOTIFY("Enabled Slalom Step")
		return;
	}
	else {
		NOTIFY("Failed to enable Slalom step")
		return;
	}
	return;

}


HOOK(bool, __fastcall, SpinAttackStep, 0x1406CB3E0, app::player::StateSpinAttack* self, app::player::PlayerHsmContext* ctx, float deltaTime) {
	auto* HSMComp = ctx->playerObject->GetComponent<app::player::GOCPlayerHsm>();
	bounceRFL("restore");
	HSMComp->ChangeState(11, 0);
	return originalSpinAttackStep(self, ctx, deltaTime);
}

HOOK(bool, __fastcall, StompingBounceStep, 0x1406D1620, app::player::StateStompingBounce* self, app::player::PlayerHsmContext* ctx, float deltaTime) {
	Redirect(ctx, StateDirector::bouncejump);
	return originalStompingBounceStep(self, ctx, deltaTime);
}

HOOK(bool, __fastcall, DriftDashStep, 0x1406AFB70, app::player::StateDriftDash* self, app::player::PlayerHsmContext* ctx, float deltaTime) {
	tryDamage(ctx, false);
	driftdash_inputs(ctx);
	return originalDriftDashStep(self, ctx, deltaTime);
}

HOOK(bool, __fastcall, DropDashStep, 0x1406AFFB0, app::player::StateDropDash* self, app::player::PlayerHsmContext* ctx, float deltaTime) {
	drop_driftdash_transition(ctx, deltaTime);
	return originalDropDashStep(self, ctx, deltaTime);
}

HOOK(bool, __fastcall, SpinDashStep, 0x1406CB790, app::player::StateSpinDash* self, app::player::PlayerHsmContext* ctx, float deltaTime) {
	drop_driftdash_transition(ctx, deltaTime);
	return originalSpinDashStep(self, ctx, deltaTime);
} // don't ask me why i do this

HOOK(bool, __fastcall, SlalomStep, 0x1406C9390, app::player::StateSlalomStep * self, app::player::PlayerHsmContext * ctx, float deltaTime) {
	auto* input = ctx->playerObject->GetComponent<hh::game::GOCInput>();
	auto* inputcomp = input->GetInputComponent();
	if (inputcomp->actionMonitors[3].state & 256) {
		Redirect(ctx, StateDirector::preserveboost);
	}
	return originalSlalomStep(self, ctx, deltaTime);
}

HOOK(bool, __fastcall, BounceJumpStep, 0x1406C1C80, app::player::StateBounceJump* self, app::player::PlayerHsmContext* ctx, float deltaTime) {
	
	
	bouncejump_inputs(ctx);
	tryDamage(ctx, 0);
	return originalBounceJumpStep(self, ctx, deltaTime);
}

HOOK(void, __fastcall, EnterStompingDown, 0x14a85c590, app::player::StateStompingDown* self, app::player::PlayerHsmContext* ctx, int previousState) {
	csl::math::Vector4 currVelocity = ctx->gocPlayerKinematicParams->velocity;
	prevVelocity = currVelocity;
	float prevX = prevVelocity.x();
	float prevZ = prevVelocity.z();
	NOTIFY("sample velocity on entrance");
	std::cout << "PrevX Enter is " << prevX << "\n" << std::endl;
	std::cout << "PrevZ Enter is " << prevZ << "\n" << std::endl;
	Bouncinggg("sample");
	originalEnterStompingDown(self, ctx, previousState);
}

HOOK(bool, __fastcall, StompingDownStep, 0x1406d1760, app::player::StateStompingDown* self, app::player::PlayerHsmContext* ctx, float deltaTime) {
	NOTIFY("trying super cool thing");
	originalStompingDownStep(self, ctx, deltaTime);
	pizdecSuperBiker3D(ctx, deltaTime);
	return (self, ctx, deltaTime);
}

HOOK(bool, __fastcall, GrindDoubleJumpStep, 0x1406BEAB0, app::player::StateGrindDoubleJump* self, app::player::PlayerHsmContext* ctx, float deltaTime) {
	tryDamage(ctx, 0);
	return originalGrindDoubleJumpStep(self, ctx, deltaTime);
}

HOOK(void, __fastcall, StateDriftDashLeave, 0x1406AF8D0, app::player::StateDriftDash* self, app::player::PlayerHsmContext* ctx, int nextState) {
	//auto* GOCSound = ctx->playerObject->GetComponent<hh::snd::GOCSound>();
	tryDamage(ctx, true);
	//GOCSound->StopAll(1);
	originalStateDriftDashLeave(self, ctx, nextState);
}

HOOK(void, __fastcall, StateBounceJumpEnter, 0x1406C06E0, app::player::StateBounceJump* self, app::player::PlayerHsmContext* ctx, int previousState) {
	std::cout << "Bounce previous State " << previousState << "\n" << std::endl;
	if (previousState == 55) {
		Bouncinggg("apply"); // if bouncing, then we hack to retain velocity
		NOTIFY("hacking bounce speed");
	}
	else {
		jumpSpeed_func("apply");
		NOTIFY("or.. not");
	}
	originalStateBounceJumpEnter(self, ctx, previousState);
}

HOOK(void, __fastcall, StateBounceJumpLeave, 0x1406C15F0, app::player::StateBounceJump* self, app::player::PlayerHsmContext* ctx, int nextState) {
	jumpSpeed_func("restore");
	bounceRFL("restore");
	originalStateBounceJumpLeave(self, ctx, nextState);
}

HOOK(void, __fastcall, SlidingEnter, 0x1406CBB70, app::player::StateSliding* self, app::player::PlayerHsmContext* ctx, int previousState) {
	auto* HSMComp = ctx->playerObject->GetComponent<app::player::GOCPlayerHsm>();

	if (previousState == 20) {
		HSMComp->hsm.ChangeState(111, 0);
		NOTIFY("Rolling from squat")
	}

	if (previousState != 20) {
		HSMComp->hsm.ChangeState(107, 0);
		tryDamage(ctx, false);
		NOTIFY("Rolling...")
	}
}

HOOK(bool, __fastcall, StompingLandStep, 0x1406d1d30, app::player::StateStompingLand* self, app::player::PlayerHsmContext* ctx, float deltaTime) {
	auto* input = ctx->playerObject->GetComponent<hh::game::GOCInput>();
	auto* inputcomp = input->GetInputComponent();
	auto* HSMComp = ctx->playerObject->GetComponent<app::player::GOCPlayerHsm>();
	if (inputcomp->actionMonitors[7].state & 256) {
		HSMComp->ChangeState(11,0);
	}
	else
	{
		originalStompingLandStep(self, ctx, deltaTime);
	}
	return self, ctx, deltaTime;
}

BOOL WINAPI DllMain(_In_ HINSTANCE hInstance, _In_ DWORD reason, _In_ LPVOID reserved)
{
	switch (reason)
	{
	case DLL_PROCESS_ATTACH:
		INSTALL_HOOK(GameModeTitleInit);
		INSTALL_HOOK(InitPlayer);
		INSTALL_HOOK(SpinAttackStep);
		INSTALL_HOOK(DriftDashStep);
		INSTALL_HOOK(DropDashStep);
		INSTALL_HOOK(SpinDashStep);
		INSTALL_HOOK(SlalomStep);
		INSTALL_HOOK(BounceJumpStep);	
		INSTALL_HOOK(EnterStompingDown);
		INSTALL_HOOK(StompingDownStep);
		INSTALL_HOOK(GrindDoubleJumpStep);
		INSTALL_HOOK(StateDriftDashLeave);
		INSTALL_HOOK(StateBounceJumpEnter);
		INSTALL_HOOK(StateBounceJumpLeave);
		INSTALL_HOOK(SlidingEnter);
		INSTALL_HOOK(StompingLandStep);
		break;
	case DLL_PROCESS_DETACH:
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
		break;
	}

	return TRUE;
}
