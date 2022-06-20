//
//  IOFramebuffer_vtable.hpp
//  WhateverGreen
//
//  Created by joevt on 2022-04-23.
//  Copyright © 2022 vit9696. All rights reserved.
//

// IOGraphicsDevice
		onevtableitem( 0 , = 266, void             , hideCursor                    , ( IORegistryEntry *service ) )
		onevtableitem( 0 ,      , void             , showCursor                    , ( IORegistryEntry *service, IOGPoint * cursorLoc, int frame ) )
		onevtableitem( 0 ,      , void             , moveCursor                    , ( IORegistryEntry *service, IOGPoint * cursorLoc, int frame ) )
		onevtableitem( 0 ,      , void             , getVBLTime                    , ( IORegistryEntry *service, AbsoluteTime * time, AbsoluteTime * delta ) )
		onevtableitem( 0 ,      , void             , getBoundingRect               , ( IORegistryEntry *service, IOGBounds ** bounds ) ) // called everytime you move the mouse
// IOFramebuffer
		onevtableitem( 1 , = 271, IOReturn         , doI2CRequest                  , ( IORegistryEntry *service, UInt32 bus, struct IOI2CBusTiming * timing, struct IOI2CRequest * request ) )
		onevtableitem( 1 ,      , IOReturn         , diagnoseReport                , ( IORegistryEntry *service, void * param1, void * param2, void * param3, void * param4 ) )
		onevtableitem( 1 ,      , IOReturn         , setGammaTable                 , ( IORegistryEntry *service, UInt32 channelCount, UInt32 dataCount, UInt32 dataWidth, void * data, bool syncToVBL ) )
		onevtableitem( 1 , = 303, IOReturn         , open                          , ( IORegistryEntry *service ) )
		onevtableitem( 1 ,      , void             , close                         , ( IORegistryEntry *service ) )
		onevtableitem( 1 ,      , bool             , isConsoleDevice               , ( IORegistryEntry *service ) )
		onevtableitem( 1 ,      , IOReturn         , setupForCurrentConfig         , ( IORegistryEntry *service ) )
		onevtableitem( 1 ,      , bool             , serializeInfo                 , ( IORegistryEntry *service, OSSerialize * s ) )
		onevtableitem( 1 ,      , bool             , setNumber                     , ( IORegistryEntry *service, OSDictionary * dict, const char * key, UInt32 number ) )
		onevtableitem( 1 ,      , IODeviceMemory * , getApertureRange              , ( IORegistryEntry *service, IOPixelAperture aperture ) )
		onevtableitem( 1 ,      , IODeviceMemory * , getVRAMRange                  , ( IORegistryEntry *service ) )
		onevtableitem( 1 ,      , IOReturn         , enableController              , ( IORegistryEntry *service ) )
		onevtableitem( 1 ,      , const char *     , getPixelFormats               , ( IORegistryEntry *service ) )
		onevtableitem( 1 ,      , IOItemCount      , getDisplayModeCount           , ( IORegistryEntry *service ) )
		onevtableitem( 1 ,      , IOReturn         , getDisplayModes               , ( IORegistryEntry *service, IODisplayModeID * allDisplayModes ) )
		onevtableitem( 1 ,      , IOReturn         , getInformationForDisplayMode  , ( IORegistryEntry *service, IODisplayModeID displayMode, IODisplayModeInformation * info ) )
		onevtableitem( 1 ,      , UInt64           , getPixelFormatsForDisplayMode , ( IORegistryEntry *service, IODisplayModeID displayMode, IOIndex depth ) )
		onevtableitem( 1 ,      , IOReturn         , getPixelInformation           , ( IORegistryEntry *service, IODisplayModeID displayMode, IOIndex depth, IOPixelAperture aperture, IOPixelInformation * pixelInfo ) )
		onevtableitem( 1 ,      , IOReturn         , getCurrentDisplayMode         , ( IORegistryEntry *service, IODisplayModeID * displayMode, IOIndex * depth ) )
		onevtableitem( 1 ,      , IOReturn         , setDisplayMode                , ( IORegistryEntry *service, IODisplayModeID displayMode, IOIndex depth ) )
		onevtableitem( 1 ,      , IOReturn         , setApertureEnable             , ( IORegistryEntry *service, IOPixelAperture aperture, IOOptionBits enable ) )
		onevtableitem( 1 ,      , IOReturn         , setStartupDisplayMode         , ( IORegistryEntry *service, IODisplayModeID displayMode, IOIndex depth ) )
		onevtableitem( 1 ,      , IOReturn         , getStartupDisplayMode         , ( IORegistryEntry *service, IODisplayModeID * displayMode, IOIndex * depth ) )
		onevtableitem( 1 ,      , IOReturn         , setCLUTWithEntries            , ( IORegistryEntry *service, IOColorEntry * colors, UInt32 index, UInt32 numEntries, IOOptionBits options ) )
		onevtableitem( 1 ,      , IOReturn         , setGammaTable2                , ( IORegistryEntry *service, UInt32 channelCount, UInt32 dataCount, UInt32 dataWidth, void * data ) )
		onevtableitem( 1 ,      , IOReturn         , setAttribute                  , ( IORegistryEntry *service, IOSelect attribute, uintptr_t value ) )
		onevtableitem( 1 ,      , IOReturn         , getAttribute                  , ( IORegistryEntry *service, IOSelect attribute, uintptr_t * value ) )
		onevtableitem( 1 ,      , IOReturn         , getTimingInfoForDisplayMode   , ( IORegistryEntry *service, IODisplayModeID displayMode, IOTimingInformation * info ) )
		onevtableitem( 1 ,      , IOReturn         , validateDetailedTiming        , ( IORegistryEntry *service, void * description, IOByteCount descripSize ) )
		onevtableitem( 1 ,      , IOReturn         , setDetailedTimings            , ( IORegistryEntry *service, OSArray * array ) )
		onevtableitem( 1 ,      , IOItemCount      , getConnectionCount            , ( IORegistryEntry *service ) )
		onevtableitem( 1 ,      , IOReturn         , setAttributeForConnection     , ( IORegistryEntry *service, IOIndex connectIndex, IOSelect attribute, uintptr_t value ) )
		onevtableitem( 1 ,      , IOReturn         , getAttributeForConnection     , ( IORegistryEntry *service, IOIndex connectIndex, IOSelect attribute, uintptr_t * value ) )
		onevtableitem( 0 ,      , bool             , convertCursorImage            , ( IORegistryEntry *service, void * cursorImage, IOHardwareCursorDescriptor * description, IOHardwareCursorInfo * cursor ) )
		onevtableitem( 0 ,      , IOReturn         , setCursorImage                , ( IORegistryEntry *service, void * cursorImage ) )
		onevtableitem( 0 ,      , IOReturn         , setCursorState                , ( IORegistryEntry *service, SInt32 x, SInt32 y, bool visible ) )
		onevtableitem( 0 ,      , void             , flushCursor                   , ( IORegistryEntry *service ) )
		onevtableitem( 1 ,      , IOReturn         , getAppleSense                 , ( IORegistryEntry *service, IOIndex connectIndex, UInt32 * senseType, UInt32 * primary, UInt32 * extended, UInt32 * displayType ) )
		onevtableitem( 1 ,      , IOReturn         , connectFlags                  , ( IORegistryEntry *service, IOIndex connectIndex, IODisplayModeID displayMode, IOOptionBits * flags ) )
		onevtableitem( 0 ,      , void             , setDDCClock                   , ( IORegistryEntry *service, IOIndex bus, UInt32 value ) )
		onevtableitem( 0 ,      , void             , setDDCData                    , ( IORegistryEntry *service, IOIndex bus, UInt32 value ) )
		onevtableitem( 0 ,      , bool             , readDDCClock                  , ( IORegistryEntry *service, IOIndex bus ) )
		onevtableitem( 0 ,      , bool             , readDDCData                   , ( IORegistryEntry *service, IOIndex bus ) )
		onevtableitem( 1 ,      , IOReturn         , enableDDCRaster               , ( IORegistryEntry *service, bool enable ) )
		onevtableitem( 1 ,      , bool             , hasDDCConnect                 , ( IORegistryEntry *service, IOIndex connectIndex ) )
		onevtableitem( 1 ,      , IOReturn         , getDDCBlock                   , ( IORegistryEntry *service, IOIndex connectIndex, UInt32 blockNumber, IOSelect blockType, IOOptionBits options, UInt8 * data, IOByteCount * length ) )
		onevtableitem( 1 ,      , IOReturn         , registerForInterruptType      , ( IORegistryEntry *service, IOSelect interruptType, IOFBInterruptProc proc, OSObject * target, void * ref, void ** interruptRef ) )
		onevtableitem( 1 ,      , IOReturn         , unregisterInterrupt           , ( IORegistryEntry *service, void * interruptRef ) )
		onevtableitem( 0 ,      , IOReturn         , setInterruptState             , ( IORegistryEntry *service, void * interruptRef, UInt32 state ) ) // this happens twice every 5 seconds
		onevtableitem( 1 ,      , IOReturn         , getNotificationSemaphore      , ( IORegistryEntry *service, IOSelect interruptType, semaphore_t * semaphore ) )
