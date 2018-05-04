/*
 * Copyright (c) 2008-2018 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/
 *
 * MRtrix3 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/
 */


#include "app.h"
#include "timer.h"
#include "file/config.h"
#include "header.h"
#include "algo/copy.h"
#include "gui/opengl/gl.h"
#include "gui/opengl/lighting.h"
#include "gui/dialog/dialog.h"
#include "gui/dialog/file.h"
#include "gui/dialog/opengl.h"
#include "gui/dialog/progress.h"
#include "gui/dialog/image_properties.h"
#include "gui/mrview/mode/base.h"
#include "gui/mrview/mode/list.h"
#include "gui/mrview/tool/base.h"
#include "gui/mrview/tool/list.h"


namespace MR
{
  namespace GUI
  {
    namespace MRView
    {
      using namespace App;
/*
#define MODE(classname, specifier, name, description) #specifier ", "
#define MODE_OPTION(classname, specifier, name, description) MODE(classname, specifier, name, description)

      const OptionGroup Window::options = OptionGroup ("General options")
        + Option ("mode", "select initial display mode by its short ID. Valid mode IDs are: "
#include "gui/mrview/mode/list.h"
            )
        + Argument ("name");
#undef MODE
#undef MODE_OPTION
*/

      Window* Window::main = nullptr;

      namespace {

        Qt::KeyboardModifiers get_modifier (const char* key, Qt::KeyboardModifiers default_key) {
          std::string value = lowercase (MR::File::Config::get (key));
          if (value.empty())
            return default_key;

          if (value == "shift") return Qt::ShiftModifier;
          if (value == "alt") return Qt::AltModifier;
#ifdef MRTRIX_MACOSX
          if (value == "ctrl") return Qt::MetaModifier;
          if (value == "cmd") return Qt::ControlModifier;
#else
          if (value == "ctrl") return Qt::ControlModifier;
          if (value == "meta" || value == "win") return Qt::MetaModifier;
#endif

          throw Exception ("no such modifier \"" + value + "\" (parsed from config file)");
          return Qt::NoModifier;
        }
      }






      std::string get_modifier (Qt::KeyboardModifiers key) {
        switch (key) {
          case Qt::ShiftModifier: return "Shift";
          case Qt::AltModifier: return "Alt";
#ifdef MRTRIX_MACOSX
          case Qt::ControlModifier: return "Cmd";
          case Qt::MetaModifier: return "Ctrl";
#else
          case Qt::ControlModifier: return "Ctrl";
          case Qt::MetaModifier: return "Win";
#endif
          default: assert (0);
        }
        return "Invalid";
      }


      // GLArea definitions:

