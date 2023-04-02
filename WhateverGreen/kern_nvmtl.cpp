//
//  kern_nvmtl.cpp
//  WhateverGreen
//
//  Copyright © 2022 joevt. All rights reserved.
//
//	This patch serves no purpose except as an example of a user patch.
//

#include "kern_nvmtl.hpp"

#include <Headers/kern_api.hpp>
#include <Headers/kern_iokit.hpp>

// GeForceMTLDriver.bundle
#define pathGeForceMTLDriver "/System/Library/Extensions/GeForceMTLDriver.bundle/Contents/MacOS/GeForceMTLDriver"

// process for 10.12.x and later
#define procWindowServer_Skylight "/System/Library/PrivateFrameworks/SkyLight.framework/Versions/A/Resources/WindowServer"

static const uint8_t find1[] {
	0x48, 0x8D, 0x35, 0xD4, 0x83, 0x07, 0x00,        // lea        rsi, qword [cfstring_com_nvidia_OpenGL] ; argument "applicationID" for method imp___stubs__CFPreferencesCopyAppValue, @"com.nvidia.OpenGL"
	0x49, 0x89, 0xFD,                                // mov        r13, rdi
	0xE8, 0xFA, 0xFB, 0x01, 0x00,                    // call       imp___stubs__CFPreferencesCopyAppValue ; CFPreferencesCopyAppValue
};

static const uint8_t find2[] {
	0x48, 0x8D, 0x35, 0x75, 0x83, 0x07, 0x00,        // lea        rsi, qword [cfstring_com_nvidia_OpenGL] ; argument "applicationID" for method imp___stubs__CFPreferencesCopyAppValue, @"com.nvidia.OpenGL"
	0x49, 0x89, 0xFC,                                // mov        r12, rdi
	0xE8, 0x9B, 0xFB, 0x01, 0x00,                    // call       imp___stubs__CFPreferencesCopyAppValue ; CFPreferencesCopyAppValue
};

static const uint8_t repl1and2[] {
	0x31, 0xC0,               // xor        eax, eax
	0x90,                     // nop
	0x90,                     // nop
	0x90,                     // nop
	0x90,                     // nop
	0x90,                     // nop
	0x90,                     // nop
	0x90,                     // nop
	0x90,                     // nop
	0x90,                     // nop
	0x90,                     // nop
	0x90,                     // nop
	0x90,                     // nop
	0x90,                     // nop
};

static const uint8_t find3[] {
	0xE8, 0x0C, 0xFB, 0x01, 0x00,                    // call       imp___stubs__CFRelease       ; CFRelease, CODE XREF=sub_1677d2+315, sub_1677d2+568
};

static const uint8_t repl3[] {
	0x90, 0x90, 0x90, 0x90, 0x90,                    // nop
};

static UserPatcher::BinaryModPatch bundlePatches[] {
	{
		CPU_TYPE_X86_64, 0, // flags
		find1, repl1and2, arrsize(find1), // find, replace, size
		0, // skip  = 0 -> replace all occurrences
		1, // count = 1 -> 1 set of hex inside the target binaries
		UserPatcher::FileSegment::SegmentTextText,
		1 // enabled
	},
	{
		CPU_TYPE_X86_64, 0, // flags
		find2, repl1and2, arrsize(find2), // find, replace, size
		0, // skip  = 0 -> replace all occurrences
		1, // count = 1 -> 1 set of hex inside the target binaries
		UserPatcher::FileSegment::SegmentTextText,
		1 // enabled
	},
	{
		CPU_TYPE_X86_64, 0, // flags
		find3, repl3, arrsize(find3), // find, replace, size
		0, // skip  = 0 -> replace all occurrences
		1, // count = 1 -> 1 set of hex inside the target binaries
		UserPatcher::FileSegment::SegmentTextText,
		1 // enabled
	},
};

static UserPatcher::BinaryModInfo binaryMod { pathGeForceMTLDriver, bundlePatches, arrsize(bundlePatches) };

static UserPatcher::ProcInfo procInfo { procWindowServer_Skylight, sizeof(procWindowServer_Skylight) - 1, 1 };

void NVMTL::init() {
	DBGLOG("nvmtl", "[ NVMTL::init");
	if (checkKernelArgument("-nvmtlon")) {
		if (getKernelVersion() >= KernelVersion::Monterey && getKernelMinorVersion() >= 6) { // macOS 12.5 == Darwin 21.6
			lilu.onProcLoadForce(&procInfo, 1, nullptr, nullptr, &binaryMod, 1);
		}
	}
	DBGLOG("nvmtl", "] NVMTL::init");
}

void NVMTL::deinit() {
	DBGLOG("nvmtl", "[ NVMTL::deinit");
	DBGLOG("nvmtl", "] NVMTL::deinit");
}

void NVMTL::processKernel(KernelPatcher &patcher, DeviceInfo *info) {
}

bool NVMTL::processKext(KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size) {
	return false;
}
