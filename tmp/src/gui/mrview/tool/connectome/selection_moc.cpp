/****************************************************************************
** Meta object code from reading C++ file 'selection.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.10)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../../../src/gui/mrview/tool/connectome/selection.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'selection.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.10. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_MR__GUI__MRView__Tool__NodeSelectionSettings_t {
    QByteArrayData data[3];
    char stringdata0[58];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_MR__GUI__MRView__Tool__NodeSelectionSettings_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_MR__GUI__MRView__Tool__NodeSelectionSettings_t qt_meta_stringdata_MR__GUI__MRView__Tool__NodeSelectionSettings = {
    {
QT_MOC_LITERAL(0, 0, 44), // "MR::GUI::MRView::Tool::NodeSe..."
QT_MOC_LITERAL(1, 45, 11), // "dataChanged"
QT_MOC_LITERAL(2, 57, 0) // ""

    },
    "MR::GUI::MRView::Tool::NodeSelectionSettings\0"
    "dataChanged\0"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_MR__GUI__MRView__Tool__NodeSelectionSettings[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       1,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       1,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    0,   19,    2, 0x06 /* Public */,

 // signals: parameters
    QMetaType::Void,

       0        // eod
};

void MR::GUI::MRView::Tool::NodeSelectionSettings::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<NodeSelectionSettings *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->dataChanged(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (NodeSelectionSettings::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&NodeSelectionSettings::dataChanged)) {
                *result = 0;
                return;
            }
        }
    }
    (void)_a;
}

QT_INIT_METAOBJECT const QMetaObject MR::GUI::MRView::Tool::NodeSelectionSettings::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_MR__GUI__MRView__Tool__NodeSelectionSettings.data,
    qt_meta_data_MR__GUI__MRView__Tool__NodeSelectionSettings,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *MR::GUI::MRView::Tool::NodeSelectionSettings::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *MR::GUI::MRView::Tool::NodeSelectionSettings::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_MR__GUI__MRView__Tool__NodeSelectionSettings.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int MR::GUI::MRView::Tool::NodeSelectionSettings::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 1)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 1;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 1)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 1;
    }
    return _id;
}

