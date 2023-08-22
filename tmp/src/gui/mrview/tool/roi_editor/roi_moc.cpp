/****************************************************************************
** Meta object code from reading C++ file 'roi.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.10)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../../../src/gui/mrview/tool/roi_editor/roi.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'roi.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.10. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_MR__GUI__MRView__Tool__ROI_t {
    QByteArrayData data[21];
    char stringdata0[270];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_MR__GUI__MRView__Tool__ROI_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_MR__GUI__MRView__Tool__ROI_t qt_meta_stringdata_MR__GUI__MRView__Tool__ROI = {
    {
QT_MOC_LITERAL(0, 0, 26), // "MR::GUI::MRView::Tool::ROI"
QT_MOC_LITERAL(1, 27, 8), // "new_slot"
QT_MOC_LITERAL(2, 36, 0), // ""
QT_MOC_LITERAL(3, 37, 9), // "open_slot"
QT_MOC_LITERAL(4, 47, 9), // "save_slot"
QT_MOC_LITERAL(5, 57, 10), // "close_slot"
QT_MOC_LITERAL(6, 68, 9), // "draw_slot"
QT_MOC_LITERAL(7, 78, 9), // "undo_slot"
QT_MOC_LITERAL(8, 88, 9), // "redo_slot"
QT_MOC_LITERAL(9, 98, 13), // "hide_all_slot"
QT_MOC_LITERAL(10, 112, 15), // "slice_copy_slot"
QT_MOC_LITERAL(11, 128, 8), // "QAction*"
QT_MOC_LITERAL(12, 137, 16), // "select_edit_mode"
QT_MOC_LITERAL(13, 154, 17), // "toggle_shown_slot"
QT_MOC_LITERAL(14, 172, 11), // "QModelIndex"
QT_MOC_LITERAL(15, 184, 16), // "update_selection"
QT_MOC_LITERAL(16, 201, 11), // "update_slot"
QT_MOC_LITERAL(17, 213, 14), // "colour_changed"
QT_MOC_LITERAL(18, 228, 15), // "opacity_changed"
QT_MOC_LITERAL(19, 244, 6), // "unused"
QT_MOC_LITERAL(20, 251, 18) // "model_rows_changed"

    },
    "MR::GUI::MRView::Tool::ROI\0new_slot\0"
    "\0open_slot\0save_slot\0close_slot\0"
    "draw_slot\0undo_slot\0redo_slot\0"
    "hide_all_slot\0slice_copy_slot\0QAction*\0"
    "select_edit_mode\0toggle_shown_slot\0"
    "QModelIndex\0update_selection\0update_slot\0"
    "colour_changed\0opacity_changed\0unused\0"
    "model_rows_changed"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_MR__GUI__MRView__Tool__ROI[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
      16,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags
       1,    0,   94,    2, 0x08 /* Private */,
       3,    0,   95,    2, 0x08 /* Private */,
       4,    0,   96,    2, 0x08 /* Private */,
       5,    0,   97,    2, 0x08 /* Private */,
       6,    0,   98,    2, 0x08 /* Private */,
       7,    0,   99,    2, 0x08 /* Private */,
       8,    0,  100,    2, 0x08 /* Private */,
       9,    0,  101,    2, 0x08 /* Private */,
      10,    1,  102,    2, 0x08 /* Private */,
      12,    1,  105,    2, 0x08 /* Private */,
      13,    2,  108,    2, 0x08 /* Private */,
      15,    0,  113,    2, 0x08 /* Private */,
      16,    0,  114,    2, 0x08 /* Private */,
      17,    0,  115,    2, 0x08 /* Private */,
      18,    1,  116,    2, 0x08 /* Private */,
      20,    0,  119,    2, 0x08 /* Private */,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 11,    2,
    QMetaType::Void, 0x80000000 | 11,    2,
    QMetaType::Void, 0x80000000 | 14, 0x80000000 | 14,    2,    2,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int,   19,
    QMetaType::Void,

       0        // eod
};

void MR::GUI::MRView::Tool::ROI::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<ROI *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->new_slot(); break;
        case 1: _t->open_slot(); break;
        case 2: _t->save_slot(); break;
        case 3: _t->close_slot(); break;
        case 4: _t->draw_slot(); break;
        case 5: _t->undo_slot(); break;
        case 6: _t->redo_slot(); break;
        case 7: _t->hide_all_slot(); break;
        case 8: _t->slice_copy_slot((*reinterpret_cast< QAction*(*)>(_a[1]))); break;
        case 9: _t->select_edit_mode((*reinterpret_cast< QAction*(*)>(_a[1]))); break;
        case 10: _t->toggle_shown_slot((*reinterpret_cast< const QModelIndex(*)>(_a[1])),(*reinterpret_cast< const QModelIndex(*)>(_a[2]))); break;
        case 11: _t->update_selection(); break;
        case 12: _t->update_slot(); break;
        case 13: _t->colour_changed(); break;
        case 14: _t->opacity_changed((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 15: _t->model_rows_changed(); break;
        default: ;
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject MR::GUI::MRView::Tool::ROI::staticMetaObject = { {
    QMetaObject::SuperData::link<Base::staticMetaObject>(),
    qt_meta_stringdata_MR__GUI__MRView__Tool__ROI.data,
    qt_meta_data_MR__GUI__MRView__Tool__ROI,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *MR::GUI::MRView::Tool::ROI::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *MR::GUI::MRView::Tool::ROI::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_MR__GUI__MRView__Tool__ROI.stringdata0))
        return static_cast<void*>(this);
    return Base::qt_metacast(_clname);
}

int MR::GUI::MRView::Tool::ROI::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = Base::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 16)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 16;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 16)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 16;
    }
    return _id;
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
