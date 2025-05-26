#pragma once
#include <cstdint>
// LaunchData 구조체 정의
#pragma pack(push, 1)
struct MissileInfo
{
	uint8_t mockType; // 모의기 타입
	unsigned int id;  // 미사일 ID
	long long x;	  // 위도
	long long y;	  // 경도
	long long z;	  // 고도
	double angle;	  // 발사각
	int speed;		  // 속도
};
#pragma pack(pop)