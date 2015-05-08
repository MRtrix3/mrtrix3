/*
   Copyright 2014 Brain Research Institute, Melbourne, Australia

   Written by Robert E. Smith, 2015.

   This file is part of MRtrix.

   MRtrix is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   MRtrix is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with MRtrix.  If not, see <http://www.gnu.org/licenses/>.

*/

#ifndef __gui_mrview_tool_connectome_h__
#define __gui_mrview_tool_connectome_h__

#include <map>
#include <vector>

#include "point.h"

#include "gui/opengl/gl.h"
#include "gui/opengl/lighting.h"
#include "gui/opengl/shader.h"
#include "gui/mrview/adjust_button.h"
#include "gui/mrview/colourmap_button.h"
#include "gui/mrview/image.h"
#include "gui/mrview/mode/base.h"
#include "gui/mrview/tool/base.h"
#include "gui/shapes/cube.h"
#include "gui/shapes/cylinder.h"
#include "gui/shapes/sphere.h"
#include "gui/color_button.h"
#include "gui/projection.h"

#include "image/buffer_preload.h"
#include "image/buffer_scratch.h"

#include "mesh/mesh.h"

#include "connectome/mat2vec.h"

#include "dwi/tractography/connectomics/config.h"
#include "dwi/tractography/connectomics/connectomics.h"
#include "dwi/tractography/connectomics/lut.h"





// TODO Elements that still need to be added to the Connectome tool:
//
// * Drawing edges
//   - Display options:
//     * Colour by: Corresponding nodes? Would require some plotting cleverness
//   - Draw as streamlines
//     This would probably require some cross-talk with the tractography tool,
//     but the theory would essentially be:
//     * Read in a tractogram, assigning streamlines to node pairs; and for
//       every edge in the connectome, build a mean exemplar streamline path,
//       constrain to start & end at the node centres
//     * Re-sample the resulting exemplars to an appropriate step size, and
//       store this as an entry in the tractography tool
//     * The Connectome tool would retain access to streamline lengths etc., so
//       that it can write to the scalar & colour buffers
//
//
// * Drawing nodes
//   - When in 2D mode, mesh mode should detect triangles intersecting with the
//     focus plane, and draw the projections on that plane as lines
//     (preferably with adjustable thickness)
//     This may be best handled within a geometry shader: Detect that polygon
//     intersects viewing plane, emit two vertices, draw with GL_LINES
//     (shader will need access to the following two vertices, and run on only
//     every third vertex)
//   - Drawing as overlay: Volume render seems to work, but doesn't always update immediately
//   - In 2D mode, use mask image extent / node location & size to detect when there is no
//     need to process a particular node (need to save the image extent from construction)
//   - Drawing as spheres
//     * May be desirable in some instances to symmetrize the node centre-of-mass positions...?
//     * When in 2D mode, as with mesh mode, detect triangles intersecting with the viewing
//       plane and draw as lines
//   - Draw as points
//   - Meshes
//     * Get right hand rule working, use face culling
//     * Some kind of mesh smoothing
//       -> e.g. "Non-Iterative, Feature-Preserving Mesh Smoothing"
//   - Drawing as cubes: Instead of relying on flat normals, just duplicate the vertices
//     and store normals for each; keep things simple
//     (leave this until necessary, i.e. trying to do a full polygon depth search)
//
// * OpenGL drawing general:
//   - Solve the 'QWidget::repaint: Recursive repaint detected' issue
//     (arose with implementation of the node overlay image)
//   - Make transparency sliders a little more sensible
//     (may need linear scale in 2D mode, non-linear in 3D)
//   - Consider generating all polygonal geometry, store in a vector, sort by camera distance,
//     update index vector accordingly, do a single draw call for both edges and nodes
//     (this is the only way transparency of both nodes and edges can work)
//
// * Nodes GUI section
//   - For colour by file: Need additional elements to appear: Colour map picker w. option
//     to invert colourmap, and upper / lower threshold adjustbars
//   - For size by file, need upper / lower threshold adjustbars in addition to the
//     size slider
//   - Prevent other non-sensible behaviour, e.g.:
//     * Trying to colour by LUT when no LUT is provided
//   - Implement list view with list of nodes, enable manual manupulation of nodes
//
// * Toolbar load
//   - Speed up the node tessellation; either doing it all in a single pass, or multi-threading
//
// * Toolbar
//   - Figure out why the toolbar is initially being drawn twice
//     -> May be something to do with dual screen...
//   - Add lighting checkbox; need to be able to take screenshots with quantitative colour mapping
//   - Implement check for determining when the shader needs to be updated
//   - Disable LUT and config file options if no image is loaded
//   - Enable collapsing of node / edge visualisation groups; will make room for future additions
//
// * Additional functionalities:
//   - Print node name in the GL window
//     How to get access to shorter node names? Rely on user making a new LUT?
//   - External window with capability of showing bar plots for different node parameters,
//     clicking on a node in the main GL window highlights that node in those external plots
//
// * Icons
//   - Main parcellation image






