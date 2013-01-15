/****************************************************************************
** Meta object code from reading C++ file 'tractography.h'
**
** Created: Tue Jan 15 15:32:11 2013
**      by: The Qt Meta Object Compiler version 63 (Qt 4.8.4)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "tractography.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'tractography.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.4. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_MR__GUI__MRView__Tool__Tractography[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
       7,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: signature, parameters, type, tag, flags
      37,   36,   36,   36, 0x08,
      60,   36,   36,   36, 0x08,
      90,   84,   36,   36, 0x08,
     116,   36,   36,   36, 0x08,
     151,  143,   36,   36, 0x08,
     188,  180,   36,   36, 0x08,
     216,  206,   36,   36, 0x08,

       0        // eod
};

static const char qt_meta_stringdata_MR__GUI__MRView__Tool__Tractography[] = {
    "MR::GUI::MRView::Tool::Tractography\0"
    "\0tractogram_open_slot()\0tractogram_close_slot()\0"
    "index\0toggle_shown(QModelIndex)\0"
    "on_slab_thickness_change()\0checked\0"
    "on_crop_to_slab_change(bool)\0opacity\0"
    "opacity_slot(int)\0thickness\0"
    "line_thickness_slot(int)\0"
};

void MR::GUI::MRView::Tool::Tractography::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        Tractography *_t = static_cast<Tractography *>(_o);
        switch (_id) {
        case 0: _t->tractogram_open_slot(); break;
        case 1: _t->tractogram_close_slot(); break;
        case 2: _t->toggle_shown((*reinterpret_cast< const QModelIndex(*)>(_a[1]))); break;
        case 3: _t->on_slab_thickness_change(); break;
        case 4: _t->on_crop_to_slab_change((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 5: _t->opacity_slot((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 6: _t->line_thickness_slot((*reinterpret_cast< int(*)>(_a[1]))); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData MR::GUI::MRView::Tool::Tractography::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject MR::GUI::MRView::Tool::Tractography::staticMetaObject = {
    { &Base::staticMetaObject, qt_meta_stringdata_MR__GUI__MRView__Tool__Tractography,
      qt_meta_data_MR__GUI__MRView__Tool__Tractography, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &MR::GUI::MRView::Tool::Tractography::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *MR::GUI::MRView::Tool::Tractography::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *MR::GUI::MRView::Tool::Tractography::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_MR__GUI__MRView__Tool__Tractography))
        return static_cast<void*>(const_cast< Tractography*>(this));
    return Base::qt_metacast(_clname);
}

int MR::GUI::MRView::Tool::Tractography::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = Base::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 7)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 7;
    }
    return _id;
}
QT_END_MOC_NAMESPACE
