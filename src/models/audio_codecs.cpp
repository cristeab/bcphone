#include "audio_codecs.h"
#include "settings.h"

AudioCodecs::AudioCodecs(QObject *parent) : GenericCodecs(parent)
{
}

void AudioCodecs::init()
{
    //load existing codec priorities
    setCodecsInfo(Settings::audioCodecInfo());
    qInfo() << "Restoring" << _codecInfo.size() << "codec priorities";
    for (const auto &it: qAsConst(_codecInfo)) {
        emit codecPriorityChanged(it.codecId, it.priority, it.priority);
    }
}

void AudioCodecs::setCodecsInfo(const QList<CodecInfo> &info)
{
    _codecInfo = info;
    //format codec ID
    for (auto &ci: _codecInfo) {
        const auto tok = ci.codecId.split('/');
        if (1 < tok.length()) {
            const int sampFreq = tok.at(1).toInt();
            if (0 == sampFreq) {
                qWarning() << "Sampling frequency is not in kHz";
                continue;
            }
            ci.formattedCodecId = tok.at(0) + " (" + QString::number(sampFreq) + " Hz)";
        }
    }
    sort(PRIORITY, Qt::DescendingOrder);
}

void AudioCodecs::saveCodecsInfo()
{
    Settings::saveAudioCodecInfo(_codecInfo);
}
