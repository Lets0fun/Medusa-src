#pragma once

struct UserCmd;

namespace Tickbase
{
	float realTime{ 0.0f };
	int targetTickShift{ 0 };
	int tickShift{ 0 };
	int shiftCommand{ 0 };
	int shiftedTickbase{ 0 };
	int ticksAllowedForProcessing{ 0 };
	int chokedPackets{ 0 };
	int pauseTicks{ 0 };
	bool shifting{ false };
	bool finalTick{ false };
	bool hasHadTickbaseActive{ false };
	int nextShiftAmount = 0;
	void getCmd(UserCmd* cmd);
	void start(UserCmd* cmd) noexcept;
	void end(UserCmd* cmd, bool sendPacket) noexcept;
	bool shiftOffensive(UserCmd* cmd, int amount, bool forceShift = false) noexcept;
	bool shiftDefensive(UserCmd* cmd, int amount, bool forceShift = false) noexcept;
	bool shiftHideShots(UserCmd* cmd, int shiftAmount, bool forceShift = false) noexcept;
	bool canRun() noexcept;
	bool canShiftDT(int shiftAmount, bool forceShift = false) noexcept;
	bool canShiftHS(int shiftAmount, bool forceShift = false) noexcept;
	int getCorrectTickbase(int commandNumber) noexcept;
	int& pausedTicks() noexcept;	
	int getTargetTickShift() noexcept;
	int getTickshift() noexcept;
	void resetTickshift() noexcept;
	bool& isFinalTick() noexcept;
	bool& isShifting() noexcept;
	void updateInput() noexcept;
	void reset() noexcept;
}