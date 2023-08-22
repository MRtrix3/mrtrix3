/****************************************************************************
** Meta object code from reading C++ file 'lightbox.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.10)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../../src/gui/mrview/mode/lightbox.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'lightbox.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.10. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_MR__GUI__MRView__Mode__LightBox_t {
    QByteArrayData data[11];
    char stringdata0[176];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_MR__GUI__MRView__Mode__LightBox_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_MR__GUI__MRView__Mode__LightBox_t qt_meta_stringdata_MR__GUI__MRView__Mode__LightBox = {
    {
QT_MOC_LITERAL(0, 0, 31), // "MR::GUI::MRView::Mode::LightBox"
QT_MOC_LITERAL(1, 32, 21), // "slice_increment_reset"
QT_MOC_LITERAL(2, 54, 0), // ""
QT_MOC_LITERAL(3, 55, 10), // "nrows_slot"
QT_MOC_LITERAL(4, 66, 5), // "value"
QT_MOC_LITERAL(5, 72, 13), // "ncolumns_slot"
QT_MOC_LITERAL(6, 86, 14), // "slice_inc_slot"
QT_MOC_LITERAL(7, 101, 15), // "volume_inc_slot"
QT_MOC_LITERAL(8, 117, 14), // "show_grid_slot"
QT_MOC_LITERAL(9, 132, 17), // "show_volumes_slot"
QT_MOC_LITERAL(10, 150, 25) // "image_volume_changed_slot"

    },
    "MR::GUI::MRView::Mode::LightBox\0"
    "slice_increment_reset\0\0nrows_slot\0"
    "value\0ncolumns_slot\0slice_inc_slot\0"
    "volume_inc_slot\0show_grid_slot\0"
    "show_volumes_slot\0image_volume_changed_slot"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_MR__GUI__MRView__Mode__LightBox[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       8,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       1,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    0,   54,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
       3,    1,   55,    2, 0x0a /* Public */,
       5,    1,   58,    2, 0x0a /* Public */,
       6,    1,   61,    2, 0x0a /* Public */,
       7,    1,   64,    2, 0x0a /* Public */,
       8,    1,   67,    2, 0x0a /* Public */,
       9,    1,   70,    2, 0x0a /* Public */,
      10,    0,   73,    2, 0x0a /* Public */,

 // signals: parameters
    QMetaType::Void,

 // slots: parameters
    QMetaType::Void, QMetaType::Int,    4,
    QMetaType::Void, QMetaType::Int,    4,
    QMetaType::Void, QMetaType::Float,    4,
    QMetaType::Void, QMetaType::Int,    4,
    QMetaType::Void, QMetaType::Bool,    4,
    QMetaType::Void, QMetaType::Bool,    4,
    QMetaType::Void,

       0        // eod
};

void MR::GUI::MRView::Mode::LightBox::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<LightBox *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->slice_increment_reset(); break;
        case 1: _t->nrows_slot((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 2: _t->ncolumns_slot((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 3: _t->slice_inc_slot((*reinterpret_cast< float(*)>(_a[1]))); break;
        case 4: _t->volume_inc_slot((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 5: _t->show_grid_slot((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 6: _t->show_volumes_slot((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 7: _t->image_volume_changed_slot(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (LightBox::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&LightBox::slice_increment_reset)) {
                *result = 0;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject MR::GUI::MRView::Mode::LightBox::staticMetaObject = { {
    QMetaObject::SuperData::link<Slice::staticMetaObject>(),
    qt_meta_stringdata_MR__GUI__MRView__Mode__LightBox.data,
    qt_meta_data_MR__GUI__MRView__Mode__LightBox,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *MR::GUI::MRView::Mode::LightBox::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *MR::GUI::MRView::Mode::LightBox::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_MR__GUI__MRView__Mode__LightBox.stringdata0))
        return static_cast<void*>(this);
    return Slice::qt_metacast(_clname);
}

int MR::GUI::MRView::Mode::LightBox::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = Slice::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 8)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 8;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 8)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 8;
    }
    return _id;
}

// SIGNAL 0
void MR::GUI::MRView::Mode::LightBox::slice_increment_reset()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
