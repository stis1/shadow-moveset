#define NOTIFY(str, ...) \
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_GREEN | FOREGROUND_INTENSITY); \
    std::cout << "[Shadow Moveset]: " << str << std::endl; \
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
#include <iostream>
#include <optional>
#include <fstream>

static Eigen::Vector4f prevVelocity;
static bool WantToBounce = false;

//
csl::math::Vector2 deadZone(0, 0);
static float rollingAirMultiplier = 0;
static float fallToRollAtitude = 0;
//static float bounceBreakForce = 0;
//
void setPhysicsVariables() {
	std::ifstream ifs{ "mod.ini" };
	std::string line{};

	while (std::getline(ifs, line)) {
		if (line.starts_with("deadZone=")) {
			std::string diff{ line.substr(strlen("deadZone=")) };
			std::istringstream iss{ diff };
			iss >> deadZone.x();
			iss >> deadZone.y();
			NOTIFY("deadzone setup");
			std::cout << "[Shadow Moveset]: Stomp deadzone " << deadZone.x() << std::endl;
		}
		if (line.starts_with("rollingAirMultiplier=")) {
			std::string diff{ line.substr(strlen("rollingAirMultiplier=")) };
			std::istringstream iss{ diff };
			iss >> rollingAirMultiplier;
			NOTIFY("rollingAirMultiplier setup");
			std::cout << "[Shadow Moveset]: Rolling Air Multiplier " << rollingAirMultiplier << std::endl;
		}
		if (line.starts_with("fallToRollAtitude=")) {
			std::string diff{ line.substr(strlen("fallToRollAtitude=")) };
			std::istringstream iss{ diff };
			iss >> fallToRollAtitude;
			NOTIFY("fallToRollAtitude setup");
			std::cout << "[Shadow Moveset]: Fall to Roll atitude " << fallToRollAtitude << std::endl;
		}
		return;
		//if (line.starts_with("bounceBreakForce=")) {
		//	std::string diff{ line.substr(strlen("bounceBreakForce=")) };
		//	std::istringstream iss{ diff };
		//	iss >> bounceBreakForce;
		//	NOTIFY("Bounce break setup");
		//	std::cout << "[Shadow Moveset]: Bounce Break  " << bounceBreakForce << std::endl;
		//}
	}
}
//

auto* pParams = hh::fnd::ResourceManager::GetInstance()->GetResource<hh::fnd::ResReflectionT<app::player::PlayerParameters>>("player_common")->GetData();
auto* pLvlInfo = hh::game::GameManager::GetInstance()->GetService<app::level::LevelInfo>();

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

void checkHomingReticle() {
	auto* pGameManager = hh::game::GameManager::GetInstance();
	auto* pCursor = pGameManager->gameObjectLayers[19]->objects[0];
	//auto* pGOCTinyFsm = pCursor->GetComponent<app_cmn::fsm::GOCTinyFsm2>();


}

