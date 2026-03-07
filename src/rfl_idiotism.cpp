#include <Pch.h>

void rfl_idiotism::player_common_tweak() {
	auto* pParams = hh::fnd::ResourceManager::GetInstance()->GetResource<hh::fnd::ResReflectionT<app::player::PlayerParameters>>("player_common")->GetData();
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
#if _DEBUG
	NOTIFY("APPLIED HARDCODE")
#endif
		// ... okay maybe not that big rn, but soon it'd be BIG 
		// or not..
		// should not be used for bundled dll in modpack
}

void rfl_idiotism::jump_speed_func(std::string arg) {
	auto* pParams = hh::fnd::ResourceManager::GetInstance()->GetResource<hh::fnd::ResReflectionT<app::player::PlayerParameters>>("player_common")->GetData();
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

		static jumpSpeedParams WS_B{};
		static jumpSpeedParams FV_B{};
		static jumpSpeedParams SV_B{};
		static jumpSpeedParams BS_B{};

		static jumpSpeedParams WS{
			0, 0, -4, 1500, 1000, 15
		};
		static jumpSpeedParams FV{
			0, 0, -4, 15, 1000, 15
		};
		static jumpSpeedParams SV{
			0, 0, -4, 15, 1000, 10
		};
		static jumpSpeedParams BS{
			0, 0, -4, 15, 1000, 15
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
#if _DEBUG
			NOTIFY("Backed up jumpSpeed params")
#endif
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
			pParams->boss.jumpSpeed.accel = BS.brake;
			pParams->boss.jumpSpeed.accel = BS.accel;
			pParams->boss.jumpSpeed.limitUpSpeed = BS.limitUpSpeed;
			pParams->boss.jumpSpeed.minStickSpeed = BS.StickSpeed;
#if _DEBUG
			NOTIFY("Applied jumpSpeed params")
#endif
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
#if _DEBUG
			NOTIFY("Restored jumpSpeed params")
#endif		
				return;
		}
		return;
	}
	return;
}

void rfl_idiotism::bounceRFL(std::string arg) {
	auto* pParams = hh::fnd::ResourceManager::GetInstance()->GetResource<hh::fnd::ResReflectionT<app::player::PlayerParameters>>("player_common")->GetData();
	static float WS_B{};
	static float FV_B{};
	static float SV_B{};
	static float BS_B{};

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
