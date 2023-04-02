//
//  kern_nvmtl.hpp
//  WhateverGreen
//
//  Copyright © 2022 joevt. All rights reserved.
//

#ifndef kern_nvmtl_hpp
#define kern_nvmtl_hpp

#include <Headers/kern_patcher.hpp>
#include <Headers/kern_devinfo.hpp>
#include <Headers/kern_user.hpp>

class NVMTL {
public:
	void init();
	void deinit();

	void processKernel(KernelPatcher &patcher, DeviceInfo *info);

	bool processKext(KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size);
};

#endif /* kern_nvmtl_hpp */