namespace MR
{
  namespace GUI
  {
    namespace MRView
    {
      namespace Tool
      {
        class Connectome : public Base, public ColourMapButtonObserver
        {
            Q_OBJECT

            typedef MR::DWI::Tractography::Connectomics::node_t    node_t;
            typedef MR::DWI::Tractography::Connectomics::Node_info Node_info;
            typedef MR::DWI::Tractography::Connectomics::Node_map  Node_map;

            enum node_geometry_t   { NODE_GEOM_SPHERE, NODE_GEOM_CUBE, NODE_GEOM_OVERLAY, NODE_GEOM_MESH };
            enum node_colour_t     { NODE_COLOUR_FIXED, NODE_COLOUR_RANDOM, NODE_COLOUR_LUT, NODE_COLOUR_FILE };
            enum node_size_t       { NODE_SIZE_FIXED, NODE_SIZE_VOLUME, NODE_SIZE_FILE };
            enum node_visibility_t { NODE_VIS_ALL, NODE_VIS_NONE, NODE_VIS_FILE, NODE_VIS_DEGREE, NODE_VIS_MANUAL };
            enum node_alpha_t      { NODE_ALPHA_FIXED, NODE_ALPHA_LUT, NODE_ALPHA_FILE };

            enum edge_geometry_t   { EDGE_GEOM_LINE, EDGE_GEOM_CYLINDER };
            enum edge_colour_t     { EDGE_COLOUR_FIXED, EDGE_COLOUR_DIR, EDGE_COLOUR_FILE };
            enum edge_size_t       { EDGE_SIZE_FIXED, EDGE_SIZE_FILE };
            enum edge_visibility_t { EDGE_VIS_ALL, EDGE_VIS_NONE, EDGE_VIS_NODES, EDGE_VIS_FILE };
            enum edge_alpha_t      { EDGE_ALPHA_FIXED, EDGE_ALPHA_FILE };

          private:

            class Shader : public GL::Shader::Program {
              public:
                Shader() : GL::Shader::Program () { }
                virtual ~Shader() { }

                bool need_update (const Connectome&) const;
                virtual void update (const Connectome&) = 0;

                void start (const Connectome& parent) {
                  if (*this == 0 || need_update (parent))
                    recompile (parent);
                  GL::Shader::Program::start();
                }

              protected:
                std::string vertex_shader_source, fragment_shader_source;

              private:
                void recompile (const Connectome& parent);
            };

            class NodeShader : public Shader
            {
              public:
                NodeShader() : Shader () { }
                ~NodeShader() { }
                void update (const Connectome&) override;
            } node_shader;

            class EdgeShader : public Shader
            {
              public:
                EdgeShader() : Shader () { }
                ~EdgeShader() { }
                void update (const Connectome&) override;
            } edge_shader;


          public:

            Connectome (Window& main_window, Dock* parent);

            virtual ~Connectome ();

            void draw (const Projection& transform, bool is_3D, int axis, int slice);
            void drawOverlays (const Projection& transform) override;
            bool process_batch_command (const std::string& cmd, const std::string& args);

            size_t num_nodes() const { return nodes.size() ? nodes.size() - 1 : 0; }
            size_t num_edges() const { return edges.size(); }

          private slots:
            void image_open_slot ();
            void lut_open_slot (int);
            void config_open_slot ();
            void hide_all_slot ();

            void node_geometry_selection_slot (int);
            void node_colour_selection_slot (int);
            void node_size_selection_slot (int);
            void node_visibility_selection_slot (int);
            void node_alpha_selection_slot (int);

            void sphere_lod_slot (int);
            void overlay_interp_slot (int);
            void node_colour_change_slot();
            void node_size_value_slot();
            void node_visibility_parameter_slot();
            void node_alpha_value_slot (int);
            void node_alpha_parameter_slot();

            void edge_geometry_selection_slot (int);
            void edge_colour_selection_slot (int);
            void edge_size_selection_slot (int);
            void edge_visibility_selection_slot (int);
            void edge_alpha_selection_slot (int);

            void cylinder_lod_slot (int);
            void edge_colour_change_slot();
            void edge_size_value_slot();
            void edge_alpha_value_slot (int);

          protected:

            QPushButton *image_button, *hide_all_button;
            QComboBox *lut_combobox;
            QPushButton *config_button;

