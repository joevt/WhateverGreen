//
//  kern_iofbdebug.cpp
//  WhateverGreen
//
//  Created by joevt on 2022-04-23.
//  Copyright © 2022 vit9696. All rights reserved.
//

#include <Headers/kern_api.hpp>
#include <Headers/kern_cpu.hpp>
#include <Headers/kern_devinfo.hpp>
#include <Headers/kern_iokit.hpp>
#include <Headers/kern_file.hpp>

#include "kern_iofbdebug.hpp"

#include <IOKit/i2c/IOI2CInterface.h>
#include <IOKit/ndrvsupport/IONDRVLibraries.h>
#include <IOKit/graphics/IOGraphicsTypes.h>


int bprintf(char * buf, size_t bufSize, const char * format, ...) __printflike(3, 4);

int bprintf(char * buf, size_t bufSize, const char * format, ...) {
	// can't use scnprintf for Mojave installer or earlier installers.
	va_list va;
	va_start(va, format);
	int len = vsnprintf(buf, bufSize, format, va);
	va_end(va);
	return (len > bufSize) ? (int)bufSize : len;
}


//========================================================================================
// dpcd info

/* Sideband MSG Buffers */
#define DP_SIDEBAND_MSG_DOWN_REQ_BASE   0x1000   /* 1.2 MST */
#define DP_SIDEBAND_MSG_UP_REP_BASE     0x1200   /* 1.2 MST */
#define DP_SIDEBAND_MSG_DOWN_REP_BASE   0x1400   /* 1.2 MST */
#define DP_SIDEBAND_MSG_UP_REQ_BASE     0x1600   /* 1.2 MST */

#define DP_DEVICE_SERVICE_IRQ_VECTOR        0x201
# define DP_REMOTE_CONTROL_COMMAND_PENDING  (1 << 0)
# define DP_AUTOMATED_TEST_REQUEST          (1 << 1)
# define DP_CP_IRQ                          (1 << 2)
# define DP_MCCS_IRQ                        (1 << 3)
# define DP_DOWN_REP_MSG_RDY                (1 << 4) /* 1.2 MST */
# define DP_UP_REQ_MSG_RDY                  (1 << 5) /* 1.2 MST */
# define DP_SINK_SPECIFIC_IRQ               (1 << 6)

//========================================================================================
// Apple private or old or inaccessible

#define kConnectionUnderscan 'pscn'
// https://github.com/apple-oss-distributions/IOGraphics/blob/main/IOGraphicsFamily/IOFramebuffer.cpp
#define kTempAttribute	'thrm'

// <IOKit/graphics/IOGraphicsTypes.h>
// kConnectionColorMode attribute
enum {
	kIODisplayColorModeReserved   = 0x00000000,
	kIODisplayColorModeRGB        = 0x00000001,
	kIODisplayColorModeYCbCr422   = 0x00000010,
	kIODisplayColorModeYCbCr444   = 0x00000100,
	kIODisplayColorModeRGBLimited = 0x00001000,
	kIODisplayColorModeAuto       = 0x10000000,
};

// https://github.com/apple-oss-distributions/IOGraphics/blob/main/IOGraphicsFamily/IOKit/graphics/IOGraphicsTypesPrivate.h

enum { // 30
	// This is the ID given to a programmable timing used at boot time
	k1 = kIODisplayModeIDBootProgrammable, // = (IODisplayModeID)0xFFFFFFFB,
	// Lowest (unsigned) DisplayModeID reserved by Apple
	k2 = kIODisplayModeIDReservedBase, // = (IODisplayModeID)0x80000000

	// This is the ID given to a programmable timing used at boot time
	kIODisplayModeIDInvalid     = (IODisplayModeID) 0xFFFFFFFF,
	kIODisplayModeIDCurrent     = (IODisplayModeID) 0x00000000,
	kIODisplayModeIDAliasBase   = (IODisplayModeID) 0x40000000,

	// https://github.com/apple-oss-distributions/IOKitUser/blob/rel/IOKitUser-1445/graphics.subproj/IOGraphicsLib.c
	kIOFBSWOfflineDisplayModeID = (IODisplayModeID) 0xffffff00,
	kArbModeIDSeedMask           = (IODisplayModeID) 0x00007000, //  (connectRef->arbModeIDSeed is incremented everytime IOFBBuildModeList is called) // https://github.com/apple-oss-distributions/IOKitUser/blob/rel/IOKitUser-1445/graphics.subproj/IOGraphicsLibInternal.h
};

enum { // 36
	// options for IOServiceRequestProbe()
	kIOFBForceReadEDID                  = 0x00000100,
	kIOFBAVProbe                        = 0x00000200,
	kIOFBSetTransform                   = 0x00000400,
	kIOFBTransformShift                 = 16,
	kIOFBScalerUnderscan                = 0x01000000,
};

enum { // 45
	// transforms
	kIOFBRotateFlags                    = 0x0000000f,

	kIOFBSwapAxes                       = 0x00000001,
	kIOFBInvertX                        = 0x00000002,
	kIOFBInvertY                        = 0x00000004,

	kIOFBRotate0                        = 0x00000000,
	kIOFBRotate90                       = kIOFBSwapAxes | kIOFBInvertX,
	kIOFBRotate180                      = kIOFBInvertX  | kIOFBInvertY,
	kIOFBRotate270                      = kIOFBSwapAxes | kIOFBInvertY
};

enum { // 71
	// Controller attributes
	kIOFBSpeedAttribute                 = ' dgs',
	kIOFBWSStartAttribute               = 'wsup',
	kIOFBProcessConnectChangeAttribute  = 'wsch',
	kIOFBEndConnectChangeAttribute      = 'wsed',

	kIOFBMatchedConnectChangeAttribute  = 'wsmc',

	// Connection attributes
	kConnectionInTVMode                 = 'tvmd',
	kConnectionWSSB                     = 'wssb',

	kConnectionRawBacklight             = 'bklt',
	kConnectionBacklightSave            = 'bksv',

	kConnectionVendorTag                = 'vtag'
};

/*! @enum FramebufferConstants
	@constant kIOFBVRAMMemory The memory type for IOConnectMapMemory() to get the VRAM memory. Use a memory type equal to the IOPixelAperture index to get a particular pixel aperture.
*/
enum { // 106
	kIOFBVRAMMemory = 110
};

#define kIOFBGammaHeaderSizeKey         "IOFBGammaHeaderSize" // 110

#define kIONDRVFramebufferGenerationKey "IONDRVFramebufferGeneration"

#define kIOFramebufferOpenGLIndexKey    "IOFramebufferOpenGLIndex"

#define kIOFBCurrentPixelClockKey       "IOFBCurrentPixelClock"
#define kIOFBCurrentPixelCountKey       "IOFBCurrentPixelCount"
#define kIOFBCurrentPixelCountRealKey   "IOFBCurrentPixelCountReal"

#define kIOFBTransformKey               "IOFBTransform"
#define kIOFBRotatePrefsKey             "framebuffer-rotation"
#define kIOFBStartupTimingPrefsKey      "startup-timing"

#define kIOFBCapturedKey                "IOFBCaptured"

#define kIOFBMirrorDisplayModeSafeKey   "IOFBMirrorDisplayModeSafe"

#define kIOFBConnectInterruptDelayKey   "connect-interrupt-delay"

#define kIOFBUIScaleKey					"IOFBUIScale"

#define kIOGraphicsPrefsKey             "IOGraphicsPrefs"
#define kIODisplayPrefKeyKey            "IODisplayPrefsKey"
#define kIODisplayPrefKeyKeyOld         "IODisplayPrefsKeyOld"
#define kIOGraphicsPrefsParametersKey   "IOGraphicsPrefsParameters"
#define kIOGraphicsIgnoreParametersKey  "IOGraphicsIgnoreParameters" // 136

#define detailedTimingModeID            __reservedA[0] // 156

#define kIOFBDPDeviceIDKey          "dp-device-id" // 214
#define kIOFBDPDeviceTypeKey        "device-type"
#define kIOFBDPDeviceTypeDongleKey  "branch-device"

enum // 218
{
	kDPRegisterLinkStatus      = 0x200,
	kDPRegisterLinkStatusCount = 6,
	kDPRegisterServiceIRQ      = 0x201,
};

enum
{
	kDPLinkStatusSinkCountMask = 0x3f,
};

enum
{
	kDPIRQRemoteControlCommandPending = 0x01,
	kDPIRQAutomatedTestRequest        = 0x02,
	kDPIRQContentProtection           = 0x04,
	kDPIRQMCCS                        = 0x08,
	kDPIRQSinkSpecific                = 0x40,
};

enum // 244
{
	// values for graphic-options & kIOMirrorDefaultAttribute
//  kIOMirrorDefault        = 0x00000001,
//  kIOMirrorForced         = 0x00000002,
	kIOGPlatformYCbCr       = 0x00000004,
	kIOFBDesktopModeAllowed = 0x00000008,   // https://github.com/apple-oss-distributions/IOGraphics/blob/rel/IOGraphics-305/IOGraphicsFamily/IOFramebuffer.cpp // gIOFBDesktopModeAllowed
//  kIOMirrorHint           = 0x00010000,
	kIOMirrorNoAutoHDMI     = 0x00000010,
	kIOMirrorHint           = 0x00010000,	// https://github.com/apple-oss-distributions/IOGraphics/blob/rel/IOGraphics-585/IONDRVSupport/IONDRVFramebuffer.cpp
};




//========================================================================================
// IOFB patch

static const char *pathIOGraphics[] { "/System/Library/Extensions/IOGraphicsFamily.kext/IOGraphicsFamily" };

static KernelPatcher::KextInfo kextList[] {
	{ "com.apple.iokit.IOGraphicsFamily", pathIOGraphics, arrsize(pathIOGraphics), {true}, {}, KernelPatcher::KextInfo::Unloaded },
};

enum : size_t {
	KextIOGraphics,
};


IOFB *IOFB::callbackIOFB;


void IOFB::init() {
	DBGLOG("iofb", "[ init");
	callbackIOFB = this;
	lilu.onKextLoadForce(kextList, arrsize(kextList));

	DBGLOG("iofb", "] init");
}

void IOFB::deinit() {
	DBGLOG("iofb", "[ deinit");
	DBGLOG("iofb", "] deinit");
}

void IOFB::processKernel(KernelPatcher &patcher, DeviceInfo *info) {
	// -iofbon -> force enable
	// -iofboff -> force disable
	DBGLOG("iofb", "[ processKernel");
	disableIOFB = checkKernelArgument("-iofboff");

	bool patchIODB = false;

	if (!disableIOFB) {
		if (checkKernelArgument("-iofbon")) {
			patchIODB = true;
		}
	}

	if (!patchIODB) {
		for (size_t i = 0; i < arrsize(kextList); i++)
			kextList[i].switchOff();
		disableIOFB = true;
	}

	DBGLOG("iofb", "] processKernel patchIODB:%d disableIOFB:%d", patchIODB, disableIOFB);
}


bool IOFB::processKext(KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size) {
	//DBGLOG("iofb", "[ processKext");

	if (kextList[KextIOGraphics].loadIndex == index) {
		// __ZN13IOFramebuffer6attachEP9IOService:        // IOFramebuffer::attach(IOService*)
		KernelPatcher::RouteRequest request("__ZN13IOFramebuffer6initFBEv", wrapFramebufferInit, orgFramebufferInit);
		patcher.routeMultiple(index, &request, 1, address, size);

		if (!gIOGATFlagsPtr) {
			gIOGATFlagsPtr = (uint32_t *)patcher.solveSymbol<void *>(index, "_gIOGATFlags", address, size);
		}

		DBGLOG("iofb", "[] processKext kextIOGraphics true");
		return true;
	}

	//DBGLOG("iofb", "] processKext false");
	return false;
}


IOFB::IOFBvtable *IOFB::getIOFBvtable(IOFramebuffer *service) {
	uintptr_t vtable = (UInt64)reinterpret_cast<uintptr_t **>(service)[0];

	for (int i = 0; i < callbackIOFB->iofbvtablesCount; i++) {
		if (callbackIOFB->iofbvtables[i] && callbackIOFB->iofbvtables[i]->vtable == vtable) {
			return callbackIOFB->iofbvtables[i];
		}
	}

	if (callbackIOFB->iofbvtablesCount == 0) {
		callbackIOFB->iofbvtables = new IOFBvtablePtr[100];
		if (!callbackIOFB->iofbvtables) {
			DBGLOG("iofb", "[] getIOFBvtable cannot create list of vtables");
			return NULL;
		}
	}

	if (callbackIOFB->iofbvtablesCount >= 100) {
		DBGLOG("iofb", "[] getIOFBvtable reached maximum vtable count");
		return NULL;
	}

	int currentiofbvtablescount = OSIncrementAtomic(&callbackIOFB->iofbvtablesCount);
	if (currentiofbvtablescount >= 100) {
		DBGLOG("iofb", "[] getIOFBvtable reached maximum vtable count in another thread");
		callbackIOFB->iofbvtablesCount = 100;
		return NULL;
	}

	callbackIOFB->iofbvtables[currentiofbvtablescount] = new IOFBvtable;
	if (!callbackIOFB->iofbvtables[currentiofbvtablescount]) {
		DBGLOG("iofb", "[] getIOFBvtable cannot create storage");
		return NULL;
	}

	DBGLOG("iofb", "[] getIOFBvtable create storage");
	callbackIOFB->iofbvtables[currentiofbvtablescount]->vtable = vtable;
	return callbackIOFB->iofbvtables[currentiofbvtablescount];
}


IOFB::IOFBVars *IOFB::getIOFBVars(IOFramebuffer *service) {
	for (int i = 0; i < callbackIOFB->iofbvarsCount; i++) {
		if (callbackIOFB->iofbvars[i] && callbackIOFB->iofbvars[i]->fb == service) {
			return callbackIOFB->iofbvars[i];
		}
	}

	if (callbackIOFB->iofbvarsCount == 0) {
		callbackIOFB->iofbvars = new IOFBVarsPtr[100];
		if (!callbackIOFB->iofbvars) {
			DBGLOG("iofb", "[] getIOFBVars cannot create list of framebuffers");
			return NULL;
		}
	}

	if (callbackIOFB->iofbvarsCount >= 100) {
		DBGLOG("iofb", "[] getIOFBVars reached maximum framebuffer count");
		return NULL;
	}

	int currentiofbvarscount = OSIncrementAtomic(&callbackIOFB->iofbvarsCount);
	if (currentiofbvarscount >= 100) {
		DBGLOG("iofb", "[] getIOFBVars reached maximum framebuffer count in another thread");
		callbackIOFB->iofbvarsCount = 100;
		return NULL;
	}

	callbackIOFB->iofbvars[currentiofbvarscount] = new IOFBVars;
	if (!callbackIOFB->iofbvars[currentiofbvarscount]) {
		DBGLOG("iofb", "[] getIOFBVars cannot create storage");
		return NULL;
	}

	DBGLOG("iofb", "[] getIOFBVars create storage");
	callbackIOFB->iofbvars[currentiofbvarscount]->fb = (IOFramebuffer*)service;
	callbackIOFB->iofbvars[currentiofbvarscount]->iofbvtable = getIOFBvtable(service);
	return callbackIOFB->iofbvars[currentiofbvarscount];
}


