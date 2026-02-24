#define EXPORT extern "C" __declspec(dllexport)
//#include<iostream>
EXPORT void Init()
{
}

EXPORT void PostInit()
{
}

EXPORT void OnFrame()
{
	//auto* pGameManager = hh::game::GameManager::GetInstance();
	//if (pGameManager) {
	//	auto* pPlayer = pGameManager->GetGameObject("Shadow");
	//	if (pPlayer) {
	//		//auto* pGOCPlayerHsm = pPlayer->GetComponent<app::player::GOCPlayerHsm>();
	//		//auto* pPlayerCollision = pPlayer->GetComponent<app::player::GOCPlayerCollider>()->GetPlayerCollision();
	//		//float collisionSize = pPlayerCollision->qword2C;
	//		//int currentState = pGOCPlayerHsm->GetCurrentState();
	//		//std::cout << "CURRENT STATE: " << currentState << std::endl;
	//		//std::cout << "HITBOX SIZE: " << collisionSize << std::endl;
	//		auto* pBBstatus = pPlayer->GetComponent<app::player::GOCPlayerBlackboard>()->blackboard->GetContent<app::player::BlackboardStatus>();
	//		bool outofControl = pBBstatus->GetWorldFlag(app::player::BlackboardStatus::WorldFlag::UNK6);
	//		std::cout << "OUT OF CONTROL: " << outofControl << std::endl;
	//	}
	//}
}