            QComboBox *node_geometry_combobox, *node_colour_combobox, *node_size_combobox, *node_visibility_combobox, *node_alpha_combobox;

            QLabel *node_geometry_sphere_lod_label;
            QSpinBox *node_geometry_sphere_lod_spinbox;
            QCheckBox *node_geometry_overlay_interp_checkbox;

            QColorButton *node_colour_fixedcolour_button;
            ColourMapButton *node_colour_colourmap_button;

            AdjustButton *node_size_button;

            QLabel *node_visibility_threshold_label;
            AdjustButton *node_visibility_threshold_button;
            QCheckBox *node_visibility_threshold_invert_checkbox;

            QSlider *node_alpha_slider;
            QLabel *node_alpha_range_label;
            AdjustButton *node_alpha_lower_button, *node_alpha_upper_button;
            QCheckBox *node_alpha_invert_checkbox;

            QComboBox *edge_geometry_combobox, *edge_colour_combobox, *edge_size_combobox, *edge_visibility_combobox, *edge_alpha_combobox;

            QLabel *edge_geometry_cylinder_lod_label;
            QSpinBox *edge_geometry_cylinder_lod_spinbox;

            QColorButton *edge_colour_fixedcolour_button;
            ColourMapButton *edge_colour_colourmap_button;

            AdjustButton *edge_size_button;

            QSlider *edge_alpha_slider;


          private:

            // Stores all information relating to the drawing of individual nodes, both fixed and variable
            class Node
            {
              public:
                Node (const Point<float>&, const size_t, MR::Image::BufferScratch<bool>&);
                Node ();

                void render_mesh() const { mesh.render(); }

                const Point<float>& get_com() const { return centre_of_mass; }
                size_t get_volume() const { return volume; }

                void set_name (const std::string& i) { name = i; }
                const std::string& get_name() const { return name; }
                void set_size (const float i) { size = i; }
                float get_size() const { return size; }
                void set_colour (const Point<float>& i) { colour = i; }
                const Point<float>& get_colour() const { return colour; }
                void set_alpha (const float i) { alpha = i; }
                float get_alpha() const { return alpha; }
                void set_visible (const bool i) { visible = i; }
                bool is_visible() const { return visible; }


              private:
                const Point<float> centre_of_mass;
                const size_t volume;

                std::string name;
                float size;
                Point<float> colour;
                float alpha;
                bool visible;

                // Helper class to manage the storage and display of the mesh for each node
                class Mesh {
                  public:
                    Mesh (const MR::Mesh::Mesh&);
                    Mesh (const Mesh&) = delete;
                    Mesh (Mesh&&);
                    Mesh ();
                    ~Mesh() { }
                    Mesh& operator= (Mesh&&);
                    void render() const;
                  private:
                    GLsizei count;
                    GL::VertexBuffer vertex_buffer, normal_buffer;
                    GL::VertexArrayObject vertex_array_object;
                    GL::IndexBuffer index_buffer;
                } mesh;

            };

            // Stores all information relating to the drawing of individual edges, both fixed and variable
            // Try to store more than would otherwise be optimal in here, in order to simplify the drawing process
            class Edge
            {
              public:
                Edge (const Connectome&, const node_t, const node_t);
                Edge (Edge&&);
                Edge ();
                ~Edge();

                void render_line() const;

                node_t get_node_index (const size_t i) const { assert (i==0 || i==1); return node_indices[i]; }
                const Point<float> get_node_centre (const size_t i) const { assert (i==0 || i==1); return node_centres[i]; }
                Point<float> get_com() const { return (node_centres[0] + node_centres[1]) * 0.5; }

                const GLfloat* get_rot_matrix() const { return rot_matrix; }

                const Point<float>& get_dir() const { return dir; }
                void set_size (const float i) { size = i; }
                float get_size() const { return size; }
                void set_colour (const Point<float>& i) { colour = i; }
                const Point<float>& get_colour() const { return colour; }
                void set_alpha (const float i) { alpha = i; }
                float get_alpha() const { return alpha; }
                void set_visible (const bool i) { visible = i; }
                bool is_visible() const { return visible; }
                bool is_diagonal() const { return (node_indices[0] == node_indices[1]); }

              private:
                const node_t node_indices[2];
                const Point<float> node_centres[2];
                const Point<float> dir;

                GLfloat* rot_matrix;

                float size;
                Point<float> colour;
                float alpha;
                bool visible;

            };

