#pragma once

#include "generic_codecs.h"

class VideoCodecs : public GenericCodecs
{
    Q_OBJECT
    QML_ANONYMOUS

public:
    explicit VideoCodecs(QObject *parent = nullptr) : GenericCodecs(parent) {}
    void init() override;
    void setCodecsInfo(const QList<CodecInfo> &info) override;
};
