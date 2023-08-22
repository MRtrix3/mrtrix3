/****************************************************************************
** Meta object code from reading C++ file 'lighting_dock.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.10)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../src/gui/lighting_dock.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'lighting_dock.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.10. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_MR__GUI__LightingSettings_t {
    QByteArrayData data[8];
    char stringdata0[134];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_MR__GUI__LightingSettings_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_MR__GUI__LightingSettings_t qt_meta_stringdata_MR__GUI__LightingSettings = {
    {
QT_MOC_LITERAL(0, 0, 25), // "MR::GUI::LightingSettings"
QT_MOC_LITERAL(1, 26, 22), // "ambient_intensity_slot"
QT_MOC_LITERAL(2, 49, 0), // ""
QT_MOC_LITERAL(3, 50, 5), // "value"
QT_MOC_LITERAL(4, 56, 22), // "diffuse_intensity_slot"
QT_MOC_LITERAL(5, 79, 23), // "specular_intensity_slot"
QT_MOC_LITERAL(6, 103, 10), // "shine_slot"
QT_MOC_LITERAL(7, 114, 19) // "light_position_slot"

    },
    "MR::GUI::LightingSettings\0"
    "ambient_intensity_slot\0\0value\0"
    "diffuse_intensity_slot\0specular_intensity_slot\0"
    "shine_slot\0light_position_slot"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_MR__GUI__LightingSettings[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       5,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags
       1,    1,   39,    2, 0x09 /* Protected */,
       4,    1,   42,    2, 0x09 /* Protected */,
       5,    1,   45,    2, 0x09 /* Protected */,
       6,    1,   48,    2, 0x09 /* Protected */,
       7,    0,   51,    2, 0x09 /* Protected */,

 // slots: parameters
    QMetaType::Void, QMetaType::Int,    3,
    QMetaType::Void, QMetaType::Int,    3,
    QMetaType::Void, QMetaType::Int,    3,
    QMetaType::Void, QMetaType::Int,    3,
    QMetaType::Void,

       0        // eod
};

void MR::GUI::LightingSettings::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<LightingSettings *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->ambient_intensity_slot((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 1: _t->diffuse_intensity_slot((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 2: _t->specular_intensity_slot((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 3: _t->shine_slot((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 4: _t->light_position_slot(); break;
        default: ;
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject MR::GUI::LightingSettings::staticMetaObject = { {
    QMetaObject::SuperData::link<QFrame::staticMetaObject>(),
    qt_meta_stringdata_MR__GUI__LightingSettings.data,
    qt_meta_data_MR__GUI__LightingSettings,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *MR::GUI::LightingSettings::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *MR::GUI::LightingSettings::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_MR__GUI__LightingSettings.stringdata0))
        return static_cast<void*>(this);
    return QFrame::qt_metacast(_clname);
}

int MR::GUI::LightingSettings::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QFrame::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 5)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 5;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 5)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 5;
    }
    return _id;
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
