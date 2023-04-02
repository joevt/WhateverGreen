//
//  kern_igfx_debug.cpp
//  WhateverGreen
//
//  Copyright © 2020 vit9696. All rights reserved.
//

#include <Headers/kern_patcher.hpp>
#include <Headers/kern_devinfo.hpp>
#include <Headers/kern_cpu.hpp>
#include <IOKit/IOService.h>
#include <IOKit/graphics/IOFramebuffer.h>
#include "kern_agdc.hpp"
#include "kern_igfx.hpp"
#include "kern_iofbdebug.hpp"

#ifdef DEBUG

static mach_vm_address_t fbdebugOrgEnableController;
static mach_vm_address_t fbdebugOrgGetAttribute;
static mach_vm_address_t fbdebugOrgSetAttribute;
static mach_vm_address_t fbdebugOrgSetDisplayMode;
static mach_vm_address_t fbdebugOrgConnectionProbe;
static mach_vm_address_t fbdebugOrgGetDisplayStatus;
static mach_vm_address_t fbdebugOrgGetOnlineInfo;
static mach_vm_address_t fbdebugOrgDoSetPowerState;
static mach_vm_address_t fbdebugOrgValidateDisplayMode;
static mach_vm_address_t fbdebugOrgIsMultilinkDisplay;
static mach_vm_address_t fbdebugOrgHasExternalDisplay;
static mach_vm_address_t fbdebugOrgSetDPPowerState;
static mach_vm_address_t fbdebugOrgSetDisplayPipe;
static mach_vm_address_t fbdebugOrgSetFBMemory;
static mach_vm_address_t fbdebugOrgFBClientDoAttribute;

// Check these, 10.15.4 changed their logic?
// AppleIntelFramebuffer::getPixelInformation
// AppleIntelFramebuffer::populateFBState

static IOReturn fbdebugWrapEnableController(IOService *framebuffer) {
	auto idxnum = OSDynamicCast(OSNumber, framebuffer->getProperty("IOFBDependentIndex"));
	int idx = (idxnum != nullptr) ? (int) idxnum->unsigned32BitValue() : -1;
	SYSLOG("igfx", "[ enableController %d", idx);
	IOReturn ret = FunctionCast(fbdebugWrapEnableController, fbdebugOrgEnableController)(framebuffer);
	char resultStr[40];
	SYSLOG("igfx", "] enableController %d%s", idx, DumpOneReturn(resultStr, sizeof(resultStr), ret));
	return ret;
}

static IOReturn fbdebugWrapGetAttribute(IOService *framebuffer, IOIndex connectIndex, IOSelect attribute, uintptr_t * value) {
	auto idxnum = OSDynamicCast(OSNumber, framebuffer->getProperty("IOFBDependentIndex"));
	int idx = (idxnum != nullptr) ? (int) idxnum->unsigned32BitValue() : -1;
	UInt32 attributeChar = IOFBAttribChars(attribute);
	const char *attributeStr = IOFBGetAttributeName(attribute, true);
	SYSLOG("igfx", "[ getAttributeForConnection %d %d 0x%x=\'%.4s\'%s%s (non-null:%s)", idx, connectIndex, attribute, (char*)&attributeChar, attributeStr ? "=" : "", attributeStr ? attributeStr : "", value ? "true" : "false");
	IOReturn ret = FunctionCast(fbdebugWrapGetAttribute, fbdebugOrgGetAttribute)(framebuffer, connectIndex, attribute, value);
	char resultStr[40];
	SYSLOG("igfx", "] getAttributeForConnection %d %d 0x%x=\'%.4s\'%s%s%s / %lx", idx, connectIndex, attribute, (char*)&attributeChar, attributeStr ? "=" : "", attributeStr ? attributeStr : "", DumpOneReturn(resultStr, sizeof(resultStr), ret), value ? *value : 0);
	return ret;
}

static IOReturn fbdebugWrapSetAttribute(IOService *framebuffer, IOIndex connectIndex, IOSelect attribute, uintptr_t value) {
	auto idxnum = OSDynamicCast(OSNumber, framebuffer->getProperty("IOFBDependentIndex"));
	int idx = (idxnum != nullptr) ? (int) idxnum->unsigned32BitValue() : -1;
	UInt32 attributeChar = IOFBAttribChars(attribute);
	const char *attributeStr = IOFBGetAttributeName(attribute, true);
	SYSLOG("igfx", "[ setAttributeForConnection %d %d 0x%x=\'%.4s\'%s%s -> %lx", idx, connectIndex, attribute, (char*)&attributeChar, attributeStr ? "=" : "", attributeStr ? attributeStr : "", value);
	IOReturn ret = FunctionCast(fbdebugWrapSetAttribute, fbdebugOrgSetAttribute)(framebuffer, connectIndex, attribute, value);
	char resultStr[40];
	SYSLOG("igfx", "] setAttributeForConnection %d %d 0x%x=\'%.4s\'%s%s -> %lx%s", idx, connectIndex, attribute, (char*)&attributeChar, attributeStr ? "=" : "", attributeStr ? attributeStr : "", value, DumpOneReturn(resultStr, sizeof(resultStr), ret));
	return ret;
}

