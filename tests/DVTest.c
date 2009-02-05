/*
 *  DVtest.c
 *  IOFWDVComponents
 *
 *  Created by wgulland on Tue Oct 17 2000.
 *  Copyright (c) 2000 Apple Computer, Inc. All rights reserved.
 *
 */

#include <Carbon/Carbon.h>

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "DVFamily.h"

typedef struct DVFuncsStruct {
    UInt32 (*fDVCountDevices)( void );
    OSErr (*fDVGetIndDevice)( DVDeviceID * pDVDevice, UInt32 index );
    OSErr (*fDVSetDeviceName)( DVDeviceID deviceID, char * str );
    OSErr (*fDVGetDeviceName)( DVDeviceID deviceID, char * str );

    OSErr (*fDVOpenDriver)( DVDeviceID deviceID, DVDeviceRefNum *pRefNum );
    OSErr (*fDVCloseDriver)( DVDeviceRefNum refNum );

    OSErr (*fDVDoAVCTransaction)( DVDeviceRefNum refNum, AVCTransactionParamsPtr pParams );

    OSErr (*fDVIsEnabled)( DVDeviceRefNum refNum, Boolean *isEnabled);
    OSErr (*fDVGetDeviceStandard)( DVDeviceRefNum refNum, UInt32 * pStandard );

    // DV Isoch Read
    OSErr (*fDVEnableRead)( DVDeviceRefNum refNum );
    OSErr (*fDVDisableRead)( DVDeviceRefNum refNum );
    OSErr (*fDVReadFrame)( DVDeviceRefNum refNum, Ptr *ppReadBuffer, UInt32 * pSize );
    OSErr (*fDVReleaseFrame)( DVDeviceRefNum refNum, Ptr pReadBuffer );

    // DV Isoch Write
    OSErr (*fDVEnableWrite)( DVDeviceRefNum refNum );
    OSErr (*fDVDisableWrite)( DVDeviceRefNum refNum );
    OSErr (*fDVGetEmptyFrame)( DVDeviceRefNum refNum, Ptr *ppEmptyFrameBuffer, UInt32 * pSize );
    OSErr (*fDVWriteFrame)( DVDeviceRefNum refNum, Ptr pWriteBuffer );

    // Notifications
    OSErr (*fDVNewNotification)( DVDeviceRefNum refNum, DVNotifyProc notifyProc,
						void *userData, DVNotificationID *pNotifyID );	
    OSErr (*fDVNotifyMeWhen)( DVDeviceRefNum refNum, DVNotificationID notifyID, UInt32 events);
    OSErr (*fDVCancelNotification)( DVDeviceRefNum refNum, DVNotificationID notifyID );
    OSErr (*fDVDisposeNotification)( DVDeviceRefNum refNum, DVNotificationID notifyID );

} DVFuncs, *DVFuncsPtr;

static DVFuncs sDVFuncs;
static char *sFile = "/tmp/dump.dv";

