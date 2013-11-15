#include "gui/opengl/gl.h"
#include "gui/opengl/lighting.h"

#include <QMessageBox>
#include <QAction>
#include <QMenuBar>
#include <QStatusBar>
#include <QDockWidget>
#include <QGLWidget>

#include "app.h"
#include "file/config.h"
#include "image/header.h"
#include "image/voxel.h"
#include "image/copy.h"
#include "gui/dialog/file.h"
#include "gui/dialog/opengl.h"
#include "gui/dialog/image_properties.h"
#include "gui/mrview/mode/base.h"
#include "gui/mrview/mode/list.h"
#include "gui/mrview/tool/base.h"
#include "gui/mrview/tool/list.h"


#include "timer.h"

namespace MR
{
  namespace GUI
  {
    namespace MRView
    {
      using namespace App;

#define MODE(classname, specifier, name, description) #specifier ", "
#define MODE_OPTION(classname, specifier, name, description) MODE(classname, specifier, name, description)

      const OptionGroup Window::options = OptionGroup ("General options")
        + Option ("mode", "select initial display mode by its short ID. Valid mode IDs are: "
#include "gui/mrview/mode/list.h"
            )
        + Argument ("name");
#undef MODE
#undef MODE_OPTION


      namespace {

        Qt::KeyboardModifiers get_modifier (const char* key, Qt::KeyboardModifiers default_key) {
          std::string value = lowercase (MR::File::Config::get (key));
          if (value.empty()) 
            return default_key;

          if (value == "shift") return Qt::ShiftModifier;
          if (value == "ctrl") return Qt::ControlModifier;
          if (value == "alt") return Qt::AltModifier;
          if (value == "meta" || value == "win") return Qt::MetaModifier;

          throw Exception ("no such modifier \"" + value + "\" (parsed from config file)");
          return Qt::NoModifier;
        }

      }

      std::string get_modifier (Qt::KeyboardModifiers key) {
        switch (key) {
          case Qt::ShiftModifier: return "Shift";
          case Qt::ControlModifier: return "Ctrl";
          case Qt::AltModifier: return "Alt";
          case Qt::MetaModifier: return "Win";
          default: assert (0);
        }
        return "Invalid";
      }


      // GLArea definitions:

      inline Window::GLArea::GLArea (Window& parent) :
        QGLWidget (QGLFormat (QGL::DoubleBuffer | QGL::DepthBuffer | QGL::StencilBuffer | QGL::Rgba), &parent),
        main (parent) {
          setCursor (Cursor::crosshair);
          setMouseTracking (true);
          setAcceptDrops (true);
          setFocusPolicy (Qt::StrongFocus);
          QFont font_ = font();
          font_.setPointSize (MR::File::Config::get_int ("FontSize", 10));
          setFont (font_);
        }