      Window::GLArea::GLArea (Window& parent) :
        GL::Area (&parent) {
          setCursor (Cursor::crosshair);
          setMouseTracking (true);
          setAcceptDrops (true);
          setMinimumSize (256, 256);
          setFocusPolicy (Qt::StrongFocus);
          grabGesture (Qt::PinchGesture);
          grabGesture (Qt::PanGesture);
          QFont font_ = font();
          //CONF option: FontSize
          //CONF The size (in points) of the font to be used in OpenGL viewports (mrview and shview).
          //CONF default: 10
          font_.setPointSize (MR::File::Config::get_int ("FontSize", 10));
          setFont (font_);
          QSizePolicy policy (QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
          policy.setHorizontalStretch (255);
          policy.setVerticalStretch (255);
          setSizePolicy (policy);
        }

      QSize Window::GLArea::sizeHint () const {
        //CONF option: MRViewInitWindowSize
        //CONF Initial window size of MRView in pixels.
        //CONF default: 512,512
        std::string init_size_string = lowercase (MR::File::Config::get ("MRViewInitWindowSize"));
        vector<int> init_window_size;
        if (init_size_string.length())
          init_window_size = parse_ints(init_size_string);
        if (init_window_size.size() == 2)
          return QSize (init_window_size[0], init_window_size[1]);
        else
          return QSize (512, 512);
      }
      void Window::GLArea::dragEnterEvent (QDragEnterEvent* event) {
        event->acceptProposedAction();
      }
      void Window::GLArea::dragMoveEvent (QDragMoveEvent* event) {
        event->acceptProposedAction();
      }
      void Window::GLArea::dragLeaveEvent (QDragLeaveEvent* event) {
        event->accept();
      }
      void Window::GLArea::dropEvent (QDropEvent* event) {
        const QMimeData* mimeData = event->mimeData();
        if (mimeData->hasUrls()) {
          vector<std::unique_ptr<MR::Header>> list;
          QList<QUrl> urlList = mimeData->urls();
          for (int i = 0; i < urlList.size() && i < 32; ++i) {
            try {
              list.push_back (make_unique<MR::Header> (MR::Header::open (urlList.at (i).path().toUtf8().constData())));
            }
            catch (Exception& e) {
              e.display();
            }
          }
          if (list.size())
            main->add_images (list);
        }
      }

      void Window::GLArea::initializeGL () {
        main->initGL();
      }
      void Window::GLArea::paintGL () {
        main->paintGL();
      }
      void Window::GLArea::mousePressEvent (QMouseEvent* event) {
        main->mousePressEventGL (event);
      }
      void Window::GLArea::mouseMoveEvent (QMouseEvent* event) {
        main->mouseMoveEventGL (event);
      }
      void Window::GLArea::mouseReleaseEvent (QMouseEvent* event) {
        main->mouseReleaseEventGL (event);
      }
      void Window::GLArea::wheelEvent (QWheelEvent* event) {
        main->wheelEventGL (event);
      }

      bool Window::GLArea::event (QEvent* event) {
        if (event->type() == QEvent::Gesture)
          return main->gestureEventGL (static_cast<QGestureEvent*>(event));
        return QWidget::event(event);
      }










      //CONF option: MRViewFocusModifierKey
      //CONF default: meta (cmd on MacOSX)
      //CONF Modifier key to select focus mode in MRView. Valid
      //CONF choices include shift, alt, ctrl, meta (on MacOSX: shift, alt,
      //CONF ctrl, cmd).

      //CONF option: MRViewMoveModifierKey
      //CONF default: shift
      //CONF Modifier key to select move mode in MRView. Valid
      //CONF choices include shift, alt, ctrl, meta (on MacOSX: shift, alt,
      //CONF ctrl, cmd).

      //CONF option: MRViewRotateModifierKey
      //CONF default: ctrl
      //CONF Modifier key to select rotate mode in MRView. Valid
      //CONF choices include shift, alt, ctrl, meta (on MacOSX: shift, alt,
      //CONF ctrl, cmd).


      // Main Window class:

      Window::Window() :
        glarea (new GLArea (*this)),
        mode (nullptr),
        font (glarea->font()),
#ifdef MRTRIX_MACOSX
        FocusModifier (get_modifier ("MRViewFocusModifierKey", Qt::AltModifier)),
#else
        FocusModifier (get_modifier ("MRViewFocusModifierKey", Qt::MetaModifier)),
#endif
        MoveModifier (get_modifier ("MRViewMoveModifierKey", Qt::ShiftModifier)),
        RotateModifier (get_modifier ("MRViewRotateModifierKey", Qt::ControlModifier)),
        mouse_action (NoAction),
        focal_point { NAN, NAN, NAN },
        camera_target { NAN, NAN, NAN },
        orient (),
        field_of_view (100.0),
        anatomical_plane (2),
        colourbar_position (ColourMap::Position::BottomRight),
        tools_colourbar_position (ColourMap::Position::TopRight),
        snap_to_image_axes_and_voxel (true),
        tool_has_focus (nullptr),
        best_FPS (NAN),
        show_FPS (false),
        current_option (0) {
          main = this;
          GUI::App::set_main_window (this);
          GUI::Dialog::init();

          setDockOptions (AllowTabbedDocks | VerticalTabs);
          setDocumentMode (true);

          //CONF option: IconSize
          //CONF default: 30
          //CONF The size of the icons in the main MRView toolbar.
          setWindowTitle (tr ("MRView"));
          setWindowIcon (QPixmap (":/mrtrix.png"));
          {
            int iconsize = MR::File::Config::get_int ("IconSize", 30);
            setIconSize (QSize (iconsize, iconsize));
          }
          setCentralWidget (glarea);
          QToolBar* toolbar;
          QAction* action;
          QMenu* menu;
          QToolButton* button;

          setTabPosition (Qt::AllDockWidgetAreas, QTabWidget::East);

          // Main toolbar:

          //CONF option: InitialToolBarPosition
          //CONF default: top
          //CONF The starting position of the MRView toolbar. Valid values are:
          //CONF top, bottom, left, right.
          Qt::ToolBarArea toolbar_position = Qt::TopToolBarArea;
          {
            std::string toolbar_pos_spec = lowercase (MR::File::Config::get ("InitialToolBarPosition"));
            if (toolbar_pos_spec.size()) {
              if (toolbar_pos_spec == "bottom") toolbar_position = Qt::BottomToolBarArea;
              else if (toolbar_pos_spec == "left") toolbar_position = Qt::LeftToolBarArea;
              else if (toolbar_pos_spec == "right") toolbar_position = Qt::RightToolBarArea;
              else if (toolbar_pos_spec != "top")
                WARN ("invalid value for configuration entry \"InitialToolBarPosition\"");
            }
          }

          //CONF option: ToolbarStyle
          //CONF default: 2
          //CONF The style of the main toolbar buttons in MRView. See Qt's
          //CONF documentation for Qt::ToolButtonStyle.
          Qt::ToolButtonStyle button_style = static_cast<Qt::ToolButtonStyle> (MR::File::Config::get_int ("ToolbarStyle", 2));

          toolbar = new QToolBar ("Main toolbar", this);
          addToolBar (toolbar_position, toolbar);
          action = toolbar->toggleViewAction ();
          action->setShortcut (tr ("Ctrl+M"));
          addAction (action);


          // File menu:
          menu = new QMenu (tr ("File menu"), this);

          action = menu->addAction (tr ("Open..."), this, SLOT (image_open_slot()));
          action->setShortcut (tr ("Ctrl+O"));
          addAction (action);

          save_action = menu->addAction (tr ("Save..."), this, SLOT (image_save_slot()));
          save_action->setShortcut (tr ("Ctrl+S"));
          addAction (save_action);

          close_action = menu->addAction (tr ("Close"), this, SLOT (image_close_slot()));
          close_action->setShortcut (tr ("Ctrl+W"));
          addAction (close_action);

          menu->addSeparator();

          action = menu->addAction (tr ("DICOM import..."), this, SLOT (image_import_DICOM_slot()));
          action->setShortcut (tr ("Ctrl+D"));
          addAction (action);

          menu->addSeparator();

          action = menu->addAction (tr ("Quit"), this, SLOT (close()));
          action->setShortcut (tr ("Ctrl+Q"));
          addAction (action);


          button = new QToolButton (this);
          button->setText ("File");
          button->setToolButtonStyle (button_style);
          button->setToolTip (tr ("Load and save images"));
          button->setIcon (QIcon (":/file.svg"));
          button->setPopupMode (QToolButton::InstantPopup);
          button->setMenu (menu);
          toolbar->addWidget (button);


          // Image menu:
          image_menu = new QMenu (tr ("Image menu"), this);

          image_group = new QActionGroup (this);
          image_group->setExclusive (true);
          connect (image_group, SIGNAL (triggered (QAction*)), this, SLOT (image_select_slot (QAction*)));

          properties_action = image_menu->addAction (tr ("Properties..."), this, SLOT (image_properties_slot()));
          properties_action->setToolTip (tr ("Display the properties of the current image\n\nShortcut: Ctrl+P"));
          addAction (properties_action);

          image_menu->addSeparator();

          next_slice_action = image_menu->addAction (tr ("Next slice"), this, SLOT (slice_next_slot()));
          next_slice_action->setShortcut (tr ("Up"));
          addAction (next_slice_action);

          prev_slice_action = image_menu->addAction (tr ("Previous slice"), this, SLOT (slice_previous_slot()));
          prev_slice_action->setShortcut (tr ("Down"));
          addAction (prev_slice_action);

          next_image_volume_action = image_menu->addAction (tr ("Next volume"), this, SLOT (image_next_volume_slot()));
          next_image_volume_action->setShortcut (tr ("Right"));
          addAction (next_image_volume_action);

          prev_image_volume_action = image_menu->addAction (tr ("Previous volume"), this, SLOT (image_previous_volume_slot()));
          prev_image_volume_action->setShortcut (tr ("Left"));
          addAction (prev_image_volume_action);

          goto_image_volume_action = image_menu->addAction (tr ("Go to volume..."), this, SLOT (image_goto_volume_slot()));
          goto_image_volume_action->setShortcut (tr ("g"));
          addAction (goto_image_volume_action);

          next_image_volume_group_action = image_menu->addAction (tr ("Next volume group"), this, SLOT (image_next_volume_group_slot()));
          next_image_volume_group_action->setShortcut (tr ("Shift+Right"));
          addAction (next_image_volume_group_action);

          prev_image_volume_group_action = image_menu->addAction (tr("Previous volume group"), this, SLOT (image_previous_volume_group_slot()));
          prev_image_volume_group_action->setShortcut (tr ("Shift+Left"));
          addAction (prev_image_volume_group_action);

          goto_image_volume_group_action = image_menu->addAction (tr ("Go to volume group..."), this, SLOT (image_goto_volume_group_slot()));
          goto_image_volume_group_action->setShortcut (tr ("Shift+g"));
          addAction (goto_image_volume_group_action);

          image_menu->addSeparator();

          next_image_action = image_menu->addAction (tr ("Next image"), this, SLOT (image_next_slot()));
          next_image_action->setShortcut (tr ("PgDown"));
          addAction (next_image_action);

          prev_image_action = image_menu->addAction (tr ("Previous image"), this, SLOT (image_previous_slot()));
          prev_image_action->setShortcut (tr ("PgUp"));
          addAction (prev_image_action);

          image_list_area = image_menu->addSeparator();

          button = new QToolButton (this);
          button->setText ("Image");
          button->setToolButtonStyle (button_style);
          button->setToolTip (tr ("Navigate the image"));
          button->setIcon (QIcon (":/image.svg"));
          button->setPopupMode (QToolButton::InstantPopup);
          button->setMenu (image_menu);
          toolbar->addWidget (button);


          // Colourmap menu:
          colourmap_button = new ColourMapButton (this, *this, true, true, false);
          colourmap_button->setText ("Colourmap");
          colourmap_button->setToolButtonStyle (button_style);
          colourmap_button->setToolTip (tr ("Change the colourmap"));
          colourmap_button->setPopupMode (QToolButton::InstantPopup);

          QMenu* colourmap_menu = colourmap_button->menu();

          invert_scale_action = colourmap_menu->addAction (tr ("Invert"), this, SLOT (invert_scaling_slot()));
          invert_scale_action->setCheckable (true);
          invert_scale_action->setShortcut (tr("U"));
          addAction (invert_scale_action);

          colourmap_menu->addSeparator();

          reset_windowing_action = colourmap_menu->addAction (tr ("Reset brightness/contrast"), this, SLOT (image_reset_slot()));
          reset_windowing_action->setShortcut (tr ("Esc"));
          addAction (reset_windowing_action);

          //CONF option: ImageInterpolation
          //CONF default: true
          //CONF Interpolation switched on in the main image.
          image_interpolate_action = colourmap_menu->addAction (tr ("Interpolate"), this, SLOT (image_interpolate_slot()));
          image_interpolate_action->setShortcut (tr ("I"));
          image_interpolate_action->setCheckable (true);
          image_interpolate_action->setChecked (File::Config::get_bool("ImageInterpolation", true));
          addAction (image_interpolate_action);

          toolbar->addWidget (colourmap_button);


          // Mode menu:
          mode_group = new QActionGroup (this);
          mode_group->setExclusive (true);
          connect (mode_group, SIGNAL (triggered (QAction*)), this, SLOT (select_mode_slot (QAction*)));

          menu = new QMenu ("Display mode", this);
#define MODE(classname, specifier, name, description) \
          menu->addAction (new Mode::Action<Mode::classname> (mode_group, #name, #description, n++));
#define MODE_OPTION(classname, specifier, name, description) MODE(classname, specifier, name, description)
          {
            size_t n = 1;
#include "gui/mrview/mode/list.h"
          }
#undef MODE
#undef MODE_OPTION
          mode_group->actions()[0]->setChecked (true);
          for (int n = 0; n < mode_group->actions().size(); ++n)
            addAction (mode_group->actions()[n]);


          menu->addSeparator();

          plane_group = new QActionGroup (this);
          plane_group->setExclusive (true);
          connect (plane_group, SIGNAL (triggered (QAction*)), this, SLOT (select_plane_slot (QAction*)));

          axial_action = menu->addAction (tr ("Axial"));
          axial_action->setShortcut (tr ("A"));
          axial_action->setCheckable (true);
          plane_group->addAction (axial_action);
          addAction (axial_action);

          sagittal_action = menu->addAction (tr ("Sagittal"));
          sagittal_action->setShortcut (tr ("S"));
          sagittal_action->setCheckable (true);
          plane_group->addAction (sagittal_action);
          addAction (sagittal_action);

          coronal_action = menu->addAction (tr ("Coronal"));
          coronal_action->setShortcut (tr ("C"));
          coronal_action->setCheckable (true);
          plane_group->addAction (coronal_action);
          addAction (coronal_action);

          menu->addSeparator();

          action = menu->addAction (tr ("Zoom in"), this, SLOT (zoom_in_slot()));
          action->setShortcut (tr("Ctrl++"));
          addAction (action);

          action = menu->addAction (tr ("Zoom out"), this, SLOT (zoom_out_slot()));
          action->setShortcut (tr("Ctrl+-"));
          addAction (action);

          menu->addSeparator();

          action = menu->addAction (tr ("Toggle all annotations"), this, SLOT (toggle_annotations_slot()));
          action->setShortcut (tr("Space"));
          addAction (action);

          //CONF option: MRViewShowFocus
          //CONF default: true
          //CONF Focus cross hair shown in main image.
          show_crosshairs_action = menu->addAction (tr ("Show focus"), glarea, SLOT (update()));
          show_crosshairs_action->setShortcut (tr("F"));
          show_crosshairs_action->setCheckable (true);
          show_crosshairs_action->setChecked (File::Config::get_bool("MRViewShowFocus",true));
          addAction (show_crosshairs_action);

          //CONF option: MRViewShowComments
          //CONF default: true
          //CONF Comments shown in main image overlay.
          show_comments_action = menu->addAction (tr ("Show comments"), glarea, SLOT (update()));
          show_comments_action->setToolTip (tr ("Show/hide image comments\n\nShortcut: H"));
          show_comments_action->setShortcut (tr("H"));
          show_comments_action->setCheckable (true);
          show_comments_action->setChecked (File::Config::get_bool("MRViewShowComments",true));
          addAction (show_comments_action);

          //CONF option: MRViewShowVoxelInformation
          //CONF default: true
          //CONF Voxel information shown in main image overlay.
          show_voxel_info_action = menu->addAction (tr ("Show voxel information"), glarea, SLOT (update()));
          show_voxel_info_action->setShortcut (tr("V"));
          show_voxel_info_action->setCheckable (true);
          show_voxel_info_action->setChecked (File::Config::get_bool("MRViewShowVoxelInformation",true));
          addAction (show_voxel_info_action);

          //CONF option: MRViewShowOrientationLabel
          //CONF default: true
          //CONF Anatomical orientation information shown in main image overlay.
          show_orientation_labels_action = menu->addAction (tr ("Show orientation labels"), glarea, SLOT (update()));
          show_orientation_labels_action->setShortcut (tr("O"));
          show_orientation_labels_action->setCheckable (true);
          show_orientation_labels_action->setChecked (File::Config::get_bool("MRViewShowOrientationLabel",true));
          addAction (show_orientation_labels_action);

          //CONF option: MRViewShowColourbar
          //CONF default: true
          //CONF Colourbar shown in main image overlay.
          show_colourbar_action = menu->addAction (tr ("Show colour bar"), glarea, SLOT (update()));
          show_colourbar_action->setShortcut (tr("B"));
          show_colourbar_action->setCheckable (true);
          show_colourbar_action->setChecked (File::Config::get_bool("MRViewShowColourbar",true));
          addAction (show_colourbar_action);

          menu->addSeparator();

          action = menu->addAction (tr ("Background colour..."), this, SLOT (background_colour_slot()));
          action->setCheckable (false);
          addAction (action);

          image_hide_action = menu->addAction (tr ("Hide main image"), this, SLOT (hide_image_slot()));
          image_hide_action->setShortcut (tr ("M"));
          image_hide_action->setCheckable (true);
          image_hide_action->setChecked (false);
          addAction (image_hide_action);

          menu->addSeparator();

          full_screen_action = menu->addAction (tr ("Full screen"), this, SLOT (full_screen_slot()));
          full_screen_action->setShortcut (tr ("F11"));
          full_screen_action->setCheckable (true);
          full_screen_action->setChecked (false);
          addAction (full_screen_action);

          action = menu->addAction (tr ("Reset view"), this, SLOT(reset_view_slot()));
          action->setShortcut (tr ("R"));
          addAction (action);

          button = new QToolButton (this);
          button->setText ("View");
          button->setToolButtonStyle (button_style);
          button->setToolTip (tr ("View modes and options"));
          button->setIcon (QIcon (":/view.svg"));
          button->setMenu (menu);
          button->setPopupMode (QToolButton::InstantPopup);
          toolbar->addWidget (button);

          toolbar->addSeparator();


          // Mouse mode actions:
          mode_action_group = new QActionGroup (this);
          mode_action_group->setExclusive (true);
          connect (mode_action_group, SIGNAL (triggered (QAction*)), this, SLOT (select_mouse_mode_slot (QAction*)));

          std::string modifier;
          action = toolbar->addAction (QIcon (":/select_contrast.svg"), tr ("Change focus / contrast"));
          action->setToolTip (tr ((
                  "Left-click: set focus\n"
                  "Right-click: change brightness/constrast\n\n"
                  "Shortcut: 1\n\n"
                  "Hold down " + get_modifier (FocusModifier) + " key to use this mode\n"
                  "regardless of currently selected mode").c_str()));
          action->setShortcut (tr("1"));
          action->setCheckable (true);
          action->setChecked (true);
          mode_action_group->addAction (action);

          action = toolbar->addAction (QIcon (":/move.svg"), tr ("Move viewport"));
          action->setToolTip (tr ((
                  "Left-click: move in-plane\n"
                  "Right-click: move through-plane\n\n"
                  "Shortcut: 2\n\n"
                  "Hold down " + get_modifier (MoveModifier) + " key to use this mode\n"
                  "regardless of currently selected mode").c_str()));
          action->setShortcut (tr("2"));
          action->setCheckable (true);
          mode_action_group->addAction (action);

          action = toolbar->addAction (QIcon (":/rotate.svg"), tr ("Move camera"));
          action->setToolTip (tr ((
                  "Left-click: move camera in-plane\n"
                  "Right-click: rotate camera about view axis\n\n"
                  "Shortcut: 3\n\n"
                  "Hold down " + get_modifier (RotateModifier) + " key to use this mode\n"
                  "regardless of currently selected mode").c_str()));
          action->setShortcut (tr("3"));
          action->setCheckable (true);
          mode_action_group->addAction (action);

          for (int n = 0; n < mode_action_group->actions().size(); ++n)
            addAction (mode_action_group->actions()[n]);

          toolbar->addSeparator();

          snap_to_image_action = toolbar->addAction (QIcon (":/lock.svg"),
              tr ("Snap to image"), this, SLOT (snap_to_image_slot()));
          snap_to_image_action->setToolTip (tr (
                "Snap focus and view orientation to\n"
                "image voxel grid and axes respectively\n\n"
                "Shortcut: L"));
          snap_to_image_action->setShortcut (tr("L"));
          snap_to_image_action->setCheckable (true);
          snap_to_image_action->setChecked (snap_to_image_axes_and_voxel);
          addAction (snap_to_image_action);

          toolbar->addSeparator();


          // Dynamic spacer:
          QWidget* spacer = new QWidget();
          spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
          toolbar->addWidget (spacer);


          // Tool menu:
          tool_group = new QActionGroup (this);
          tool_group->setExclusive (false);
          connect (tool_group, SIGNAL (triggered (QAction*)), this, SLOT (select_tool_slot (QAction*)));

          menu = new QMenu (tr ("Tools"), this);
#undef TOOL
#define TOOL(classname, name, description) \
          menu->addAction (new Action<Tool::classname> (tool_group, #name, #description, n++));
          {
            using namespace Tool;
            size_t n = 1;
#include "gui/mrview/tool/list.h"
          }
          for (int n = 0; n < tool_group->actions().size(); ++n)
            addAction (tool_group->actions()[n]);

          button = new QToolButton (this);
          button->setText ("Tool");
          button->setToolButtonStyle (button_style);
          button->setToolTip (tr ("Open different tools"));
          button->setIcon (QIcon (":/tools.svg"));
          button->setMenu (menu);
          button->setPopupMode (QToolButton::InstantPopup);
          toolbar->addWidget (button);

          toolbar->addSeparator();


          // Information menu:

          menu = new QMenu (tr ("Info"), this);

          menu->addAction (tr ("About MRView"), this, SLOT (about_slot()));
          menu->addAction (tr ("About Qt"), this, SLOT (aboutQt_slot()));
          menu->addAction (tr ("OpenGL information"), this, SLOT (OpenGL_slot()));


          button = new QToolButton (this);
          button->setToolButtonStyle (button_style);
          button->setToolTip (tr ("Information"));
          button->setIcon (QIcon (":/info.svg"));
          button->setPopupMode (QToolButton::InstantPopup);
          button->setMenu (menu);
          toolbar->addWidget (button);



          lighting_ = new GL::Lighting (this);
          connect (lighting_, SIGNAL (changed()), glarea, SLOT (update()));

          set_image_menu ();

          //CONF option: MRViewColourBarPosition
          //CONF default: bottomright
          //CONF The position of the colourbar within the main window in MRView.
          //CONF Valid values are: bottomleft, bottomright, topleft, topright.
          std::string cbar_pos = lowercase (MR::File::Config::get ("MRViewColourBarPosition", "bottomright"));
          colourbar_position = parse_colourmap_position_str(cbar_pos);
          if(!colourbar_position)
            WARN ("invalid specifier \"" + cbar_pos + "\" for config file entry \"MRViewColourBarPosition\"");

          //CONF option: MRViewToolsColourBarPosition
          //CONF default: topright
          //CONF The position of all visible tool colourbars within the main window in MRView.
          //CONF Valid values are: bottomleft, bottomright, topleft, topright.
          cbar_pos = lowercase (MR::File::Config::get ("MRViewToolsColourBarPosition", "topright"));
          tools_colourbar_position = parse_colourmap_position_str(cbar_pos);
          if(!tools_colourbar_position)
            WARN ("invalid specifier \"" + cbar_pos + "\" for config file entry \"MRViewToolsColourBarPosition\"");


        }





      void Window::parse_arguments ()
      {
        if (MR::App::argument.size()) {
          if (MR::App::option.size())  {
            // check that first non-standard option appears after last argument:
            size_t last_arg_pos = 1;
            for (; MR::App::argv[last_arg_pos] != MR::App::argument.back().c_str(); ++last_arg_pos)
              if (MR::App::argv[last_arg_pos] == nullptr)
                throw Exception ("FIXME: error determining position of last argument!");

            // identify first non-standard option:
            size_t first_option = 0;
            for (; first_option < MR::App::option.size(); ++first_option) {
              if (size_t (MR::App::option[first_option].opt - &MR::App::__standard_options[0]) >= MR::App::__standard_options.size())
                break;
            }
            if (MR::App::option.size() > first_option) {
              first_option = MR::App::option[first_option].args - MR::App::argv;
              if (first_option < last_arg_pos)
                throw Exception ("options must appear after the last argument - see help page for details");
            }
          }

          vector<std::unique_ptr<MR::Header>> list;
          for (size_t n = 0; n < MR::App::argument.size(); ++n) {
            try { list.push_back (make_unique<MR::Header> (MR::Header::open (MR::App::argument[n]))); }
            catch (Exception& e) { e.display(); }
          }
          add_images (list);
        }

        QTimer::singleShot (10, this, SLOT(process_commandline_option_slot()));
      }






      ColourMap::Position Window::parse_colourmap_position_str (const std::string& position_str) {

        ColourMap::Position pos(ColourMap::Position::None);

        if(position_str == "bottomleft")
          pos = ColourMap::Position::BottomLeft;
        else if(position_str == "bottomright")
          pos = ColourMap::Position::BottomRight;
        else if(position_str == "topleft")
          pos = ColourMap::Position::TopLeft;
        else if(position_str == "topright")
          pos = ColourMap::Position::TopRight;

        return pos;
      }



      Window::~Window ()
      {
        glarea->makeCurrent();
        QList<QAction*> tools = tool_group->actions();
        for (QAction* action : tools)
          delete action;
        mode.reset();
        QList<QAction*> images = image_group->actions();
        for (QAction* action : images)
          delete action;
      }







      void Window::image_open_slot ()
      {
        vector<std::string> image_list = Dialog::File::get_images (this, "Select images to open");
        if (image_list.empty())
          return;

        vector<std::unique_ptr<MR::Header>> list;
        for (size_t n = 0; n < image_list.size(); ++n) {
          try {
            list.push_back (make_unique<MR::Header> (MR::Header::open (image_list[n])));
          }
          catch (Exception& E) {
            E.display();
          }
        }
        add_images (list);
      }



      void Window::image_import_DICOM_slot ()
      {
        std::string folder = Dialog::File::get_folder (this, "Select DICOM folder to import");
        if (folder.empty())
          return;


        try {
          vector<std::unique_ptr<MR::Header>> list;
          list.push_back (make_unique<MR::Header> (MR::Header::open (folder)));
          add_images (list);
        }
        catch (Exception& E) {
          E.display();
        }
      }




      void Window::add_images (vector<std::unique_ptr<MR::Header>>& list)
      {
        for (size_t i = 0; i < list.size(); ++i) {
          const std::string name = list[i]->name(); // Gets move-constructed out
          QAction* action = new Image (std::move (*list[i]));
          action->setText (shorten (name, 20, 0).c_str());
          action->setParent (Window::main);
          action->setCheckable (true);
          action->setToolTip (name.c_str());
          action->setStatusTip (name.c_str());
          image_group->addAction (action);
          image_menu->addAction (action);
          connect (action, SIGNAL(scalingChanged()), this, SLOT(on_scaling_changed()));

          if (!i) image_select_slot (action);
        }
        set_image_menu();
      }



      void Window::image_save_slot ()
      {
        std::string image_name = Dialog::File::get_save_image_name (this, "Select image destination");
        if (image_name.empty())
          return;

        try {
          auto dest = MR::Image<cfloat>::create (image_name, image()->header());
          MR::copy_with_progress (image()->image, dest);
        }
        catch (Exception& E) {
          E.display();
        }
      }




      void Window::image_close_slot ()
      {
        Image* imagep = image();
        assert (imagep);
        QList<QAction*> list = image_group->actions();
        if (list.size() > 1) {
          for (int n = 0; n < list.size(); ++n) {
            if (imagep == list[n]) {
              image_select_slot (list[ (n+1) %list.size()]);
              break;
            }
          }
        }
        image_group->removeAction (imagep);
        delete imagep;
        set_image_menu();
      }




      void Window::image_properties_slot ()
      {
        assert (image());
        Dialog::ImageProperties props (this, image()->header());
        props.exec();
      }



      void Window::select_mode_slot (QAction* action)
      {
        glarea->makeCurrent();
        mode.reset (dynamic_cast<GUI::MRView::Mode::__Action__*> (action)->create());
        mode->set_visible(! image_hide_action->isChecked());
        set_mode_features();
        emit modeChanged();
        glarea->update();
      }


      void Window::select_mouse_mode_slot (QAction* action)
      {
        bool rotate_button_checked = mode_action_group->actions().indexOf (action) == 2;
        if (rotate_button_checked)
          set_snap_to_image (false);
        snap_to_image_action->setEnabled (!rotate_button_checked);
        set_cursor();
      }



      void Window::select_tool_slot (QAction* action)
      {
        Tool::Dock* tool = dynamic_cast<Tool::__Action__*>(action)->dock;
        if (!tool) {
          create_tool (action, true);
          return;
        }

        if (action->isChecked()) {
          if (!tool->isVisible())
            tool->show();
          tool->raise();
        } else {
          tool->close();
        }
      }





      void Window::create_tool (QAction* action, bool show)
      {
        if (dynamic_cast<Tool::__Action__*>(action)->dock)
          return;

        Tool::Dock* tool = dynamic_cast<Tool::__Action__*>(action)->create();
        connect (tool, SIGNAL (visibilityChanged (bool)), action, SLOT (setChecked (bool)));

        //CONF option: MRViewDockFloating
        //CONF default: 0 (false)
        //CONF Whether MRView tools should start docked in the main window, or
        //CONF floating (detached from the main window).
        bool floating = MR::File::Config::get_int ("MRViewDockFloating", 0);

        if (!floating) {

          for (int i = 0; i < tool_group->actions().size(); ++i) {
            Tool::Dock* other_tool = dynamic_cast<Tool::__Action__*>(tool_group->actions()[i])->dock;
            if (other_tool && other_tool != tool) {
              QList<QDockWidget*> list = QMainWindow::tabifiedDockWidgets (other_tool);
              if (list.size())
                QMainWindow::tabifyDockWidget (list.last(), tool);
              else
                QMainWindow::tabifyDockWidget (other_tool, tool);
              break;
            }
          }

        }

        tool->setFloating (floating);
        if (show) {
          tool->show();
          tool->raise();
        }
        else {
          tool->close();
        }
      }


      void Window::selected_colourmap (size_t colourmap, const ColourMapButton&)
      {
        Image* imagep = image();
        if (imagep) {
          imagep->set_colourmap (colourmap);
          glarea->update();
        }
      }



      void Window::selected_custom_colour (const QColor& colour, const ColourMapButton&)
      {
        Image* imagep = image();
        if (imagep) {
          std::array<GLubyte, 3> c_colour{{GLubyte(colour.red()), GLubyte(colour.green()), GLubyte(colour.blue())}};
          imagep->set_colour(c_colour);
          glarea->update();
        }
      }



      void Window::invert_scaling_slot ()
      {
        if (image()) {
          image()->set_invert_scale (invert_scale_action->isChecked());
          glarea->update();
        }
      }



      void Window::snap_to_image_slot ()
      {
        if (image()) {
          snap_to_image_axes_and_voxel = snap_to_image_action->isChecked();
          if (snap_to_image_axes_and_voxel)
            mode->reset_orientation();
          glarea->update();
        }
      }




      void Window::on_scaling_changed ()
      {
        emit scalingChanged();
      }




      void Window::updateGL ()
      {
        glarea->update();
      }

      void Window::drawGL ()
      {
        glarea->repaint();
      }



      void Window::image_reset_slot ()
      {
        Image* imagep = image();
        if (imagep) {
          imagep->reset_windowing (anatomical_plane, snap_to_image_action->isChecked());
          on_scaling_changed();
          glarea->update();
        }
      }



      void Window::image_interpolate_slot ()
      {
        Image* imagep = image();
        if (imagep) {
          imagep->set_interpolate (image_interpolate_action->isChecked());
          glarea->update();
        }
      }


      void Window::full_screen_slot ()
      {
        if (full_screen_action->isChecked())
          showFullScreen();
        else
          showNormal();
      }

      void Window::select_plane_slot (QAction* action)
      {
        if (action == axial_action) set_plane (2);
        else if (action == sagittal_action) set_plane (0);
        else if (action == coronal_action) set_plane (1);
        else assert (0);
        glarea->update();
      }





      void Window::zoom_in_slot ()
      {
        set_FOV (FOV() * std::exp (-0.1));
        glarea->update();
      }

      void Window::zoom_out_slot ()
      {
        set_FOV (FOV() * std::exp (0.1));
        glarea->update();
      }




      void Window::reset_view_slot ()
      {
        if (image()) {
          mode->reset_event();
          QList<QAction*> tools = tool_group->actions();
          for (QAction* action : tools) {
            Tool::Dock* dock = dynamic_cast<Tool::__Action__*>(action)->dock;
            if (dock)
              dock->tool->reset_event();
          }
        }
      }



      void Window::background_colour_slot ()
      {
        QColor colour = QColorDialog::getColor(Qt::black, this, "Select background colour", QColorDialog::DontUseNativeDialog);

        if (colour.isValid()) {
          background_colour[0] = GLubyte(colour.red()) / 255.0f;
          background_colour[1] = GLubyte(colour.green()) / 255.0f;
          background_colour[2] = GLubyte(colour.blue()) / 255.0f;
          glarea->update();
        }

      }


      void Window::set_image_visibility (bool flag) {
        image_hide_action->setChecked(! flag);
        mode->set_visible(flag);
      }



      void Window::hide_image_slot ()
      {
        bool visible = ! image_hide_action->isChecked();
        mode->set_visible(visible);
        emit imageVisibilityChanged(visible);
      }



      void Window::slice_next_slot ()
      {
        assert (mode);
        if (image())
          mode->slice_move_event (1);
      }

      void Window::slice_previous_slot ()
      {
        assert (mode);
        if (image())
          mode->slice_move_event (-1);
      }


      void Window::image_next_slot ()
      {
        QAction* action = image_group->checkedAction();
        int N = image_group->actions().size();
        int n = image_group->actions().indexOf (action);
        image_select_slot (image_group->actions()[(n+1)%N]);
      }




      void Window::image_previous_slot ()
      {
        QAction* action = image_group->checkedAction();
        int N = image_group->actions().size();
        int n = image_group->actions().indexOf (action);
        image_select_slot (image_group->actions()[(n+N-1)%N]);
      }




      void Window::image_next_volume_slot ()
      {
        size_t vol = image()->image.index(3)+1;
        set_image_volume (3, vol);
        emit volumeChanged(vol);
      }




      void Window::image_previous_volume_slot ()
      {
        size_t vol = image()->image.index(3)-1;
        set_image_volume (3, vol);
        emit volumeChanged(vol);
      }


      void Window::image_goto_volume_slot ()
      {
        size_t maxvol = image()->image.size(3) - 1;
        auto label = std::string ("volume (0...") + str(maxvol) + std::string (")");
        bool ok;
        size_t vol = QInputDialog::getInt (this, tr("Go to..."),
          label.c_str(), image()->image.index(3), 0, maxvol, 1, &ok);
        if (ok) {
          set_image_volume (3, vol);
          emit volumeChanged(vol);
        }
      }

      void Window::image_goto_volume_group_slot ()
      {
        size_t maxvolgroup = image()->image.size(4) - 1;
        auto label = std::string ("volume group (0...") + str(maxvolgroup) + std::string (")");
        bool ok;
        size_t grp = QInputDialog::getInt (this, tr("Go to..."),
          label.c_str(), image()->image.index(4), 0, maxvolgroup, 1, &ok);
        if (ok) {
          set_image_volume (4, grp);
          emit volumeGroupChanged(grp);
        }
      }


      void Window::image_next_volume_group_slot ()
      {
        size_t vol = image()->image.index(4)+1;
        set_image_volume (4, vol);
        emit volumeGroupChanged(vol);
      }




      void Window::image_previous_volume_group_slot ()
      {
        size_t vol = image()->image.index(4)-1;
        set_image_volume (4, vol);
        emit volumeGroupChanged(vol);
      }




      void Window::image_select_slot (QAction* action)
      {
        action->setChecked (true);
        image_interpolate_action->setChecked (image()->interpolate());
        size_t cmap_index = image()->colourmap;
        colourmap_button->colourmap_actions[cmap_index]->setChecked (true);
        invert_scale_action->setChecked (image()->scale_inverted());
        mode->image_changed_event();
        setWindowTitle (image()->image.name().c_str());
        set_image_navigation_menu();
        image()->set_allowed_features (
            mode->features & Mode::ShaderThreshold,
            mode->features & Mode::ShaderTransparency,
            mode->features & Mode::ShaderLighting);
        emit imageChanged();
        glarea->update();
      }


      void Window::toggle_annotations_slot ()
      {
        int current_annotations = 0x00000000;
        if (show_crosshairs()) current_annotations |= 0x00000001;
        if (show_comments()) current_annotations |= 0x00000002;
        if (show_voxel_info()) current_annotations |= 0x00000004;
        if (show_orientation_labels()) current_annotations |= 0x00000008;
        if (show_colourbar()) current_annotations |= 0x00000010;

        if (current_annotations) {
          annotations = current_annotations;
          show_crosshairs_action->setChecked (false);
          show_comments_action->setChecked (false);
          show_voxel_info_action->setChecked (false);
          show_orientation_labels_action->setChecked (false);
          show_colourbar_action->setChecked (false);
        }
        else {
          if (!annotations)
            annotations = 0xFFFFFFFF;
          show_crosshairs_action->setChecked (annotations & 0x00000001);
          show_comments_action->setChecked (annotations & 0x00000002);
          show_voxel_info_action->setChecked (annotations & 0x00000004);
          show_orientation_labels_action->setChecked (annotations & 0x00000008);
          show_colourbar_action->setChecked (annotations & 0x00000010);
        }
        glarea->update();
      }


      void Window::set_image_menu ()
      {
        int N = image_group->actions().size();
        next_image_action->setEnabled (N>1);
        prev_image_action->setEnabled (N>1);
        reset_windowing_action->setEnabled (N>0);
        colourmap_button->setEnabled (N>0);
        save_action->setEnabled (N>0);
        close_action->setEnabled (N>0);
        properties_action->setEnabled (N>0);
        set_image_navigation_menu();
        glarea->update();
      }

      int Window::get_mouse_mode ()
      {
        if (mouse_action == NoAction && modifiers_ != Qt::NoModifier) {
          if (modifiers_ == FocusModifier && ( mode->features & Mode::FocusContrast ))
            return 1;
          else if (modifiers_ == MoveModifier && ( mode->features & Mode::MoveTarget ))
            return 2;
          else if (modifiers_ == RotateModifier && ( mode->features & Mode::TiltRotate ))
            return 3;
        }

        if (mouse_action == NoAction)
          return mode_action_group->actions().indexOf (mode_action_group->checkedAction()) + 1;

        return 0;
      }


      void Window::set_cursor ()
      {
        MouseAction cursor = mouse_action;

        if (cursor == NoAction) {
          switch (get_mouse_mode()) {
            case 1: cursor = SetFocus; break;
            case 2: cursor = Pan; break;
            case 3: cursor = Tilt; break;
            default: assert (0);
          }
        }

        if (tool_has_focus && modifiers_ == Qt::NoModifier) {
          QCursor* ptr = tool_has_focus->get_cursor();
          if (ptr) {
            glarea->setCursor (*ptr);
            return;
          }
        }

        switch (cursor) {
          case SetFocus: glarea->setCursor (Cursor::crosshair); break;
          case Contrast: glarea->setCursor (Cursor::window); break;
          case Pan: glarea->setCursor (Cursor::pan_crosshair); break;
          case PanThrough: glarea->setCursor (Cursor::forward_backward); break;
          case Tilt: glarea->setCursor (Cursor::throughplane_rotate); break;
          case Rotate: glarea->setCursor (Cursor::inplane_rotate); break;
          default: assert (0);
        }
      }



      void Window::set_mode_features ()
      {
        mode_action_group->actions()[0]->setEnabled (mode->features & Mode::FocusContrast);
        mode_action_group->actions()[1]->setEnabled (mode->features & Mode::MoveTarget);
        mode_action_group->actions()[2]->setEnabled (mode->features & Mode::TiltRotate);
        if (!mode_action_group->checkedAction()->isEnabled())
          mode_action_group->actions()[0]->setChecked (true);
        if (image())
          image()->set_allowed_features (
              mode->features & Mode::ShaderThreshold,
              mode->features & Mode::ShaderTransparency,
              mode->features & Mode::ShaderLighting);
      }


      void Window::set_image_navigation_menu ()
      {
        bool show_next_volume (false), show_goto_volume (false), show_prev_volume (false);
        bool show_next_volume_group (false), show_goto_volume_group (false), show_prev_volume_group (false);
        Image* imagep = image();
        if (imagep) {
          if (imagep->image.ndim() > 3) {
            if (imagep->image.size(3) > 1)
              show_goto_volume = true;
            if (imagep->image.index(3) > 0)
              show_prev_volume = true;
            if (imagep->image.index(3) < imagep->image.size(3)-1)
              show_next_volume = true;

            if (imagep->image.ndim() > 4) {
              if (imagep->image.size(4) > 1)
                show_goto_volume_group = true;
              if (imagep->image.index(4) > 0)
                show_prev_volume_group = true;
              if (imagep->image.index(4) < imagep->image.size(4)-1)
                show_next_volume_group = true;
            }
          }
        }
        prev_image_volume_action->setEnabled (show_prev_volume);
        next_image_volume_action->setEnabled (show_next_volume);
        goto_image_volume_action->setEnabled (show_goto_volume);
        prev_image_volume_group_action->setEnabled (show_prev_volume_group);
        next_image_volume_group_action->setEnabled (show_next_volume_group);
        goto_image_volume_group_action->setEnabled (show_goto_volume_group);
      }





      void Window::OpenGL_slot ()
      {
        Dialog::OpenGL glinfo (this, glarea->format());
        glinfo.exec();
      }




      void Window::about_slot ()
      {
        std::string message =
          std::string ("<h1>MRView</h1>The MRtrix viewer, version ") + MR::App::mrtrix_version + "<br>"
          "<em>" + str (8*sizeof (size_t)) + " bit "
#ifdef NDEBUG
          "release"
#else
          "debug"
#endif
          " version, built " + MR::App::build_date +  "</em><p>"
          "<h4>Authors:</h4>" + MR::join (MR::split (MR::App::AUTHOR, ",;&\n", true), "<br>") +
          "<p><em>" + MR::App::COPYRIGHT + "</em>";

        QMessageBox::about (this, tr ("About MRView"), message.c_str());
      }




      void Window::aboutQt_slot ()
      {
        QMessageBox::aboutQt (this);
      }


      void Window::paintGL ()
      {
        ASSERT_GL_MRVIEW_CONTEXT_IS_CURRENT;
        GL_CHECK_ERROR;
        gl::ClearColor (background_colour[0], background_colour[1], background_colour[2], 1.0);

        if (glarea->format().samples() > 1)
          gl::Enable (gl::MULTISAMPLE);

        GL_CHECK_ERROR;

        ASSERT_GL_MRVIEW_CONTEXT_IS_CURRENT;
        mode->paintGL();
        ASSERT_GL_MRVIEW_CONTEXT_IS_CURRENT;
        GL_CHECK_ERROR;

        if (show_FPS) {
          render_times.push_back (Timer::current_time());
          while (render_times.size() > 10)
            render_times.erase (render_times.begin());
          double FPS = NAN;
          std::string FPS_string = "-";
          std::string FPS_best_string = "-";

          if (render_times.back() - best_FPS_time > 3.0)
            best_FPS = NAN;

          if (render_times.size() == 10) {
            FPS = (render_times.size()-1.0) / (render_times.back()-render_times.front());
            FPS_string = str (FPS, 4);
            if (!std::isfinite (best_FPS) || FPS > best_FPS) {
              best_FPS = FPS;
              best_FPS_time = render_times.back();
            }
          }
          else
            best_FPS = NAN;

          if (std::isfinite (best_FPS))
            FPS_best_string = str (best_FPS, 4);
          mode->projection.setup_render_text (0.0, 1.0, 0.0);
          mode->projection.render_text ("max FPS: " + FPS_best_string, RightEdge | TopEdge);
          mode->projection.render_text ("FPS: " + FPS_string, RightEdge | TopEdge, 1);
          mode->projection.done_render_text();
        }

        // need to clear alpha channel when using QOpenGLWidget (Qt >= 5.4)
        // otherwise we get transparent windows...
#if QT_VERSION >= 0x050400
        gl::ColorMask (false, false, false, true);
        gl::Clear (gl::COLOR_BUFFER_BIT);
        glColorMask (true, true, true, true);
#endif
        GL_CHECK_ERROR;
        ASSERT_GL_MRVIEW_CONTEXT_IS_CURRENT;
      }


      void Window::initGL ()
      {
        ASSERT_GL_MRVIEW_CONTEXT_IS_CURRENT;
        GL::init ();

        font.initGL();
        gl::Enable (gl::DEPTH_TEST);
        //CONF option: MRViewImageBackgroundColour
        //CONF default: 0,0,0 (black)
        //CONF The default image background colour in the main MRView window.
        File::Config::get_RGB ("MRViewImageBackgroundColour", background_colour, 0.0f, 0.0f, 0.0f);
        gl::ClearColor (background_colour[0], background_colour[1], background_colour[2], 1.0);
        mode.reset (dynamic_cast<Mode::__Action__*> (mode_group->actions()[0])->create());
        set_mode_features();

        ASSERT_GL_MRVIEW_CONTEXT_IS_CURRENT;
      }


      template <class Event> void Window::grab_mouse_state (Event* event)
      {
        buttons_ = event->buttons();
        modifiers_ = event->modifiers() & ( FocusModifier | MoveModifier | RotateModifier );
        mouse_displacement_ = QPoint (0,0);
        mouse_position_ = event->pos();
        mouse_position_.setY (glarea->height() - mouse_position_.y());
      }


      template <class Event> void Window::update_mouse_state (Event* event)
      {
        mouse_displacement_ = mouse_position_;
        mouse_position_ = event->pos();
        mouse_position_.setY (glarea->height() - mouse_position_.y());
        mouse_displacement_ = mouse_position_ - mouse_displacement_;
      }


      void Window::keyPressEvent (QKeyEvent* event)
      {
        modifiers_ = event->modifiers() & ( FocusModifier | MoveModifier | RotateModifier );
        set_cursor();
      }


      void Window::keyReleaseEvent (QKeyEvent* event)
      {
        modifiers_ = event->modifiers() & ( FocusModifier | MoveModifier | RotateModifier );
        set_cursor();
      }


      void Window::mousePressEventGL (QMouseEvent* event)
      {
        assert (mode);

        grab_mouse_state (event);
        if (image())
          mode->mouse_press_event();

        if (tool_has_focus && modifiers_ == Qt::NoModifier) {
          if (tool_has_focus->mouse_press_event()) {
            mouse_action = NoAction;
            event->accept();
            return;
          }
        }

        int group = get_mouse_mode();

        if (buttons_ == Qt::MidButton)
          mouse_action = Pan;
        else if (group == 1) {
          if (buttons_ == Qt::LeftButton) {
            mouse_action = SetFocus;
            if (image())
              mode->set_focus_event();
          }
          else if (buttons_ == Qt::RightButton)
            mouse_action = Contrast;
        }
        else if (group == 2) {
          if (buttons_ == Qt::LeftButton)
            mouse_action = Pan;
          else if (buttons_ == Qt::RightButton)
            mouse_action = PanThrough;
        }
        else if (group == 3) {
          if (buttons_ == Qt::LeftButton)
            mouse_action = Tilt;
          else if (buttons_ == Qt::RightButton)
            mouse_action = Rotate;
        }

        set_cursor();
        event->accept();
      }


      void Window::mouseMoveEventGL (QMouseEvent* event)
      {
        assert (mode);
        if (!image())
          return;

        update_mouse_state (event);

        if (mouse_action == NoAction) {
          if (tool_has_focus)
            if (tool_has_focus->mouse_move_event())
              event->accept();
          return;
        }


        switch (mouse_action) {
          case SetFocus: mode->set_focus_event(); break;
          case Contrast: mode->contrast_event(); break;
          case Pan: mode->pan_event(); break;
          case PanThrough: mode->panthrough_event(); break;
          case Tilt: mode->tilt_event(); break;
          case Rotate: mode->rotate_event(); break;
          default: return;
        }
        event->accept();
        glarea->update();
      }


      void Window::mouseReleaseEventGL (QMouseEvent*)
      {
        assert (mode);
        mode->mouse_release_event();

        if (tool_has_focus && mouse_action == NoAction)
          if (tool_has_focus->mouse_release_event())
            return;

        mouse_action = NoAction;
        set_cursor();
      }


      void Window::wheelEventGL (QWheelEvent* event)
      {
        assert (mode);
#if QT_VERSION >= 0x050500
        QPoint delta;
        if (event->source() == Qt::MouseEventNotSynthesized)
          delta = event->angleDelta();
        else
          delta = 30 * event->pixelDelta();
#else
        QPoint delta = event->orientation() == Qt::Vertical ? QPoint (0, event->delta()) : QPoint (event->delta(), 0);
#endif
        if (delta.isNull())
          return;

        if (delta.y()) {

          if (image()) {
            grab_mouse_state (event);
            mode->mouse_press_event();

            if (buttons_ == Qt::NoButton) {

              if (modifiers_ == Qt::ControlModifier) {
                set_FOV (FOV() * std::exp (-delta.y()/1200.0));
                glarea->update();
                event->accept();
                return;
              }

              float dx = delta.y()/120.0;
              if (modifiers_ == Qt::ShiftModifier) dx *= 10.0;
              else if (modifiers_ != Qt::NoModifier)
                return;

              mode->slice_move_event (dx);
              event->accept();
              return;
            }
          }

          if (buttons_ == Qt::RightButton && modifiers_ == Qt::NoModifier) {
            if (image_group->actions().size() > 1) {
              QAction* action = image_group->checkedAction();
              int N = image_group->actions().size();
              int n = image_group->actions().indexOf (action);
              image_select_slot (image_group->actions()[(n+N+int(std::round(delta.y()/120.0)))%N]);
            }
          }
        }

      }




      bool Window::gestureEventGL (QGestureEvent* event)
      {
        assert (mode);

        if (QGesture* pan = event->gesture(Qt::PanGesture)) {
          QPanGesture* e = static_cast<QPanGesture*> (pan);
          mouse_displacement_ = QPoint (e->delta().x(), -e->delta().y());
          mode->pan_event();
        }

        if (QGesture* pinch = event->gesture(Qt::PinchGesture)) {
          QPinchGesture* e = static_cast<QPinchGesture*> (pinch);
          QPinchGesture::ChangeFlags changeFlags = e->changeFlags();
          if (changeFlags & QPinchGesture::RotationAngleChanged) {
            // TODO
          }
          if (changeFlags & QPinchGesture::ScaleFactorChanged) {
            set_FOV (FOV() / e->scaleFactor());
            glarea->update();
          }
        }
        return true;
      }



      void Window::closeEvent (QCloseEvent* event)
      {
        qApp->quit();
        event->accept();
      }


      void Window::process_commandline_option_slot ()
      {
        if (current_option >= MR::App::option.size())
          return;

        process_commandline_option();
        ++current_option;

        QTimer::singleShot (10, this, SLOT(process_commandline_option_slot()));
        glarea->update();
      }





      void Window::process_commandline_option ()
      {
        auto& opt (MR::App::option[current_option]);

#undef TOOL
#define TOOL(classname, name, description) \
        stub = lowercase (#classname "."); \
        if (stub.compare (0, stub.size(), std::string (opt.opt->id), 0, stub.size()) == 0) { \
          create_tool (tool_group->actions()[tool_id], false); \
          if (dynamic_cast<Tool::__Action__*>(tool_group->actions()[tool_id])->dock->tool->process_commandline_option (opt)) \
          return; \
        } \
        ++tool_id;

        try {

          // see whether option is claimed by any tools:
          size_t tool_id = 0;
          std::string stub;
#include "gui/mrview/tool/list.h"


          // process general options:
          if (opt.opt->is ("mode")) {
            int n = int(opt[0]) - 1;
            if (n < 0 || n >= mode_group->actions().size())
              throw Exception ("invalid mode index \"" + str(n) + "\" in batch command");
            select_mode_slot (mode_group->actions()[n]);
            return;
          }

          if (opt.opt->is ("size")) {
            vector<int> glsize = parse_ints (opt[0]);
            if (glsize.size() != 2)
              throw Exception ("invalid argument \"" + std::string(opt.args[0]) + "\" to -size batch command");
            if (glsize[0] < 1 || glsize[1] < 1)
              throw Exception ("values provided to -size option must be positive");
            QSize oldsize = glarea->size();
            QSize winsize = size();
            resize (winsize.width() - oldsize.width() + glsize[0], winsize.height() - oldsize.height() + glsize[1]);
            return;
          }

          if (opt.opt->is ("reset")) {
            reset_view_slot();
            return;
          }

          if (opt.opt->is ("fov")) {
            float fov = opt[0];
            set_FOV (fov);
            glarea->update();
            return;
          }

          if (opt.opt->is ("focus")) {
            try {
              auto pos = parse_floats (opt[0]);
              if (pos.size() != 3)
                throw Exception ("-focus option expects a comma-separated list of 3 floating-point values");
              set_focus (Eigen::Vector3f { float(pos[0]), float(pos[1]), float(pos[2]) });
            }
            catch (Exception& E) {
              try {
                show_crosshairs_action->setChecked (to<bool> (opt[0]));
              }
              catch (Exception& E2) {
                throw Exception ("-focus option expects a boolean or a comma-separated list of 3 floating-point values");
              }
            }
            glarea->update();
            return;
          }

          if (opt.opt->is ("target")) {
            if (image()) {
              vector<default_type> pos = parse_floats (opt[0]);
              if (pos.size() != 3)
                throw Exception ("-target option expects a comma-separated list of 3 floating-point values");
              set_target (Eigen::Vector3f { float(pos[0]), float(pos[1]), float(pos[2]) });
              glarea->update();
            }
            return;
          }

          if (opt.opt->is ("voxel")) {
            if (image()) {
              vector<default_type> pos = parse_floats (opt[0]);
              if (pos.size() != 3)
                throw Exception ("-voxel option expects a comma-separated list of 3 floating-point values");
              set_focus (image()->transform().voxel2scanner.cast<float>() *  Eigen::Vector3f { float(pos[0]), float(pos[1]), float(pos[2]) });
              glarea->update();
            }
            return;
          }

          if (opt.opt->is ("volume")) {
            if (image()) {
              auto pos = parse_ints (opt[0]);
              for (size_t n = 0; n < std::min (pos.size(), image()->image.ndim()); ++n) {
                if (pos[n] < 0 || pos[n] >= image()->image.size(n+3))
                  throw Exception ("volume index outside of image dimensions");
                set_image_volume (n+3, pos[n]);
                set_image_navigation_menu();
              }
              glarea->update();
            }
            return;
          }

          if (opt.opt->is ("fov")) {
            float fov = opt[0];
            set_FOV (fov);
            glarea->update();
            return;
          }

          if (opt.opt->is ("plane")) {
            int n = opt[0];
            set_plane (n);
            glarea->update();
            return;
          }

          if (opt.opt->is ("lock")) {
            bool n = opt[0];
            snap_to_image_action->setChecked (n);
            snap_to_image_slot();
            return;
          }

          if (opt.opt->is ("select_image")) {
            int n = int(opt[0]) - 1;
            if (n < 0 || n >= image_group->actions().size())
              throw Exception ("invalid image index requested for option -select_image");
            image_select_slot (image_group->actions()[n]);
            return;
          }

          if (opt.opt->is ("load")) {
            vector<std::unique_ptr<MR::Header>> list;
            try { list.push_back (make_unique<MR::Header> (MR::Header::open (opt[0]))); }
            catch (Exception& e) { e.display(); }
            add_images (list);
            return;
          }

          if (opt.opt->is ("autoscale")) {
            image_reset_slot();
            return;
          }

          if (opt.opt->is ("colourmap")) {
            int n = int(opt[0]) - 1;
            if (n < 0 || n >= static_cast<int>(colourmap_button->colourmap_actions.size()))
              throw Exception ("invalid image colourmap index \"" + str(n+1) + "\" requested in batch command");
            colourmap_button->set_colourmap_index(n);
            return;
          }

          if (opt.opt->is ("interpolation")) {
            try {
              image_interpolate_action->setChecked (to<bool> (opt[0]));
            }
            catch (Exception& E) {
              throw Exception ("-interpolation option expects a boolean");
            }
            image_interpolate_slot();
            return;
          }

          if (opt.opt->is ("intensity_range")) {
            if (image()) {
              vector<default_type> param = parse_floats (opt[0]);
              if (param.size() != 2)
                throw Exception ("-intensity_range options expects comma-separated list of two floating-point values");
              image()->set_windowing (param[0], param[1]);
              glarea->update();
            }
            return;
          }

          if (opt.opt->is ("position")) {
            vector<int> pos = opt[0];
            if (pos.size() != 2)
              throw Exception ("invalid argument \"" + std::string(opt[0]) + "\" to -position option");
            move (pos[0], pos[1]);
            return;
          }

          if (opt.opt->is ("fullscreen")) {
            full_screen_action->setChecked (true);
            full_screen_slot();
            return;
          }

          if (opt.opt->is ("noannotations")) {
            toggle_annotations_slot ();
            return;
          }

          if (opt.opt->is ("comments")) {
            try {
              show_comments_action->setChecked (to<bool> (opt[0]));
            }
            catch (Exception& E) {
              throw Exception ("-comments option expects a boolean");
            }
            glarea->update();
            return;
          }

          if (opt.opt->is ("voxelinfo")) {
            try {
              show_voxel_info_action->setChecked (to<bool> (opt[0]));
            }
            catch (Exception& E) {
              throw Exception ("-voxelinfo option expects a boolean");
            }
            glarea->update();
            return;
          }

          if (opt.opt->is ("orientationlabel")) {
            try {
              show_orientation_labels_action->setChecked (to<bool> (opt[0]));
            }
            catch (Exception& E) {
              throw Exception ("-orientationlabel option expects a boolean");
            }
            glarea->update();
            return;
          }

          if (opt.opt->is ("colourbar")) {
            try {
              show_colourbar_action->setChecked (to<bool> (opt[0]));
            }
            catch (Exception& E) {
              throw Exception ("-colourbar option expects a boolean");
            }
            glarea->update();
            return;
          }

          if (opt.opt->is ("imagevisible")) {
            bool visible;
            try {
              visible = to<bool> (opt[0]);
            }
            catch (Exception& E) {
              throw Exception ("-imagevisible option expects a boolean");
            }
            if (image_hide_action->isChecked() == visible)
              set_image_visibility (visible);
            return;
          }


          if (opt.opt->is ("fps")) {
            show_FPS = true;
            return;
          }

          if (opt.opt->is ("exit")) {
            qApp->processEvents();
            qApp->quit();
            return;
          }

          assert (opt.opt->is ("info") or opt.opt->is ("debug") or ("shouldn't reach here!" && false));
        }
        catch (Exception& E) {
          E.display();
          qApp->quit();
        }
      }


      void Window::add_commandline_options (MR::App::OptionList& options)
      {
        options
          + OptionGroup ("View options")

          + Option ("mode", "Switch to view mode specified by the integer index, as per the view menu.").allow_multiple()
          +   Argument ("index").type_integer()

          + Option ("load", "Load image specified and make it current.").allow_multiple()
          +   Argument ("image").type_image_in()

          + Option ("reset", "Reset the view according to current image. This resets the FOV, projection and focus.").allow_multiple()

          + Option ("fov", "Set the field of view, in mm.").allow_multiple()
          +   Argument ("value").type_float()

          + Option ("focus", "Either set the position of the crosshairs in scanner coordinates, "
              "with the new position supplied as a comma-separated list of floating-point values or "
              "show or hide the focus cross hair using a boolean value as argument.").allow_multiple()
          +   Argument ("x,y,z or boolean")

          + Option ("target", "Set the target location for the viewing window (the scanner coordinate "
              "that will appear at the centre of the viewing window")
          +   Argument ("x,y,z").type_sequence_float()

          + Option ("voxel", "Set the position of the crosshairs in voxel coordinates, "
              "relative the image currently displayed. The new position should be supplied "
              "as a comma-separated list of floating-point values.").allow_multiple()
          +   Argument ("x,y,z").type_sequence_float()

          + Option ("volume", "Set the volume index for the image displayed, "
              "as a comma-separated list of integers.").allow_multiple()
          +   Argument ("idx").type_sequence_int()

          + Option ("plane", "Set the viewing plane, according to the mappping 0: sagittal; 1: coronal; 2: axial.").allow_multiple()
          +   Argument ("index").type_integer (0,2)

          + Option ("lock", "Set whether view is locked to image axes (0: no, 1: yes).").allow_multiple()
          +   Argument ("yesno").type_bool()

          + Option ("select_image", "Switch to image number specified, with reference to the list of currently loaded images.").allow_multiple()
          +   Argument ("index").type_integer (0)

          + Option ("autoscale", "Reset the image scaling to automatically determined range.").allow_multiple()

          + Option ("interpolation", "Enable or disable image interpolation in main image.").allow_multiple()
          +   Argument ("boolean").type_bool ()

          + Option ("colourmap", "Switch the image colourmap to that specified, as per the colourmap menu.").allow_multiple()
          +   Argument ("index").type_integer (0)

          + Option ("noannotations", "Hide all image annotation overlays").allow_multiple()

          + Option ("comments", "Show or hide image comments overlay.").allow_multiple()
          +   Argument ("boolean").type_bool ()

          + Option ("voxelinfo", "Show or hide voxel information overlay.").allow_multiple()
          +   Argument ("boolean").type_bool ()

          + Option ("orientationlabel", "Show or hide orientation label overlay.").allow_multiple()
          +   Argument ("boolean").type_bool ()

          + Option ("colourbar", "Show or hide colourbar overlay.").allow_multiple()
          +   Argument ("boolean").type_bool ()

          + Option ("imagevisible", "Show or hide the main image.").allow_multiple()
          +   Argument ("boolean").type_bool ()

          + Option ("intensity_range", "Set the image intensity range to that specified.").allow_multiple()
          +   Argument ("min,max").type_sequence_float()

          + OptionGroup ("Window management options")

          + Option ("size", "Set the size of the view area, in pixel units.").allow_multiple()
          +   Argument ("width,height").type_sequence_int()

          + Option ("position", "Set the position of the main window, in pixel units.").allow_multiple()
          +   Argument ("x,y").type_sequence_int()

          + Option ("fullscreen", "Start fullscreen.")

          + Option ("exit", "Quit MRView.")

          + OptionGroup ("Debugging options")

          + Option ("fps", "Display frames per second, averaged over the last 10 frames. "
              "The maximum over the last 3 seconds is also displayed.")

          + OptionGroup ("Other options")

          + NoRealignOption;

      }




    }
  }
}



