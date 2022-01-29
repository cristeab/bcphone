#pragma once

#include "generic_codecs.h"

class AudioCodecs : public GenericCodecs
{
    Q_OBJECT

public:
    explicit AudioCodecs(QObject *parent = nullptr);
    void init() override;
    void setCodecsInfo(const QList<CodecInfo> &info) override;
    void saveCodecsInfo() override;
};