static IOReturn fbdebugWrapSetDisplayMode(IOService *framebuffer, IODisplayModeID displayMode, IOIndex depth) {
	auto idxnum = OSDynamicCast(OSNumber, framebuffer->getProperty("IOFBDependentIndex"));
	int idx = (idxnum != nullptr) ? (int) idxnum->unsigned32BitValue() : -1;
	SYSLOG("igfx", "[ setDisplayMode %d 0x%x depth:%d", idx, displayMode, depth);
	IOReturn ret = FunctionCast(fbdebugWrapSetDisplayMode, fbdebugOrgSetDisplayMode)(framebuffer, displayMode, depth);
	char resultStr[40];
	SYSLOG("igfx", "] setDisplayMode %d 0x%x depth:%d%s", idx, displayMode, depth, DumpOneReturn(resultStr, sizeof(resultStr), ret));
	return ret;
}

static IOReturn fbdebugWrapConnectionProbe(IOService *framebuffer, uint8_t unk1, uint8_t unk2) {
	auto idxnum = OSDynamicCast(OSNumber, framebuffer->getProperty("IOFBDependentIndex"));
	int idx = (idxnum != nullptr) ? (int) idxnum->unsigned32BitValue() : -1;
	SYSLOG("igfx", "[ connectionProbe %d 0x%x 0x%x", idx, unk1, unk2);
	IOReturn ret = FunctionCast(fbdebugWrapConnectionProbe, fbdebugOrgConnectionProbe)(framebuffer, unk1, unk2);
	char resultStr[40];
	SYSLOG("igfx", "] connectionProbe %d 0x%x 0x%x%s", idx, unk1, unk2, DumpOneReturn(resultStr, sizeof(resultStr), ret));
	return ret;
}

static uint32_t fbdebugWrapGetDisplayStatus(IOService *framebuffer, void *displayPath) {
	auto idxnum = OSDynamicCast(OSNumber, framebuffer->getProperty("IOFBDependentIndex"));
	int idx = (idxnum != nullptr) ? (int) idxnum->unsigned32BitValue() : -1;
	SYSLOG("igfx", "[ getDisplayStatus %d", idx);
	uint32_t ret = FunctionCast(fbdebugWrapGetDisplayStatus, fbdebugOrgGetDisplayStatus)(framebuffer, displayPath);
	SYSLOG("igfx", "] getDisplayStatus %d - %u", idx, ret);
	//FIXME: This is just a hack.
	SYSLOG("igfx", "[HACK] forcing STATUS 1");
	ret = 1;
	return ret;
}

static IOReturn fbdebugWrapGetOnlineInfo(IOService *framebuffer, void *displayPath, uint8_t *displayConnected, uint8_t *edid, void *displayPortType, bool *unk1, bool unk2) {
	auto idxnum = OSDynamicCast(OSNumber, framebuffer->getProperty("IOFBDependentIndex"));
	int idx = (idxnum != nullptr) ? (int) idxnum->unsigned32BitValue() : -1;
	SYSLOG("igfx", "[ GetOnlineInfo %d %d", idx, unk2);
	IOReturn ret = FunctionCast(fbdebugWrapGetOnlineInfo, fbdebugOrgGetOnlineInfo)(framebuffer, displayPath, displayConnected, edid, displayPortType, unk1, unk2);
	char resultStr[40];
	SYSLOG("igfx", "] GetOnlineInfo %d %d -> 0x%x%s", idx, unk2, displayConnected ? *displayConnected : (uint32_t)-1, DumpOneReturn(resultStr, sizeof(resultStr), ret));
	return ret;
}

static IOReturn fbdebugWrapDoSetPowerState(IOService *framebuffer, uint32_t state) {
	// state 0 = sleep, 1 = wake, 2 = doze, cap at doze if higher.
	auto idxnum = OSDynamicCast(OSNumber, framebuffer->getProperty("IOFBDependentIndex"));
	int idx = (idxnum != nullptr) ? (int) idxnum->unsigned32BitValue() : -1;
	SYSLOG("igfx", "[ doSetPowerState %d %u", idx, state);
	IOReturn ret = FunctionCast(fbdebugWrapDoSetPowerState, fbdebugOrgDoSetPowerState)(framebuffer, state);
	char resultStr[40];
	SYSLOG("igfx", "] doSetPowerState %d %u%s", idx, state, DumpOneReturn(resultStr, sizeof(resultStr), ret));
	return ret;
}

