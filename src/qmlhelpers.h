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

#define QML_WRITABLE_PROPERTY_POD(type, name, setter, defaultValue) \
    protected: \
	Q_PROPERTY(type name MEMBER _##name WRITE setter NOTIFY name##Changed) \
    public: \
	constexpr void setter(type value) { \
		if (value != _##name) { \
			_##name = value; \
			emit name##Changed(); \
	} \
} \
	constexpr const type name() const { return _##name; } \
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
	constexpr const type name() const { return _##name; } \
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

#define QML_READABLE_PROPERTY_POD(type, name, setter, defaultValue) \
    protected: \
	Q_PROPERTY(type name MEMBER _##name NOTIFY name##Changed) \
    public: \
	constexpr void setter(type value) { \
		if (value != _##name) { \
			_##name = value; \
			emit name##Changed(); \
	} \
} \
	constexpr const type name() const { return _##name; } \
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

#define QML_CONSTANT_PROPERTY_POD(type, name, defaultValue) \
    protected: \
	Q_PROPERTY(type name MEMBER _##name CONSTANT) \
    public: \
	constexpr const type name() const { return _##name; } \
    private: \
	const type _##name = defaultValue;

#define QML_CONSTANT_PROPERTY_PTR(type, name) \
    protected: \
        Q_PROPERTY(type* name MEMBER _##name CONSTANT) \
    public: \
	constexpr type* name() { return _##name; } \
    private: \
        type *const _##name = new type(this);
