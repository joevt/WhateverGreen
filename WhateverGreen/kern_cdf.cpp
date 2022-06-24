//
//  kern_cdf.cpp
//  WhateverGreen
//
//  Copyright © 2018 vit9696. All rights reserved.
//

#include "kern_cdf.hpp"

#include <Headers/kern_api.hpp>
#include <Headers/kern_iokit.hpp>

// NVDAGK100Hal.kext    - system built-in, for Kepler
static const char *pathGKHal[] = {
	"/System/Library/Extensions/NVDAGK100Hal.kext/Contents/MacOS/NVDAGK100Hal"
};

// NVDAGK100HalWeb.kext - from web driver, for Kepler
static const char *pathGKWeb[] = {
	/**/   "/Library/Extensions/NVDAGK100HalWeb.kext/Contents/MacOS/NVDAGK100HalWeb",
	"/System/Library/Extensions/NVDAGK100HalWeb.kext/Contents/MacOS/NVDAGK100HalWeb"
};

// NVDAGM100HalWeb.kext - from web driver, for Maxwell
static const char *pathGMWeb[] = {
	/**/   "/Library/Extensions/NVDAGM100HalWeb.kext/Contents/MacOS/NVDAGM100HalWeb",
	"/System/Library/Extensions/NVDAGM100HalWeb.kext/Contents/MacOS/NVDAGM100HalWeb"
};

// NVDAGP100HalWeb.kext - from web driver, for Pascal
static const char *pathGPWeb[] = {
	/**/   "/Library/Extensions/NVDAGP100HalWeb.kext/Contents/MacOS/NVDAGP100HalWeb",
	"/System/Library/Extensions/NVDAGP100HalWeb.kext/Contents/MacOS/NVDAGP100HalWeb"
};

static KernelPatcher::KextInfo kextList[] {
	{ "com.apple.nvidia.driver.NVDAGK100Hal", pathGKHal, arrsize(pathGKHal), {}, {}, KernelPatcher::KextInfo::Unloaded },
	{ "com.nvidia.web.NVDAGK100HalWeb"      , pathGKWeb, arrsize(pathGKWeb), {}, {}, KernelPatcher::KextInfo::Unloaded },
	{ "com.nvidia.web.NVDAGM100HalWeb"      , pathGMWeb, arrsize(pathGMWeb), {}, {}, KernelPatcher::KextInfo::Unloaded },
	{ "com.nvidia.web.NVDAGP100HalWeb"      , pathGPWeb, arrsize(pathGPWeb), {}, {}, KernelPatcher::KextInfo::Unloaded },
};

enum : size_t {
	KextGK100HalSys,
	KextGK100HalWeb,
	KextGM100HalWeb,
	KextGP100HalWeb
};

// target framework for 10.4.x to 10.11.x
#define binaryIOKitFramework       "/System/Library/Frameworks/IOKit.framework/Versions/A/IOKit"
// target framework as of 10.12.x
#define binaryCoreDisplayFramework "/System/Library/Frameworks/CoreDisplay.framework/Versions/A/CoreDisplay"

// accompanied process for 10.4.x to 10.7.x (exists up to 10.11.x)
#define procWindowServer_ApplicationServices "/System/Library/Frameworks/ApplicationServices.framework/Versions/A/Frameworks/CoreGraphics.framework/Versions/A/Resources/WindowServer"
// accompanied process for 10.8.x to 10.11.x
#define procWindowServer_CoreGraphics        "/System/Library/Frameworks/CoreGraphics.framework/Versions/A/Resources/WindowServer"
// accompanied process for 10.12.x to 11.x
#define procWindowServer_Skylight            "/System/Library/PrivateFrameworks/SkyLight.framework/Versions/A/Resources/WindowServer"

// Patches
//
// for NVDAGK100Hal and NVDAGK100HalWeb
//
// Reference:
// https://github.com/Floris497/mac-pixel-clock-patch-V2/blob/master/NVIDIA-patcher.command
//
static const uint8_t gk100Find[] = { 0x88, 0x84, 0x02, 0x00 };
static const uint8_t gk100Repl[] = { 0x80, 0x1A, 0x06, 0x00 };
//
// for NVDAGM100HalWeb and NVDAGP100HalWeb
//
// Reference:
// https://github.com/Floris497/mac-pixel-clock-patch-V2/blob/master/NVIDIA-WEB-MAXWELL-patcher.command
//
static const uint8_t gmp100Find[] = { 0x88, 0x84, 0x02, 0x00 };
static const uint8_t gmp100Repl[] = { 0x40, 0x42, 0x0F, 0x00 };
//
// for frameworks
//
// Reference:
// https://github.com/Floris497/mac-pixel-clock-patch-V2/blob/master/CoreDisplay-patcher.command
//
// Modified by PMheart (jmpq adress optimisations)
//
// Patches int CheckTimingWithRange(IOFBConnectRef, IODisplayTimingRange *, IODetailedTimingInformation *)
// to always return 0 and thus effectively allow the use of any resolution regardless of connection
// capability. May cause blackscreen with several configurations.