static bool fbdebugWrapIsMultilinkDisplay(IOService *framebuffer) {
	bool ret = FunctionCast(fbdebugWrapIsMultilinkDisplay, fbdebugOrgIsMultilinkDisplay)(framebuffer);
	SYSLOG("igfx", "[] IsMultiLinkDisplay %s", ret ? "true" : "false");
	return ret;
}

static IOReturn fbdebugWrapValidateDisplayMode(IOService *framebuffer, uint32_t mode, void const **modeDescription, IODetailedTimingInformationV2 **timing) {
	auto idxnum = OSDynamicCast(OSNumber, framebuffer->getProperty("IOFBDependentIndex"));
	int idx = (idxnum != nullptr) ? (int) idxnum->unsigned32BitValue() : -1;
	SYSLOG("igfx", "[ validateDisplayMode %d 0x%x", idx, mode);
	IOReturn ret = FunctionCast(fbdebugWrapValidateDisplayMode, fbdebugOrgValidateDisplayMode)(framebuffer, mode, modeDescription, timing);
	int w = -1, h = -1;
	if (ret == kIOReturnSuccess && timing && timing) {
		w = (int)(*timing)->horizontalActive;
		h = (int)(*timing)->verticalActive;
	}
	char resultStr[40];
	SYSLOG("igfx", "] validateDisplayMode %d 0x%x -> %d/%d%s", idx, mode, w, h, DumpOneReturn(resultStr, sizeof(resultStr), ret));
	// Mostly comes from AppleIntelFramebufferController::hwSetCursorState, which itself comes from deferredMoveCursor/showCursor.
	// SYSTRACE("igfx", "validateDisplayMode trace");
	return ret;
}

static bool fbdebugWrapHasExternalDisplay(IOService *controller) {
	bool ret = FunctionCast(fbdebugWrapHasExternalDisplay, fbdebugOrgHasExternalDisplay)(controller);
	SYSLOG("igfx", "[] hasExternalDisplay %s", ret ? "true" : "false");
	return ret;
}

static IOReturn fbdebugWrapSetDPPowerState(IOService *controller, IOService *framebuffer, bool status, void *displayPath) {
	auto idxnum = OSDynamicCast(OSNumber, framebuffer->getProperty("IOFBDependentIndex"));
	int idx = (idxnum != nullptr) ? (int) idxnum->unsigned32BitValue() : -1;
	SYSLOG("igfx", "[ SetDPPowerState %d %d", idx, status);
	IOReturn ret = FunctionCast(fbdebugWrapSetDPPowerState, fbdebugOrgSetDPPowerState)(controller, framebuffer, status, displayPath);
	char resultStr[40];
	SYSLOG("igfx", "] SetDPPowerState %d %d%s", idx, status, DumpOneReturn(resultStr, sizeof(resultStr), ret));
	return ret;
}

static bool fbdebugWrapSetDisplayPipe(IOService *controller, void *displayPath) {
	SYSLOG("igfx", "[ setDisplayPipe");
	bool ret = FunctionCast(fbdebugWrapSetDisplayPipe, fbdebugOrgSetDisplayPipe)(controller, displayPath);
	SYSLOG("igfx", "] setDisplayPipe - %s", ret ? "true" : "false");
	return ret;
}

static bool fbdebugWrapSetFBMemory(IOService *controller, IOService *framebuffer) {
	auto idxnum = OSDynamicCast(OSNumber, framebuffer->getProperty("IOFBDependentIndex"));
	int idx = (idxnum != nullptr) ? (int) idxnum->unsigned32BitValue() : -1;
	SYSLOG("igfx", "[ setFBMemory %d", idx);
	bool ret = FunctionCast(fbdebugWrapSetFBMemory, fbdebugOrgSetFBMemory)(controller, framebuffer);
	SYSLOG("igfx", "] setFBMemory %d - %s", idx, ret ? "true" : "false");
	return ret;
}

