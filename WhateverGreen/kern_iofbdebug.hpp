//
//  kern_iofbdebug.hpp
//  WhateverGreen
//
//  Created by joevt on 2022-04-23.
//  Copyright © 2022 vit9696. All rights reserved.
//

#ifndef kern_iofbdebug_hpp
#define kern_iofbdebug_hpp

#include <Headers/kern_iokit.hpp>
#include <IOKit/ndrvsupport/IONDRVFramebuffer.h>
#include "kern_agdc.hpp"

char * DumpOneDetailedTimingInformationPtr(char * buf, size_t bufSize, const void * IOFBDetailedTiming, size_t timingSize);
char * DumpOneReturn(char * buf, size_t bufSize, IOReturn val);
UInt32 IOFBAttribChars(UInt32 attribute);
const char * IOFBGetAttributeName(UInt32 attribute, bool forConnection);
const char * AGDCGetAttributeName(UInt32 attribute);


class IOFB {
public:
	void init();
	void deinit();

	/**
	 *  Property patching routine
	 *
	 *  @param patcher  KernelPatcher instance
	 *  @param info     device info
	 */
	void processKernel(KernelPatcher &patcher, DeviceInfo *info);

	/**
	 *  Patch kext if needed and prepare other patches
	 *
	 *  @param patcher KernelPatcher instance
	 *  @param index   kinfo handle
	 *  @param address kinfo load address
	 *  @param size    kinfo memory size
	 *
	 *  @return true if patched anything
	 */
	bool processKext(KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size);

private:

	/**
	 *  IOFramebuffer vtable
	 */
	#define onevtableitem(_patch, _index, _result, _name, _params) using t_ ## _name = _result (*) _params;
	#include "IOFramebuffer_vtable.hpp"

	class IOFBvtable {
		public:
			uintptr_t vtable = 0;
			#define onevtableitem(_patch, _index, _result, _name, _params) t_ ## _name org ## _name { nullptr };
			#include "IOFramebuffer_vtable.hpp"
	};
	typedef IOFBvtable* IOFBvtablePtr;
	IOFBvtablePtr *iofbvtables;
	SInt32 iofbvtablesCount = 0;
	SInt32 iofbvtablesMaxCount = 0;

	static IOFBvtable *getIOFBvtable(IOFramebuffer *service);

	struct IOFramebuffer_vtableIndex { enum : size_t {
		#define onevtableitem(_patch, _index, _result, _name, _params) _name _index ,
		#include "IOFramebuffer_vtable.hpp"
	}; };

	#define onevtableitem(_patch, _index, _result, _name, _params) static _result wrap ## _name _params;
	#include "IOFramebuffer_vtable.hpp"

	mach_vm_address_t orgFramebufferInit {};
	static void wrapFramebufferInit(IOFramebuffer *fb);


	/**
	 *  AppleGraphicsDeviceControl vtable
	 */
	#define onevtableitem(_patch, _index, _result, _name, _params) using t_ ## _name = _result (*) _params;
	#include "AppleGraphicsDeviceControl_vtable.hpp"

	class AGDCvtable {
		public:
			uintptr_t vtable = 0;
			#define onevtableitem(_patch, _index, _result, _name, _params) t_ ## _name org ## _name { nullptr };
			#include "AppleGraphicsDeviceControl_vtable.hpp"
	};
	typedef AGDCvtable* AGDCvtablePtr;
	AGDCvtablePtr *agdcvtables;
	SInt32 agdcvtablesCount = 0;
	SInt32 agdcvtablesMaxCount = 0;

	static AGDCvtable *getAGDCvtable(IOService *service);

	struct AppleGraphicsDeviceControl_vtableIndex { enum : size_t {
		#define onevtableitem(_patch, _index, _result, _name, _params) _name _index ,
		#include "AppleGraphicsDeviceControl_vtable.hpp"
	}; };

	#define onevtableitem(_patch, _index, _result, _name, _params) static _result wrap ## _name _params;
	#include "AppleGraphicsDeviceControl_vtable.hpp"