// 10.4.11
static const uint8_t frameworkFind_10_4[] {
	0xA8, 0x01,                                     // test al, 0x1
	0x74, 0x0A,                                     // je +10
	0xB8, 0x01, 0x00, 0x00, 0x00,                   // mov eax, 0x1
	0xE9                                            // jmp <somewhere>
};

static const uint8_t frameworkRepl_10_4[] {
	0x90, 0x90, 0x90, 0x90,                         // nop (4x)
	0xB8, 0x00, 0x00, 0x00, 0x00,                   // mov eax, 0x0
	0xE9                                            // jmp <somewhere>
};

// 10.5.8
static const uint8_t frameworkFind_10_5[] {
	0x89, 0xD0,                                     // mov eax, edx
	0x83, 0xE0, 0x01,                               // and eax, 0x1
	0x85, 0xC0,                                     // test eax, eax
	0x0F, 0x85                                      // jne <somewhere>
};

static const uint8_t frameworkRepl_10_5[] {
	0x90, 0x90, 0x90,                               // nop (3x)
	0xB9, 0x00, 0x00, 0x00, 0x00,                   // mov ecx, 0x0
	0xE9                                            // jmp <somewhere>
};

// 10.6.8
static const uint8_t frameworkFind_10_6[] {
	0xB9, 0x01, 0x00, 0x00, 0x00,                   // mov ecx, 0x1
	0xA8, 0x01,                                     // test al, 0x1
	0x0F, 0x85                                      // jne <somewhere>
};

static const uint8_t frameworkRepl_10_6[] {
	0x90, 0x90, 0x90,                               // nop (3x)
	0xB9, 0x00, 0x00, 0x00, 0x00,                   // mov ecx, 0x0
	0xE9                                            // jmp <somewhere>
};

// 10.7.5
static const uint8_t frameworkFind_10_7[] {
	0xF6, 0xC1, 0x01,                               // test cl, 0x1
	0x74, 0x0A,                                     // je +10
	0xB8, 0x01, 0x00, 0x00, 0x00,                   // mov eax, 0x1
	0xE9                                            // jmp <somewhere>
};

static const uint8_t frameworkRepl_10_7[] {
	0x90, 0x90, 0x90, 0x90, 0x90,                   // nop (5x)
	0xB8, 0x00, 0x00, 0x00, 0x00,                   // mov eax, 0x0
	0xE9                                            // jmp <somewhere>
};

// 10.8.5
static const uint8_t frameworkFind_10_8[] {
	0xBF, 0x01, 0x00, 0x00, 0x00,                   // mov edi, 0x1
	0xA8, 0x01,                                     // test al, 0x1
	0x0F, 0x85                                      // jne <somewhere>
};

static const uint8_t frameworkRepl_10_8[] {
	0x90, 0x90, 0x90,                               // nop (3x)
	0xBF, 0x00, 0x00, 0x00, 0x00,                   // mov edi, 0x0
	0xE9                                            // jmp <somewhere>
};

// 10.9.5, 10.10.5, 10.11.6, 10.12.6, 10.13.3
static const uint8_t frameworkFind_10_9[] {
	0xB8, 0x01, 0x00, 0x00, 0x00,                   // mov  eax, 0x1
	0xF6, 0xC1, 0x01,                               // test cl, 0x1
	0x0F, 0x85                                      // jne  <somewhere>
};

static const uint8_t frameworkRepl_10_9[] {
	0x90, 0x90, 0x90, 0x90,                         // nop (4x)
	0xB8, 0x00, 0x00, 0x00, 0x00,                   // mov  eax, 0x0
	0xE9                                            // jmp <somewhere>
};

// 10.13.4, 10.13.6 // use more specific match below for 10.14.6, 10.15.7
static const uint8_t frameworkFind_10_13_4[] {
	0xBB, 0x01, 0x00, 0x00, 0x00,                   // mov ebx, 0x1
	0xA8, 0x01,                                     // test al, 0x1
	0x0F, 0x85                                      // jne <somewhere>
};