static CFBundleRef findMe()
{
    CFURLRef    bundleURL;
    CFBundleRef myBundle;
    Boolean didLoad = false;
    // Make a CFURLRef from the CFString representation of the 
    // bundle's path. See the Core Foundation URL Services chapter 
    // for details.
    bundleURL = CFURLCreateWithFileSystemPath( 
                    kCFAllocatorDefault, 
                    CFSTR("/System/Library/Extensions/DVFamily.bundle"),
                    kCFURLPOSIXPathStyle,
                    true );
    
    printf("Bundle URL: %p\n", bundleURL);
    // Make a bundle instance using the URLRef.
    myBundle = CFBundleCreate( kCFAllocatorDefault, bundleURL );
    printf("Bundle: %p\n", myBundle);

    // Try to load the executable from my bundle.
    didLoad = CFBundleLoadExecutable( myBundle );
    printf("loaded? %d\n", didLoad);
    
    sDVFuncs.fDVCountDevices = CFBundleGetFunctionPointerForName( 
                myBundle, CFSTR("DVCountDevices") );
    sDVFuncs.fDVGetIndDevice = CFBundleGetFunctionPointerForName( 
                myBundle, CFSTR("DVGetIndDevice") );
    sDVFuncs.fDVSetDeviceName = CFBundleGetFunctionPointerForName( 
                myBundle, CFSTR("DVSetDeviceName") );
    sDVFuncs.fDVGetDeviceName = CFBundleGetFunctionPointerForName( 
                myBundle, CFSTR("DVGetDeviceName") );
    sDVFuncs.fDVOpenDriver = CFBundleGetFunctionPointerForName( 
                myBundle, CFSTR("DVOpenDriver") );
    sDVFuncs.fDVCloseDriver = CFBundleGetFunctionPointerForName( 
                myBundle, CFSTR("DVCloseDriver") );

// AVC Stuff
    sDVFuncs.fDVDoAVCTransaction = CFBundleGetFunctionPointerForName( 
                myBundle, CFSTR("DVDoAVCTransaction") );

    sDVFuncs.fDVIsEnabled = CFBundleGetFunctionPointerForName( 
                myBundle, CFSTR("DVIsEnabled") );
    sDVFuncs.fDVGetDeviceStandard = CFBundleGetFunctionPointerForName( 
                myBundle, CFSTR("DVGetDeviceStandard") );

// Isoch I/O
    sDVFuncs.fDVEnableRead = CFBundleGetFunctionPointerForName( 
                myBundle, CFSTR("DVEnableRead") );
    sDVFuncs.fDVDisableRead = CFBundleGetFunctionPointerForName( 
                myBundle, CFSTR("DVDisableRead") );
    sDVFuncs.fDVReadFrame = CFBundleGetFunctionPointerForName( 
                myBundle, CFSTR("DVReadFrame") );
    sDVFuncs.fDVReleaseFrame = CFBundleGetFunctionPointerForName( 
                myBundle, CFSTR("DVReleaseFrame") );
    sDVFuncs.fDVEnableWrite = CFBundleGetFunctionPointerForName( 
                myBundle, CFSTR("DVEnableWrite") );
    sDVFuncs.fDVDisableWrite = CFBundleGetFunctionPointerForName( 
                myBundle, CFSTR("DVDisableWrite") );
    sDVFuncs.fDVGetEmptyFrame = CFBundleGetFunctionPointerForName( 
                myBundle, CFSTR("DVGetEmptyFrame") );
    sDVFuncs.fDVWriteFrame = CFBundleGetFunctionPointerForName( 
                myBundle, CFSTR("DVWriteFrame") );

// Notifications
    sDVFuncs.fDVNewNotification = CFBundleGetFunctionPointerForName( 
                myBundle, CFSTR("DVNewNotification") );
    sDVFuncs.fDVNotifyMeWhen = CFBundleGetFunctionPointerForName( 
                myBundle, CFSTR("DVNotifyMeWhen") );
    sDVFuncs.fDVCancelNotification = CFBundleGetFunctionPointerForName( 
                myBundle, CFSTR("DVCancelNotification") );
    sDVFuncs.fDVDisposeNotification = CFBundleGetFunctionPointerForName( 
                myBundle, CFSTR("DVDisposeNotification") );

    // Any CF objects returned from functions with "create" or 
    // "copy" in their names must be released by us!
    CFRelease( bundleURL );
    return myBundle;
}

static OSErr doControlTest(DVDeviceRefNum refNum, UInt8 op1, UInt8 op2)
{
        //Component control;
    AVCTransactionParams avcParams;
    char in[4], out[16];
    OSErr err;

    // fill up the avc frame
    in[0]	= kAVCControlCommand;
    in[1] 	= 0x20;	// for now
    in[2] 	= op1;
    in[3] 	= op2;

    // fill up the transaction parameter block
    avcParams.commandBufferPtr = in;
    avcParams.commandLength = sizeof(in);
    avcParams.responseBufferPtr = out;
    avcParams.responseBufferSize = sizeof(out);
    avcParams.responseHandler = NULL;

    err = sDVFuncs.fDVDoAVCTransaction(refNum, &avcParams );
    if(err)
        printf("Error %d calling DVDoAVCTransaction(%ld)\n", err, refNum);
        
    return err;
}

static void readFrames(DVDeviceRefNum refNum, int file, int numFrames)
{
    OSErr err, wait;
    Ptr pReadBuffer;
    UInt32 size;
    int i;
    
    err = sDVFuncs.fDVEnableRead(refNum);
    
    for(i=0; i<numFrames; i++) {
        wait = sDVFuncs.fDVReadFrame( refNum, &pReadBuffer, &size );
        while(wait == -1) {
            usleep(10000);	// 10 milliseconds
            wait = sDVFuncs.fDVReadFrame( refNum, &pReadBuffer, &size );
        }
        if(file)
            write(file, pReadBuffer, size);
        err = sDVFuncs.fDVReleaseFrame( refNum, pReadBuffer );        
    }

    err = sDVFuncs.fDVDisableRead( refNum );

}

