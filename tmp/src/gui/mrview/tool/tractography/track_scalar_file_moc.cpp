/****************************************************************************
** Meta object code from reading C++ file 'track_scalar_file.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.10)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../../../src/gui/mrview/tool/tractography/track_scalar_file.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'track_scalar_file.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.10. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_MR__GUI__MRView__Tool__TrackScalarFileOptions_t {
    QByteArrayData data[11];
    char stringdata0[259];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_MR__GUI__MRView__Tool__TrackScalarFileOptions_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_MR__GUI__MRView__Tool__TrackScalarFileOptions_t qt_meta_stringdata_MR__GUI__MRView__Tool__TrackScalarFileOptions = {
    {
QT_MOC_LITERAL(0, 0, 45), // "MR::GUI::MRView::Tool::TrackS..."
QT_MOC_LITERAL(1, 46, 37), // "open_intensity_track_scalar_f..."
QT_MOC_LITERAL(2, 84, 0), // ""
QT_MOC_LITERAL(3, 85, 11), // "std::string"
QT_MOC_LITERAL(4, 97, 19), // "on_set_scaling_slot"
QT_MOC_LITERAL(5, 117, 26), // "threshold_scalar_file_slot"
QT_MOC_LITERAL(6, 144, 23), // "threshold_lower_changed"
QT_MOC_LITERAL(7, 168, 6), // "unused"
QT_MOC_LITERAL(8, 175, 23), // "threshold_upper_changed"
QT_MOC_LITERAL(9, 199, 29), // "threshold_lower_value_changed"
QT_MOC_LITERAL(10, 229, 29) // "threshold_upper_value_changed"

    },
    "MR::GUI::MRView::Tool::TrackScalarFileOptions\0"
    "open_intensity_track_scalar_file_slot\0"
    "\0std::string\0on_set_scaling_slot\0"
    "threshold_scalar_file_slot\0"
    "threshold_lower_changed\0unused\0"
    "threshold_upper_changed\0"
    "threshold_lower_value_changed\0"
    "threshold_upper_value_changed"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_MR__GUI__MRView__Tool__TrackScalarFileOptions[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       8,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags
       1,    0,   54,    2, 0x0a /* Public */,
       1,    1,   55,    2, 0x0a /* Public */,
       4,    0,   58,    2, 0x08 /* Private */,
       5,    1,   59,    2, 0x08 /* Private */,
       6,    1,   62,    2, 0x08 /* Private */,
       8,    1,   65,    2, 0x08 /* Private */,
       9,    0,   68,    2, 0x08 /* Private */,
      10,    0,   69,    2, 0x08 /* Private */,

 // slots: parameters
    QMetaType::Bool,
    QMetaType::Bool, 0x80000000 | 3,    2,
    QMetaType::Void,
    QMetaType::Bool, QMetaType::Int,    2,
    QMetaType::Void, QMetaType::Int,    7,
    QMetaType::Void, QMetaType::Int,    7,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

void MR::GUI::MRView::Tool::TrackScalarFileOptions::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<TrackScalarFileOptions *>(_o);
        (void)_t;
        switch (_id) {
        case 0: { bool _r = _t->open_intensity_track_scalar_file_slot();
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 1: { bool _r = _t->open_intensity_track_scalar_file_slot((*reinterpret_cast< std::string(*)>(_a[1])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 2: _t->on_set_scaling_slot(); break;
        case 3: { bool _r = _t->threshold_scalar_file_slot((*reinterpret_cast< int(*)>(_a[1])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 4: _t->threshold_lower_changed((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 5: _t->threshold_upper_changed((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 6: _t->threshold_lower_value_changed(); break;
        case 7: _t->threshold_upper_value_changed(); break;
        default: ;
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject MR::GUI::MRView::Tool::TrackScalarFileOptions::staticMetaObject = { {
    QMetaObject::SuperData::link<QGroupBox::staticMetaObject>(),
    qt_meta_stringdata_MR__GUI__MRView__Tool__TrackScalarFileOptions.data,
    qt_meta_data_MR__GUI__MRView__Tool__TrackScalarFileOptions,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *MR::GUI::MRView::Tool::TrackScalarFileOptions::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *MR::GUI::MRView::Tool::TrackScalarFileOptions::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_MR__GUI__MRView__Tool__TrackScalarFileOptions.stringdata0))
        return static_cast<void*>(this);
    if (!strcmp(_clname, "ColourMapButtonObserver"))
        return static_cast< ColourMapButtonObserver*>(this);
    if (!strcmp(_clname, "DisplayableVisitor"))
        return static_cast< DisplayableVisitor*>(this);
    return QGroupBox::qt_metacast(_clname);
}

int MR::GUI::MRView::Tool::TrackScalarFileOptions::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QGroupBox::qt_metacall(_c, _id, _a);
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
QT_WARNING_POP
QT_END_MOC_NAMESPACE
