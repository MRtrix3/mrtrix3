/****************************************************************************
** Meta object code from reading C++ file 'localsocketreader.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.10)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../../src/gui/mrview/sync/localsocketreader.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'localsocketreader.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.10. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_MR__GUI__MRView__Sync__LocalSocketReader_t {
    QByteArrayData data[6];
    char stringdata0[111];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_MR__GUI__MRView__Sync__LocalSocketReader_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_MR__GUI__MRView__Sync__LocalSocketReader_t qt_meta_stringdata_MR__GUI__MRView__Sync__LocalSocketReader = {
    {
QT_MOC_LITERAL(0, 0, 40), // "MR::GUI::MRView::Sync::LocalS..."
QT_MOC_LITERAL(1, 41, 12), // "DataReceived"
QT_MOC_LITERAL(2, 54, 0), // ""
QT_MOC_LITERAL(3, 55, 36), // "vector<std::shared_ptr<QByteA..."
QT_MOC_LITERAL(4, 92, 3), // "dat"
QT_MOC_LITERAL(5, 96, 14) // "OnDataReceived"

    },
    "MR::GUI::MRView::Sync::LocalSocketReader\0"
    "DataReceived\0\0vector<std::shared_ptr<QByteArray> >\0"
    "dat\0OnDataReceived"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_MR__GUI__MRView__Sync__LocalSocketReader[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       2,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       1,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    1,   24,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
       5,    0,   27,    2, 0x08 /* Private */,

 // signals: parameters
    QMetaType::Void, 0x80000000 | 3,    4,

 // slots: parameters
    QMetaType::Void,

       0        // eod
};

void MR::GUI::MRView::Sync::LocalSocketReader::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<LocalSocketReader *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->DataReceived((*reinterpret_cast< vector<std::shared_ptr<QByteArray> >(*)>(_a[1]))); break;
        case 1: _t->OnDataReceived(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (LocalSocketReader::*)(vector<std::shared_ptr<QByteArray>> );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&LocalSocketReader::DataReceived)) {
                *result = 0;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject MR::GUI::MRView::Sync::LocalSocketReader::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_MR__GUI__MRView__Sync__LocalSocketReader.data,
    qt_meta_data_MR__GUI__MRView__Sync__LocalSocketReader,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *MR::GUI::MRView::Sync::LocalSocketReader::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *MR::GUI::MRView::Sync::LocalSocketReader::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_MR__GUI__MRView__Sync__LocalSocketReader.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int MR::GUI::MRView::Sync::LocalSocketReader::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 2)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 2;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 2)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 2;
    }
    return _id;
}

// SIGNAL 0
void MR::GUI::MRView::Sync::LocalSocketReader::DataReceived(vector<std::shared_ptr<QByteArray>> _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
