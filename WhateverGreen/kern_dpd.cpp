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

static const uint8_t dellUP3218KFind1[] = "\x41\x81\xF8\x47\x41\x00\x00\x75\x2C"; // cmp r8d, 0x4147;  jne xx
static const uint8_t dellUP3218KRepl1[] = "\x41\x81\xF8\x47\x41\x00\x00\xeb\x2C"; // cmp r8d, 0x4147;  jmp xx

static const uint8_t dellUP3218KFind2[] = "\x41\x81\xFD\x47\x41\x00\x00\x75\x2C"; // cmp r13d, 0x4147;  jne xx
static const uint8_t dellUP3218KRepl2[] = "\x41\x81\xFD\x47\x41\x00\x00\xeb\x2C"; // cmp r13d, 0x4147;  jne xx

static UserPatcher::BinaryModPatch binaryPatch[] {
	{
		CPU_TYPE_X86_64, 0, // flags
		overridepathFind, overridepathRepl, arrsize(overridepathFind), // find, replace, size (including extra null byte)
		0, // skip  = 0 -> replace all occurrences
		1, // count = 1 -> 1 set of hex inside the target binaries
		UserPatcher::FileSegment::SegmentTextCstring,
		UserPatcher::ProcInfo::SectionDisabled
	},
	{
		CPU_TYPE_X86_64, 0, // flags
		mtddpathFind, mtddpathRepl, arrsize(mtddpathFind), // find, replace, size (including extra null byte)
		0, // skip  = 0 -> replace all occurrences
		1, // count = 1 -> 1 set of hex inside the target binaries
		UserPatcher::FileSegment::SegmentTextCstring,
		UserPatcher::ProcInfo::SectionDisabled
	},
	{
		CPU_TYPE_X86_64, 0, // flags
		dellUP3218KFind1, dellUP3218KRepl1, arrsize(dellUP3218KFind1) - 1, // find, replace, size (exclude extra null byte)
		0, // skip  = 0 -> replace all occurrences
		1, // count = 1 -> 1 set of hex inside the target binaries
		UserPatcher::FileSegment::SegmentTextText,
		UserPatcher::ProcInfo::SectionDisabled
	},
	{
		CPU_TYPE_X86_64, 0, // flags
		dellUP3218KFind2, dellUP3218KRepl2, arrsize(dellUP3218KFind2) - 1, // find, replace, size (exclude extra null byte)
		0, // skip  = 0 -> replace all occurrences
		1, // count = 1 -> 1 set of hex inside the target binaries
		UserPatcher::FileSegment::SegmentTextText,
		UserPatcher::ProcInfo::SectionDisabled
	},
};

typedef enum {
	dpdOverridePath = 1,
	dpdUP3218KCheck = 2,
} DPDPatchFlags;

static UserPatcher::BinaryModInfo binaryMod { binarydisplaypolicyd, binaryPatch, arrsize(binaryPatch) };

static UserPatcher::ProcInfo procInfo { procdisplaypolicyd, sizeof(procdisplaypolicyd) - 1, UserPatcher::ProcInfo::SectionDisabled };

void DPD::init() {
	// -dpdon -> force enable
	// -dpdoff -> force disable

	DBGLOG("dpd", "[ DPD::init");
	UInt32 patches = 0;
	if (!checkKernelArgument("-dpdoff") && lilu_get_boot_args("dpd", &patches, sizeof(patches))) {
		if (getKernelVersion() < KernelVersion::Catalina) patches &= ~dpdOverridePath;
		if (getKernelVersion() < KernelVersion::Ventura ) patches &= ~dpdUP3218KCheck;
		if (patches & dpdOverridePath) {
			DBGLOG("dpd", "enabled patch for Displays Override path");
			binaryPatch[0].section = 1;
			binaryPatch[1].section = 1;
		}
		if (patches & dpdUP3218KCheck) {
			DBGLOG("dpd", "enabled patch for DellUP3218K check");
			binaryPatch[2].section = 1;
			binaryPatch[3].section = 1;
		}
		if (patches) {
			procInfo.section = 1;
			lilu.onProcLoadForce(&procInfo, 1, nullptr, nullptr, &binaryMod, 1);
		}
	}
	DBGLOG("dpd", "] DPD::init patches:%d", patches);
}

void DPD::deinit() {
	DBGLOG("dpd", "[ DPD::deinit");
	DBGLOG("dpd", "] DPD::deinit");
}

void DPD::processKernel(KernelPatcher &patcher, DeviceInfo *info) {
}

bool DPD::processKext(KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size) {
	return false;
}
