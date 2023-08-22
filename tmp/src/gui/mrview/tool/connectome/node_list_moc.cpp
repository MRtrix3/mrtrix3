/****************************************************************************
** Meta object code from reading C++ file 'node_list.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.10)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../../../src/gui/mrview/tool/connectome/node_list.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'node_list.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.10. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_MR__GUI__MRView__Tool__Node_list_t {
    QByteArrayData data[6];
    char stringdata0[134];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_MR__GUI__MRView__Tool__Node_list_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_MR__GUI__MRView__Tool__Node_list_t qt_meta_stringdata_MR__GUI__MRView__Tool__Node_list = {
    {
QT_MOC_LITERAL(0, 0, 32), // "MR::GUI::MRView::Tool::Node_list"
QT_MOC_LITERAL(1, 33, 20), // "clear_selection_slot"
QT_MOC_LITERAL(2, 54, 0), // ""
QT_MOC_LITERAL(3, 55, 27), // "node_selection_changed_slot"
QT_MOC_LITERAL(4, 83, 14), // "QItemSelection"
QT_MOC_LITERAL(5, 98, 35) // "node_selection_settings_dialo..."

    },
    "MR::GUI::MRView::Tool::Node_list\0"
    "clear_selection_slot\0\0node_selection_changed_slot\0"
    "QItemSelection\0node_selection_settings_dialog_slot"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_MR__GUI__MRView__Tool__Node_list[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       3,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags
       1,    0,   29,    2, 0x08 /* Private */,
       3,    2,   30,    2, 0x08 /* Private */,
       5,    0,   35,    2, 0x08 /* Private */,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 4, 0x80000000 | 4,    2,    2,
    QMetaType::Void,

       0        // eod
};

void MR::GUI::MRView::Tool::Node_list::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<Node_list *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->clear_selection_slot(); break;
        case 1: _t->node_selection_changed_slot((*reinterpret_cast< const QItemSelection(*)>(_a[1])),(*reinterpret_cast< const QItemSelection(*)>(_a[2]))); break;
        case 2: _t->node_selection_settings_dialog_slot(); break;
        default: ;
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject MR::GUI::MRView::Tool::Node_list::staticMetaObject = { {
    QMetaObject::SuperData::link<Tool::Base::staticMetaObject>(),
    qt_meta_stringdata_MR__GUI__MRView__Tool__Node_list.data,
    qt_meta_data_MR__GUI__MRView__Tool__Node_list,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *MR::GUI::MRView::Tool::Node_list::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *MR::GUI::MRView::Tool::Node_list::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_MR__GUI__MRView__Tool__Node_list.stringdata0))
        return static_cast<void*>(this);
    return Tool::Base::qt_metacast(_clname);
}

int MR::GUI::MRView::Tool::Node_list::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = Tool::Base::qt_metacall(_c, _id, _a);
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
QT_WARNING_POP
QT_END_MOC_NAMESPACE
