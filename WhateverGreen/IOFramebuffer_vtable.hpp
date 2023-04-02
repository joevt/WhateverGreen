//
//  IOFramebuffer_vtable.hpp
//  WhateverGreen
//
//  Created by joevt on 2022-04-23.
//  Copyright © 2022 vit9696. All rights reserved.
//

// IOGraphicsDevice
		onevtableitem( 0 , = 266, void             , hideCursor                    , ( IOFramebuffer *service ) )
		onevtableitem( 0 ,      , void             , showCursor                    , ( IOFramebuffer *service, IOGPoint * cursorLoc, int frame ) )
		onevtableitem( 0 ,      , void             , moveCursor                    , ( IOFramebuffer *service, IOGPoint * cursorLoc, int frame ) )
		onevtableitem( 0 ,      , void             , getVBLTime                    , ( IOFramebuffer *service, AbsoluteTime * time, AbsoluteTime * delta ) )
		onevtableitem( 0 ,      , void             , getBoundingRect               , ( IOFramebuffer *service, IOGBounds ** bounds ) ) // called everytime you move the mouse
// IOFramebuffer
		onevtableitem( 1 , = 271, IOReturn         , doI2CRequest                  , ( IOFramebuffer *service, UInt32 bus, struct IOI2CBusTiming * timing, struct IOI2CRequest * request ) )
		onevtableitem( 1 ,      , IOReturn         , diagnoseReport                , ( IOFramebuffer *service, void * param1, void * param2, void * param3, void * param4 ) )
		onevtableitem( 1 ,      , IOReturn         , setGammaTable                 , ( IOFramebuffer *service, UInt32 channelCount, UInt32 dataCount, UInt32 dataWidth, void * data, bool syncToVBL ) )
		onevtableitem( 1 , = 303, IOReturn         , open                          , ( IOFramebuffer *service ) )
		onevtableitem( 1 ,      , void             , close                         , ( IOFramebuffer *service ) )
		onevtableitem( 1 ,      , bool             , isConsoleDevice               , ( IOFramebuffer *service ) )
		onevtableitem( 1 ,      , IOReturn         , setupForCurrentConfig         , ( IOFramebuffer *service ) )
		onevtableitem( 1 ,      , bool             , serializeInfo                 , ( IOFramebuffer *service, OSSerialize * s ) )
		onevtableitem( 1 ,      , bool             , setNumber                     , ( IOFramebuffer *service, OSDictionary * dict, const char * key, UInt32 number ) )
		onevtableitem( 1 ,      , IODeviceMemory * , getApertureRange              , ( IOFramebuffer *service, IOPixelAperture aperture ) )
		onevtableitem( 1 ,      , IODeviceMemory * , getVRAMRange                  , ( IOFramebuffer *service ) )
		onevtableitem( 1 ,      , IOReturn         , enableController              , ( IOFramebuffer *service ) )
		onevtableitem( 1 ,      , const char *     , getPixelFormats               , ( IOFramebuffer *service ) )
		onevtableitem( 1 ,      , IOItemCount      , getDisplayModeCount           , ( IOFramebuffer *service ) )
		onevtableitem( 1 ,      , IOReturn         , getDisplayModes               , ( IOFramebuffer *service, IODisplayModeID * allDisplayModes ) )
		onevtableitem( 1 ,      , IOReturn         , getInformationForDisplayMode  , ( IOFramebuffer *service, IODisplayModeID displayMode, IODisplayModeInformation * info ) )
		onevtableitem( 1 ,      , UInt64           , getPixelFormatsForDisplayMode , ( IOFramebuffer *service, IODisplayModeID displayMode, IOIndex depth ) )
		onevtableitem( 1 ,      , IOReturn         , getPixelInformation           , ( IOFramebuffer *service, IODisplayModeID displayMode, IOIndex depth, IOPixelAperture aperture, IOPixelInformation * pixelInfo ) )
		onevtableitem( 1 ,      , IOReturn         , getCurrentDisplayMode         , ( IOFramebuffer *service, IODisplayModeID * displayMode, IOIndex * depth ) )
		onevtableitem( 1 ,      , IOReturn         , setDisplayMode                , ( IOFramebuffer *service, IODisplayModeID displayMode, IOIndex depth ) )
		onevtableitem( 1 ,      , IOReturn         , setApertureEnable             , ( IOFramebuffer *service, IOPixelAperture aperture, IOOptionBits enable ) )
		onevtableitem( 1 ,      , IOReturn         , setStartupDisplayMode         , ( IOFramebuffer *service, IODisplayModeID displayMode, IOIndex depth ) )
		onevtableitem( 1 ,      , IOReturn         , getStartupDisplayMode         , ( IOFramebuffer *service, IODisplayModeID * displayMode, IOIndex * depth ) )
		onevtableitem( 1 ,      , IOReturn         , setCLUTWithEntries            , ( IOFramebuffer *service, IOColorEntry * colors, UInt32 index, UInt32 numEntries, IOOptionBits options ) )
		onevtableitem( 1 ,      , IOReturn         , setGammaTable2                , ( IOFramebuffer *service, UInt32 channelCount, UInt32 dataCount, UInt32 dataWidth, void * data ) )
		onevtableitem( 1 ,      , IOReturn         , setAttribute                  , ( IOFramebuffer *service, IOSelect attribute, uintptr_t value ) )
		onevtableitem( 1 ,      , IOReturn         , getAttribute                  , ( IOFramebuffer *service, IOSelect attribute, uintptr_t * value ) )
		onevtableitem( 1 ,      , IOReturn         , getTimingInfoForDisplayMode   , ( IOFramebuffer *service, IODisplayModeID displayMode, IOTimingInformation * info ) )
		onevtableitem( 1 ,      , IOReturn         , validateDetailedTiming        , ( IOFramebuffer *service, void * description, IOByteCount descripSize ) )
		onevtableitem( 1 ,      , IOReturn         , setDetailedTimings            , ( IOFramebuffer *service, OSArray * array ) )
		onevtableitem( 1 ,      , IOItemCount      , getConnectionCount            , ( IOFramebuffer *service ) )
		onevtableitem( 1 ,      , IOReturn         , setAttributeForConnection     , ( IOFramebuffer *service, IOIndex connectIndex, IOSelect attribute, uintptr_t value ) )
		onevtableitem( 1 ,      , IOReturn         , getAttributeForConnection     , ( IOFramebuffer *service, IOIndex connectIndex, IOSelect attribute, uintptr_t * value ) )
		onevtableitem( 0 ,      , bool             , convertCursorImage            , ( IOFramebuffer *service, void * cursorImage, IOHardwareCursorDescriptor * description, IOHardwareCursorInfo * cursor ) )
		onevtableitem( 0 ,      , IOReturn         , setCursorImage                , ( IOFramebuffer *service, void * cursorImage ) )
		onevtableitem( 0 ,      , IOReturn         , setCursorState                , ( IOFramebuffer *service, SInt32 x, SInt32 y, bool visible ) )
		onevtableitem( 0 ,      , void             , flushCursor                   , ( IOFramebuffer *service ) )
		onevtableitem( 1 ,      , IOReturn         , getAppleSense                 , ( IOFramebuffer *service, IOIndex connectIndex, UInt32 * senseType, UInt32 * primary, UInt32 * extended, UInt32 * displayType ) )
		onevtableitem( 1 ,      , IOReturn         , connectFlags                  , ( IOFramebuffer *service, IOIndex connectIndex, IODisplayModeID displayMode, IOOptionBits * flags ) )
		onevtableitem( 0 ,      , void             , setDDCClock                   , ( IOFramebuffer *service, IOIndex bus, UInt32 value ) )
		onevtableitem( 0 ,      , void             , setDDCData                    , ( IOFramebuffer *service, IOIndex bus, UInt32 value ) )
		onevtableitem( 0 ,      , bool             , readDDCClock                  , ( IOFramebuffer *service, IOIndex bus ) )
		onevtableitem( 0 ,      , bool             , readDDCData                   , ( IOFramebuffer *service, IOIndex bus ) )
		onevtableitem( 1 ,      , IOReturn         , enableDDCRaster               , ( IOFramebuffer *service, bool enable ) )
		onevtableitem( 1 ,      , bool             , hasDDCConnect                 , ( IOFramebuffer *service, IOIndex connectIndex ) )
		onevtableitem( 1 ,      , IOReturn         , getDDCBlock                   , ( IOFramebuffer *service, IOIndex connectIndex, UInt32 blockNumber, IOSelect blockType, IOOptionBits options, UInt8 * data, IOByteCount * length ) )
		onevtableitem( 1 ,      , IOReturn         , registerForInterruptType      , ( IOFramebuffer *service, IOSelect interruptType, IOFBInterruptProc proc, OSObject * target, void * ref, void ** interruptRef ) )
		onevtableitem( 1 ,      , IOReturn         , unregisterInterrupt           , ( IOFramebuffer *service, void * interruptRef ) )
		onevtableitem( 0 ,      , IOReturn         , setInterruptState             , ( IOFramebuffer *service, void * interruptRef, UInt32 state ) ) // this happens twice every 5 seconds
		onevtableitem( 1 ,      , IOReturn         , getNotificationSemaphore      , ( IOFramebuffer *service, IOSelect interruptType, semaphore_t * semaphore ) )
