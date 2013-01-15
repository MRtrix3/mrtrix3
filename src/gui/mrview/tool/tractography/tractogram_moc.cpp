/****************************************************************************
** Meta object code from reading C++ file 'tractogram.h'
**
** Created: Tue Jan 15 16:18:39 2013
**      by: The Qt Meta Object Compiler version 63 (Qt 4.8.4)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "tractogram.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'tractogram.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.4. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_MR__GUI__MRView__Tool__Tractogram[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
       1,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       1,       // signalCount

 // signals: signature, parameters, type, tag, flags
      35,   34,   34,   34, 0x05,

       0        // eod
};

static const char qt_meta_stringdata_MR__GUI__MRView__Tool__Tractogram[] = {
    "MR::GUI::MRView::Tool::Tractogram\0\0"
    "scalingChanged()\0"
};

void MR::GUI::MRView::Tool::Tractogram::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        Tractogram *_t = static_cast<Tractogram *>(_o);
        switch (_id) {
        case 0: _t->scalingChanged(); break;
        default: ;
        }
    }
    Q_UNUSED(_a);
}

const QMetaObjectExtraData MR::GUI::MRView::Tool::Tractogram::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject MR::GUI::MRView::Tool::Tractogram::staticMetaObject = {
    { &Displayable::staticMetaObject, qt_meta_stringdata_MR__GUI__MRView__Tool__Tractogram,
      qt_meta_data_MR__GUI__MRView__Tool__Tractogram, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &MR::GUI::MRView::Tool::Tractogram::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *MR::GUI::MRView::Tool::Tractogram::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *MR::GUI::MRView::Tool::Tractogram::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_MR__GUI__MRView__Tool__Tractogram))
        return static_cast<void*>(const_cast< Tractogram*>(this));
    return Displayable::qt_metacast(_clname);
}

int MR::GUI::MRView::Tool::Tractogram::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = Displayable::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 1)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 1;
    }
    return _id;
}

// SIGNAL 0
void MR::GUI::MRView::Tool::Tractogram::scalingChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 0, 0);
}
QT_END_MOC_NAMESPACE