static IOReturn fbdebugWrapFBClientDoAttribute(void *fbclient, uint32_t attribute, unsigned long* unk1, unsigned long unk2, unsigned long* unk3, unsigned long* unk4, void* externalMethodArguments) {
	//FIXME: we are just getting rid of AGDC.
	if (attribute == kAGDCRegisterCallback) {
		SYSLOG("igfx", "[HACK] FBClientDoAttribute -> disabling AGDC!");
		return kIOReturnUnsupported;
	}

	const char *attributeStr = AGDCGetAttributeName(attribute);
	SYSLOG("igfx", "[ IntelFBClientControl::doAttribute 0x%x%s%s", attribute, attributeStr ? "=" : "", attributeStr ? attributeStr : "");
	IOReturn ret = FunctionCast(fbdebugWrapFBClientDoAttribute, fbdebugOrgFBClientDoAttribute)(fbclient, attribute, unk1, unk2, unk3, unk4, externalMethodArguments);
	char resultStr[40];
	SYSLOG("igfx", "] IntelFBClientControl::doAttribute%s", DumpOneReturn(resultStr, sizeof(resultStr), ret));

	return ret;
}

void IGFX::FramebufferDebugSupport::processFramebufferKext(KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size) {
	SYSLOG("igfx", "using framebuffer debug r15");

	if (callbackIGFX->modAGDCDisabler.enabled)
		PANIC("igfx", "igfxagdc=0 is not compatible with framebuffer debugging");

	KernelPatcher::RouteRequest requests[] = {
		{"__ZN21AppleIntelFramebuffer16enableControllerEv", fbdebugWrapEnableController, fbdebugOrgEnableController},
		{"__ZN21AppleIntelFramebuffer25setAttributeForConnectionEijm", fbdebugWrapSetAttribute, fbdebugOrgSetAttribute},
		{"__ZN21AppleIntelFramebuffer25getAttributeForConnectionEijPm", fbdebugWrapGetAttribute, fbdebugOrgGetAttribute},
		{"__ZN21AppleIntelFramebuffer14setDisplayModeEii", fbdebugWrapSetDisplayMode, fbdebugOrgSetDisplayMode},
		{"__ZN21AppleIntelFramebuffer15connectionProbeEjj", fbdebugWrapConnectionProbe, fbdebugOrgConnectionProbe},
		{"__ZN21AppleIntelFramebuffer16getDisplayStatusEP21AppleIntelDisplayPath", fbdebugWrapGetDisplayStatus, fbdebugOrgGetDisplayStatus},
		{"__ZN21AppleIntelFramebuffer13GetOnlineInfoEP21AppleIntelDisplayPathPhS2_PNS0_15DisplayPortTypeEPbb", fbdebugWrapGetOnlineInfo, fbdebugOrgGetOnlineInfo},
		{"__ZN21AppleIntelFramebuffer15doSetPowerStateEj", fbdebugWrapDoSetPowerState, fbdebugOrgDoSetPowerState},
		{"__ZN21AppleIntelFramebuffer18IsMultiLinkDisplayEv", fbdebugWrapIsMultilinkDisplay, fbdebugOrgIsMultilinkDisplay},
		{"__ZN21AppleIntelFramebuffer19validateDisplayModeEiPPKNS_15ModeDescriptionEPPK29IODetailedTimingInformationV2", fbdebugWrapValidateDisplayMode, fbdebugOrgValidateDisplayMode},
		{"__ZN31AppleIntelFramebufferController18hasExternalDisplayEv", fbdebugWrapHasExternalDisplay, fbdebugOrgHasExternalDisplay},
		{"__ZN31AppleIntelFramebufferController15SetDPPowerStateEP21AppleIntelFramebufferhP21AppleIntelDisplayPath", fbdebugWrapSetDPPowerState, fbdebugOrgSetDPPowerState},
		{"__ZN31AppleIntelFramebufferController14setDisplayPipeEP21AppleIntelDisplayPath", fbdebugWrapSetDisplayPipe, fbdebugOrgSetDisplayPipe},
		{"__ZN31AppleIntelFramebufferController11setFBMemoryEP21AppleIntelFramebuffer", fbdebugWrapSetFBMemory, fbdebugOrgSetFBMemory},
		{"__ZN20IntelFBClientControl11doAttributeEjPmmS0_S0_P25IOExternalMethodArguments", fbdebugWrapFBClientDoAttribute, fbdebugOrgFBClientDoAttribute},
	};

	if (!patcher.routeMultiple(index, requests, address, size, true, true))
		SYSLOG("igfx", "DBG: Failed to route igfx tracing.");
}

#else
void IGFX::FramebufferDebugSupport::processFramebufferKext(KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size) {
	PANIC("igfx", "DBG: Framebuffer debug support is only available in debug build.");
}
#endif

void IGFX::FramebufferDebugSupport::init() {
	// We only need to patch the framebuffer driver
	requiresPatchingFramebuffer = true;
}

void IGFX::FramebufferDebugSupport::processKernel(KernelPatcher &patcher, DeviceInfo *info) {
#ifdef DEBUG
	enabled = checkKernelArgument("-igfxfbdbg");
#endif
}
