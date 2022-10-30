#pragma once

#include <QObject>

#define QML_WRITABLE_PROPERTY(type, name, setter, defaultValue) \
    protected: \
        Q_PROPERTY(type name MEMBER _##name WRITE setter NOTIFY name##Changed) \
    public: \
        void setter(const type &value) { \
            if (value != _##name) { \
                _##name = value; \
                emit name##Changed(); \
            } \
        } \
        const type& name() const { return _##name; } \
    Q_SIGNALS: \
        void name##Changed(); \
    private: \
        type _##name = defaultValue;

#define QML_WRITABLE_PROPERTY_FLOAT(type, name, setter, defaultValue) \
    protected: \
        Q_PROPERTY(type name MEMBER _##name WRITE setter NOTIFY name##Changed) \
    public: \
        void setter(type value) { \
            if (!qFuzzyCompare(value, _##name)) { \
                _##name = value; \
                emit name##Changed(); \
            } \
        } \
        const type& name() const { return _##name; } \
    Q_SIGNALS: \
        void name##Changed(); \
    private: \
        type _##name = defaultValue;

#define QML_READABLE_PROPERTY(type, name, setter, defaultValue) \
    protected: \
        Q_PROPERTY(type name MEMBER _##name NOTIFY name##Changed) \
    public: \
        void setter(const type &value) { \
            if (value != _##name) { \
                _##name = value; \
                emit name##Changed(); \
            } \
        } \
        const type& name() const { return _##name; } \
    Q_SIGNALS: \
        void name##Changed(); \
    private: \
        type _##name = defaultValue;

#define QML_CONSTANT_PROPERTY(type, name, defaultValue) \
    protected: \
        Q_PROPERTY(type name MEMBER _##name CONSTANT) \
    public: \
        const type& name() const { return _##name; } \
    private: \
        const type _##name = defaultValue;

#define QML_CONSTANT_PROPERTY_PTR(type, name) \
    protected: \
        Q_PROPERTY(type* name MEMBER _##name CONSTANT) \
    public: \
        type* name() { return _##name; } \
    private: \
        type *const _##name = new type(this);

#define QML_CONSTANT_PROPERTY_SET_PTR(type, name, setter) \
    protected: \
        Q_PROPERTY(type* name MEMBER _##name CONSTANT) \
    public: \
        type* name() const { return _##name; } \
    private: \
        void setter(type *value) { \
            _##name = value; \
        } \
        type* _##name = nullptr;

// NOTE : to avoid "no suitable class found" MOC note
class QmlProperty : public QObject { Q_OBJECT };
