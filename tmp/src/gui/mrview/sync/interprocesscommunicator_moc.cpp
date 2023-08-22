/****************************************************************************
** Meta object code from reading C++ file 'interprocesscommunicator.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.10)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../../src/gui/mrview/sync/interprocesscommunicator.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'interprocesscommunicator.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.10. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_MR__GUI__MRView__Sync__InterprocessCommunicator_t {
    QByteArrayData data[8];
    char stringdata0[151];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_MR__GUI__MRView__Sync__InterprocessCommunicator_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_MR__GUI__MRView__Sync__InterprocessCommunicator_t qt_meta_stringdata_MR__GUI__MRView__Sync__InterprocessCommunicator = {
    {
QT_MOC_LITERAL(0, 0, 47), // "MR::GUI::MRView::Sync::Interp..."
QT_MOC_LITERAL(1, 48, 16), // "SyncDataReceived"
QT_MOC_LITERAL(2, 65, 0), // ""
QT_MOC_LITERAL(3, 66, 36), // "vector<std::shared_ptr<QByteA..."
QT_MOC_LITERAL(4, 103, 4), // "data"
QT_MOC_LITERAL(5, 108, 23), // "OnNewIncomingConnection"
QT_MOC_LITERAL(6, 132, 14), // "OnDataReceived"
QT_MOC_LITERAL(7, 147, 3) // "dat"

    },
    "MR::GUI::MRView::Sync::InterprocessCommunicator\0"
    "SyncDataReceived\0\0"
    "vector<std::shared_ptr<QByteArray> >\0"
    "data\0OnNewIncomingConnection\0"
    "OnDataReceived\0dat"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_MR__GUI__MRView__Sync__InterprocessCommunicator[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       3,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       1,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    1,   29,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
       5,    0,   32,    2, 0x08 /* Private */,
       6,    1,   33,    2, 0x08 /* Private */,

 // signals: parameters
    QMetaType::Void, 0x80000000 | 3,    4,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 3,    7,

       0        // eod
};

void MR::GUI::MRView::Sync::InterprocessCommunicator::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<InterprocessCommunicator *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->SyncDataReceived((*reinterpret_cast< vector<std::shared_ptr<QByteArray> >(*)>(_a[1]))); break;
        case 1: _t->OnNewIncomingConnection(); break;
        case 2: _t->OnDataReceived((*reinterpret_cast< vector<std::shared_ptr<QByteArray> >(*)>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (InterprocessCommunicator::*)(vector<std::shared_ptr<QByteArray>> );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&InterprocessCommunicator::SyncDataReceived)) {
                *result = 0;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject MR::GUI::MRView::Sync::InterprocessCommunicator::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_MR__GUI__MRView__Sync__InterprocessCommunicator.data,
    qt_meta_data_MR__GUI__MRView__Sync__InterprocessCommunicator,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *MR::GUI::MRView::Sync::InterprocessCommunicator::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *MR::GUI::MRView::Sync::InterprocessCommunicator::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_MR__GUI__MRView__Sync__InterprocessCommunicator.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int MR::GUI::MRView::Sync::InterprocessCommunicator::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 3)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 3;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 3)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 3;
    }
    return _id;
}

// SIGNAL 0
void MR::GUI::MRView::Sync::InterprocessCommunicator::SyncDataReceived(vector<std::shared_ptr<QByteArray>> _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