// IONDRVFramebuffer
		onevtableitem( 0 , = 350, IOReturn         , doDriverIO                    , ( IONDRVFramebuffer *service, UInt32 commandID, void * contents, UInt32 commandCode, UInt32 commandKind ) )
		onevtableitem( 0 , = 382, IOReturn         , checkDriver                   , ( IONDRVFramebuffer *service ) )
		onevtableitem( 1 ,      , UInt32           , iterateAllModes               , ( IONDRVFramebuffer *service, IODisplayModeID * displayModeIDs ) )
		onevtableitem( 1 ,      , IOReturn         , getResInfoForMode             , ( IONDRVFramebuffer *service, IODisplayModeID modeID, IODisplayModeInformation * theInfo ) )
		onevtableitem( 1 ,      , IOReturn         , getResInfoForArbMode          , ( IONDRVFramebuffer *service, IODisplayModeID modeID, IODisplayModeInformation * theInfo ) )
		onevtableitem( 1 ,      , IOReturn         , validateDisplayMode           , ( IONDRVFramebuffer *service, IODisplayModeID mode, IOOptionBits flags, VDDetailedTimingRec ** detailed ) )
		onevtableitem( 1 ,      , IOReturn         , setDetailedTiming             , ( IONDRVFramebuffer *service, IODisplayModeID mode, IOOptionBits options, void * description, IOByteCount descripSize ) )
		onevtableitem( 1 ,      , void             , getCurrentConfiguration       , ( IONDRVFramebuffer *service ) )
		onevtableitem( 0 ,      , IOReturn         , doControl                     , ( IONDRVFramebuffer *service, UInt32 code, void * params ) )
		onevtableitem( 0 ,      , IOReturn         , doStatus                      , ( IONDRVFramebuffer *service, UInt32 code, void * params ) )
		onevtableitem( 0 ,      , IODeviceMemory * , makeSubRange                  , ( IONDRVFramebuffer *service, IOPhysicalAddress64 start, IOPhysicalLength64  length ) )
		onevtableitem( 0 ,      , IODeviceMemory * , findVRAM                      , ( IONDRVFramebuffer *service ) )
		onevtableitem( 0 ,      , IOTVector *      , undefinedSymbolHandler        , ( IONDRVFramebuffer *service, const char * libraryName, const char * symbolName ) )

#undef onevtableitem
#undef onevtableitem0
#undef onevtableitem1
