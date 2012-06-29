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
#include "gui/mrview/window.h"
#include "gui/mrview/shader.h"
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

#define MODE(classname, specifier, name, description) #specifier ", "
#define MODE_OPTION(classname, specifier, name, description) MODE(classname, specifier, name, description)

      const OptionGroup Window::options = OptionGroup ("General options")
        + Option ("mode", "select initial display mode by its short ID. Valid mode IDs are: "
#include "gui/mrview/mode/list.h"
            )
        + Argument ("name");
#undef MODE
#undef MODE_OPTION




      // GLArea definitions:

      inline Window::GLArea::GLArea (Window& parent) :
        QGLWidget (QGLFormat (QGL::DoubleBuffer | QGL::DepthBuffer | QGL::StencilBuffer | QGL::Rgba), &parent),
        main (parent) {
          setCursor (Cursor::crosshair);
          setMouseTracking (true);
          setAcceptDrops (true);
          setFocusPolicy (Qt::StrongFocus);
        }

      QSize Window::GLArea::minimumSizeHint () const {
        return QSize (256, 256);
      }
      QSize Window::GLArea::sizeHint () const {
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
        FocusModifier (Qt::MetaModifier),
        MoveModifier (Qt::ControlModifier),
        RotateModifier (Qt::ShiftModifier),
        orient (NAN, NAN, NAN, NAN),
        field_of_view (100.0),
        proj (2)
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
          if (toolbar_pos_spec == "bottom") toolbar_position = Qt::BottomToolBarArea;
          else if (toolbar_pos_spec == "left") toolbar_position = Qt::LeftToolBarArea;
          else if (toolbar_pos_spec == "right") toolbar_position = Qt::RightToolBarArea;
          else if (toolbar_pos_spec != "top")
            error ("invalid value for configuration entry \"InitialToolBarPosition\"");
        }

        toolbar = new QToolBar ("Main toolbar", this);
        addToolBar (toolbar_position, toolbar);
        action = toolbar->toggleViewAction ();
        action->setShortcut (tr ("Ctrl+M"));
        addAction (action);

        // Image menu:

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

        properties_action = menu->addAction (tr ("Properties..."), this, SLOT (image_properties_slot()));
        properties_action->setToolTip (tr ("Display the properties of the current image\n\nShortcut: Ctrl+P"));
        addAction (properties_action);

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
        next_image_action->setShortcut (tr ("Tab"));
        addAction (next_image_action);

        prev_image_action = image_menu->addAction (tr ("Previous image"), this, SLOT (image_previous_slot()));
        prev_image_action->setShortcut (tr ("Shift+Tab"));
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

        ColourMap::init (this, colourmap_group, colourmap_menu, colourmap_actions);
        connect (colourmap_group, SIGNAL (triggered (QAction*)), this, SLOT (select_colourmap_slot()));

        colourmap_menu->addSeparator();

        invert_scale_action = colourmap_menu->addAction (tr ("Invert scaling"), this, SLOT (invert_scaling_slot()));
        invert_scale_action->setCheckable (true);
        invert_scale_action->setShortcut (tr("U"));
        addAction (invert_scale_action);


        invert_colourmap_action = colourmap_menu->addAction (tr ("Invert Colourmap"), this, SLOT (invert_colourmap_slot()));
        invert_colourmap_action->setCheckable (true);
        invert_colourmap_action->setShortcut (tr("Shift+U"));
        addAction (invert_colourmap_action);

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

        projection_group = new QActionGroup (this);
        projection_group->setExclusive (true);
        connect (projection_group, SIGNAL (triggered (QAction*)), this, SLOT (select_projection_slot (QAction*)));

        axial_action = menu->addAction (tr ("Axial"));
        axial_action->setShortcut (tr ("A"));
        axial_action->setCheckable (true);
        projection_group->addAction (axial_action);
        addAction (axial_action);

        sagittal_action = menu->addAction (tr ("Sagittal"));
        sagittal_action->setShortcut (tr ("S"));
        sagittal_action->setCheckable (true);
        projection_group->addAction (sagittal_action);
        addAction (sagittal_action);

        coronal_action = menu->addAction (tr ("Coronal"));
        coronal_action->setShortcut (tr ("C"));
        coronal_action->setCheckable (true);
        projection_group->addAction (coronal_action);
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
        menu->addAction (new Action<classname> (tool_group, #name, #description, n++));
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

        action = toolbar->addAction (QIcon (":/select_contrast.svg"), tr ("Change focus / contrast"));
        action->setToolTip (tr (
              "Left-click: set focus\n"
              "Right-click: change brightness/constrast\n\n"
              "Shortcut: 1\n\n"
              "Hold down Win key to use this mode\n"
              "regardless of currently selected mode"));
        action->setShortcut (tr("1"));
        action->setCheckable (true);
        action->setChecked (true);
        mode_action_group->addAction (action);

        action = toolbar->addAction (QIcon (":/move.svg"), tr ("Move viewport"));
        action->setToolTip (tr (
              "Left-click: move in-plane\n"
              "Right-click: move through-plane\n\n"
              "Shortcut: 2\n\n"
              "Hold down Ctrl key to use this mode\n"
              "regardless of currently selected mode"));
        action->setShortcut (tr("2"));
        action->setCheckable (true);
        mode_action_group->addAction (action);

        action = toolbar->addAction (QIcon (":/rotate.svg"), tr ("Move camera"));
        action->setToolTip (tr (
              "Left-click: move camera in-plane\n"
              "Right-click: rotate camera about view axis\n\n"
              "Shortcut: 2\n\n"
              "Hold down Shift key to use this mode\n"
              "regardless of currently selected mode"));
        action->setShortcut (tr("3"));
        action->setCheckable (true);
        mode_action_group->addAction (action);

        for (int n = 0; n < mode_action_group->actions().size(); ++n)
          addAction (mode_action_group->actions()[n]);

        extra_controls_action = toolbar->addAction (QIcon (":/controls.svg"),
            tr ("Additional controls"), this, SLOT (mode_control_slot()));
        extra_controls_action->setShortcut (tr("0"));
        extra_controls_action->setToolTip (tr ("Adjust additional viewing parameters"));
        extra_controls_action->setEnabled (false);
        addAction (extra_controls_action);

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
      }




      Window::~Window ()
      {
        mode = NULL;
        delete glarea;
        delete [] colourmap_actions;
      }







      void Window::image_open_slot ()
      {
        Dialog::File dialog (this, "Select images to open", true, true);
        if (dialog.exec()) {
          VecPtr<MR::Image::Header> list;
          dialog.get_images (list);
          add_images (list);
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
        Dialog::File dialog (this, "Select image destination", false, false);
        if (dialog.exec()) {
          std::vector<std::string> selection;
          dialog.get_selection (selection);
          if (selection.size() != 1) return;
          try {
            MR::Image::Buffer<cfloat> dest (selection[0], image()->header());
            MR::Image::Buffer<cfloat>::voxel_type vox (dest);
            MR::Image::copy_with_progress (image()->voxel(), vox);
          }
          catch (Exception& E) {
            E.display();
          }
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
        set_mode_actions();
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
          addDockWidget (Qt::RightDockWidgetArea, tool);
          tool->show();
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
          imagep->set_colourmap (ColourMap::from_menu (n));
          glarea->updateGL();
        }
      }

      void Window::invert_colourmap_slot () 
      {
        if (image()) {
          image()->set_invert_map (invert_colourmap_action->isChecked());
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



      void Window::image_reset_slot ()
      {
        Image* imagep = image();
        if (imagep) {
          imagep->reset_windowing();
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

      void Window::select_projection_slot (QAction* action)
      {
        if (action == axial_action) set_projection (2);
        else if (action == sagittal_action) set_projection (0);
        else if (action == coronal_action) set_projection (1);
        else assert (0);
        glarea->updateGL();
      }


      void Window::mode_control_slot ()
      {
        TEST;
      }

      void Window::reset_view_slot ()
      {
        if (image())
          mode->reset_event();
      }


      void Window::slice_next_slot () 
      {
        mode->slice_move_event (1);
        updateGL();
      }

      void Window::slice_previous_slot () 
      {
        mode->slice_move_event (-1);
        updateGL();
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
        colourmap_group->actions()[image()->colourmap_index()]->setChecked (true);
        invert_scale_action->setChecked (image()->scale_inverted());
        invert_colourmap_action->setChecked (image()->colourmap_inverted());
        setWindowTitle (image()->interp.name().c_str());
        set_image_navigation_menu();
        glarea->updateGL();
      }


      void Window::toggle_annotations_slot ()
      {
        int current_annotations = 0x00000000;
        if (show_crosshairs()) current_annotations |= 0x00000001;
        if (show_comments()) current_annotations |= 0x00000002;
        if (show_voxel_info()) current_annotations |= 0x00000004;
        if (show_orientation_labels()) current_annotations |= 0x00000008;

        if (current_annotations) {
          annotations = current_annotations;
          show_crosshairs_action->setChecked (false);
          show_comments_action->setChecked (false);
          show_voxel_info_action->setChecked (false);
          show_orientation_labels_action->setChecked (false);
        }
        else {
          if (!annotations) 
            annotations = 0xFFFFFFFF;
          show_crosshairs_action->setChecked (annotations & 0x00000001);
          show_comments_action->setChecked (annotations & 0x00000002);
          show_voxel_info_action->setChecked (annotations & 0x00000004);
          show_orientation_labels_action->setChecked (annotations & 0x00000008);
        }
        updateGL();
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

      void Window::set_cursor (int action_mode, bool right_action)
      {
        if (action_mode == 1) {
          if (right_action) glarea->setCursor (Cursor::window);
          else glarea->setCursor (Cursor::crosshair);
        }
        else if (action_mode == 2) {
          if (right_action) glarea->setCursor (Cursor::forward_backward);
          else glarea->setCursor (Cursor::pan_crosshair);
        }
        else if (action_mode == 3) {
          if (right_action) glarea->setCursor (Cursor::inplane_rotate);
          else glarea->setCursor (Cursor::throughplane_rotate);
        }
      }


      inline void Window::set_cursor ()
      {
        if (mode_action_group->checkedAction() == mode_action_group->actions()[0])
          set_cursor (1);
        else if (mode_action_group->checkedAction() == mode_action_group->actions()[1])
          set_cursor (2);
        else if (mode_action_group->checkedAction() == mode_action_group->actions()[2])
          set_cursor (3);
      }



      inline void Window::set_mode_actions ()
      {
        mode_action_group->actions()[0]->setEnabled (mode->mouse_actions & Mode::FocusContrast);
        mode_action_group->actions()[1]->setEnabled (mode->mouse_actions & Mode::MoveTarget);
        mode_action_group->actions()[2]->setEnabled (mode->mouse_actions & Mode::TiltRotate);
        extra_controls_action->setEnabled (mode->mouse_actions & Mode::ExtraControls);
        if (!mode_action_group->checkedAction()->isEnabled())
          mode_action_group->actions()[0]->setChecked (true);
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
                                      "Author: %s<p><em>%s</em>",
                                      App::VERSION[0], App::VERSION[1], App::VERSION[2], int (8*sizeof (size_t)),
#ifdef NDEBUG
                                      "release"
#else
                                      "debug"
#endif
                                      , App::AUTHOR, App::COPYRIGHT);

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

        CHECK_GL_EXTENSION (ARB_fragment_shader);
        CHECK_GL_EXTENSION (ARB_vertex_shader);
        CHECK_GL_EXTENSION (ARB_geometry_shader4);
        CHECK_GL_EXTENSION (EXT_texture3D);
        CHECK_GL_EXTENSION (ARB_texture_non_power_of_two);
        CHECK_GL_EXTENSION (ARB_vertex_buffer_object);
        CHECK_GL_EXTENSION (ARB_pixel_buffer_object);
        CHECK_GL_EXTENSION (ARB_framebuffer_object);
        CHECK_GL_EXTENSION (ARB_multisample);

        GLint max_num;
        glGetIntegerv (GL_MAX_GEOMETRY_OUTPUT_VERTICES_ARB, &max_num);
        inform ("maximum number of vertices for geometry shader: " + str (max_num));

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
              set_mode_actions();
              goto mode_selected;
            }
          }
          throw Exception ("unknown mode \"" + std::string (opt[0][0]) + "\"");
        }
        else {
          mode = dynamic_cast<Mode::__Action__*> (mode_group->actions()[0])->create (*this);
          set_mode_actions();
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


      inline int Window::get_modifier (Qt::KeyboardModifiers keys)
      {
        if (keys == Qt::NoModifier)
          return 0;
        if (keys == FocusModifier && ( mode->mouse_actions & Mode::FocusContrast )) 
          return 1;
        if (keys == MoveModifier && ( mode->mouse_actions & Mode::MoveTarget ))
          return 2;
        if (keys == RotateModifier && ( mode->mouse_actions & Mode::TiltRotate ))
          return 3;
        return 0;
      }
/*
      inline void Window::KeyPressEventGL (QKeyEvent* event) 
      {
        if (mouse_action != NoAction) return;
        int group = get_modifier (event->modifiers());
        VAR (group);
        if (group == 0) set_cursor();
        else set_cursor (group);
      }

      inline void Window::KeyReleaseEventGL (QKeyEvent* event)
      {
        if (mouse_action != NoAction) return;
        int group = get_modifier (event->modifiers());
        VAR (group);
        if (group == 0) set_cursor();
        else set_cursor (group);
      }
*/
      inline void Window::mousePressEventGL (QMouseEvent* event)
      {
        assert (mode);
        if (!image()) 
          return;

        grab_mouse_state (event);
        mode->mouse_press_event();

        int group = get_modifier();
        if (group == 0) {
          while (mode_action_group->checkedAction() != mode_action_group->actions()[group])
            ++group;
          ++group;
        }

        if (buttons_ == Qt::MidButton) {
          mouse_action = Pan;
          set_cursor (2);
        }
        else if (group == 1) {
          if (buttons_ == Qt::LeftButton) {
            mouse_action = SetFocus;
            set_cursor (1);
            mode->set_focus_event();
          }
          else if (buttons_ == Qt::RightButton) {
            mouse_action = Contrast;
            set_cursor (1, true);
          }
          else return;
        }
        else if (group == 2) {
          if (buttons_ == Qt::LeftButton) {
            mouse_action = Pan;
            set_cursor (2);
          }
          else if (buttons_ == Qt::RightButton) {
            mouse_action = PanThrough;
            set_cursor (2, true);
          }
          else return;
        }
        else if (group == 3) {
          if (buttons_ == Qt::LeftButton) {
            mouse_action = Tilt;
            set_cursor (3);
          }
          else if (buttons_ == Qt::RightButton) {
            mouse_action = Rotate;
            set_cursor (3, true);
          }
          else return;
        }
        else return;

        event->accept();
      }



      inline void Window::mouseMoveEventGL (QMouseEvent* event)
      {
        assert (mode);
        if (!image()) 
          return;
        if (mouse_action == NoAction)
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
        set_cursor();
        mouse_action = NoAction;
      }

      inline void Window::wheelEventGL (QWheelEvent* event)
      {
        assert (mode);
        if (image()) {
          grab_mouse_state (event);
          mode->mouse_press_event();

          if (event->orientation() == Qt::Vertical) {
            if (buttons_ == Qt::NoButton) {

              if (modifiers_ == Qt::ControlModifier) {
                set_FOV (FOV() * Math::exp (-event->delta()/1200.0));
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
              updateGL();
              return;
            }

            else if (buttons_ == Qt::RightButton && modifiers_ == Qt::NoModifier) {
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

          }
        }

      }





    }
  }
}



