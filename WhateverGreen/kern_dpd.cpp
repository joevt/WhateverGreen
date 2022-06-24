//
//  kern_dpd.cpp
//  WhateverGreen
//
//  Created by joevt on 2022-06-18.
//  Copyright © 2022 vit9696. All rights reserved.
//

#include "kern_dpd.hpp"

#include <Headers/kern_api.hpp>
#include <Headers/kern_iokit.hpp>

#define procdisplaypolicyd "/usr/libexec/displaypolicyd"
#define binarydisplaypolicyd procdisplaypolicyd

static const uint8_t overridepathFind[] = "/System/Library/Displays/Contents/Resources/Overrides/DisplayVendorID-%x/DisplayProductID-%x";
static const uint8_t overridepathRepl[] =        "/Library/Displays/Contents/Resources/Overrides/DisplayVendorID-%x/DisplayProductID-%x\0\0\0\0\0\0\0";
static const uint8_t mtddpathFind[]     = "/System/Library/Displays/Contents/Resources/Overrides/DisplayVendorID-%x/DisplayProductID-%x.mtdd";
static const uint8_t mtddpathRepl[]     =        "/Library/Displays/Contents/Resources/Overrides/DisplayVendorID-%x/DisplayProductID-%x.mtdd\0\0\0\0\0\0\0";

static UserPatcher::BinaryModPatch binaryPatch[] {
	{
	CPU_TYPE_X86_64, 0, // flags
	overridepathFind, overridepathRepl, arrsize(overridepathFind), // find, replace, size
	0, // skip  = 0 -> replace all occurrences
	1, // count = 1 -> 1 set of hex inside the target binaries
	UserPatcher::FileSegment::SegmentTextCstring,
	1 // enabled
	},
	{
	CPU_TYPE_X86_64, 0, // flags
	mtddpathFind, mtddpathRepl, arrsize(mtddpathFind), // find, replace, size
	0, // skip  = 0 -> replace all occurrences
	1, // count = 1 -> 1 set of hex inside the target binaries
	UserPatcher::FileSegment::SegmentTextCstring,
	1 // enabled
	},
};

static UserPatcher::BinaryModInfo binaryMod { binarydisplaypolicyd, binaryPatch, arrsize(binaryPatch) };

static UserPatcher::ProcInfo procInfo { procdisplaypolicyd, sizeof(procdisplaypolicyd) - 1, 1 };

DPD *DPD::callbackDPD;

void DPD::init() {
	DBGLOG("dpd", "[ DPD::init");
	callbackDPD = this;

	// nothing should be applied when -dpdoff is passed
	if (!checkKernelArgument("-dpdoff") && getKernelVersion() >= KernelVersion::Catalina) {
		currentProcInfo = &procInfo;
		currentModInfo = &binaryMod;
		lilu.onProcLoadForce(currentProcInfo, 1, nullptr, nullptr, currentModInfo, 1);
	}
	DBGLOG("dpd", "] DPD::init");
}

void DPD::deinit() {
	DBGLOG("dpd", "[ DPD::deinit");
	DBGLOG("dpd", "] DPD::deinit");
}

void DPD::processKernel(KernelPatcher &patcher, DeviceInfo *info) {
	// -dpdon -> force enable
	// -dpdoff -> force disable
	DBGLOG("dpd", "[ DPD::processKernel");
	disableDPD = checkKernelArgument("-dpdoff");

	bool patchCommon = false;

	if (!disableDPD) {
		if (checkKernelArgument("-dpdon")) {
			patchCommon = true;
		}
	}

	if (!patchCommon && currentProcInfo && currentModInfo) {
		currentProcInfo->section = UserPatcher::ProcInfo::SectionDisabled;
		for (size_t i = 0; i < currentModInfo->count; i++)
			currentModInfo->patches[i].section = UserPatcher::ProcInfo::SectionDisabled;
	}
	DBGLOG("dpd", "] DPD::processKernel patchCommon:%d disableDPD:%d", patchCommon, disableDPD);
}

bool DPD::processKext(KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size) {
	DBGLOG("dpd", "[ DPD::processKext");
	if (disableDPD) {
		return false;
		DBGLOG("dpd", "] DPD::processKext false (disableDPD = true)");
	}

	DBGLOG("dpd", "] DPD::processKext false");
	return false;
}
