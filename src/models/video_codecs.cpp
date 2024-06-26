#include "video_codecs.h"
#include "settings.h"

void VideoCodecs::init()
{
    //load existing codec priorities
    setCodecsInfo(Settings::videoCodecInfo());
    qInfo() << "Restoring" << _codecInfo.size() << "video codec priorities";
    for (const auto &it: std::as_const(_codecInfo)) {
        emit codecPriorityChanged(it.codecId, it.priority, it.priority);
    }
}

void VideoCodecs::setCodecsInfo(const QList<CodecInfo> &info)
{
    _codecInfo = info;
    //format codec ID
    for (auto &ci: _codecInfo) {
        const auto tok = ci.codecId.split('/');
        if (0 < tok.length()) {
            ci.formattedCodecId = tok.at(0);
        }
    }
    sort(PRIORITY, Qt::DescendingOrder);
}