	mach_vm_address_t orgAppleGraphicsDeviceControlstart {};
	static void wrapAppleGraphicsDeviceControlstart(IOService* agdc);


	/**
	 *  EDID override
	 */
	class IOFBEDIDOverride {
		public:
			/**
			 *  an EDID Override
			 */
			UInt8 *orgEDID;
			UInt32 orgSize;
			UInt8 *newEDID;
			UInt32 newSize;
	};
	IOFBEDIDOverride *iofbEDIDOverrides { nullptr };
	SInt32 iofbedidCount = 0;
	SInt32 iofbedidMaxCount = 0;

	void setEDIDOverride(UInt8 *orgEDID, UInt32 orgSize, UInt8 *newEDID, UInt32 newSize);


	/**
	 *  IOFB settings per IOFramebuffer service
	 */
	class IOFBVars {
		public:
			IOFramebuffer *fb = NULL;
			IOFBvtable *iofbvtable = NULL;
			int index = 0;

			bool iofbValidateAll = false;
			bool iofbSidebandDownRep = false;
#if 0
			int iofbDumpsetDetailedTimings = 1; // 2
			int iofbDumpgetPixelInformation = 1; // 0
			int iofbDumpvalidateDetailedTiming = 1; // 0
			int iofbDumpdoI2CRequest = 1; // 0
			int iofbDumpAttributes = 1; // 0
#else
			int iofbDumpsetDetailedTimings = 2;
			int iofbDumpgetPixelInformation = 0;
			int iofbDumpvalidateDetailedTiming = 0;
			int iofbDumpdoI2CRequest = 0;
			int iofbDumpAttributes = 0;
#endif
			// 0 = no
			// 1 = yes
			// 2 = only if error

			UInt32 iofbColorModesSupported = 0; // set (get)
			UInt32 iofbControllerDepthsSupported = 0; // get only
			UInt32 iofbColorDepthsSupported = 0; // set only
			// 0 = don't override
			// -1 = all bits

			IOFBEDIDOverride *edidOverride = NULL;
	};
	typedef IOFBVars* IOFBVarsPtr;
	IOFBVarsPtr *iofbvars;
	SInt32 iofbvarsCount = 0;
	SInt32 iofbvarsMaxCount = 0;

	static IOFBVars *getIOFBVars(IOFramebuffer *service);


	/**
	 *  AGDC settings per AppleGraphicsDeviceControl service
	 */
	class AGDCVars {
		public:
			IOService *agdc = NULL; // AppleGraphicsDeviceControl*
			AGDCvtable *agdcvtable = NULL;
			int index = 0;
	};
	typedef AGDCVars* AGDCVarsPtr;
	AGDCVarsPtr *agdcvars;
	SInt32 agdcvarsCount = 0;
	SInt32 agdcvarsMaxCount = 0;

	static AGDCVars *getAGDCVars(IOService *service);


	/**
	 *  Private self instance for callbacks
	 */
	static IOFB *callbackIOFB;

	/**
	 *  gIOGATFlags, imported from the framebuffer kext, defined in IOGraphicsFamily/IOFramebuffer.cpp
	 */
	uint32_t *gIOGATFlagsPtr {nullptr};


	/**
	 *  Miscellaneous functions
	 */
	static void Dump_doDeviceAttribute( IOReturn result, bool isstart, bool istwo, IOFB::AGDCVars *agdcVars, uint32_t attribute, unsigned long* structIn, unsigned long structInSize, unsigned long* structOut, unsigned long* structOutSize, void* es );
	static char * DumpOneDisplayParameters(char * buf, size_t bufSize, uintptr_t * value, uintptr_t numParameters);
	void UpdateAttribute( IOFramebuffer *service, bool set, IOIndex connectIndex, IOSelect attribute, IOReturn result, uintptr_t * value, unsigned int size);
};

#endif /* kern_iofbdebug_hpp */
