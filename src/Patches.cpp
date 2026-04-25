#include <Pch.h>

void Patches::Patch() {
	patch_Step_StateJump();
	patch_Step_StateWallJump();
	patch_Enter_HomingFinished();
	/////////////////////////////
	patch_ProcessMsg_JumpDash();
	patch_Leave_StateWallJump();
	patch_Leave_StateLightDash();
}

///////////////////////////////////////
void patch_Step_StateJump() {
	// replacing jbe that'd transit statejump into ball animation, with doing nothing, behavior overriden in hook
	WriteProtected<byte>(0x14A7C123A, 0xE9); // jmp
	WriteProtected<byte>(0x14A7C123B, 0x43); // where
	WriteProtected<byte>(0x14A7C123C, 0x01);
	WriteProtected<byte>(0x14A7C123D, 0x00);
	WriteProtected<byte>(0x14A7C123E, 0x00);
	WriteProtected<byte>(0x14A7C123F, 0x90); // nop
}

void patch_Step_StateWallJump() {
	// replacing jz with jnz iirc to avoid StateWallJump transiting into StateFall for some reason
	WriteProtected<byte>(0x01406e0bd6, 0x75);
}

void patch_Enter_HomingFinished() {
	WriteProtected<byte>(0x140697B69, 0xEB);
}
///////////////////////////////////////


///////////////////////////////////////
///////////////////////////////////////
void patch_ProcessMsg_JumpDash() {
	WriteProtected<void*>(0x14125BCF8, &ProcessMessage_JumpDash);
}

void patch_Leave_StateWallJump() {
	WriteProtected<void*>(0x1412631A0, &Leave_WallJump);
}

void patch_Leave_StateLightDash() {
	WriteProtected<void*>(0x14125BED8, &Leave_LightDash);
}
///////////////////////////////////////
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

inline bool СоникТимЧтобыЭтоНеБыло(app::player::PlayerHsmContext* ctx) {
	uint32_t unk105b = ctx->gocPlayerHsm->unk105b; // i am really curious on what is happening here
	if (ctx->gocPlayerHsm->GetCurrentState() == unk105b) {
		return 0;
	}
	if (unk105b == 28 || unk105b == 33) {
		return 1;
	}
	return unk105b == 37;
}

bool ProcessMessage_JumpDash(app::player::StateJumpDash* self, app::player::PlayerHsmContext* ctx, hh::fnd::Message* message) {
	if (message->ID == hh::fnd::MessageID::EightFourNineSeven) {
		if (!СоникТимЧтобыЭтоНеБыло(ctx)) {
			ctx->gocPlayerHsm->ChangeState(64, 0);
			return 1;
		}
	}
	return self->app::player::GOCPlayerStateBase<app::player::PlayerHsmContext>::ProcessMessage(*ctx, *message);
}
///////////////////////////////////////
///////////////////////////////////////