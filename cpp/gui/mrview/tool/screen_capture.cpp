/* Copyright (c) 2008-2026 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Covered Software is provided under this License on an "as is"
 * basis, without warranty of any kind, either expressed, implied, or
 * statutory, including, without limitation, warranties that the
 * Covered Software is free of defects, merchantable, fit for a
 * particular purpose or non-infringing.
 * See the Mozilla Public License v. 2.0 for more details.
 *
 * For more details, see http://www.mrtrix.org/.
 */

#include "eigen_plugins/eigen_plugins.h"
#include <Eigen/Geometry>
#include <QProgressDialog>
#include <filesystem>
#include <memory>

#include "dialog/file.h"
#include "file/config.h"
#include "file/path.h"
#include "mrtrix.h"
#include "mrview/mode/base.h"
#include "mrview/tool/screen_capture.h"
#include "mrview/window.h"
#include "opengl/transformation.h"

namespace MR::GUI::MRView::Tool {

Capture::Capture(Dock *parent)
    : Base(parent), rotation_type(RotationType::World), translation_type(TranslationType::Voxel), is_playing(false) {
  VBoxLayout *main_box = new VBoxLayout(this);

  QGroupBox *rotate_group_box = new QGroupBox(tr("Rotate"));
  GridLayout *rotate_layout = new GridLayout;
  rotate_layout->setContentsMargins(5, 5, 5, 5);
  rotate_layout->setSpacing(5);
  main_box->addWidget(rotate_group_box);
  rotate_group_box->setLayout(rotate_layout);

  rotate_layout->addWidget(new QLabel(tr("Type: ")), 0, 0);
  rotation_type_combobox = new QComboBox;
  rotation_type_combobox->insertItem(0, tr("World"), RotationType::World);
  rotation_type_combobox->insertItem(1, tr("Camera"), RotationType::Eye);
  rotation_type_combobox->insertItem(2, tr("Image"), RotationType::Image);
  connect(rotation_type_combobox, SIGNAL(activated(int)), this, SLOT(on_rotation_type(int)));
  rotate_layout->addWidget(rotation_type_combobox, 0, 1, 1, 4);

  rotate_layout->addWidget(new QLabel(tr("Axis: ")), 1, 0);
  rotation_axis_x = new AdjustButton(this);
  rotate_layout->addWidget(rotation_axis_x, 1, 1);
  rotation_axis_x->setValue(0.0);
  rotation_axis_x->setRate(0.1);

  rotation_axis_y = new AdjustButton(this);
  rotate_layout->addWidget(rotation_axis_y, 1, 2);
  rotation_axis_y->setValue(0.0);
  rotation_axis_y->setRate(0.1);

  rotation_axis_z = new AdjustButton(this);
  rotate_layout->addWidget(rotation_axis_z, 1, 3);
  rotation_axis_z->setValue(1.0);
  rotation_axis_z->setRate(0.1);

  rotate_layout->addWidget(new QLabel(tr("Angle: ")), 2, 0);
  degrees_button = new AdjustButton(this);
  rotate_layout->addWidget(degrees_button, 2, 1, 1, 3);
  degrees_button->setValue(0.0);
  degrees_button->setRate(0.1);

  QGroupBox *translate_group_box = new QGroupBox(tr("Translate"));
  GridLayout *translate_layout = new GridLayout;
  translate_layout->setContentsMargins(5, 5, 5, 5);
  translate_layout->setSpacing(5);
  main_box->addWidget(translate_group_box);
  translate_group_box->setLayout(translate_layout);

  translate_layout->addWidget(new QLabel(tr("Type: ")), 0, 0);
  translation_type_combobox = new QComboBox;
  translation_type_combobox->insertItem(0, tr("Voxel"), TranslationType::Voxel);
  translation_type_combobox->insertItem(1, tr("Scanner (mm)"), TranslationType::Scanner);
  translation_type_combobox->insertItem(2, tr("Camera (mm)"), TranslationType::Camera);
  connect(translation_type_combobox, SIGNAL(activated(int)), this, SLOT(on_translation_type(int)));
  translate_layout->addWidget(translation_type_combobox, 0, 1, 1, 4);

  translate_layout->addWidget(new QLabel(tr("Axis: ")), 1, 0);
  translate_x = new AdjustButton(this);
  translate_layout->addWidget(translate_x, 1, 1);
  translate_x->setValue(0.0);
  translate_x->setRate(0.1);

  translate_y = new AdjustButton(this);
  translate_layout->addWidget(translate_y, 1, 2);
  translate_y->setValue(0.0);
  translate_y->setRate(0.1);

  translate_z = new AdjustButton(this);
  translate_layout->addWidget(translate_z, 1, 3);
  translate_z->setValue(0.0);
  translate_z->setRate(0.1);

  QGroupBox *volume_group_box = new QGroupBox(tr("Volume"));
  GridLayout *volume_layout = new GridLayout;
  volume_layout->setContentsMargins(5, 5, 5, 5);
  volume_layout->setSpacing(5);
  main_box->addWidget(volume_group_box);
  volume_group_box->setLayout(volume_layout);

  volume_layout->addWidget(new QLabel(tr("Axis: ")), 0, 0);
  volume_axis = new SpinBox(this);
  volume_axis->setMinimum(3);
  volume_axis->setValue(3);
  volume_layout->addWidget(volume_axis, 0, 1);

  volume_layout->addWidget(new QLabel(tr("Target: ")), 0, 2);
  target_volume = new SpinBox(this);
  volume_layout->addWidget(target_volume, 0, 3);
  target_volume->setMinimum(0);
  target_volume->setMaximum(std::numeric_limits<int>::max());
  target_volume->setValue(0);

  QGroupBox *FOV_group_box = new QGroupBox(tr("FOV"));
  GridLayout *FOV_layout = new GridLayout;
  FOV_layout->setContentsMargins(5, 5, 5, 5);
  FOV_layout->setSpacing(5);
  main_box->addWidget(FOV_group_box);
  FOV_group_box->setLayout(FOV_layout);

  FOV_layout->addWidget(new QLabel(tr("Multiplier: ")), 0, 0);
  FOV_multipler = new AdjustButton(this);
  FOV_layout->addWidget(FOV_multipler, 0, 1);
  FOV_multipler->setValue(1.0);
  FOV_multipler->setRate(0.01);

  QGroupBox *output_group_box = new QGroupBox(tr("Output"));
  main_box->addWidget(output_group_box);
  GridLayout *output_grid_layout = new GridLayout;
  output_group_box->setLayout(output_grid_layout);

  output_grid_layout->addWidget(new QLabel(tr("Prefix: ")), 0, 0);
  prefix_textbox = new QLineEdit("screenshot", this);
  output_grid_layout->addWidget(prefix_textbox, 0, 1);
  connect(prefix_textbox, SIGNAL(textChanged(const QString &)), this, SLOT(on_output_update()));

  folder_button = new QPushButton(tr("Select output folder"), this);
  folder_button->setToolTip(tr("Output folder"));
  connect(folder_button, SIGNAL(clicked()), this, SLOT(select_output_folder_slot()));
  output_grid_layout->addWidget(folder_button, 1, 0, 1, 2);

  output_grid_layout->addWidget(new QLabel(tr("Super-sampling: ")), 2, 0);
  supersample = new SpinBox(this);
  supersample->setMinimum(1);
  supersample->setMaximum(8);
  supersample->setToolTip(tr("Render exported images off-screen at this integer multiple of the window resolution"));
  // CONF option: MRViewScreenshotSuperSample
  // CONF default: 1
  // CONF The default super-sampling (super-resolution) factor for the MRView screenshot tool.
  // CONF A value greater than one renders each exported image off-screen at this integer multiple
  // CONF of the window resolution, improving image quality at the cost of additional computation
  // CONF and graphics memory; the interactive window continues to render at native resolution.
  supersample->setValue(File::Config::get_int("MRViewScreenshotSuperSample", 1));
  output_grid_layout->addWidget(supersample, 2, 1);

  output_grid_layout->addWidget(new QLabel(tr("Anti-aliasing: ")), 3, 0);
  msaa = new QComboBox(this);
  // Multi-sample counts are restricted to powers of two by graphics hardware, hence a combo-box
  // rather than a spin-box; the value is additionally clamped to the GL maximum at capture time.
  msaa->insertItem(0, tr("None"), 1);
  msaa->insertItem(1, tr("2x"), 2);
  msaa->insertItem(2, tr("4x"), 4);
  msaa->insertItem(3, tr("8x"), 8);
  msaa->insertItem(4, tr("16x"), 16);
  msaa->setToolTip(tr("Off-screen multi-sample anti-aliasing applied to exported images"));
  // CONF option: MRViewScreenshotMSAA
  // CONF default: 1
  // CONF The default multi-sample anti-aliasing factor for the MRView screenshot tool.
  // CONF A value greater than one renders each exported image off-screen with this number of
  // CONF samples per pixel, smoothing edges; the value is rounded to a supported power of two and
  // CONF the interactive window continues to render at its configured quality (see MSAA).
  set_msaa_value(File::Config::get_int("MRViewScreenshotMSAA", 1));
  output_grid_layout->addWidget(msaa, 3, 1);

  output_grid_layout->addWidget(new QLabel(tr("Down-sampling: ")), 4, 0);
  downsample = new SpinBox(this);
  downsample->setMinimum(1);
  downsample->setMaximum(8);
  downsample->setToolTip(tr(
      "Reduce exported images by this integer factor (export resolution = native x super-sampling / down-sampling)"));
  // CONF option: MRViewScreenshotDownSample
  // CONF default: 1
  // CONF The default down-sampling factor for the MRView screenshot tool.
  // CONF The exported image resolution is the native window resolution multiplied by the
  // CONF super-sampling factor and divided by this down-sampling factor; combining super-sampling
  // CONF with an equal down-sampling factor yields anti-aliased images at the native resolution.
  downsample->setValue(File::Config::get_int("MRViewScreenshotDownSample", 1));
  output_grid_layout->addWidget(downsample, 4, 1);

  QGroupBox *capture_group_box = new QGroupBox(tr("Capture"));
  main_box->addWidget(capture_group_box);
  GridLayout *capture_grid_layout = new GridLayout;
  capture_group_box->setLayout(capture_grid_layout);

  capture_grid_layout->addWidget(new QLabel(tr("Start Index: ")), 0, 0);
  start_index = new SpinBox(this);
  start_index->setMinimum(0);
  start_index->setMaximum(std::numeric_limits<int>::max());
  start_index->setMinimumWidth(50);
  start_index->setValue(0);
  capture_grid_layout->addWidget(start_index, 0, 1);

  capture_grid_layout->addWidget(new QLabel(tr("Frames: ")), 0, 2);
  frames = new SpinBox(this);
  frames->setMinimumWidth(50);
  frames->setMinimum(1);
  frames->setMaximum(std::numeric_limits<int>::max());
  frames->setValue(1);
  capture_grid_layout->addWidget(frames, 0, 3);

  QPushButton *preview = new QPushButton(this);
  preview->setToolTip(tr("Play preview"));
  preview->setIcon(QIcon(":/start.svg"));
  connect(preview, SIGNAL(clicked()), this, SLOT(on_screen_preview()));
  capture_grid_layout->addWidget(preview, 2, 0);

  QPushButton *stop = new QPushButton(this);
  stop->setToolTip(tr("Stop preview"));
  stop->setIcon(QIcon(":/stop.svg"));
  connect(stop, SIGNAL(clicked()), this, SLOT(on_screen_stop()));
  capture_grid_layout->addWidget(stop, 2, 1);

  QPushButton *restore = new QPushButton(this);
  restore->setToolTip(tr("Restore"));
  restore->setIcon(QIcon(":/restore.svg"));
  connect(restore, SIGNAL(clicked()), this, SLOT(on_restore_capture_state()));
  capture_grid_layout->addWidget(restore, 2, 2);

  QPushButton *capture = new QPushButton(this);
  capture->setToolTip(tr("Record"));
  capture->setIcon(QIcon(":/record.svg"));
  connect(capture, SIGNAL(clicked()), this, SLOT(on_screen_capture()));
  capture_grid_layout->addWidget(capture, 2, 3);

  main_box->addStretch();

  current_folder = ".";

  connect(&window(), SIGNAL(imageChanged()), this, SLOT(on_image_changed()));
  on_image_changed();
}

void Capture::on_image_changed() {
  cached_state.clear();
  const auto image = window().image();
  if (!image)
    return;

  const int max_axis = std::max(static_cast<int>(image->header().ndim() - 1), 0);
  volume_axis->setMaximum(max_axis);
  volume_axis->setValue(std::min(volume_axis->value(), max_axis));
}

void Capture::on_rotation_type(int index) {
  rotation_type = static_cast<RotationType>(rotation_type_combobox->itemData(index).toInt());
}

void Capture::on_translation_type(int index) {
  translation_type = static_cast<TranslationType>(translation_type_combobox->itemData(index).toInt());
}

void Capture::on_screen_preview() {
  if (!is_playing)
    run(false);
}

void Capture::on_screen_capture() {
  if (!is_playing)
    run(true);
}

void Capture::on_screen_stop() { is_playing = false; }

void Capture::cache_capture_state() {
  if (!window().image())
    return;
  auto &image(window().image()->image);

  cached_state.emplace(cached_state.end(),
                       window().orientation(),
                       window().focus(),
                       window().target(),
                       window().FOV(),
                       volume_axis->value() < static_cast<ssize_t>(image.ndim()) ? image.index(volume_axis->value())
                                                                                 : 0,
                       volume_axis->value(),
                       start_index->value(),
                       window().plane());

  if (cached_state.size() > max_cache_size)
    cached_state.pop_front();
}

void Capture::on_restore_capture_state() {
  if (!window().image() || cached_state.empty())
    return;

  const CaptureState &state = cached_state.back();

  window().set_plane(state.plane);
  window().set_orientation(state.orientation);
  window().set_focus(state.focus);
  window().set_target(state.target);
  window().set_FOV(state.fov);
  window().set_image_volume(state.volume_axis, state.volume);
  start_index->setValue(state.frame_index);

  cached_state.pop_back();
}

void Capture::run(bool with_capture) {
  Window &win(window());
  MRView::Image *img(win.image());

  if (!img)
    return;

  is_playing = true;

  cache_capture_state();

  auto &image(img->image);

  if (std::isnan(rotation_axis_x->value()))
    rotation_axis_x->setValue(0.0);
  if (std::isnan(rotation_axis_y->value()))
    rotation_axis_y->setValue(0.0);
  if (std::isnan(rotation_axis_z->value()))
    rotation_axis_z->setValue(0.0);
  if (std::isnan(degrees_button->value()))
    degrees_button->setValue(0.0);

  if (std::isnan(translate_x->value()))
    translate_x->setValue(0.0);
  if (std::isnan(translate_y->value()))
    translate_y->setValue(0.0);
  if (std::isnan(translate_z->value()))
    translate_z->setValue(0.0);

  if (std::isnan(target_volume->value()))
    target_volume->setValue(0.0);

  if (std::isnan(FOV_multipler->value()))
    FOV_multipler->setValue(1.0);

  if (window().snap_to_image() && degrees_button->value() > 0.0)
    window().set_snap_to_image(false);

  size_t frames_value = frames->value();

  std::string prefix(prefix_textbox->text().toUtf8().constData());
  float radians = degrees_button->value() * (Math::pi / 180.0) / frames_value;
  size_t first_index = start_index->value();

  float volume = 0.0F, volume_inc = 0.0F;
  if (volume_axis->value() < static_cast<ssize_t>(image.ndim())) {
    if (target_volume->value() >= image.size(volume_axis->value()))
      target_volume->setValue(image.size(volume_axis->value()) - 1);
    volume = static_cast<float>(image.index(volume_axis->value()));
    volume_inc = static_cast<float>(target_volume->value()) / static_cast<float>(frames_value);
  }

  if (with_capture && !std::filesystem::exists(current_folder))
    std::filesystem::create_directories(current_folder);

  const int ratio = supersample->value();
  const int msaa_value = msaa->currentData().toInt();
  const int downsample_value = downsample->value();

  // For multi-frame captures, present a progress dialog. It is advanced only between frames, while the
  // off-screen framebuffer is unbound, so its event processing does not interfere with the rendering of
  // any individual image (granular progress within a single large render would require tiled rendering).
  std::unique_ptr<QProgressDialog> progress;
  if (with_capture && frames_value > 1) {
    progress = std::make_unique<QProgressDialog>(
        tr("Capturing screenshots..."), tr("Cancel"), 0, static_cast<int>(frames_value), this);
    progress->setWindowModality(Qt::WindowModal);
    progress->setMinimumDuration(0);
  }

  for (size_t i = first_index; i < first_index + frames_value; ++i) {
    if (!is_playing)
      break;

    if (progress) {
      if (progress->wasCanceled()) {
        is_playing = false;
        break;
      }
      progress->setValue(static_cast<int>(i - first_index));
    }

    if (with_capture)
      win.captureGL(current_folder / (prefix + printf("%04d.png", i)), ratio, msaa_value, downsample_value);

    // Rotation
    Eigen::Quaternionf orientation(win.orientation());
    Eigen::Vector3f axis{rotation_axis_x->value(), rotation_axis_y->value(), rotation_axis_z->value()};
    axis.normalize();
    const Eigen::Quaternionf rotation(Eigen::AngleAxisf(radians, axis));

    switch (rotation_type) {
    case RotationType::World:
      orientation = rotation * orientation;
      break;
    case RotationType::Eye:
    case RotationType::Image:
      orientation *= rotation;
      break;
    default:
      break;
    }

    win.set_orientation(orientation);

    // Translation
    Eigen::Vector3f trans_vec{translate_x->value(), translate_y->value(), translate_z->value()};
    trans_vec /= frames_value;

    Eigen::Vector3f focus(win.focus());
    Eigen::Vector3f target(win.target());

    switch (translation_type) {
    case TranslationType::Voxel:
      trans_vec = img->voxel2scanner().rotation() * trans_vec;
      break;
    case TranslationType::Camera: {
      const Mode::Base *mode = window().get_current_mode();
      if (mode) {
        const GL::vec4 trans_gl_vec = mode->get_current_projection()->modelview_inverse() *
                                      GL::vec4(trans_vec[0], trans_vec[1], trans_vec[2], 0.0f);
        trans_vec[0] = trans_gl_vec[0];
        trans_vec[1] = trans_gl_vec[1];
        trans_vec[2] = trans_gl_vec[2];
      }
      break;
    }
    case TranslationType::Scanner:
      [[fallthrough]];
    default:
      break;
    }

    Eigen::Vector3f focus_delta(trans_vec);

    // If rotating image we need to offset the translation so that the rotation is relative to
    // the center (i.e. target) of the image
    if (rotation_type == RotationType::Image) {
      GL::vec4 target_after = GL::mat4(rotation) * GL::vec4(target[0], target[1], target[2], 1.0f);
      trans_vec += Eigen::Vector3f{target_after[0], target_after[1], target_after[2]} - target;
    }

    focus += focus_delta;
    win.set_focus(focus);

    target += trans_vec;
    win.set_target(target);

    // Volume
    if (volume_axis->value() < static_cast<ssize_t>(image.ndim())) {
      volume += volume_inc;
      win.set_image_volume(volume_axis->value(), std::round(volume));
    }

    // FOV
    win.set_FOV(window().FOV() * (std::pow(FOV_multipler->value(), 1.0F / static_cast<float>(frames_value))));

    start_index->setValue(i + 1);
    this->window().updateGL();
    qApp->processEvents();
  }

  if (progress)
    progress->setValue(static_cast<int>(frames_value));

  is_playing = false;
}

void Capture::select_output_folder_slot() {
  auto load_paths = Dialog::File::input_dirpath(this, "Directory", current_folder);
  if (load_paths.empty())
    return;
  current_folder = load_paths.last_directory;
  folder_button->setText(qstr(shorten(load_paths.single_selection.filename().string(), 20, 0)));
  folder_button->setToolTip(qstr(load_paths.single_selection.string()));
  on_output_update();
}

void Capture::on_output_update() { start_index->setValue(0); }

void Capture::set_msaa_value(int value) {
  // Items are ordered by ascending sample count; select the largest that does not exceed the request.
  int best_index = 0;
  for (int i = 0; i < msaa->count(); ++i) {
    if (msaa->itemData(i).toInt() <= value)
      best_index = i;
  }
  msaa->setCurrentIndex(best_index);
}

void Capture::add_commandline_options(MR::App::OptionList &options) {
  using namespace MR::App;
  // clang-format off
  options + OptionGroup("Screen Capture tool options")

      + Option("capture.folder",
               "Set the output folder for the screen capture tool.").allow_multiple()
        + Argument("path").type_directory_out(DirOutMode::MayExist)

      + Option("capture.prefix",
               "Set the output file prefix for the screen capture tool.").allow_multiple()
        + Argument("string").type_text()

      + Option("capture.supersample",
               "Set the super-sampling (super-resolution) factor for the screen capture tool.").allow_multiple()
        + Argument("factor").type_integer(1)

      + Option("capture.msaa",
               "Set the multi-sample anti-aliasing factor for the screen capture tool "
               "(rounded to a supported power of two).").allow_multiple()
        + Argument("factor").type_integer(1)

      + Option("capture.downsample",
               "Set the down-sampling factor for the screen capture tool; the exported resolution is "
               "the native resolution times the super-sampling factor divided by this factor.").allow_multiple()
        + Argument("factor").type_integer(1)

      + Option("capture.grab",
               "Start the screen capture process.").allow_multiple();
  // clang-format on
}

bool Capture::process_commandline_option(const MR::App::ParsedOption &opt) {
  if (opt.opt->is("capture.folder")) {
    current_folder = static_cast<std::filesystem::path>(opt[0]);
    QString path(qstr(shorten(current_folder.filename().string(), 20, 0)));
    folder_button->setText(path);
    folder_button->setToolTip(qstr(current_folder.string()));
    on_output_update();
    return true;
  }

  if (opt.opt->is("capture.prefix")) {
    prefix_textbox->setText(qstr(opt[0]));
    on_output_update();
    return true;
  }

  if (opt.opt->is("capture.supersample")) {
    supersample->setValue(static_cast<int>(opt[0]));
    return true;
  }

  if (opt.opt->is("capture.msaa")) {
    set_msaa_value(static_cast<int>(opt[0]));
    return true;
  }

  if (opt.opt->is("capture.downsample")) {
    downsample->setValue(static_cast<int>(opt[0]));
    return true;
  }

  if (opt.opt->is("capture.grab")) {
    this->window().updateGL();
    qApp->processEvents();
    on_screen_capture();
    return true;
  }

  return false;
}

} // namespace MR::GUI::MRView::Tool
