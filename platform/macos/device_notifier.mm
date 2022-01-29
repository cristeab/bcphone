#include "device_notifier.h"
#include "softphone.h"
#import <CoreAudio/CoreAudio.h>
#include <QDebug>

namespace DeviceNotifier {

static
void onAudioDeviceChanged(void *userData)
{
    if (NULL != userData) {
        Softphone *sp = reinterpret_cast<Softphone*>(userData);
        emit sp->audioDevicesChanged();
    }
}

static
OSStatus defaultOutputDeviceChanged(AudioObjectID inObjectID,
                            UInt32 inNumberAddresses,
                            const AudioObjectPropertyAddress inAddresses[],
                            void *inClientData)
{
    Q_UNUSED(inObjectID)
    Q_UNUSED(inNumberAddresses)
    Q_UNUSED(inAddresses)
    qInfo() << "Default output device changed";
    onAudioDeviceChanged(inClientData);
    return noErr;
}

static
OSStatus defaultInputDeviceChanged(AudioObjectID inObjectID,
                            UInt32 inNumberAddresses,
                            const AudioObjectPropertyAddress inAddresses[],
                            void *inClientData)
{
    Q_UNUSED(inObjectID)
    Q_UNUSED(inNumberAddresses)
    Q_UNUSED(inAddresses)
    qInfo() << "Default input device changed";
    onAudioDeviceChanged(inClientData);
    return noErr;
}

void setupAudio(void *userData)
{
    AudioObjectPropertyAddress outDevAddr = {
        kAudioHardwarePropertyDefaultOutputDevice,
        kAudioObjectPropertyScopeGlobal,
        kAudioObjectPropertyElementMaster
    };

    OSStatus ret = AudioObjectAddPropertyListener(kAudioObjectSystemObject, &outDevAddr,
                                                  defaultOutputDeviceChanged, userData);
    if (noErr != ret) {
        qCritical() << "Cannot install notifier for default output audio device";
    }

    AudioObjectPropertyAddress inDevAddr = {
        kAudioHardwarePropertyDefaultInputDevice,
        kAudioObjectPropertyScopeGlobal,
        kAudioObjectPropertyElementMaster
    };
    ret = AudioObjectAddPropertyListener(kAudioObjectSystemObject, &inDevAddr,
                                         defaultInputDeviceChanged, userData);
    if (noErr != ret) {
        qCritical() << "Cannot install notifier for default output audio device";
    }
}

} // DeviceNotifier