static void writeFrames(DVDeviceRefNum refNum, int file, int numFrames)
{
    OSErr err, wait;
    Ptr pReadBuffer;
    UInt32 size;
    int i;
    
    err = sDVFuncs.fDVEnableWrite(refNum);
    
    for(i=0; i<numFrames; i++) {
        wait = sDVFuncs.fDVGetEmptyFrame( refNum, &pReadBuffer, &size );
        while(wait == -1) {
            usleep(10000);	// 10 milliseconds
            wait = sDVFuncs.fDVGetEmptyFrame( refNum, &pReadBuffer, &size );
        }
        if(file)
            read(file, pReadBuffer, size);
        err = sDVFuncs.fDVWriteFrame( refNum, pReadBuffer );        
    }

    err = sDVFuncs.fDVDisableWrite( refNum );

}


static OSStatus myNotifyProc(DVEventRecordPtr event, void *userData )
{
    printf("event for device %d, event %d, userdata %p\n", 
        event->eventHeader.deviceID, event->eventHeader.theEvent, userData);
    return noErr;
}

int main(int argc, char **argv)
{
    CFBundleRef myBundle;
    UInt32 numDevs, i;
    UInt32 standard;
    Boolean isEnabled;
    OSErr err;
    DVDeviceID device;
    DVDeviceRefNum refNum;
    DVNotificationID notifyID;
    char name[256];
    int file;
    
    myBundle = findMe();
    
    numDevs = sDVFuncs.fDVCountDevices();
    printf("Number of devices: %ld\n", numDevs);
    if(numDevs == 0) {
        err = sDVFuncs.fDVNewNotification( kEveryDVDeviceRefNum, myNotifyProc,
						0x1234, &notifyID );
        if(err)
            printf("Error %d calling DVNewNotification(, %ld)\n", err, kEveryDVDeviceRefNum);
	err = sDVFuncs.fDVNotifyMeWhen( kEveryDVDeviceRefNum, notifyID,
            kDVDeviceAdded | kDVDeviceRemoved);
        if(err)
            printf("Error %d calling NotifyMeWhen(%ld, %ld)\n", err, kEveryDVDeviceRefNum, notifyID);

        while (true) {
        printf("Waiting for devices: %ld\n", numDevs);
        usleep(1000000);	// 1000 milliseconds
        numDevs = sDVFuncs.fDVCountDevices();
        }
    }
    for(i=1; i<=numDevs; i++) {
        err = sDVFuncs.fDVGetIndDevice(&device, i);
        if(err)
            printf("Error %d calling DVGetIndDevice(, %ld)\n", err, i);
        err = sDVFuncs.fDVGetDeviceName(device, name);
        if(err)
            printf("Error %d calling DVGetDeviceName(%ld)\n", err, device);
        else
            printf("Device %ld name is %s\n", device, name);
            
        err = sDVFuncs.fDVOpenDriver( device, &refNum );
        if(err)
            printf("Error %d calling DVOpenDriver(%ld)\n", err, device);
            
        err = sDVFuncs.fDVGetDeviceStandard(refNum, &standard);
        if(err)
            printf("Error %d calling DVGetDeviceStandard(%ld)\n", err, device);
        else if(standard == kNTSCStandard)
            printf("Device %ld video standard is NTSC\n", device);
        else if(standard == kPALStandard)
            printf("Device %ld video standard is PAL\n", device);
        else
            printf("Device %ld, unknown video standard %ld\n", device, standard);

	err = sDVFuncs.fDVIsEnabled(refNum, &isEnabled);
        if(err)
            printf("Error %d calling DVIsEnabled(%ld)\n", err, device);
        else
            printf("Device %ld isEnabled: %d\n", device, isEnabled);
        
#if 0
        file = open(sFile, O_CREAT | O_WRONLY | O_TRUNC, 0666);
// Start camera playing
        
        err = doControlTest(refNum, kAVCPlayOpcode, kAVCPlayForward);
        
        readFrames(refNum, file, 30);
       
        //sleep(10);
        err = doControlTest(refNum, kAVCWindOpcode, kAVCWindStop);
        
        err = sDVFuncs.fDVCloseDriver( refNum );
        if(err)
            printf("Error %d calling DVCloseDriver(%ld)\n", err, device);
        printf("Destruction test...\n");
        while(true) {
            readFrames(refNum, 0, 30);
        }
#else
        file = open(sFile, O_RDONLY, 0666);
        printf("opend file, %d\n", file);
        writeFrames(refNum, file, 300);
#endif    
    }
    
    // Unload the bundle's executable code. 
    CFBundleUnloadExecutable( myBundle );
    CFRelease( myBundle );
    return 0;
}