// SIGNAL 0
void MR::GUI::MRView::Tool::NodeSelectionSettings::dataChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}
struct qt_meta_stringdata_MR__GUI__MRView__Tool__NodeSelectionSettingsFrame_t {
    QByteArrayData data[30];
    char stringdata0[793];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_MR__GUI__MRView__Tool__NodeSelectionSettingsFrame_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_MR__GUI__MRView__Tool__NodeSelectionSettingsFrame_t qt_meta_stringdata_MR__GUI__MRView__Tool__NodeSelectionSettingsFrame = {
    {
QT_MOC_LITERAL(0, 0, 49), // "MR::GUI::MRView::Tool::NodeSe..."
QT_MOC_LITERAL(1, 50, 29), // "node_selected_visibility_slot"
QT_MOC_LITERAL(2, 80, 0), // ""
QT_MOC_LITERAL(3, 81, 30), // "node_selected_colour_fade_slot"
QT_MOC_LITERAL(4, 112, 25), // "node_selected_colour_slot"
QT_MOC_LITERAL(5, 138, 23), // "node_selected_size_slot"
QT_MOC_LITERAL(6, 162, 24), // "node_selected_alpha_slot"
QT_MOC_LITERAL(7, 187, 29), // "edge_selected_visibility_slot"
QT_MOC_LITERAL(8, 217, 30), // "edge_selected_colour_fade_slot"
QT_MOC_LITERAL(9, 248, 25), // "edge_selected_colour_slot"
QT_MOC_LITERAL(10, 274, 23), // "edge_selected_size_slot"
QT_MOC_LITERAL(11, 298, 24), // "edge_selected_alpha_slot"
QT_MOC_LITERAL(12, 323, 32), // "node_associated_colour_fade_slot"
QT_MOC_LITERAL(13, 356, 27), // "node_associated_colour_slot"
QT_MOC_LITERAL(14, 384, 25), // "node_associated_size_slot"
QT_MOC_LITERAL(15, 410, 26), // "node_associated_alpha_slot"
QT_MOC_LITERAL(16, 437, 32), // "edge_associated_colour_fade_slot"
QT_MOC_LITERAL(17, 470, 27), // "edge_associated_colour_slot"
QT_MOC_LITERAL(18, 498, 25), // "edge_associated_size_slot"
QT_MOC_LITERAL(19, 524, 26), // "edge_associated_alpha_slot"
QT_MOC_LITERAL(20, 551, 26), // "node_other_visibility_slot"
QT_MOC_LITERAL(21, 578, 27), // "node_other_colour_fade_slot"
QT_MOC_LITERAL(22, 606, 22), // "node_other_colour_slot"
QT_MOC_LITERAL(23, 629, 20), // "node_other_size_slot"
QT_MOC_LITERAL(24, 650, 21), // "node_other_alpha_slot"
QT_MOC_LITERAL(25, 672, 26), // "edge_other_visibility_slot"
QT_MOC_LITERAL(26, 699, 27), // "edge_other_colour_fade_slot"
QT_MOC_LITERAL(27, 727, 22), // "edge_other_colour_slot"
QT_MOC_LITERAL(28, 750, 20), // "edge_other_size_slot"
QT_MOC_LITERAL(29, 771, 21) // "edge_other_alpha_slot"

    },
    "MR::GUI::MRView::Tool::NodeSelectionSettingsFrame\0"
    "node_selected_visibility_slot\0\0"
    "node_selected_colour_fade_slot\0"
    "node_selected_colour_slot\0"
    "node_selected_size_slot\0"
    "node_selected_alpha_slot\0"
    "edge_selected_visibility_slot\0"
    "edge_selected_colour_fade_slot\0"
    "edge_selected_colour_slot\0"
    "edge_selected_size_slot\0"
    "edge_selected_alpha_slot\0"
    "node_associated_colour_fade_slot\0"
    "node_associated_colour_slot\0"
    "node_associated_size_slot\0"
    "node_associated_alpha_slot\0"
    "edge_associated_colour_fade_slot\0"
    "edge_associated_colour_slot\0"
    "edge_associated_size_slot\0"
    "edge_associated_alpha_slot\0"
    "node_other_visibility_slot\0"
    "node_other_colour_fade_slot\0"
    "node_other_colour_slot\0node_other_size_slot\0"
    "node_other_alpha_slot\0edge_other_visibility_slot\0"
    "edge_other_colour_fade_slot\0"
    "edge_other_colour_slot\0edge_other_size_slot\0"
    "edge_other_alpha_slot"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_MR__GUI__MRView__Tool__NodeSelectionSettingsFrame[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
      28,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags
       1,    0,  154,    2, 0x09 /* Protected */,
       3,    0,  155,    2, 0x09 /* Protected */,
       4,    0,  156,    2, 0x09 /* Protected */,
       5,    0,  157,    2, 0x09 /* Protected */,
       6,    0,  158,    2, 0x09 /* Protected */,
       7,    0,  159,    2, 0x09 /* Protected */,
       8,    0,  160,    2, 0x09 /* Protected */,
       9,    0,  161,    2, 0x09 /* Protected */,
      10,    0,  162,    2, 0x09 /* Protected */,
      11,    0,  163,    2, 0x09 /* Protected */,
      12,    0,  164,    2, 0x09 /* Protected */,
      13,    0,  165,    2, 0x09 /* Protected */,
      14,    0,  166,    2, 0x09 /* Protected */,
      15,    0,  167,    2, 0x09 /* Protected */,
      16,    0,  168,    2, 0x09 /* Protected */,
      17,    0,  169,    2, 0x09 /* Protected */,
      18,    0,  170,    2, 0x09 /* Protected */,
      19,    0,  171,    2, 0x09 /* Protected */,
      20,    0,  172,    2, 0x09 /* Protected */,
      21,    0,  173,    2, 0x09 /* Protected */,
      22,    0,  174,    2, 0x09 /* Protected */,
      23,    0,  175,    2, 0x09 /* Protected */,
      24,    0,  176,    2, 0x09 /* Protected */,
      25,    0,  177,    2, 0x09 /* Protected */,
      26,    0,  178,    2, 0x09 /* Protected */,
      27,    0,  179,    2, 0x09 /* Protected */,
      28,    0,  180,    2, 0x09 /* Protected */,
      29,    0,  181,    2, 0x09 /* Protected */,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

void MR::GUI::MRView::Tool::NodeSelectionSettingsFrame::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<NodeSelectionSettingsFrame *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->node_selected_visibility_slot(); break;
        case 1: _t->node_selected_colour_fade_slot(); break;
        case 2: _t->node_selected_colour_slot(); break;
        case 3: _t->node_selected_size_slot(); break;
        case 4: _t->node_selected_alpha_slot(); break;
        case 5: _t->edge_selected_visibility_slot(); break;
        case 6: _t->edge_selected_colour_fade_slot(); break;
        case 7: _t->edge_selected_colour_slot(); break;
        case 8: _t->edge_selected_size_slot(); break;
        case 9: _t->edge_selected_alpha_slot(); break;
        case 10: _t->node_associated_colour_fade_slot(); break;
        case 11: _t->node_associated_colour_slot(); break;
        case 12: _t->node_associated_size_slot(); break;
        case 13: _t->node_associated_alpha_slot(); break;
        case 14: _t->edge_associated_colour_fade_slot(); break;
        case 15: _t->edge_associated_colour_slot(); break;
        case 16: _t->edge_associated_size_slot(); break;
        case 17: _t->edge_associated_alpha_slot(); break;
        case 18: _t->node_other_visibility_slot(); break;
        case 19: _t->node_other_colour_fade_slot(); break;
        case 20: _t->node_other_colour_slot(); break;
        case 21: _t->node_other_size_slot(); break;
        case 22: _t->node_other_alpha_slot(); break;
        case 23: _t->edge_other_visibility_slot(); break;
        case 24: _t->edge_other_colour_fade_slot(); break;
        case 25: _t->edge_other_colour_slot(); break;
        case 26: _t->edge_other_size_slot(); break;
        case 27: _t->edge_other_alpha_slot(); break;
        default: ;
        }
    }
    (void)_a;
}

QT_INIT_METAOBJECT const QMetaObject MR::GUI::MRView::Tool::NodeSelectionSettingsFrame::staticMetaObject = { {
    QMetaObject::SuperData::link<QFrame::staticMetaObject>(),
    qt_meta_stringdata_MR__GUI__MRView__Tool__NodeSelectionSettingsFrame.data,
    qt_meta_data_MR__GUI__MRView__Tool__NodeSelectionSettingsFrame,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *MR::GUI::MRView::Tool::NodeSelectionSettingsFrame::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *MR::GUI::MRView::Tool::NodeSelectionSettingsFrame::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_MR__GUI__MRView__Tool__NodeSelectionSettingsFrame.stringdata0))
        return static_cast<void*>(this);
    return QFrame::qt_metacast(_clname);
}

int MR::GUI::MRView::Tool::NodeSelectionSettingsFrame::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QFrame::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 28)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 28;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 28)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 28;
    }
    return _id;
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
