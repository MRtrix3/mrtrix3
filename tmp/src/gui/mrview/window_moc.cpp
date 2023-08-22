/****************************************************************************
** Meta object code from reading C++ file 'window.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.10)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../src/gui/mrview/window.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'window.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.10. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_MR__GUI__MRView__Window_t {
    QByteArrayData data[56];
    char stringdata0[978];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_MR__GUI__MRView__Window_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_MR__GUI__MRView__Window_t qt_meta_stringdata_MR__GUI__MRView__Window = {
    {
QT_MOC_LITERAL(0, 0, 23), // "MR::GUI::MRView::Window"
QT_MOC_LITERAL(1, 24, 12), // "focusChanged"
QT_MOC_LITERAL(2, 37, 0), // ""
QT_MOC_LITERAL(3, 38, 13), // "targetChanged"
QT_MOC_LITERAL(4, 52, 12), // "sliceChanged"
QT_MOC_LITERAL(5, 65, 12), // "planeChanged"
QT_MOC_LITERAL(6, 78, 18), // "orientationChanged"
QT_MOC_LITERAL(7, 97, 18), // "fieldOfViewChanged"
QT_MOC_LITERAL(8, 116, 11), // "modeChanged"
QT_MOC_LITERAL(9, 128, 12), // "imageChanged"
QT_MOC_LITERAL(10, 141, 22), // "imageVisibilityChanged"
QT_MOC_LITERAL(11, 164, 14), // "scalingChanged"
QT_MOC_LITERAL(12, 179, 13), // "volumeChanged"
QT_MOC_LITERAL(13, 193, 11), // "syncChanged"
QT_MOC_LITERAL(14, 205, 18), // "on_scaling_changed"
QT_MOC_LITERAL(15, 224, 8), // "updateGL"
QT_MOC_LITERAL(16, 233, 6), // "drawGL"
QT_MOC_LITERAL(17, 240, 15), // "image_open_slot"
QT_MOC_LITERAL(18, 256, 23), // "image_import_DICOM_slot"
QT_MOC_LITERAL(19, 280, 15), // "image_save_slot"
QT_MOC_LITERAL(20, 296, 16), // "image_close_slot"
QT_MOC_LITERAL(21, 313, 21), // "image_properties_slot"
QT_MOC_LITERAL(22, 335, 16), // "select_mode_slot"
QT_MOC_LITERAL(23, 352, 8), // "QAction*"
QT_MOC_LITERAL(24, 361, 6), // "action"
QT_MOC_LITERAL(25, 368, 22), // "select_mouse_mode_slot"
QT_MOC_LITERAL(26, 391, 16), // "select_tool_slot"
QT_MOC_LITERAL(27, 408, 17), // "select_plane_slot"
QT_MOC_LITERAL(28, 426, 12), // "zoom_in_slot"
QT_MOC_LITERAL(29, 439, 13), // "zoom_out_slot"
QT_MOC_LITERAL(30, 453, 19), // "invert_scaling_slot"
QT_MOC_LITERAL(31, 473, 16), // "full_screen_slot"
QT_MOC_LITERAL(32, 490, 23), // "toggle_annotations_slot"
QT_MOC_LITERAL(33, 514, 18), // "snap_to_image_slot"
QT_MOC_LITERAL(34, 533, 17), // "wrap_volumes_slot"
QT_MOC_LITERAL(35, 551, 9), // "sync_slot"
QT_MOC_LITERAL(36, 561, 15), // "hide_image_slot"
QT_MOC_LITERAL(37, 577, 15), // "slice_next_slot"
QT_MOC_LITERAL(38, 593, 19), // "slice_previous_slot"
QT_MOC_LITERAL(39, 613, 15), // "image_next_slot"
QT_MOC_LITERAL(40, 629, 19), // "image_previous_slot"
QT_MOC_LITERAL(41, 649, 22), // "image_next_volume_slot"
QT_MOC_LITERAL(42, 672, 26), // "image_previous_volume_slot"
QT_MOC_LITERAL(43, 699, 22), // "image_goto_volume_slot"
QT_MOC_LITERAL(44, 722, 28), // "image_next_volume_group_slot"
QT_MOC_LITERAL(45, 751, 28), // "image_goto_volume_group_slot"
QT_MOC_LITERAL(46, 780, 32), // "image_previous_volume_group_slot"
QT_MOC_LITERAL(47, 813, 16), // "image_reset_slot"
QT_MOC_LITERAL(48, 830, 22), // "image_interpolate_slot"
QT_MOC_LITERAL(49, 853, 17), // "image_select_slot"
QT_MOC_LITERAL(50, 871, 15), // "reset_view_slot"
QT_MOC_LITERAL(51, 887, 22), // "background_colour_slot"
QT_MOC_LITERAL(52, 910, 11), // "OpenGL_slot"
QT_MOC_LITERAL(53, 922, 10), // "about_slot"
QT_MOC_LITERAL(54, 933, 12), // "aboutQt_slot"
QT_MOC_LITERAL(55, 946, 31) // "process_commandline_option_slot"

    },
    "MR::GUI::MRView::Window\0focusChanged\0"
    "\0targetChanged\0sliceChanged\0planeChanged\0"
    "orientationChanged\0fieldOfViewChanged\0"
    "modeChanged\0imageChanged\0"
    "imageVisibilityChanged\0scalingChanged\0"
    "volumeChanged\0syncChanged\0on_scaling_changed\0"
    "updateGL\0drawGL\0image_open_slot\0"
    "image_import_DICOM_slot\0image_save_slot\0"
    "image_close_slot\0image_properties_slot\0"
    "select_mode_slot\0QAction*\0action\0"
    "select_mouse_mode_slot\0select_tool_slot\0"
    "select_plane_slot\0zoom_in_slot\0"
    "zoom_out_slot\0invert_scaling_slot\0"
    "full_screen_slot\0toggle_annotations_slot\0"
    "snap_to_image_slot\0wrap_volumes_slot\0"
    "sync_slot\0hide_image_slot\0slice_next_slot\0"
    "slice_previous_slot\0image_next_slot\0"
    "image_previous_slot\0image_next_volume_slot\0"
    "image_previous_volume_slot\0"
    "image_goto_volume_slot\0"
    "image_next_volume_group_slot\0"
    "image_goto_volume_group_slot\0"
    "image_previous_volume_group_slot\0"
    "image_reset_slot\0image_interpolate_slot\0"
    "image_select_slot\0reset_view_slot\0"
    "background_colour_slot\0OpenGL_slot\0"
    "about_slot\0aboutQt_slot\0"
    "process_commandline_option_slot"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_MR__GUI__MRView__Window[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
      52,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
      12,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    0,  274,    2, 0x06 /* Public */,
       3,    0,  275,    2, 0x06 /* Public */,
       4,    0,  276,    2, 0x06 /* Public */,
       5,    0,  277,    2, 0x06 /* Public */,
       6,    0,  278,    2, 0x06 /* Public */,
       7,    0,  279,    2, 0x06 /* Public */,
       8,    0,  280,    2, 0x06 /* Public */,
       9,    0,  281,    2, 0x06 /* Public */,
      10,    1,  282,    2, 0x06 /* Public */,
      11,    0,  285,    2, 0x06 /* Public */,
      12,    0,  286,    2, 0x06 /* Public */,
      13,    0,  287,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
      14,    0,  288,    2, 0x0a /* Public */,
      15,    0,  289,    2, 0x0a /* Public */,
      16,    0,  290,    2, 0x0a /* Public */,
      17,    0,  291,    2, 0x08 /* Private */,
      18,    0,  292,    2, 0x08 /* Private */,
      19,    0,  293,    2, 0x08 /* Private */,
      20,    0,  294,    2, 0x08 /* Private */,
      21,    0,  295,    2, 0x08 /* Private */,
      22,    1,  296,    2, 0x08 /* Private */,
      25,    1,  299,    2, 0x08 /* Private */,
      26,    1,  302,    2, 0x08 /* Private */,
      27,    1,  305,    2, 0x08 /* Private */,
      28,    0,  308,    2, 0x08 /* Private */,
      29,    0,  309,    2, 0x08 /* Private */,
      30,    0,  310,    2, 0x08 /* Private */,
      31,    0,  311,    2, 0x08 /* Private */,
      32,    0,  312,    2, 0x08 /* Private */,
      33,    0,  313,    2, 0x08 /* Private */,
      34,    0,  314,    2, 0x08 /* Private */,
      35,    0,  315,    2, 0x08 /* Private */,
      36,    0,  316,    2, 0x08 /* Private */,
      37,    0,  317,    2, 0x08 /* Private */,
      38,    0,  318,    2, 0x08 /* Private */,
      39,    0,  319,    2, 0x08 /* Private */,
      40,    0,  320,    2, 0x08 /* Private */,
      41,    0,  321,    2, 0x08 /* Private */,
      42,    0,  322,    2, 0x08 /* Private */,
      43,    0,  323,    2, 0x08 /* Private */,
      44,    0,  324,    2, 0x08 /* Private */,
      45,    0,  325,    2, 0x08 /* Private */,
      46,    0,  326,    2, 0x08 /* Private */,
      47,    0,  327,    2, 0x08 /* Private */,
      48,    0,  328,    2, 0x08 /* Private */,
      49,    1,  329,    2, 0x08 /* Private */,
      50,    0,  332,    2, 0x08 /* Private */,
      51,    0,  333,    2, 0x08 /* Private */,
      52,    0,  334,    2, 0x08 /* Private */,
      53,    0,  335,    2, 0x08 /* Private */,
      54,    0,  336,    2, 0x08 /* Private */,
      55,    0,  337,    2, 0x08 /* Private */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Bool,    2,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 23,   24,
    QMetaType::Void, 0x80000000 | 23,   24,
    QMetaType::Void, 0x80000000 | 23,   24,
    QMetaType::Void, 0x80000000 | 23,   24,
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
    QMetaType::Void, 0x80000000 | 23,   24,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

