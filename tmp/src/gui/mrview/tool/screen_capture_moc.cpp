/****************************************************************************
** Meta object code from reading C++ file 'screen_capture.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.10)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../../src/gui/mrview/tool/screen_capture.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'screen_capture.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.10. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_MR__GUI__MRView__Tool__Capture_t {
    QByteArrayData data[11];
    char stringdata0[205];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_MR__GUI__MRView__Tool__Capture_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_MR__GUI__MRView__Tool__Capture_t qt_meta_stringdata_MR__GUI__MRView__Tool__Capture = {
    {
QT_MOC_LITERAL(0, 0, 30), // "MR::GUI::MRView::Tool::Capture"
QT_MOC_LITERAL(1, 31, 16), // "on_image_changed"
QT_MOC_LITERAL(2, 48, 0), // ""
QT_MOC_LITERAL(3, 49, 16), // "on_rotation_type"
QT_MOC_LITERAL(4, 66, 19), // "on_translation_type"
QT_MOC_LITERAL(5, 86, 17), // "on_screen_capture"
QT_MOC_LITERAL(6, 104, 17), // "on_screen_preview"
QT_MOC_LITERAL(7, 122, 14), // "on_screen_stop"
QT_MOC_LITERAL(8, 137, 25), // "select_output_folder_slot"
QT_MOC_LITERAL(9, 163, 16), // "on_output_update"
QT_MOC_LITERAL(10, 180, 24) // "on_restore_capture_state"

    },
    "MR::GUI::MRView::Tool::Capture\0"
    "on_image_changed\0\0on_rotation_type\0"
    "on_translation_type\0on_screen_capture\0"
    "on_screen_preview\0on_screen_stop\0"
    "select_output_folder_slot\0on_output_update\0"
    "on_restore_capture_state"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_MR__GUI__MRView__Tool__Capture[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       9,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags
       1,    0,   59,    2, 0x08 /* Private */,
       3,    1,   60,    2, 0x08 /* Private */,
       4,    1,   63,    2, 0x08 /* Private */,
       5,    0,   66,    2, 0x08 /* Private */,
       6,    0,   67,    2, 0x08 /* Private */,
       7,    0,   68,    2, 0x08 /* Private */,
       8,    0,   69,    2, 0x08 /* Private */,
       9,    0,   70,    2, 0x08 /* Private */,
      10,    0,   71,    2, 0x08 /* Private */,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int,    2,
    QMetaType::Void, QMetaType::Int,    2,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

void MR::GUI::MRView::Tool::Capture::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<Capture *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->on_image_changed(); break;
        case 1: _t->on_rotation_type((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 2: _t->on_translation_type((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 3: _t->on_screen_capture(); break;
        case 4: _t->on_screen_preview(); break;
        case 5: _t->on_screen_stop(); break;
        case 6: _t->select_output_folder_slot(); break;
        case 7: _t->on_output_update(); break;
        case 8: _t->on_restore_capture_state(); break;
        default: ;
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject MR::GUI::MRView::Tool::Capture::staticMetaObject = { {
    QMetaObject::SuperData::link<Base::staticMetaObject>(),
    qt_meta_stringdata_MR__GUI__MRView__Tool__Capture.data,
    qt_meta_data_MR__GUI__MRView__Tool__Capture,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *MR::GUI::MRView::Tool::Capture::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *MR::GUI::MRView::Tool::Capture::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_MR__GUI__MRView__Tool__Capture.stringdata0))
        return static_cast<void*>(this);
    return Base::qt_metacast(_clname);
}

int MR::GUI::MRView::Tool::Capture::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = Base::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 9)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 9;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 9)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 9;
    }
    return _id;
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
