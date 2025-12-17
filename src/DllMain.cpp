void Redirect_SpinAttack(app::player::PlayerHsmContext* ctx)
{
	ctx->playerObject->GetComponent<app::player::GOCPlayerHsm>()->hsm.ChangeState(11);
};

void Redirect_Slide(app::player::PlayerHsmContext* ctx)
{
	ctx->playerObject->GetComponent<app::player::GOCPlayerHsm>()->hsm.ChangeState(109);
}

HOOK(bool, __fastcall, SpinAttackStep, 0x1406CB3E0, app::player::StateSpinAttack* self, app::player::PlayerHsmContext* ctx, float deltaTime) {
	Redirect_SpinAttack(ctx);
	return originalSpinAttackStep(self, ctx, deltaTime);
}

HOOK(bool, __fastcall, SlidingStep, 0x1406CC600, app::player::StateSliding* self, app::player::PlayerHsmContext* ctx, float deltaTime) {
	Redirect_Slide(ctx);
	return originalSlidingStep(self, ctx, deltaTime);
}

BOOL WINAPI DllMain(_In_ HINSTANCE hInstance, _In_ DWORD reason, _In_ LPVOID reserved)
{
	switch (reason)
	{
	case DLL_PROCESS_ATTACH:
		INSTALL_HOOK(SpinAttackStep);
		INSTALL_HOOK(SlidingStep);
		break;
	case DLL_PROCESS_DETACH:
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
		break;
	}

	return TRUE;
}