static const uint8_t frameworkRepl_10_13_4[] {
	0x90, 0x90, 0x90,                               // nop (3x)
	0xBB, 0x00, 0x00, 0x00, 0x00,                   // mov ebx, 0x0
	0xE9                                            // jmp <somewhere>
};

// 10.14.6, 10.15.7, 11.6.4, 12.2.1
// Make sure there's only one match in each dyld shared cache (especially if we don't have the code to limit the patch to the CoreDisplay part of the cache)
// LANG=C grep -obUa "\x8B\x42\x20\xBB\x01\x00\x00\x00\xA8\x01\x0F\x85" /Volumes/*/S*/L*/dyld/dyld_shared_cache_x86_64* /Volumes/*/S*/L*/F*/CoreDisplay.framework/Versions/A/CoreDisplay
// Don't run this test on a Mac that has dyld patches enabled - the file may appear to have patches applied to it but that's only in RAM.
static const uint8_t frameworkFind_10_14[] {
	0x8B, 0x42, 0x20,                               // mov eax, dword [rdx+0x20]
	0xBB, 0x01, 0x00, 0x00, 0x00,                   // mov ebx, 0x1
	0xA8, 0x01,                                     // test al, 0x1
	0x0F, 0x85                                      // jne <somewhere>
};

static const uint8_t frameworkRepl_10_14[] {
	0x90, 0x90, 0x90, 0x90, 0x90, 0x90,             // nop (6x)
	0xBB, 0x00, 0x00, 0x00, 0x00,                   // mov ebx, 0x0
	0xE9                                            // jmp <somewhere>
};

static UserPatcher::BinaryModPatch frameworkPatch {
	CPU_TYPE_X86_64, 0, // flags
	NULL, NULL, 0, // find, replace, size
	0, // skip  = 0 -> replace all occurrences
	1, // count = 1 -> 1 set of hex inside the target binaries
	UserPatcher::FileSegment::SegmentTextText,
	1 // enabled
};

static UserPatcher::BinaryModInfo binaryMod { NULL, &frameworkPatch, 1 };

static UserPatcher::ProcInfo procInfo { NULL, 0, 1 };

CDF *CDF::callbackCDF;

