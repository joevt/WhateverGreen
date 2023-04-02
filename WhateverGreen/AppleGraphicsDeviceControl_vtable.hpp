//
//  AppleGraphicsDeviceControl_vtable.hpp
//  WhateverGreen
//
//  Created by joevt on 2022-07-25.
//  Copyright © 2022 vit9696. All rights reserved.
//

// AppleGraphicsDeviceControl
		onevtableitem( 1 , = 266, IOReturn, vendor_doDeviceAttribute , ( IOService *fbclient, uint32_t attribute, unsigned long* unk1, unsigned long unk2, unsigned long* unk3, unsigned long* unk4, IOExternalMethodArguments* externalMethodArguments ) )
		onevtableitem( 1 ,      , IOReturn, vendor_doDeviceAttribute2, ( IOService *fbclient, uint32_t attribute, unsigned long* unk1, unsigned long unk2, unsigned long* unk3, unsigned long* unk4, AGDCClientState_t* agdcClientState ) )

#undef onevtableitem
#undef onevtableitem0
#undef onevtableitem1
