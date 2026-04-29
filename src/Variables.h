#pragma once

extern csl::math::Vector4 prevVelocity; // someone kill me please
extern csl::math::Vector3 jumpEnterNormal;
extern bool WantToBounce;
extern bool wasI;
extern bool rolling;
extern csl::math::Vector2 stompInput;
extern csl::math::Vector2 bounceEnterInput;
extern int bounceEnterId;
extern int stompDownEnterId;

extern float divingIdleTimer;
extern bool divingIdleStatus;

// jump
extern float jumpTimer;
extern bool jumpStatus;
extern bool jumpAnim;
extern bool isShortHop;
extern bool jumpTricked;

extern float wallJumpTimer;

extern float homingFinishedTimer;