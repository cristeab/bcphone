#pragma once

#include "qmlhelpers.h"
#include "settings.h"
#include "models/ring_tones_model.h"
#include "models/video_devices.h"
#include "models/contacts_model.h"
#include "models/call_history_model.h"
#include "models/audio_codecs.h"
#include "models/video_codecs.h"
#include "models/active_call_model.h"
#include "models/presence_model.h"
#include "models/chat_list_proxy.h"
#include "models/messages_proxy_model.h"
#include <QTimer>
#include <QString>
#include <QMap>

class SipClient;

class Softphone : public QObject {
	Q_OBJECT
public:
    enum class SipRegistrationStatus { Unregistered, RegistrationInProgress, Registered };
    Q_ENUM(SipRegistrationStatus)

private:
    QML_READABLE_PROPERTY(QString, sipRegistrationText, setSipRegistrationText, tr("Not Registered"))
    QML_READABLE_PROPERTY(SipRegistrationStatus, sipRegistrationStatus, setSipRegistrationStatus, SipRegistrationStatus::Unregistered)

    QML_READABLE_PROPERTY(bool, showBusy, setShowBusy, false)
    QML_READABLE_PROPERTY(bool, loggedOut, setLoggedOut, false)

    QML_WRITABLE_PROPERTY(QString, dialogMessage, setDialogMessage, "")
    QML_READABLE_PROPERTY(bool, dialogError, setDialogError, false)
    QML_WRITABLE_PROPERTY(bool, dialogRetry, setDialogRetry, false)

    QML_WRITABLE_PROPERTY(QString, dialedText, setDialedText, "")
    QML_WRITABLE_PROPERTY(QString, currentUserId, setCurrentUserId, "")
    QML_WRITABLE_PROPERTY(bool, activeCall, setActiveCall, false)
    QML_READABLE_PROPERTY(bool, confirmedCall, setConfirmedCall, false)

    QML_WRITABLE_PROPERTY(bool, muteMicrophone, setMuteMicrophone, true)
    QML_WRITABLE_PROPERTY(bool, record, setRecord, false)
    QML_WRITABLE_PROPERTY(bool, holdCall, setHoldCall, false)

    QML_READABLE_PROPERTY(bool, hasVideo, setHasVideo, false)
    QML_WRITABLE_PROPERTY(bool, enableVideo, setEnableVideo, false)

    QML_WRITABLE_PROPERTY(int, winPosX, setWinPosX, 0)
    QML_WRITABLE_PROPERTY(int, winPosY, setWinPosY, 0)
    QML_WRITABLE_PROPERTY(int, winWidth, setWinWidth, 0)

    QML_CONSTANT_PROPERTY_PTR(Settings, settings)
    QML_CONSTANT_PROPERTY_PTR(AudioDevices, inputAudioDevices)
    QML_CONSTANT_PROPERTY_PTR(AudioDevices, outputAudioDevices)
    QML_CONSTANT_PROPERTY_PTR(VideoDevices, videoDevices)
    QML_CONSTANT_PROPERTY_PTR(AudioCodecs, audioCodecs)
    QML_CONSTANT_PROPERTY_PTR(VideoCodecs, videoCodecs)
    QML_CONSTANT_PROPERTY_PTR(RingTonesModel, ringTonesModel)
    QML_CONSTANT_PROPERTY_PTR(ContactsModel, contactsModel)
    QML_CONSTANT_PROPERTY_PTR(CallHistoryModel, callHistoryModel)
    QML_CONSTANT_PROPERTY_PTR(ActiveCallModel, activeCallModel)
    QML_CONSTANT_PROPERTY_PTR(PresenceModel, presenceModel)

    QML_CONSTANT_PROPERTY_PTR(ChatListProxy, chatList)
    QML_CONSTANT_PROPERTY_PTR(MessagesProxyModel, messagesModel)

    QML_CONSTANT_PROPERTY(int, invalidCallId, PJSUA_INVALID_ID)

    QML_WRITABLE_PROPERTY(bool, blindTransfer, setBlindTransfer, false)
    QML_WRITABLE_PROPERTY(QString, blindTransferUserName, setBlindTransferUserName, "")

    QML_WRITABLE_PROPERTY(bool, conference, setConference, false)

    QML_WRITABLE_PROPERTY(bool, isRegisterRequested, setIsRegisterRequested, false)

public:
    Softphone();

    bool start();
    void setMainForm(QObject *mainForm) { _mainForm = mainForm; }

    Q_INVOKABLE bool registerAccount();
    Q_INVOKABLE bool unregisterAccount();
    Q_INVOKABLE bool makeCall(const QString &userId);
    Q_INVOKABLE bool answer(int callId);
    Q_INVOKABLE bool hangup(int callId);
    Q_INVOKABLE void hangupAll();
    Q_INVOKABLE bool unsupervisedTransfer(const QString &phoneNumber);
    Q_INVOKABLE bool holdAndAnswer(int callId);
    Q_INVOKABLE bool swap(int callId);
    Q_INVOKABLE bool merge(int callId);
    Q_INVOKABLE bool sendDtmf(const QString &dtmf);
    Q_INVOKABLE void manuallyRegister();
    Q_INVOKABLE bool playDigit(const QString& digit);

    bool hold(bool value, int callId);
    bool mute(bool value, int callId);
    bool rec(bool value, int callId);
    QString convertNumber(const QString &num);

    static void errorHandler(const QString &title, pj_status_t status = PJ_SUCCESS,
                             bool emitSignal = false);

signals:
    void phoneStateChanged();
    void audioDevicesChanged();
    void incoming(int callCount,
                  int callId,
                  const QString &userId,
                  const QString &userName,
                  bool isConf);
    void disconnected(int callId);

private:
    Q_DISABLE_COPY_MOVE(Softphone)

    static void printSslBackendVersion();
    void onConfirmed(int callId);
    void onCalling(int callId, const QString &userId, const QString &userName);
    void onIncoming(int callId, const QString &userId, const QString &userName);
    void onDisconnected(int callId);

    void onMicrophoneVolumeChanged();
    void onSpeakersVolumeChanged();

    void raiseWindow();

    bool disableAudio(bool force = false);

    void errorDialog(const QString& msg) {
        setDialogError(true);
        setDialogMessage(msg);
    }

    SipClient *_sipClient = nullptr;
    QObject *_mainForm = nullptr;
    QHash<pjsua_call_id, pjsua_player_id> _playerId;
    QHash<pjsua_call_id, pjsua_recorder_id> _recId;
    QHash<pjsua_call_id, pjsua_player_id> _playbackPlayerId;
    bool _manualHangup = false;
    bool _audioEnabled = false;
};