      QSize Window::GLArea::minimumSizeHint () const {
        return QSize (256, 256);
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
              list.push_back (new MR::Image::Header (urlList.at (i).path().toAscii().constData()));
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




      // Main Window class:

      Window::Window() :
        glarea (new GLArea (*this)),
        mode (NULL),
        font (glarea->font()),
        FocusModifier (get_modifier ("MRViewFocusModifierKey", Qt::MetaModifier)),
        MoveModifier (get_modifier ("MRViewMoveModifierKey", Qt::ShiftModifier)),
        RotateModifier (get_modifier ("MRViewRotateModifierKey", Qt::ControlModifier)),
        mouse_action (NoAction),
        orient (NAN, NAN, NAN, NAN),
        field_of_view (100.0),
        anatomical_plane (2),
        colourbar_position_index (2),
        snap_to_image_axes_and_voxel (true)
      {
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

        // Main toolbar:

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

        toolbar = new QToolBar ("Main toolbar", this);
        addToolBar (toolbar_position, toolbar);
        action = toolbar->toggleViewAction ();
        action->setShortcut (tr ("Ctrl+M"));
        addAction (action);

        // Start menu:

        menu = new QMenu (tr ("Start menu"), this);

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
        button->setToolTip (tr ("Start menu"));
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
        button->setToolTip (tr ("Image menu"));
        button->setIcon (QIcon (":/image.svg"));
        button->setPopupMode (QToolButton::InstantPopup);
        button->setMenu (image_menu);
        toolbar->addWidget (button);


        // Colourmap menu:

        colourmap_menu = new QMenu (tr ("Colourmap menu"), this);

        ColourMap::create_menu (this, colourmap_group, colourmap_menu, colourmap_actions, true);
        connect (colourmap_group, SIGNAL (triggered (QAction*)), this, SLOT (select_colourmap_slot()));

        colourmap_menu->addSeparator();

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

        button = new QToolButton (this);
        button->setToolTip (tr ("Colourmap menu"));
        button->setIcon (QIcon (":/colourmap.svg"));
        button->setPopupMode (QToolButton::InstantPopup);
        button->setMenu (colourmap_menu);
        toolbar->addWidget (button);




        // Mode menu:
        mode_group = new QActionGroup (this);
        mode_group->setExclusive (true);
        connect (mode_group, SIGNAL (triggered (QAction*)), this, SLOT (select_mode_slot (QAction*)));

        menu = new QMenu ("Display mode", this);
#define MODE(classname, specifier, name, description) \
        menu->addAction (new Action<classname> (mode_group, #name, #description, n++));
#define MODE_OPTION(classname, specifier, name, description) MODE(classname, specifier, name, description)
        {
          using namespace Mode;
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

        show_crosshairs_action = menu->addAction (tr ("Show focus"), glarea, SLOT (updateGL()));
        show_crosshairs_action->setShortcut (tr("F"));
        show_crosshairs_action->setCheckable (true);
        show_crosshairs_action->setChecked (true);
        addAction (show_crosshairs_action);

        show_comments_action = menu->addAction (tr ("Show comments"), glarea, SLOT (updateGL()));
        show_comments_action->setToolTip (tr ("Show/hide image comments\n\nShortcut: H"));
        show_comments_action->setShortcut (tr("H"));
        show_comments_action->setCheckable (true);
        show_comments_action->setChecked (true);
        addAction (show_comments_action);

        show_voxel_info_action = menu->addAction (tr ("Show voxel information"), glarea, SLOT (updateGL()));
        show_voxel_info_action->setShortcut (tr("V"));
        show_voxel_info_action->setCheckable (true);
        show_voxel_info_action->setChecked (true);
        addAction (show_voxel_info_action);

        show_orientation_labels_action = menu->addAction (tr ("Show orientation labels"), glarea, SLOT (updateGL()));
        show_orientation_labels_action->setShortcut (tr("O"));
        show_orientation_labels_action->setCheckable (true);
        show_orientation_labels_action->setChecked (true);
        addAction (show_orientation_labels_action);

        show_colourbar_action = menu->addAction (tr ("Show colour bar"), glarea, SLOT (updateGL()));
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

        extra_controls_action = toolbar->addAction (QIcon (":/controls.svg"),
            tr ("Additional controls"), this, SLOT (mode_control_slot()));
        extra_controls_action->setShortcut (tr("0"));
        extra_controls_action->setToolTip (tr ("Adjust additional viewing parameters"));
        extra_controls_action->setCheckable (true);
        extra_controls_action->setChecked (false);
        extra_controls_action->setEnabled (false);
        addAction (extra_controls_action);

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
        button->setToolTip (tr ("Help"));
        button->setIcon (QIcon (":/help.svg"));
        button->setPopupMode (QToolButton::InstantPopup);
        button->setMenu (menu);
        toolbar->addWidget (button);



        lighting_ = new GL::Lighting (this);
        connect (lighting_, SIGNAL (changed()), glarea, SLOT (updateGL()));

        set_image_menu ();

        std::string cbar_pos = lowercase (MR::File::Config::get ("MRViewColourBarPosition"));
        if (cbar_pos.size()) {
          if (cbar_pos == "bottomleft") colourbar_position_index = 1;
          else if (cbar_pos == "bottomright") colourbar_position_index = 2;
          else if (cbar_pos == "topleft") colourbar_position_index = 3;
          else if (cbar_pos == "topright") colourbar_position_index = 4;
          else 
            WARN ("invalid specifier \"" + cbar_pos + "\" for config file entry \"MRViewColourBarPosition\"");
        }
      }




      Window::~Window ()
      {
        mode = NULL;
        delete glarea;
        delete [] colourmap_actions;
      }







      void Window::image_open_slot ()
      {
        std::vector<std::string> image_list = Dialog::File::get_images (this, "Select images to open");
        if (image_list.empty())
          return;

        VecPtr<MR::Image::Header> list;
        for (size_t n = 0; n < image_list.size(); ++n) 
          list.push_back (new MR::Image::Header (image_list[n]));
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
          MR::Image::Buffer<cfloat>::voxel_type vox (dest);
          MR::Image::copy_with_progress (image()->voxel(), vox);
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
        glarea->updateGL();
      }


      void Window::select_mouse_mode_slot (QAction* action)
      {
        set_cursor();
      }



      void Window::select_tool_slot (QAction* action)
      {
        Tool::Dock* tool = dynamic_cast<Tool::__Action__*>(action)->instance;
        if (!tool) {
          tool = dynamic_cast<Tool::__Action__*>(action)->create (*this);
          connect (tool, SIGNAL (visibilityChanged (bool)), action, SLOT (setChecked (bool)));
        }
        if (action->isChecked())
          tool->show();
        else 
          tool->close();
        glarea->updateGL();
      }




      void Window::select_colourmap_slot ()
      {
        Image* imagep = image();
        if (imagep) {
          QAction* action = colourmap_group->checkedAction();
          size_t n = 0;
          while (action != colourmap_actions[n])
            ++n;
          imagep->set_colourmap (n);
          glarea->updateGL();
        }
      }



      void Window::invert_scaling_slot ()
      {
        if (image()) {
          image()->set_invert_scale (invert_scale_action->isChecked());
          glarea->updateGL();
        }
      }



      void Window::snap_to_image_slot ()
      {
        if (image()) {
          snap_to_image_axes_and_voxel = snap_to_image_action->isChecked();
          glarea->updateGL();
        }
      }




      void Window::on_scaling_changed () 
      {
        emit scalingChanged();
      }


      void Window::image_reset_slot ()
      {
        Image* imagep = image();
        if (imagep) {
          imagep->reset_windowing();
          on_scaling_changed();
          glarea->updateGL();
        }
      }



      void Window::image_interpolate_slot ()
      {
        Image* imagep = image();
        if (imagep) {
          imagep->set_interpolate (image_interpolate_action->isChecked());
          glarea->updateGL();
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
        glarea->updateGL();
      }


      void Window::mode_control_slot ()
      {
        assert (mode->features & Mode::ExtraControls);
        Tool::Dock* extra_controls = mode->get_extra_controls();
        if (extra_controls_action->isChecked())
          extra_controls->show();
        else 
          extra_controls->close();
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
        assert (image());
        ++image()->interp[3];
        set_image_navigation_menu();
        glarea->updateGL();
      }




      void Window::image_previous_volume_slot ()
      {
        assert (image());
        --image()->interp[3];
        set_image_navigation_menu();
        glarea->updateGL();
      }




      void Window::image_next_volume_group_slot () 
      {
        assert (image());
        ++image()->interp[4];
        set_image_navigation_menu();
        glarea->updateGL();
      }




      void Window::image_previous_volume_group_slot ()
      {
        assert (image());
        --image()->interp[4];
        set_image_navigation_menu();
        glarea->updateGL();
      }




      void Window::image_select_slot (QAction* action)
      {
        action->setChecked (true);
        image_interpolate_action->setChecked (image()->interpolate());
        size_t cmap_index = image()->colourmap();
        colourmap_group->actions()[cmap_index]->setChecked (true);
        invert_scale_action->setChecked (image()->scale_inverted());
        setWindowTitle (image()->interp.name().c_str());
        set_image_navigation_menu();
        image()->set_allowed_features (
            mode->features & Mode::ShaderThreshold,
            mode->features & Mode::ShaderTransparency,
            mode->features & Mode::ShaderLighting);
        emit imageChanged();
        glarea->updateGL();
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
        glarea->updateGL();
      }


      inline void Window::set_image_menu ()
      {
        int N = image_group->actions().size();
        next_image_action->setEnabled (N>1);
        prev_image_action->setEnabled (N>1);
        reset_windowing_action->setEnabled (N>0);
        colourmap_menu->setEnabled (N>0);
        save_action->setEnabled (N>0);
        close_action->setEnabled (N>0);
        properties_action->setEnabled (N>0);
        set_image_navigation_menu();
        glarea->updateGL();
      }

      inline int Window::get_mouse_mode ()
      {
        modifiers_ = QApplication::keyboardModifiers();

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


      inline void Window::set_cursor ()
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



      inline void Window::set_mode_features ()
      {
        mode_action_group->actions()[0]->setEnabled (mode->features & Mode::FocusContrast);
        mode_action_group->actions()[1]->setEnabled (mode->features & Mode::MoveTarget);
        mode_action_group->actions()[2]->setEnabled (mode->features & Mode::TiltRotate);
        extra_controls_action->setEnabled (mode->features & Mode::ExtraControls);
        if (! (mode->features & Mode::ExtraControls) ) 
          extra_controls_action->setChecked (false);
        if (!mode_action_group->checkedAction()->isEnabled())
          mode_action_group->actions()[0]->setChecked (true);
        if (image()) 
          image()->set_allowed_features (
              mode->features & Mode::ShaderThreshold,
              mode->features & Mode::ShaderTransparency,
              mode->features & Mode::ShaderLighting);
      }


      inline void Window::set_image_navigation_menu ()
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
        Dialog::OpenGL gl (this);
        gl.exec();
      }




      void Window::about_slot ()
      {
        std::string message = printf ("<h1>MRView</h1>The MRtrix viewer, version %zu.%zu.%zu<br>"
                                      "<em>%d bit %s version, built " __DATE__ "</em><p>"
                                      "<h4>Authors:</h4>%s<p><em>%s</em>",
                                      App::VERSION[0], App::VERSION[1], App::VERSION[2], int (8*sizeof (size_t)),
#ifdef NDEBUG
                                      "release"
#else
                                      "debug"
#endif
                                      , MR::join (MR::split (App::AUTHOR, ",;&\n", true), "<br>").c_str(), App::COPYRIGHT);

        QMessageBox::about (this, tr ("About MRView"), message.c_str());
      }




      void Window::aboutQt_slot ()
      {
        QMessageBox::aboutQt (this);
      }




      inline void Window::paintGL ()
      {
        glEnable (GL_MULTISAMPLE);
        if (mode->in_paint())
          return;

        glDrawBuffer (GL_BACK);
        mode->paintGL();

        DEBUG_OPENGL;
      }



      inline void Window::initGL ()
      {
        GL::init ();

        font.initGL();

        glClearColor (0.0, 0.0, 0.0, 0.0);
        glEnable (GL_DEPTH_TEST);

        Options opt = get_options ("mode");
        if (opt.size()) {
#undef MODE
#define MODE(classname, specifier, name, description) \
   #specifier,
#define MODE_OPTION(classname, specifier, name, description) MODE(classname, specifier, name, description)
          const char* specifier_list[] = { 
#include "gui/mrview/mode/list.h"
            NULL
          };
#undef MODE
#undef MODE_OPTION
          std::string specifier (lowercase (opt[0][0]));
          for (int n = 0; specifier_list[n]; ++n) {
            if (specifier == lowercase (specifier_list[n])) {
              mode = dynamic_cast<Mode::__Action__*> (mode_group->actions()[n])->create (*this);
              set_mode_features();
              goto mode_selected;
            }
          }
          throw Exception ("unknown mode \"" + std::string (opt[0][0]) + "\"");
        }
        else {
          mode = dynamic_cast<Mode::__Action__*> (mode_group->actions()[0])->create (*this);
          set_mode_features();
        }

mode_selected:
        DEBUG_OPENGL;
      }


      template <class Event> inline void Window::grab_mouse_state (Event* event)
      {
        buttons_ = event->buttons();
        modifiers_ = event->modifiers();
        mouse_displacement_ = QPoint (0,0);
        mouse_position_ = event->pos();
        mouse_position_.setY (glarea->height() - mouse_position_.y());
      }

      template <class Event> inline void Window::update_mouse_state (Event* event)
      {
        mouse_displacement_ = mouse_position_;
        mouse_position_ = event->pos();
        mouse_position_.setY (glarea->height() - mouse_position_.y());
        mouse_displacement_ = mouse_position_ - mouse_displacement_;
      }


      void Window::keyPressEvent (QKeyEvent* event) 
      {
        set_cursor();
      }

      void Window::keyReleaseEvent (QKeyEvent* event)
      {
        set_cursor();
      }

      inline void Window::mousePressEventGL (QMouseEvent* event)
      {
        assert (mode);

        grab_mouse_state (event);
        if (image()) 
          mode->mouse_press_event();

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



      inline void Window::mouseMoveEventGL (QMouseEvent* event)
      {
        assert (mode);
        if (mouse_action == NoAction)
          return;
        if (!image()) 
          return;

        update_mouse_state (event);
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

      inline void Window::mouseReleaseEventGL (QMouseEvent* event)
      {
        assert (mode);
        mode->mouse_release_event();
        mouse_action = NoAction;
        set_cursor();
      }

      inline void Window::wheelEventGL (QWheelEvent* event)
      {
        assert (mode);

        if (event->orientation() == Qt::Vertical) {

          if (image()) {
          grab_mouse_state (event);
          mode->mouse_press_event();

            if (buttons_ == Qt::NoButton) {

              if (modifiers_ == Qt::ControlModifier) {
                set_FOV (FOV() * Math::exp (-event->delta()/1200.0));
                glarea->updateGL();
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


  }
}
}