void tryDamage(app::player::PlayerHsmContext* ctx, bool leaveMeAlone) {
	auto* pPluginCollision = ctx->gocPlayerHsm->statePluginManager->GetPlugin<app::player::StatePluginCollision>();
	auto* pBBbattle = ctx->gocPlayerBlackboard->blackboard->GetContent<app::player::BlackboardBattle>();
	auto* pGOCPlayerHsm = ctx->playerObject->GetComponent<app::player::GOCPlayerHsm>();

	static bool isInBallState = false;

	bool isBoosting = ctx->gocPlayerBlackboard->blackboard->GetContent<app::player::BlackboardStatus>()->GetStateFlag(app::player::BlackboardStatus::StateFlag::BOOST);
	int getID = pGOCPlayerHsm->GetCurrentState();

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

void bounceRFL(std::string arg) {
	static float WS_B = 0;
	static float FV_B = 0;
	static float SV_B = 0;
	static float BS_B = 0;

	if (arg == "backup") {
		WS_B = pParams->whiteSpace.doubleJump.bounceSpeed;
		FV_B = pParams->forwardView.doubleJump.bounceSpeed;
		SV_B = pParams->sideView.doubleJump.bounceSpeed;
		BS_B = pParams->boss.doubleJump.bounceSpeed;
	}
	if (arg == "restore") {
		pParams->whiteSpace.doubleJump.bounceSpeed = WS_B;
		pParams->forwardView.doubleJump.bounceSpeed = FV_B;
		pParams->sideView.doubleJump.bounceSpeed = SV_B;
		pParams->boss.doubleJump.bounceSpeed = BS_B;
	}
}

void rolling_air_hack(app::player::PlayerHsmContext* ctx) {
	csl::math::Vector4 velocity_m = ctx->gocPlayerKinematicParams->velocity;
	float vMagnitude = velocity_m.y();

	if (pParams) {
		pParams->whiteSpace.doubleJump.bounceSpeed = vMagnitude*rollingAirMultiplier;
		pParams->forwardView.doubleJump.bounceSpeed = vMagnitude*rollingAirMultiplier;
		pParams->sideView.doubleJump.bounceSpeed = vMagnitude*rollingAirMultiplier;
		pParams->boss.doubleJump.bounceSpeed = vMagnitude*rollingAirMultiplier;
	}	
}

void Redirect(app::player::PlayerHsmContext* ctx, StateDirector to_what)
{
	auto* pGOCPlayerHsm = ctx->playerObject->GetComponent<app::player::GOCPlayerHsm>();
	switch (to_what) 
	{
		case StateDirector::driftdash:
			pGOCPlayerHsm->ChangeState(107, 0);
			tryDamage(ctx, false);
			NOTIFY("DIRECTED TO DRIFTDASH");
			break;
		case StateDirector::bouncejump:
			pGOCPlayerHsm->ChangeState(11, 0);
			rolling_air_hack(ctx);
			tryDamage(ctx, false);
			NOTIFY("DIRECTED TO BOUNCEJUMP");
			break;
		case StateDirector::jump:
			pGOCPlayerHsm->ChangeState(9,0);
			NOTIFY("DIRECTED TO JUMP");
			break;
		case StateDirector::run:
			pGOCPlayerHsm->ChangeState(4, 0);
			NOTIFY("DIRECTED TO RUN");
			break;
		case StateDirector::boost:
			pGOCPlayerHsm->ChangeState(4, 0);
			ctx->blackboardStatus->SetStateFlag(app::player::BlackboardStatus::StateFlag::BOOST, 1);
			tryDamage(ctx, 0);
			NOTIFY("DIRECTED TO BOOST");
			break;
		case StateDirector::homing:
			pGOCPlayerHsm->ChangeState(63, 0);
			NOTIFY("HOMING SHADOW");
			break;
		case StateDirector::preserveboost: // for boosting inside slalom, maybe others, bad implementation if being honest
			ctx->blackboardStatus->SetStateFlag(app::player::BlackboardStatus::StateFlag::BOOST, 1);
			tryDamage(ctx, false);
			NOTIFY("KEPT BOOST");
			break;
		case StateDirector::doublejump:
			pGOCPlayerHsm->ChangeState(10, 0);
			NOTIFY("DIRECTED TO DOUBLE JUMP");
			break;
		case StateDirector::stomp:
			pGOCPlayerHsm->ChangeState(52, 0);
			NOTIFY("DIRECTED TO STOMP");
			break;
		case StateDirector::falls:
			pGOCPlayerHsm->ChangeState(15, 0);
			NOTIFY("Shadow shall fall");
			break;
		default:
			break;
	}
	return;
}

void driftdash_inputs(app::player::PlayerHsmContext* ctx) {
	auto* pGOCInput = ctx->playerObject->GetComponent<hh::game::GOCInput>()->GetInputComponent();
	auto isGrounded = pLvlInfo->playerInformation->isGrounded;

	// player input
	bool boostHeld = pGOCInput->actionMonitors[3].state & 256; // 3 is action, 256 or 512 is whether held or tapped
	bool boostTap = pGOCInput->actionMonitors[3].state & 512;
	bool circleTap = pGOCInput->actionMonitors[7].state & 512;
	bool crossTap = pGOCInput->actionMonitors[0].state & 512;
	bool squareTap = pGOCInput->actionMonitors[1].state & 512;

	if (isGrounded == 0) {
		Redirect(ctx, StateDirector::bouncejump);
		return;
	}
	if (circleTap) {
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
	auto* pGOCInput = ctx->playerObject->GetComponent<hh::game::GOCInput>()->GetInputComponent();
	auto* pGOCPlayerHSM = ctx->playerObject->GetComponent<app::player::GOCPlayerHsm>();

	if (pGOCInput->actionMonitors[1].state & 512) {
		Redirect(ctx, StateDirector::homing);
	}
	if (pGOCInput->actionMonitors[0].state & 512) {
		Redirect(ctx, StateDirector::doublejump);
	}
	if (pGOCInput->actionMonitors[7].state & 512) {
		pGOCPlayerHSM->ChangeState(52, 0);
	}
	if (pGOCInput->actionMonitors[3].state & 256) {
		Redirect(ctx, StateDirector::preserveboost);
	}
	//if (inputcomp->actionMonitors[7].state & 256 && Timer > 0.15f) {
	//	Redirect(ctx, StateDirector::bouncejump);
	//}
}

//void spinAttackCalculator(app::player::PlayerHsmContext* ctx) {
//	csl::math::Vector4 velocity_m = ctx->gocPlayerKinematicParams->velocity;
//	float vMagnitude = velocity_m.y();
//
//	if (vMagnitude >= -9) {
//		pParams->whiteSpace.spinAttack.jumpForce = 15.0f;
//		pParams->whiteSpace.spinAttack.jumpAddForce = 15.0f;
//		pParams->forwardView.spinAttack.jumpForce = 15.0f;
//		pParams->forwardView.spinAttack.jumpAddForce = 15.0f;
//		pParams->sideView.spinAttack.jumpForce = 15.0f;
//		pParams->sideView.spinAttack.jumpAddForce = 15.0f;
//		pParams->boss.spinAttack.jumpForce = 15.0f;
//		pParams->boss.spinAttack.jumpAddForce = 15.0f;
//	}
//	else {
//		pParams->whiteSpace.spinAttack.jumpForce = vMagnitude/-1;
//		pParams->whiteSpace.spinAttack.jumpAddForce = vMagnitude/-1;
//		pParams->forwardView.spinAttack.jumpForce = vMagnitude/-1;
//		pParams->forwardView.spinAttack.jumpAddForce = vMagnitude/-1;
//		pParams->sideView.spinAttack.jumpForce = vMagnitude/-1;
//		pParams->sideView.spinAttack.jumpAddForce = vMagnitude/-1;
//		pParams->boss.spinAttack.jumpForce = vMagnitude/-1;
//		pParams->boss.spinAttack.jumpAddForce = vMagnitude/-1;
//	}
//}

void drop_driftdash_transition(app::player::PlayerHsmContext* ctx, float deltaTime) 
{
	auto* pGOCPlayerHsm = ctx->playerObject->GetComponent<app::player::GOCPlayerHsm>();
	int getID = pGOCPlayerHsm->GetCurrentState();

	static float DDash_Timer = 0.0f;

	if (DDash_Timer < 0.15f && getID != 111) {
		DDash_Timer += deltaTime;
		pGOCPlayerHsm->ChangeState(109, 0);
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
	if (pParams) 
	{
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
			WS_B.maxStickBrake = pParams->whiteSpace.jumpSpeed.maxStickBrake;
			WS_B.minStickBrake = pParams->whiteSpace.jumpSpeed.minStickBrake;
			WS_B.brake = pParams->whiteSpace.jumpSpeed.brake;
			WS_B.accel = pParams->whiteSpace.jumpSpeed.accel;
			WS_B.limitUpSpeed = pParams->whiteSpace.jumpSpeed.limitUpSpeed;
			WS_B.StickSpeed = pParams->whiteSpace.jumpSpeed.minStickSpeed;
			// forwardView
			FV_B.maxStickBrake = pParams->forwardView.jumpSpeed.maxStickBrake;
			FV_B.minStickBrake = pParams->forwardView.jumpSpeed.minStickBrake;
			FV_B.brake = pParams->forwardView.jumpSpeed.brake;
			FV_B.accel = pParams->forwardView.jumpSpeed.accel;
			FV_B.limitUpSpeed = pParams->forwardView.jumpSpeed.limitUpSpeed;
			FV_B.StickSpeed = pParams->forwardView.jumpSpeed.minStickSpeed;
			// sideView
			SV_B.maxStickBrake = pParams->sideView.jumpSpeed.maxStickBrake;
			SV_B.minStickBrake = pParams->sideView.jumpSpeed.minStickBrake;
			SV_B.brake = pParams->sideView.jumpSpeed.brake;
			SV_B.accel = pParams->sideView.jumpSpeed.accel;
			SV_B.limitUpSpeed = pParams->sideView.jumpSpeed.limitUpSpeed;
			SV_B.StickSpeed = pParams->sideView.jumpSpeed.minStickSpeed;
			// boss
			BS_B.maxStickBrake = pParams->boss.jumpSpeed.maxStickBrake;
			BS_B.minStickBrake = pParams->boss.jumpSpeed.minStickBrake;
			BS_B.brake = pParams->boss.jumpSpeed.brake;
			BS_B.accel = pParams->boss.jumpSpeed.accel;
			BS_B.limitUpSpeed = pParams->boss.jumpSpeed.limitUpSpeed;
			BS_B.StickSpeed = pParams->boss.jumpSpeed.minStickSpeed;
			NOTIFY("Backed up jumpSpeed params")
			return;
		}
		if (arg == "apply") {
			// whiteSpace
			pParams->whiteSpace.jumpSpeed.maxStickBrake = WS.maxStickBrake;
			pParams->whiteSpace.jumpSpeed.minStickBrake = WS.minStickBrake;
			pParams->whiteSpace.jumpSpeed.brake = WS.brake;
			pParams->whiteSpace.jumpSpeed.accel = WS.accel;
			pParams->whiteSpace.jumpSpeed.limitUpSpeed = WS.limitUpSpeed;
			pParams->whiteSpace.jumpSpeed.minStickSpeed = WS.StickSpeed;
			// forwardView
			pParams->forwardView.jumpSpeed.maxStickBrake = FV.maxStickBrake;
			pParams->forwardView.jumpSpeed.minStickBrake = FV.minStickBrake;
			pParams->forwardView.jumpSpeed.brake = FV.brake;
			pParams->forwardView.jumpSpeed.accel = FV.accel;
			pParams->forwardView.jumpSpeed.limitUpSpeed = FV.limitUpSpeed;
			pParams->forwardView.jumpSpeed.minStickSpeed = FV.StickSpeed;
			// sideView
			pParams->sideView.jumpSpeed.maxStickBrake = SV.maxStickBrake;
			pParams->sideView.jumpSpeed.minStickBrake = SV.minStickBrake;
			pParams->sideView.jumpSpeed.brake = SV.brake;
			pParams->sideView.jumpSpeed.accel = SV.accel;
			pParams->sideView.jumpSpeed.limitUpSpeed = SV.limitUpSpeed;
			pParams->sideView.jumpSpeed.minStickSpeed = SV.StickSpeed;
			// boss
			pParams->boss.jumpSpeed.maxStickBrake = BS.maxStickBrake;
			pParams->boss.jumpSpeed.minStickBrake = BS.minStickBrake;
			pParams->boss.jumpSpeed.brake = BS.brake;
			pParams->boss.jumpSpeed.accel = BS.accel;
			pParams->boss.jumpSpeed.limitUpSpeed = BS.limitUpSpeed;
			pParams->boss.jumpSpeed.minStickSpeed = BS.StickSpeed;
			NOTIFY("Applied jumpSpeed params")
			return;
		}
		if (arg == "restore") {
			// whiteSpace
			pParams->whiteSpace.jumpSpeed.maxStickBrake = WS_B.maxStickBrake;
			pParams->whiteSpace.jumpSpeed.minStickBrake = WS_B.minStickBrake;
			pParams->whiteSpace.jumpSpeed.brake = WS_B.brake;
			pParams->whiteSpace.jumpSpeed.accel = WS_B.accel;
			pParams->whiteSpace.jumpSpeed.limitUpSpeed = WS_B.limitUpSpeed;
			pParams->whiteSpace.jumpSpeed.minStickSpeed = WS_B.StickSpeed;
			// forwardView
			pParams->forwardView.jumpSpeed.maxStickBrake = FV_B.maxStickBrake;
			pParams->forwardView.jumpSpeed.minStickBrake = FV_B.minStickBrake;
			pParams->forwardView.jumpSpeed.brake = FV_B.brake;
			pParams->forwardView.jumpSpeed.accel = FV_B.accel;
			pParams->forwardView.jumpSpeed.limitUpSpeed = FV_B.limitUpSpeed;
			pParams->forwardView.jumpSpeed.minStickSpeed = FV_B.StickSpeed;
			// sideView
			pParams->sideView.jumpSpeed.maxStickBrake = SV_B.maxStickBrake;
			pParams->sideView.jumpSpeed.minStickBrake = SV_B.minStickBrake;
			pParams->sideView.jumpSpeed.brake = SV_B.brake;
			pParams->sideView.jumpSpeed.accel = SV_B.accel;
			pParams->sideView.jumpSpeed.limitUpSpeed = SV_B.limitUpSpeed;
			pParams->sideView.jumpSpeed.minStickSpeed = SV_B.StickSpeed;
			// boss
			pParams->boss.jumpSpeed.maxStickBrake = BS_B.maxStickBrake;
			pParams->boss.jumpSpeed.minStickBrake = BS_B.minStickBrake;
			pParams->boss.jumpSpeed.brake = BS_B.brake;
			pParams->boss.jumpSpeed.accel = BS_B.accel;
			pParams->boss.jumpSpeed.limitUpSpeed = BS_B.limitUpSpeed;
			pParams->boss.jumpSpeed.minStickSpeed = BS_B.StickSpeed;
			NOTIFY("Restored jumpSpeed params")
			return;
		}
		return;
	}
	return;
}

// !!! big func, don't open it !!! 
void player_common_tweak() {
	//whiteSpace
	pParams->whiteSpace.spinAttack.limitSpeedMax = 100.0f;
	pParams->whiteSpace.spinAttack.brakeForce = 0.0f;
	pParams->whiteSpace.driftDash.brake = 3.0f;
	pParams->whiteSpace.driftDash.checkDashSpeed = 15.0f;
	pParams->whiteSpace.driftDash.checkDashTime = 0.45f;
	pParams->whiteSpace.driftDash.endSteeringSpeed = 120.0f;
	pParams->whiteSpace.driftDash.maxSpeed = 100.0f;
	pParams->whiteSpace.driftDash.outOfControlSpeed = 80.0f;
	pParams->whiteSpace.spindash.minSpeed = 20.0f;
	// forwardView
	pParams->forwardView.spinAttack.limitSpeedMax = 80.0f;
	pParams->forwardView.spinAttack.brakeForce = 0.0f;
	pParams->forwardView.driftDash.brake = 3.0f;
	pParams->forwardView.driftDash.checkDashSpeed = 15.0f;
	pParams->forwardView.driftDash.checkDashTime = 0.45f;
	pParams->forwardView.driftDash.endSteeringSpeed = 120.0f;
	pParams->forwardView.driftDash.maxSpeed = 100.0f;
	pParams->forwardView.driftDash.outOfControlSpeed = 80.0f;
	pParams->forwardView.spindash.minSpeed = 20.0f;
	// sideView
	pParams->sideView.spinAttack.limitSpeedMax = 80.0f;
	pParams->sideView.spinAttack.brakeForce = 0.0f;
	pParams->sideView.driftDash.brake = 3.0f;
	pParams->sideView.driftDash.checkDashSpeed = 15.0f;
	pParams->sideView.driftDash.checkDashTime = 0.45f;
	pParams->sideView.driftDash.endSteeringSpeed = 120.0f;
	pParams->sideView.driftDash.maxSpeed = 100.0f;
	pParams->sideView.driftDash.outOfControlSpeed = 80.0f;
	pParams->sideView.spindash.minSpeed = 20.0f;
	// boss
	pParams->boss.spinAttack.limitSpeedMax = 80.0f;
	pParams->boss.spinAttack.brakeForce = 0.0f;
	pParams->boss.driftDash.brake = 3.0f;
	pParams->boss.driftDash.checkDashSpeed = 15.0f;
	pParams->boss.driftDash.checkDashTime = 0.45f;
	pParams->boss.driftDash.endSteeringSpeed = 120.0f;
	pParams->boss.driftDash.maxSpeed = 100.0f;
	pParams->boss.driftDash.outOfControlSpeed = 80.0f;
	pParams->boss.spindash.minSpeed = 20.0f;
	NOTIFY("APPLIED HARDCODE")
	// ... okay maybe not that big rn, but soon it'd be BIG 
	// or not..
	// should not be used for bundled dll in modpack
}

void pizdecSuperBiker3D(app::player::PlayerHsmContext* ctx, float deltaTime) {
	csl::math::Vector4 Velocity = ctx->gocPlayerKinematicParams->velocity;
	float Y = Velocity.y();
	
	const float bounceBreakForce = 20.0f;

	float prevX = prevVelocity.x();
	float prevZ = prevVelocity.z();
	csl::math::Vector4 horVector(prevX, 0, prevZ, 0);
	
	#if _DEBUG
	std::cout << "PrevX infunc is " << prevX << std::endl;
	std::cout << "PrevZ infunc is " << prevZ << std::endl;
	#endif

	if (horVector.norm() < bounceBreakForce * deltaTime) {
		Velocity = csl::math::Vector4(0, 0, 0, 0);
	}
	else {

		Eigen::Vector4f horVectornormalize = horVector / horVector.norm();
		Velocity = (horVectornormalize * (horVector.norm() - (bounceBreakForce * deltaTime)));
		Velocity.y() = Y;
	}
	prevVelocity = Velocity;

	#if _DEBUG
	std::cout << "Velocity X " << Velocity.x() << std::endl;
	std::cout << "Velocity Y " << Velocity.y() << std::endl;
	std::cout << "Velocity Z " << Velocity.z() << std::endl;
	#endif
}

void Bouncinggg(std::string arg) {
	float Speed = pLvlInfo->playerInformation->Speed.value();
	static float SampleSpeed = 0;

	if (arg == "sample") {
		SampleSpeed = Speed;
		return;
	}
	if (arg == "apply") {
		pParams->whiteSpace.jumpSpeed.minStickSpeed = SampleSpeed;
		pParams->forwardView.jumpSpeed.minStickSpeed = SampleSpeed;
		pParams->sideView.jumpSpeed.minStickSpeed = SampleSpeed;
		pParams->boss.jumpSpeed.minStickSpeed = SampleSpeed;
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
	
	auto* pBBstatus = self->GetComponent<app::player::GOCPlayerBlackboard>()->blackboard->GetContent<app::player::BlackboardStatus>();
	
	//player_common_tweak();
	jumpSpeed_func("backup");
	bounceRFL("backup");
	setPhysicsVariables();
	if (pBBstatus) {
		pBBstatus->SetCombatFlag(app::player::BlackboardStatus::CombatFlag::SLALOM_STEP, 1);
		NOTIFY("Enabled Slalom Step")
		return;
	}
	else {
		NOTIFY("Failed to enable Slalom step")
		return;
	}
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
	auto* pGOCInput = ctx->playerObject->GetComponent<hh::game::GOCInput>()->GetInputComponent();
	if (pGOCInput->actionMonitors[3].state & 256) {
		Redirect(ctx, StateDirector::preserveboost);
	}
	return originalSlalomStep(self, ctx, deltaTime);
}

HOOK(bool, __fastcall, BounceJumpStep, 0x1406C1C80, app::player::StateBounceJump* self, app::player::PlayerHsmContext* ctx, float deltaTime) {
	if (!WantToBounce) {
		jumpSpeed_func("apply");
	}
	bouncejump_inputs(ctx);
	tryDamage(ctx, 0);
	return originalBounceJumpStep(self, ctx, deltaTime);
}

HOOK(void, __fastcall, EnterStompingDown, 0x14a85c590, app::player::StateStompingDown* self, app::player::PlayerHsmContext* ctx, int previousState) {
	auto* pLvlInfo = hh::game::GameManager::GetInstance()->GetService<app::level::LevelInfo>();
	auto* pGOCPlayerHSM = ctx->playerObject->GetComponent<app::player::GOCPlayerHsm>();
	auto* pGOCPlayerKinematic = ctx->playerObject->GetComponent<app::player::GOCPlayerKinematicParams>();
	float atitude = pLvlInfo->playerInformation->altitude.value();

	csl::math::Vector4 currVelocity = pGOCPlayerKinematic->velocity;
	
	prevVelocity = currVelocity;
	
	float prevX = prevVelocity.x();
	float prevZ = prevVelocity.z();
	
	#if _DEBUG
	NOTIFY("sample velocity on EnterStateStompingDown");
	std::cout << "PrevX Enter is " << prevX << std::endl;
	std::cout << "PrevZ Enter is " << prevZ << std::endl;
	#endif
	Bouncinggg("sample");
	if (atitude < fallToRollAtitude) {
		pGOCPlayerHSM->hsm.ChangeState(111, 0);
	}
	else {
		originalEnterStompingDown(self, ctx, previousState);
	}
}

HOOK(bool, __fastcall, StompingDownStep, 0x1406d1760, app::player::StateStompingDown* self, app::player::PlayerHsmContext* ctx, float deltaTime) {
	auto* pGOCInput = ctx->playerObject->GetComponent<hh::game::GOCInput>()->GetInputComponent();
	csl::math::Vector2 currentInput = pLvlInfo->playerInformation->leftStickInput.value();

	originalStompingDownStep(self, ctx, deltaTime);
	if (currentInput.norm() > deadZone.norm()) {
		pizdecSuperBiker3D(ctx, deltaTime);
		WantToBounce = true;
		NOTIFY("BOUNCING")
	}
	if (currentInput.norm() < deadZone.norm()) {
		WantToBounce = false;
	}
	if (!WantToBounce) {
		jumpSpeed_func("apply");
	}
	return (self, ctx, deltaTime);
}

HOOK(bool, __fastcall, GrindDoubleJumpStep, 0x1406BEAB0, app::player::StateGrindDoubleJump* self, app::player::PlayerHsmContext* ctx, float deltaTime) {
	tryDamage(ctx, 0);
	return originalGrindDoubleJumpStep(self, ctx, deltaTime);
}

HOOK(void, __fastcall, StateDriftDashLeave, 0x1406AF8D0, app::player::StateDriftDash* self, app::player::PlayerHsmContext* ctx, int nextState) {
	tryDamage(ctx, true);
	originalStateDriftDashLeave(self, ctx, nextState);
}

HOOK(void, __fastcall, StateBounceJumpEnter, 0x1406C06E0, app::player::StateBounceJump* self, app::player::PlayerHsmContext* ctx, int previousState) {
	csl::math::Vector2 currentInput = pLvlInfo->playerInformation->leftStickInput.value();
	
	std::cout << "Bounce previous State " << previousState << std::endl;

	if (previousState == 55 && currentInput.norm() > deadZone.norm()) {
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
	auto* pGOCPlayerHsm = ctx->playerObject->GetComponent<app::player::GOCPlayerHsm>();

	if (previousState == 20) {
		pGOCPlayerHsm->hsm.ChangeState(111, 0);
		NOTIFY("Rolling from squat")
	}

	if (previousState != 20) {
		pGOCPlayerHsm->hsm.ChangeState(107, 0);
		tryDamage(ctx, false);
		NOTIFY("Rolling...")
	}
}

HOOK(bool, __fastcall, StompingLandStep, 0x1406d1d30, app::player::StateStompingLand* self, app::player::PlayerHsmContext* ctx, float deltaTime) {
	auto* pGOCInput = ctx->playerObject->GetComponent<hh::game::GOCInput>()->GetInputComponent();
	auto* pGOCPlayerHsm = ctx->playerObject->GetComponent<app::player::GOCPlayerHsm>();
	
	if (pGOCInput->actionMonitors[7].state & 256 && WantToBounce) {
		pGOCPlayerHsm->ChangeState(11,0);
	}
	else {
		originalStompingLandStep(self, ctx, deltaTime);
	}
	return (self, ctx, deltaTime);
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