void CDF::init() {
	DBGLOG("cdf", "[ CDF::init");
	callbackCDF = this;
	lilu.onKextLoadForce(kextList, arrsize(kextList));

	// nothing should be applied when -cdfoff is passed
	if (!checkKernelArgument("-cdfoff")) {
		#define onepatch(_proc, _bin, _patch, _ver) \
		if (_ver) { \
			procInfo.path = procWindowServer_ ## _proc; \
			procInfo.len = sizeof(procWindowServer_ ## _proc) - 1; \
			binaryMod.path = binary ## _bin ## Framework; \
			frameworkPatch.find = frameworkFind_ ## _patch; \
			frameworkPatch.replace = frameworkRepl_ ## _patch; \
			frameworkPatch.size = arrsize(frameworkFind_ ## _patch); \
		}
		if (0) {}
		else onepatch(ApplicationServices, IOKit      , 10_4   ,  getKernelVersion() == KernelVersion::Tiger)
		else onepatch(ApplicationServices, IOKit      , 10_5   ,  getKernelVersion() == KernelVersion::Leopard)
		else onepatch(ApplicationServices, IOKit      , 10_6   ,  getKernelVersion() == KernelVersion::SnowLeopard)
		else onepatch(ApplicationServices, IOKit      , 10_7   ,  getKernelVersion() == KernelVersion::Lion)
		else onepatch(CoreGraphics       , IOKit      , 10_8   ,  getKernelVersion() == KernelVersion::MountainLion)
		else onepatch(CoreGraphics       , IOKit      , 10_9   ,  getKernelVersion() >= KernelVersion::Mavericks  && getKernelVersion() <= KernelVersion::ElCapitan)
		else onepatch(Skylight           , CoreDisplay, 10_9   , (getKernelVersion() == KernelVersion::HighSierra && getKernelMinorVersion() <  5) || getKernelVersion() == KernelVersion::Sierra)
		else onepatch(Skylight           , CoreDisplay, 10_13_4,  getKernelVersion() == KernelVersion::HighSierra && getKernelMinorVersion() >= 5)
		else onepatch(Skylight           , CoreDisplay, 10_14  ,  getKernelVersion() >= KernelVersion::Mojave)
	}
	
	if (procInfo.path) {
		currentProcInfo = &procInfo;
		currentModInfo = &binaryMod;
		lilu.onProcLoadForce(currentProcInfo, 1, nullptr, nullptr, currentModInfo, 1);
	}
	DBGLOG("cdf", "] CDF::init");
}

void CDF::deinit() {
	DBGLOG("cdf", "[ CDF::deinit");
	DBGLOG("cdf", "] CDF::deinit");
}

void CDF::processKernel(KernelPatcher &patcher, DeviceInfo *info) {
	// -cdfon -> force enable
	// -cdfoff -> force disable
	// enable-hdmi20 -> enable nvidia/intel
	DBGLOG("cdf", "[ CDF::processKernel");
	disableHDMI20 = checkKernelArgument("-cdfoff");

	bool patchNVIDIA = false;
	bool patchCommon = false;

	if (!disableHDMI20) {
		if (checkKernelArgument("-cdfon")) {
			patchNVIDIA = patchCommon = true;
		} else {
			for (size_t i = 0; i < info->videoExternal.size(); i++) {
				if (info->videoExternal[i].vendor == WIOKit::VendorID::NVIDIA)
					patchNVIDIA |= info->videoExternal[i].video->getProperty("enable-hdmi20") != nullptr;
			}

			if (patchNVIDIA || (info->videoBuiltin && info->videoBuiltin->getProperty("enable-hdmi20")))
				patchCommon = true;
		}
	}

	if (!patchNVIDIA) {
		for (size_t i = 0; i < arrsize(kextList); i++)
			kextList[i].switchOff();
		disableHDMI20 = true;
	}

	if (!patchCommon && currentProcInfo && currentModInfo) {
		currentProcInfo->section = UserPatcher::ProcInfo::SectionDisabled;
		for (size_t i = 0; i < currentModInfo->count; i++)
			currentModInfo->patches[i].section = UserPatcher::ProcInfo::SectionDisabled;
	}
	DBGLOG("cdf", "] CDF::processKernel patchNVIDIA:%d patchCommon:%d disableHDMI20:%d", patchNVIDIA, patchCommon, disableHDMI20);
}

bool CDF::processKext(KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size) {
	DBGLOG("cdf", "[ CDF::processKext");
	if (disableHDMI20) {
		return false;
		DBGLOG("cdf", "] CDF::processKext false (disableHDMI20 = true)");
	}

	if (kextList[KextGK100HalSys].loadIndex == index) {
		KernelPatcher::LookupPatch patch {&kextList[KextGK100HalSys], gk100Find, gk100Repl, sizeof(gk100Find), 1};
		patcher.applyLookupPatch(&patch);
		if (patcher.getError() != KernelPatcher::Error::NoError) {
			SYSLOG("cdf", "failed to apply gk100 patch %d", patcher.getError());
			patcher.clearError();
		}
		DBGLOG("cdf", "] CDF::processKext true (KextGK100HalSys)");
		return true;
	}

	if (kextList[KextGK100HalWeb].loadIndex == index) {
		KernelPatcher::LookupPatch patch {&kextList[KextGK100HalWeb], gk100Find, gk100Repl, sizeof(gk100Find), 1};
		patcher.applyLookupPatch(&patch);
		if (patcher.getError() != KernelPatcher::Error::NoError) {
			SYSLOG("cdf", "failed to apply gk100 web patch %d", patcher.getError());
			patcher.clearError();
		}
		DBGLOG("cdf", "] CDF::processKext true (KextGK100HalWeb)");
		return true;
	}

	if (kextList[KextGM100HalWeb].loadIndex == index) {
		KernelPatcher::LookupPatch patch {&kextList[KextGM100HalWeb], gmp100Find, gmp100Repl, sizeof(gmp100Find), 1};
		patcher.applyLookupPatch(&patch);
		if (patcher.getError() != KernelPatcher::Error::NoError) {
			SYSLOG("cdf", "failed to apply gm100 web patch %d", patcher.getError());
			patcher.clearError();
		}
		DBGLOG("cdf", "] CDF::processKext true (KextGM100HalWeb)");
		return true;
	}

	if (kextList[KextGP100HalWeb].loadIndex == index) {
		KernelPatcher::LookupPatch patch {&kextList[KextGP100HalWeb], gmp100Find, gmp100Repl, sizeof(gmp100Find), 1};
		patcher.applyLookupPatch(&patch);
		if (patcher.getError() != KernelPatcher::Error::NoError) {
			SYSLOG("cdf", "failed to apply gp100 web patch %d", patcher.getError());
			patcher.clearError();
		}
		DBGLOG("cdf", "] CDF::processKext true (KextGP100HalWeb)");
		return true;
	}

	DBGLOG("cdf", "] CDF::processKext false");
	return false;
}
