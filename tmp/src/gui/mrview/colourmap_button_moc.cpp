/****************************************************************************
** Meta object code from reading C++ file 'colourmap_button.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.10)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../src/gui/mrview/colourmap_button.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'colourmap_button.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.10. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_MR__GUI__MRView__ColourMapButton_t {
    QByteArrayData data[12];
    char stringdata0[198];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_MR__GUI__MRView__ColourMapButton_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_MR__GUI__MRView__ColourMapButton_t qt_meta_stringdata_MR__GUI__MRView__ColourMapButton = {
    {
QT_MOC_LITERAL(0, 0, 32), // "MR::GUI::MRView::ColourMapButton"
QT_MOC_LITERAL(1, 33, 21), // "select_colourmap_slot"
QT_MOC_LITERAL(2, 55, 0), // ""
QT_MOC_LITERAL(3, 56, 8), // "QAction*"
QT_MOC_LITERAL(4, 65, 6), // "action"
QT_MOC_LITERAL(5, 72, 18), // "select_colour_slot"
QT_MOC_LITERAL(6, 91, 25), // "select_random_colour_slot"
QT_MOC_LITERAL(7, 117, 20), // "show_colour_bar_slot"
QT_MOC_LITERAL(8, 138, 7), // "visible"
QT_MOC_LITERAL(9, 146, 21), // "invert_colourmap_slot"
QT_MOC_LITERAL(10, 168, 8), // "inverted"
QT_MOC_LITERAL(11, 177, 20) // "reset_intensity_slot"

    },
    "MR::GUI::MRView::ColourMapButton\0"
    "select_colourmap_slot\0\0QAction*\0action\0"
    "select_colour_slot\0select_random_colour_slot\0"
    "show_colour_bar_slot\0visible\0"
    "invert_colourmap_slot\0inverted\0"
    "reset_intensity_slot"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_MR__GUI__MRView__ColourMapButton[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       6,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags
       1,    1,   44,    2, 0x08 /* Private */,
       5,    0,   47,    2, 0x08 /* Private */,
       6,    0,   48,    2, 0x08 /* Private */,
       7,    1,   49,    2, 0x08 /* Private */,
       9,    1,   52,    2, 0x08 /* Private */,
      11,    0,   55,    2, 0x08 /* Private */,

 // slots: parameters
    QMetaType::Void, 0x80000000 | 3,    4,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Bool,    8,
    QMetaType::Void, QMetaType::Bool,   10,
    QMetaType::Void,

       0        // eod
};

void MR::GUI::MRView::ColourMapButton::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<ColourMapButton *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->select_colourmap_slot((*reinterpret_cast< QAction*(*)>(_a[1]))); break;
        case 1: _t->select_colour_slot(); break;
        case 2: _t->select_random_colour_slot(); break;
        case 3: _t->show_colour_bar_slot((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 4: _t->invert_colourmap_slot((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 5: _t->reset_intensity_slot(); break;
        default: ;
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject MR::GUI::MRView::ColourMapButton::staticMetaObject = { {
    QMetaObject::SuperData::link<QToolButton::staticMetaObject>(),
    qt_meta_stringdata_MR__GUI__MRView__ColourMapButton.data,
    qt_meta_data_MR__GUI__MRView__ColourMapButton,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *MR::GUI::MRView::ColourMapButton::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *MR::GUI::MRView::ColourMapButton::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_MR__GUI__MRView__ColourMapButton.stringdata0))
        return static_cast<void*>(this);
    return QToolButton::qt_metacast(_clname);
}

int MR::GUI::MRView::ColourMapButton::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QToolButton::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 6)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 6;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 6)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 6;
    }
    return _id;
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
