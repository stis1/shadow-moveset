#pragma once

class Patches {
public:
	static void Patch();
private:
	static void patch_Step_StateJump();
	static void patch_Step_StateWallJump();
	static void patch_Enter_HomingFinished();
	static void patch_ProcessMsg_JumpDash();
	static void patch_Leave_StateWallJump();
	static void patch_Leave_StateLightDash();
	static void Leave_WallJump(app::player::StateWallJump* self, app::player::PlayerHsmContext* ctx, int nextState);
	static void Leave_LightDash(app::player::StateLightDash* self, app::player::PlayerHsmContext* ctx, int nextState);
	static bool ProcessMessage_JumpDash(app::player::StateJumpDash* self, app::player::PlayerHsmContext* ctx, hh::fnd::Message* message);
};