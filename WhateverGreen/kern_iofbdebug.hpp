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
	 *  IOFramebuffer vtable function types
	 */
	#define onevtableitem(_patch, _index, _result, _name, _params) using t_ ## _name = _result (*) _params;
	#include "IOFramebuffer_vtable.hpp"

	/**
	 *  Variables per IOFramebuffer service
	 */
	class IOFBvtable {
		public:
			uintptr_t vtable = 0;
			#define onevtableitem(_patch, _index, _result, _name, _params) t_ ## _name org ## _name { nullptr };
			#include "IOFramebuffer_vtable.hpp"
	};

	class IOFBVars {
		public:
			/**
			 *  IOFB settings per IOFramebuffer service
			 */
			IOFramebuffer *fb = NULL;
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

			IOFBvtable *iofbvtable = NULL;;
	};
	typedef IOFBVars* IOFBVarsPtr;
	typedef IOFBvtable* IOFBvtablePtr;
	IOFBVarsPtr *iofbvars;
	IOFBvtablePtr *iofbvtables;
	SInt32 iofbvarsCount = 0;
	SInt32 iofbvtablesCount = 0;

	/**
	 *  Private self instance for callbacks
	 */
	static IOFB *callbackIOFB;

	/**
	 *  Each IOFramebuffer has it's own IOFB settings
	 */
	static IOFBVars *getIOFBVars(IOFramebuffer *service);

	/**
	 *  A vtable is shared by multiple framebuffers of the same class
	 */
	static IOFBvtable *getIOFBvtable(IOFramebuffer *service);


	/**
	 *  Original IOGraphics framebuffer init handler
	 */
	mach_vm_address_t orgFramebufferInit {};

	/**
	 *  IOFramebuffer initialisation wrapper used for screen distortion fixes
	 *
	 *  @param fb  framebuffer instance
	 */
	static void wrapFramebufferInit(IOFramebuffer *fb);


	/**
	 *  Disable the patches based on -iofboff boot-arg
	 */
	bool disableIOFB = false;

	/**
	 *  gIOGATFlags, imported from the framebuffer kext, defined in IOGraphicsFamily/IOFramebuffer.cpp
	 */
	uint32_t *gIOGATFlagsPtr {nullptr};


	struct IOFramebuffer_vtableIndex { enum : size_t {
#define onevtableitem(_patch, _index, _result, _name, _params) _name _index ,
#include "IOFramebuffer_vtable.hpp"
	}; };
#define onevtableitem(_patch, _index, _result, _name, _params) static _result wrap ## _name _params;
#include "IOFramebuffer_vtable.hpp"


	static char * DumpOneDisplayParameters(char * buf, size_t bufSize, uintptr_t * value, uintptr_t numParameters);
	void UpdateAttribute( IOFramebuffer *service, bool set, IOIndex connectIndex, IOSelect attribute, IOReturn result, uintptr_t * value, unsigned int size);
};

#endif /* kern_iofbdebug_hpp */