void IOFB::wrapFramebufferInit(IOFramebuffer *fb) {
	DBGLOG("iofb", "[ wrapFramebufferInit fb:0x%llx vtable:0x%llx", (UInt64)fb, (UInt64)reinterpret_cast<uintptr_t **>(fb)[0]);

	IOFBVars *iofbVars = callbackIOFB->getIOFBVars(fb);
	if (iofbVars) {
		size_t maxvtableIndex = 0;

		if (fb->metaCast("IONDRVFramebuffer")) {
			maxvtableIndex = IOFramebuffer_vtableIndex::getNotificationSemaphore;
			DBGLOG("iofb", "maxvtableIndex = IONDRVFramebuffer::getNotificationSemaphore = %d", (int)maxvtableIndex);
		} else if (fb->metaCast("IOFramebuffer")) {
			maxvtableIndex = IOFramebuffer_vtableIndex::undefinedSymbolHandler;
			DBGLOG("iofb", "maxvtableIndex = IOFramebuffer::undefinedSymbolHandler = %d", (int)maxvtableIndex);
		}

		/*
			Why doesn't OSDynamicCast work in this context?
			if (OSDynamicCast(IONDRVFramebuffer, fb)) {
				...
			} else if (OSDynamicCast(IOFramebuffer, fb)) {
				...
			}
		*/

		#define onevtableitem1(_patch, _index, _result, _name, _params) \
			if (IOFB::IOFramebuffer_vtableIndex::_name <= maxvtableIndex) { \
				if (0) DBGLOG("iofb", "routeVirtual IOFB::IOFramebuffer_vtableIndex::%s = %d, wrap%s, &iofbVars->org%s", # _name, (int)IOFB::IOFramebuffer_vtableIndex::_name, # _name, # _name); \
				KernelPatcher::routeVirtual(fb, IOFB::IOFramebuffer_vtableIndex::_name, wrap ## _name, &iofbVars->iofbvtable->org ## _name); \
			}
		#define onevtableitem0(...)
		#define onevtableitem(_patch, _index, _result, _name, _params) onevtableitem ## _patch(_patch, _index, _result, _name, _params)
		#include "IOFramebuffer_vtable.hpp"
	}

	//DBGLOG("iofb", "[ wrapFramebufferInit");
	FunctionCast(wrapFramebufferInit, callbackIOFB->orgFramebufferInit)(fb);
	//DBGLOG("iofb", "] wrapFramebufferInit");

	DBGLOG("iofb", "] wrapFramebufferInit");
}

//========================================================================================
//  How to convert IOFramebuffer_vtable.hpp into wrap* functions:

/*

Search:
\s*onevtableitem\(\s*(\w*)\s*,\s*(.*?)\s*,\s*(.*?)\s*,\s*(\w+)\s*,\s*(\(.*\))\s*\)
Replace:
\3 IOFB::wrap\4\5\n{\n\tDBGLOG("iofb", "[ \4");\n\t\3 result = iofbVars->org\4\5;\n\tDBGLOG("iofb", "] \4");\n\treturn result;\n}\n\n

Search:
(iofbVars->\w+\( (\w+, )*)[^,\n)]+\b(\w+)
Replace:
\1\3
Repeat until not found.

Search:
void result = (.*\n.*\n)\treturn result;\n
Replace:
\1


*/


//========================================================================================
// String functions


char * UNKNOWN_FLAG(char * buf, size_t bufSize, UInt32 value) {
	buf[0] = '\0';
	if (value) {
		bprintf(buf, bufSize, "?0x%x,", value);
	}
	return buf;
}

char * UNKNOWN_VALUE(char * buf, size_t bufSize, unsigned long value) {
	bprintf(buf, bufSize, "?0x%lx", value);
	return buf;
}

char * ONE_VALUE(char * buf, size_t bufSize, const char *format, unsigned long value) {
	bprintf(buf, bufSize, format, value);
	return buf;
}

char * HEX(char * buf, size_t bufSize, void* bytes, size_t byteslen) {
	char *result = buf;
	buf[0] = '\0';
	size_t inc;
	if (bytes) {
		while (byteslen && bufSize > 2) {
			inc = bprintf(buf, bufSize, "%02x", *(UInt8*)bytes);
			buf += inc;
			bufSize -= inc;
			byteslen--;
			bytes = (UInt8*)bytes + 1;
		}
	}
	else {
		inc = bprintf(buf, bufSize, "NULL");
	}
	return result;
}

#ifdef DEBUG
static char * GetOneFlagsStr(char * flagsstr, size_t size, UInt64 flags) {
	if (flagsstr) {
		bprintf(flagsstr, size, "%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s",
			flags & kDisplayModeValidFlag              ?                  "Valid," : "",
			flags & kDisplayModeSafeFlag               ?                   "Safe," : "",
			flags & kDisplayModeDefaultFlag            ?                "Default," : "",
			flags & kDisplayModeAlwaysShowFlag         ?             "AlwaysShow," : "",
			flags & kDisplayModeNotResizeFlag          ?              "NotResize," : "",
			flags & kDisplayModeRequiresPanFlag        ?            "RequiresPan," : "",
			flags & kDisplayModeInterlacedFlag         ?             "Interlaced," : "",
			flags & kDisplayModeNeverShowFlag          ?              "NeverShow," : "",
			flags & kDisplayModeSimulscanFlag          ?              "Simulscan," : "",
			flags & kDisplayModeNotPresetFlag          ?              "NotPreset," : "",
			flags & kDisplayModeBuiltInFlag            ?                "BuiltIn," : "",
			flags & kDisplayModeStretchedFlag          ?              "Stretched," : "",
			flags & kDisplayModeNotGraphicsQualityFlag ?     "NotGraphicsQuality," : "",
			flags & kDisplayModeValidateAgainstDisplay ? "ValidateAgainstDisplay," : "",
			flags & kDisplayModeTelevisionFlag         ?             "Television," : "",
			flags & kDisplayModeValidForMirroringFlag  ?      "ValidForMirroring," : "",
			flags & kDisplayModeAcceleratorBackedFlag  ?      "AcceleratorBacked," : "",
			flags & kDisplayModeValidForHiResFlag      ?          "ValidForHiRes," : "",
			flags & kDisplayModeValidForAirPlayFlag    ?        "ValidForAirPlay," : "",

			(flags & 0x0000c000) == 0x00004000         ?                 "¿4<<12," :
			(flags & 0x0000c000) == 0x00008000         ?                 "¿8<<12," :
			(flags & 0x0000c000) == 0x0000c000         ?                 "¿C<<12," : "",

			(flags & 0x000f0000) == 0x00010000         ?                 "¿1<<16," :
			(flags & 0x000f0000) == 0x00020000         ?                 "¿2<<16," :
			(flags & 0x000f0000) == 0x00030000         ?                 "¿3<<16," :
			(flags & 0x000f0000) == 0x00040000         ?                 "¿4<<16," :
			(flags & 0x000f0000) == 0x00050000         ?                 "¿5<<16," :
			(flags & 0x000f0000) == 0x00060000         ?                 "¿6<<16," :
			(flags & 0x000f0000) == 0x00070000         ?                 "¿7<<16," :
			(flags & 0x000f0000) == 0x00080000         ?                 "¿8<<16," :
			(flags & 0x000f0000) == 0x00090000         ?                 "¿9<<16," :
			(flags & 0x000f0000) == 0x000a0000         ?                 "¿A<<16," :
			(flags & 0x000f0000) == 0x000b0000         ?                 "¿B<<16," :
			(flags & 0x000f0000) == 0x000c0000         ?                 "¿C<<16," :
			(flags & 0x000f0000) == 0x000d0000         ?                 "¿D<<16," :
			(flags & 0x000f0000) == 0x000e0000         ?                 "¿E<<16," :
			(flags & 0x000f0000) == 0x000f0000         ?                 "¿F<<16," : "",

			(flags & 0x0c000000) == 0x04000000         ?                 "¿4<<24," :
			(flags & 0x0c000000) == 0x08000000         ?                 "¿8<<24," :
			(flags & 0x0c000000) == 0x0c000000         ?                 "¿C<<24," : "",

			(flags & 0xf0000000) == 0x10000000         ?                 "¿1<<28," :
			(flags & 0xf0000000) == 0x20000000         ?                 "¿2<<28," :
			(flags & 0xf0000000) == 0x30000000         ?                 "¿3<<28," :
			(flags & 0xf0000000) == 0x40000000         ?                 "¿4<<28," :
			(flags & 0xf0000000) == 0x50000000         ?                 "¿5<<28," :
			(flags & 0xf0000000) == 0x60000000         ?                 "¿6<<28," : // used only by CGSDisplayModeDescription.flags for the 10bpc modes
			(flags & 0xf0000000) == 0x70000000         ?                 "¿7<<28," :
			(flags & 0xf0000000) == 0x80000000         ?                 "¿8<<28," :
			(flags & 0xf0000000) == 0x90000000         ?                 "¿9<<28," :
			(flags & 0xf0000000) == 0xa0000000         ?                 "¿A<<28," :
			(flags & 0xf0000000) == 0xb0000000         ?                 "¿B<<28," :
			(flags & 0xf0000000) == 0xc0000000         ?                 "¿C<<28," :
			(flags & 0xf0000000) == 0xd0000000         ?                 "¿D<<28," :
			(flags & 0xf0000000) == 0xe0000000         ?                 "¿E<<28," :
			(flags & 0xf0000000) == 0xf0000000         ?                 "¿F<<28," : ""
		);
	}
	return flagsstr;
} // GetOneFlagsStr
#endif

char appleTimingIDStr[20];

const char *GetOneAppleTimingID(IOAppleTimingID appleTimingID) {
	return
	appleTimingID == kIOTimingIDInvalid               ?             "Invalid=0" :
	appleTimingID == kIOTimingIDApple_FixedRateLCD    ?    "Apple_FixedRateLCD" :
	appleTimingID == kIOTimingIDApple_512x384_60hz    ?    "Apple_512x384_60hz" :
	appleTimingID == kIOTimingIDApple_560x384_60hz    ?    "Apple_560x384_60hz" :
	appleTimingID == kIOTimingIDApple_640x480_67hz    ?    "Apple_640x480_67hz" :
	appleTimingID == kIOTimingIDApple_640x400_67hz    ?    "Apple_640x400_67hz" :
	appleTimingID == kIOTimingIDVESA_640x480_60hz     ?     "VESA_640x480_60hz" :
	appleTimingID == kIOTimingIDVESA_640x480_72hz     ?     "VESA_640x480_72hz" :
	appleTimingID == kIOTimingIDVESA_640x480_75hz     ?     "VESA_640x480_75hz" :
	appleTimingID == kIOTimingIDVESA_640x480_85hz     ?     "VESA_640x480_85hz" :
	appleTimingID == kIOTimingIDGTF_640x480_120hz     ?     "GTF_640x480_120hz" :
	appleTimingID == kIOTimingIDApple_640x870_75hz    ?    "Apple_640x870_75hz" :
	appleTimingID == kIOTimingIDApple_640x818_75hz    ?    "Apple_640x818_75hz" :
	appleTimingID == kIOTimingIDApple_832x624_75hz    ?    "Apple_832x624_75hz" :
	appleTimingID == kIOTimingIDVESA_800x600_56hz     ?     "VESA_800x600_56hz" :
	appleTimingID == kIOTimingIDVESA_800x600_60hz     ?     "VESA_800x600_60hz" :
	appleTimingID == kIOTimingIDVESA_800x600_72hz     ?     "VESA_800x600_72hz" :
	appleTimingID == kIOTimingIDVESA_800x600_75hz     ?     "VESA_800x600_75hz" :
	appleTimingID == kIOTimingIDVESA_800x600_85hz     ?     "VESA_800x600_85hz" :
	appleTimingID == kIOTimingIDVESA_1024x768_60hz    ?    "VESA_1024x768_60hz" :
	appleTimingID == kIOTimingIDVESA_1024x768_70hz    ?    "VESA_1024x768_70hz" :
	appleTimingID == kIOTimingIDVESA_1024x768_75hz    ?    "VESA_1024x768_75hz" :
	appleTimingID == kIOTimingIDVESA_1024x768_85hz    ?    "VESA_1024x768_85hz" :
	appleTimingID == kIOTimingIDApple_1024x768_75hz   ?   "Apple_1024x768_75hz" :
	appleTimingID == kIOTimingIDVESA_1152x864_75hz    ?    "VESA_1152x864_75hz" :
	appleTimingID == kIOTimingIDApple_1152x870_75hz   ?   "Apple_1152x870_75hz" :
	appleTimingID == kIOTimingIDAppleNTSC_ST          ?          "AppleNTSC_ST" :
	appleTimingID == kIOTimingIDAppleNTSC_FF          ?          "AppleNTSC_FF" :
	appleTimingID == kIOTimingIDAppleNTSC_STconv      ?      "AppleNTSC_STconv" :
	appleTimingID == kIOTimingIDAppleNTSC_FFconv      ?      "AppleNTSC_FFconv" :
	appleTimingID == kIOTimingIDApplePAL_ST           ?           "ApplePAL_ST" :
	appleTimingID == kIOTimingIDApplePAL_FF           ?           "ApplePAL_FF" :
	appleTimingID == kIOTimingIDApplePAL_STconv       ?       "ApplePAL_STconv" :
	appleTimingID == kIOTimingIDApplePAL_FFconv       ?       "ApplePAL_FFconv" :
	appleTimingID == kIOTimingIDVESA_1280x960_75hz    ?    "VESA_1280x960_75hz" :
	appleTimingID == kIOTimingIDVESA_1280x960_60hz    ?    "VESA_1280x960_60hz" :
	appleTimingID == kIOTimingIDVESA_1280x960_85hz    ?    "VESA_1280x960_85hz" :
	appleTimingID == kIOTimingIDVESA_1280x1024_60hz   ?   "VESA_1280x1024_60hz" :
	appleTimingID == kIOTimingIDVESA_1280x1024_75hz   ?   "VESA_1280x1024_75hz" :
	appleTimingID == kIOTimingIDVESA_1280x1024_85hz   ?   "VESA_1280x1024_85hz" :
	appleTimingID == kIOTimingIDVESA_1600x1200_60hz   ?   "VESA_1600x1200_60hz" :
	appleTimingID == kIOTimingIDVESA_1600x1200_65hz   ?   "VESA_1600x1200_65hz" :
	appleTimingID == kIOTimingIDVESA_1600x1200_70hz   ?   "VESA_1600x1200_70hz" :
	appleTimingID == kIOTimingIDVESA_1600x1200_75hz   ?   "VESA_1600x1200_75hz" :
	appleTimingID == kIOTimingIDVESA_1600x1200_80hz   ?   "VESA_1600x1200_80hz" :
	appleTimingID == kIOTimingIDVESA_1600x1200_85hz   ?   "VESA_1600x1200_85hz" :
	appleTimingID == kIOTimingIDVESA_1792x1344_60hz   ?   "VESA_1792x1344_60hz" :
	appleTimingID == kIOTimingIDVESA_1792x1344_75hz   ?   "VESA_1792x1344_75hz" :
	appleTimingID == kIOTimingIDVESA_1856x1392_60hz   ?   "VESA_1856x1392_60hz" :
	appleTimingID == kIOTimingIDVESA_1856x1392_75hz   ?   "VESA_1856x1392_75hz" :
	appleTimingID == kIOTimingIDVESA_1920x1440_60hz   ?   "VESA_1920x1440_60hz" :
	appleTimingID == kIOTimingIDVESA_1920x1440_75hz   ?   "VESA_1920x1440_75hz" :
	appleTimingID == kIOTimingIDSMPTE240M_60hz        ?        "SMPTE240M_60hz" :
	appleTimingID == kIOTimingIDFilmRate_48hz         ?         "FilmRate_48hz" :
	appleTimingID == kIOTimingIDSony_1600x1024_76hz   ?   "Sony_1600x1024_76hz" :
	appleTimingID == kIOTimingIDSony_1920x1080_60hz   ?   "Sony_1920x1080_60hz" :
	appleTimingID == kIOTimingIDSony_1920x1080_72hz   ?   "Sony_1920x1080_72hz" :
	appleTimingID == kIOTimingIDSony_1920x1200_76hz   ?   "Sony_1920x1200_76hz" :
	appleTimingID == kIOTimingIDApple_0x0_0hz_Offline ? "Apple_0x0_0hz_Offline" :
	appleTimingID == kIOTimingIDVESA_848x480_60hz     ?     "VESA_848x480_60hz" :
	appleTimingID == kIOTimingIDVESA_1360x768_60hz    ?    "VESA_1360x768_60hz" :
	UNKNOWN_VALUE(appleTimingIDStr, sizeof(appleTimingIDStr), appleTimingID);
}
#define abs(x) ((x) < 0 ? -(x) : (x))
#define FLOAT(_name, _value) SInt64 _name ## 1000 = 1000.0 * (_value); SInt64 _name ## whole = _name ## 1000 / 1000; UInt64 _name ## frac = abs(_name ## 1000) % 1000;
#define PRIF "%lld.%03lld"
#define CASTF(_name) _name ## whole, _name ## frac
#define PRIG "%lld.%0.*lld"
#define CASTG(_name) _name ## whole, _name ## frac % 10 ? 3 : _name ## frac % 100 ? 2 : 1, _name ## frac % 10 ? _name ## frac : _name ## frac % 100 ? _name ## frac / 10 : _name ## frac / 100

// "detailed"
char * DumpOneDetailedTimingInformationPtr(char * buf, size_t bufSize, const void * IOFBDetailedTiming, size_t timingSize) {
	int inc;
	char * result = buf;

	char horizontalSyncConfig[20];
	char horizontalSyncLevel[20];
	char verticalSyncConfig[20];
	char verticalSyncLevel[20];
	char scalerFlags[20];
	char signalConfig[20];
	char signalLevels[20];
	char pixelEncoding[20];
	char bitsPerColorComponent[20];
	char colorimetry[20];
	char dynamicRange[20];
	char __reservedA1[20];
	char __reservedA2[20];
	char __reservedB0[20];
	char __reservedB1[20];
	char __reservedB2[20];

	switch (timingSize) {
		case sizeof(IODetailedTimingInformationV1): inc = bprintf(buf, bufSize, "V1"); break;
		case sizeof(IODetailedTimingInformationV2): inc = bprintf(buf, bufSize, "V2"); break;
		default                                   : inc = bprintf(buf, bufSize, "Unexpected size:%ld", (long)timingSize); break;
	}
	buf += inc; bufSize -= inc;

	if (timingSize >= sizeof(IODetailedTimingInformationV2)) {
		IODetailedTimingInformationV2 *timing = (IODetailedTimingInformationV2 *)IOFBDetailedTiming;

		FLOAT(Hz, timing->pixelClock * 1.0 / ((timing->horizontalActive + timing->horizontalBlanking) * (timing->verticalActive + timing->verticalBlanking)))
		FLOAT(kHz, timing->pixelClock * 1.0 / ((timing->horizontalActive + timing->horizontalBlanking) * 1000.0))
		FLOAT(MHz, timing->pixelClock / 1000000.0)
		FLOAT(errMin, ((SInt64)timing->minPixelClock - (SInt64)timing->pixelClock) / 1000000.0)
		FLOAT(errMax, ((SInt64)timing->maxPixelClock - (SInt64)timing->pixelClock) / 1000000.0)
		FLOAT(dscBPP, timing->dscCompressedBitsPerPixel / 16.0)

		inc = bprintf(buf, bufSize, " id:0x%08x %dx%d@" PRIF "Hz " PRIF "kHz " PRIF "MHz (errMHz " PRIG "," PRIG ")  h(%d %d %d %s%s)  v(%d %d %d %s%s)  border(h%d:%d v%d:%d)  active:%dx%d %s inset:%dx%d flags(%s%s%s%s%s%s%s%s%s%s%s%s%s%s) signal(%s%s%s%s%s%s%s%s) levels:%s links:%d",
			timing->detailedTimingModeID, // mode

			timing->horizontalScaled ? timing->horizontalScaled : timing->horizontalActive,
			timing->verticalScaled ? timing->verticalScaled : timing->verticalActive,

			CASTF(Hz),
			CASTF(kHz),
			CASTF(MHz),

			CASTG(errMin), // Hz - With error what is slowest actual clock
			CASTG(errMax), // Hz - With error what is fasted actual clock

			timing->horizontalSyncOffset,           // pixels
			timing->horizontalSyncPulseWidth,       // pixels
			timing->horizontalBlanking - timing->horizontalSyncOffset - timing->horizontalSyncPulseWidth, // pixels

			timing->horizontalSyncConfig == kIOSyncPositivePolarity ? "+" :
			timing->horizontalSyncConfig == 0 ? "-" :
			UNKNOWN_VALUE(horizontalSyncConfig, sizeof(horizontalSyncConfig), timing->horizontalSyncConfig),

			timing->horizontalSyncLevel == 0 ? "" :
			UNKNOWN_VALUE(horizontalSyncLevel, sizeof(horizontalSyncLevel), timing->horizontalSyncLevel), // Future use (init to 0)

			timing->verticalSyncOffset,             // lines
			timing->verticalSyncPulseWidth,         // lines
			timing->verticalBlanking - timing->verticalSyncOffset - timing->verticalSyncPulseWidth, // lines

			timing->verticalSyncConfig == kIOSyncPositivePolarity ? "+" :
			timing->verticalSyncConfig == 0 ? "-" :
			UNKNOWN_VALUE(verticalSyncConfig, sizeof(verticalSyncConfig), timing->verticalSyncConfig),

			timing->verticalSyncLevel == 0 ? "" :
			UNKNOWN_VALUE(verticalSyncLevel, sizeof(verticalSyncLevel), timing->verticalSyncLevel), // Future use (init to 0)

			timing->horizontalBorderLeft,           // pixels
			timing->horizontalBorderRight,          // pixels
			timing->verticalBorderTop,              // lines
			timing->verticalBorderBottom,           // lines

			timing->horizontalActive,               // pixels
			timing->verticalActive,                 // lines

			(timing->horizontalScaled && timing->verticalScaled) ? "(scaled)" :
			((!timing->horizontalScaled) != (!timing->verticalScaled)) ? "(scaled?)" : "(not scaled)",

			timing->horizontalScaledInset,          // pixels
			timing->verticalScaledInset,            // lines

			(timing->scalerFlags & kIOScaleStretchToFit) ? "fit," : "",
			(timing->scalerFlags & 2) ? "2?," : "",
			(timing->scalerFlags & 4) ? "4?," : "",
			(timing->scalerFlags & 8) ? "8?," : "",
			((timing->scalerFlags & 0x70) == kIOScaleSwapAxes ) ?    "swap," : "", // 1
			((timing->scalerFlags & 0x70) == kIOScaleInvertX  ) ? "invertx," : "", // 2
			((timing->scalerFlags & 0x70) == kIOScaleInvertY  ) ? "inverty," : "", // 4
			((timing->scalerFlags & 0x70) == kIOScaleRotate0  ) ?      "0°," : "", // 0
			((timing->scalerFlags & 0x70) == kIOScaleRotate90 ) ?     "90°," : "", // 3
			((timing->scalerFlags & 0x70) == kIOScaleRotate180) ?    "180°," : "", // 6
			((timing->scalerFlags & 0x70) == kIOScaleRotate270) ?    "270°," : "", // 5
			((timing->scalerFlags & 0x70) == 0x70             ) ? "swap,invertx,inverty," : "",
			((timing->scalerFlags & 0x80)                     ) ?   "0x80?," : "",
			UNKNOWN_FLAG(scalerFlags, sizeof(scalerFlags), timing->scalerFlags & 0xffffff00),

			(timing->signalConfig & kIODigitalSignal      ) ?               "digital," : "",
			(timing->signalConfig & kIOAnalogSetupExpected) ? "analog setup expected," : "",
			(timing->signalConfig & kIOInterlacedCEATiming) ?        "interlaced CEA," : "",
			(timing->signalConfig & kIONTSCTiming         ) ?                  "NTSC," : "",
			(timing->signalConfig & kIOPALTiming          ) ?                   "PAL," : "",
			(timing->signalConfig & kIODSCBlockPredEnable ) ? "DSC block pred enable," : "",
			(timing->signalConfig & kIOMultiAlignedTiming ) ?         "multi aligned," : "",
			UNKNOWN_FLAG(signalConfig, sizeof(signalConfig), timing->signalConfig & 0xffffff80),

			(timing->signalLevels == kIOAnalogSignalLevel_0700_0300) ? "0700_0300" :
			(timing->signalLevels == kIOAnalogSignalLevel_0714_0286) ? "0714_0286" :
			(timing->signalLevels == kIOAnalogSignalLevel_1000_0400) ? "1000_0400" :
			(timing->signalLevels == kIOAnalogSignalLevel_0700_0000) ? "0700_0000" :
			UNKNOWN_VALUE(signalLevels, sizeof(signalLevels), timing->signalLevels),

			timing->numLinks
		);
		buf += inc; bufSize -= inc;

		if ( 0
			|| timing->verticalBlankingExtension
			|| timing->pixelEncoding
			|| timing->bitsPerColorComponent
			|| timing->colorimetry
			|| timing->dynamicRange
			|| timing->dscCompressedBitsPerPixel
			|| timing->dscSliceHeight
			|| timing->dscSliceWidth
			|| timing->verticalBlankingMaxStretchPerFrame
			|| timing->verticalBlankingMaxShrinkPerFrame
		)
		inc = bprintf(buf, bufSize, " vbext:%d vbstretch:%d vbshrink:%d encodings(%s%s%s%s%s) bpc(%s%s%s%s%s%s) colorimetry(%s%s%s%s%s%s%s%s%s%s%s) dynamicrange(%s%s%s%s%s%s%s) dsc(%dx%d " PRIG "bpp)",
			timing->verticalBlankingExtension,      // lines (AdaptiveSync: 0 for non-AdaptiveSync support)
			timing->verticalBlankingMaxStretchPerFrame,
			timing->verticalBlankingMaxShrinkPerFrame,

			(timing->pixelEncoding & kIOPixelEncodingRGB444  ) ? "RGB," : "",
			(timing->pixelEncoding & kIOPixelEncodingYCbCr444) ? "444," : "",
			(timing->pixelEncoding & kIOPixelEncodingYCbCr422) ? "422," : "",
			(timing->pixelEncoding & kIOPixelEncodingYCbCr420) ? "420," : "",
			UNKNOWN_FLAG(pixelEncoding, sizeof(pixelEncoding), timing->pixelEncoding & 0xfff0),

			(timing->bitsPerColorComponent & kIOBitsPerColorComponent6 ) ?  "6," : "",
			(timing->bitsPerColorComponent & kIOBitsPerColorComponent8 ) ?  "8," : "",
			(timing->bitsPerColorComponent & kIOBitsPerColorComponent10) ? "10," : "",
			(timing->bitsPerColorComponent & kIOBitsPerColorComponent12) ? "12," : "",
			(timing->bitsPerColorComponent & kIOBitsPerColorComponent16) ? "16," : "",
			UNKNOWN_FLAG(bitsPerColorComponent, sizeof(bitsPerColorComponent), timing->bitsPerColorComponent & 0xffe0),

			(timing->colorimetry & kIOColorimetryNativeRGB) ? "NativeRGB," : "",
			(timing->colorimetry & kIOColorimetrysRGB     ) ?      "sRGB," : "",
			(timing->colorimetry & kIOColorimetryDCIP3    ) ?     "DCIP3," : "",
			(timing->colorimetry & kIOColorimetryAdobeRGB ) ?  "AdobeRGB," : "",
			(timing->colorimetry & kIOColorimetryxvYCC    ) ?     "xvYCC," : "",
			(timing->colorimetry & kIOColorimetryWGRGB    ) ?     "WGRGB," : "",
			(timing->colorimetry & kIOColorimetryBT601    ) ?     "BT601," : "",
			(timing->colorimetry & kIOColorimetryBT709    ) ?     "BT709," : "",
			(timing->colorimetry & kIOColorimetryBT2020   ) ?    "BT2020," : "",
			(timing->colorimetry & kIOColorimetryBT2100   ) ?    "BT2100," : "",
			UNKNOWN_FLAG(colorimetry, sizeof(colorimetry), timing->colorimetry & 0xfc00),

			(timing->dynamicRange & kIODynamicRangeSDR                ) ?                 "SDR," : "",
			(timing->dynamicRange & kIODynamicRangeHDR10              ) ?               "HDR10," : "",
			(timing->dynamicRange & kIODynamicRangeDolbyNormalMode    ) ?     "DolbyNormalMode," : "",
			(timing->dynamicRange & kIODynamicRangeDolbyTunnelMode    ) ?     "DolbyTunnelMode," : "",
			(timing->dynamicRange & kIODynamicRangeTraditionalGammaHDR) ? "TraditionalGammaHDR," : "",
			(timing->dynamicRange & kIODynamicRangeTraditionalGammaSDR) ? "TraditionalGammaSDR," : "",
			UNKNOWN_FLAG(dynamicRange, sizeof(dynamicRange), timing->dynamicRange & 0xffc0),

			timing->dscSliceWidth,
			timing->dscSliceHeight,

			CASTG(dscBPP)
		);
		buf += inc; bufSize -= inc;

		inc = bprintf(buf, bufSize, "%s%s%s%s%s%s%s%s%s%s",
			timing->__reservedA[1] ? " reservedA1:" : "",
			timing->__reservedA[1] ? UNKNOWN_VALUE(__reservedA1, sizeof(__reservedA1), timing->__reservedA[1]) : "",
			timing->__reservedA[2] ? " reservedA2:" : "",
			timing->__reservedA[2] ? UNKNOWN_VALUE(__reservedA2, sizeof(__reservedA2), timing->__reservedA[2]) : "",
			timing->__reservedB[0] ? " reservedB0:" : "",
			timing->__reservedB[0] ? UNKNOWN_VALUE(__reservedB0, sizeof(__reservedB0), timing->__reservedB[0]) : "",
			timing->__reservedB[1] ? " reservedB1:" : "",
			timing->__reservedB[1] ? UNKNOWN_VALUE(__reservedB1, sizeof(__reservedB1), timing->__reservedB[1]) : "",
			timing->__reservedB[2] ? " reservedB2:" : "",
			timing->__reservedB[2] ? UNKNOWN_VALUE(__reservedB2, sizeof(__reservedB2), timing->__reservedB[2]) : ""
		);
		buf += inc; bufSize -= inc;

	} else if (timingSize >= sizeof(IODetailedTimingInformationV1)) {
		IODetailedTimingInformationV1 *timing = (IODetailedTimingInformationV1 *)IOFBDetailedTiming;

		FLOAT(Hz, timing->pixelClock * 1.0 / ((timing->horizontalActive + timing->horizontalBlanking) * (timing->verticalActive + timing->verticalBlanking)))
		FLOAT(kHz, timing->pixelClock * 1.0 / ((timing->horizontalActive + timing->horizontalBlanking) * 1000.0))
		FLOAT(MHz, timing->pixelClock / 1000000.0)

		inc = bprintf(buf, bufSize, " %dx%d@" PRIF "Hz " PRIF "kHz " PRIF "MHz  h(%d %d %d)  v(%d %d %d)  border(h%d v%d)",
			timing->horizontalActive,               // pixels
			timing->verticalActive,                 // lines
			CASTF(Hz), // Hz
			CASTF(kHz), // kHz
			CASTF(MHz),

			timing->horizontalSyncOffset,           // pixels
			timing->horizontalSyncWidth,            // pixels
			timing->horizontalBlanking - timing->horizontalSyncOffset - timing->horizontalSyncWidth, // pixels

			timing->verticalSyncOffset,             // lines
			timing->verticalSyncWidth,              // lines
			timing->verticalBlanking - timing->verticalSyncOffset - timing->verticalSyncWidth, // lines

			timing->horizontalBorder,       // pixels
			timing->verticalBorder          // lines
		);
	}

	return result;
} // DumpOneDetailedTimingInformationPtr

#ifdef DEBUG
// "timing"
static char * DumpOneTimingInformationPtr(char * buf, size_t bufSize, IOTimingInformation * info) {
	int inc;
	char * result = buf;

	char flags[20];
	char timinginfo[1000];

	inc = bprintf(buf, bufSize, "appleTimingID:%s flags:%s%s%s",
		GetOneAppleTimingID(info->appleTimingID),
		info->flags & kIODetailedTimingValid ? "DetailedTimingValid," : "",
		info->flags & kIOScalingInfoValid    ?    "ScalingInfoValid," : "",
		UNKNOWN_FLAG(flags, sizeof(flags), info->flags & 0x3fffffff)
	);
	buf += inc; bufSize -= inc;

	if (info->flags & kIODetailedTimingValid) {
		inc = bprintf(buf, bufSize, " detailed:{ %s }",
			DumpOneDetailedTimingInformationPtr(timinginfo, sizeof(timinginfo), &info->detailedInfo, sizeof(info->detailedInfo.v2))
		);
	}

	return result;
} // DumpOneTimingInformationPtr

// "info"
static char * DumpOneDisplayModeInformationPtr(char * buf, size_t bufSize, IODisplayModeInformation * info) {
	int inc;
	char * result = buf;

	char reserved0[10];
	char reserved1[10];
	char reserved2[10];
	char flagsstr[200];

	FLOAT(refreshRate, info->refreshRate / 65536.0)

	inc = bprintf(buf, bufSize, "%dx%d@" PRIF "Hz maxdepth:%d flags:%s imagesize:%dx%dmm%s%s%s%s%s%s",
		info->nominalWidth,
		info->nominalHeight,
		CASTF(refreshRate),
		info->maxDepthIndex,
		GetOneFlagsStr(flagsstr, sizeof(flagsstr), info->flags),
		info->imageWidth,
		info->imageHeight,
		info->reserved[ 0 ] ? " reserved0:" : "",
		info->reserved[ 0 ] ? UNKNOWN_VALUE(reserved0, sizeof(reserved0), info->reserved[ 0 ]) : "",
		info->reserved[ 1 ] ? " reserved1:" : "",
		info->reserved[ 1 ] ? UNKNOWN_VALUE(reserved1, sizeof(reserved1), info->reserved[ 1 ]) : "",
		info->reserved[ 2 ] ? " reserved2:" : "",
		info->reserved[ 2 ] ? UNKNOWN_VALUE(reserved2, sizeof(reserved2), info->reserved[ 2 ]) : ""
	);
	return result;
} // DumpOneDisplayModeInformationPtr

// "desc"
static char * DumpOneFBDisplayModeDescriptionPtr(char * buf, size_t bufSize, IOFBDisplayModeDescription * desc) {
	int inc;
	char * result = buf;
	char temp[1000];

	inc = bprintf(buf, bufSize, "info:{ %s }",
		DumpOneDisplayModeInformationPtr(temp, sizeof(temp), &desc->info)
	);
	buf += inc; bufSize -= inc;
	inc = bprintf(buf, bufSize, " timing:{ %s }",
		DumpOneTimingInformationPtr(temp, sizeof(temp), &desc->timingInfo)
	);
	return result;
} // DumpOneFBDisplayModeDescriptionPtr
#endif

UInt32 IOFBAttribChars(UInt32 attribute) {
	UInt32 aswap = OSSwapBigToHostInt32(attribute);
	char* achar = (char*)&aswap;
	if (!achar[0]) achar[0] = '.';
	if (!achar[1]) achar[1] = '.';
	if (!achar[2]) achar[2] = '.';
	if (!achar[3]) achar[3] = '.';
	return aswap;
}

#define STRTOORD4(_s) ((_s[0]<<24) | (_s[1]<<16) | (_s[2]<<8) | (_s[3]))

const char * IOFBGetAttributeName(UInt32 attribute, bool forConnection)
{
	switch (attribute) {
		case kIOPowerStateAttribute           : return "kIOPowerStateAttribute";
		case kIOPowerAttribute                : return forConnection ? "kConnectionPower" : "kIOPowerAttribute";
		case kIODriverPowerAttribute          : return "kIODriverPowerAttribute";
		case kIOHardwareCursorAttribute       : return "kIOHardwareCursorAttribute";
		case kIOMirrorAttribute               : return "kIOMirrorAttribute";
		case kIOMirrorDefaultAttribute        : return "kIOMirrorDefaultAttribute";
		case kIOCapturedAttribute             : return "kIOCapturedAttribute";
		case kIOCursorControlAttribute        : return "kIOCursorControlAttribute";
		case kIOSystemPowerAttribute          : return "kIOSystemPowerAttribute";
		case kIOWindowServerActiveAttribute   : return "kIOWindowServerActiveAttribute";
		case kIOVRAMSaveAttribute             : return "kIOVRAMSaveAttribute";
		case kIODeferCLUTSetAttribute         : return "kIODeferCLUTSetAttribute";
		case kIOClamshellStateAttribute       : return "kIOClamshellStateAttribute";
		case kIOFBDisplayPortTrainingAttribute: return "kIOFBDisplayPortTrainingAttribute";
		case kIOFBDisplayState                : return "kIOFBDisplayState";
		case kIOFBVariableRefreshRate         : return "kIOFBVariableRefreshRate";
		case kIOFBLimitHDCPAttribute          : return "kIOFBLimitHDCPAttribute";
		case kIOFBLimitHDCPStateAttribute     : return "kIOFBLimitHDCPStateAttribute";
		case kIOFBStop                        : return "kIOFBStop";
		case kIOFBRedGammaScaleAttribute      : return "kIOFBRedGammaScaleAttribute";
		case kIOFBGreenGammaScaleAttribute    : return "kIOFBGreenGammaScaleAttribute";
		case kIOFBBlueGammaScaleAttribute     : return "kIOFBBlueGammaScaleAttribute";
		case kIOFBHDRMetaDataAttribute        : return "kIOFBHDRMetaDataAttribute";
		case kIOBuiltinPanelPowerAttribute    : return "kIOBuiltinPanelPowerAttribute";

		case kIOFBSpeedAttribute                 : return "kIOFBSpeedAttribute";
		case kIOFBWSStartAttribute               : return "kIOFBWSStartAttribute";
		case kIOFBProcessConnectChangeAttribute  : return "kIOFBProcessConnectChangeAttribute";
		case kIOFBEndConnectChangeAttribute      : return "kIOFBEndConnectChangeAttribute";
		case kIOFBMatchedConnectChangeAttribute  : return "kIOFBMatchedConnectChangeAttribute";
		// Connection attributes
		case kConnectionInTVMode                 : return "kConnectionInTVMode";
		case kConnectionWSSB                     : return "kConnectionWSSB";
		case kConnectionRawBacklight             : return "kConnectionRawBacklight";
		case kConnectionBacklightSave            : return "kConnectionBacklightSave";
		case kConnectionVendorTag                : return "kConnectionVendorTag";

		case kConnectionFlags                    : return "kConnectionFlags";

		case kConnectionSyncEnable               : return "kConnectionSyncEnable";
		case kConnectionSyncFlags                : return "kConnectionSyncFlags";

		case kConnectionSupportsAppleSense       : return "kConnectionSupportsAppleSense";
		case kConnectionSupportsLLDDCSense       : return "kConnectionSupportsLLDDCSense";
		case kConnectionSupportsHLDDCSense       : return "kConnectionSupportsHLDDCSense";
		case kConnectionEnable                   : return "kConnectionEnable";
		case kConnectionCheckEnable              : return "kConnectionCheckEnable";
		case kConnectionProbe                    : return "kConnectionProbe";
		case kConnectionIgnore                   : return "kConnectionIgnore";
		case kConnectionChanged                  : return "kConnectionChanged";
//		case kConnectionPower                    : return "kConnectionPower"; // kIOPowerAttribute
		case kConnectionPostWake                 : return "kConnectionPostWake";
		case kConnectionDisplayParameterCount    : return "kConnectionDisplayParameterCount";
		case kConnectionDisplayParameters        : return "kConnectionDisplayParameters";
		case kConnectionOverscan                 : return "kConnectionOverscan";
		case kConnectionVideoBest                : return "kConnectionVideoBest";

		case kConnectionRedGammaScale            : return "kConnectionRedGammaScale";
		case kConnectionGreenGammaScale          : return "kConnectionGreenGammaScale";
		case kConnectionBlueGammaScale           : return "kConnectionBlueGammaScale";
		case kConnectionGammaScale               : return "kConnectionGammaScale";
		case kConnectionFlushParameters          : return "kConnectionFlushParameters";

		case kConnectionVBLMultiplier            : return "kConnectionVBLMultiplier";

		case kConnectionHandleDisplayPortEvent   : return "kConnectionHandleDisplayPortEvent";

		case kConnectionPanelTimingDisable       : return "kConnectionPanelTimingDisable";

		case kConnectionColorMode                : return "kConnectionColorMode";
		case kConnectionColorModesSupported      : return "kConnectionColorModesSupported";
		case kConnectionColorDepthsSupported     : return "kConnectionColorDepthsSupported";

		case kConnectionControllerDepthsSupported: return "kConnectionControllerDepthsSupported";
		case kConnectionControllerColorDepth     : return "kConnectionControllerColorDepth";
		case kConnectionControllerDitherControl  : return "kConnectionControllerDitherControl";

		case kConnectionDisplayFlags             : return "kConnectionDisplayFlags";

		case kConnectionEnableAudio              : return "kConnectionEnableAudio";
		case kConnectionAudioStreaming           : return "kConnectionAudioStreaming";

		case kConnectionStartOfFrameTime         : return "kConnectionStartOfFrameTime";

		case kConnectionUnderscan                : return "kConnectionUnderscan";
		case kTempAttribute                      : return "kTempAttribute";
		case STRTOORD4(kIODisplaySelectedColorModeKey) : return "kIODisplaySelectedColorModeKey";

		default                               : return NULL;
	}
} // GetAttributeName

static char * DumpOneAttribute(char * buf, size_t bufSize, UInt32 attribute, bool forConnection)
{
	char * result = buf;
	UInt32 attributeChar = IOFBAttribChars(attribute);
	const char *attributeStr = IOFBGetAttributeName(attribute, forConnection);
	bprintf(buf, bufSize, "0x%x=\'%.4s\'%s%s", attribute, (char*)&attributeChar, attributeStr ? "=" : "", attributeStr ? attributeStr : "");
	return result;
}

#ifdef DEBUG
static char * DumpOneAttributeValue(char * buf, size_t bufSize, UInt32 attribute, bool set, bool forConnection, uintptr_t * valuePtr, unsigned int len)
{
	char * result = buf;
	char unknown[100];

	size_t inc;
	if (!valuePtr) {
		switch (attribute) {
			case kConnectionChanged:
			case kConnectionSupportsAppleSense:
			case kConnectionSupportsHLDDCSense:
			case kConnectionSupportsLLDDCSense:
				inc = bprintf(buf, bufSize, "n/a");
				break;
			default:
				inc = bprintf(buf, bufSize, "NULL");
				break;
		}
		buf += inc; bufSize -= inc;
	}
	else {
		UInt64 value = 0;
		inc = 0;
		switch (len) {
			case 1: value = *(UInt8 *)valuePtr; inc = bprintf(buf, bufSize, "0x%02llx:", value); break;
			case 2: value = *(UInt16*)valuePtr; inc = bprintf(buf, bufSize, "0x%04llx:", value); break;
			case 4: value = *(UInt32*)valuePtr; inc = bprintf(buf, bufSize, "0x%08llx:", value); break;
			case 8:
				if (attribute != kConnectionDisplayParameters) {
					value = *(UInt64*)valuePtr;
					if (attribute != kConnectionHandleDisplayPortEvent) { // don't waant to print a pointer
						int hexdigits =
							(value >> 32) ?
								(value >> 32) == 0x0ffffffff ?
									8
								:
									16
							:
								8;
						UInt64 printvalue = value;
						if (hexdigits == 8) printvalue &= 0x0ffffffff;
						inc = bprintf(buf, bufSize, "0x%0*llx:", hexdigits, printvalue);
					}
				}
				break;
		}
		buf += inc; bufSize -= inc;

		switch (attribute) {
			case kConnectionFlags:
				bprintf(buf, bufSize, "%s%s%s",
					value & kIOConnectionBuiltIn    ?    "BuiltIn," : "",
					value & kIOConnectionStereoSync ? "StereoSync," : "",
					UNKNOWN_FLAG(unknown, sizeof(unknown), value & 0xffff77ff)
				); break;

			case kConnectionSyncFlags:
				bprintf(buf, bufSize, "%s%s%s%s%s%s%s%s%s",
					value & kIOHSyncDisable          ?          "HSyncDisable," : "",
					value & kIOVSyncDisable          ?          "VSyncDisable," : "",
					value & kIOCSyncDisable          ?          "CSyncDisable," : "",
					value & kIONoSeparateSyncControl ? "NoSeparateSyncControl," : "",
					value & kIOTriStateSyncs         ?         "TriStateSyncs," : "",
					value & kIOSyncOnBlue            ?            "SyncOnBlue," : "",
					value & kIOSyncOnGreen           ?           "SyncOnGreen," : "",
					value & kIOSyncOnRed             ?             "SyncOnRed," : "",
					UNKNOWN_FLAG(unknown, sizeof(unknown), value & 0xffffff00)
				); break;

			case kConnectionHandleDisplayPortEvent:
				if (set) {
					valuePtr = (uintptr_t*)value;
					if (valuePtr) {
						value = *valuePtr;
						inc = bprintf(buf, bufSize, "%lld:", value);
						buf += inc; bufSize -= inc;
					}
					bprintf(buf, bufSize, "%s",
						valuePtr == NULL ? "NULL" :
						*valuePtr == kIODPEventStart                       ?                       "Start" :
						*valuePtr == kIODPEventIdle                        ?                        "Idle" :
						*valuePtr == kIODPEventForceRetrain                ?                "ForceRetrain" :
						*valuePtr == kIODPEventRemoteControlCommandPending ? "RemoteControlCommandPending" :
						*valuePtr == kIODPEventAutomatedTestRequest        ?        "AutomatedTestRequest" :
						*valuePtr == kIODPEventContentProtection           ?           "ContentProtection" :
						*valuePtr == kIODPEventMCCS                        ?                        "MCCS" :
						*valuePtr == kIODPEventSinkSpecific                ?                "SinkSpecific" :
						UNKNOWN_VALUE(unknown, sizeof(unknown), *valuePtr)
					);
				}
				else {
				}
				break;

			case kConnectionColorMode:
			case kConnectionColorModesSupported:
				bprintf(buf, bufSize, "%s%s%s%s%s%s%s",
					value == kIODisplayColorModeReserved  ?    "Reserved" : "",
					value & kIODisplayColorModeRGB        ?        "RGB," : "",
					value & kIODisplayColorModeYCbCr422   ?        "422," : "",
					value & kIODisplayColorModeYCbCr444   ?        "444," : "",
					value & kIODisplayColorModeRGBLimited ? "RGBLimited," : "",
					value & kIODisplayColorModeAuto       ?       "Auto," : "",
					UNKNOWN_FLAG(unknown, sizeof(unknown), value & 0xefffeeee)
				); break;

			case kConnectionColorDepthsSupported:
			case kConnectionControllerDepthsSupported:
			case kConnectionControllerColorDepth:
				bprintf(buf, bufSize, "%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s",
					value == kIODisplayRGBColorComponentBitsUnknown ? "Unknown" : "",
					value & kIODisplayRGBColorComponentBits6        ?  "RGB 6," : "",
					value & kIODisplayRGBColorComponentBits8        ?  "RGB 8," : "",
					value & kIODisplayRGBColorComponentBits10       ? "RGB 10," : "",
					value & kIODisplayRGBColorComponentBits12       ? "RGB 12," : "",
					value & kIODisplayRGBColorComponentBits14       ? "RGB 14," : "",
					value & kIODisplayRGBColorComponentBits16       ? "RGB 16," : "",
					value & kIODisplayYCbCr444ColorComponentBits6   ?  "444 6," : "",
					value & kIODisplayYCbCr444ColorComponentBits8   ?  "444 8," : "",
					value & kIODisplayYCbCr444ColorComponentBits10  ? "444 10," : "",
					value & kIODisplayYCbCr444ColorComponentBits12  ? "444 12," : "",
					value & kIODisplayYCbCr444ColorComponentBits14  ? "444 14," : "",
					value & kIODisplayYCbCr444ColorComponentBits16  ? "444 16," : "",
					value & kIODisplayYCbCr422ColorComponentBits6   ?  "422 6," : "",
					value & kIODisplayYCbCr422ColorComponentBits8   ?  "422 8," : "",
					value & kIODisplayYCbCr422ColorComponentBits10  ? "422 10," : "",
					value & kIODisplayYCbCr422ColorComponentBits12  ? "422 12," : "",
					value & kIODisplayYCbCr422ColorComponentBits14  ? "422 14," : "",
					value & kIODisplayYCbCr422ColorComponentBits16  ? "422 16," : "",
					UNKNOWN_FLAG(unknown, sizeof(unknown), value & 0xffc00000)
				); break;

			case kConnectionControllerDitherControl:
				bprintf(buf, bufSize, "%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s",
					((value >> kIODisplayDitherRGBShift     ) & 0xff) == kIODisplayDitherDisable         ?          "RGB Disabled," : "",
					((value >> kIODisplayDitherRGBShift     ) & 0xff) == kIODisplayDitherAll             ?               "RGB All," : "",
					((value >> kIODisplayDitherRGBShift     ) & 0xff) & kIODisplayDitherSpatial          ?           "RGB Spatial," : "",
					((value >> kIODisplayDitherRGBShift     ) & 0xff) & kIODisplayDitherTemporal         ?          "RGB Temporal," : "",
					((value >> kIODisplayDitherRGBShift     ) & 0xff) & kIODisplayDitherFrameRateControl ?  "RGB FrameRateControl," : "",
					((value >> kIODisplayDitherRGBShift     ) & 0xff) & kIODisplayDitherDefault          ?           "RGB Default," : "",
					((value >> kIODisplayDitherRGBShift     ) & 0xff) & 0x70                             ?                 "RGB ?," : "",
					((value >> kIODisplayDitherYCbCr444Shift) & 0xff) == kIODisplayDitherDisable         ?          "444 Disabled," : "",
					((value >> kIODisplayDitherYCbCr444Shift) & 0xff) == kIODisplayDitherAll             ?               "444 All," : "",
					((value >> kIODisplayDitherYCbCr444Shift) & 0xff) & kIODisplayDitherSpatial          ?           "444 Spatial," : "",
					((value >> kIODisplayDitherYCbCr444Shift) & 0xff) & kIODisplayDitherTemporal         ?          "444 Temporal," : "",
					((value >> kIODisplayDitherYCbCr444Shift) & 0xff) & kIODisplayDitherFrameRateControl ?  "444 FrameRateControl," : "",
					((value >> kIODisplayDitherYCbCr444Shift) & 0xff) & kIODisplayDitherDefault          ?           "444 Default," : "",
					((value >> kIODisplayDitherYCbCr444Shift) & 0xff) & 0x70                             ?                 "444 ?," : "",
					((value >> kIODisplayDitherYCbCr422Shift) & 0xff) == kIODisplayDitherDisable         ?          "422 Disabled," : "",
					((value >> kIODisplayDitherYCbCr422Shift) & 0xff) == kIODisplayDitherAll             ?               "422 All," : "",
					((value >> kIODisplayDitherYCbCr422Shift) & 0xff) & kIODisplayDitherSpatial          ?           "422 Spatial," : "",
					((value >> kIODisplayDitherYCbCr422Shift) & 0xff) & kIODisplayDitherTemporal         ?          "422 Temporal," : "",
					((value >> kIODisplayDitherYCbCr422Shift) & 0xff) & kIODisplayDitherFrameRateControl ?  "422 FrameRateControl," : "",
					((value >> kIODisplayDitherYCbCr422Shift) & 0xff) & kIODisplayDitherDefault          ?           "422 Default," : "",
					((value >> kIODisplayDitherYCbCr422Shift) & 0xff) & 0x70                             ?                 "422 ?," : "",
					UNKNOWN_FLAG(unknown, sizeof(unknown), value & 0xff000000)
				); break;

			case kConnectionDisplayFlags:
				bprintf(buf, bufSize, "%s%s",
					value == kIODisplayNeedsCEAUnderscan ? "NeedsCEAUnderscan," : "",
						UNKNOWN_FLAG(unknown, sizeof(unknown), value & 0xfffffffe)
				); break;

			case kIOMirrorAttribute:
				bprintf(buf, bufSize, "%s%s%s%s",
					value & kIOMirrorIsPrimary  ?  "kIOMirrorIsPrimary," : "",
					value & kIOMirrorHWClipped  ?  "kIOMirrorHWClipped," : "",
					value & kIOMirrorIsMirrored ? "kIOMirrorIsMirrored," : "",
					UNKNOWN_FLAG(unknown, sizeof(unknown), value & 0x1fffffff)
				); break;

			case kIOMirrorDefaultAttribute:
				bprintf(buf, bufSize, "%s%s%s%s%s%s%s",
					value & kIOMirrorDefault        ?        "kIOMirrorDefault," : "",
					value & kIOMirrorForced         ?         "kIOMirrorForced," : "",
					value & kIOGPlatformYCbCr       ?       "kIOGPlatformYCbCr," : "",
					value & kIOFBDesktopModeAllowed ? "kIOFBDesktopModeAllowed," : "",
					value & kIOMirrorNoAutoHDMI     ?     "kIOMirrorNoAutoHDMI," : "",
					value & kIOMirrorHint           ?           "kIOMirrorHint," : "",
					UNKNOWN_FLAG(unknown, sizeof(unknown), value & 0xfffeffe0)
				); break;

			case kConnectionDisplayParameterCount:
			case kTempAttribute:
			case kIOFBSpeedAttribute:
			case kIOPowerStateAttribute:
			case kConnectionOverscan:
			case kConnectionUnderscan:
			case STRTOORD4(kIODisplaySelectedColorModeKey):
				bprintf(buf, bufSize, "%lld", value);
				break;

			case 'dith':
			case kIOHardwareCursorAttribute:
			case kIOClamshellStateAttribute:
			case kConnectionEnable:
			case kConnectionCheckEnable:
			case kIOVRAMSaveAttribute:
			case kIOCapturedAttribute:
			case kIOFBLimitHDCPAttribute:
			case kIOFBLimitHDCPStateAttribute:
			case kConnectionFlushParameters:
				bprintf(buf, bufSize, "%s", value == 1 ? "true" : value == 0 ? "false" : UNKNOWN_VALUE(unknown, sizeof(unknown), value));
				break;

			case kConnectionChanged:
			case kConnectionSupportsAppleSense:
			case kConnectionSupportsHLDDCSense:
			case kConnectionSupportsLLDDCSense:
				bprintf(buf, bufSize, "n/a");
				break;

			default:
				bprintf(buf, bufSize, "0x%llx", value);
		}
	}
	return result;
} // DumpOneAttributeValue
#endif

char * IOFB::DumpOneDisplayParameters(char * buf, size_t bufSize, uintptr_t * value, uintptr_t numParameters)
{
	char * result = buf;
	buf[0] = '\0';
	char attributeStr[100];
	for (int i = 0; i < numParameters; i++) {
		int inc = bprintf(buf, bufSize, "%s%s", i ? "," : "", DumpOneAttribute(attributeStr, sizeof(attributeStr), (UInt32)value[i], true));
		buf += inc; bufSize -= inc;
	}
	return result;
} // DumpOneDisplayParameters



static char * DumpOneCommFlags(char * buf, size_t bufSize, UInt32 commFlags)
{
	char * result = buf;
	bprintf(buf, bufSize, "%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s",
		(commFlags & 0x00000001                 ) ?            "0?," : "",
		(commFlags & kIOI2CUseSubAddressCommFlag) ? "UseSubAddress," : "",
		(commFlags & 0x00000004                 ) ?            "2?," : "",
		(commFlags & 0x00000008                 ) ?            "3?," : "",
		(commFlags & 0x00000010                 ) ?            "4?," : "",
		(commFlags & 0x00000020                 ) ?            "5?," : "",
		(commFlags & 0x00000040                 ) ?            "6?," : "",
		(commFlags & 0x00000080                 ) ?            "7?," : "",
		(commFlags & 0x00000100                 ) ?            "8?," : "",
		(commFlags & 0x00000200                 ) ?            "9?," : "",
		(commFlags & 0x00000400                 ) ?           "10?," : "",
		(commFlags & 0x00000800                 ) ?           "11?," : "",
		(commFlags & 0x00001000                 ) ?           "12?," : "",
		(commFlags & 0x00002000                 ) ?           "13?," : "",
		(commFlags & 0x00004000                 ) ?           "14?," : "",
		(commFlags & 0x00008000                 ) ?           "15?," : "",
		(commFlags & 0x00010000                 ) ?           "16?," : "",
		(commFlags & 0x00020000                 ) ?           "17?," : "",
		(commFlags & 0x00040000                 ) ?           "18?," : "",
		(commFlags & 0x00080000                 ) ?           "19?," : "",
		(commFlags & 0x00100000                 ) ?           "20?," : "",
		(commFlags & 0x00200000                 ) ?           "21?," : "",
		(commFlags & 0x00400000                 ) ?           "22?," : "",
		(commFlags & 0x00800000                 ) ?           "23?," : "",
		(commFlags & 0x01000000                 ) ?           "24?," : "",
		(commFlags & 0x02000000                 ) ?           "25?," : "",
		(commFlags & 0x04000000                 ) ?           "26?," : "",
		(commFlags & 0x08000000                 ) ?           "27?," : "",
		(commFlags & 0x10000000                 ) ?           "28?," : "",
		(commFlags & 0x20000000                 ) ?           "29?," : "",
		(commFlags & 0x40000000                 ) ?           "30?," : "",
		(commFlags & 0x80000000                 ) ?           "31?," : ""
	);
	return result;
} // DumpOneCommFlags

#ifdef DEBUG
static char * DumpOneInterruptType(char * buf, size_t bufSize, IOSelect interruptType)
{
	char * result = buf;
	char unknown[20];
	bprintf(buf, bufSize, "%s",
		interruptType == kIOFBVBLInterruptType                   ? "kIOFBVBLInterruptType" :
		interruptType == kIOFBHBLInterruptType                   ? "kIOFBHBLInterruptType" :
		interruptType == kIOFBFrameInterruptType                 ? "kIOFBFrameInterruptType" :
		interruptType == kIOFBConnectInterruptType               ? "kIOFBConnectInterruptType" :
		interruptType == kIOFBChangedInterruptType               ? "kIOFBChangedInterruptType" :
		interruptType == kIOFBOfflineInterruptType               ? "kIOFBOfflineInterruptType" :
		interruptType == kIOFBOnlineInterruptType                ? "kIOFBOnlineInterruptType" :
		interruptType == kIOFBDisplayPortInterruptType           ? "kIOFBDisplayPortInterruptType" :
		interruptType == kIOFBDisplayPortLinkChangeInterruptType ? "kIOFBDisplayPortLinkChangeInterruptType" :
		interruptType == kIOFBMCCSInterruptType                  ? "kIOFBMCCSInterruptType" :
		interruptType == kIOFBWakeInterruptType                  ? "kIOFBWakeInterruptType" :
		interruptType == kIOFBVBLInterruptType                   ? "kIOFBVBLInterruptType" :
		interruptType == kIOFBHBLInterruptType                   ? "kIOFBHBLInterruptType" :
		UNKNOWN_VALUE(unknown, sizeof(unknown), interruptType)
	);
	return result;
} // DumpOneInterruptType
#endif

char * DumpOneReturn(char * buf, size_t bufSize, IOReturn val)
{
	char * result = buf;
	char unknown[20];
	bprintf(buf, bufSize, "%s%s",
		val != kIOReturnSuccess ? " result:" : "",
		val == kIOReturnSuccess          ? "" :
		val == kIOReturnError            ? "kIOReturnError" :
		val == kIOReturnNoMemory         ? "kIOReturnNoMemory" :
		val == kIOReturnNoResources      ? "kIOReturnNoResources" :
		val == kIOReturnIPCError         ? "kIOReturnIPCError" :
		val == kIOReturnNoDevice         ? "kIOReturnNoDevice" :
		val == kIOReturnNotPrivileged    ? "kIOReturnNotPrivileged" :
		val == kIOReturnBadArgument      ? "kIOReturnBadArgument" :
		val == kIOReturnLockedRead       ? "kIOReturnLockedRead" :
		val == kIOReturnLockedWrite      ? "kIOReturnLockedWrite" :
		val == kIOReturnExclusiveAccess  ? "kIOReturnExclusiveAccess" :
		val == kIOReturnBadMessageID     ? "kIOReturnBadMessageID" :
		val == kIOReturnUnsupported      ? "kIOReturnUnsupported" :
		val == kIOReturnVMError          ? "kIOReturnVMError" :
		val == kIOReturnInternalError    ? "kIOReturnInternalError" :
		val == kIOReturnIOError          ? "kIOReturnIOError" :
		val == kIOReturnCannotLock       ? "kIOReturnCannotLock" :
		val == kIOReturnNotOpen          ? "kIOReturnNotOpen" :
		val == kIOReturnNotReadable      ? "kIOReturnNotReadable" :
		val == kIOReturnNotWritable      ? "kIOReturnNotWritable" :
		val == kIOReturnNotAligned       ? "kIOReturnNotAligned" :
		val == kIOReturnBadMedia         ? "kIOReturnBadMedia" :
		val == kIOReturnStillOpen        ? "kIOReturnStillOpen" :
		val == kIOReturnRLDError         ? "kIOReturnRLDError" :
		val == kIOReturnDMAError         ? "kIOReturnDMAError" :
		val == kIOReturnBusy             ? "kIOReturnBusy" :
		val == kIOReturnTimeout          ? "kIOReturnTimeout" :
		val == kIOReturnOffline          ? "kIOReturnOffline" :
		val == kIOReturnNotReady         ? "kIOReturnNotReady" :
		val == kIOReturnNotAttached      ? "kIOReturnNotAttached" :
		val == kIOReturnNoChannels       ? "kIOReturnNoChannels" :
		val == kIOReturnNoSpace          ? "kIOReturnNoSpace" :
		val == kIOReturnPortExists       ? "kIOReturnPortExists" :
		val == kIOReturnCannotWire       ? "kIOReturnCannotWire" :
		val == kIOReturnNoInterrupt      ? "kIOReturnNoInterrupt" :
		val == kIOReturnNoFrames         ? "kIOReturnNoFrames" :
		val == kIOReturnMessageTooLarge  ? "kIOReturnMessageTooLarge" :
		val == kIOReturnNotPermitted     ? "kIOReturnNotPermitted" :
		val == kIOReturnNoPower          ? "kIOReturnNoPower" :
		val == kIOReturnNoMedia          ? "kIOReturnNoMedia" :
		val == kIOReturnUnformattedMedia ? "kIOReturnUnformattedMedia" :
		val == kIOReturnUnsupportedMode  ? "kIOReturnUnsupportedMode" :
		val == kIOReturnUnderrun         ? "kIOReturnUnderrun" :
		val == kIOReturnOverrun          ? "kIOReturnOverrun" :
		val == kIOReturnDeviceError      ? "kIOReturnDeviceError" :
		val == kIOReturnNoCompletion     ? "kIOReturnNoCompletion" :
		val == kIOReturnAborted          ? "kIOReturnAborted" :
		val == kIOReturnNoBandwidth      ? "kIOReturnNoBandwidth" :
		val == kIOReturnNotResponding    ? "kIOReturnNotResponding" :
		val == kIOReturnIsoTooOld        ? "kIOReturnIsoTooOld" :
		val == kIOReturnIsoTooNew        ? "kIOReturnIsoTooNew" :
		val == kIOReturnNotFound         ? "kIOReturnNotFound" :
		val == kIOReturnInvalid          ? "kIOReturnInvalid" :
		val == nrLockedErr               ? "nrLockedErr" :
		val == nrNotEnoughMemoryErr      ? "nrNotEnoughMemoryErr" :
		val == nrInvalidNodeErr          ? "nrInvalidNodeErr" :
		val == nrNotFoundErr             ? "nrNotFoundErr" :
		val == nrNotCreatedErr           ? "nrNotCreatedErr" :
		val == nrNameErr                 ? "nrNameErr" :
		val == nrNotSlotDeviceErr        ? "nrNotSlotDeviceErr" :
		val == nrDataTruncatedErr        ? "nrDataTruncatedErr" :
		val == nrPowerErr                ? "nrPowerErr" :
		val == nrPowerSwitchAbortErr     ? "nrPowerSwitchAbortErr" :
		val == nrTypeMismatchErr         ? "nrTypeMismatchErr" :
		val == nrNotModifiedErr          ? "nrNotModifiedErr" :
		val == nrOverrunErr              ? "nrOverrunErr" :
		val == nrResultCodeBase          ? "nrResultCodeBase" :
		val == nrPathNotFound            ? "nrPathNotFound" :
		val == nrPathBufferTooSmall      ? "nrPathBufferTooSmall" :
		val == nrInvalidEntryIterationOp ? "nrInvalidEntryIterationOp" :
		val == nrPropertyAlreadyExists   ? "nrPropertyAlreadyExists" :
		val == nrIterationDone           ? "nrIterationDone" :
		val == nrExitedIteratorScope     ? "nrExitedIteratorScope" :
		val == nrTransactionAborted      ? "nrTransactionAborted" :
		val == gestaltUndefSelectorErr   ? "gestaltUndefSelectorErr" :
		val == paramErr                  ? "paramErr" :
		val == noHardwareErr             ? "noHardwareErr" :
		val == notEnoughHardwareErr      ? "notEnoughHardwareErr" :
		val == userCanceledErr           ? "userCanceledErr" :
		val == qErr                      ? "qErr" :
		val == vTypErr                   ? "vTypErr" :
		val == corErr                    ? "corErr" :
		val == unimpErr                  ? "unimpErr" :
		val == SlpTypeErr                ? "SlpTypeErr" :
		val == seNoDB                    ? "seNoDB" :
		val == controlErr                ? "controlErr" :
		val == statusErr                 ? "statusErr" :
		val == readErr                   ? "readErr" :
		val == writErr                   ? "writErr" :
		val == badUnitErr                ? "badUnitErr" :
		val == unitEmptyErr              ? "unitEmptyErr" :
		val == openErr                   ? "openErr" :
		val == closErr                   ? "closErr" :
		val == dRemovErr                 ? "dRemovErr" :
		val == dInstErr                  ? "dInstErr" :
		val == badCksmErr                ? "badCksmErr" :
		val == nrLockedErr               ? "nrLockedErr" :
		val == nrNotEnoughMemoryErr      ? "nrNotEnoughMemoryErr" :
		val == nrInvalidNodeErr          ? "nrInvalidNodeErr" :
		val == nrNotFoundErr             ? "nrNotFoundErr" :
		val == nrNotCreatedErr           ? "nrNotCreatedErr" :
		val == nrNameErr                 ? "nrNameErr" :
		val == nrNotSlotDeviceErr        ? "nrNotSlotDeviceErr" :
		val == nrDataTruncatedErr        ? "nrDataTruncatedErr" :
		val == nrPowerErr                ? "nrPowerErr" :
		UNKNOWN_VALUE(unknown, sizeof(unknown), val)
	);
	return result;
} // DumpOneReturn


#ifdef DEBUG
static char * DumpOnePrimarySense(char * buf, size_t bufSize, UInt32 val)
{
	char * result = buf;
	char unknown[20];
	bprintf(buf, bufSize, "%s",
		val == kRSCZero  ? "kRSCZero" :
		val == kRSCOne   ? "kRSCOne" :
		val == kRSCTwo   ? "kRSCTwo" :
		val == kRSCThree ? "kRSCThree" :
		val == kRSCFour  ? "kRSCFour" :
		val == kRSCFive  ? "kRSCFive" :
		val == kRSCSix   ? "kRSCSix" :
		val == kRSCSeven ? "kRSCSeven" :
		UNKNOWN_VALUE(unknown, sizeof(unknown), val)
	);
	return result;
} // DumpOnePrimarySense

static char * DumpOnePrimaryExtendedSense(char * buf, size_t bufSize, UInt32 val)
{
	char * result = buf;
	char unknown[20];
	bprintf(buf, bufSize, "%s",
		val == kESCZero21Inch            ? "kESCZero21Inch" :
		val == kESCOnePortraitMono       ? "kESCOnePortraitMono" :
		val == kESCTwo12Inch             ? "kESCTwo12Inch" :
		val == kESCThree21InchRadius     ? "kESCThree21InchRadius" :
		val == kESCThree21InchMonoRadius ? "kESCThree21InchMonoRadius" :
		val == kESCThree21InchMono       ? "kESCThree21InchMono" :
		val == kESCFourNTSC              ? "kESCFourNTSC" :
		val == kESCFivePortrait          ? "kESCFivePortrait" :
		val == kESCSixMSB1               ? "kESCSixMSB1" :
		val == kESCSixMSB2               ? "kESCSixMSB2" :
		val == kESCSixMSB3               ? "kESCSixMSB3" :
		val == kESCSixStandard           ? "kESCSixStandard" :
		val == kESCSevenPAL              ? "kESCSevenPAL" :
		val == kESCSevenNTSC             ? "kESCSevenNTSC" :
		val == kESCSevenVGA              ? "kESCSevenVGA" :
		val == kESCSeven16Inch           ? "kESCSeven16Inch" :
		val == kESCSevenPALAlternate     ? "kESCSevenPALAlternate" :
		val == kESCSeven19Inch           ? "kESCSeven19Inch" :
		val == kESCSevenDDC              ? "kESCSevenDDC" :
		val == kESCSevenNoDisplay        ? "kESCSevenNoDisplay" :
		UNKNOWN_VALUE(unknown, sizeof(unknown), val)
	);
	return result;
} // DumpOnePrimaryExtendedSense

static char * DumpOneDisplayType(char * buf, size_t bufSize, UInt32 val)
{
	char * result = buf;
	char unknown[20];
	bprintf(buf, bufSize, "%s",
		val == kUnknownConnect       ? "kUnknownConnect" :
		val == kPanelConnect         ? "kPanelConnect" :
		val == kPanelTFTConnect      ? "kPanelTFTConnect" :
		val == kFixedModeCRTConnect  ? "kFixedModeCRTConnect" :
		val == kMultiModeCRT1Connect ? "kMultiModeCRT1Connect" :
		val == kMultiModeCRT2Connect ? "kMultiModeCRT2Connect" :
		val == kMultiModeCRT3Connect ? "kMultiModeCRT3Connect" :
		val == kMultiModeCRT4Connect ? "kMultiModeCRT4Connect" :
		val == kModelessConnect      ? "kModelessConnect" :
		val == kFullPageConnect      ? "kFullPageConnect" :
		val == kVGAConnect           ? "kVGAConnect" :
		val == kNTSCConnect          ? "kNTSCConnect" :
		val == kPALConnect           ? "kPALConnect" :
		val == kHRConnect            ? "kHRConnect" :
		val == kPanelFSTNConnect     ? "kPanelFSTNConnect" :
		val == kMonoTwoPageConnect   ? "kMonoTwoPageConnect" :
		val == kColorTwoPageConnect  ? "kColorTwoPageConnect" :
		val == kColor16Connect       ? "kColor16Connect" :
		val == kColor19Connect       ? "kColor19Connect" :
		val == kGenericCRT           ? "kGenericCRT" :
		val == kGenericLCD           ? "kGenericLCD" :
		val == kDDCConnect           ? "kDDCConnect" :
		val == kNoConnect            ? "kNoConnect" :
		UNKNOWN_VALUE(unknown, sizeof(unknown), val)
	);
	return result;
} // DumpOneDisplayType
#endif

static char * DumpOneTransactionType(char * buf, size_t bufSize, UInt32 val)
{
	char * result = buf;
	char unknown[20];
	bprintf(buf, bufSize, "%s",
		val == kIOI2CNoTransactionType                ?                "No" :
		val == kIOI2CSimpleTransactionType            ?            "Simple" :
		val == kIOI2CDDCciReplyTransactionType        ?        "DDCciReply" :
		val == kIOI2CCombinedTransactionType          ?          "Combined" :
		val == kIOI2CDisplayPortNativeTransactionType ? "DisplayPortNative" :
		UNKNOWN_VALUE(unknown, sizeof(unknown), val)
	);
	return result;
} // DumpOneTransactionType

static char * DumpOneI2CReserved(char * buf, size_t bufSize, IOI2CRequest * request)
{
	char * result = buf;
	char __reservedA[10];
	char __reservedB[20];
	char __padA[10];
	char __padB[10];
#if !defined(__LP64__)
	char __padC[44];
#endif
	char __reservedC[90];

	if ( request &&
		(
			request->__reservedA[0] | request->__reservedA[1] |
			request->__padA |
			request->__reservedB[0] | request->__reservedB[1] |
			request->__padB |
			#if !defined(__LP64__)
			request->__padC |
			#endif
			request->__reservedC[0] | request->__reservedC[1] | request->__reservedC[2] | request->__reservedC[3] | request->__reservedC[4] |
			request->__reservedC[5] | request->__reservedC[6] | request->__reservedC[7] | request->__reservedC[8] | request->__reservedC[9]
		)
	) {
		bprintf(buf, bufSize, " reserved:%s%s%s%s%s%s",
			HEX(__reservedA, sizeof(__reservedA), &request->__reservedA, sizeof(request->__reservedA)),
			HEX(__padA,      sizeof(__padA)     , &request->__padA     , sizeof(request->__padA     )),
			HEX(__reservedB, sizeof(__reservedB), &request->__reservedB, sizeof(request->__reservedB)),
			HEX(__padB,      sizeof(__padB)     , &request->__padB     , sizeof(request->__padB     )),
			#if !defined(__LP64__)
			//	HEX(__padC,      sizeof(__padC)     , &request->__padC     , sizeof(request->__padC     )),
			#else
			"",
			#endif
			HEX(__reservedC, sizeof(__reservedC), &request->__reservedC, sizeof(request->__reservedC))
		);
	}
	else {
		bprintf(buf, bufSize, "");
	}
	return result;
} // DumpOneI2CReserved

//========================================================================================
// Utility functions

#define kIOFBAttributesKey "IOFBAttributes"

typedef struct Attribute {
	bool      set;
	bool      notnull;
	IOIndex   connectIndex;
	IOSelect  attribute;
	IOReturn  result;
	UInt64    value;
} Attribute;

void IOFB::UpdateAttribute( IOFramebuffer *service, bool set, IOIndex connectIndex, IOSelect attribute, IOReturn result, uintptr_t * value, unsigned int size)
{
	Attribute newatr = { set, value != NULL, connectIndex, attribute, result };
	OSArray *array = OSDynamicCast(OSArray, service->getProperty(kIOFBAttributesKey));
	OSData *data;

	int idx = 0;
	if (!array) {
		array = OSArray::withCapacity(0);
	}
	else {
		array = (OSArray*)array->copyCollection();
		if (array) {
			while ((data = (OSData *)array->getObject(idx))) {
				Attribute *atr = (Attribute *)data->getBytesNoCopy();
				if (!atr || (atr->set == newatr.set && atr->connectIndex == newatr.connectIndex && atr->attribute == newatr.attribute && atr->result == newatr.result)) {
					array->removeObject(idx);
					continue;
				}
				idx++;
			}
		}
	}
	if (array) {
		OSData *data = OSData::withBytes(&newatr, offsetof(Attribute, value));
		if (value) {
			data->appendBytes(value, size);
		}
		array->setObject(idx, data);
		data->release();
		
		service->setProperty(kIOFBAttributesKey, array);
	}
} // UpdateAttribute

//========================================================================================
// Wrapped functions

void IOFB::wraphideCursor( IOFramebuffer *service )
{
	IOFBVars *iofbVars = callbackIOFB->getIOFBVars(service);
	DBGLOG("iofb", "[ hideCursor fb:0x%llx", (UInt64)service);
	iofbVars->iofbvtable->orghideCursor( service );
	DBGLOG("iofb", "] hideCursor");
}

void IOFB::wrapshowCursor( IOFramebuffer *service, IOGPoint * cursorLoc, int frame )
{
	IOFBVars *iofbVars = callbackIOFB->getIOFBVars(service);
	DBGLOG("iofb", "[ showCursor fb:0x%llx", (UInt64)service);
	iofbVars->iofbvtable->orgshowCursor( service, cursorLoc, frame );
	DBGLOG("iofb", "] showCursor");
}

void IOFB::wrapmoveCursor( IOFramebuffer *service, IOGPoint * cursorLoc, int frame )
{
	IOFBVars *iofbVars = callbackIOFB->getIOFBVars(service);
	DBGLOG("iofb", "[ moveCursor fb:0x%llx", (UInt64)service);
	iofbVars->iofbvtable->orgmoveCursor( service, cursorLoc, frame );
	DBGLOG("iofb", "] moveCursor");
}

void IOFB::wrapgetVBLTime( IOFramebuffer *service, AbsoluteTime * time, AbsoluteTime * delta )
{
	IOFBVars *iofbVars = callbackIOFB->getIOFBVars(service);
	DBGLOG("iofb", "[ getVBLTime fb:0x%llx", (UInt64)service);
	iofbVars->iofbvtable->orggetVBLTime( service, time, delta );
	DBGLOG("iofb", "] getVBLTime");
}

void IOFB::wrapgetBoundingRect( IOFramebuffer *service, IOGBounds ** bounds )
{
	IOFBVars *iofbVars = callbackIOFB->getIOFBVars(service);
	DBGLOG("iofb", "[ getBoundingRect fb:0x%llx", (UInt64)service);
	iofbVars->iofbvtable->orggetBoundingRect( service, bounds );
	if (bounds && *bounds) {
		DBGLOG("iofb", "] getBoundingRect bounds(ltrb):%d,%d,%d,%d", (*bounds)->minx, (*bounds)->miny, (*bounds)->maxx, (*bounds)->maxy);
	}
	else {
		DBGLOG("iofb", "] getBoundingRect bounds:NULL");
	}
}

UInt8 whole[300];
int wholelen = 0;

IOReturn IOFB::wrapdoI2CRequest( IOFramebuffer *service, UInt32 bus, struct IOI2CBusTiming * timing, struct IOI2CRequest * request )
{
	IOFBVars *iofbVars = callbackIOFB->getIOFBVars(service);
	IOReturn result = kIOReturnSuccess;
	char resultStr[40];

	if (request->sendTransactionType == kIOI2CSimpleTransactionType && request->sendAddress == 0xfb && request->sendSubAddress == 0xfb && request->sendBuffer && request->sendBytes >= 16) {
		UInt32 category = ((UInt32*)request->sendBuffer)[0];
		UInt32 val1     = ((UInt32*)request->sendBuffer)[1];
		UInt32 val2     = ((UInt32*)request->sendBuffer)[2];
		UInt32 val3     = ((UInt32*)request->sendBuffer)[3];
		if (request->replyTransactionType == kIOI2CSimpleTransactionType && request->replyAddress == 0xfb && request->replySubAddress == 0xfb && request->replyBuffer && request->replyBytes) {
			switch (category) {
				case 'iofb':
					switch (val1) {
						case 'iofb'                              : *(UInt32*)request->replyBuffer = 'iofb'                                  ; break;
						case 'vala'                              : *(UInt32*)request->replyBuffer = iofbVars->iofbValidateAll               ; break;
						case 'sbnd'                              : *(UInt32*)request->replyBuffer = iofbVars->iofbSidebandDownRep           ; break;
						case 'sdtm'                              : *(UInt32*)request->replyBuffer = iofbVars->iofbDumpsetDetailedTimings    ; break;
						case 'pixi'                              : *(UInt32*)request->replyBuffer = iofbVars->iofbDumpgetPixelInformation   ; break;
						case 'vald'                              : *(UInt32*)request->replyBuffer = iofbVars->iofbDumpvalidateDetailedTiming; break;
						case 'i2cr'                              : *(UInt32*)request->replyBuffer = iofbVars->iofbDumpdoI2CRequest          ; break;
						case 'attr'                              : *(UInt32*)request->replyBuffer = iofbVars->iofbDumpAttributes            ; break;
						case kConnectionColorModesSupported      : *(UInt32*)request->replyBuffer = iofbVars->iofbColorModesSupported       ; break;
						case kConnectionControllerDepthsSupported: *(UInt32*)request->replyBuffer = iofbVars->iofbControllerDepthsSupported ; break;
						case kConnectionColorDepthsSupported     : *(UInt32*)request->replyBuffer = iofbVars->iofbColorDepthsSupported      ; break;
						case 'trac'                              : *(UInt32*)request->replyBuffer = callbackIOFB->gIOGATFlagsPtr ? *callbackIOFB->gIOGATFlagsPtr : 0 ; break;
						case 'dbge'                              : *(UInt32*)request->replyBuffer = ADDPR(debugEnabled)                     ; break;
						default                                  : result = kIOReturnBadArgument; break;
					}
					break;
				case 'atfc': result = wrapgetAttributeForConnection(service, val1, val2, (uintptr_t*)request->replyBuffer); break;
				case 'attr': result = wrapgetAttribute(service, val1, (uintptr_t*)request->replyBuffer)                   ; break;
				default    : result = kIOReturnBadArgument; break;
			}
			DBGLOG("iofb", "[] IofbGetAttribute %x %x %x %x%s", category, val1, val2, val3, DumpOneReturn(resultStr, sizeof(resultStr), request->result));
		}
		else {
			switch (category) {
				case 'iofb':
					switch (val1) {
						case 'vala'                              : iofbVars->iofbValidateAll                = val2; break;
						case 'sbnd'                              : iofbVars->iofbSidebandDownRep            = val2; break;
						case 'sdtm'                              : iofbVars->iofbDumpsetDetailedTimings     = val2; break;
						case 'pixi'                              : iofbVars->iofbDumpgetPixelInformation    = val2; break;
						case 'vald'                              : iofbVars->iofbDumpvalidateDetailedTiming = val2; break;
						case 'i2cr'                              : iofbVars->iofbDumpdoI2CRequest           = val2; break;
						case 'attr'                              : iofbVars->iofbDumpAttributes             = val2; break;
						case kConnectionColorModesSupported      : iofbVars->iofbColorModesSupported        = val2; break;
						case kConnectionControllerDepthsSupported: iofbVars->iofbControllerDepthsSupported  = val2; break;
						case kConnectionColorDepthsSupported     : iofbVars->iofbColorDepthsSupported       = val2; break;
						case 'trac'                              : if (callbackIOFB->gIOGATFlagsPtr) *callbackIOFB->gIOGATFlagsPtr = val2; break;
						case 'dbge'                              :
							// This is a flag that affects all WhateverGreen patches.
							// Maybe there should be a different flag for each source file?
							ADDPR(debugEnabled) = true; DBGLOG("iofb", "Setting debugEnabled to %d", val2);
							ADDPR(debugEnabled) = val2;
							break;
						default: result = kIOReturnBadArgument; break;
					}
					break;
				case 'atfc': result = wrapsetAttributeForConnection(service, val1, val2, val3); break;
				case 'attr': result = wrapsetAttribute(service, val1, val2); break;
				default    : result = kIOReturnBadArgument; break;
			}
			DBGLOG("iofb", "[] IofbSetAttribute %x %x %x %x%s", category, val1, val2, val3, DumpOneReturn(resultStr, sizeof(resultStr), request->result));
		}
		request->result = result;
		return result;
	}

	char transactionType[20];
	char hexBytes[260];
	char commFlags[100];
	char reservedStr[200];

	char istr[1000];
	char *buf;
	int bufSize;
	int inc;
	buf = istr; bufSize = sizeof(istr);

	if (iofbVars->iofbDumpdoI2CRequest) {
		inc = bprintf(buf, bufSize, "[] doI2CRequest fb:0x%llx bus:%d", (UInt64)service, bus);
		buf += inc; bufSize -= inc;
	}

	IOI2CRequest reqcopy;

	if (!request) {
		if (iofbVars->iofbDumpdoI2CRequest) {
			inc = bprintf(buf, bufSize, " request:NULL");
			buf += inc; bufSize -= inc;
		}
	}
	else {
		reqcopy = *request;

		if (iofbVars->iofbDumpdoI2CRequest) {
			inc = bprintf(buf, bufSize, " delay:%lld flags:%s completion:0x%llx",
				reqcopy.minReplyDelay,
				DumpOneCommFlags(commFlags, sizeof(commFlags), reqcopy.commFlags),
				(UInt64)reqcopy.completion
			);
			buf += inc; bufSize -= inc;

			if (reqcopy.sendTransactionType || reqcopy.sendAddress || reqcopy.sendSubAddress || reqcopy.sendBytes) {
				inc = bprintf(buf, bufSize, " (send:%s addr:0x%x.%x bytes:%d:%s)%s",
					DumpOneTransactionType(transactionType, sizeof(transactionType), reqcopy.sendTransactionType),
					reqcopy.sendAddress,
					reqcopy.sendSubAddress,
					reqcopy.sendBytes,
					HEX(hexBytes, sizeof(hexBytes), (UInt8*)reqcopy.sendBuffer, reqcopy.sendBytes),
					DumpOneI2CReserved(reservedStr, sizeof(reservedStr), &reqcopy)
				);
				buf += inc; bufSize -= inc;
			}
		}

		if (iofbVars->iofbSidebandDownRep && reqcopy.sendTransactionType == kIOI2CDisplayPortNativeTransactionType && reqcopy.sendAddress == DP_SIDEBAND_MSG_DOWN_REQ_BASE && reqcopy.sendBuffer  && reqcopy.sendBytes) {
			service->removeProperty("sideband");
		}
	}

	result = iofbVars->iofbvtable->orgdoI2CRequest( service, bus, timing, request );
	if (iofbVars->iofbDumpdoI2CRequest) {
		if (request) {
			if (reqcopy.replyTransactionType || reqcopy.replyAddress || reqcopy.replySubAddress || reqcopy.replyBytes) {
				inc = bprintf(buf, bufSize, " (reply:%s addr:0x%x.%x bytes:%d:%s)%s",
					DumpOneTransactionType(transactionType, sizeof(transactionType), reqcopy.replyTransactionType),
					reqcopy.replyAddress,
					reqcopy.replySubAddress,
					reqcopy.replyBytes,
					HEX(hexBytes, sizeof(hexBytes), (UInt8*)reqcopy.replyBuffer, reqcopy.replyBytes),
					DumpOneI2CReserved(reservedStr, sizeof(reservedStr), request)
				);
				buf += inc; bufSize -= inc;
			}
			if (request->result) {
				inc = bprintf(buf, bufSize, " request%s", DumpOneReturn(resultStr, sizeof(resultStr), request->result));
				buf += inc; bufSize -= inc;
			}
		}

		inc = bprintf(buf, bufSize, "%s", DumpOneReturn(resultStr, sizeof(resultStr), result));
		buf += inc; bufSize -= inc;
	}

	if (iofbVars->iofbDumpdoI2CRequest == 1 || ((result || (request && request->result)) && iofbVars->iofbDumpdoI2CRequest == 2)) {
		DBGLOG("iofb", "%s", istr);
	}
	buf = istr; bufSize = sizeof(istr);

	if (iofbVars->iofbSidebandDownRep && request && reqcopy.replyTransactionType == kIOI2CDisplayPortNativeTransactionType &&
		reqcopy.replyBuffer && reqcopy.replyAddress <= kDPRegisterServiceIRQ && reqcopy.replyAddress + reqcopy.replyBytes > kDPRegisterServiceIRQ &&
		*(UInt8*)(reqcopy.replyBuffer + kDPRegisterServiceIRQ - reqcopy.replyAddress) & DP_DOWN_REP_MSG_RDY
	) {
		UInt8 part[48];
		int partlen = 0;
		int partremain = 16;
		int lct;
		int bodylen;
		int eotm = 0;
		int sotm = 0;

		bzero(part, sizeof(part));
		while (partremain) {
			int replysize = partremain;
			if (replysize > 16) replysize = 16;

			// DBGLOG("iofb", "reading %d to sideband part %llx + %d; partremain=%d", replysize, (UInt64)part, partlen, partremain);

			IOReturn res;
			IOI2CRequest req;
			bzero(&req, sizeof(req));
			req.replyTransactionType = kIOI2CDisplayPortNativeTransactionType;
			req.replyAddress = DP_SIDEBAND_MSG_DOWN_REP_BASE + partlen;
			req.replyBuffer = (vm_address_t)&part[partlen];
			req.replyBytes = replysize;
			res = wrapdoI2CRequest(service, bus, timing, &req);
			if (res || req.result) {
				// DBGLOG("iofb", "abort reading part");
				break;
			}

			if (partlen == 0) {
				lct        = part[0] >> 4;
				bodylen    = part[1 + lct / 2] & 0x3f;
				eotm       = part[1 + lct / 2 + 1] & 0x40;
				sotm       = part[1 + lct / 2 + 1] & 0x80;
				partremain =      1 + lct / 2 + 2 + bodylen;
				if (replysize > partremain) {
					replysize = partremain;
				}

				// DBGLOG("iofb", "lct=%d bodylen=%d eotm=%d sotm=%d partremain=%d", lct, bodylen, eotm, sotm, partremain);
				if (sotm) {
					wholelen = 0;
					// DBGLOG("iofb", "reset sideband %llx %d", (UInt64)whole, wholelen);
				}
			}

			partlen += replysize;
			partremain -= replysize;
		} // while partremain

		if (!partremain) {
			// DBGLOG("iofb", "copy %llx %d -> %llx+%d", (UInt64)part, partlen, (UInt64)whole, wholelen);
			memcpy(&whole[wholelen], part, partlen);
			wholelen += partlen;
		}

		if (eotm) {
			// DBGLOG("iofb", "set sideband %llx %d and reset", (UInt64)whole, wholelen);
			service->setProperty("sideband", whole, wholelen);
			wholelen = 0;
		}
	} // have down reply

	return result;
}

IOReturn IOFB::wrapdiagnoseReport( IOFramebuffer *service, void * param1, void * param2, void * param3, void * param4 )
{
	IOFBVars *iofbVars = callbackIOFB->getIOFBVars(service);
	DBGLOG("iofb", "[ diagnoseReport fb:0x%llx", (UInt64)service);
	IOReturn result = iofbVars->iofbvtable->orgdiagnoseReport( service, param1, param2, param3, param4 );
	DBGLOG("iofb", "] diagnoseReport");
	return result;
}

IOReturn IOFB::wrapsetGammaTable( IOFramebuffer *service, UInt32 channelCount, UInt32 dataCount, UInt32 dataWidth, void * data, bool syncToVBL )
{
	IOFBVars *iofbVars = callbackIOFB->getIOFBVars(service);
	DBGLOG("iofb", "[ setGammaTable fb:0x%llx channelCount:%d dataCount:%d dataWidth:%d syncToVBL:%d", (UInt64)service, channelCount, dataCount, dataWidth, syncToVBL );
	IOReturn result = iofbVars->iofbvtable->orgsetGammaTable( service, channelCount, dataCount, dataWidth, data, syncToVBL );
	DBGLOG("iofb", "] setGammaTable");
	return result;
}

IOReturn IOFB::wrapopen( IOFramebuffer *service )
{
	IOFBVars *iofbVars = callbackIOFB->getIOFBVars(service);
	DBGLOG("iofb", "[ open fb:0x%llx", (UInt64)service);
	IOReturn result = iofbVars->iofbvtable->orgopen( service );
	DBGLOG("iofb", "] open");
	return result;
}

void IOFB::wrapclose( IOFramebuffer *service )
{
	IOFBVars *iofbVars = callbackIOFB->getIOFBVars(service);
	DBGLOG("iofb", "[ close fb:0x%llx", (UInt64)service);
	iofbVars->iofbvtable->orgclose( service );
	DBGLOG("iofb", "] close");
}

bool IOFB::wrapisConsoleDevice( IOFramebuffer *service )
{
	IOFBVars *iofbVars = callbackIOFB->getIOFBVars(service);
	DBGLOG("iofb", "[ isConsoleDevice fb:0x%llx", (UInt64)service);
	bool result = iofbVars->iofbvtable->orgisConsoleDevice( service );
	DBGLOG("iofb", "] isConsoleDevice result:%d", result);
	return result;
}

IOReturn IOFB::wrapsetupForCurrentConfig( IOFramebuffer *service )
{
	IOFBVars *iofbVars = callbackIOFB->getIOFBVars(service);
	#ifdef DEBUG
	char resultStr[40];
	#endif
	DBGLOG("iofb", "[ setupForCurrentConfig fb:0x%llx", (UInt64)service);
	IOReturn result = iofbVars->iofbvtable->orgsetupForCurrentConfig( service );
	DBGLOG("iofb", "] setupForCurrentConfig%s", DumpOneReturn(resultStr, sizeof(resultStr), result));
	return result;
}

bool IOFB::wrapserializeInfo( IOFramebuffer *service, OSSerialize * s )
{
	IOFBVars *iofbVars = callbackIOFB->getIOFBVars(service);
	DBGLOG("iofb", "[ serializeInfo fb:0x%llx", (UInt64)service);
	bool result = iofbVars->iofbvtable->orgserializeInfo( service, s );
	DBGLOG("iofb", "] serializeInfo result:%d", result);
	return result;
}

bool IOFB::wrapsetNumber( IOFramebuffer *service, OSDictionary * dict, const char * key, UInt32 number )
{
	IOFBVars *iofbVars = callbackIOFB->getIOFBVars(service);
	bool result = iofbVars->iofbvtable->orgsetNumber( service, dict, key, number );
	DBGLOG("iofb", "[ setNumber fb:0x%llx key:%s number:%d result:%d", (UInt64)service, key, number, result);
	return result;
}

IODeviceMemory * IOFB::wrapgetApertureRange( IOFramebuffer *service, IOPixelAperture aperture )
{
	IOFBVars *iofbVars = callbackIOFB->getIOFBVars(service);
	IODeviceMemory * result = iofbVars->iofbvtable->orggetApertureRange( service, aperture );
	DBGLOG("iofb", "[] getApertureRange fb:0x%llx aperture:%d result:0x%llx", (UInt64)service, aperture, (UInt64)result);
	return result;
}

IODeviceMemory * IOFB::wrapgetVRAMRange( IOFramebuffer *service )
{
	IOFBVars *iofbVars = callbackIOFB->getIOFBVars(service);
	IODeviceMemory * result = iofbVars->iofbvtable->orggetVRAMRange( service );
	DBGLOG("iofb", "[] getVRAMRange fb:0x%llx result:0x%llx", (UInt64)service, (UInt64)result);
	return result;
}

IOReturn IOFB::wrapenableController( IOFramebuffer *service )
{
	IOFBVars *iofbVars = callbackIOFB->getIOFBVars(service);
	#ifdef DEBUG
	char resultStr[40];
	#endif
	DBGLOG("iofb", "[ enableController fb:0x%llx", (UInt64)service);
	IOReturn result = iofbVars->iofbvtable->orgenableController( service );
	DBGLOG("iofb", "] enableController%s", DumpOneReturn(resultStr, sizeof(resultStr), result));
	return result;
}

const char * IOFB::wrapgetPixelFormats( IOFramebuffer *service )
{
	IOFBVars *iofbVars = callbackIOFB->getIOFBVars(service);
	DBGLOG("iofb", "[ getPixelFormats fb:0x%llx", (UInt64)service);
	const char * result = iofbVars->iofbvtable->orggetPixelFormats( service );
	for (const char *pixelFormat = result; pixelFormat && *pixelFormat; pixelFormat += strlen(pixelFormat) + 1) {
		DBGLOG("iofb", "pixelFormat:%s", pixelFormat);
	}
	DBGLOG("iofb", "] getPixelFormats");
	return result;
}

IOItemCount IOFB::wrapgetDisplayModeCount( IOFramebuffer *service )
{
	IOFBVars *iofbVars = callbackIOFB->getIOFBVars(service);
	IOItemCount result = iofbVars->iofbvtable->orggetDisplayModeCount( service );
	DBGLOG("iofb", "[] getDisplayModeCount fb:0x%llx result:%d", (UInt64)service, result);
	return result;
}

IOReturn IOFB::wrapgetDisplayModes( IOFramebuffer *service, IODisplayModeID * allDisplayModes )
{
	IOFBVars *iofbVars = callbackIOFB->getIOFBVars(service);
	DBGLOG("iofb", "[ getDisplayModes fb:0x%llx", (UInt64)service);
	IOItemCount count = iofbVars->iofbvtable->orggetDisplayModeCount( service );
	IOReturn result = iofbVars->iofbvtable->orggetDisplayModes( service, allDisplayModes );
	for (int i = 0; allDisplayModes && count; allDisplayModes++, count--, i++) {
		DBGLOG("iofb", "DisplayModes[%d] = id:0x%08x", i, *allDisplayModes);
	}
	#ifdef DEBUG
	char resultStr[40];
	#endif
	DBGLOG("iofb", "] getDisplayModes%s", DumpOneReturn(resultStr, sizeof(resultStr), result));
	return result;
}

IOReturn IOFB::wrapgetInformationForDisplayMode( IOFramebuffer *service, IODisplayModeID displayMode, IODisplayModeInformation * info )
{
	IOFBVars *iofbVars = callbackIOFB->getIOFBVars(service);
	IOReturn result = iofbVars->iofbvtable->orggetInformationForDisplayMode( service, displayMode, info );
	#ifdef DEBUG
	char resultStr[40];
	char modeInfo[1000];
	#endif
	DBGLOG("iofb", "[] getInformationForDisplayMode fb:0x%llx%s info:{ %s }",
		(UInt64)service,
		DumpOneReturn(resultStr, sizeof(resultStr), result),
		DumpOneDisplayModeInformationPtr(modeInfo, sizeof(modeInfo), info)
	);
	return result;
}

UInt64 IOFB::wrapgetPixelFormatsForDisplayMode( IOFramebuffer *service, IODisplayModeID displayMode, IOIndex depth )
{
	IOFBVars *iofbVars = callbackIOFB->getIOFBVars(service);
	UInt64 result = iofbVars->iofbvtable->orggetPixelFormatsForDisplayMode( service, displayMode, depth );
	DBGLOG("iofb", "[] getPixelFormatsForDisplayMode fb:0x%llx id:0x%08x depth:%d result:0x%llx", (UInt64)service, displayMode, depth, result);
	return result;
}

IOReturn IOFB::wrapgetPixelInformation( IOFramebuffer *service, IODisplayModeID displayMode, IOIndex depth, IOPixelAperture aperture, IOPixelInformation * pixelInfo )
{
	IOFBVars *iofbVars = callbackIOFB->getIOFBVars(service);
	IOReturn result = iofbVars->iofbvtable->orggetPixelInformation( service, displayMode, depth, aperture, pixelInfo );
	IOReturn newresult = result;
	const char* resultChange = "";

	if (iofbVars->iofbValidateAll && result) {
		IOTimingInformation info;
		bzero(&info, sizeof(info));
		IOReturn result1 = iofbVars->iofbvtable->orggetTimingInfoForDisplayMode( service, displayMode, &info );
		if (result1) {
			resultChange = " -> invalid displayMode";
		}
		else {
			resultChange = " -> success";
			newresult = kIOReturnSuccess;
			if (pixelInfo) {
				pixelInfo->activeWidth = info.detailedInfo.v2.horizontalActive;
				pixelInfo->activeHeight = info.detailedInfo.v2.verticalActive;
				pixelInfo->bytesPerRow = info.detailedInfo.v2.horizontalActive * 4;
				pixelInfo->bitsPerPixel = 32;
				pixelInfo->bitsPerComponent = depth ? 10 : 8;
				pixelInfo->componentCount = 3;
				pixelInfo->pixelType = kIORGBDirectPixels;
				strncpy(pixelInfo->pixelFormat, depth ? kIO30BitDirectPixels : IO32BitDirectPixels, sizeof(pixelInfo->pixelFormat));
				pixelInfo->componentMasks[0] = depth ? 0xff0000 : 0x3ff00000;
				pixelInfo->componentMasks[1] = depth ? 0xff00 : 0xffc00;
				pixelInfo->componentMasks[2] = depth ? 0xff : 0x3ff;
			}
		}
	}

	if (iofbVars->iofbDumpgetPixelInformation == 1 || (result && iofbVars->iofbDumpgetPixelInformation == 2)) {
		char reserved0[10];
		char reserved1[10];
		char pixelType[10];
		char resultStr[40];
		char pixinfo[1000];
		int inc = 0;

		inc += bprintf(pixinfo + inc, sizeof(pixinfo) - inc, "fb:0x%llx id:0x%08x depth:%d aperture:%d%s%s",
			(UInt64)service, displayMode, depth, aperture,
			DumpOneReturn(resultStr, sizeof(resultStr), result), resultChange
		);

		if (!result) {
			inc += bprintf(pixinfo + inc, sizeof(pixinfo) - inc, " %dx%d rowbytes:%d plane:%d bpp:%d bpc:%d cpp:%d type:%s format:%s masks:",
				pixelInfo->activeWidth,
				pixelInfo->activeHeight,

				pixelInfo->bytesPerRow,
				pixelInfo->bytesPerPlane,

				pixelInfo->bitsPerPixel,
				pixelInfo->bitsPerComponent,
				pixelInfo->componentCount,

				pixelInfo->pixelType == kIOCLUTPixels                   ?                   "CLUT" :
				pixelInfo->pixelType == kIOFixedCLUTPixels              ?              "FixedCLUT" :
				pixelInfo->pixelType == kIORGBDirectPixels              ?              "RGBDirect" :
				pixelInfo->pixelType == kIOMonoDirectPixels             ?             "MonoDirect" :
				pixelInfo->pixelType == kIOMonoInverseDirectPixels      ?      "MonoInverseDirect" :
				pixelInfo->pixelType == kIORGBSignedDirectPixels        ?        "RGBSignedDirect" :
				pixelInfo->pixelType == kIORGBSignedFloatingPointPixels ? "RGBSignedFloatingPoint" :
				UNKNOWN_VALUE(pixelType, sizeof(pixelType), pixelInfo->pixelType),

				pixelInfo->pixelFormat
			);

			int i;
			for (i = 16; i > 0 && pixelInfo->componentMasks[i-1] == 0; i--) {}
			for (int j = 0; j < i; j++) {
				inc += bprintf(pixinfo + inc, sizeof(pixinfo) - inc, "%s%x",
					j ? "." : "",
					(unsigned int)pixelInfo->componentMasks[j]
				);
			}

			inc += bprintf(pixinfo + inc, sizeof(pixinfo) - inc, " flags:%d%s%s%s%s",
				(unsigned int)pixelInfo->flags,
				pixelInfo->reserved[ 0 ] ? " reserved0:" : "",
				pixelInfo->reserved[ 0 ] ? UNKNOWN_VALUE(reserved0, sizeof(reserved0), pixelInfo->reserved[ 0 ]) : "",
				pixelInfo->reserved[ 1 ] ? " reserved1:" : "",
				pixelInfo->reserved[ 1 ] ? UNKNOWN_VALUE(reserved1, sizeof(reserved1), pixelInfo->reserved[ 1 ]) : ""
			);
		}

		DBGLOG("iofb", "[] getPixelInformation %s", pixinfo);
	}

	return newresult;
}

IOReturn IOFB::wrapgetCurrentDisplayMode( IOFramebuffer *service, IODisplayModeID * displayMode, IOIndex * depth )
{
	IOFBVars *iofbVars = callbackIOFB->getIOFBVars(service);
	IOReturn result = iofbVars->iofbvtable->orggetCurrentDisplayMode( service, displayMode, depth );
	#ifdef DEBUG
	char resultStr[40];
	#endif
	DBGLOG("iofb", "[] getCurrentDisplayMode fb:0x%llx%s id:0x%08x depth:%d", (UInt64)service, DumpOneReturn(resultStr, sizeof(resultStr), result), *displayMode, *depth);
	return result;
}

IOReturn IOFB::wrapsetDisplayMode( IOFramebuffer *service, IODisplayModeID displayMode, IOIndex depth )
{
	IOFBVars *iofbVars = callbackIOFB->getIOFBVars(service);
	IOReturn result = iofbVars->iofbvtable->orgsetDisplayMode( service, displayMode, depth );
	#ifdef DEBUG
	char resultStr[40];
	#endif
	DBGLOG("iofb", "[] setDisplayMode fb:0x%llx id:0x%08x depth:%d%s", (UInt64)service, displayMode, depth, DumpOneReturn(resultStr, sizeof(resultStr), result));
	return result;
}

IOReturn IOFB::wrapsetApertureEnable( IOFramebuffer *service, IOPixelAperture aperture, IOOptionBits enable )
{
	IOFBVars *iofbVars = callbackIOFB->getIOFBVars(service);
	DBGLOG("iofb", "[ setApertureEnable fb:0x%llx aperture:%d enable:%d", (UInt64)service, aperture, enable);
	IOReturn result = iofbVars->iofbvtable->orgsetApertureEnable( service, aperture, enable );
	#ifdef DEBUG
	char resultStr[40];
	#endif
	DBGLOG("iofb", "] setApertureEnable%s", DumpOneReturn(resultStr, sizeof(resultStr), result));
	return result;
}

IOReturn IOFB::wrapsetStartupDisplayMode( IOFramebuffer *service, IODisplayModeID displayMode, IOIndex depth )
{
	IOFBVars *iofbVars = callbackIOFB->getIOFBVars(service);
	IOReturn result = iofbVars->iofbvtable->orgsetStartupDisplayMode( service, displayMode, depth );
	#ifdef DEBUG
	char resultStr[40];
	#endif
	DBGLOG("iofb", "[] setStartupDisplayMode fb:0x%llx id:0x%08x depth:%d%s",
		(UInt64)service, displayMode, depth,
		DumpOneReturn(resultStr, sizeof(resultStr), result)
	);
	return result;
}

IOReturn IOFB::wrapgetStartupDisplayMode( IOFramebuffer *service, IODisplayModeID * displayMode, IOIndex * depth )
{
	IOFBVars *iofbVars = callbackIOFB->getIOFBVars(service);
	DBGLOG("iofb", "[ getStartupDisplayMode fb:0x%llx", (UInt64)service);
	IOReturn result = iofbVars->iofbvtable->orggetStartupDisplayMode( service, displayMode, depth );
	#ifdef DEBUG
	char resultStr[40];
	#endif
	DBGLOG("iofb", "] getStartupDisplayMode%s id:0x%08x depth:%d", DumpOneReturn(resultStr, sizeof(resultStr), result), *displayMode, *depth);
	return result;
}

IOReturn IOFB::wrapsetCLUTWithEntries( IOFramebuffer *service, IOColorEntry * colors, UInt32 index, UInt32 numEntries, IOOptionBits options )
{
	IOFBVars *iofbVars = callbackIOFB->getIOFBVars(service);
	DBGLOG("iofb", "[ setCLUTWithEntries fb:0x%llx index:%d entries:%d options:%x", (UInt64)service, index, numEntries, options);
	IOReturn result = iofbVars->iofbvtable->orgsetCLUTWithEntries( service, colors, index, numEntries, options );
	#ifdef DEBUG
	char resultStr[40];
	#endif
	DBGLOG("iofb", "] setCLUTWithEntries%s", DumpOneReturn(resultStr, sizeof(resultStr), result));
	return result;
}

IOReturn IOFB::wrapsetGammaTable2( IOFramebuffer *service, UInt32 channelCount, UInt32 dataCount, UInt32 dataWidth, void * data )
{
	IOFBVars *iofbVars = callbackIOFB->getIOFBVars(service);
	DBGLOG("iofb", "[ setGammaTable2 fb:0x%llx channelCount:%d dataCount:%d dataWidth:%d", (UInt64)service, channelCount, dataCount, dataWidth);
	IOReturn result = iofbVars->iofbvtable->orgsetGammaTable2( service, channelCount, dataCount, dataWidth, data );
	#ifdef DEBUG
	char resultStr[40];
	#endif
	DBGLOG("iofb", "] setGammaTable2%s", DumpOneReturn(resultStr, sizeof(resultStr), result));
	return result;
}

IOReturn IOFB::wrapsetAttribute( IOFramebuffer *service, IOSelect attribute, uintptr_t value )
{
	IOFBVars *iofbVars = callbackIOFB->getIOFBVars(service);
	#ifdef DEBUG
	char attributeStr[100];
	char valueStr[1000];
	char resultStr[40];
	#endif
	IOReturn result = iofbVars->iofbvtable->orgsetAttribute( service, attribute, value );
	if (iofbVars->iofbDumpAttributes == 1 || (result && iofbVars->iofbDumpAttributes == 2)) {
		DBGLOG("iofb", "[] setAttribute fb:0x%llx attribute:%s value:%s%s", (UInt64)service,
			DumpOneAttribute(attributeStr, sizeof(attributeStr), attribute, false),
			DumpOneAttributeValue(valueStr, sizeof(valueStr), attribute, true, false, &value, sizeof(value)),
			DumpOneReturn(resultStr, sizeof(resultStr), result)
		);
	}
	callbackIOFB->UpdateAttribute( service, true, -1, attribute, result, &value, sizeof(value));
	return result;
}

IOReturn IOFB::wrapgetAttribute( IOFramebuffer *service, IOSelect attribute, uintptr_t * value )
{
	IOFBVars *iofbVars = callbackIOFB->getIOFBVars(service);
	#ifdef DEBUG
	char attributeStr[100];
	char valueStr[1000];
	char resultStr[40];
	#endif
	IOReturn result = iofbVars->iofbvtable->orggetAttribute( service, attribute, value );
	if (iofbVars->iofbDumpAttributes == 1 || (result && iofbVars->iofbDumpAttributes == 2)) {
		DBGLOG("iofb", "[] getAttribute fb:0x%llx attribute:%s%s value:%s", (UInt64)service,
			DumpOneAttribute(attributeStr, sizeof(attributeStr), attribute, false),
			DumpOneReturn(resultStr, sizeof(resultStr), result),
			DumpOneAttributeValue(valueStr, sizeof(valueStr), attribute, false, false, value, sizeof(*value))
		);
	}
	callbackIOFB->UpdateAttribute( service, false, -1, attribute, result, value, value ? sizeof(*value) : 0);
	return result;
}

IOReturn IOFB::wrapgetTimingInfoForDisplayMode( IOFramebuffer *service, IODisplayModeID displayMode, IOTimingInformation * info )
{
	IOFBVars *iofbVars = callbackIOFB->getIOFBVars(service);
	IOReturn result = iofbVars->iofbvtable->orggetTimingInfoForDisplayMode( service, displayMode, info );
	#ifdef DEBUG
	char timinginfo[1000];
	char resultStr[40];
	#endif
	if (result) {
		DBGLOG("iofb", "[] getTimingInfoForDisplayMode fb:0x%llx id:0x%08x%s",
			(UInt64)service, displayMode,
			DumpOneReturn(resultStr, sizeof(resultStr), result)
		);
	}
	else {
		DBGLOG("iofb", "[] getTimingInfoForDisplayMode fb:0x%llx id:0x%08x%s timing:{ %s }",
			(UInt64)service, displayMode,
			DumpOneReturn(resultStr, sizeof(resultStr), result),
			DumpOneTimingInformationPtr(timinginfo, sizeof(timinginfo), info)
		);
	}
	return result;
}

#define kIOFBInvalidModesKey "IOFBInvalidModes"

IOReturn IOFB::wrapvalidateDetailedTiming( IOFramebuffer *service, void * description, IOByteCount descripSize )
{
	IOFBVars *iofbVars = callbackIOFB->getIOFBVars(service);
	IOReturn result = iofbVars->iofbvtable->orgvalidateDetailedTiming( service, description, descripSize );
	IOReturn newresult = result;
	const char* resultChange = "";

	#ifdef DEBUG
	IOFBDisplayModeDescription* desc = (IOFBDisplayModeDescription*)description;
	#endif

	if (result) {
		if (iofbVars->iofbValidateAll) {
			if (descripSize == sizeof(IOFBDisplayModeDescription) /* && desc->info.nominalWidth && desc->info.nominalHeight */) {
				resultChange = " -> success";
				newresult = kIOReturnSuccess;
			}
		}

		OSArray *array = OSDynamicCast(OSArray, service->getProperty(kIOFBInvalidModesKey));
		OSData *data;

		int idx = 0;
		if (!array) {
			array = OSArray::withCapacity(0);
		}
		else {
			array = (OSArray*)array->copyCollection();
			if (array) {
				while ((data = (OSData *)array->getObject(idx))) {
					const void *bytes = data->getBytesNoCopy();
					unsigned int len = data->getLength();
					if (!bytes || ( (*(IOReturn*)bytes == result) && (len == sizeof(IOReturn) + descripSize) && (!memcmp((UInt8*)bytes + sizeof(IOReturn), description, descripSize)) )) {
						array->removeObject(idx);
						continue;
					}
					idx++;
				}
			}
		}
		if (array) {
			OSData *data = OSData::withBytes(&result, sizeof(result));
			data->appendBytes(description, (unsigned int)descripSize);
			array->setObject(idx, data);
			data->release();
			
			service->setProperty(kIOFBInvalidModesKey, array);
		}
	}

	if (iofbVars->iofbDumpvalidateDetailedTiming == 1 || (result && iofbVars->iofbDumpvalidateDetailedTiming == 2)) {
		#ifdef DEBUG
		char timinginfo[1000];
		char resultStr[40];
		#endif

		if (descripSize == sizeof(IOFBDisplayModeDescription))
		{
			DBGLOG("iofb", "[] validateDetailedTiming fb:0x%llx size:%lld desc:{ %s }%s%s", (UInt64)service, descripSize,
				DumpOneFBDisplayModeDescriptionPtr(timinginfo, sizeof(timinginfo), desc),
				DumpOneReturn(resultStr, sizeof(resultStr), result), resultChange
			);
		}
		else {
			DBGLOG("iofb", "[] validateDetailedTiming fb:0x%llx size:%lld detailed:{ %s }%s%s", (UInt64)service, descripSize,
				DumpOneDetailedTimingInformationPtr(timinginfo, sizeof(timinginfo), description, descripSize),
				DumpOneReturn(resultStr, sizeof(resultStr), result), resultChange
			);
		}
	}
/*
	else {
		DBGLOG("iofb", "[] validateDetailedTiming fb:0x%llx", (UInt64)service);
	}
*/

	return newresult;
}

IOReturn IOFB::wrapsetDetailedTimings( IOFramebuffer *service, OSArray * array )
{
	IOFBVars *iofbVars = callbackIOFB->getIOFBVars(service);
	DBGLOG("iofb", "[ setDetailedTimings fb:0x%llx array:0x%llx count:%u", (UInt64)service, (UInt64)array, array ? array->getCount() : 0);

	IOReturn result = iofbVars->iofbvtable->orgsetDetailedTimings( service, array );

	#ifdef DEBUG
	char resultStr[40];
	char timinginfo[1000];
	#endif
	if (iofbVars->iofbDumpsetDetailedTimings == 1 || (result && iofbVars->iofbDumpsetDetailedTimings == 2)) {
		OSData * data;
		if (array) {
	//		for (int idx = 0; (data = OSDynamicCast(OSData, array->getObject(idx))); idx++) {
			for (int idx = 0; (data = (OSData *)array->getObject(idx)); idx++) {
				if (data->metaCast("OSData")) {
					DBGLOG("iofb", "[%d] = detailed:{ %s }", idx,
						DumpOneDetailedTimingInformationPtr(timinginfo, sizeof(timinginfo), data->getBytesNoCopy(), data->getLength())
					);
				}
				else {
					DBGLOG("iofb", "[%d] = not data", idx);
				}
			}
		}
	}

	DBGLOG("iofb", "] setDetailedTimings%s", DumpOneReturn(resultStr, sizeof(resultStr), result));
	return result;
}

IOItemCount IOFB::wrapgetConnectionCount( IOFramebuffer *service )
{
	IOFBVars *iofbVars = callbackIOFB->getIOFBVars(service);
	IOItemCount result = iofbVars->iofbvtable->orggetConnectionCount( service );
	DBGLOG("iofb", "[] getConnectionCount fb:0x%llx result:%d", (UInt64)service, result);
	return result;
}

IOReturn IOFB::wrapsetAttributeForConnection( IOFramebuffer *service, IOIndex connectIndex, IOSelect attribute, uintptr_t value )
{
	IOFBVars *iofbVars = callbackIOFB->getIOFBVars(service);
	#ifdef DEBUG
	char attributeStr[100];
	char valueStr[1000];
	char resultStr[40];
	#endif
	IOReturn result = iofbVars->iofbvtable->orgsetAttributeForConnection( service, connectIndex, attribute, value );

	if (iofbVars->iofbDumpAttributes == 1 || (result && iofbVars->iofbDumpAttributes == 2)) {
		DBGLOG("iofb", "[] setAttributeForConnection fb:0x%llx connectIndex:%d attribute:%s value:%s%s",
			(UInt64)service, connectIndex,
			DumpOneAttribute(attributeStr, sizeof(attributeStr), attribute, true),
			DumpOneAttributeValue(valueStr, sizeof(valueStr), attribute, true, true, &value, sizeof(value)),
			DumpOneReturn(resultStr, sizeof(resultStr), result)
		);
	}
	callbackIOFB->UpdateAttribute( service, true, connectIndex, attribute, result, &value, sizeof(value));

	/**/ if (attribute == kConnectionColorModesSupported && iofbVars->iofbColorModesSupported && (value != iofbVars->iofbColorModesSupported)) {
		wrapsetAttributeForConnection(service, connectIndex, attribute, iofbVars->iofbColorModesSupported);
	}
	else if (attribute == kConnectionColorDepthsSupported && iofbVars->iofbColorDepthsSupported && (value != iofbVars->iofbColorDepthsSupported)) {
		wrapsetAttributeForConnection(service, connectIndex, attribute, iofbVars->iofbColorDepthsSupported);
	}

	return result;
}

IOReturn IOFB::wrapgetAttributeForConnection( IOFramebuffer *service, IOIndex connectIndex, IOSelect attribute, uintptr_t * value )
{
	IOFBVars *iofbVars = callbackIOFB->getIOFBVars(service);
	IOReturn result = iofbVars->iofbvtable->orggetAttributeForConnection( service, connectIndex, attribute, value );

	unsigned int size = 0;
#ifdef DEBUG
	char attributeStr[100];
	char valueStr[1000];
	char resultStr[40];

	valueStr[0] = '\0';
	int inc = 0;
#endif
	if (value) {
		if (attribute == kConnectionDisplayParameters) {
			uintptr_t numParameters = 0;
			if (kIOReturnSuccess != iofbVars->iofbvtable->orggetAttributeForConnection(service, connectIndex, kConnectionDisplayParameterCount, &numParameters)) {
				#ifdef DEBUG
				inc = bprintf(valueStr, sizeof(valueStr), "(error getting parameter count)");
				#endif
			}
			else {
				#ifdef DEBUG
				DumpOneDisplayParameters(valueStr + inc, sizeof(valueStr) - inc, value, numParameters);
				#endif
			}
			size = (unsigned int)numParameters * sizeof(uintptr_t);

		}
		else if (attribute == kConnectionHandleDisplayPortEvent) {
			size = 16 * sizeof(uintptr_t);
			#ifdef DEBUG
			HEX(valueStr, sizeof(valueStr), value, size);
			#endif
		}
		else {
			size = sizeof(*value);
			#ifdef DEBUG
			DumpOneAttributeValue(valueStr, sizeof(valueStr), attribute, false, true, value, size);
			#endif
		}
	}
	else {
		size = 0;
		#ifdef DEBUG
		bprintf(valueStr, sizeof(valueStr), "NULL");
		#endif
	}

#ifdef DEBUG
	if (iofbVars->iofbDumpAttributes == 1 || (result && iofbVars->iofbDumpAttributes == 2)) {
		DBGLOG("iofb", "[] getAttributeForConnection fb:0x%llx connectIndex:%d attribute:%s%s value:%s",
			(UInt64)service, connectIndex, DumpOneAttribute(attributeStr, sizeof(attributeStr), attribute, true),
			DumpOneReturn(resultStr, sizeof(resultStr), result),
			valueStr
		);
	}
#endif
	
	callbackIOFB->UpdateAttribute( service, false, connectIndex, attribute, result, value, size);

	if (attribute == kConnectionControllerDepthsSupported && iofbVars->iofbControllerDepthsSupported && (!value || *value != iofbVars->iofbControllerDepthsSupported)) {
		wrapsetAttributeForConnection(service, connectIndex, attribute, iofbVars->iofbControllerDepthsSupported);
		*value = iofbVars->iofbControllerDepthsSupported;
	}

	return result;
}

bool IOFB::wrapconvertCursorImage( IOFramebuffer *service, void * cursorImage, IOHardwareCursorDescriptor * description, IOHardwareCursorInfo * cursor )
{
	IOFBVars *iofbVars = callbackIOFB->getIOFBVars(service);
	DBGLOG("iofb", "[ convertCursorImage fb:0x%llx", (UInt64)service);
	bool result = iofbVars->iofbvtable->orgconvertCursorImage( service, cursorImage, description, cursor );
	DBGLOG("iofb", "] convertCursorImage result:%d", result);
	return result;
}

IOReturn IOFB::wrapsetCursorImage( IOFramebuffer *service, void * cursorImage )
{
	IOFBVars *iofbVars = callbackIOFB->getIOFBVars(service);
	DBGLOG("iofb", "[ setCursorImage fb:0x%llx", (UInt64)service);
	IOReturn result = iofbVars->iofbvtable->orgsetCursorImage( service, cursorImage );
	#ifdef DEBUG
	char resultStr[40];
	#endif
	DBGLOG("iofb", "] setCursorImage%s", DumpOneReturn(resultStr, sizeof(resultStr), result));
	return result;
}

IOReturn IOFB::wrapsetCursorState( IOFramebuffer *service, SInt32 x, SInt32 y, bool visible )
{
	IOFBVars *iofbVars = callbackIOFB->getIOFBVars(service);
	DBGLOG("iofb", "[ setCursorState fb:0x%llx %d,%d visible:%d", (UInt64)service, x, y, visible);
	IOReturn result = iofbVars->iofbvtable->orgsetCursorState( service, x, y, visible );
	#ifdef DEBUG
	char resultStr[40];
	#endif
	DBGLOG("iofb", "] setCursorState%s", DumpOneReturn(resultStr, sizeof(resultStr), result));
	return result;
}

void IOFB::wrapflushCursor( IOFramebuffer *service )
{
	IOFBVars *iofbVars = callbackIOFB->getIOFBVars(service);
	DBGLOG("iofb", "[ flushCursor fb:0x%llx", (UInt64)service);
	iofbVars->iofbvtable->orgflushCursor( service );
	DBGLOG("iofb", "] flushCursor");
}

IOReturn IOFB::wrapgetAppleSense( IOFramebuffer *service, IOIndex connectIndex, UInt32 * senseType, UInt32 * primary, UInt32 * extended, UInt32 * displayType )
{
	IOFBVars *iofbVars = callbackIOFB->getIOFBVars(service);
	IOReturn result = iofbVars->iofbvtable->orggetAppleSense( service, connectIndex, senseType, primary, extended, displayType );
	#ifdef DEBUG
	char resultStr[40];
	char senseTypeStr[30];
	char primaryStr[30];
	char extendedStr[30];
	char displayTypeStr[30];
	#endif
	DBGLOG("iofb", "[] getAppleSense fb:0x%llx connectIndex:%d%s senseType:%s primary:%s extended:%s displayType:%s",
		(UInt64)service, connectIndex,
		DumpOneReturn(resultStr, sizeof(resultStr), result),
		senseType ?
			*senseType == 0 ? "Legacy" :
			*senseType == 1 ? "Not_legacy" :
			UNKNOWN_VALUE(senseTypeStr, sizeof(senseTypeStr), *senseType)
		:
			"NULL",
		primary ? DumpOnePrimarySense(primaryStr, sizeof(primaryStr), *primary) : "NULL",
		extended ? DumpOnePrimaryExtendedSense(extendedStr, sizeof(extendedStr), *extended) : "NULL",
		displayType ? DumpOneDisplayType(displayTypeStr, sizeof(displayTypeStr), *displayType) : "NULL"
	);
	return result;
}

IOReturn IOFB::wrapconnectFlags( IOFramebuffer *service, IOIndex connectIndex, IODisplayModeID displayMode, IOOptionBits * flags )
{
	IOFBVars *iofbVars = callbackIOFB->getIOFBVars(service);
	IOReturn result = iofbVars->iofbvtable->orgconnectFlags( service, connectIndex, displayMode, flags );
	#ifdef DEBUG
	char flagsstr[200];
	char resultStr[40];
	#endif
	DBGLOG("iofb", "[] connectFlags fb:0x%llx connectIndex:%d id:0x%08x%s flags:%s",
		(UInt64)service, connectIndex, displayMode,
		DumpOneReturn(resultStr, sizeof(resultStr), result),
		flags ? GetOneFlagsStr(flagsstr, sizeof(flagsstr), *flags) : "NULL"
	);
	return result;
}

void IOFB::wrapsetDDCClock( IOFramebuffer *service, IOIndex bus, UInt32 value )
{
	IOFBVars *iofbVars = callbackIOFB->getIOFBVars(service);
	DBGLOG("iofb", "[ setDDCClock fb:0x%llx bus:%d value:%d", (UInt64)service, bus, value);
	iofbVars->iofbvtable->orgsetDDCClock( service, bus, value );
	DBGLOG("iofb", "] setDDCClock");
}

void IOFB::wrapsetDDCData( IOFramebuffer *service, IOIndex bus, UInt32 value )
{
	IOFBVars *iofbVars = callbackIOFB->getIOFBVars(service);
	DBGLOG("iofb", "[ setDDCData fb:0x%llx bus:%d value:%d", (UInt64)service, bus, value);
	iofbVars->iofbvtable->orgsetDDCData( service, bus, value );
	DBGLOG("iofb", "] setDDCData");
}

bool IOFB::wrapreadDDCClock( IOFramebuffer *service, IOIndex bus )
{
	IOFBVars *iofbVars = callbackIOFB->getIOFBVars(service);
	DBGLOG("iofb", "[ readDDCClock fb:0x%llx bus:%d", (UInt64)service, bus);
	bool result = iofbVars->iofbvtable->orgreadDDCClock( service, bus );
	DBGLOG("iofb", "] readDDCClock result:%d", result);
	return result;
}

bool IOFB::wrapreadDDCData( IOFramebuffer *service, IOIndex bus )
{
	IOFBVars *iofbVars = callbackIOFB->getIOFBVars(service);
	DBGLOG("iofb", "[ readDDCData fb:0x%llx bus:%d", (UInt64)service, bus);
	bool result = iofbVars->iofbvtable->orgreadDDCData( service, bus );
	DBGLOG("iofb", "] readDDCData result:%d", result);
	return result;
}

IOReturn IOFB::wrapenableDDCRaster( IOFramebuffer *service, bool enable )
{
	IOFBVars *iofbVars = callbackIOFB->getIOFBVars(service);
	DBGLOG("iofb", "[ enableDDCRaster fb:0x%llx enable:%d", (UInt64)service, enable);
	IOReturn result = iofbVars->iofbvtable->orgenableDDCRaster( service, enable );
	#ifdef DEBUG
	char resultStr[40];
	#endif
	DBGLOG("iofb", "] enableDDCRaster%s", DumpOneReturn(resultStr, sizeof(resultStr), result));
	return result;
}

bool IOFB::wraphasDDCConnect( IOFramebuffer *service, IOIndex connectIndex )
{
	IOFBVars *iofbVars = callbackIOFB->getIOFBVars(service);
	bool result = iofbVars->iofbvtable->orghasDDCConnect( service, connectIndex );
	DBGLOG("iofb", "[] hasDDCConnect fb:0x%llx connectIndex:%d result:%d", (UInt64)service, connectIndex, result);
	return result;
}

IOReturn IOFB::wrapgetDDCBlock( IOFramebuffer *service, IOIndex connectIndex, UInt32 blockNumber, IOSelect blockType, IOOptionBits options, UInt8 * data, IOByteCount * length )
{
	IOFBVars *iofbVars = callbackIOFB->getIOFBVars(service);
	char hex[1000];
	IOReturn result = iofbVars->iofbvtable->orggetDDCBlock( service, connectIndex, blockNumber, blockType, options, data, length );
	char resultStr[40];
	unsigned int len = (length ? (unsigned int)*length : 128);
	DBGLOG("iofb", "[] getDDCBlock fb:0x%llx connectIndex:%d blockNumber:%d blockType:%d options:%x%s bytes:%s",
		(UInt64)service, connectIndex, blockNumber, blockType, options,
		DumpOneReturn(resultStr, sizeof(resultStr), result), HEX(hex, sizeof(hex), data, len)
	);

	if (!result) {
		char propertyName[20];
		bprintf(propertyName, sizeof(propertyName), "IOFBEDID%d", connectIndex);
		OSData *oldData = OSDynamicCast(OSData, service->getProperty(propertyName));
		OSData *edidData;
		unsigned int newLength = (blockNumber - 1) * 128 + len;
		if (oldData) {
			edidData = OSData::withData(oldData);
		}
		else {
			edidData = OSData::withCapacity(128);
		}
		if (edidData) {
			if (edidData->getLength() < newLength) {
				edidData->appendBytes(NULL, newLength - edidData->getLength());
			}
		}
		if (edidData) {
			memcpy((UInt8 *)edidData->getBytesNoCopy() + (blockNumber - 1) * 128, data, len);
			service->setProperty(propertyName, edidData);
			edidData->release();
		}
	}

	return result;
}

IOReturn IOFB::wrapregisterForInterruptType( IOFramebuffer *service, IOSelect interruptType, IOFBInterruptProc proc, OSObject * target, void * ref, void ** interruptRef )
{
	IOFBVars *iofbVars = callbackIOFB->getIOFBVars(service);
	#ifdef DEBUG
	char interrupt[100];
	char resultStr[40];
	#endif
	IOReturn result = iofbVars->iofbvtable->orgregisterForInterruptType( service, interruptType, proc, target, ref, interruptRef );
	DBGLOG("iofb", "[] registerForInterruptType fb:0x%llx interruptType:%s%s", (UInt64)service,
		DumpOneInterruptType(interrupt, sizeof(interrupt), interruptType),
		DumpOneReturn(resultStr, sizeof(resultStr), result));
	return result;
}

IOReturn IOFB::wrapunregisterInterrupt( IOFramebuffer *service, void * interruptRef )
{
	IOFBVars *iofbVars = callbackIOFB->getIOFBVars(service);
	IOReturn result = iofbVars->iofbvtable->orgunregisterInterrupt( service, interruptRef );
	#ifdef DEBUG
	char resultStr[40];
	#endif
	DBGLOG("iofb", "[] unregisterInterrupt fb:0x%llx interruptRef:0x%llx%s", (UInt64)service, (UInt64)interruptRef, DumpOneReturn(resultStr, sizeof(resultStr), result));
	return result;
}

IOReturn IOFB::wrapsetInterruptState( IOFramebuffer *service, void * interruptRef, UInt32 state )
{
	IOFBVars *iofbVars = callbackIOFB->getIOFBVars(service);
	DBGLOG("iofb", "[ setInterruptState fb:0x%llx interruptRef:0x%llx state:%x", (UInt64)service, (UInt64)interruptRef, state);
	IOReturn result = iofbVars->iofbvtable->orgsetInterruptState( service, interruptRef, state );
	#ifdef DEBUG
	char resultStr[40];
	#endif
	DBGLOG("iofb", "] setInterruptState%s", DumpOneReturn(resultStr, sizeof(resultStr), result));
	return result;
}

IOReturn IOFB::wrapgetNotificationSemaphore( IOFramebuffer *service, IOSelect interruptType, semaphore_t * semaphore )
{
	IOFBVars *iofbVars = callbackIOFB->getIOFBVars(service);
	#ifdef DEBUG
	char interrupt[100];
	#endif
	DBGLOG("iofb", "[ getNotificationSemaphore fb:0x%llx interruptType:%s", (UInt64)service, DumpOneInterruptType(interrupt, sizeof(interrupt), interruptType));
	IOReturn result = iofbVars->iofbvtable->orggetNotificationSemaphore( service, interruptType, semaphore );
	#ifdef DEBUG
	char resultStr[40];
	#endif
	DBGLOG("iofb", "] getNotificationSemaphore%s", DumpOneReturn(resultStr, sizeof(resultStr), result));
	return result;
}

IOReturn IOFB::wrapdoDriverIO( IONDRVFramebuffer *service, UInt32 commandID, void * contents, UInt32 commandCode, UInt32 commandKind )
{
	IOFBVars *iofbVars = callbackIOFB->getIOFBVars(service);
	DBGLOG("iofb", "[ doDriverIO fb:0x%llx commandID:%d commandCode:%d commandKind:%d", (UInt64)service, commandID, commandCode, commandKind);
	IOReturn result = iofbVars->iofbvtable->orgdoDriverIO( service, commandID, contents, commandCode, commandKind );
	#ifdef DEBUG
	char resultStr[40];
	#endif
	DBGLOG("iofb", "] doDriverIO%s", DumpOneReturn(resultStr, sizeof(resultStr), result));
	return result;
}

IOReturn IOFB::wrapcheckDriver( IONDRVFramebuffer *service )
{
	IOFBVars *iofbVars = callbackIOFB->getIOFBVars(service);
	DBGLOG("iofb", "[ checkDriver fb:0x%llx", (UInt64)service);
	IOReturn result = iofbVars->iofbvtable->orgcheckDriver( service );
	#ifdef DEBUG
	char resultStr[40];
	#endif
DBGLOG("iofb", "] checkDriver%s", DumpOneReturn(resultStr, sizeof(resultStr), result));
	return result;
}

UInt32 IOFB::wrapiterateAllModes( IONDRVFramebuffer *service, IODisplayModeID * displayModeIDs )
{
	IOFBVars *iofbVars = callbackIOFB->getIOFBVars(service);
	DBGLOG("iofb", "[ iterateAllModes fb:0x%llx", (UInt64)service);
	UInt32 result = iofbVars->iofbvtable->orgiterateAllModes( service, displayModeIDs );
	DBGLOG("iofb", "] iterateAllModes result:0x%x", result);
	return result;
}

IOReturn IOFB::wrapgetResInfoForMode( IONDRVFramebuffer *service, IODisplayModeID modeID, IODisplayModeInformation * theInfo )
{
	IOFBVars *iofbVars = callbackIOFB->getIOFBVars(service);
	DBGLOG("iofb", "[ getResInfoForMode fb:0x%llx id:0x%08x", (UInt64)service, modeID);
	IOReturn result = iofbVars->iofbvtable->orggetResInfoForMode( service, modeID, theInfo );
	#ifdef DEBUG
	char modeInfo[1000];
	char resultStr[40];
	#endif
	DBGLOG("iofb", "] getResInfoForMode%s info:{ %s }",
		DumpOneReturn(resultStr, sizeof(resultStr), result),
		DumpOneDisplayModeInformationPtr(modeInfo, sizeof(modeInfo), theInfo)
	);
	return result;
}

IOReturn IOFB::wrapgetResInfoForArbMode( IONDRVFramebuffer *service, IODisplayModeID modeID, IODisplayModeInformation * theInfo )
{
	IOFBVars *iofbVars = callbackIOFB->getIOFBVars(service);
	DBGLOG("iofb", "[ getResInfoForArbMode fb:0x%llx id:0x%08x", (UInt64)service, modeID);
	IOReturn result = iofbVars->iofbvtable->orggetResInfoForArbMode( service, modeID, theInfo );
	#ifdef DEBUG
	char modeInfo[1000];
	char resultStr[40];
	#endif
	DBGLOG("iofb", "] getResInfoForArbMode%s info:{ %s }",
		DumpOneReturn(resultStr, sizeof(resultStr), result),
		DumpOneDisplayModeInformationPtr(modeInfo, sizeof(modeInfo), theInfo)
	);
	return result;
}

IOReturn IOFB::wrapvalidateDisplayMode( IONDRVFramebuffer *service, IODisplayModeID mode, IOOptionBits flags, VDDetailedTimingRec ** detailed )
{
	IOFBVars *iofbVars = callbackIOFB->getIOFBVars(service);
	DBGLOG("iofb", "[ validateDisplayMode fb:0x%llx id:0x%08x flags:%x", (UInt64)service, mode, flags);
	IOReturn result = iofbVars->iofbvtable->orgvalidateDisplayMode( service, mode, flags, detailed );
	#ifdef DEBUG
	char resultStr[40];
	#endif
	DBGLOG("iofb", "] validateDisplayMode%s", DumpOneReturn(resultStr, sizeof(resultStr), result));
	return result;
}

IOReturn IOFB::wrapsetDetailedTiming( IONDRVFramebuffer *service, IODisplayModeID mode, IOOptionBits options, void * description, IOByteCount descripSize )
{
	IOFBVars *iofbVars = callbackIOFB->getIOFBVars(service);
	DBGLOG("iofb", "[ setDetailedTiming fb:0x%llx id:0x%08x options:%x", (UInt64)service, mode, options);
	IOReturn result = iofbVars->iofbvtable->orgsetDetailedTiming( service, mode, options, description, descripSize );
	#ifdef DEBUG
	char resultStr[40];
	#endif
	DBGLOG("iofb", "] setDetailedTiming%s", DumpOneReturn(resultStr, sizeof(resultStr), result));
	return result;
}

void IOFB::wrapgetCurrentConfiguration( IONDRVFramebuffer *service )
{
	IOFBVars *iofbVars = callbackIOFB->getIOFBVars(service);
	DBGLOG("iofb", "[ getCurrentConfiguration fb:0x%llx", (UInt64)service);
	iofbVars->iofbvtable->orggetCurrentConfiguration( service );
	DBGLOG("iofb", "] getCurrentConfiguration");
}

IOReturn IOFB::wrapdoControl( IONDRVFramebuffer *service, UInt32 code, void * params )
{
	IOFBVars *iofbVars = callbackIOFB->getIOFBVars(service);
	DBGLOG("iofb", "[ doControl fb:0x%llx code:%d", (UInt64)service, code);
	IOReturn result = iofbVars->iofbvtable->orgdoControl( service, code, params );
	#ifdef DEBUG
	char resultStr[40];
	#endif
	DBGLOG("iofb", "] doControl%s", DumpOneReturn(resultStr, sizeof(resultStr), result));
	return result;
}

IOReturn IOFB::wrapdoStatus( IONDRVFramebuffer *service, UInt32 code, void * params )
{
	IOFBVars *iofbVars = callbackIOFB->getIOFBVars(service);
	DBGLOG("iofb", "[ doStatus fb:0x%llx code:%d", (UInt64)service, code);
	IOReturn result = iofbVars->iofbvtable->orgdoStatus( service, code, params );
	#ifdef DEBUG
	char resultStr[40];
	#endif
	DBGLOG("iofb", "] doStatus%s", DumpOneReturn(resultStr, sizeof(resultStr), result));
	return result;
}

IODeviceMemory * IOFB::wrapmakeSubRange( IONDRVFramebuffer *service, IOPhysicalAddress64 start, IOPhysicalLength64  length )
{
	IOFBVars *iofbVars = callbackIOFB->getIOFBVars(service);
	DBGLOG("iofb", "[ makeSubRange fb:0x%llx start:0x%llx length:0x%llx", (UInt64)service, start, length);
	IODeviceMemory * result = iofbVars->iofbvtable->orgmakeSubRange( service, start, length );
	DBGLOG("iofb", "] makeSubRange result:0x%llx", (UInt64)result);
	return result;
}

IODeviceMemory * IOFB::wrapfindVRAM( IONDRVFramebuffer *service )
{
	IOFBVars *iofbVars = callbackIOFB->getIOFBVars(service);
	DBGLOG("iofb", "[ findVRAM fb:0x%llx", (UInt64)service);
	IODeviceMemory * result = iofbVars->iofbvtable->orgfindVRAM( service );
	DBGLOG("iofb", "] findVRAM result:0x%llx", (UInt64)result);
	return result;
}

IOTVector * IOFB::wrapundefinedSymbolHandler( IONDRVFramebuffer *service, const char * libraryName, const char * symbolName )
{
	IOFBVars *iofbVars = callbackIOFB->getIOFBVars(service);
	DBGLOG("iofb", "[ undefinedSymbolHandler fb:0x%llx \"%s\" \"%s\"", (UInt64)service, libraryName, symbolName);
	IOTVector * result = iofbVars->iofbvtable->orgundefinedSymbolHandler( service, libraryName, symbolName );
	DBGLOG("iofb", "] undefinedSymbolHandler result:0x%llx", (UInt64)result);
	return result;
}


//========================================================================================
