/****************************************************************************
** Meta object code from reading C++ file 'overlay.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.10)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../../src/gui/mrview/tool/overlay.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'overlay.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.10. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_MR__GUI__MRView__Tool__Overlay_t {
    QByteArrayData data[20];
    char stringdata0[364];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_MR__GUI__MRView__Tool__Overlay_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_MR__GUI__MRView__Tool__Overlay_t qt_meta_stringdata_MR__GUI__MRView__Tool__Overlay = {
    {
QT_MOC_LITERAL(0, 0, 30), // "MR::GUI::MRView::Tool::Overlay"
QT_MOC_LITERAL(1, 31, 15), // "image_open_slot"
QT_MOC_LITERAL(2, 47, 0), // ""
QT_MOC_LITERAL(3, 48, 16), // "image_close_slot"
QT_MOC_LITERAL(4, 65, 13), // "hide_all_slot"
QT_MOC_LITERAL(5, 79, 17), // "toggle_shown_slot"
QT_MOC_LITERAL(6, 97, 11), // "QModelIndex"
QT_MOC_LITERAL(7, 109, 22), // "selection_changed_slot"
QT_MOC_LITERAL(8, 132, 14), // "QItemSelection"
QT_MOC_LITERAL(9, 147, 21), // "right_click_menu_slot"
QT_MOC_LITERAL(10, 169, 16), // "onSetVolumeIndex"
QT_MOC_LITERAL(11, 186, 11), // "update_slot"
QT_MOC_LITERAL(12, 198, 6), // "unused"
QT_MOC_LITERAL(13, 205, 14), // "values_changed"
QT_MOC_LITERAL(14, 220, 23), // "upper_threshold_changed"
QT_MOC_LITERAL(15, 244, 23), // "lower_threshold_changed"
QT_MOC_LITERAL(16, 268, 29), // "upper_threshold_value_changed"
QT_MOC_LITERAL(17, 298, 29), // "lower_threshold_value_changed"
QT_MOC_LITERAL(18, 328, 15), // "opacity_changed"
QT_MOC_LITERAL(19, 344, 19) // "interpolate_changed"

    },
    "MR::GUI::MRView::Tool::Overlay\0"
    "image_open_slot\0\0image_close_slot\0"
    "hide_all_slot\0toggle_shown_slot\0"
    "QModelIndex\0selection_changed_slot\0"
    "QItemSelection\0right_click_menu_slot\0"
    "onSetVolumeIndex\0update_slot\0unused\0"
    "values_changed\0upper_threshold_changed\0"
    "lower_threshold_changed\0"
    "upper_threshold_value_changed\0"
    "lower_threshold_value_changed\0"
    "opacity_changed\0interpolate_changed"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_MR__GUI__MRView__Tool__Overlay[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
      15,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags
       1,    0,   89,    2, 0x08 /* Private */,
       3,    0,   90,    2, 0x08 /* Private */,
       4,    0,   91,    2, 0x08 /* Private */,
       5,    2,   92,    2, 0x08 /* Private */,
       7,    2,   97,    2, 0x08 /* Private */,
       9,    1,  102,    2, 0x08 /* Private */,
      10,    0,  105,    2, 0x08 /* Private */,
      11,    1,  106,    2, 0x08 /* Private */,
      13,    0,  109,    2, 0x08 /* Private */,
      14,    1,  110,    2, 0x08 /* Private */,
      15,    1,  113,    2, 0x08 /* Private */,
      16,    0,  116,    2, 0x08 /* Private */,
      17,    0,  117,    2, 0x08 /* Private */,
      18,    1,  118,    2, 0x08 /* Private */,
      19,    0,  121,    2, 0x08 /* Private */,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 6, 0x80000000 | 6,    2,    2,
    QMetaType::Void, 0x80000000 | 8, 0x80000000 | 8,    2,    2,
    QMetaType::Void, QMetaType::QPoint,    2,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int,   12,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int,   12,
    QMetaType::Void, QMetaType::Int,   12,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int,   12,
    QMetaType::Void,

       0        // eod
};

void MR::GUI::MRView::Tool::Overlay::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<Overlay *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->image_open_slot(); break;
        case 1: _t->image_close_slot(); break;
        case 2: _t->hide_all_slot(); break;
        case 3: _t->toggle_shown_slot((*reinterpret_cast< const QModelIndex(*)>(_a[1])),(*reinterpret_cast< const QModelIndex(*)>(_a[2]))); break;
        case 4: _t->selection_changed_slot((*reinterpret_cast< const QItemSelection(*)>(_a[1])),(*reinterpret_cast< const QItemSelection(*)>(_a[2]))); break;
        case 5: _t->right_click_menu_slot((*reinterpret_cast< const QPoint(*)>(_a[1]))); break;
        case 6: _t->onSetVolumeIndex(); break;
        case 7: _t->update_slot((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 8: _t->values_changed(); break;
        case 9: _t->upper_threshold_changed((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 10: _t->lower_threshold_changed((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 11: _t->upper_threshold_value_changed(); break;
        case 12: _t->lower_threshold_value_changed(); break;
        case 13: _t->opacity_changed((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 14: _t->interpolate_changed(); break;
        default: ;
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject MR::GUI::MRView::Tool::Overlay::staticMetaObject = { {
    QMetaObject::SuperData::link<Base::staticMetaObject>(),
    qt_meta_stringdata_MR__GUI__MRView__Tool__Overlay.data,
    qt_meta_data_MR__GUI__MRView__Tool__Overlay,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *MR::GUI::MRView::Tool::Overlay::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *MR::GUI::MRView::Tool::Overlay::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_MR__GUI__MRView__Tool__Overlay.stringdata0))
        return static_cast<void*>(this);
    if (!strcmp(_clname, "ColourMapButtonObserver"))
        return static_cast< ColourMapButtonObserver*>(this);
    if (!strcmp(_clname, "DisplayableVisitor"))
        return static_cast< DisplayableVisitor*>(this);
    return Base::qt_metacast(_clname);
}

int MR::GUI::MRView::Tool::Overlay::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = Base::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 15)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 15;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 15)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 15;
    }
    return _id;
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