// IONDRVFramebuffer
		onevtableitem( 0 , = 350, IOReturn         , doDriverIO                    , ( IORegistryEntry *service, UInt32 commandID, void * contents, UInt32 commandCode, UInt32 commandKind ) )
		onevtableitem( 0 , = 382, IOReturn         , checkDriver                   , ( IORegistryEntry *service ) )
		onevtableitem( 1 ,      , UInt32           , iterateAllModes               , ( IORegistryEntry *service, IODisplayModeID * displayModeIDs ) )
		onevtableitem( 1 ,      , IOReturn         , getResInfoForMode             , ( IORegistryEntry *service, IODisplayModeID modeID, IODisplayModeInformation * theInfo ) )
		onevtableitem( 1 ,      , IOReturn         , getResInfoForArbMode          , ( IORegistryEntry *service, IODisplayModeID modeID, IODisplayModeInformation * theInfo ) )
		onevtableitem( 1 ,      , IOReturn         , validateDisplayMode           , ( IORegistryEntry *service, IODisplayModeID mode, IOOptionBits flags, VDDetailedTimingRec ** detailed ) )
		onevtableitem( 1 ,      , IOReturn         , setDetailedTiming             , ( IORegistryEntry *service, IODisplayModeID mode, IOOptionBits options, void * description, IOByteCount descripSize ) )
		onevtableitem( 1 ,      , void             , getCurrentConfiguration       , ( IORegistryEntry *service ) )
		onevtableitem( 0 ,      , IOReturn         , doControl                     , ( IORegistryEntry *service, UInt32 code, void * params ) )
		onevtableitem( 0 ,      , IOReturn         , doStatus                      , ( IORegistryEntry *service, UInt32 code, void * params ) )
		onevtableitem( 0 ,      , IODeviceMemory * , makeSubRange                  , ( IORegistryEntry *service, IOPhysicalAddress64 start, IOPhysicalLength64  length ) )
		onevtableitem( 0 ,      , IODeviceMemory * , findVRAM                      , ( IORegistryEntry *service ) )
		onevtableitem( 0 ,      , IOTVector *      , undefinedSymbolHandler        , ( IORegistryEntry *service, const char * libraryName, const char * symbolName ) )

#undef onevtableitem
#undef onevtableitem0
#undef onevtableitem1
