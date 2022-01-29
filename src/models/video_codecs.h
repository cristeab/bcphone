#pragma once

#include "generic_codecs.h"

class VideoCodecs : public GenericCodecs
{
    Q_OBJECT

public:
    explicit VideoCodecs(QObject *parent = nullptr);
    void init() override;
    void setCodecsInfo(const QList<CodecInfo> &info) override;
    void saveCodecsInfo() override;
};