void MR::GUI::MRView::Window::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<Window *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->focusChanged(); break;
        case 1: _t->targetChanged(); break;
        case 2: _t->sliceChanged(); break;
        case 3: _t->planeChanged(); break;
        case 4: _t->orientationChanged(); break;
        case 5: _t->fieldOfViewChanged(); break;
        case 6: _t->modeChanged(); break;
        case 7: _t->imageChanged(); break;
        case 8: _t->imageVisibilityChanged((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 9: _t->scalingChanged(); break;
        case 10: _t->volumeChanged(); break;
        case 11: _t->syncChanged(); break;
        case 12: _t->on_scaling_changed(); break;
        case 13: _t->updateGL(); break;
        case 14: _t->drawGL(); break;
        case 15: _t->image_open_slot(); break;
        case 16: _t->image_import_DICOM_slot(); break;
        case 17: _t->image_save_slot(); break;
        case 18: _t->image_close_slot(); break;
        case 19: _t->image_properties_slot(); break;
        case 20: _t->select_mode_slot((*reinterpret_cast< QAction*(*)>(_a[1]))); break;
        case 21: _t->select_mouse_mode_slot((*reinterpret_cast< QAction*(*)>(_a[1]))); break;
        case 22: _t->select_tool_slot((*reinterpret_cast< QAction*(*)>(_a[1]))); break;
        case 23: _t->select_plane_slot((*reinterpret_cast< QAction*(*)>(_a[1]))); break;
        case 24: _t->zoom_in_slot(); break;
        case 25: _t->zoom_out_slot(); break;
        case 26: _t->invert_scaling_slot(); break;
        case 27: _t->full_screen_slot(); break;
        case 28: _t->toggle_annotations_slot(); break;
        case 29: _t->snap_to_image_slot(); break;
        case 30: _t->wrap_volumes_slot(); break;
        case 31: _t->sync_slot(); break;
        case 32: _t->hide_image_slot(); break;
        case 33: _t->slice_next_slot(); break;
        case 34: _t->slice_previous_slot(); break;
        case 35: _t->image_next_slot(); break;
        case 36: _t->image_previous_slot(); break;
        case 37: _t->image_next_volume_slot(); break;
        case 38: _t->image_previous_volume_slot(); break;
        case 39: _t->image_goto_volume_slot(); break;
        case 40: _t->image_next_volume_group_slot(); break;
        case 41: _t->image_goto_volume_group_slot(); break;
        case 42: _t->image_previous_volume_group_slot(); break;
        case 43: _t->image_reset_slot(); break;
        case 44: _t->image_interpolate_slot(); break;
        case 45: _t->image_select_slot((*reinterpret_cast< QAction*(*)>(_a[1]))); break;
        case 46: _t->reset_view_slot(); break;
        case 47: _t->background_colour_slot(); break;
        case 48: _t->OpenGL_slot(); break;
        case 49: _t->about_slot(); break;
        case 50: _t->aboutQt_slot(); break;
        case 51: _t->process_commandline_option_slot(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (Window::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&Window::focusChanged)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (Window::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&Window::targetChanged)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (Window::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&Window::sliceChanged)) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (Window::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&Window::planeChanged)) {
                *result = 3;
                return;
            }
        }
        {
            using _t = void (Window::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&Window::orientationChanged)) {
                *result = 4;
                return;
            }
        }
        {
            using _t = void (Window::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&Window::fieldOfViewChanged)) {
                *result = 5;
                return;
            }
        }
        {
            using _t = void (Window::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&Window::modeChanged)) {
                *result = 6;
                return;
            }
        }
        {
            using _t = void (Window::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&Window::imageChanged)) {
                *result = 7;
                return;
            }
        }
        {
            using _t = void (Window::*)(bool );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&Window::imageVisibilityChanged)) {
                *result = 8;
                return;
            }
        }
        {
            using _t = void (Window::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&Window::scalingChanged)) {
                *result = 9;
                return;
            }
        }
        {
            using _t = void (Window::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&Window::volumeChanged)) {
                *result = 10;
                return;
            }
        }
        {
            using _t = void (Window::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&Window::syncChanged)) {
                *result = 11;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject MR::GUI::MRView::Window::staticMetaObject = { {
    QMetaObject::SuperData::link<QMainWindow::staticMetaObject>(),
    qt_meta_stringdata_MR__GUI__MRView__Window.data,
    qt_meta_data_MR__GUI__MRView__Window,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *MR::GUI::MRView::Window::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *MR::GUI::MRView::Window::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_MR__GUI__MRView__Window.stringdata0))
        return static_cast<void*>(this);
    if (!strcmp(_clname, "ColourMapButtonObserver"))
        return static_cast< ColourMapButtonObserver*>(this);
    return QMainWindow::qt_metacast(_clname);
}

int MR::GUI::MRView::Window::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QMainWindow::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 52)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 52;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 52)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 52;
    }
    return _id;
}

// SIGNAL 0
void MR::GUI::MRView::Window::focusChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void MR::GUI::MRView::Window::targetChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void MR::GUI::MRView::Window::sliceChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}

// SIGNAL 3
void MR::GUI::MRView::Window::planeChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 3, nullptr);
}

// SIGNAL 4
void MR::GUI::MRView::Window::orientationChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 4, nullptr);
}

// SIGNAL 5
void MR::GUI::MRView::Window::fieldOfViewChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 5, nullptr);
}

// SIGNAL 6
void MR::GUI::MRView::Window::modeChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 6, nullptr);
}

// SIGNAL 7
void MR::GUI::MRView::Window::imageChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 7, nullptr);
}

// SIGNAL 8
void MR::GUI::MRView::Window::imageVisibilityChanged(bool _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 8, _a);
}

// SIGNAL 9
void MR::GUI::MRView::Window::scalingChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 9, nullptr);
}

// SIGNAL 10
void MR::GUI::MRView::Window::volumeChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 10, nullptr);
}

// SIGNAL 11
void MR::GUI::MRView::Window::syncChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 11, nullptr);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
