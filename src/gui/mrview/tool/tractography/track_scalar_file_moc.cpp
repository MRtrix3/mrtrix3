/****************************************************************************
** Meta object code from reading C++ file 'track_scalar_file.h'
**
** Created: Wed May 8 11:59:14 2013
**      by: The Qt Meta Object Compiler version 63 (Qt 4.8.4)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "track_scalar_file.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'track_scalar_file.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.4. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_MR__GUI__MRView__Tool__TrackScalarFile[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
      11,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: signature, parameters, type, tag, flags
      45,   39,   40,   39, 0x0a,
      75,   39,   39,   39, 0x08,
      98,   39,   39,   39, 0x08,
     122,   39,   39,   39, 0x08,
     151,  144,   39,   39, 0x08,
     180,  144,   39,   39, 0x08,
     209,   39,   39,   39, 0x08,
     241,   39,   39,   39, 0x08,
     273,   39,   39,   39, 0x08,
     297,   39,   39,   39, 0x08,
     328,   39,   39,   39, 0x08,

       0        // eod
};

static const char qt_meta_stringdata_MR__GUI__MRView__Tool__TrackScalarFile[] = {
    "MR::GUI::MRView::Tool::TrackScalarFile\0"
    "\0bool\0open_track_scalar_file_slot()\0"
    "show_colour_bar_slot()\0select_colourmap_slot()\0"
    "on_set_scaling_slot()\0unused\0"
    "threshold_lower_changed(int)\0"
    "threshold_upper_changed(int)\0"
    "threshold_lower_value_changed()\0"
    "threshold_upper_value_changed()\0"
    "invert_colourmap_slot()\0"
    "scalarfile_by_direction_slot()\0"
    "reset_intensity_slot()\0"
};

void MR::GUI::MRView::Tool::TrackScalarFile::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        TrackScalarFile *_t = static_cast<TrackScalarFile *>(_o);
        switch (_id) {
        case 0: { bool _r = _t->open_track_scalar_file_slot();
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = _r; }  break;
        case 1: _t->show_colour_bar_slot(); break;
        case 2: _t->select_colourmap_slot(); break;
        case 3: _t->on_set_scaling_slot(); break;
        case 4: _t->threshold_lower_changed((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 5: _t->threshold_upper_changed((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 6: _t->threshold_lower_value_changed(); break;
        case 7: _t->threshold_upper_value_changed(); break;
        case 8: _t->invert_colourmap_slot(); break;
        case 9: _t->scalarfile_by_direction_slot(); break;
        case 10: _t->reset_intensity_slot(); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData MR::GUI::MRView::Tool::TrackScalarFile::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject MR::GUI::MRView::Tool::TrackScalarFile::staticMetaObject = {
    { &Base::staticMetaObject, qt_meta_stringdata_MR__GUI__MRView__Tool__TrackScalarFile,
      qt_meta_data_MR__GUI__MRView__Tool__TrackScalarFile, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &MR::GUI::MRView::Tool::TrackScalarFile::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *MR::GUI::MRView::Tool::TrackScalarFile::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *MR::GUI::MRView::Tool::TrackScalarFile::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_MR__GUI__MRView__Tool__TrackScalarFile))
        return static_cast<void*>(const_cast< TrackScalarFile*>(this));
    return Base::qt_metacast(_clname);
}

int MR::GUI::MRView::Tool::TrackScalarFile::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = Base::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 11)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 11;
    }
    return _id;
}
QT_END_MOC_NAMESPACE
