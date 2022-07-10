//
//  kern_dpd.hpp
//  WhateverGreen
//
//  Created by joevt on 2022-06-18.
//  Copyright © 2022 vit9696. All rights reserved.
//

#ifndef kern_dpd_hpp
#define kern_dpd_hpp

#include <Headers/kern_patcher.hpp>
#include <Headers/kern_devinfo.hpp>
#include <Headers/kern_user.hpp>

class DPD {
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
	 *  Private self instance for callbacks
	 */
	static DPD *callbackDPD;
};

#endif /* kern_dpd_hpp */
