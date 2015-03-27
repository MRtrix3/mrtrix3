#include "app.h"
#include "timer.h"
#include "file/config.h"
#include "image/header.h"
#include "image/voxel.h"
#include "image/copy.h"
#include "gui/opengl/gl.h"
#include "gui/opengl/lighting.h"
#include "gui/dialog/file.h"
#include "gui/dialog/opengl.h"
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
        QGLWidget (GL::core_format(), &parent),
        main (parent) {
          setCursor (Cursor::crosshair);
          setMouseTracking (true);
          setAcceptDrops (true);
          setMinimumSize (256, 256);
          setFocusPolicy (Qt::StrongFocus);
          QFont font_ = font();
          font_.setPointSize (MR::File::Config::get_int ("FontSize", 10));
          setFont (font_);
          QSizePolicy policy (QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
          policy.setHorizontalStretch (255);
          policy.setVerticalStretch (255);
          setSizePolicy (policy);
        }

      QSize Window::GLArea::sizeHint () const {
        std::string init_size_string = lowercase (MR::File::Config::get ("MRViewInitWindowSize"));
        std::vector<int> init_window_size;
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
          VecPtr<MR::Image::Header> list;
          QList<QUrl> urlList = mimeData->urls();
          for (int i = 0; i < urlList.size() && i < 32; ++i) {
            try {
              list.push_back (new MR::Image::Header (urlList.at (i).path().toUtf8().constData()));
            }
            catch (Exception& e) {
              e.display();
            }
          }
          if (list.size())
            main.add_images (list);
        }
      }

      void Window::GLArea::initializeGL () {
        main.initGL();
      }
      void Window::GLArea::paintGL () {
        main.paintGL();
      }
      void Window::GLArea::mousePressEvent (QMouseEvent* event) {
        main.mousePressEventGL (event);
      }
      void Window::GLArea::mouseMoveEvent (QMouseEvent* event) {
        main.mouseMoveEventGL (event);
      }
      void Window::GLArea::mouseReleaseEvent (QMouseEvent* event) {
        main.mouseReleaseEventGL (event);
      }
      void Window::GLArea::wheelEvent (QWheelEvent* event) {
        main.wheelEventGL (event);
      }










      //CONF option: MRViewFocusModifierKey 
      //CONF default: alt (cmd on MacOSX) 
      //CONF modifier key to select focus mode in MRView. Valid
      //CONF choices include shift, alt, ctrl, meta (on MacOSX: shift, alt,
      //CONF ctrl, cmd).
      
      //CONF option: MRViewMoveModifierKey 
      //CONF default: shift
      //CONF modifier key to select move mode in MRView. Valid
      //CONF choices include shift, alt, ctrl, meta (on MacOSX: shift, alt,
      //CONF ctrl, cmd).
      
      //CONF option: MRViewRotateModifierKey
      //CONF default: ctrl
      //CONF modifier key to select rotate mode in MRView. Valid
      //CONF choices include shift, alt, ctrl, meta (on MacOSX: shift, alt,
      //CONF ctrl, cmd).


      // Main Window class:

      Window::Window() :
        glarea (new GLArea (*this)),
        glrefresh_timer (new QTimer (this)),
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
        orient (NAN, NAN, NAN, NAN),
        field_of_view (100.0),
        anatomical_plane (2),
        colourbar_position_index (2),
        snap_to_image_axes_and_voxel (true),
        tool_has_focus (nullptr)
      {

        setDockOptions (AllowTabbedDocks | VerticalTabs);
        setDocumentMode (true);

        Options opt = get_options ("batch");
        for (size_t n = 0; n < opt.size(); ++n) {
          std::ifstream batch_file (opt[n][0].c_str());
          if (!batch_file) 
            throw Exception ("error opening batch file \"" + opt[n][0] + "\": " + strerror (errno));
          std::string command;
          while (getline (batch_file, command)) 
            batch_commands.push_back (command);
        }

        opt = get_options ("run");
        for (size_t n = 0; n < opt.size(); ++n) 
          batch_commands.push_back (opt[n][0]);


        //CONF option: IconSize
        //CONF default: 24
        //CONF The size of the icons in the main MRView toolbar.
        setWindowTitle (tr ("MRView"));
        setWindowIcon (QPixmap (":/mrtrix.png"));
        { 
          int iconsize = MR::File::Config::get_int ("IconSize", 24);
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
        button->setToolTip (tr ("File menu"));
        button->setIcon (QIcon (":/start.svg"));
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

        next_image_volume_group_action = image_menu->addAction (tr ("Next volume group"), this, SLOT (image_next_volume_group_slot()));
        next_image_volume_group_action->setShortcut (tr ("Shift+Right"));
        addAction (next_image_volume_group_action);

        prev_image_volume_group_action = image_menu->addAction (tr("Previous volume group"), this, SLOT (image_previous_volume_group_slot()));
        prev_image_volume_group_action->setShortcut (tr ("Shift+Left"));
        addAction (prev_image_volume_group_action);

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
        button->setToolTip (tr ("Image menu"));
        button->setIcon (QIcon (":/image.svg"));
        button->setPopupMode (QToolButton::InstantPopup);
        button->setMenu (image_menu);
        toolbar->addWidget (button);


        // Colourmap menu:

        colourmap_button = new ColourMapButton (this, *this, true, true, false);
        colourmap_button->setText ("Colourmap");
        colourmap_button->setToolButtonStyle (button_style);
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

        image_interpolate_action = colourmap_menu->addAction (tr ("Interpolate"), this, SLOT (image_interpolate_slot()));
        image_interpolate_action->setShortcut (tr ("I"));
        image_interpolate_action->setCheckable (true);
        image_interpolate_action->setChecked (true);
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

        action = menu->addAction (tr ("Toggle all annotations"), this, SLOT (toggle_annotations_slot()));
        action->setShortcut (tr("Space"));
        addAction (action);

        show_crosshairs_action = menu->addAction (tr ("Show focus"), this, SLOT (updateGL()));
        show_crosshairs_action->setShortcut (tr("F"));
        show_crosshairs_action->setCheckable (true);
        show_crosshairs_action->setChecked (true);
        addAction (show_crosshairs_action);

        show_comments_action = menu->addAction (tr ("Show comments"), this, SLOT (updateGL()));
        show_comments_action->setToolTip (tr ("Show/hide image comments\n\nShortcut: H"));
        show_comments_action->setShortcut (tr("H"));
        show_comments_action->setCheckable (true);
        show_comments_action->setChecked (true);
        addAction (show_comments_action);

        show_voxel_info_action = menu->addAction (tr ("Show voxel information"), this, SLOT (updateGL()));
        show_voxel_info_action->setShortcut (tr("V"));
        show_voxel_info_action->setCheckable (true);
        show_voxel_info_action->setChecked (true);
        addAction (show_voxel_info_action);

        show_orientation_labels_action = menu->addAction (tr ("Show orientation labels"), this, SLOT (updateGL()));
        show_orientation_labels_action->setShortcut (tr("O"));
        show_orientation_labels_action->setCheckable (true);
        show_orientation_labels_action->setChecked (true);
        addAction (show_orientation_labels_action);

        show_colourbar_action = menu->addAction (tr ("Show colour bar"), this, SLOT (updateGL()));
        show_colourbar_action->setShortcut (tr("B"));
        show_colourbar_action->setCheckable (true);
        show_colourbar_action->setChecked (true);
        addAction (show_colourbar_action);

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
        button->setToolTip (tr ("Display"));
        button->setIcon (QIcon (":/mode.svg"));
        button->setMenu (menu);
        button->setPopupMode (QToolButton::InstantPopup);
        toolbar->addWidget (button);




        // Tool menu:
        tool_group = new QActionGroup (this);
        tool_group->setExclusive (false);
        connect (tool_group, SIGNAL (triggered (QAction*)), this, SLOT (select_tool_slot (QAction*)));

        menu = new QMenu (tr ("Tools"), this);
#undef TOOL
#define TOOL(classname, name, description) \
        menu->addAction (new Action<Tool::classname> (tool_group, #name, #description, n++));
#define TOOL_OPTION(classname, name, description) TOOL(classname, name, description)
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
        button->setToolTip (tr ("Select additional tools..."));
        button->setIcon (QIcon (":/tools.svg"));
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

        // Help menu:

        menu = new QMenu (tr ("Help"), this);

        menu->addAction (tr("OpenGL"), this, SLOT (OpenGL_slot()));
        menu->addAction (tr ("About"), this, SLOT (about_slot()));
        menu->addAction (tr ("about Qt"), this, SLOT (aboutQt_slot()));

        button = new QToolButton (this);
        button->setText ("Help");
        button->setToolButtonStyle (button_style);
        button->setToolTip (tr ("Help"));
        button->setIcon (QIcon (":/help.svg"));
        button->setPopupMode (QToolButton::InstantPopup);
        button->setMenu (menu);
        toolbar->addWidget (button);



        lighting_ = new GL::Lighting (this);
        connect (lighting_, SIGNAL (changed()), this, SLOT (updateGL()));

        set_image_menu ();

        //CONF option: MRViewColourBarPosition
        //CONF default: bottomright
        //CONF The position of the colourbar within the main window in MRView.
        //CONF Valid values are: bottomleft, bottomright, topleft, topright.
        std::string cbar_pos = lowercase (MR::File::Config::get ("MRViewColourBarPosition"));
        if (cbar_pos.size()) {
          if (cbar_pos == "bottomleft") colourbar_position_index = 1;
          else if (cbar_pos == "bottomright") colourbar_position_index = 2;
          else if (cbar_pos == "topleft") colourbar_position_index = 3;
          else if (cbar_pos == "topright") colourbar_position_index = 4;
          else 
            WARN ("invalid specifier \"" + cbar_pos + "\" for config file entry \"MRViewColourBarPosition\"");
        }

        glrefresh_timer->setSingleShot (true);
        connect (glrefresh_timer, SIGNAL (timeout()), glarea, SLOT (updateGL()));
      }




      Window::~Window ()
      {
        mode = nullptr;
        delete glarea;
        delete glrefresh_timer;
      }







      void Window::image_open_slot ()
      {
        std::vector<std::string> image_list = Dialog::File::get_images (this, "Select images to open");
        if (image_list.empty())
          return;

        VecPtr<MR::Image::Header> list;
        for (size_t n = 0; n < image_list.size(); ++n) {
          try {
            list.push_back (new MR::Image::Header (image_list[n]));
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
          VecPtr<MR::Image::Header> list;
          list.push_back (new MR::Image::Header (folder));
          add_images (list);
        }
        catch (Exception& E) {
          E.display();
        }
      }




      void Window::add_images (VecPtr<MR::Image::Header>& list)
      {
        for (size_t i = 0; i < list.size(); ++i) {
          QAction* action = new Image (*this, *list[i]);
          image_group->addAction (action);
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
          MR::Image::Buffer<cfloat> dest (image_name, image()->header());
          auto src (image()->voxel());
          MR::Image::copy_with_progress (src, dest.voxel());
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
        mode = dynamic_cast<GUI::MRView::Mode::__Action__*> (action)->create (*this);
        set_mode_features();
        emit modeChanged();
        updateGL();
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
          tool = dynamic_cast<Tool::__Action__*>(action)->create (*this);
          connect (tool, SIGNAL (visibilityChanged (bool)), action, SLOT (setChecked (bool)));
          if (MR::File::Config::get_int ("MRViewDockFloating", 0))
            return;
          for (int i = 0; i < tool_group->actions().size(); ++i) {
            Tool::Dock* other_tool = dynamic_cast<Tool::__Action__*>(tool_group->actions()[i])->dock;
            if (other_tool && other_tool != tool) {
              QList<QDockWidget* > list = QMainWindow::tabifiedDockWidgets (other_tool);
              if (list.size())
                QMainWindow::tabifyDockWidget (list.last(), tool);
              else
                QMainWindow::tabifyDockWidget (other_tool, tool);
              tool->setFloating (false);
              tool->raise();
              return;
            }
          }
          //CONF option: MRViewDockFloating
          //CONF default: 0 (false)
          //CONF Whether Tools should start docked in the main window, or
          //CONF floating (detached from the main window).
          tool->setFloating (MR::File::Config::get_int ("MRViewDockFloating", 0));
          tool->show();
        }
        if (action->isChecked()) {
          if (!tool->isVisible())
            tool->show();
          tool->raise();
        } else {
          tool->close();
        }
        updateGL();
      }


      void Window::selected_colourmap (size_t colourmap, const ColourMapButton&)
      {
          Image* imagep = image();
          if (imagep) {
            imagep->set_colourmap (colourmap);
            updateGL();
          }
      }



      void Window::selected_custom_colour (const QColor& colour, const ColourMapButton&)
      {
          Image* imagep = image();
          if (imagep) {
            std::array<GLubyte, 3> c_colour{{GLubyte(colour.red()), GLubyte(colour.green()), GLubyte(colour.blue())}};
            imagep->set_colour(c_colour);
            updateGL();
          }
      }



      void Window::invert_scaling_slot ()
      {
        if (image()) {
          image()->set_invert_scale (invert_scale_action->isChecked());
          updateGL();
        }
      }



      void Window::snap_to_image_slot ()
      {
        if (image()) {
          snap_to_image_axes_and_voxel = snap_to_image_action->isChecked();
          if (snap_to_image_axes_and_voxel) 
            mode->reset_orientation();
          updateGL();
        }
      }




      void Window::on_scaling_changed () 
      {
        emit scalingChanged();
      }




      void Window::updateGL () 
      { 
        if (glrefresh_timer->isActive())
          return;

        glrefresh_timer->start();
      }



      void Window::image_reset_slot ()
      {
        Image* imagep = image();
        if (imagep) {
          imagep->reset_windowing();
          on_scaling_changed();
          updateGL();
        }
      }



      void Window::image_interpolate_slot ()
      {
        Image* imagep = image();
        if (imagep) {
          imagep->set_interpolate (image_interpolate_action->isChecked());
          updateGL();
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
        updateGL();
      }


      void Window::reset_view_slot ()
      {
        if (image())
          mode->reset_event();
      }


      void Window::slice_next_slot () 
      {
        mode->slice_move_event (1);
      }

      void Window::slice_previous_slot () 
      {
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
        set_image_volume (3, image()->interp[3]+1);
      }




      void Window::image_previous_volume_slot ()
      {
        set_image_volume (3, image()->interp[3]-1);
      }




      void Window::image_next_volume_group_slot () 
      {
        set_image_volume (4, image()->interp[4]+1);
      }




      void Window::image_previous_volume_group_slot ()
      {
        set_image_volume (4, image()->interp[4]-1);
      }




      void Window::image_select_slot (QAction* action)
      {
        action->setChecked (true);
        image_interpolate_action->setChecked (image()->interpolate());
        size_t cmap_index = image()->colourmap;
        colourmap_button->colourmap_actions[cmap_index]->setChecked (true);
        invert_scale_action->setChecked (image()->scale_inverted());
        mode->image_changed_event();
        setWindowTitle (image()->interp.name().c_str());
        set_image_navigation_menu();
        image()->set_allowed_features (
            mode->features & Mode::ShaderThreshold,
            mode->features & Mode::ShaderTransparency,
            mode->features & Mode::ShaderLighting);
        emit imageChanged();
        updateGL();
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
        updateGL();
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
        updateGL();
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
        bool show_next_volume (false), show_prev_volume (false);
        bool show_next_volume_group (false), show_prev_volume_group (false);
        Image* imagep = image();
        if (imagep) {
          if (imagep->interp.ndim() > 3) {
            if (imagep->interp[3] > 0) 
              show_prev_volume = true;
            if (imagep->interp[3] < imagep->interp.dim(3)-1) 
              show_next_volume = true;

            if (imagep->interp.ndim() > 4) {
              if (imagep->interp[4] > 0) 
                show_prev_volume_group = true;
              if (imagep->interp[4] < imagep->interp.dim(4)-1) 
                show_next_volume_group = true;
            }
          }
        }
        prev_image_volume_action->setEnabled (show_prev_volume);
        next_image_volume_action->setEnabled (show_next_volume);
        prev_image_volume_group_action->setEnabled (show_prev_volume_group);
        next_image_volume_group_action->setEnabled (show_next_volume_group);
      }





      void Window::OpenGL_slot ()
      {
        Dialog::OpenGL gl (this, glarea->format());
        gl.exec();
      }




      void Window::about_slot ()
      {
        std::string message = 
          std::string ("<h1>MRView</h1>The MRtrix viewer, version ") + App::mrtrix_version + "<br>"
          "<em>" + str (8*sizeof (size_t)) + " bit " 
#ifdef NDEBUG
          "release"
#else
          "debug"
#endif
          " version, built " + App::build_date +  "</em><p>"
          "<h4>Authors:</h4>" + MR::join (MR::split (App::AUTHOR, ",;&\n", true), "<br>") + 
          "<p><em>" + App::COPYRIGHT + "</em>";

        QMessageBox::about (this, tr ("About MRView"), message.c_str());
      }




      void Window::aboutQt_slot ()
      {
        QMessageBox::aboutQt (this);
      }


      void Window::paintGL ()
      {
        gl::Enable (gl::MULTISAMPLE);
        if (mode->in_paint())
          return;

        gl::DrawBuffer (gl::BACK);
        mode->paintGL();
      }


      void Window::initGL ()
      {
        GL::init ();

        font.initGL();

        gl::ClearColor (0.0, 0.0, 0.0, 0.0);
        gl::Enable (gl::DEPTH_TEST);

        mode = dynamic_cast<Mode::__Action__*> (mode_group->actions()[0])->create (*this);
        set_mode_features();

        if (batch_commands.size()) 
          QTimer::singleShot (0, this, SLOT (process_batch_command()));
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

        if (event->orientation() == Qt::Vertical) {

          if (image()) {
            grab_mouse_state (event);
            mode->mouse_press_event();

            if (buttons_ == Qt::NoButton) {

              if (modifiers_ == Qt::ControlModifier) {
                set_FOV (FOV() * std::exp (-event->delta()/1200.0));
                updateGL();
                event->accept();
                return;
              }

              int delta = event->delta() / 120.0;
              if (modifiers_ == Qt::ShiftModifier) delta *= 10.0;
              else if (modifiers_ != Qt::NoModifier) 
                return;

              mode->slice_move_event (delta);
              event->accept();
              return;
            }
          }

          if (buttons_ == Qt::LeftButton && modifiers_ == Qt::NoModifier) {
            int current = 0, num = 0;
            for (int i = 0; i < mode_action_group->actions().size(); ++i) {
              if (mode_action_group->actions()[current] != mode_action_group->checkedAction())
                current = num;
              if (mode_action_group->actions()[i]->isEnabled())
                ++num;
            }

            current = (current + num - int(event->delta()/120.0)) % num;

            num = 0;
            for (int i = 0; i < mode_action_group->actions().size(); ++i) {
              if (mode_action_group->actions()[i]->isEnabled()) {
                if (current == num) {
                  mode_action_group->actions()[i]->setChecked (true);
                  break;
                }
                ++num;
              }
            }
            mouse_action = NoAction;
            set_cursor();
            return;
          }

          if (buttons_ == Qt::RightButton && modifiers_ == Qt::NoModifier) {
            if (image_group->actions().size() > 1) {
              QAction* action = image_group->checkedAction();
              int N = image_group->actions().size();
              int n = image_group->actions().indexOf (action);
              image_select_slot (image_group->actions()[(n+N+int(event->delta()/120.0))%N]);
            }
          }
        }

      }


      void Window::closeEvent (QCloseEvent* event) 
      {
        qApp->quit();
        event->accept();
      }


      void Window::process_batch_command ()
      {
        assert (batch_commands.size());
        try {
          std::string line;
          do {
            if (batch_commands.empty())
              return;
            line = batch_commands[0];
            batch_commands.erase (batch_commands.begin(), batch_commands.begin()+1);
            line = strip (line.substr (0, line.find_first_of ('#')));
          } while (line.empty());

          std::string cmd = line.substr (0, line.find_first_of (" :\t"));
          std::string args;
          if (line.size() > cmd.size()+1)
            args = strip (line.substr (cmd.size()+1));

          // starts of commands proper:
          
          // BATCH_COMMAND view.mode index # Switch to view mode specified by the integer index. as per the view menu.
          if (cmd == "view.mode") { 
            int n = to<int> (args) - 1;
            if (n < 0 || n >= mode_group->actions().size())
              throw Exception ("invalid mode index \"" + args + "\" in batch command");
            select_mode_slot (mode_group->actions()[n]);
          }

          // BATCH_COMMAND view.size width,height # Set the size of the view area, in pixel units.
          else if (cmd == "view.size") { 
            std::vector<int> glsize = parse_ints (args);
            if (glsize.size() != 2)
              throw Exception ("invalid argument \"" + args + "\" to view.size batch command");
            QSize oldsize = glarea->size();
            QSize winsize = size();
            resize (winsize.width() - oldsize.width() + glsize[0], winsize.height() - oldsize.height() + glsize[1]);
          }

          // BATCH_COMMAND view.reset # Reset the view according to current image. This resets the FOV, projection, and focus.
          else if (cmd == "view.reset") 
            reset_view_slot();

          // BATCH_COMMAND view.fov num # Set the field of view, in mm.
          else if (cmd == "view.fov") { 
            float fov = to<float> (args);
            set_FOV (fov);
            updateGL();
          }
          
          // BATCH_COMMAND view.focus x,y,z # Set the position of the crosshairs in scanner coordinates, with the new position supplied as a comma-separated list of floating-point values. 
          else if (cmd == "view.focus") { 
            std::vector<float> pos = parse_floats (args);
            if (pos.size() != 3) 
              throw Exception ("batch command \"" + cmd + "\" expects a comma-separated list of 3 floating-point values");
            set_focus (Point<> (pos[0], pos[1], pos[2]));
            updateGL();
          }
          
          // BATCH_COMMAND view.voxel x,y,z # Set the position of the crosshairs in voxel coordinates, relative the image currently displayed. The new position should be supplied as a comma-separated list of floating-point values. 
          else if (cmd == "view.voxel") { 
            if (image()) {
              std::vector<float> pos = parse_floats (args);
              if (pos.size() != 3) 
                throw Exception ("batch command \"" + cmd + "\" expects a comma-separated list of 3 floating-point values");
              set_focus (image()->interp.voxel2scanner (Point<> (pos[0], pos[1], pos[2])));
              updateGL();
            }
          }

          // BATCH_COMMAND view.fov num # Set the field of view, in mm.
          else if (cmd == "view.fov") { 
            float fov = to<float> (args);
            set_FOV (fov);
            updateGL();
          }
          
          // BATCH_COMMAND view.plane num # Set the viewing plane, according to the mappping 0: sagittal; 1: coronal; 2: axial.
          else if (cmd == "view.plane") { 
            int n = to<int> (args);
            set_plane (n);
            updateGL();
          }

          // BATCH_COMMAND view.lock # Set whether view is locked to image axes (0: no, 1: yes).
          else if (cmd == "view.lock") { 
            bool n = to<bool> (args);
            snap_to_image_action->setChecked (n);
            snap_to_image_slot();
          }

          // BATCH_COMMAND image.select index # Switch to image number specified, with reference to the list of currently loaded images.
          else if (cmd == "image.select") {
            int n = to<int> (args) - 1;
            if (n < 0 || n >= image_group->actions().size())
              throw Exception ("invalid image index requested in batch command");
            image_select_slot (image_group->actions()[n]);
          }

          // BATCH_COMMAND image.load path # Load image specified and make it current.
          else if (cmd == "image.load") { 
            VecPtr<MR::Image::Header> list; 
            try { list.push_back (new MR::Image::Header (args)); }
            catch (Exception& e) { e.display(); }
            add_images (list);
          }

          // BATCH_COMMAND image.reset # Reset the image scaling.
          else if (cmd == "image.reset") 
            image_reset_slot();

          // BATCH_COMMAND image.colourmap index # Switch the image colourmap to that specified, as per the colourmap menu.
          else if (cmd == "image.colourmap") { 
            int n = to<int> (args) - 1;
            if (n < 0 || n >= static_cast<int>(colourmap_button->colourmap_actions.size()))
              throw Exception ("invalid image colourmap index \"" + args + "\" requested in batch command");
            colourmap_button->set_colourmap_index(n);
          }

          // BATCH_COMMAND image.range min max # Set the image intensity range to that specified
          else if (cmd == "image.range") { 
            if (image()) {
              std::vector<std::string> param = split (args);
              if (param.size() != 2) 
                throw Exception ("batch command image.range expects two arguments");
              image()->set_windowing (to<float> (param[0]), to<float> (param[1]));
              updateGL();
            }
          }

          // BATCH_COMMAND tool.open index # Start the tool specified, indexed as per the tool menu
          else if (cmd == "tool.open") {
            int n = to<int> (args) - 1;
            if (n < 0 || n >= tool_group->actions().size())
              throw Exception ("invalid tool index \"" + args + "\" requested in batch command");
            tool_group->actions()[n]->setChecked (true);
            select_tool_slot (tool_group->actions()[n]);
          }

          // BATCH_COMMAND window.position x,y # Set the position of the main window, in pixel units.
          else if (cmd == "window.position") { 
            std::vector<int> pos = parse_ints (args);
            if (pos.size() != 2)
              throw Exception ("invalid argument \"" + args + "\" to view.position batch command");
            move (pos[0], pos[1]);
          }

          // BATCH_COMMAND window.fullscreen # Show fullscreen or windowed (0: windowed, 1: fullscreen).
          else if (cmd == "window.fullscreen") { 
            bool n = to<bool> (args);
            full_screen_action->setChecked (n);
            full_screen_slot();
          }

          // BATCH_COMMAND exit # quit MRView.
          else if (cmd == "exit") 
            qApp->quit();

          else { // process by tool
            int n = 0; 
            while (n < tools()->actions().size()) {
              Tool::Dock* dock = dynamic_cast<Tool::__Action__*>(tools()->actions()[n])->dock;
              if (dock)
                if (dock->tool->process_batch_command (cmd, args)) 
                  break;
              ++n;
            }
            if (n >= tools()->actions().size())
              WARN ("batch command \"" + cmd + "\" unclaimed by main window or any active tool - ignored");
          }


          // end of commands
        }

        catch (Exception& E) {
          E.display();
          qApp->quit();
        }

        if (batch_commands.size())
          QTimer::singleShot (0, this, SLOT (process_batch_command()));
      }

    }
  }
}



