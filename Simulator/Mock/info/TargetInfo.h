#pragma once
#include <cstdint>

#pragma pack(push, 1)
struct TargetInfo
{
	uint8_t mockType; // 모의기 타입
	unsigned int id;  // 미사일 ID
	float x;		  // 위도
	float y;		  // 경도
	float z;		  // 고도
	float angle;	  // 발사각
	int speed;		  // 속도
};
#pragma pack(pop)