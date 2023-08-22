/****************************************************************************
** Meta object code from reading C++ file 'preview.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.10)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../../../src/gui/mrview/tool/odf/preview.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'preview.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.10. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_MR__GUI__MRView__Tool__ODF_Preview_t {
    QByteArrayData data[7];
    char stringdata0[143];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_MR__GUI__MRView__Tool__ODF_Preview_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_MR__GUI__MRView__Tool__ODF_Preview_t qt_meta_stringdata_MR__GUI__MRView__Tool__ODF_Preview = {
    {
QT_MOC_LITERAL(0, 0, 34), // "MR::GUI::MRView::Tool::ODF_Pr..."
QT_MOC_LITERAL(1, 35, 30), // "lock_orientation_to_image_slot"
QT_MOC_LITERAL(2, 66, 0), // ""
QT_MOC_LITERAL(3, 67, 18), // "interpolation_slot"
QT_MOC_LITERAL(4, 86, 14), // "show_axes_slot"
QT_MOC_LITERAL(5, 101, 20), // "level_of_detail_slot"
QT_MOC_LITERAL(6, 122, 20) // "lighting_update_slot"

    },
    "MR::GUI::MRView::Tool::ODF_Preview\0"
    "lock_orientation_to_image_slot\0\0"
    "interpolation_slot\0show_axes_slot\0"
    "level_of_detail_slot\0lighting_update_slot"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_MR__GUI__MRView__Tool__ODF_Preview[] = {

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
       1,    1,   39,    2, 0x08 /* Private */,
       3,    1,   42,    2, 0x08 /* Private */,
       4,    1,   45,    2, 0x08 /* Private */,
       5,    1,   48,    2, 0x08 /* Private */,
       6,    0,   51,    2, 0x08 /* Private */,

 // slots: parameters
    QMetaType::Void, QMetaType::Int,    2,
    QMetaType::Void, QMetaType::Int,    2,
    QMetaType::Void, QMetaType::Int,    2,
    QMetaType::Void, QMetaType::Int,    2,
    QMetaType::Void,

       0        // eod
};

void MR::GUI::MRView::Tool::ODF_Preview::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<ODF_Preview *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->lock_orientation_to_image_slot((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 1: _t->interpolation_slot((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 2: _t->show_axes_slot((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 3: _t->level_of_detail_slot((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 4: _t->lighting_update_slot(); break;
        default: ;
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject MR::GUI::MRView::Tool::ODF_Preview::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_meta_stringdata_MR__GUI__MRView__Tool__ODF_Preview.data,
    qt_meta_data_MR__GUI__MRView__Tool__ODF_Preview,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *MR::GUI::MRView::Tool::ODF_Preview::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *MR::GUI::MRView::Tool::ODF_Preview::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_MR__GUI__MRView__Tool__ODF_Preview.stringdata0))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int MR::GUI::MRView::Tool::ODF_Preview::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
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