            // Vector that stores the name of the file imported, so it can be displayed in the GUI
            class FileDataVector : public Math::Vector<float>
            {
              public:
                FileDataVector () : Math::Vector<float>(), min (NAN), max (NAN) { }
                FileDataVector (const FileDataVector& V) : Math::Vector<float> (V), name (V.name), min (V.min), max (V.max) { }
                FileDataVector (size_t nelements) : Math::Vector<float> (nelements), min (NAN), max (NAN) { }
                FileDataVector (const std::string& file) : Math::Vector<float> (file), name (Path::basename (file).c_str()), min (NAN), max (NAN) { calc_minmax(); }

                FileDataVector& load (const std::string&);
                FileDataVector& clear();

                const QString& get_name() const { return name; }
                void set_name (const std::string& s) { name = s.c_str(); }

                float get_min() const { return min; }
                float get_max() const { return max; }

              private:
                QString name;
                float min, max;

                void calc_minmax();

            };

            // Class to handle the node image overlay
            class NodeOverlay : public MR::GUI::MRView::ImageBase
            {
              public:
                NodeOverlay (const MR::Image::Info&);

                void update_texture2D (const int, const int) override;
                void update_texture3D() override;

                typedef MR::Image::BufferScratch<float>::voxel_type voxel_type;
                voxel_type voxel() { need_update = true; return voxel_type (data); }

              private:
                MR::Image::BufferScratch<float> data;
                bool need_update;

              public:
                class Shader : public Displayable::Shader {
                  public:
                    virtual std::string vertex_shader_source (const Displayable&);
                    virtual std::string fragment_shader_source (const Displayable&);
                } slice_shader;
            };




            // For the sake of viewing nodes as an overlay, need to ALWAYS
            // have access to the parcellation image
            Ptr< MR::Image::BufferPreload<node_t> > buffer;


            std::vector<Node> nodes;
            std::vector<Edge> edges;


            // For converting connectome matrices to vectors
            MR::Connectome::Mat2Vec mat2vec;


            // If a lookup table is provided, this container will store the
            //   properties of each node as provided in that file (e.g. name & colour)
            Node_map lut;

            // If a connectome configuration file is provided, this will map
            //   each structure name to an index in the parcellation image;
            //   this can then be used to produce the lookup table
            std::vector<std::string> config;

            // If both a LUT and a config file have been provided, this provides
            //   a direct vector mapping from image node index to a position in
            //   the lookup table, pre-generated
            std::vector<Node_map::const_iterator> lut_mapping;


            // Used when the geometry of node visualisation is a sphere
            Shapes::Sphere sphere;
            GL::VertexArrayObject sphere_VAO;

            // Used when the geometry of node visualisation is a cube
            Shapes::Cube cube;
            GL::VertexArrayObject cube_VAO;

            // Used when the geometry of node visualisation is an image overlay
            Ptr<NodeOverlay> node_overlay;

            // Used when the geometry of edge visualisation is a cylinder
            Shapes::Cylinder cylinder;
            GL::VertexArrayObject cylinder_VAO;


            // Fixed lighting settings from the main window
            const GL::Lighting& lighting;


            // Current node visualisation settings
            node_geometry_t node_geometry;
            node_colour_t node_colour;
            node_size_t node_size;
            node_visibility_t node_visibility;
            node_alpha_t node_alpha;

            // Other values that need to be stored w.r.t. node visualisation
            Point<float> node_fixed_colour;
            float node_fixed_alpha;
            float node_size_scale_factor;
            float voxel_volume;
            FileDataVector node_values_from_file_colour;
            FileDataVector node_values_from_file_size;
            FileDataVector node_values_from_file_visibility;
            FileDataVector node_values_from_file_alpha;


            // Current edge visualisation settings
            edge_geometry_t edge_geometry;
            edge_colour_t edge_colour;
            edge_size_t edge_size;
            edge_visibility_t edge_visibility;
            edge_alpha_t edge_alpha;

            // Other values that need to be stored w.r.t. node visualisation
            Point<float> edge_fixed_colour;
            float edge_fixed_alpha;
            float edge_size_scale_factor;
            FileDataVector edge_values_from_file_colour;
            FileDataVector edge_values_from_file_size;
            FileDataVector edge_values_from_file_visibility;
            FileDataVector edge_values_from_file_alpha;


            // Helper functions
            void clear_all();
            void initialise (const std::string&);

            void import_file_for_node_property (FileDataVector&, const std::string&);
            void import_file_for_edge_property (FileDataVector&, const std::string&);

            void load_properties();

            void calculate_node_colours();
            void calculate_node_sizes();
            void calculate_node_visibility();
            void calculate_node_alphas();

            void update_node_overlay();

            void calculate_edge_colours();
            void calculate_edge_sizes();
            void calculate_edge_visibility();
            void calculate_edge_alphas();

        };








      }
    }
  }
}

#endif




