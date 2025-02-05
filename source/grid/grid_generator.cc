// ---------------------------------------------------------------------
//
// Copyright (C) 1999 - 2019 by the deal.II authors
//
// This file is part of the deal.II library.
//
// The deal.II library is free software; you can use it, redistribute
// it, and/or modify it under the terms of the GNU Lesser General
// Public License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
// The full text of the license can be found in the file LICENSE.md at
// the top level directory of deal.II.
//
// ---------------------------------------------------------------------

#include <deal.II/distributed/shared_tria.h>
#include <deal.II/distributed/tria.h>

#include <deal.II/grid/grid_generator.h>
#include <deal.II/grid/grid_reordering.h>
#include <deal.II/grid/grid_tools.h>
#include <deal.II/grid/intergrid_map.h>
#include <deal.II/grid/manifold_lib.h>
#include <deal.II/grid/tria.h>
#include <deal.II/grid/tria_accessor.h>
#include <deal.II/grid/tria_iterator.h>

#include <cmath>
#include <limits>

DEAL_II_NAMESPACE_OPEN


namespace GridGenerator
{
  namespace
  {
    /**
     * Perform the action specified by the @p colorize flag of the
     * hyper_rectangle() function of this class.
     */
    template <int dim, int spacedim>
    void
    colorize_hyper_rectangle(Triangulation<dim, spacedim> &tria)
    {
      // there is nothing to do in 1d
      if (dim > 1)
        {
          // there is only one cell, so
          // simple task
          const typename Triangulation<dim, spacedim>::cell_iterator cell =
            tria.begin();
          for (unsigned int f = 0; f < GeometryInfo<dim>::faces_per_cell; ++f)
            cell->face(f)->set_boundary_id(f);
        }
    }



    template <int spacedim>
    void colorize_subdivided_hyper_rectangle(Triangulation<1, spacedim> &tria,
                                             const Point<spacedim> &,
                                             const Point<spacedim> &,
                                             const double)
    {
      for (typename Triangulation<1, spacedim>::cell_iterator cell =
             tria.begin();
           cell != tria.end();
           ++cell)
        if (cell->center()(0) > 0)
          cell->set_material_id(1);
      // boundary indicators are set to
      // 0 (left) and 1 (right) by default.
    }



    template <int dim, int spacedim>
    void
    colorize_subdivided_hyper_rectangle(Triangulation<dim, spacedim> &tria,
                                        const Point<spacedim> &       p1,
                                        const Point<spacedim> &       p2,
                                        const double                  epsilon)
    {
      // run through all faces and check
      // if one of their center coordinates matches
      // one of the corner points. Comparisons
      // are made using an epsilon which
      // should be smaller than the smallest cell
      // diameter.

      typename Triangulation<dim, spacedim>::face_iterator face =
                                                             tria.begin_face(),
                                                           endface =
                                                             tria.end_face();
      for (; face != endface; ++face)
        if (face->at_boundary())
          if (face->boundary_id() == 0)
            {
              const Point<spacedim> center(face->center());

              if (std::abs(center(0) - p1[0]) < epsilon)
                face->set_boundary_id(0);
              else if (std::abs(center(0) - p2[0]) < epsilon)
                face->set_boundary_id(1);
              else if (dim > 1 && std::abs(center(1) - p1[1]) < epsilon)
                face->set_boundary_id(2);
              else if (dim > 1 && std::abs(center(1) - p2[1]) < epsilon)
                face->set_boundary_id(3);
              else if (dim > 2 && std::abs(center(2) - p1[2]) < epsilon)
                face->set_boundary_id(4);
              else if (dim > 2 && std::abs(center(2) - p2[2]) < epsilon)
                face->set_boundary_id(5);
              else
                // triangulation says it
                // is on the boundary,
                // but we could not find
                // on which boundary.
                Assert(false, ExcInternalError());
            }

      for (typename Triangulation<dim, spacedim>::cell_iterator cell =
             tria.begin();
           cell != tria.end();
           ++cell)
        {
          char id = 0;
          for (unsigned int d = 0; d < dim; ++d)
            if (cell->center()(d) > 0)
              id += (1 << d);
          cell->set_material_id(id);
        }
    }


    /**
     * Assign boundary number zero to the inner shell boundary and 1 to the
     * outer.
     */
    void colorize_hyper_shell(Triangulation<2> &tria,
                              const Point<2> &,
                              const double,
                              const double)
    {
      // In spite of receiving geometrical
      // data, we do this only based on
      // topology.

      // For the mesh based on cube,
      // this is highly irregular
      for (Triangulation<2>::cell_iterator cell = tria.begin();
           cell != tria.end();
           ++cell)
        {
          Assert(cell->face(2)->at_boundary(), ExcInternalError());
          cell->face(2)->set_all_boundary_ids(1);
        }
    }


    /**
     * Assign boundary number zero to the inner shell boundary and 1 to the
     * outer.
     */
    void colorize_hyper_shell(Triangulation<3> &tria,
                              const Point<3> &,
                              const double,
                              const double)
    {
      // the following uses a good amount
      // of knowledge about the
      // orientation of cells. this is
      // probably not good style...
      if (tria.n_cells() == 6)
        {
          Triangulation<3>::cell_iterator cell = tria.begin();

          Assert(cell->face(4)->at_boundary(), ExcInternalError());
          cell->face(4)->set_all_boundary_ids(1);

          ++cell;
          Assert(cell->face(2)->at_boundary(), ExcInternalError());
          cell->face(2)->set_all_boundary_ids(1);

          ++cell;
          Assert(cell->face(2)->at_boundary(), ExcInternalError());
          cell->face(2)->set_all_boundary_ids(1);

          ++cell;
          Assert(cell->face(0)->at_boundary(), ExcInternalError());
          cell->face(0)->set_all_boundary_ids(1);

          ++cell;
          Assert(cell->face(2)->at_boundary(), ExcInternalError());
          cell->face(2)->set_all_boundary_ids(1);

          ++cell;
          Assert(cell->face(0)->at_boundary(), ExcInternalError());
          cell->face(0)->set_all_boundary_ids(1);
        }
      else if (tria.n_cells() == 12)
        {
          // again use some internal
          // knowledge
          for (Triangulation<3>::cell_iterator cell = tria.begin();
               cell != tria.end();
               ++cell)
            {
              Assert(cell->face(5)->at_boundary(), ExcInternalError());
              cell->face(5)->set_all_boundary_ids(1);
            }
        }
      else if (tria.n_cells() == 96)
        {
          // the 96-cell hypershell is
          // based on a once refined
          // 12-cell mesh. consequently,
          // since the outer faces all
          // are face_no==5 above, so
          // they are here (unless they
          // are in the interior). Use
          // this to assign boundary
          // indicators, but also make
          // sure that we encounter
          // exactly 48 such faces
          unsigned int count = 0;
          for (Triangulation<3>::cell_iterator cell = tria.begin();
               cell != tria.end();
               ++cell)
            if (cell->face(5)->at_boundary())
              {
                cell->face(5)->set_all_boundary_ids(1);
                ++count;
              }
          Assert(count == 48, ExcInternalError());
        }
      else
        Assert(false, ExcNotImplemented());
    }



    /**
     * Assign boundary number zero the inner shell boundary, one to the outer
     * shell boundary, two to the face with x=0, three to the face with y=0,
     * four to the face with z=0.
     */
    void colorize_quarter_hyper_shell(Triangulation<3> &tria,
                                      const Point<3> &  center,
                                      const double      inner_radius,
                                      const double      outer_radius)
    {
      if (tria.n_cells() != 3)
        AssertThrow(false, ExcNotImplemented());

      double middle = (outer_radius - inner_radius) / 2e0 + inner_radius;
      double eps    = 1e-3 * middle;
      Triangulation<3>::cell_iterator cell = tria.begin();

      for (; cell != tria.end(); ++cell)
        for (unsigned int f = 0; f < GeometryInfo<3>::faces_per_cell; ++f)
          {
            if (!cell->face(f)->at_boundary())
              continue;

            double radius = cell->face(f)->center().norm() - center.norm();
            if (std::fabs(cell->face(f)->center()(0)) <
                eps) // x = 0 set boundary 2
              {
                cell->face(f)->set_boundary_id(2);
                for (unsigned int j = 0; j < GeometryInfo<3>::lines_per_face;
                     ++j)
                  if (cell->face(f)->line(j)->at_boundary())
                    if (std::fabs(cell->face(f)->line(j)->vertex(0).norm() -
                                  cell->face(f)->line(j)->vertex(1).norm()) >
                        eps)
                      cell->face(f)->line(j)->set_boundary_id(2);
              }
            else if (std::fabs(cell->face(f)->center()(1)) <
                     eps) // y = 0 set boundary 3
              {
                cell->face(f)->set_boundary_id(3);
                for (unsigned int j = 0; j < GeometryInfo<3>::lines_per_face;
                     ++j)
                  if (cell->face(f)->line(j)->at_boundary())
                    if (std::fabs(cell->face(f)->line(j)->vertex(0).norm() -
                                  cell->face(f)->line(j)->vertex(1).norm()) >
                        eps)
                      cell->face(f)->line(j)->set_boundary_id(3);
              }
            else if (std::fabs(cell->face(f)->center()(2)) <
                     eps) // z = 0 set boundary 4
              {
                cell->face(f)->set_boundary_id(4);
                for (unsigned int j = 0; j < GeometryInfo<3>::lines_per_face;
                     ++j)
                  if (cell->face(f)->line(j)->at_boundary())
                    if (std::fabs(cell->face(f)->line(j)->vertex(0).norm() -
                                  cell->face(f)->line(j)->vertex(1).norm()) >
                        eps)
                      cell->face(f)->line(j)->set_boundary_id(4);
              }
            else if (radius < middle) // inner radius set boundary 0
              {
                cell->face(f)->set_boundary_id(0);
                for (unsigned int j = 0; j < GeometryInfo<3>::lines_per_face;
                     ++j)
                  if (cell->face(f)->line(j)->at_boundary())
                    if (std::fabs(cell->face(f)->line(j)->vertex(0).norm() -
                                  cell->face(f)->line(j)->vertex(1).norm()) <
                        eps)
                      cell->face(f)->line(j)->set_boundary_id(0);
              }
            else if (radius > middle) // outer radius set boundary 1
              {
                cell->face(f)->set_boundary_id(1);
                for (unsigned int j = 0; j < GeometryInfo<3>::lines_per_face;
                     ++j)
                  if (cell->face(f)->line(j)->at_boundary())
                    if (std::fabs(cell->face(f)->line(j)->vertex(0).norm() -
                                  cell->face(f)->line(j)->vertex(1).norm()) <
                        eps)
                      cell->face(f)->line(j)->set_boundary_id(1);
              }
            else
              Assert(false, ExcInternalError());
          }
    }

  } // namespace


  template <int dim, int spacedim>
  void
  hyper_rectangle(Triangulation<dim, spacedim> &tria,
                  const Point<dim> &            p_1,
                  const Point<dim> &            p_2,
                  const bool                    colorize)
  {
    // First, extend dimensions from dim to spacedim and
    // normalize such that p1 is lower in all coordinate
    // directions. Additional entries will be 0.
    Point<spacedim> p1, p2;
    for (unsigned int i = 0; i < dim; ++i)
      {
        p1(i) = std::min(p_1(i), p_2(i));
        p2(i) = std::max(p_1(i), p_2(i));
      }

    std::vector<Point<spacedim>> vertices(GeometryInfo<dim>::vertices_per_cell);
    switch (dim)
      {
        case 1:
          vertices[0] = p1;
          vertices[1] = p2;
          break;
        case 2:
          vertices[0] = vertices[1] = p1;
          vertices[2] = vertices[3] = p2;

          vertices[1](0) = p2(0);
          vertices[2](0) = p1(0);
          break;
        case 3:
          vertices[0] = vertices[1] = vertices[2] = vertices[3] = p1;
          vertices[4] = vertices[5] = vertices[6] = vertices[7] = p2;

          vertices[1](0) = p2(0);
          vertices[2](1) = p2(1);
          vertices[3](0) = p2(0);
          vertices[3](1) = p2(1);

          vertices[4](0) = p1(0);
          vertices[4](1) = p1(1);
          vertices[5](1) = p1(1);
          vertices[6](0) = p1(0);

          break;
        default:
          Assert(false, ExcNotImplemented());
      }

    // Prepare cell data
    std::vector<CellData<dim>> cells(1);
    for (unsigned int i = 0; i < GeometryInfo<dim>::vertices_per_cell; ++i)
      cells[0].vertices[i] = i;
    cells[0].material_id = 0;

    tria.create_triangulation(vertices, cells, SubCellData());

    // Assign boundary indicators
    if (colorize)
      colorize_hyper_rectangle(tria);
  }


  template <int dim, int spacedim>
  void
  hyper_cube(Triangulation<dim, spacedim> &tria,
             const double                  left,
             const double                  right,
             const bool                    colorize)
  {
    Assert(left < right,
           ExcMessage("Invalid left-to-right bounds of hypercube"));

    Point<dim> p1, p2;
    for (unsigned int i = 0; i < dim; ++i)
      {
        p1(i) = left;
        p2(i) = right;
      }
    hyper_rectangle(tria, p1, p2, colorize);
  }

  template <int dim>
  void
  simplex(Triangulation<dim> &tria, const std::vector<Point<dim>> &vertices)
  {
    AssertDimension(vertices.size(), dim + 1);
    Assert(dim > 1, ExcNotImplemented());
    Assert(dim < 4, ExcNotImplemented());

#ifdef DEBUG
    Tensor<2, dim> vector_matrix;
    for (unsigned int d = 0; d < dim; ++d)
      for (unsigned int c = 1; c <= dim; ++c)
        vector_matrix[c - 1][d] = vertices[c](d) - vertices[0](d);
    Assert(determinant(vector_matrix) > 0.,
           ExcMessage("Vertices of simplex must form a right handed system"));
#endif

    // Set up the vertices by first copying into points.
    std::vector<Point<dim>> points = vertices;
    Point<dim>              center;
    // Compute the edge midpoints and add up everything to compute the
    // center point.
    for (unsigned int i = 0; i <= dim; ++i)
      {
        points.push_back(0.5 * (points[i] + points[(i + 1) % (dim + 1)]));
        center += points[i];
      }
    if (dim > 2)
      {
        // In 3D, we have some more edges to deal with
        for (unsigned int i = 1; i < dim; ++i)
          points.push_back(0.5 * (points[i - 1] + points[i + 1]));
        // And we need face midpoints
        for (unsigned int i = 0; i <= dim; ++i)
          points.push_back(1. / 3. *
                           (points[i] + points[(i + 1) % (dim + 1)] +
                            points[(i + 2) % (dim + 1)]));
      }
    points.push_back((1. / (dim + 1)) * center);

    std::vector<CellData<dim>> cells(dim + 1);
    switch (dim)
      {
        case 2:
          AssertDimension(points.size(), 7);
          cells[0].vertices[0] = 0;
          cells[0].vertices[1] = 3;
          cells[0].vertices[2] = 5;
          cells[0].vertices[3] = 6;
          cells[0].material_id = 0;

          cells[1].vertices[0] = 3;
          cells[1].vertices[1] = 1;
          cells[1].vertices[2] = 6;
          cells[1].vertices[3] = 4;
          cells[1].material_id = 0;

          cells[2].vertices[0] = 5;
          cells[2].vertices[1] = 6;
          cells[2].vertices[2] = 2;
          cells[2].vertices[3] = 4;
          cells[2].material_id = 0;
          break;
        case 3:
          AssertDimension(points.size(), 15);
          cells[0].vertices[0] = 0;
          cells[0].vertices[1] = 4;
          cells[0].vertices[2] = 8;
          cells[0].vertices[3] = 10;
          cells[0].vertices[4] = 7;
          cells[0].vertices[5] = 13;
          cells[0].vertices[6] = 12;
          cells[0].vertices[7] = 14;
          cells[0].material_id = 0;

          cells[1].vertices[0] = 4;
          cells[1].vertices[1] = 1;
          cells[1].vertices[2] = 10;
          cells[1].vertices[3] = 5;
          cells[1].vertices[4] = 13;
          cells[1].vertices[5] = 9;
          cells[1].vertices[6] = 14;
          cells[1].vertices[7] = 11;
          cells[1].material_id = 0;

          cells[2].vertices[0] = 8;
          cells[2].vertices[1] = 10;
          cells[2].vertices[2] = 2;
          cells[2].vertices[3] = 5;
          cells[2].vertices[4] = 12;
          cells[2].vertices[5] = 14;
          cells[2].vertices[6] = 6;
          cells[2].vertices[7] = 11;
          cells[2].material_id = 0;

          cells[3].vertices[0] = 7;
          cells[3].vertices[1] = 13;
          cells[3].vertices[2] = 12;
          cells[3].vertices[3] = 14;
          cells[3].vertices[4] = 3;
          cells[3].vertices[5] = 9;
          cells[3].vertices[6] = 6;
          cells[3].vertices[7] = 11;
          cells[3].material_id = 0;
          break;
        default:
          Assert(false, ExcNotImplemented());
      }
    tria.create_triangulation(points, cells, SubCellData());
  }


  void moebius(Triangulation<3> & tria,
               const unsigned int n_cells,
               const unsigned int n_rotations,
               const double       R,
               const double       r)
  {
    const unsigned int dim = 3;
    Assert(n_cells > 4,
           ExcMessage(
             "More than 4 cells are needed to create a moebius grid."));
    Assert(r > 0 && R > 0,
           ExcMessage("Outer and inner radius must be positive."));
    Assert(R > r,
           ExcMessage("Outer radius must be greater than inner radius."));


    std::vector<Point<dim>> vertices(4 * n_cells);
    double beta_step  = n_rotations * numbers::PI / 2.0 / n_cells;
    double alpha_step = 2.0 * numbers::PI / n_cells;

    for (unsigned int i = 0; i < n_cells; ++i)
      for (unsigned int j = 0; j < 4; ++j)
        {
          vertices[4 * i + j][0] =
            R * std::cos(i * alpha_step) +
            r * std::cos(i * beta_step + j * numbers::PI / 2.0) *
              std::cos(i * alpha_step);
          vertices[4 * i + j][1] =
            R * std::sin(i * alpha_step) +
            r * std::cos(i * beta_step + j * numbers::PI / 2.0) *
              std::sin(i * alpha_step);
          vertices[4 * i + j][2] =
            r * std::sin(i * beta_step + j * numbers::PI / 2.0);
        }

    unsigned int offset = 0;

    std::vector<CellData<dim>> cells(n_cells);
    for (unsigned int i = 0; i < n_cells; ++i)
      {
        for (unsigned int j = 0; j < 2; ++j)
          {
            cells[i].vertices[0 + 4 * j] = offset + 0 + 4 * j;
            cells[i].vertices[1 + 4 * j] = offset + 3 + 4 * j;
            cells[i].vertices[2 + 4 * j] = offset + 2 + 4 * j;
            cells[i].vertices[3 + 4 * j] = offset + 1 + 4 * j;
          }
        offset += 4;
        cells[i].material_id = 0;
      }

    // now correct the last four vertices
    cells[n_cells - 1].vertices[4] = (0 + n_rotations) % 4;
    cells[n_cells - 1].vertices[5] = (3 + n_rotations) % 4;
    cells[n_cells - 1].vertices[6] = (2 + n_rotations) % 4;
    cells[n_cells - 1].vertices[7] = (1 + n_rotations) % 4;

    GridReordering<dim>::invert_all_cells_of_negative_grid(vertices, cells);
    tria.create_triangulation_compatibility(vertices, cells, SubCellData());
  }



  template <>
  void torus<2, 3>(Triangulation<2, 3> &tria,
                   const double         R,
                   const double         r,
                   const unsigned int)
  {
    Assert(R > r,
           ExcMessage("Outer radius R must be greater than the inner "
                      "radius r."));
    Assert(r > 0.0, ExcMessage("The inner radius r must be positive."));

    const unsigned int           dim      = 2;
    const unsigned int           spacedim = 3;
    std::vector<Point<spacedim>> vertices(16);

    vertices[0]  = Point<spacedim>(R - r, 0, 0);
    vertices[1]  = Point<spacedim>(R, -r, 0);
    vertices[2]  = Point<spacedim>(R + r, 0, 0);
    vertices[3]  = Point<spacedim>(R, r, 0);
    vertices[4]  = Point<spacedim>(0, 0, R - r);
    vertices[5]  = Point<spacedim>(0, -r, R);
    vertices[6]  = Point<spacedim>(0, 0, R + r);
    vertices[7]  = Point<spacedim>(0, r, R);
    vertices[8]  = Point<spacedim>(-(R - r), 0, 0);
    vertices[9]  = Point<spacedim>(-R, -r, 0);
    vertices[10] = Point<spacedim>(-(R + r), 0, 0);
    vertices[11] = Point<spacedim>(-R, r, 0);
    vertices[12] = Point<spacedim>(0, 0, -(R - r));
    vertices[13] = Point<spacedim>(0, -r, -R);
    vertices[14] = Point<spacedim>(0, 0, -(R + r));
    vertices[15] = Point<spacedim>(0, r, -R);

    std::vector<CellData<dim>> cells(16);
    // Right Hand Orientation
    cells[0].vertices[0] = 0;
    cells[0].vertices[1] = 4;
    cells[0].vertices[2] = 7;
    cells[0].vertices[3] = 3;
    cells[0].material_id = 0;

    cells[1].vertices[0] = 1;
    cells[1].vertices[1] = 5;
    cells[1].vertices[2] = 4;
    cells[1].vertices[3] = 0;
    cells[1].material_id = 0;

    cells[2].vertices[0] = 2;
    cells[2].vertices[1] = 6;
    cells[2].vertices[2] = 5;
    cells[2].vertices[3] = 1;
    cells[2].material_id = 0;

    cells[3].vertices[0] = 3;
    cells[3].vertices[1] = 7;
    cells[3].vertices[2] = 6;
    cells[3].vertices[3] = 2;
    cells[3].material_id = 0;

    cells[4].vertices[0] = 4;
    cells[4].vertices[1] = 8;
    cells[4].vertices[2] = 11;
    cells[4].vertices[3] = 7;
    cells[4].material_id = 0;

    cells[5].vertices[0] = 5;
    cells[5].vertices[1] = 9;
    cells[5].vertices[2] = 8;
    cells[5].vertices[3] = 4;
    cells[5].material_id = 0;

    cells[6].vertices[0] = 6;
    cells[6].vertices[1] = 10;
    cells[6].vertices[2] = 9;
    cells[6].vertices[3] = 5;
    cells[6].material_id = 0;

    cells[7].vertices[0] = 7;
    cells[7].vertices[1] = 11;
    cells[7].vertices[2] = 10;
    cells[7].vertices[3] = 6;
    cells[7].material_id = 0;

    cells[8].vertices[0] = 8;
    cells[8].vertices[1] = 12;
    cells[8].vertices[2] = 15;
    cells[8].vertices[3] = 11;
    cells[8].material_id = 0;

    cells[9].vertices[0] = 9;
    cells[9].vertices[1] = 13;
    cells[9].vertices[2] = 12;
    cells[9].vertices[3] = 8;
    cells[9].material_id = 0;

    cells[10].vertices[0] = 10;
    cells[10].vertices[1] = 14;
    cells[10].vertices[2] = 13;
    cells[10].vertices[3] = 9;
    cells[10].material_id = 0;

    cells[11].vertices[0] = 11;
    cells[11].vertices[1] = 15;
    cells[11].vertices[2] = 14;
    cells[11].vertices[3] = 10;
    cells[11].material_id = 0;

    cells[12].vertices[0] = 12;
    cells[12].vertices[1] = 0;
    cells[12].vertices[2] = 3;
    cells[12].vertices[3] = 15;
    cells[12].material_id = 0;

    cells[13].vertices[0] = 13;
    cells[13].vertices[1] = 1;
    cells[13].vertices[2] = 0;
    cells[13].vertices[3] = 12;
    cells[13].material_id = 0;

    cells[14].vertices[0] = 14;
    cells[14].vertices[1] = 2;
    cells[14].vertices[2] = 1;
    cells[14].vertices[3] = 13;
    cells[14].material_id = 0;

    cells[15].vertices[0] = 15;
    cells[15].vertices[1] = 3;
    cells[15].vertices[2] = 2;
    cells[15].vertices[3] = 14;
    cells[15].material_id = 0;

    // Must call this to be able to create a
    // correct triangulation in dealii, read
    // GridReordering<> doc
    GridReordering<dim, spacedim>::reorder_cells(cells);
    tria.create_triangulation_compatibility(vertices, cells, SubCellData());

    tria.set_all_manifold_ids(0);
    tria.set_manifold(0, TorusManifold<2>(R, r));
  }



  template <>
  void torus<3, 3>(Triangulation<3, 3> &tria,
                   const double         R,
                   const double         r,
                   const unsigned int   n_cells_toroidal)
  {
    Assert(R > r,
           ExcMessage("Outer radius R must be greater than the inner "
                      "radius r."));
    Assert(r > 0.0, ExcMessage("The inner radius r must be positive."));
    Assert(n_cells_toroidal > 2,
           ExcMessage("Number of cells in toroidal direction has "
                      "to be at least 3."));

    // the first 8 vertices are in the x-y-plane
    Point<3> const        p = Point<3>(R, 0.0, 0.0);
    double const          a = 1. / (1 + std::sqrt(2.0));
    std::vector<Point<3>> vertices(8 * n_cells_toroidal);
    vertices[0] = p + Point<3>(-1, -1, 0) * (r / std::sqrt(2.0)),
    vertices[1] = p + Point<3>(+1, -1, 0) * (r / std::sqrt(2.0)),
    vertices[2] = p + Point<3>(-1, -1, 0) * (r / std::sqrt(2.0) * a),
    vertices[3] = p + Point<3>(+1, -1, 0) * (r / std::sqrt(2.0) * a),
    vertices[4] = p + Point<3>(-1, +1, 0) * (r / std::sqrt(2.0) * a),
    vertices[5] = p + Point<3>(+1, +1, 0) * (r / std::sqrt(2.0) * a),
    vertices[6] = p + Point<3>(-1, +1, 0) * (r / std::sqrt(2.0)),
    vertices[7] = p + Point<3>(+1, +1, 0) * (r / std::sqrt(2.0));

    // create remaining vertices by rotating around negative y-axis (the
    // direction is to ensure positive cell measures)
    double phi_cell = 2.0 * numbers::PI / n_cells_toroidal;
    for (unsigned int c = 1; c < n_cells_toroidal; ++c)
      {
        for (unsigned int v = 0; v < 8; ++v)
          {
            double const r_2d      = vertices[v][0];
            vertices[8 * c + v][0] = r_2d * std::cos(phi_cell * c);
            vertices[8 * c + v][1] = vertices[v][1];
            vertices[8 * c + v][2] = r_2d * std::sin(phi_cell * c);
          }
      }

    // cell connectivity
    std::vector<CellData<3>> cells(5 * n_cells_toroidal);
    for (unsigned int c = 0; c < n_cells_toroidal; ++c)
      {
        for (unsigned int j = 0; j < 2; ++j)
          {
            unsigned int offset = (8 * (c + j)) % (8 * n_cells_toroidal);
            // cell 0 in x-y-plane
            cells[5 * c].vertices[0 + j * 4] = offset + 0;
            cells[5 * c].vertices[1 + j * 4] = offset + 1;
            cells[5 * c].vertices[2 + j * 4] = offset + 2;
            cells[5 * c].vertices[3 + j * 4] = offset + 3;
            // cell 1 in x-y-plane
            cells[5 * c + 1].vertices[0 + j * 4] = offset + 2;
            cells[5 * c + 1].vertices[1 + j * 4] = offset + 3;
            cells[5 * c + 1].vertices[2 + j * 4] = offset + 4;
            cells[5 * c + 1].vertices[3 + j * 4] = offset + 5;
            // cell 2 in x-y-plane
            cells[5 * c + 2].vertices[0 + j * 4] = offset + 4;
            cells[5 * c + 2].vertices[1 + j * 4] = offset + 5;
            cells[5 * c + 2].vertices[2 + j * 4] = offset + 6;
            cells[5 * c + 2].vertices[3 + j * 4] = offset + 7;
            // cell 3 in x-y-plane
            cells[5 * c + 3].vertices[0 + j * 4] = offset + 0;
            cells[5 * c + 3].vertices[1 + j * 4] = offset + 2;
            cells[5 * c + 3].vertices[2 + j * 4] = offset + 6;
            cells[5 * c + 3].vertices[3 + j * 4] = offset + 4;
            // cell 4 in x-y-plane
            cells[5 * c + 4].vertices[0 + j * 4] = offset + 3;
            cells[5 * c + 4].vertices[1 + j * 4] = offset + 1;
            cells[5 * c + 4].vertices[2 + j * 4] = offset + 5;
            cells[5 * c + 4].vertices[3 + j * 4] = offset + 7;
          }

        cells[5 * c].material_id     = 0;
        cells[5 * c + 1].material_id = 0;
        cells[5 * c + 2].material_id = 0;
        cells[5 * c + 3].material_id = 0;
        cells[5 * c + 4].material_id = 0;
      }

    tria.create_triangulation(vertices, cells, SubCellData());

    tria.reset_all_manifolds();
    tria.set_all_manifold_ids(0);

    for (auto &cell : tria.cell_iterators())
      {
        bool cell_at_boundary = false;
        for (unsigned int f = 0; f < GeometryInfo<3>::faces_per_cell; ++f)
          if (cell->at_boundary(f))
            cell_at_boundary = true;
        if (cell_at_boundary == false)
          cell->set_all_manifold_ids(2);
      }
    tria.set_all_manifold_ids_on_boundary(1);
    tria.set_manifold(1, TorusManifold<3>(2, 0.5));
    tria.set_manifold(2,
                      CylindricalManifold<3>(Tensor<1, 3>({0., 1., 0.}),
                                             Point<3>()));
    TransfiniteInterpolationManifold<3> transfinite;
    transfinite.initialize(tria);
    tria.set_manifold(0, transfinite);
  }



  template <int dim, int spacedim>
  void
  general_cell(Triangulation<dim, spacedim> &      tria,
               const std::vector<Point<spacedim>> &vertices,
               const bool                          colorize)
  {
    Assert(vertices.size() == dealii::GeometryInfo<dim>::vertices_per_cell,
           ExcMessage("Wrong number of vertices."));

    // First create a hyper_rectangle and then deform it.
    hyper_cube(tria, 0, 1, colorize);

    typename Triangulation<dim, spacedim>::active_cell_iterator cell =
      tria.begin_active();
    for (unsigned int i = 0; i < GeometryInfo<dim>::vertices_per_cell; ++i)
      cell->vertex(i) = vertices[i];

    // Check that the order of the vertices makes sense, i.e., the volume of the
    // cell is positive.
    Assert(GridTools::volume(tria) > 0.,
           ExcMessage(
             "The volume of the cell is not greater than zero. "
             "This could be due to the wrong ordering of the vertices."));
  }



  template <>
  void parallelogram(Triangulation<3> &,
                     const Point<3> (&/*corners*/)[3],
                     const bool /*colorize*/)
  {
    Assert(false, ExcNotImplemented());
  }

  template <>
  void parallelogram(Triangulation<1> &,
                     const Point<1> (&/*corners*/)[1],
                     const bool /*colorize*/)
  {
    Assert(false, ExcNotImplemented());
  }

  // Implementation for 2D only
  template <>
  void parallelogram(Triangulation<2> &tria,
                     const Point<2> (&corners)[2],
                     const bool colorize)
  {
    Point<2>                    origin;
    std::array<Tensor<1, 2>, 2> edges;
    edges[0] = corners[0];
    edges[1] = corners[1];
    std::vector<unsigned int> subdivisions;
    subdivided_parallelepiped<2, 2>(
      tria, origin, edges, subdivisions, colorize);
  }



  template <int dim>
  void
  parallelepiped(Triangulation<dim> &tria,
                 const Point<dim> (&corners)[dim],
                 const bool colorize)
  {
    unsigned int n_subdivisions[dim];
    for (unsigned int i = 0; i < dim; ++i)
      n_subdivisions[i] = 1;

    // and call the function below
    subdivided_parallelepiped(tria, n_subdivisions, corners, colorize);
  }

  template <int dim>
  void
  subdivided_parallelepiped(Triangulation<dim> &tria,
                            const unsigned int  n_subdivisions,
                            const Point<dim> (&corners)[dim],
                            const bool colorize)
  {
    // Equalize number of subdivisions in each dim-direction, their
    // validity will be checked later
    unsigned int n_subdivisions_[dim];
    for (unsigned int i = 0; i < dim; ++i)
      n_subdivisions_[i] = n_subdivisions;

    // and call the function below
    subdivided_parallelepiped(tria, n_subdivisions_, corners, colorize);
  }

  template <int dim>
  void
  subdivided_parallelepiped(Triangulation<dim> &tria,
#ifndef _MSC_VER
                            const unsigned int (&n_subdivisions)[dim],
#else
                            const unsigned int *n_subdivisions,
#endif
                            const Point<dim> (&corners)[dim],
                            const bool colorize)
  {
    Point<dim>                      origin;
    std::vector<unsigned int>       subdivisions;
    std::array<Tensor<1, dim>, dim> edges;
    for (unsigned int i = 0; i < dim; ++i)
      {
        subdivisions.push_back(n_subdivisions[i]);
        edges[i] = corners[i];
      }

    subdivided_parallelepiped<dim, dim>(
      tria, origin, edges, subdivisions, colorize);
  }

  // Parallelepiped implementation in 1d, 2d, and 3d. @note The
  // implementation in 1d is similar to hyper_rectangle(), and in 2d is
  // similar to parallelogram().
  //
  // The GridReordering::reorder_grid is made use of towards the end of
  // this function. Thus the triangulation is explicitly constructed for
  // all dim here since it is slightly different in that respect
  // (cf. hyper_rectangle(), parallelogram()).
  template <int dim, int spacedim>
  void
  subdivided_parallelepiped(Triangulation<dim, spacedim> &              tria,
                            const Point<spacedim> &                     origin,
                            const std::array<Tensor<1, spacedim>, dim> &edges,
                            const std::vector<unsigned int> &subdivisions,
                            const bool                       colorize)
  {
    std::vector<unsigned int> compute_subdivisions = subdivisions;
    if (compute_subdivisions.size() == 0)
      {
        compute_subdivisions.resize(dim, 1);
      }

    Assert(compute_subdivisions.size() == dim,
           ExcMessage("One subdivision must be provided for each dimension."));
    // check subdivisions
    for (unsigned int i = 0; i < dim; ++i)
      {
        Assert(compute_subdivisions[i] > 0,
               ExcInvalidRepetitions(subdivisions[i]));
        Assert(
          edges[i].norm() > 0,
          ExcMessage(
            "Edges in subdivided_parallelepiped() must not be degenerate."));
      }

    /*
     * Verify that the edge points to the right in 1D, vectors are oriented in
     * a counter clockwise direction in 2D, or form a right handed system in
     * 3D.
     */
    bool twisted_data = false;
    switch (dim)
      {
        case 1:
          {
            twisted_data = (edges[0][0] < 0);
            break;
          }
        case 2:
          {
            if (spacedim == 2) // this check does not make sense otherwise
              {
                const double plane_normal =
                  edges[0][0] * edges[1][1] - edges[0][1] * edges[1][0];
                twisted_data = (plane_normal < 0.0);
              }
            break;
          }
        case 3:
          {
            // Check that the first two vectors are not linear combinations to
            // avoid zero division later on.
            Assert(std::abs(edges[0] * edges[1] /
                              (edges[0].norm() * edges[1].norm()) -
                            1.0) > 1.0e-15,
                   ExcMessage(
                     "Edges in subdivided_parallelepiped() must point in"
                     " different directions."));
            const Tensor<1, spacedim> plane_normal =
              cross_product_3d(edges[0], edges[1]);

            /*
             * Ensure that edges 1, 2, and 3 form a right-handed set of
             * vectors. This works by applying the definition of the dot product
             *
             *     cos(theta) = dot(x, y)/(norm(x)*norm(y))
             *
             * and then, since the normal vector and third edge should both
             * point away from the plane formed by the first two edges, the
             * angle between them must be between 0 and pi/2; hence we just need
             *
             *     0 < dot(x, y).
             */
            twisted_data = (plane_normal * edges[2] < 0.0);
            break;
          }
        default:
          Assert(false, ExcInternalError());
      }
    (void)twisted_data; // make the static analyzer happy
    Assert(
      !twisted_data,
      ExcInvalidInputOrientation(
        "The triangulation you are trying to create will consist of cells"
        " with negative measures. This is usually the result of input data"
        " that does not define a right-handed coordinate system. The usual"
        " fix for this is to ensure that in 1D the given point is to the"
        " right of the origin (or the given edge tensor is positive), in 2D"
        " that the two edges (and their cross product) obey the right-hand"
        " rule (which may usually be done by switching the order of the"
        " points or edge tensors), or in 3D that the edges form a"
        " right-handed coordinate system (which may also be accomplished by"
        " switching the order of the first two points or edge tensors)."));

    // Check corners do not overlap (unique)
    for (unsigned int i = 0; i < dim; ++i)
      for (unsigned int j = i + 1; j < dim; ++j)
        Assert((edges[i] != edges[j]),
               ExcMessage(
                 "Degenerate edges of subdivided_parallelepiped encountered."));

    // Create a list of points
    std::vector<Point<spacedim>> points;

    switch (dim)
      {
        case 1:
          for (unsigned int x = 0; x <= compute_subdivisions[0]; ++x)
            points.push_back(origin + edges[0] / compute_subdivisions[0] * x);
          break;

        case 2:
          for (unsigned int y = 0; y <= compute_subdivisions[1]; ++y)
            for (unsigned int x = 0; x <= compute_subdivisions[0]; ++x)
              points.push_back(origin + edges[0] / compute_subdivisions[0] * x +
                               edges[1] / compute_subdivisions[1] * y);
          break;

        case 3:
          for (unsigned int z = 0; z <= compute_subdivisions[2]; ++z)
            for (unsigned int y = 0; y <= compute_subdivisions[1]; ++y)
              for (unsigned int x = 0; x <= compute_subdivisions[0]; ++x)
                points.push_back(origin +
                                 edges[0] / compute_subdivisions[0] * x +
                                 edges[1] / compute_subdivisions[1] * y +
                                 edges[2] / compute_subdivisions[2] * z);
          break;

        default:
          Assert(false, ExcNotImplemented());
      }

    // Prepare cell data
    unsigned int n_cells = 1;
    for (unsigned int i = 0; i < dim; ++i)
      n_cells *= compute_subdivisions[i];
    std::vector<CellData<dim>> cells(n_cells);

    // Create fixed ordering of
    switch (dim)
      {
        case 1:
          for (unsigned int x = 0; x < compute_subdivisions[0]; ++x)
            {
              cells[x].vertices[0] = x;
              cells[x].vertices[1] = x + 1;

              // wipe material id
              cells[x].material_id = 0;
            }
          break;

        case 2:
          {
            // Shorthand
            const unsigned int n_dy = compute_subdivisions[1];
            const unsigned int n_dx = compute_subdivisions[0];

            for (unsigned int y = 0; y < n_dy; ++y)
              for (unsigned int x = 0; x < n_dx; ++x)
                {
                  const unsigned int c = y * n_dx + x;
                  cells[c].vertices[0] = y * (n_dx + 1) + x;
                  cells[c].vertices[1] = y * (n_dx + 1) + x + 1;
                  cells[c].vertices[2] = (y + 1) * (n_dx + 1) + x;
                  cells[c].vertices[3] = (y + 1) * (n_dx + 1) + x + 1;

                  // wipe material id
                  cells[c].material_id = 0;
                }
          }
          break;

        case 3:
          {
            // Shorthand
            const unsigned int n_dz = compute_subdivisions[2];
            const unsigned int n_dy = compute_subdivisions[1];
            const unsigned int n_dx = compute_subdivisions[0];

            for (unsigned int z = 0; z < n_dz; ++z)
              for (unsigned int y = 0; y < n_dy; ++y)
                for (unsigned int x = 0; x < n_dx; ++x)
                  {
                    const unsigned int c = z * n_dy * n_dx + y * n_dx + x;

                    cells[c].vertices[0] =
                      z * (n_dy + 1) * (n_dx + 1) + y * (n_dx + 1) + x;
                    cells[c].vertices[1] =
                      z * (n_dy + 1) * (n_dx + 1) + y * (n_dx + 1) + x + 1;
                    cells[c].vertices[2] =
                      z * (n_dy + 1) * (n_dx + 1) + (y + 1) * (n_dx + 1) + x;
                    cells[c].vertices[3] = z * (n_dy + 1) * (n_dx + 1) +
                                           (y + 1) * (n_dx + 1) + x + 1;
                    cells[c].vertices[4] =
                      (z + 1) * (n_dy + 1) * (n_dx + 1) + y * (n_dx + 1) + x;
                    cells[c].vertices[5] = (z + 1) * (n_dy + 1) * (n_dx + 1) +
                                           y * (n_dx + 1) + x + 1;
                    cells[c].vertices[6] = (z + 1) * (n_dy + 1) * (n_dx + 1) +
                                           (y + 1) * (n_dx + 1) + x;
                    cells[c].vertices[7] = (z + 1) * (n_dy + 1) * (n_dx + 1) +
                                           (y + 1) * (n_dx + 1) + x + 1;

                    // wipe material id
                    cells[c].material_id = 0;
                  }
            break;
          }

        default:
          Assert(false, ExcNotImplemented());
      }

    // Create triangulation
    // reorder the cells to ensure that they satisfy the convention for
    // edge and face directions
    GridReordering<dim>::reorder_cells(cells, true);
    tria.create_triangulation(points, cells, SubCellData());

    // Finally assign boundary indicators according to hyper_rectangle
    if (colorize)
      {
        typename Triangulation<dim>::active_cell_iterator cell =
                                                            tria.begin_active(),
                                                          endc = tria.end();
        for (; cell != endc; ++cell)
          {
            for (unsigned int face = 0;
                 face < GeometryInfo<dim>::faces_per_cell;
                 ++face)
              {
                if (cell->face(face)->at_boundary())
                  cell->face(face)->set_boundary_id(face);
              }
          }
      }
  }


  template <int dim, int spacedim>
  void
  subdivided_hyper_cube(Triangulation<dim, spacedim> &tria,
                        const unsigned int            repetitions,
                        const double                  left,
                        const double                  right)
  {
    Assert(repetitions >= 1, ExcInvalidRepetitions(repetitions));
    Assert(left < right,
           ExcMessage("Invalid left-to-right bounds of hypercube"));

    Point<dim> p0, p1;
    for (unsigned int i = 0; i < dim; ++i)
      {
        p0[i] = left;
        p1[i] = right;
      }

    std::vector<unsigned int> reps(dim, repetitions);
    subdivided_hyper_rectangle(tria, reps, p0, p1);
  }



  template <int dim, int spacedim>
  void
  subdivided_hyper_rectangle(Triangulation<dim, spacedim> &   tria,
                             const std::vector<unsigned int> &repetitions,
                             const Point<dim> &               p_1,
                             const Point<dim> &               p_2,
                             const bool                       colorize)
  {
    Assert(repetitions.size() == dim, ExcInvalidRepetitionsDimension(dim));

    // First, extend dimensions from dim to spacedim and
    // normalize such that p1 is lower in all coordinate
    // directions. Additional entries will be 0.
    Point<spacedim> p1, p2;
    for (unsigned int i = 0; i < dim; ++i)
      {
        p1(i) = std::min(p_1(i), p_2(i));
        p2(i) = std::max(p_1(i), p_2(i));
      }

    // calculate deltas and validate input
    std::vector<Point<spacedim>> delta(dim);
    for (unsigned int i = 0; i < dim; ++i)
      {
        Assert(repetitions[i] >= 1, ExcInvalidRepetitions(repetitions[i]));

        delta[i][i] = (p2[i] - p1[i]) / repetitions[i];
        Assert(
          delta[i][i] > 0.0,
          ExcMessage(
            "The first dim entries of coordinates of p1 and p2 need to be different."));
      }

    // then generate the points
    std::vector<Point<spacedim>> points;
    switch (dim)
      {
        case 1:
          for (unsigned int x = 0; x <= repetitions[0]; ++x)
            points.push_back(p1 + x * delta[0]);
          break;

        case 2:
          for (unsigned int y = 0; y <= repetitions[1]; ++y)
            for (unsigned int x = 0; x <= repetitions[0]; ++x)
              points.push_back(p1 + x * delta[0] + y * delta[1]);
          break;

        case 3:
          for (unsigned int z = 0; z <= repetitions[2]; ++z)
            for (unsigned int y = 0; y <= repetitions[1]; ++y)
              for (unsigned int x = 0; x <= repetitions[0]; ++x)
                points.push_back(p1 + x * delta[0] + y * delta[1] +
                                 z * delta[2]);
          break;

        default:
          Assert(false, ExcNotImplemented());
      }

    // next create the cells
    std::vector<CellData<dim>> cells;
    switch (dim)
      {
        case 1:
          {
            cells.resize(repetitions[0]);
            for (unsigned int x = 0; x < repetitions[0]; ++x)
              {
                cells[x].vertices[0] = x;
                cells[x].vertices[1] = x + 1;
                cells[x].material_id = 0;
              }
            break;
          }

        case 2:
          {
            cells.resize(repetitions[1] * repetitions[0]);
            for (unsigned int y = 0; y < repetitions[1]; ++y)
              for (unsigned int x = 0; x < repetitions[0]; ++x)
                {
                  const unsigned int c = x + y * repetitions[0];
                  cells[c].vertices[0] = y * (repetitions[0] + 1) + x;
                  cells[c].vertices[1] = y * (repetitions[0] + 1) + x + 1;
                  cells[c].vertices[2] = (y + 1) * (repetitions[0] + 1) + x;
                  cells[c].vertices[3] = (y + 1) * (repetitions[0] + 1) + x + 1;
                  cells[c].material_id = 0;
                }
            break;
          }

        case 3:
          {
            const unsigned int n_x = (repetitions[0] + 1);
            const unsigned int n_xy =
              (repetitions[0] + 1) * (repetitions[1] + 1);

            cells.resize(repetitions[2] * repetitions[1] * repetitions[0]);
            for (unsigned int z = 0; z < repetitions[2]; ++z)
              for (unsigned int y = 0; y < repetitions[1]; ++y)
                for (unsigned int x = 0; x < repetitions[0]; ++x)
                  {
                    const unsigned int c = x + y * repetitions[0] +
                                           z * repetitions[0] * repetitions[1];
                    cells[c].vertices[0] = z * n_xy + y * n_x + x;
                    cells[c].vertices[1] = z * n_xy + y * n_x + x + 1;
                    cells[c].vertices[2] = z * n_xy + (y + 1) * n_x + x;
                    cells[c].vertices[3] = z * n_xy + (y + 1) * n_x + x + 1;
                    cells[c].vertices[4] = (z + 1) * n_xy + y * n_x + x;
                    cells[c].vertices[5] = (z + 1) * n_xy + y * n_x + x + 1;
                    cells[c].vertices[6] = (z + 1) * n_xy + (y + 1) * n_x + x;
                    cells[c].vertices[7] =
                      (z + 1) * n_xy + (y + 1) * n_x + x + 1;
                    cells[c].material_id = 0;
                  }
            break;
          }

        default:
          Assert(false, ExcNotImplemented());
      }

    tria.create_triangulation(points, cells, SubCellData());

    if (colorize)
      {
        // to colorize, run through all
        // faces of all cells and set
        // boundary indicator to the
        // correct value if it was 0.

        // use a large epsilon to
        // compare numbers to avoid
        // roundoff problems.
        double epsilon = 10;
        for (unsigned int i = 0; i < dim; ++i)
          epsilon = std::min(epsilon, 0.01 * delta[i][i]);
        Assert(epsilon > 0,
               ExcMessage(
                 "The distance between corner points must be positive."))

          // actual code is external since
          // 1-D is different from 2/3D.
          colorize_subdivided_hyper_rectangle(tria, p1, p2, epsilon);
      }
  }



  template <int dim>
  void
  subdivided_hyper_rectangle(Triangulation<dim> &                    tria,
                             const std::vector<std::vector<double>> &step_sz,
                             const Point<dim> &                      p_1,
                             const Point<dim> &                      p_2,
                             const bool                              colorize)
  {
    Assert(step_sz.size() == dim, ExcInvalidRepetitionsDimension(dim));

    // First, normalize input such that
    // p1 is lower in all coordinate
    // directions and check the consistency of
    // step sizes, i.e. that they all
    // add up to the sizes specified by
    // p_1 and p_2
    Point<dim>                       p1(p_1);
    Point<dim>                       p2(p_2);
    std::vector<std::vector<double>> step_sizes(step_sz);

    for (unsigned int i = 0; i < dim; ++i)
      {
        if (p1(i) > p2(i))
          {
            std::swap(p1(i), p2(i));
            std::reverse(step_sizes[i].begin(), step_sizes[i].end());
          }

        double x = 0;
        for (unsigned int j = 0; j < step_sizes.at(i).size(); j++)
          x += step_sizes[i][j];
        Assert(std::fabs(x - (p2(i) - p1(i))) <= 1e-12 * std::fabs(x),
               ExcMessage(
                 "The sequence of step sizes in coordinate direction " +
                 Utilities::int_to_string(i) +
                 " must be equal to the distance of the two given "
                 "points in this coordinate direction."));
      }


    // then generate the necessary
    // points
    std::vector<Point<dim>> points;
    switch (dim)
      {
        case 1:
          {
            double x = 0;
            for (unsigned int i = 0;; ++i)
              {
                points.push_back(Point<dim>(p1[0] + x));

                // form partial sums. in
                // the last run through
                // avoid accessing
                // non-existent values
                // and exit early instead
                if (i == step_sizes[0].size())
                  break;

                x += step_sizes[0][i];
              }
            break;
          }

        case 2:
          {
            double y = 0;
            for (unsigned int j = 0;; ++j)
              {
                double x = 0;
                for (unsigned int i = 0;; ++i)
                  {
                    points.push_back(Point<dim>(p1[0] + x, p1[1] + y));
                    if (i == step_sizes[0].size())
                      break;

                    x += step_sizes[0][i];
                  }

                if (j == step_sizes[1].size())
                  break;

                y += step_sizes[1][j];
              }
            break;
          }
        case 3:
          {
            double z = 0;
            for (unsigned int k = 0;; ++k)
              {
                double y = 0;
                for (unsigned int j = 0;; ++j)
                  {
                    double x = 0;
                    for (unsigned int i = 0;; ++i)
                      {
                        points.push_back(
                          Point<dim>(p1[0] + x, p1[1] + y, p1[2] + z));
                        if (i == step_sizes[0].size())
                          break;

                        x += step_sizes[0][i];
                      }

                    if (j == step_sizes[1].size())
                      break;

                    y += step_sizes[1][j];
                  }

                if (k == step_sizes[2].size())
                  break;

                z += step_sizes[2][k];
              }
            break;
          }

        default:
          Assert(false, ExcNotImplemented());
      }

    // next create the cells
    // Prepare cell data
    std::vector<CellData<dim>> cells;
    switch (dim)
      {
        case 1:
          {
            cells.resize(step_sizes[0].size());
            for (unsigned int x = 0; x < step_sizes[0].size(); ++x)
              {
                cells[x].vertices[0] = x;
                cells[x].vertices[1] = x + 1;
                cells[x].material_id = 0;
              }
            break;
          }

        case 2:
          {
            cells.resize(step_sizes[1].size() * step_sizes[0].size());
            for (unsigned int y = 0; y < step_sizes[1].size(); ++y)
              for (unsigned int x = 0; x < step_sizes[0].size(); ++x)
                {
                  const unsigned int c = x + y * step_sizes[0].size();
                  cells[c].vertices[0] = y * (step_sizes[0].size() + 1) + x;
                  cells[c].vertices[1] = y * (step_sizes[0].size() + 1) + x + 1;
                  cells[c].vertices[2] =
                    (y + 1) * (step_sizes[0].size() + 1) + x;
                  cells[c].vertices[3] =
                    (y + 1) * (step_sizes[0].size() + 1) + x + 1;
                  cells[c].material_id = 0;
                }
            break;
          }

        case 3:
          {
            const unsigned int n_x = (step_sizes[0].size() + 1);
            const unsigned int n_xy =
              (step_sizes[0].size() + 1) * (step_sizes[1].size() + 1);

            cells.resize(step_sizes[2].size() * step_sizes[1].size() *
                         step_sizes[0].size());
            for (unsigned int z = 0; z < step_sizes[2].size(); ++z)
              for (unsigned int y = 0; y < step_sizes[1].size(); ++y)
                for (unsigned int x = 0; x < step_sizes[0].size(); ++x)
                  {
                    const unsigned int c =
                      x + y * step_sizes[0].size() +
                      z * step_sizes[0].size() * step_sizes[1].size();
                    cells[c].vertices[0] = z * n_xy + y * n_x + x;
                    cells[c].vertices[1] = z * n_xy + y * n_x + x + 1;
                    cells[c].vertices[2] = z * n_xy + (y + 1) * n_x + x;
                    cells[c].vertices[3] = z * n_xy + (y + 1) * n_x + x + 1;
                    cells[c].vertices[4] = (z + 1) * n_xy + y * n_x + x;
                    cells[c].vertices[5] = (z + 1) * n_xy + y * n_x + x + 1;
                    cells[c].vertices[6] = (z + 1) * n_xy + (y + 1) * n_x + x;
                    cells[c].vertices[7] =
                      (z + 1) * n_xy + (y + 1) * n_x + x + 1;
                    cells[c].material_id = 0;
                  }
            break;
          }

        default:
          Assert(false, ExcNotImplemented());
      }

    tria.create_triangulation(points, cells, SubCellData());

    if (colorize)
      {
        // to colorize, run through all
        // faces of all cells and set
        // boundary indicator to the
        // correct value if it was 0.

        // use a large epsilon to
        // compare numbers to avoid
        // roundoff problems.
        double min_size =
          *std::min_element(step_sizes[0].begin(), step_sizes[0].end());
        for (unsigned int i = 1; i < dim; ++i)
          min_size = std::min(min_size,
                              *std::min_element(step_sizes[i].begin(),
                                                step_sizes[i].end()));
        const double epsilon = 0.01 * min_size;

        // actual code is external since
        // 1-D is different from 2/3D.
        colorize_subdivided_hyper_rectangle(tria, p1, p2, epsilon);
      }
  }



  template <>
  void
    subdivided_hyper_rectangle(Triangulation<1> &                      tria,
                               const std::vector<std::vector<double>> &spacing,
                               const Point<1> &                        p,
                               const Table<1, types::material_id> &material_id,
                               const bool                          colorize)
  {
    Assert(spacing.size() == 1, ExcInvalidRepetitionsDimension(1));

    const unsigned int n_cells = material_id.size(0);

    Assert(spacing[0].size() == n_cells, ExcInvalidRepetitionsDimension(1));

    double delta = std::numeric_limits<double>::max();
    for (unsigned int i = 0; i < n_cells; i++)
      {
        Assert(spacing[0][i] >= 0, ExcInvalidRepetitions(-1));
        delta = std::min(delta, spacing[0][i]);
      }

    // generate the necessary points
    std::vector<Point<1>> points;
    double                ax = p[0];
    for (unsigned int x = 0; x <= n_cells; ++x)
      {
        points.emplace_back(ax);
        if (x < n_cells)
          ax += spacing[0][x];
      }
    // create the cells
    unsigned int n_val_cells = 0;
    for (unsigned int i = 0; i < n_cells; i++)
      if (material_id[i] != numbers::invalid_material_id)
        n_val_cells++;

    std::vector<CellData<1>> cells(n_val_cells);
    unsigned int             id = 0;
    for (unsigned int x = 0; x < n_cells; ++x)
      if (material_id[x] != numbers::invalid_material_id)
        {
          cells[id].vertices[0] = x;
          cells[id].vertices[1] = x + 1;
          cells[id].material_id = material_id[x];
          id++;
        }
    // create triangulation
    SubCellData t;
    GridTools::delete_unused_vertices(points, cells, t);

    tria.create_triangulation(points, cells, t);

    // set boundary indicator
    if (colorize)
      Assert(false, ExcNotImplemented());
  }


  template <>
  void
    subdivided_hyper_rectangle(Triangulation<2> &                      tria,
                               const std::vector<std::vector<double>> &spacing,
                               const Point<2> &                        p,
                               const Table<2, types::material_id> &material_id,
                               const bool                          colorize)
  {
    Assert(spacing.size() == 2, ExcInvalidRepetitionsDimension(2));

    std::vector<unsigned int> repetitions(2);
    unsigned int              n_cells = 1;
    double                    delta   = std::numeric_limits<double>::max();
    for (unsigned int i = 0; i < 2; i++)
      {
        repetitions[i] = spacing[i].size();
        n_cells *= repetitions[i];
        for (unsigned int j = 0; j < repetitions[i]; j++)
          {
            Assert(spacing[i][j] >= 0, ExcInvalidRepetitions(-1));
            delta = std::min(delta, spacing[i][j]);
          }
        Assert(material_id.size(i) == repetitions[i],
               ExcInvalidRepetitionsDimension(i));
      }

    // generate the necessary points
    std::vector<Point<2>> points;
    double                ay = p[1];
    for (unsigned int y = 0; y <= repetitions[1]; ++y)
      {
        double ax = p[0];
        for (unsigned int x = 0; x <= repetitions[0]; ++x)
          {
            points.emplace_back(ax, ay);
            if (x < repetitions[0])
              ax += spacing[0][x];
          }
        if (y < repetitions[1])
          ay += spacing[1][y];
      }

    // create the cells
    unsigned int n_val_cells = 0;
    for (unsigned int i = 0; i < material_id.size(0); i++)
      for (unsigned int j = 0; j < material_id.size(1); j++)
        if (material_id[i][j] != numbers::invalid_material_id)
          n_val_cells++;

    std::vector<CellData<2>> cells(n_val_cells);
    unsigned int             id = 0;
    for (unsigned int y = 0; y < repetitions[1]; ++y)
      for (unsigned int x = 0; x < repetitions[0]; ++x)
        if (material_id[x][y] != numbers::invalid_material_id)
          {
            cells[id].vertices[0] = y * (repetitions[0] + 1) + x;
            cells[id].vertices[1] = y * (repetitions[0] + 1) + x + 1;
            cells[id].vertices[2] = (y + 1) * (repetitions[0] + 1) + x;
            cells[id].vertices[3] = (y + 1) * (repetitions[0] + 1) + x + 1;
            cells[id].material_id = material_id[x][y];
            id++;
          }

    // create triangulation
    SubCellData t;
    GridTools::delete_unused_vertices(points, cells, t);

    tria.create_triangulation(points, cells, t);

    // set boundary indicator
    if (colorize)
      {
        double                          eps  = 0.01 * delta;
        Triangulation<2>::cell_iterator cell = tria.begin(), endc = tria.end();
        for (; cell != endc; ++cell)
          {
            Point<2> cell_center = cell->center();
            for (unsigned int f = 0; f < GeometryInfo<2>::faces_per_cell; ++f)
              if (cell->face(f)->boundary_id() == 0)
                {
                  Point<2> face_center = cell->face(f)->center();
                  for (unsigned int i = 0; i < 2; ++i)
                    {
                      if (face_center[i] < cell_center[i] - eps)
                        cell->face(f)->set_boundary_id(i * 2);
                      if (face_center[i] > cell_center[i] + eps)
                        cell->face(f)->set_boundary_id(i * 2 + 1);
                    }
                }
          }
      }
  }


  template <>
  void
    subdivided_hyper_rectangle(Triangulation<3> &                      tria,
                               const std::vector<std::vector<double>> &spacing,
                               const Point<3> &                        p,
                               const Table<3, types::material_id> &material_id,
                               const bool                          colorize)
  {
    const unsigned int dim = 3;

    Assert(spacing.size() == dim, ExcInvalidRepetitionsDimension(dim));

    std::vector<unsigned int> repetitions(dim);
    unsigned int              n_cells = 1;
    double                    delta   = std::numeric_limits<double>::max();
    for (unsigned int i = 0; i < dim; i++)
      {
        repetitions[i] = spacing[i].size();
        n_cells *= repetitions[i];
        for (unsigned int j = 0; j < repetitions[i]; j++)
          {
            Assert(spacing[i][j] >= 0, ExcInvalidRepetitions(-1));
            delta = std::min(delta, spacing[i][j]);
          }
        Assert(material_id.size(i) == repetitions[i],
               ExcInvalidRepetitionsDimension(i));
      }

    // generate the necessary points
    std::vector<Point<dim>> points;
    double                  az = p[2];
    for (unsigned int z = 0; z <= repetitions[2]; ++z)
      {
        double ay = p[1];
        for (unsigned int y = 0; y <= repetitions[1]; ++y)
          {
            double ax = p[0];
            for (unsigned int x = 0; x <= repetitions[0]; ++x)
              {
                points.emplace_back(ax, ay, az);
                if (x < repetitions[0])
                  ax += spacing[0][x];
              }
            if (y < repetitions[1])
              ay += spacing[1][y];
          }
        if (z < repetitions[2])
          az += spacing[2][z];
      }

    // create the cells
    unsigned int n_val_cells = 0;
    for (unsigned int i = 0; i < material_id.size(0); i++)
      for (unsigned int j = 0; j < material_id.size(1); j++)
        for (unsigned int k = 0; k < material_id.size(2); k++)
          if (material_id[i][j][k] != numbers::invalid_material_id)
            n_val_cells++;

    std::vector<CellData<dim>> cells(n_val_cells);
    unsigned int               id  = 0;
    const unsigned int         n_x = (repetitions[0] + 1);
    const unsigned int n_xy = (repetitions[0] + 1) * (repetitions[1] + 1);
    for (unsigned int z = 0; z < repetitions[2]; ++z)
      for (unsigned int y = 0; y < repetitions[1]; ++y)
        for (unsigned int x = 0; x < repetitions[0]; ++x)
          if (material_id[x][y][z] != numbers::invalid_material_id)
            {
              cells[id].vertices[0] = z * n_xy + y * n_x + x;
              cells[id].vertices[1] = z * n_xy + y * n_x + x + 1;
              cells[id].vertices[2] = z * n_xy + (y + 1) * n_x + x;
              cells[id].vertices[3] = z * n_xy + (y + 1) * n_x + x + 1;
              cells[id].vertices[4] = (z + 1) * n_xy + y * n_x + x;
              cells[id].vertices[5] = (z + 1) * n_xy + y * n_x + x + 1;
              cells[id].vertices[6] = (z + 1) * n_xy + (y + 1) * n_x + x;
              cells[id].vertices[7] = (z + 1) * n_xy + (y + 1) * n_x + x + 1;
              cells[id].material_id = material_id[x][y][z];
              id++;
            }

    // create triangulation
    SubCellData t;
    GridTools::delete_unused_vertices(points, cells, t);

    tria.create_triangulation(points, cells, t);

    // set boundary indicator
    if (colorize)
      {
        double                            eps  = 0.01 * delta;
        Triangulation<dim>::cell_iterator cell = tria.begin(),
                                          endc = tria.end();
        for (; cell != endc; ++cell)
          {
            Point<dim> cell_center = cell->center();
            for (unsigned int f = 0; f < GeometryInfo<dim>::faces_per_cell; ++f)
              if (cell->face(f)->boundary_id() == 0)
                {
                  Point<dim> face_center = cell->face(f)->center();
                  for (unsigned int i = 0; i < dim; ++i)
                    {
                      if (face_center[i] < cell_center[i] - eps)
                        cell->face(f)->set_boundary_id(i * 2);
                      if (face_center[i] > cell_center[i] + eps)
                        cell->face(f)->set_boundary_id(i * 2 + 1);
                    }
                }
          }
      }
  }

  template <int dim, int spacedim>
  void
  cheese(Triangulation<dim, spacedim> &   tria,
         const std::vector<unsigned int> &holes)
  {
    AssertDimension(holes.size(), dim);
    // The corner points of the first cell. If there is a desire at
    // some point to change the geometry of the cells, they can be
    // made an argument to the function.

    Point<spacedim> p1;
    Point<spacedim> p2;
    for (unsigned int d = 0; d < dim; ++d)
      p2(d) = 1.;

    // then check that all repetitions
    // are >= 1, and calculate deltas
    // convert repetitions from double
    // to int by taking the ceiling.
    std::vector<Point<spacedim>> delta(dim);
    unsigned int                 repetitions[dim];
    for (unsigned int i = 0; i < dim; ++i)
      {
        Assert(holes[i] >= 1,
               ExcMessage("At least one hole needed in each direction"));
        repetitions[i] = 2 * holes[i] + 1;
        delta[i][i]    = (p2[i] - p1[i]);
      }

    // then generate the necessary
    // points
    std::vector<Point<spacedim>> points;
    switch (dim)
      {
        case 1:
          for (unsigned int x = 0; x <= repetitions[0]; ++x)
            points.push_back(p1 + x * delta[0]);
          break;

        case 2:
          for (unsigned int y = 0; y <= repetitions[1]; ++y)
            for (unsigned int x = 0; x <= repetitions[0]; ++x)
              points.push_back(p1 + x * delta[0] + y * delta[1]);
          break;

        case 3:
          for (unsigned int z = 0; z <= repetitions[2]; ++z)
            for (unsigned int y = 0; y <= repetitions[1]; ++y)
              for (unsigned int x = 0; x <= repetitions[0]; ++x)
                points.push_back(p1 + x * delta[0] + y * delta[1] +
                                 z * delta[2]);
          break;

        default:
          Assert(false, ExcNotImplemented());
      }

    // next create the cells
    // Prepare cell data
    std::vector<CellData<dim>> cells;
    switch (dim)
      {
        case 2:
          {
            cells.resize(repetitions[1] * repetitions[0] - holes[1] * holes[0]);
            unsigned int c = 0;
            for (unsigned int y = 0; y < repetitions[1]; ++y)
              for (unsigned int x = 0; x < repetitions[0]; ++x)
                {
                  if ((x % 2 == 1) && (y % 2 == 1))
                    continue;
                  Assert(c < cells.size(), ExcInternalError());
                  cells[c].vertices[0] = y * (repetitions[0] + 1) + x;
                  cells[c].vertices[1] = y * (repetitions[0] + 1) + x + 1;
                  cells[c].vertices[2] = (y + 1) * (repetitions[0] + 1) + x;
                  cells[c].vertices[3] = (y + 1) * (repetitions[0] + 1) + x + 1;
                  cells[c].material_id = 0;
                  ++c;
                }
            break;
          }

        case 3:
          {
            const unsigned int n_x = (repetitions[0] + 1);
            const unsigned int n_xy =
              (repetitions[0] + 1) * (repetitions[1] + 1);

            cells.resize(repetitions[2] * repetitions[1] * repetitions[0]);

            unsigned int c = 0;
            for (unsigned int z = 0; z < repetitions[2]; ++z)
              for (unsigned int y = 0; y < repetitions[1]; ++y)
                for (unsigned int x = 0; x < repetitions[0]; ++x)
                  {
                    Assert(c < cells.size(), ExcInternalError());
                    cells[c].vertices[0] = z * n_xy + y * n_x + x;
                    cells[c].vertices[1] = z * n_xy + y * n_x + x + 1;
                    cells[c].vertices[2] = z * n_xy + (y + 1) * n_x + x;
                    cells[c].vertices[3] = z * n_xy + (y + 1) * n_x + x + 1;
                    cells[c].vertices[4] = (z + 1) * n_xy + y * n_x + x;
                    cells[c].vertices[5] = (z + 1) * n_xy + y * n_x + x + 1;
                    cells[c].vertices[6] = (z + 1) * n_xy + (y + 1) * n_x + x;
                    cells[c].vertices[7] =
                      (z + 1) * n_xy + (y + 1) * n_x + x + 1;
                    cells[c].material_id = 0;
                    ++c;
                  }
            break;
          }

        default:
          Assert(false, ExcNotImplemented());
      }

    tria.create_triangulation(points, cells, SubCellData());
  }



  template <>
  void plate_with_a_hole(Triangulation<1> & /*tria*/,
                         const double /*inner_radius*/,
                         const double /*outer_radius*/,
                         const double /*pad_bottom*/,
                         const double /*pad_top*/,
                         const double /*pad_left*/,
                         const double /*pad_right*/,
                         const Point<1> /*center*/,
                         const types::manifold_id /*polar_manifold_id*/,
                         const types::manifold_id /*tfi_manifold_id*/,
                         const double /*L*/,
                         const unsigned int /*n_slices*/,
                         const bool /*colorize*/)
  {
    Assert(false, ExcNotImplemented());
  }



  template <>
  void channel_with_cylinder(Triangulation<1> & /*tria*/,
                             const double /*shell_region_width*/,
                             const unsigned int /*n_shells*/,
                             const double /*skewness*/,
                             const bool /*colorize*/)
  {
    Assert(false, ExcNotImplemented());
  }



  namespace internal
  {
    // helper function to check if point is in 2D box
    bool inline point_in_2d_box(const Point<2> &p,
                                const Point<2> &c,
                                const double    radius)
    {
      return (std::abs(p[0] - c[0]) < radius) &&
             (std::abs(p[1] - c[1]) < radius);
    }



    // Find the minimal distance between two vertices. This is useful for
    // computing a tolerance for merging vertices in
    // GridTools::merge_triangulations.
    template <int dim, int spacedim>
    double
    minimal_vertex_distance(const Triangulation<dim, spacedim> &triangulation)
    {
      double length = std::numeric_limits<double>::max();
      for (const auto &cell : triangulation.active_cell_iterators())
        for (unsigned int n = 0; n < GeometryInfo<dim>::lines_per_cell; ++n)
          length = std::min(length, cell->line(n)->diameter());
      return length;
    }
  } // namespace internal



  template <>
  void plate_with_a_hole(Triangulation<2> &       tria,
                         const double             inner_radius,
                         const double             outer_radius,
                         const double             pad_bottom,
                         const double             pad_top,
                         const double             pad_left,
                         const double             pad_right,
                         const Point<2>           new_center,
                         const types::manifold_id polar_manifold_id,
                         const types::manifold_id tfi_manifold_id,
                         const double             L,
                         const unsigned int /*n_slices*/,
                         const bool colorize)
  {
    const bool with_padding =
      pad_bottom > 0 || pad_top > 0 || pad_left > 0 || pad_right > 0;

    Assert(pad_bottom >= 0., ExcMessage("Negative bottom padding."));
    Assert(pad_top >= 0., ExcMessage("Negative top padding."));
    Assert(pad_left >= 0., ExcMessage("Negative left padding."));
    Assert(pad_right >= 0., ExcMessage("Negative right padding."));

    const Point<2> center;

    auto min_line_length = [](const Triangulation<2> &tria) -> double {
      double length = std::numeric_limits<double>::max();
      for (const auto &cell : tria.active_cell_iterators())
        for (unsigned int n = 0; n < GeometryInfo<2>::lines_per_cell; ++n)
          length = std::min(length, cell->line(n)->diameter());
      return length;
    };

    // start by setting up the cylinder triangulation
    Triangulation<2>  cylinder_tria_maybe;
    Triangulation<2> &cylinder_tria = with_padding ? cylinder_tria_maybe : tria;
    GridGenerator::hyper_cube_with_cylindrical_hole(cylinder_tria,
                                                    inner_radius,
                                                    outer_radius,
                                                    L,
                                                    /*repetitions*/ 1,
                                                    colorize);

    // we will deal with face manifold ids after we merge triangulations
    for (const auto &cell : cylinder_tria.active_cell_iterators())
      cell->set_manifold_id(tfi_manifold_id);

    const Point<2> bl(-outer_radius - pad_left, -outer_radius - pad_bottom);
    const Point<2> tr(outer_radius + pad_right, outer_radius + pad_top);
    if (with_padding)
      {
        // hyper_cube_with_cylindrical_hole will have 2 cells along
        // each face, so he element size is outer_radius

        auto add_sizes = [](std::vector<double> &step_sizes,
                            const double         padding,
                            const double         h) -> void {
          // use std::round instead of std::ceil to improve aspect ratio
          // in case padding is only slightly larger than h.
          const auto rounded =
            static_cast<unsigned int>(std::round(padding / h));
          // in case padding is much smaller than h, make sure we
          // have at least 1 element
          const unsigned int num = (padding > 0. && rounded == 0) ? 1 : rounded;
          for (unsigned int i = 0; i < num; ++i)
            step_sizes.push_back(padding / num);
        };

        std::vector<std::vector<double>> step_sizes(2);
        // x-coord
        // left:
        add_sizes(step_sizes[0], pad_left, outer_radius);
        // center
        step_sizes[0].push_back(outer_radius);
        step_sizes[0].push_back(outer_radius);
        // right
        add_sizes(step_sizes[0], pad_right, outer_radius);
        // y-coord
        //   bottom
        add_sizes(step_sizes[1], pad_bottom, outer_radius);
        //   center
        step_sizes[1].push_back(outer_radius);
        step_sizes[1].push_back(outer_radius);
        //   top
        add_sizes(step_sizes[1], pad_top, outer_radius);

        // now create bulk
        Triangulation<2> bulk_tria;
        GridGenerator::subdivided_hyper_rectangle(
          bulk_tria, step_sizes, bl, tr, colorize);

        // now remove cells reserved from the cylindrical hole
        std::set<Triangulation<2>::active_cell_iterator> cells_to_remove;
        for (const auto &cell : bulk_tria.active_cell_iterators())
          if (internal::point_in_2d_box(cell->center(), center, outer_radius))
            cells_to_remove.insert(cell);

        Triangulation<2> tria_without_cylinder;
        GridGenerator::create_triangulation_with_removed_cells(
          bulk_tria, cells_to_remove, tria_without_cylinder);

        const double tolerance =
          std::min(min_line_length(tria_without_cylinder),
                   min_line_length(cylinder_tria)) /
          2.0;

        GridGenerator::merge_triangulations(tria_without_cylinder,
                                            cylinder_tria,
                                            tria,
                                            tolerance);
      }

    // now set manifold ids:
    for (const auto &cell : tria.active_cell_iterators())
      {
        // set all non-boundary manifold ids on the cells that came from the
        // grid around the cylinder to the new TFI manifold id.
        if (cell->manifold_id() == tfi_manifold_id)
          {
            for (unsigned int face_n = 0;
                 face_n < GeometryInfo<2>::faces_per_cell;
                 ++face_n)
              {
                const auto &face = cell->face(face_n);
                if (face->at_boundary() &&
                    internal::point_in_2d_box(face->center(),
                                              center,
                                              outer_radius))
                  face->set_manifold_id(polar_manifold_id);
                else
                  face->set_manifold_id(tfi_manifold_id);
              }
          }
        else
          {
            // ensure that all other manifold ids (including the faces
            // opposite the cylinder) are set to the flat id
            cell->set_all_manifold_ids(numbers::flat_manifold_id);
          }
      }

    static constexpr double tol =
      std::numeric_limits<double>::epsilon() * 10000;
    if (colorize)
      for (const auto &cell : tria.active_cell_iterators())
        for (unsigned int face_n = 0; face_n < GeometryInfo<2>::faces_per_cell;
             ++face_n)
          {
            const auto face = cell->face(face_n);
            if (face->at_boundary())
              {
                const Point<2> center = face->center();
                // left side
                if (std::abs(center[0] - bl[0]) < tol * std::abs(bl[0]))
                  face->set_boundary_id(0);
                // right side
                else if (std::abs(center[0] - tr[0]) < tol * std::abs(tr[0]))
                  face->set_boundary_id(1);
                // bottom
                else if (std::abs(center[1] - bl[1]) < tol * std::abs(bl[1]))
                  face->set_boundary_id(2);
                // top
                else if (std::abs(center[1] - tr[1]) < tol * std::abs(tr[1]))
                  face->set_boundary_id(3);
                // cylinder boundary
                else
                  {
                    Assert(cell->manifold_id() == tfi_manifold_id,
                           ExcInternalError());
                    face->set_boundary_id(4);
                  }
              }
          }

    // move to the new center
    GridTools::shift(new_center, tria);

    PolarManifold<2> polar_manifold(new_center);
    tria.set_manifold(polar_manifold_id, polar_manifold);
    TransfiniteInterpolationManifold<2> inner_manifold;
    inner_manifold.initialize(tria);
    tria.set_manifold(tfi_manifold_id, inner_manifold);
  }



  template <>
  void plate_with_a_hole(Triangulation<3> &       tria,
                         const double             inner_radius,
                         const double             outer_radius,
                         const double             pad_bottom,
                         const double             pad_top,
                         const double             pad_left,
                         const double             pad_right,
                         const Point<3>           new_center,
                         const types::manifold_id polar_manifold_id,
                         const types::manifold_id tfi_manifold_id,
                         const double             L,
                         const unsigned int       n_slices,
                         const bool               colorize)
  {
    Triangulation<2> tria_2;
    plate_with_a_hole(tria_2,
                      inner_radius,
                      outer_radius,
                      pad_bottom,
                      pad_top,
                      pad_left,
                      pad_right,
                      Point<2>(new_center[0], new_center[1]),
                      polar_manifold_id,
                      tfi_manifold_id,
                      L,
                      n_slices,
                      colorize);

    // extrude to 3D
    extrude_triangulation(tria_2, n_slices, L, tria, true);

    // shift in Z direction to match specified center
    GridTools::shift(Point<3>(0, 0, new_center[2] - L / 2.), tria);

    // set up the new manifolds
    const Tensor<1, 3>           direction{{0.0, 0.0, 1.0}};
    const CylindricalManifold<3> cylindrical_manifold(
      direction, /*axial_point*/ new_center);
    TransfiniteInterpolationManifold<3> inner_manifold;
    inner_manifold.initialize(tria);
    tria.set_manifold(polar_manifold_id, cylindrical_manifold);
    tria.set_manifold(tfi_manifold_id, inner_manifold);
  }



  template <>
  void channel_with_cylinder(Triangulation<2> & tria,
                             const double       shell_region_width,
                             const unsigned int n_shells,
                             const double       skewness,
                             const bool         colorize)
  {
    Assert(0.0 <= shell_region_width && shell_region_width < 0.05,
           ExcMessage("The width of the shell region must be less than 0.05 "
                      "(and preferably close to 0.03)"));
    const types::manifold_id polar_manifold_id = 0;
    const types::manifold_id tfi_manifold_id   = 1;

    // We begin by setting up a grid that is 4 by 22 cells. While not
    // squares, these have pretty good aspect ratios.
    Triangulation<2> bulk_tria;
    GridGenerator::subdivided_hyper_rectangle(bulk_tria,
                                              {22u, 4u},
                                              Point<2>(0.0, 0.0),
                                              Point<2>(2.2, 0.41));
    // bulk_tria now looks like this:
    //
    //   +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    //   |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |
    //   +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    //   |  |XX|XX|  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |
    //   +--+--O--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    //   |  |XX|XX|  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |
    //   +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    //   |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |
    //   +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    //
    // Note that these cells are not quite squares: they are all 0.1 by
    // 0.1025.
    //
    // The next step is to remove the cells marked with XXs: we will place
    // the grid around the cylinder there later. The next loop does two
    // things:
    // 1. Determines which cells need to be removed from the Triangulation
    //    (i.e., find the cells marked with XX in the picture).
    // 2. Finds the location of the vertex marked with 'O' and uses that to
    //    calculate the shift vector for aligning cylinder_tria with
    //    tria_without_cylinder.
    std::set<Triangulation<2>::active_cell_iterator> cells_to_remove;
    Tensor<1, 2> cylinder_triangulation_offset;
    for (const auto &cell : bulk_tria.active_cell_iterators())
      {
        if ((cell->center() - Point<2>(0.2, 0.2)).norm() < 0.15)
          cells_to_remove.insert(cell);

        if (cylinder_triangulation_offset == Tensor<1, 2>())
          {
            for (unsigned int vertex_n = 0;
                 vertex_n < GeometryInfo<2>::vertices_per_cell;
                 ++vertex_n)
              if (cell->vertex(vertex_n) == Point<2>())
                {
                  // cylinder_tria is centered at zero, so we need to
                  // shift it up and to the right by two cells:
                  cylinder_triangulation_offset =
                    2.0 * (cell->vertex(3) - Point<2>());
                  break;
                }
          }
      }
    Triangulation<2> tria_without_cylinder;
    GridGenerator::create_triangulation_with_removed_cells(
      bulk_tria, cells_to_remove, tria_without_cylinder);

    // set up the cylinder triangulation. Note that this function sets the
    // manifold ids of the interior boundary cells to 0
    // (polar_manifold_id).
    Triangulation<2> cylinder_tria;
    GridGenerator::hyper_cube_with_cylindrical_hole(cylinder_tria,
                                                    0.05 + shell_region_width,
                                                    0.41 / 4.0);
    // The bulk cells are not quite squares, so we need to move the left
    // and right sides of cylinder_tria inwards so that it fits in
    // bulk_tria:
    for (const auto &cell : cylinder_tria.active_cell_iterators())
      for (unsigned int vertex_n = 0;
           vertex_n < GeometryInfo<2>::vertices_per_cell;
           ++vertex_n)
        {
          if (std::abs(cell->vertex(vertex_n)[0] - -0.41 / 4.0) < 1e-10)
            cell->vertex(vertex_n)[0] = -0.1;
          else if (std::abs(cell->vertex(vertex_n)[0] - 0.41 / 4.0) < 1e-10)
            cell->vertex(vertex_n)[0] = 0.1;
        }

    // Assign interior manifold ids to be the TFI id.
    for (const auto &cell : cylinder_tria.active_cell_iterators())
      {
        cell->set_manifold_id(tfi_manifold_id);
        for (unsigned int face_n = 0; face_n < GeometryInfo<2>::faces_per_cell;
             ++face_n)
          if (!cell->face(face_n)->at_boundary())
            cell->face(face_n)->set_manifold_id(tfi_manifold_id);
      }
    if (0.0 < shell_region_width)
      {
        Assert(0 < n_shells,
               ExcMessage("If the shell region has positive width then "
                          "there must be at least one shell."));
        Triangulation<2> shell_tria;
        GridGenerator::concentric_hyper_shells(shell_tria,
                                               Point<2>(),
                                               0.05,
                                               0.05 + shell_region_width,
                                               n_shells,
                                               skewness,
                                               8);

        // Make the tolerance as large as possible since these cells can
        // be quite close together
        const double vertex_tolerance =
          std::min(internal::minimal_vertex_distance(shell_tria),
                   internal::minimal_vertex_distance(cylinder_tria)) *
          0.5;

        shell_tria.set_all_manifold_ids(polar_manifold_id);
        Triangulation<2> temp;
        GridGenerator::merge_triangulations(
          shell_tria, cylinder_tria, temp, vertex_tolerance, true);
        cylinder_tria = std::move(temp);
      }
    GridTools::shift(cylinder_triangulation_offset, cylinder_tria);

    // Compute the tolerance again, since the shells may be very close to
    // each-other:
    const double vertex_tolerance =
      std::min(internal::minimal_vertex_distance(tria_without_cylinder),
               internal::minimal_vertex_distance(cylinder_tria)) /
      10;
    GridGenerator::merge_triangulations(
      tria_without_cylinder, cylinder_tria, tria, vertex_tolerance, true);

    // Ensure that all manifold ids on a polar cell really are set to the
    // polar manifold id:
    for (const auto &cell : tria.active_cell_iterators())
      if (cell->manifold_id() == polar_manifold_id)
        cell->set_all_manifold_ids(polar_manifold_id);

    // Ensure that all other manifold ids (including the interior faces
    // opposite the cylinder) are set to the flat manifold id:
    for (const auto &cell : tria.active_cell_iterators())
      if (cell->manifold_id() != polar_manifold_id &&
          cell->manifold_id() != tfi_manifold_id)
        cell->set_all_manifold_ids(numbers::flat_manifold_id);

    // We need to calculate the current center so that we can move it later:
    // to start get a unique list of (points to) vertices on the cylinder
    std::vector<Point<2> *> cylinder_pointers;
    for (const auto &face : tria.active_face_iterators())
      if (face->manifold_id() == polar_manifold_id)
        {
          cylinder_pointers.push_back(&face->vertex(0));
          cylinder_pointers.push_back(&face->vertex(1));
        }
    // de-duplicate
    std::sort(cylinder_pointers.begin(), cylinder_pointers.end());
    cylinder_pointers.erase(std::unique(cylinder_pointers.begin(),
                                        cylinder_pointers.end()),
                            cylinder_pointers.end());

    // find the current center...
    Point<2> center;
    for (const Point<2> *const ptr : cylinder_pointers)
      center += *ptr / double(cylinder_pointers.size());

    // and recenter at (0.2, 0.2)
    for (Point<2> *const ptr : cylinder_pointers)
      *ptr += Point<2>(0.2, 0.2) - center;

    // attach manifolds
    PolarManifold<2> polar_manifold(Point<2>(0.2, 0.2));
    tria.set_manifold(polar_manifold_id, polar_manifold);
    TransfiniteInterpolationManifold<2> inner_manifold;
    inner_manifold.initialize(tria);
    tria.set_manifold(tfi_manifold_id, inner_manifold);

    if (colorize)
      for (const auto &face : tria.active_face_iterators())
        if (face->at_boundary())
          {
            const Point<2> center = face->center();
            // left side
            if (std::abs(center[0] - 0.0) < 1e-10)
              face->set_boundary_id(0);
            // right side
            else if (std::abs(center[0] - 2.2) < 1e-10)
              face->set_boundary_id(1);
            // cylinder boundary
            else if (face->manifold_id() == polar_manifold_id)
              face->set_boundary_id(2);
            // sides of channel
            else
              {
                Assert(std::abs(center[1] - 0.00) < 1.0e-10 ||
                         std::abs(center[1] - 0.41) < 1.0e-10,
                       ExcInternalError());
                face->set_boundary_id(3);
              }
          }
  }



  template <>
  void channel_with_cylinder(Triangulation<3> & tria,
                             const double       shell_region_width,
                             const unsigned int n_shells,
                             const double       skewness,
                             const bool         colorize)
  {
    Triangulation<2> tria_2;
    channel_with_cylinder(
      tria_2, shell_region_width, n_shells, skewness, colorize);
    extrude_triangulation(tria_2, 5, 0.41, tria, true);

    // set up the new 3D manifolds
    const types::manifold_id      cylindrical_manifold_id = 0;
    const types::manifold_id      tfi_manifold_id         = 1;
    const PolarManifold<2> *const m_ptr =
      dynamic_cast<const PolarManifold<2> *>(
        &tria_2.get_manifold(cylindrical_manifold_id));
    Assert(m_ptr != nullptr, ExcInternalError());
    const Point<3>     axial_point(m_ptr->center[0], m_ptr->center[1], 0.0);
    const Tensor<1, 3> direction{{0.0, 0.0, 1.0}};

    const CylindricalManifold<3> cylindrical_manifold(direction, axial_point);
    TransfiniteInterpolationManifold<3> inner_manifold;
    inner_manifold.initialize(tria);
    tria.set_manifold(cylindrical_manifold_id, cylindrical_manifold);
    tria.set_manifold(tfi_manifold_id, inner_manifold);

    // From extrude_triangulation: since the maximum boundary id of tria_2 was
    // 3, the bottom boundary id is 4 and the top is 5: both are walls, so set
    // them to 3
    if (colorize)
      for (const auto &face : tria.active_face_iterators())
        if (face->boundary_id() == 4 || face->boundary_id() == 5)
          face->set_boundary_id(3);
  }



  template <int dim, int spacedim>
  void
  hyper_cross(Triangulation<dim, spacedim> &   tria,
              const std::vector<unsigned int> &sizes,
              const bool                       colorize)
  {
    AssertDimension(sizes.size(), GeometryInfo<dim>::faces_per_cell);
    Assert(dim > 1, ExcNotImplemented());
    Assert(dim < 4, ExcNotImplemented());

    // If there is a desire at some point to change the geometry of
    // the cells, this tensor can be made an argument to the function.
    Tensor<1, dim> dimensions;
    for (unsigned int d = 0; d < dim; ++d)
      dimensions[d] = 1.;

    std::vector<Point<spacedim>> points;
    unsigned int                 n_cells = 1;
    for (unsigned int i = 0; i < GeometryInfo<dim>::faces_per_cell; ++i)
      n_cells += sizes[i];

    std::vector<CellData<dim>> cells(n_cells);
    // Vertices of the center cell
    for (unsigned int i = 0; i < GeometryInfo<dim>::vertices_per_cell; ++i)
      {
        Point<spacedim> p;
        for (unsigned int d = 0; d < dim; ++d)
          p(d) = 0.5 * dimensions[d] *
                 GeometryInfo<dim>::unit_normal_orientation
                   [GeometryInfo<dim>::vertex_to_face[i][d]];
        points.push_back(p);
        cells[0].vertices[i] = i;
      }
    cells[0].material_id = 0;

    // The index of the first cell of the leg.
    unsigned int cell_index = 1;
    // The legs of the cross
    for (unsigned int face = 0; face < GeometryInfo<dim>::faces_per_cell;
         ++face)
      {
        const unsigned int oface = GeometryInfo<dim>::opposite_face[face];
        const unsigned int dir = GeometryInfo<dim>::unit_normal_direction[face];

        // We are moving in the direction of face
        for (unsigned int j = 0; j < sizes[face]; ++j, ++cell_index)
          {
            const unsigned int last_cell = (j == 0) ? 0U : (cell_index - 1);

            for (unsigned int v = 0; v < GeometryInfo<dim>::vertices_per_face;
                 ++v)
              {
                const unsigned int cellv =
                  GeometryInfo<dim>::face_to_cell_vertices(face, v);
                const unsigned int ocellv =
                  GeometryInfo<dim>::face_to_cell_vertices(oface, v);
                // First the vertices which already exist
                cells[cell_index].vertices[ocellv] =
                  cells[last_cell].vertices[cellv];

                // Now the new vertices
                cells[cell_index].vertices[cellv] = points.size();

                Point<spacedim> p = points[cells[cell_index].vertices[ocellv]];
                p(dir) += GeometryInfo<dim>::unit_normal_orientation[face] *
                          dimensions[dir];
                points.push_back(p);
              }
            cells[cell_index].material_id = (colorize) ? (face + 1U) : 0U;
          }
      }
    tria.create_triangulation(points, cells, SubCellData());
  }


  template <>
  void
    hyper_cube_slit(Triangulation<1> &, const double, const double, const bool)
  {
    Assert(false, ExcNotImplemented());
  }



  template <>
  void enclosed_hyper_cube(Triangulation<1> &,
                           const double,
                           const double,
                           const double,
                           const bool)
  {
    Assert(false, ExcNotImplemented());
  }



  template <>
  void hyper_L(Triangulation<1> &, const double, const double, const bool)
  {
    Assert(false, ExcNotImplemented());
  }



  template <>
  void
    hyper_ball(Triangulation<1> &, const Point<1> &, const double, const bool)
  {
    Assert(false, ExcNotImplemented());
  }



  template <>
  void cylinder(Triangulation<1> &, const double, const double)
  {
    Assert(false, ExcNotImplemented());
  }



  template <>
  void
    truncated_cone(Triangulation<1> &, const double, const double, const double)
  {
    Assert(false, ExcNotImplemented());
  }



  template <>
  void hyper_shell(Triangulation<1> &,
                   const Point<1> &,
                   const double,
                   const double,
                   const unsigned int,
                   const bool)
  {
    Assert(false, ExcNotImplemented());
  }

  template <>
  void cylinder_shell(Triangulation<1> &,
                      const double,
                      const double,
                      const double,
                      const unsigned int,
                      const unsigned int)
  {
    Assert(false, ExcNotImplemented());
  }


  template <>
  void quarter_hyper_ball(Triangulation<1> &, const Point<1> &, const double)
  {
    Assert(false, ExcNotImplemented());
  }


  template <>
  void half_hyper_ball(Triangulation<1> &, const Point<1> &, const double)
  {
    Assert(false, ExcNotImplemented());
  }


  template <>
  void half_hyper_shell(Triangulation<1> &,
                        const Point<1> &,
                        const double,
                        const double,
                        const unsigned int,
                        const bool)
  {
    Assert(false, ExcNotImplemented());
  }

  template <>
  void quarter_hyper_shell(Triangulation<1> &,
                           const Point<1> &,
                           const double,
                           const double,
                           const unsigned int,
                           const bool)
  {
    Assert(false, ExcNotImplemented());
  }

  template <>
  void enclosed_hyper_cube(Triangulation<2> &tria,
                           const double      left,
                           const double      right,
                           const double      thickness,
                           const bool        colorize)
  {
    Assert(left < right,
           ExcMessage("Invalid left-to-right bounds of enclosed hypercube"));

    std::vector<Point<2>> vertices(16);
    double                coords[4];
    coords[0] = left - thickness;
    coords[1] = left;
    coords[2] = right;
    coords[3] = right + thickness;

    unsigned int k = 0;
    for (const double y : coords)
      for (const double x : coords)
        vertices[k++] = Point<2>(x, y);

    const types::material_id materials[9] = {5, 4, 6, 1, 0, 2, 9, 8, 10};

    std::vector<CellData<2>> cells(9);
    k = 0;
    for (unsigned int i0 = 0; i0 < 3; ++i0)
      for (unsigned int i1 = 0; i1 < 3; ++i1)
        {
          cells[k].vertices[0] = i1 + 4 * i0;
          cells[k].vertices[1] = i1 + 4 * i0 + 1;
          cells[k].vertices[2] = i1 + 4 * i0 + 4;
          cells[k].vertices[3] = i1 + 4 * i0 + 5;
          if (colorize)
            cells[k].material_id = materials[k];
          ++k;
        }
    tria.create_triangulation(vertices,
                              cells,
                              SubCellData()); // no boundary information
  }



  // Implementation for 2D only
  template <>
  void hyper_cube_slit(Triangulation<2> &tria,
                       const double      left,
                       const double      right,
                       const bool        colorize)
  {
    const double             rl2                 = (right + left) / 2;
    const Point<2>           vertices[10]        = {Point<2>(left, left),
                                   Point<2>(rl2, left),
                                   Point<2>(rl2, rl2),
                                   Point<2>(left, rl2),
                                   Point<2>(right, left),
                                   Point<2>(right, rl2),
                                   Point<2>(rl2, right),
                                   Point<2>(left, right),
                                   Point<2>(right, right),
                                   Point<2>(rl2, left)};
    const int                cell_vertices[4][4] = {{0, 1, 3, 2},
                                     {9, 4, 2, 5},
                                     {3, 2, 7, 6},
                                     {2, 5, 6, 8}};
    std::vector<CellData<2>> cells(4, CellData<2>());
    for (unsigned int i = 0; i < 4; ++i)
      {
        for (unsigned int j = 0; j < 4; ++j)
          cells[i].vertices[j] = cell_vertices[i][j];
        cells[i].material_id = 0;
      }
    tria.create_triangulation(std::vector<Point<2>>(std::begin(vertices),
                                                    std::end(vertices)),
                              cells,
                              SubCellData()); // no boundary information

    if (colorize)
      {
        Triangulation<2>::cell_iterator cell = tria.begin();
        cell->face(1)->set_boundary_id(1);
        ++cell;
        cell->face(0)->set_boundary_id(2);
      }
  }



  template <>
  void truncated_cone(Triangulation<2> &triangulation,
                      const double      radius_0,
                      const double      radius_1,
                      const double      half_length)
  {
    Point<2> vertices_tmp[4];

    vertices_tmp[0] = Point<2>(-half_length, -radius_0);
    vertices_tmp[1] = Point<2>(half_length, -radius_1);
    vertices_tmp[2] = Point<2>(-half_length, radius_0);
    vertices_tmp[3] = Point<2>(half_length, radius_1);

    const std::vector<Point<2>> vertices(std::begin(vertices_tmp),
                                         std::end(vertices_tmp));
    unsigned int cell_vertices[1][GeometryInfo<2>::vertices_per_cell];

    for (unsigned int i = 0; i < GeometryInfo<2>::vertices_per_cell; ++i)
      cell_vertices[0][i] = i;

    std::vector<CellData<2>> cells(1, CellData<2>());

    for (unsigned int i = 0; i < GeometryInfo<2>::vertices_per_cell; ++i)
      cells[0].vertices[i] = cell_vertices[0][i];

    cells[0].material_id = 0;
    triangulation.create_triangulation(vertices, cells, SubCellData());

    Triangulation<2>::cell_iterator cell = triangulation.begin();

    cell->face(0)->set_boundary_id(1);
    cell->face(1)->set_boundary_id(2);

    for (unsigned int i = 2; i < 4; ++i)
      cell->face(i)->set_boundary_id(0);
  }



  // Implementation for 2D only
  template <>
  void hyper_L(Triangulation<2> &tria,
               const double      a,
               const double      b,
               const bool        colorize)
  {
    const Point<2> vertices[8]    = {Point<2>(a, a),
                                  Point<2>((a + b) / 2, a),
                                  Point<2>(b, a),
                                  Point<2>(a, (a + b) / 2),
                                  Point<2>((a + b) / 2, (a + b) / 2),
                                  Point<2>(b, (a + b) / 2),
                                  Point<2>(a, b),
                                  Point<2>((a + b) / 2, b)};
    const int cell_vertices[3][4] = {{0, 1, 3, 4}, {1, 2, 4, 5}, {3, 4, 6, 7}};

    std::vector<CellData<2>> cells(3, CellData<2>());

    for (unsigned int i = 0; i < 3; ++i)
      {
        for (unsigned int j = 0; j < 4; ++j)
          cells[i].vertices[j] = cell_vertices[i][j];
        cells[i].material_id = 0;
      }

    tria.create_triangulation(std::vector<Point<2>>(std::begin(vertices),
                                                    std::end(vertices)),
                              cells,
                              SubCellData());

    if (colorize)
      {
        Triangulation<2>::cell_iterator cell = tria.begin();

        cell->face(0)->set_boundary_id(0);
        cell->face(2)->set_boundary_id(1);
        cell++;

        cell->face(1)->set_boundary_id(2);
        cell->face(2)->set_boundary_id(1);
        cell->face(3)->set_boundary_id(3);
        cell++;

        cell->face(0)->set_boundary_id(0);
        cell->face(1)->set_boundary_id(4);
        cell->face(3)->set_boundary_id(5);
      }
  }



  template <int dim, int spacedim>
  void
  subdivided_hyper_L(Triangulation<dim, spacedim> &   tria,
                     const std::vector<unsigned int> &repetitions,
                     const Point<dim> &               bottom_left,
                     const Point<dim> &               top_right,
                     const std::vector<int> &         n_cells_to_remove)
  {
    Assert(dim > 1, ExcNotImplemented());
    // Check the consistency of the dimensions provided.
    AssertDimension(repetitions.size(), dim);
    AssertDimension(n_cells_to_remove.size(), dim);
    for (unsigned int d = 0; d < dim; ++d)
      {
        Assert(std::fabs(n_cells_to_remove[d]) <= repetitions[d],
               ExcMessage("Attempting to cut away too many cells."));
      }
    // Create the domain to be cut
    Triangulation<dim, spacedim> rectangle;
    GridGenerator::subdivided_hyper_rectangle(rectangle,
                                              repetitions,
                                              bottom_left,
                                              top_right);
    // compute the vertex of the cut step, we will cut according to the
    // location of the cartesian coordinates of the cell centers
    std::array<double, dim> h;
    Point<dim>              cut_step;
    for (unsigned int d = 0; d < dim; ++d)
      {
        // mesh spacing in each direction in cartesian coordinates
        h[d] = (top_right[d] - bottom_left[d]) / repetitions[d];
        // left to right, bottom to top, front to back
        if (n_cells_to_remove[d] >= 0)
          {
            // cartesian coordinates of vertex location
            cut_step[d] =
              h[d] * std::fabs(n_cells_to_remove[d]) + bottom_left[d];
          }
        // right to left, top to bottom, back to front
        else
          {
            cut_step[d] = top_right[d] - h[d] * std::fabs(n_cells_to_remove[d]);
          }
      }


    // compute cells to remove
    std::set<typename Triangulation<dim, spacedim>::active_cell_iterator>
      cells_to_remove;
    std::copy_if(
      rectangle.active_cell_iterators().begin(),
      rectangle.active_cell_iterators().end(),
      std::inserter(cells_to_remove, cells_to_remove.end()),
      [&](
        const typename Triangulation<dim, spacedim>::active_cell_iterator &cell)
        -> bool {
        for (unsigned int d = 0; d < dim; ++d)
          if ((n_cells_to_remove[d] > 0 && cell->center()[d] >= cut_step[d]) ||
              (n_cells_to_remove[d] < 0 && cell->center()[d] <= cut_step[d]))
            return false;

        return true;
      });

    GridGenerator::create_triangulation_with_removed_cells(rectangle,
                                                           cells_to_remove,
                                                           tria);
  }



  // Implementation for 2D only
  template <>
  void hyper_ball(Triangulation<2> &tria,
                  const Point<2> &  p,
                  const double      radius,
                  const bool        internal_manifolds)
  {
    // equilibrate cell sizes at
    // transition from the inner part
    // to the radial cells
    const double   a           = 1. / (1 + std::sqrt(2.0));
    const Point<2> vertices[8] = {
      p + Point<2>(-1, -1) * (radius / std::sqrt(2.0)),
      p + Point<2>(+1, -1) * (radius / std::sqrt(2.0)),
      p + Point<2>(-1, -1) * (radius / std::sqrt(2.0) * a),
      p + Point<2>(+1, -1) * (radius / std::sqrt(2.0) * a),
      p + Point<2>(-1, +1) * (radius / std::sqrt(2.0) * a),
      p + Point<2>(+1, +1) * (radius / std::sqrt(2.0) * a),
      p + Point<2>(-1, +1) * (radius / std::sqrt(2.0)),
      p + Point<2>(+1, +1) * (radius / std::sqrt(2.0))};

    const int cell_vertices[5][4] = {
      {0, 1, 2, 3}, {0, 2, 6, 4}, {2, 3, 4, 5}, {1, 7, 3, 5}, {6, 4, 7, 5}};

    std::vector<CellData<2>> cells(5, CellData<2>());

    for (unsigned int i = 0; i < 5; ++i)
      {
        for (unsigned int j = 0; j < 4; ++j)
          cells[i].vertices[j] = cell_vertices[i][j];
        cells[i].material_id = 0;
        cells[i].manifold_id = i == 2 ? 1 : numbers::flat_manifold_id;
      }

    tria.create_triangulation(std::vector<Point<2>>(std::begin(vertices),
                                                    std::end(vertices)),
                              cells,
                              SubCellData()); // no boundary information
    tria.set_all_manifold_ids_on_boundary(0);
    tria.set_manifold(0, SphericalManifold<2>(p));
    if (internal_manifolds)
      tria.set_manifold(1, SphericalManifold<2>(p));
  }



  template <>
  void hyper_shell(Triangulation<2> & tria,
                   const Point<2> &   center,
                   const double       inner_radius,
                   const double       outer_radius,
                   const unsigned int n_cells,
                   const bool         colorize)
  {
    Assert((inner_radius > 0) && (inner_radius < outer_radius),
           ExcInvalidRadii());

    const double pi = numbers::PI;

    // determine the number of cells
    // for the grid. if not provided by
    // the user determine it such that
    // the length of each cell on the
    // median (in the middle between
    // the two circles) is equal to its
    // radial extent (which is the
    // difference between the two
    // radii)
    const unsigned int N =
      (n_cells == 0 ? static_cast<unsigned int>(
                        std::ceil((2 * pi * (outer_radius + inner_radius) / 2) /
                                  (outer_radius - inner_radius))) :
                      n_cells);

    // set up N vertices on the
    // outer and N vertices on
    // the inner circle. the
    // first N ones are on the
    // outer one, and all are
    // numbered counter-clockwise
    std::vector<Point<2>> vertices(2 * N);
    for (unsigned int i = 0; i < N; ++i)
      {
        vertices[i] =
          Point<2>(std::cos(2 * pi * i / N), std::sin(2 * pi * i / N)) *
          outer_radius;
        vertices[i + N] = vertices[i] * (inner_radius / outer_radius);

        vertices[i] += center;
        vertices[i + N] += center;
      }

    std::vector<CellData<2>> cells(N, CellData<2>());

    for (unsigned int i = 0; i < N; ++i)
      {
        cells[i].vertices[0] = i;
        cells[i].vertices[1] = (i + 1) % N;
        cells[i].vertices[2] = N + i;
        cells[i].vertices[3] = N + ((i + 1) % N);

        cells[i].material_id = 0;
      }

    tria.create_triangulation(vertices, cells, SubCellData());

    if (colorize)
      colorize_hyper_shell(tria, center, inner_radius, outer_radius);

    tria.set_all_manifold_ids(0);
    tria.set_manifold(0, SphericalManifold<2>(center));
  }



  template <int dim>
  void
  eccentric_hyper_shell(Triangulation<dim> &tria,
                        const Point<dim> &  inner_center,
                        const Point<dim> &  outer_center,
                        const double        inner_radius,
                        const double        outer_radius,
                        const unsigned int  n_cells)
  {
    GridGenerator::hyper_shell(
      tria, outer_center, inner_radius, outer_radius, n_cells, true);

    // check the consistency of the dimensions provided
    Assert(
      outer_radius - inner_radius > outer_center.distance(inner_center),
      ExcInternalError(
        "The inner radius is greater than or equal to the outer radius plus eccentricity."));

    // shift nodes along the inner boundary according to the position of
    // inner_circle
    std::set<Point<dim> *> vertices_to_move;

    for (const auto &face : tria.active_face_iterators())
      if (face->boundary_id() == 0)
        for (unsigned int v = 0; v < GeometryInfo<dim>::vertices_per_face; ++v)
          vertices_to_move.insert(&face->vertex(v));

    const auto shift = inner_center - outer_center;
    for (const auto &p : vertices_to_move)
      (*p) += shift;

    // the original hyper_shell function assigns the same manifold id
    // to all cells and faces. Set all manifolds ids to a different
    // value (2), then use boundary ids to assign different manifolds to
    // the inner (0) and outer manifolds (1). Use a transfinite manifold
    // for all faces and cells aside from the boundaries.
    tria.set_all_manifold_ids(2);
    GridTools::copy_boundary_to_manifold_id(tria);

    SphericalManifold<dim> inner_manifold(inner_center);
    SphericalManifold<dim> outer_manifold(outer_center);

    TransfiniteInterpolationManifold<dim> transfinite;
    transfinite.initialize(tria);

    tria.set_manifold(0, inner_manifold);
    tria.set_manifold(1, outer_manifold);
    tria.set_manifold(2, transfinite);
  }



  // Implementation for 2D only
  template <>
  void cylinder(Triangulation<2> &tria,
                const double      radius,
                const double      half_length)
  {
    Point<2> p1(-half_length, -radius);
    Point<2> p2(half_length, radius);

    hyper_rectangle(tria, p1, p2, true);

    Triangulation<2>::face_iterator f   = tria.begin_face();
    Triangulation<2>::face_iterator end = tria.end_face();
    while (f != end)
      {
        switch (f->boundary_id())
          {
            case 0:
              f->set_boundary_id(1);
              break;
            case 1:
              f->set_boundary_id(2);
              break;
            default:
              f->set_boundary_id(0);
              break;
          }
        ++f;
      }
  }



  // Implementation for 2D only
  template <>
  void cylinder_shell(Triangulation<2> &,
                      const double,
                      const double,
                      const double,
                      const unsigned int,
                      const unsigned int)
  {
    Assert(false, ExcNotImplemented());
  }


  template <>
  void quarter_hyper_ball(Triangulation<2> &tria,
                          const Point<2> &  p,
                          const double      radius)
  {
    const unsigned int dim = 2;

    // equilibrate cell sizes at
    // transition from the inner part
    // to the radial cells
    const Point<dim> vertices[7] = {p + Point<dim>(0, 0) * radius,
                                    p + Point<dim>(+1, 0) * radius,
                                    p + Point<dim>(+1, 0) * (radius / 2),
                                    p + Point<dim>(0, +1) * (radius / 2),
                                    p + Point<dim>(+1, +1) *
                                          (radius / (2 * std::sqrt(2.0))),
                                    p + Point<dim>(0, +1) * radius,
                                    p + Point<dim>(+1, +1) *
                                          (radius / std::sqrt(2.0))};

    const int cell_vertices[3][4] = {{0, 2, 3, 4}, {1, 6, 2, 4}, {5, 3, 6, 4}};

    std::vector<CellData<dim>> cells(3, CellData<dim>());

    for (unsigned int i = 0; i < 3; ++i)
      {
        for (unsigned int j = 0; j < 4; ++j)
          cells[i].vertices[j] = cell_vertices[i][j];
        cells[i].material_id = 0;
      }

    tria.create_triangulation(std::vector<Point<dim>>(std::begin(vertices),
                                                      std::end(vertices)),
                              cells,
                              SubCellData()); // no boundary information

    Triangulation<dim>::cell_iterator cell = tria.begin();
    Triangulation<dim>::cell_iterator end  = tria.end();

    tria.set_all_manifold_ids_on_boundary(0);

    while (cell != end)
      {
        for (unsigned int i = 0; i < GeometryInfo<dim>::faces_per_cell; ++i)
          {
            if (cell->face(i)->boundary_id() ==
                numbers::internal_face_boundary_id)
              continue;

            // If one the components is the same as the respective
            // component of the center, then this is part of the plane
            if (cell->face(i)->center()(0) < p(0) + 1.e-5 * radius ||
                cell->face(i)->center()(1) < p(1) + 1.e-5 * radius)
              {
                cell->face(i)->set_boundary_id(1);
                cell->face(i)->set_manifold_id(numbers::flat_manifold_id);
              }
          }
        ++cell;
      }
    tria.set_manifold(0, SphericalManifold<2>(p));
  }


  template <>
  void half_hyper_ball(Triangulation<2> &tria,
                       const Point<2> &  p,
                       const double      radius)
  {
    // equilibrate cell sizes at
    // transition from the inner part
    // to the radial cells
    const double   a           = 1. / (1 + std::sqrt(2.0));
    const Point<2> vertices[8] = {
      p + Point<2>(0, -1) * radius,
      p + Point<2>(+1, -1) * (radius / std::sqrt(2.0)),
      p + Point<2>(0, -1) * (radius / std::sqrt(2.0) * a),
      p + Point<2>(+1, -1) * (radius / std::sqrt(2.0) * a),
      p + Point<2>(0, +1) * (radius / std::sqrt(2.0) * a),
      p + Point<2>(+1, +1) * (radius / std::sqrt(2.0) * a),
      p + Point<2>(0, +1) * radius,
      p + Point<2>(+1, +1) * (radius / std::sqrt(2.0))};

    const int cell_vertices[5][4] = {{0, 1, 2, 3},
                                     {2, 3, 4, 5},
                                     {1, 7, 3, 5},
                                     {6, 4, 7, 5}};

    std::vector<CellData<2>> cells(4, CellData<2>());

    for (unsigned int i = 0; i < 4; ++i)
      {
        for (unsigned int j = 0; j < 4; ++j)
          cells[i].vertices[j] = cell_vertices[i][j];
        cells[i].material_id = 0;
      }

    tria.create_triangulation(std::vector<Point<2>>(std::begin(vertices),
                                                    std::end(vertices)),
                              cells,
                              SubCellData()); // no boundary information

    Triangulation<2>::cell_iterator cell = tria.begin();
    Triangulation<2>::cell_iterator end  = tria.end();

    tria.set_all_manifold_ids_on_boundary(0);

    while (cell != end)
      {
        for (unsigned int i = 0; i < GeometryInfo<2>::faces_per_cell; ++i)
          {
            if (cell->face(i)->boundary_id() ==
                numbers::internal_face_boundary_id)
              continue;

            // If x is zero, then this is part of the plane
            if (cell->face(i)->center()(0) < p(0) + 1.e-5 * radius)
              {
                cell->face(i)->set_boundary_id(1);
                cell->face(i)->set_manifold_id(numbers::flat_manifold_id);
              }
          }
        ++cell;
      }
    tria.set_manifold(0, SphericalManifold<2>(p));
  }



  // Implementation for 2D only
  template <>
  void half_hyper_shell(Triangulation<2> & tria,
                        const Point<2> &   center,
                        const double       inner_radius,
                        const double       outer_radius,
                        const unsigned int n_cells,
                        const bool         colorize)
  {
    Assert((inner_radius > 0) && (inner_radius < outer_radius),
           ExcInvalidRadii());

    const double pi = numbers::PI;
    // determine the number of cells
    // for the grid. if not provided by
    // the user determine it such that
    // the length of each cell on the
    // median (in the middle between
    // the two circles) is equal to its
    // radial extent (which is the
    // difference between the two
    // radii)
    const unsigned int N =
      (n_cells == 0 ? static_cast<unsigned int>(
                        std::ceil((pi * (outer_radius + inner_radius) / 2) /
                                  (outer_radius - inner_radius))) :
                      n_cells);

    // set up N+1 vertices on the
    // outer and N+1 vertices on
    // the inner circle. the
    // first N+1 ones are on the
    // outer one, and all are
    // numbered counter-clockwise
    std::vector<Point<2>> vertices(2 * (N + 1));
    for (unsigned int i = 0; i <= N; ++i)
      {
        // enforce that the x-coordinates
        // of the first and last point of
        // each half-circle are exactly
        // zero (contrary to what we may
        // compute using the imprecise
        // value of pi)
        vertices[i] =
          Point<2>(((i == 0) || (i == N) ? 0 : std::cos(pi * i / N - pi / 2)),
                   std::sin(pi * i / N - pi / 2)) *
          outer_radius;
        vertices[i + N + 1] = vertices[i] * (inner_radius / outer_radius);

        vertices[i] += center;
        vertices[i + N + 1] += center;
      }


    std::vector<CellData<2>> cells(N, CellData<2>());

    for (unsigned int i = 0; i < N; ++i)
      {
        cells[i].vertices[0] = i;
        cells[i].vertices[1] = (i + 1) % (N + 1);
        cells[i].vertices[2] = N + 1 + i;
        cells[i].vertices[3] = N + 1 + ((i + 1) % (N + 1));

        cells[i].material_id = 0;
      }

    tria.create_triangulation(vertices, cells, SubCellData());

    if (colorize)
      {
        Triangulation<2>::cell_iterator cell = tria.begin();
        for (; cell != tria.end(); ++cell)
          {
            cell->face(2)->set_boundary_id(1);
          }
        tria.begin()->face(0)->set_boundary_id(3);

        tria.last()->face(1)->set_boundary_id(2);
      }
    tria.set_all_manifold_ids(0);
    tria.set_manifold(0, SphericalManifold<2>(center));
  }


  template <>
  void quarter_hyper_shell(Triangulation<2> & tria,
                           const Point<2> &   center,
                           const double       inner_radius,
                           const double       outer_radius,
                           const unsigned int n_cells,
                           const bool         colorize)
  {
    Assert((inner_radius > 0) && (inner_radius < outer_radius),
           ExcInvalidRadii());

    const double pi = numbers::PI;
    // determine the number of cells
    // for the grid. if not provided by
    // the user determine it such that
    // the length of each cell on the
    // median (in the middle between
    // the two circles) is equal to its
    // radial extent (which is the
    // difference between the two
    // radii)
    const unsigned int N =
      (n_cells == 0 ? static_cast<unsigned int>(
                        std::ceil((pi * (outer_radius + inner_radius) / 4) /
                                  (outer_radius - inner_radius))) :
                      n_cells);

    // set up N+1 vertices on the
    // outer and N+1 vertices on
    // the inner circle. the
    // first N+1 ones are on the
    // outer one, and all are
    // numbered counter-clockwise
    std::vector<Point<2>> vertices(2 * (N + 1));
    for (unsigned int i = 0; i <= N; ++i)
      {
        // enforce that the x-coordinates
        // of the last point is exactly
        // zero (contrary to what we may
        // compute using the imprecise
        // value of pi)
        vertices[i] = Point<2>(((i == N) ? 0 : std::cos(pi * i / N / 2)),
                               std::sin(pi * i / N / 2)) *
                      outer_radius;
        vertices[i + N + 1] = vertices[i] * (inner_radius / outer_radius);

        vertices[i] += center;
        vertices[i + N + 1] += center;
      }


    std::vector<CellData<2>> cells(N, CellData<2>());

    for (unsigned int i = 0; i < N; ++i)
      {
        cells[i].vertices[0] = i;
        cells[i].vertices[1] = (i + 1) % (N + 1);
        cells[i].vertices[2] = N + 1 + i;
        cells[i].vertices[3] = N + 1 + ((i + 1) % (N + 1));

        cells[i].material_id = 0;
      }

    tria.create_triangulation(vertices, cells, SubCellData());

    if (colorize)
      {
        Triangulation<2>::cell_iterator cell = tria.begin();
        for (; cell != tria.end(); ++cell)
          {
            cell->face(2)->set_boundary_id(1);
          }
        tria.begin()->face(0)->set_boundary_id(3);

        tria.last()->face(1)->set_boundary_id(2);
      }

    tria.set_all_manifold_ids(0);
    tria.set_manifold(0, SphericalManifold<2>(center));
  }



  // Implementation for 3D only
  template <>
  void hyper_cube_slit(Triangulation<3> &tria,
                       const double      left,
                       const double      right,
                       const bool        colorize)
  {
    const double rl2 = (right + left) / 2;
    const double len = (right - left) / 2.;

    const Point<3> vertices[20] = {
      Point<3>(left, left, -len / 2.),   Point<3>(rl2, left, -len / 2.),
      Point<3>(rl2, rl2, -len / 2.),     Point<3>(left, rl2, -len / 2.),
      Point<3>(right, left, -len / 2.),  Point<3>(right, rl2, -len / 2.),
      Point<3>(rl2, right, -len / 2.),   Point<3>(left, right, -len / 2.),
      Point<3>(right, right, -len / 2.), Point<3>(rl2, left, -len / 2.),
      Point<3>(left, left, len / 2.),    Point<3>(rl2, left, len / 2.),
      Point<3>(rl2, rl2, len / 2.),      Point<3>(left, rl2, len / 2.),
      Point<3>(right, left, len / 2.),   Point<3>(right, rl2, len / 2.),
      Point<3>(rl2, right, len / 2.),    Point<3>(left, right, len / 2.),
      Point<3>(right, right, len / 2.),  Point<3>(rl2, left, len / 2.)};
    const int cell_vertices[4][8] = {{0, 1, 3, 2, 10, 11, 13, 12},
                                     {9, 4, 2, 5, 19, 14, 12, 15},
                                     {3, 2, 7, 6, 13, 12, 17, 16},
                                     {2, 5, 6, 8, 12, 15, 16, 18}};
    std::vector<CellData<3>> cells(4, CellData<3>());
    for (unsigned int i = 0; i < 4; ++i)
      {
        for (unsigned int j = 0; j < 8; ++j)
          cells[i].vertices[j] = cell_vertices[i][j];
        cells[i].material_id = 0;
      }
    tria.create_triangulation(std::vector<Point<3>>(std::begin(vertices),
                                                    std::end(vertices)),
                              cells,
                              SubCellData()); // no boundary information

    if (colorize)
      {
        Triangulation<3>::cell_iterator cell = tria.begin();
        cell->face(1)->set_boundary_id(1);
        ++cell;
        cell->face(0)->set_boundary_id(2);
      }
  }



  // Implementation for 3D only
  template <>
  void enclosed_hyper_cube(Triangulation<3> &tria,
                           const double      left,
                           const double      right,
                           const double      thickness,
                           const bool        colorize)
  {
    Assert(left < right,
           ExcMessage("Invalid left-to-right bounds of enclosed hypercube"));

    std::vector<Point<3>> vertices(64);
    double                coords[4];
    coords[0] = left - thickness;
    coords[1] = left;
    coords[2] = right;
    coords[3] = right + thickness;

    unsigned int k = 0;
    for (const double z : coords)
      for (const double y : coords)
        for (const double x : coords)
          vertices[k++] = Point<3>(x, y, z);

    const types::material_id materials[27] = {21, 20, 22, 17, 16, 18, 25,
                                              24, 26, 5,  4,  6,  1,  0,
                                              2,  9,  8,  10, 37, 36, 38,
                                              33, 32, 34, 41, 40, 42};

    std::vector<CellData<3>> cells(27);
    k = 0;
    for (unsigned int z = 0; z < 3; ++z)
      for (unsigned int y = 0; y < 3; ++y)
        for (unsigned int x = 0; x < 3; ++x)
          {
            cells[k].vertices[0] = x + 4 * y + 16 * z;
            cells[k].vertices[1] = x + 4 * y + 16 * z + 1;
            cells[k].vertices[2] = x + 4 * y + 16 * z + 4;
            cells[k].vertices[3] = x + 4 * y + 16 * z + 5;
            cells[k].vertices[4] = x + 4 * y + 16 * z + 16;
            cells[k].vertices[5] = x + 4 * y + 16 * z + 17;
            cells[k].vertices[6] = x + 4 * y + 16 * z + 20;
            cells[k].vertices[7] = x + 4 * y + 16 * z + 21;
            if (colorize)
              cells[k].material_id = materials[k];
            ++k;
          }
    tria.create_triangulation(vertices,
                              cells,
                              SubCellData()); // no boundary information
  }



  template <>
  void truncated_cone(Triangulation<3> &triangulation,
                      const double      radius_0,
                      const double      radius_1,
                      const double      half_length)
  {
    Assert(triangulation.n_cells() == 0,
           ExcMessage("The output triangulation object needs to be empty."));
    Assert(0 < radius_0, ExcMessage("The radii must be positive."));
    Assert(0 < radius_1, ExcMessage("The radii must be positive."));
    Assert(0 < half_length, ExcMessage("The half length must be positive."));

    const auto n_slices = 1 + static_cast<unsigned int>(std::ceil(
                                half_length / std::max(radius_0, radius_1)));

    Triangulation<2> triangulation_2;
    GridGenerator::hyper_ball(triangulation_2, Point<2>(), radius_0);
    GridGenerator::extrude_triangulation(triangulation_2,
                                         n_slices,
                                         2 * half_length,
                                         triangulation);
    GridTools::rotate(numbers::PI / 2, 1, triangulation);
    GridTools::shift(Tensor<1, 3>({-half_length, 0.0, 0.0}), triangulation);
    // At this point we have a cylinder. Multiply the y and z coordinates by a
    // factor that scales (with x) linearly between radius_0 and radius_1 to fix
    // the circle radii and interior points:
    auto shift_radii = [=](const Point<3> &p) {
      const double slope  = (radius_1 / radius_0 - 1.0) / (2.0 * half_length);
      const double factor = slope * (p[0] - -half_length) + 1.0;
      return Point<3>(p[0], factor * p[1], factor * p[2]);
    };
    GridTools::transform(shift_radii, triangulation);

    // Set boundary ids at -half_length to 1 and at half_length to 2. Set the
    // manifold id on hull faces (i.e., faces not on either end) to 0.
    for (const auto &face : triangulation.active_face_iterators())
      if (face->at_boundary())
        {
          if (std::abs(face->center()[0] - -half_length) < 1e-8 * half_length)
            face->set_boundary_id(1);
          else if (std::abs(face->center()[0] - half_length) <
                   1e-8 * half_length)
            face->set_boundary_id(2);
          else
            face->set_all_manifold_ids(0);
        }

    triangulation.set_manifold(0, CylindricalManifold<3>());
  }


  // Implementation for 3D only
  template <>
  void hyper_L(Triangulation<3> &tria,
               const double      a,
               const double      b,
               const bool        colorize)
  {
    // we slice out the top back right
    // part of the cube
    const Point<3> vertices[26] = {
      // front face of the big cube
      Point<3>(a, a, a),
      Point<3>((a + b) / 2, a, a),
      Point<3>(b, a, a),
      Point<3>(a, a, (a + b) / 2),
      Point<3>((a + b) / 2, a, (a + b) / 2),
      Point<3>(b, a, (a + b) / 2),
      Point<3>(a, a, b),
      Point<3>((a + b) / 2, a, b),
      Point<3>(b, a, b),
      // middle face of the big cube
      Point<3>(a, (a + b) / 2, a),
      Point<3>((a + b) / 2, (a + b) / 2, a),
      Point<3>(b, (a + b) / 2, a),
      Point<3>(a, (a + b) / 2, (a + b) / 2),
      Point<3>((a + b) / 2, (a + b) / 2, (a + b) / 2),
      Point<3>(b, (a + b) / 2, (a + b) / 2),
      Point<3>(a, (a + b) / 2, b),
      Point<3>((a + b) / 2, (a + b) / 2, b),
      Point<3>(b, (a + b) / 2, b),
      // back face of the big cube
      // last (top right) point is missing
      Point<3>(a, b, a),
      Point<3>((a + b) / 2, b, a),
      Point<3>(b, b, a),
      Point<3>(a, b, (a + b) / 2),
      Point<3>((a + b) / 2, b, (a + b) / 2),
      Point<3>(b, b, (a + b) / 2),
      Point<3>(a, b, b),
      Point<3>((a + b) / 2, b, b)};
    const int cell_vertices[7][8] = {{0, 1, 9, 10, 3, 4, 12, 13},
                                     {1, 2, 10, 11, 4, 5, 13, 14},
                                     {3, 4, 12, 13, 6, 7, 15, 16},
                                     {4, 5, 13, 14, 7, 8, 16, 17},
                                     {9, 10, 18, 19, 12, 13, 21, 22},
                                     {10, 11, 19, 20, 13, 14, 22, 23},
                                     {12, 13, 21, 22, 15, 16, 24, 25}};

    std::vector<CellData<3>> cells(7, CellData<3>());

    for (unsigned int i = 0; i < 7; ++i)
      {
        for (unsigned int j = 0; j < 8; ++j)
          cells[i].vertices[j] = cell_vertices[i][j];
        cells[i].material_id = 0;
      }

    tria.create_triangulation(std::vector<Point<3>>(std::begin(vertices),
                                                    std::end(vertices)),
                              cells,
                              SubCellData()); // no boundary information

    if (colorize)
      {
        Assert(false, ExcNotImplemented());
      }
  }



  // Implementation for 3D only
  template <>
  void hyper_ball(Triangulation<3> &tria,
                  const Point<3> &  p,
                  const double      radius,
                  const bool        internal_manifold)
  {
    const double a =
      1. / (1 + std::sqrt(3.0)); // equilibrate cell sizes at transition
    // from the inner part to the radial
    // cells
    const unsigned int n_vertices           = 16;
    const Point<3>     vertices[n_vertices] = {
      // first the vertices of the inner
      // cell
      p + Point<3>(-1, -1, -1) * (radius / std::sqrt(3.0) * a),
      p + Point<3>(+1, -1, -1) * (radius / std::sqrt(3.0) * a),
      p + Point<3>(+1, -1, +1) * (radius / std::sqrt(3.0) * a),
      p + Point<3>(-1, -1, +1) * (radius / std::sqrt(3.0) * a),
      p + Point<3>(-1, +1, -1) * (radius / std::sqrt(3.0) * a),
      p + Point<3>(+1, +1, -1) * (radius / std::sqrt(3.0) * a),
      p + Point<3>(+1, +1, +1) * (radius / std::sqrt(3.0) * a),
      p + Point<3>(-1, +1, +1) * (radius / std::sqrt(3.0) * a),
      // now the eight vertices at
      // the outer sphere
      p + Point<3>(-1, -1, -1) * (radius / std::sqrt(3.0)),
      p + Point<3>(+1, -1, -1) * (radius / std::sqrt(3.0)),
      p + Point<3>(+1, -1, +1) * (radius / std::sqrt(3.0)),
      p + Point<3>(-1, -1, +1) * (radius / std::sqrt(3.0)),
      p + Point<3>(-1, +1, -1) * (radius / std::sqrt(3.0)),
      p + Point<3>(+1, +1, -1) * (radius / std::sqrt(3.0)),
      p + Point<3>(+1, +1, +1) * (radius / std::sqrt(3.0)),
      p + Point<3>(-1, +1, +1) * (radius / std::sqrt(3.0)),
    };

    // one needs to draw the seven cubes to
    // understand what's going on here
    const unsigned int n_cells                   = 7;
    const int          cell_vertices[n_cells][8] = {
      {0, 1, 4, 5, 3, 2, 7, 6},      // center
      {8, 9, 12, 13, 0, 1, 4, 5},    // bottom
      {9, 13, 1, 5, 10, 14, 2, 6},   // right
      {11, 10, 3, 2, 15, 14, 7, 6},  // top
      {8, 0, 12, 4, 11, 3, 15, 7},   // left
      {8, 9, 0, 1, 11, 10, 3, 2},    // front
      {12, 4, 13, 5, 15, 7, 14, 6}}; // back

    std::vector<CellData<3>> cells(n_cells, CellData<3>());

    for (unsigned int i = 0; i < n_cells; ++i)
      {
        for (unsigned int j = 0; j < GeometryInfo<3>::vertices_per_cell; ++j)
          cells[i].vertices[j] = cell_vertices[i][j];
        cells[i].material_id = 0;
        cells[i].manifold_id = i == 0 ? numbers::flat_manifold_id : 1;
      }

    tria.create_triangulation(std::vector<Point<3>>(std::begin(vertices),
                                                    std::end(vertices)),
                              cells,
                              SubCellData()); // no boundary information
    tria.set_all_manifold_ids_on_boundary(0);
    tria.set_manifold(0, SphericalManifold<3>(p));
    if (internal_manifold)
      tria.set_manifold(1, SphericalManifold<3>(p));
  }



  template <int spacedim>
  void hyper_sphere(Triangulation<spacedim - 1, spacedim> &tria,
                    const Point<spacedim> &                p,
                    const double                           radius)
  {
    Triangulation<spacedim> volume_mesh;
    GridGenerator::hyper_ball(volume_mesh, p, radius);
    std::set<types::boundary_id> boundary_ids;
    boundary_ids.insert(0);
    GridGenerator::extract_boundary_mesh(volume_mesh, tria, boundary_ids);
    tria.set_all_manifold_ids(0);
    tria.set_manifold(0, SphericalManifold<spacedim - 1, spacedim>(p));
  }



  // Implementation for 3D only
  template <>
  void cylinder(Triangulation<3> &tria,
                const double      radius,
                const double      half_length)
  {
    // Copy the base from hyper_ball<3>
    // and transform it to yz
    const double d            = radius / std::sqrt(2.0);
    const double a            = d / (1 + std::sqrt(2.0));
    Point<3>     vertices[24] = {
      Point<3>(-d, -half_length, -d),
      Point<3>(d, -half_length, -d),
      Point<3>(-a, -half_length, -a),
      Point<3>(a, -half_length, -a),
      Point<3>(-a, -half_length, a),
      Point<3>(a, -half_length, a),
      Point<3>(-d, -half_length, d),
      Point<3>(d, -half_length, d),
      Point<3>(-d, 0, -d),
      Point<3>(d, 0, -d),
      Point<3>(-a, 0, -a),
      Point<3>(a, 0, -a),
      Point<3>(-a, 0, a),
      Point<3>(a, 0, a),
      Point<3>(-d, 0, d),
      Point<3>(d, 0, d),
      Point<3>(-d, half_length, -d),
      Point<3>(d, half_length, -d),
      Point<3>(-a, half_length, -a),
      Point<3>(a, half_length, -a),
      Point<3>(-a, half_length, a),
      Point<3>(a, half_length, a),
      Point<3>(-d, half_length, d),
      Point<3>(d, half_length, d),
    };
    // Turn cylinder such that y->x
    for (auto &vertex : vertices)
      {
        const double h = vertex(1);
        vertex(1)      = -vertex(0);
        vertex(0)      = h;
      }

    int cell_vertices[10][8] = {{0, 1, 8, 9, 2, 3, 10, 11},
                                {0, 2, 8, 10, 6, 4, 14, 12},
                                {2, 3, 10, 11, 4, 5, 12, 13},
                                {1, 7, 9, 15, 3, 5, 11, 13},
                                {6, 4, 14, 12, 7, 5, 15, 13}};
    for (unsigned int i = 0; i < 5; ++i)
      for (unsigned int j = 0; j < 8; ++j)
        cell_vertices[i + 5][j] = cell_vertices[i][j] + 8;

    std::vector<CellData<3>> cells(10, CellData<3>());

    for (unsigned int i = 0; i < 10; ++i)
      {
        for (unsigned int j = 0; j < 8; ++j)
          cells[i].vertices[j] = cell_vertices[i][j];
        cells[i].material_id = 0;
      }

    tria.create_triangulation(std::vector<Point<3>>(std::begin(vertices),
                                                    std::end(vertices)),
                              cells,
                              SubCellData()); // no boundary information

    // set boundary indicators for the
    // faces at the ends to 1 and 2,
    // respectively. note that we also
    // have to deal with those lines
    // that are purely in the interior
    // of the ends. we determine whether
    // an edge is purely in the
    // interior if one of its vertices
    // is at coordinates '+-a' as set
    // above
    Triangulation<3>::cell_iterator cell = tria.begin();
    Triangulation<3>::cell_iterator end  = tria.end();

    tria.set_all_manifold_ids_on_boundary(0);

    for (; cell != end; ++cell)
      for (unsigned int i = 0; i < GeometryInfo<3>::faces_per_cell; ++i)
        if (cell->at_boundary(i))
          {
            if (cell->face(i)->center()(0) > half_length - 1.e-5)
              {
                cell->face(i)->set_boundary_id(2);
                cell->face(i)->set_manifold_id(numbers::flat_manifold_id);

                for (unsigned int e = 0; e < GeometryInfo<3>::lines_per_face;
                     ++e)
                  if ((std::fabs(cell->face(i)->line(e)->vertex(0)[1]) == a) ||
                      (std::fabs(cell->face(i)->line(e)->vertex(0)[2]) == a) ||
                      (std::fabs(cell->face(i)->line(e)->vertex(1)[1]) == a) ||
                      (std::fabs(cell->face(i)->line(e)->vertex(1)[2]) == a))
                    {
                      cell->face(i)->line(e)->set_boundary_id(2);
                      cell->face(i)->line(e)->set_manifold_id(
                        numbers::flat_manifold_id);
                    }
              }
            else if (cell->face(i)->center()(0) < -half_length + 1.e-5)
              {
                cell->face(i)->set_boundary_id(1);
                cell->face(i)->set_manifold_id(numbers::flat_manifold_id);

                for (unsigned int e = 0; e < GeometryInfo<3>::lines_per_face;
                     ++e)
                  if ((std::fabs(cell->face(i)->line(e)->vertex(0)[1]) == a) ||
                      (std::fabs(cell->face(i)->line(e)->vertex(0)[2]) == a) ||
                      (std::fabs(cell->face(i)->line(e)->vertex(1)[1]) == a) ||
                      (std::fabs(cell->face(i)->line(e)->vertex(1)[2]) == a))
                    {
                      cell->face(i)->line(e)->set_boundary_id(1);
                      cell->face(i)->line(e)->set_manifold_id(
                        numbers::flat_manifold_id);
                    }
              }
          }
    tria.set_manifold(0, CylindricalManifold<3>());
  }


  template <>
  void quarter_hyper_ball(Triangulation<3> &tria,
                          const Point<3> &  center,
                          const double      radius)
  {
    const unsigned int dim = 3;

    // equilibrate cell sizes at
    // transition from the inner part
    // to the radial cells
    const Point<dim> vertices[15] = {
      center + Point<dim>(0, 0, 0) * radius,
      center + Point<dim>(+1, 0, 0) * radius,
      center + Point<dim>(+1, 0, 0) * (radius / 2.),
      center + Point<dim>(0, +1, 0) * (radius / 2.),
      center + Point<dim>(+1, +1, 0) * (radius / (2 * std::sqrt(2.0))),
      center + Point<dim>(0, +1, 0) * radius,
      center + Point<dim>(+1, +1, 0) * (radius / std::sqrt(2.0)),
      center + Point<dim>(0, 0, 1) * radius / 2.,
      center + Point<dim>(+1, 0, 1) * radius / std::sqrt(2.0),
      center + Point<dim>(+1, 0, 1) * (radius / (2 * std::sqrt(2.0))),
      center + Point<dim>(0, +1, 1) * (radius / (2 * std::sqrt(2.0))),
      center + Point<dim>(+1, +1, 1) * (radius / (2 * std::sqrt(3.0))),
      center + Point<dim>(0, +1, 1) * radius / std::sqrt(2.0),
      center + Point<dim>(+1, +1, 1) * (radius / (std::sqrt(3.0))),
      center + Point<dim>(0, 0, 1) * radius};
    const int cell_vertices[4][8] = {{0, 2, 3, 4, 7, 9, 10, 11},
                                     {1, 6, 2, 4, 8, 13, 9, 11},
                                     {5, 3, 6, 4, 12, 10, 13, 11},
                                     {7, 9, 10, 11, 14, 8, 12, 13}};

    std::vector<CellData<dim>> cells(4, CellData<dim>());

    for (unsigned int i = 0; i < 4; ++i)
      {
        for (unsigned int j = 0; j < 8; ++j)
          cells[i].vertices[j] = cell_vertices[i][j];
        cells[i].material_id = 0;
      }

    tria.create_triangulation(std::vector<Point<dim>>(std::begin(vertices),
                                                      std::end(vertices)),
                              cells,
                              SubCellData()); // no boundary information

    Triangulation<dim>::cell_iterator cell = tria.begin();
    Triangulation<dim>::cell_iterator end  = tria.end();

    tria.set_all_manifold_ids_on_boundary(0);
    while (cell != end)
      {
        for (unsigned int i = 0; i < GeometryInfo<dim>::faces_per_cell; ++i)
          {
            if (cell->face(i)->boundary_id() ==
                numbers::internal_face_boundary_id)
              continue;

            // If x,y or z is zero, then this is part of the plane
            if (cell->face(i)->center()(0) < center(0) + 1.e-5 * radius ||
                cell->face(i)->center()(1) < center(1) + 1.e-5 * radius ||
                cell->face(i)->center()(2) < center(2) + 1.e-5 * radius)
              {
                cell->face(i)->set_boundary_id(1);
                cell->face(i)->set_manifold_id(numbers::flat_manifold_id);
                // also set the boundary indicators of the bounding lines,
                // unless both vertices are on the perimeter
                for (unsigned int j = 0; j < GeometryInfo<3>::lines_per_face;
                     ++j)
                  {
                    const Point<3> line_vertices[2] = {
                      cell->face(i)->line(j)->vertex(0),
                      cell->face(i)->line(j)->vertex(1)};
                    if ((std::fabs(line_vertices[0].distance(center) - radius) >
                         1e-5 * radius) ||
                        (std::fabs(line_vertices[1].distance(center) - radius) >
                         1e-5 * radius))
                      {
                        cell->face(i)->line(j)->set_boundary_id(1);
                        cell->face(i)->line(j)->set_manifold_id(
                          numbers::flat_manifold_id);
                      }
                  }
              }
          }
        ++cell;
      }
    tria.set_manifold(0, SphericalManifold<3>(center));
  }


  // Implementation for 3D only
  template <>
  void half_hyper_ball(Triangulation<3> &tria,
                       const Point<3> &  center,
                       const double      radius)
  {
    // These are for the two lower squares
    const double d = radius / std::sqrt(2.0);
    const double a = d / (1 + std::sqrt(2.0));
    // These are for the two upper square
    const double b = a / 2.0;
    const double c = d / 2.0;
    // And so are these
    const double hb = radius * std::sqrt(3.0) / 4.0;
    const double hc = radius * std::sqrt(3.0) / 2.0;

    Point<3> vertices[16] = {
      center + Point<3>(0, d, -d),
      center + Point<3>(0, -d, -d),
      center + Point<3>(0, a, -a),
      center + Point<3>(0, -a, -a),
      center + Point<3>(0, a, a),
      center + Point<3>(0, -a, a),
      center + Point<3>(0, d, d),
      center + Point<3>(0, -d, d),

      center + Point<3>(hc, c, -c),
      center + Point<3>(hc, -c, -c),
      center + Point<3>(hb, b, -b),
      center + Point<3>(hb, -b, -b),
      center + Point<3>(hb, b, b),
      center + Point<3>(hb, -b, b),
      center + Point<3>(hc, c, c),
      center + Point<3>(hc, -c, c),
    };

    int cell_vertices[6][8] = {{0, 1, 8, 9, 2, 3, 10, 11},
                               {0, 2, 8, 10, 6, 4, 14, 12},
                               {2, 3, 10, 11, 4, 5, 12, 13},
                               {1, 7, 9, 15, 3, 5, 11, 13},
                               {6, 4, 14, 12, 7, 5, 15, 13},
                               {8, 10, 9, 11, 14, 12, 15, 13}};

    std::vector<CellData<3>> cells(6, CellData<3>());

    for (unsigned int i = 0; i < 6; ++i)
      {
        for (unsigned int j = 0; j < 8; ++j)
          cells[i].vertices[j] = cell_vertices[i][j];
        cells[i].material_id = 0;
      }

    tria.create_triangulation(std::vector<Point<3>>(std::begin(vertices),
                                                    std::end(vertices)),
                              cells,
                              SubCellData()); // no boundary information

    Triangulation<3>::cell_iterator cell = tria.begin();
    Triangulation<3>::cell_iterator end  = tria.end();

    tria.set_all_manifold_ids_on_boundary(0);

    // go over all faces. for the ones on the flat face, set boundary
    // indicator for face and edges to one; the rest will remain at
    // zero but we have to pay attention to those edges that are
    // at the perimeter of the flat face since they should not be
    // set to one
    while (cell != end)
      {
        for (unsigned int i = 0; i < GeometryInfo<3>::faces_per_cell; ++i)
          {
            if (!cell->at_boundary(i))
              continue;

            // If the center is on the plane x=0, this is a planar element. set
            // its boundary indicator. also set the boundary indicators of the
            // bounding faces unless both vertices are on the perimeter
            if (cell->face(i)->center()(0) < center(0) + 1.e-5 * radius)
              {
                cell->face(i)->set_boundary_id(1);
                cell->face(i)->set_manifold_id(numbers::flat_manifold_id);
                for (unsigned int j = 0; j < GeometryInfo<3>::lines_per_face;
                     ++j)
                  {
                    const Point<3> line_vertices[2] = {
                      cell->face(i)->line(j)->vertex(0),
                      cell->face(i)->line(j)->vertex(1)};
                    if ((std::fabs(line_vertices[0].distance(center) - radius) >
                         1e-5 * radius) ||
                        (std::fabs(line_vertices[1].distance(center) - radius) >
                         1e-5 * radius))
                      {
                        cell->face(i)->line(j)->set_boundary_id(1);
                        cell->face(i)->line(j)->set_manifold_id(
                          numbers::flat_manifold_id);
                      }
                  }
              }
          }
        ++cell;
      }
    tria.set_manifold(0, SphericalManifold<3>(center));
  }


  template <>
  void hyper_shell(Triangulation<3> & tria,
                   const Point<3> &   p,
                   const double       inner_radius,
                   const double       outer_radius,
                   const unsigned int n_cells,
                   const bool         colorize)
  {
    Assert((inner_radius > 0) && (inner_radius < outer_radius),
           ExcInvalidRadii());

    const unsigned int n = (n_cells == 0) ? 6 : n_cells;

    const double             irad = inner_radius / std::sqrt(3.0);
    const double             orad = outer_radius / std::sqrt(3.0);
    std::vector<Point<3>>    vertices;
    std::vector<CellData<3>> cells;

    // Corner points of the cube [-1,1]^3
    static const std::array<Point<3>, 8> hexahedron = {{{-1, -1, -1}, //
                                                        {+1, -1, -1}, //
                                                        {-1, +1, -1}, //
                                                        {+1, +1, -1}, //
                                                        {-1, -1, +1}, //
                                                        {+1, -1, +1}, //
                                                        {-1, +1, +1}, //
                                                        {+1, +1, +1}}};

    // Start with the shell bounded by
    // two nested cubes
    if (n == 6)
      {
        for (unsigned int i = 0; i < 8; ++i)
          vertices.push_back(p + hexahedron[i] * irad);
        for (unsigned int i = 0; i < 8; ++i)
          vertices.push_back(p + hexahedron[i] * orad);

        const unsigned int n_cells                   = 6;
        const int          cell_vertices[n_cells][8] = {
          {8, 9, 10, 11, 0, 1, 2, 3},    // bottom
          {9, 11, 1, 3, 13, 15, 5, 7},   // right
          {12, 13, 4, 5, 14, 15, 6, 7},  // top
          {8, 0, 10, 2, 12, 4, 14, 6},   // left
          {8, 9, 0, 1, 12, 13, 4, 5},    // front
          {10, 2, 11, 3, 14, 6, 15, 7}}; // back

        cells.resize(n_cells, CellData<3>());

        for (unsigned int i = 0; i < n_cells; ++i)
          {
            for (unsigned int j = 0; j < GeometryInfo<3>::vertices_per_cell;
                 ++j)
              cells[i].vertices[j] = cell_vertices[i][j];
            cells[i].material_id = 0;
          }

        tria.create_triangulation(vertices, cells, SubCellData());
      }
    // A more regular subdivision can
    // be obtained by two nested
    // rhombic dodecahedra

    else if (n == 12)
      {
        // Octahedron inscribed in the cube [-1,1]^3
        static const std::array<Point<3>, 6> octahedron = {{{-1, 0, 0}, //
                                                            {1, 0, 0},  //
                                                            {0, -1, 0}, //
                                                            {0, 1, 0},  //
                                                            {0, 0, -1}, //
                                                            {0, 0, 1}}};

        for (unsigned int i = 0; i < 8; ++i)
          vertices.push_back(p + hexahedron[i] * irad);
        for (unsigned int i = 0; i < 6; ++i)
          vertices.push_back(p + octahedron[i] * inner_radius);
        for (unsigned int i = 0; i < 8; ++i)
          vertices.push_back(p + hexahedron[i] * orad);
        for (unsigned int i = 0; i < 6; ++i)
          vertices.push_back(p + octahedron[i] * outer_radius);

        const unsigned int n_cells            = 12;
        const unsigned int rhombi[n_cells][4] = {{10, 4, 0, 8},
                                                 {4, 13, 8, 6},
                                                 {10, 5, 4, 13},
                                                 {1, 9, 10, 5},
                                                 {9, 7, 5, 13},
                                                 {7, 11, 13, 6},
                                                 {9, 3, 7, 11},
                                                 {1, 12, 9, 3},
                                                 {12, 2, 3, 11},
                                                 {2, 8, 11, 6},
                                                 {12, 0, 2, 8},
                                                 {1, 10, 12, 0}};

        cells.resize(n_cells, CellData<3>());

        for (unsigned int i = 0; i < n_cells; ++i)
          {
            for (unsigned int j = 0; j < 4; ++j)
              {
                cells[i].vertices[j]     = rhombi[i][j];
                cells[i].vertices[j + 4] = rhombi[i][j] + 14;
              }
            cells[i].material_id = 0;
          }

        tria.create_triangulation(vertices, cells, SubCellData());
      }
    else if (n == 96)
      {
        // create a triangulation based on the 12-cell version. This function
        // was needed before SphericalManifold was written: it manually
        // adjusted the interior vertices to lie along concentric
        // spheres. Nowadays we can just refine globally:
        Triangulation<3> tmp;
        hyper_shell(tmp, p, inner_radius, outer_radius, 12);
        tmp.refine_global(1);

        // now copy the resulting level 1 cells into the new triangulation,
        cells.resize(tmp.n_active_cells(), CellData<3>());
        for (Triangulation<3>::active_cell_iterator cell = tmp.begin_active();
             cell != tmp.end();
             ++cell)
          {
            const unsigned int cell_index = cell->active_cell_index();
            for (unsigned int v = 0; v < GeometryInfo<3>::vertices_per_cell;
                 ++v)
              cells[cell_index].vertices[v] = cell->vertex_index(v);
            cells[cell_index].material_id = 0;
          }

        tria.create_triangulation(tmp.get_vertices(), cells, SubCellData());
      }
    else
      {
        Assert(false, ExcMessage("Invalid number of coarse mesh cells."));
      }

    if (colorize)
      colorize_hyper_shell(tria, p, inner_radius, outer_radius);
    tria.set_all_manifold_ids(0);
    tria.set_manifold(0, SphericalManifold<3>(p));
  }



  // Implementation for 3D only
  template <>
  void half_hyper_shell(Triangulation<3> & tria,
                        const Point<3> &   center,
                        const double       inner_radius,
                        const double       outer_radius,
                        const unsigned int n,
                        const bool         colorize)
  {
    Assert((inner_radius > 0) && (inner_radius < outer_radius),
           ExcInvalidRadii());

    if (n <= 5)
      {
        // These are for the two lower squares
        const double d = outer_radius / std::sqrt(2.0);
        const double a = inner_radius / std::sqrt(2.0);
        // These are for the two upper square
        const double b = a / 2.0;
        const double c = d / 2.0;
        // And so are these
        const double hb = inner_radius * std::sqrt(3.0) / 2.0;
        const double hc = outer_radius * std::sqrt(3.0) / 2.0;

        Point<3> vertices[16] = {
          center + Point<3>(0, d, -d),
          center + Point<3>(0, -d, -d),
          center + Point<3>(0, a, -a),
          center + Point<3>(0, -a, -a),
          center + Point<3>(0, a, a),
          center + Point<3>(0, -a, a),
          center + Point<3>(0, d, d),
          center + Point<3>(0, -d, d),

          center + Point<3>(hc, c, -c),
          center + Point<3>(hc, -c, -c),
          center + Point<3>(hb, b, -b),
          center + Point<3>(hb, -b, -b),
          center + Point<3>(hb, b, b),
          center + Point<3>(hb, -b, b),
          center + Point<3>(hc, c, c),
          center + Point<3>(hc, -c, c),
        };

        int cell_vertices[5][8] = {{0, 1, 8, 9, 2, 3, 10, 11},
                                   {0, 2, 8, 10, 6, 4, 14, 12},
                                   {1, 7, 9, 15, 3, 5, 11, 13},
                                   {6, 4, 14, 12, 7, 5, 15, 13},
                                   {8, 10, 9, 11, 14, 12, 15, 13}};

        std::vector<CellData<3>> cells(5, CellData<3>());

        for (unsigned int i = 0; i < 5; ++i)
          {
            for (unsigned int j = 0; j < 8; ++j)
              cells[i].vertices[j] = cell_vertices[i][j];
            cells[i].material_id = 0;
          }

        tria.create_triangulation(std::vector<Point<3>>(std::begin(vertices),
                                                        std::end(vertices)),
                                  cells,
                                  SubCellData()); // no boundary information
      }
    else
      {
        Assert(false, ExcIndexRange(n, 0, 5));
      }
    if (colorize)
      {
        // We want to use a standard boundary description where
        // the boundary is not curved. Hence set boundary id 2 to
        // to all faces in a first step.
        Triangulation<3>::cell_iterator cell = tria.begin();
        for (; cell != tria.end(); ++cell)
          for (unsigned int i = 0; i < GeometryInfo<3>::faces_per_cell; ++i)
            if (cell->at_boundary(i))
              cell->face(i)->set_all_boundary_ids(2);

        // Next look for the curved boundaries. If the x value of the
        // center of the face is not equal to center(0), we're on a curved
        // boundary. Then decide whether the center is nearer to the inner
        // or outer boundary to set the correct boundary id.
        for (cell = tria.begin(); cell != tria.end(); ++cell)
          for (unsigned int i = 0; i < GeometryInfo<3>::faces_per_cell; ++i)
            if (cell->at_boundary(i))
              {
                const Triangulation<3>::face_iterator face = cell->face(i);

                const Point<3> face_center(face->center());
                if (std::abs(face_center(0) - center(0)) >
                    1.e-6 * face_center.norm())
                  {
                    if (std::abs((face_center - center).norm() - inner_radius) <
                        std::abs((face_center - center).norm() - outer_radius))
                      face->set_all_boundary_ids(0);
                    else
                      face->set_all_boundary_ids(1);
                  }
              }
      }
    tria.set_all_manifold_ids(0);
    tria.set_manifold(0, SphericalManifold<3>(center));
  }


  // Implementation for 3D only
  template <>
  void quarter_hyper_shell(Triangulation<3> & tria,
                           const Point<3> &   center,
                           const double       inner_radius,
                           const double       outer_radius,
                           const unsigned int n,
                           const bool         colorize)
  {
    Assert((inner_radius > 0) && (inner_radius < outer_radius),
           ExcInvalidRadii());
    if (n == 0 || n == 3)
      {
        const double a = inner_radius * std::sqrt(2.0) / 2e0;
        const double b = outer_radius * std::sqrt(2.0) / 2e0;
        const double c = a * std::sqrt(3.0) / 2e0;
        const double d = b * std::sqrt(3.0) / 2e0;
        const double e = outer_radius / 2e0;
        const double h = inner_radius / 2e0;

        std::vector<Point<3>> vertices;

        vertices.push_back(center + Point<3>(0, inner_radius, 0)); // 0
        vertices.push_back(center + Point<3>(a, a, 0));            // 1
        vertices.push_back(center + Point<3>(b, b, 0));            // 2
        vertices.push_back(center + Point<3>(0, outer_radius, 0)); // 3
        vertices.push_back(center + Point<3>(0, a, a));            // 4
        vertices.push_back(center + Point<3>(c, c, h));            // 5
        vertices.push_back(center + Point<3>(d, d, e));            // 6
        vertices.push_back(center + Point<3>(0, b, b));            // 7
        vertices.push_back(center + Point<3>(inner_radius, 0, 0)); // 8
        vertices.push_back(center + Point<3>(outer_radius, 0, 0)); // 9
        vertices.push_back(center + Point<3>(a, 0, a));            // 10
        vertices.push_back(center + Point<3>(b, 0, b));            // 11
        vertices.push_back(center + Point<3>(0, 0, inner_radius)); // 12
        vertices.push_back(center + Point<3>(0, 0, outer_radius)); // 13

        const int cell_vertices[3][8] = {
          {0, 1, 3, 2, 4, 5, 7, 6},
          {1, 8, 2, 9, 5, 10, 6, 11},
          {4, 5, 7, 6, 12, 10, 13, 11},
        };
        std::vector<CellData<3>> cells(3);

        for (unsigned int i = 0; i < 3; ++i)
          {
            for (unsigned int j = 0; j < 8; ++j)
              cells[i].vertices[j] = cell_vertices[i][j];
            cells[i].material_id = 0;
          }

        tria.create_triangulation(vertices,
                                  cells,
                                  SubCellData()); // no boundary information
      }
    else
      {
        AssertThrow(false, ExcNotImplemented());
      }

    if (colorize)
      colorize_quarter_hyper_shell(tria, center, inner_radius, outer_radius);

    tria.set_all_manifold_ids(0);
    tria.set_manifold(0, SphericalManifold<3>(center));
  }


  // Implementation for 3D only
  template <>
  void cylinder_shell(Triangulation<3> & tria,
                      const double       length,
                      const double       inner_radius,
                      const double       outer_radius,
                      const unsigned int n_radial_cells,
                      const unsigned int n_axial_cells)
  {
    Assert((inner_radius > 0) && (inner_radius < outer_radius),
           ExcInvalidRadii());

    const double pi = numbers::PI;

    // determine the number of cells
    // for the grid. if not provided by
    // the user determine it such that
    // the length of each cell on the
    // median (in the middle between
    // the two circles) is equal to its
    // radial extent (which is the
    // difference between the two
    // radii)
    const unsigned int N_r =
      (n_radial_cells == 0 ? static_cast<unsigned int>(std::ceil(
                               (2 * pi * (outer_radius + inner_radius) / 2) /
                               (outer_radius - inner_radius))) :
                             n_radial_cells);
    const unsigned int N_z =
      (n_axial_cells == 0 ?
         static_cast<unsigned int>(std::ceil(
           length / (2 * pi * (outer_radius + inner_radius) / 2 / N_r))) :
         n_axial_cells);

    // set up N vertices on the
    // outer and N vertices on
    // the inner circle. the
    // first N ones are on the
    // outer one, and all are
    // numbered counter-clockwise
    std::vector<Point<2>> vertices_2d(2 * N_r);
    for (unsigned int i = 0; i < N_r; ++i)
      {
        vertices_2d[i] =
          Point<2>(std::cos(2 * pi * i / N_r), std::sin(2 * pi * i / N_r)) *
          outer_radius;
        vertices_2d[i + N_r] = vertices_2d[i] * (inner_radius / outer_radius);
      }

    std::vector<Point<3>> vertices_3d;
    vertices_3d.reserve(2 * N_r * (N_z + 1));
    for (unsigned int j = 0; j <= N_z; ++j)
      for (unsigned int i = 0; i < 2 * N_r; ++i)
        {
          const Point<3> v(vertices_2d[i][0],
                           vertices_2d[i][1],
                           j * length / N_z);
          vertices_3d.push_back(v);
        }

    std::vector<CellData<3>> cells(N_r * N_z, CellData<3>());

    for (unsigned int j = 0; j < N_z; ++j)
      for (unsigned int i = 0; i < N_r; ++i)
        {
          cells[i + j * N_r].vertices[0] = i + (j + 1) * 2 * N_r;
          cells[i + j * N_r].vertices[1] = (i + 1) % N_r + (j + 1) * 2 * N_r;
          cells[i + j * N_r].vertices[2] = i + j * 2 * N_r;
          cells[i + j * N_r].vertices[3] = (i + 1) % N_r + j * 2 * N_r;

          cells[i + j * N_r].vertices[4] = N_r + i + (j + 1) * 2 * N_r;
          cells[i + j * N_r].vertices[5] =
            N_r + ((i + 1) % N_r) + (j + 1) * 2 * N_r;
          cells[i + j * N_r].vertices[6] = N_r + i + j * 2 * N_r;
          cells[i + j * N_r].vertices[7] = N_r + ((i + 1) % N_r) + j * 2 * N_r;

          cells[i + j * N_r].material_id = 0;
        }

    tria.create_triangulation(vertices_3d, cells, SubCellData());
    tria.set_all_manifold_ids(0);
    tria.set_manifold(0, CylindricalManifold<3>(2));
  }



  template <int dim, int spacedim>
  void
  merge_triangulations(
    const std::initializer_list<const Triangulation<dim, spacedim> *const>
      &                           triangulations,
    Triangulation<dim, spacedim> &result,
    const double                  duplicated_vertex_tolerance,
    const bool                    copy_manifold_ids)
  {
    std::vector<Point<spacedim>> vertices;
    std::vector<CellData<dim>>   cells;
    SubCellData                  subcell_data;

    unsigned int n_accumulated_vertices = 0;
    for (const auto triangulation : triangulations)
      {
        Assert(triangulation->n_levels() == 1,
               ExcMessage("The input triangulations must be non-empty "
                          "and must not be refined."));

        std::vector<Point<spacedim>> tria_vertices;
        std::vector<CellData<dim>>   tria_cells;
        SubCellData                  tria_subcell_data;
        std::tie(tria_vertices, tria_cells, tria_subcell_data) =
          GridTools::get_coarse_mesh_description(*triangulation);

        vertices.insert(vertices.end(),
                        tria_vertices.begin(),
                        tria_vertices.end());
        for (CellData<dim> &cell_data : tria_cells)
          {
            for (unsigned int &vertex_n : cell_data.vertices)
              vertex_n += n_accumulated_vertices;
            cells.push_back(cell_data);
          }

        // Skip copying lines with no manifold information.
        if (copy_manifold_ids)
          {
            for (CellData<1> &line_data : tria_subcell_data.boundary_lines)
              {
                if (line_data.manifold_id == numbers::flat_manifold_id)
                  continue;
                for (unsigned int &vertex_n : line_data.vertices)
                  vertex_n += n_accumulated_vertices;
                line_data.boundary_id =
                  numbers::internal_face_boundary_id; // default
                subcell_data.boundary_lines.push_back(line_data);
              }

            for (CellData<2> &quad_data : tria_subcell_data.boundary_quads)
              {
                if (quad_data.manifold_id == numbers::flat_manifold_id)
                  continue;
                for (unsigned int &vertex_n : quad_data.vertices)
                  vertex_n += n_accumulated_vertices;
                quad_data.boundary_id =
                  numbers::internal_face_boundary_id; // default
                subcell_data.boundary_quads.push_back(quad_data);
              }
          }

        n_accumulated_vertices += triangulation->n_vertices();
      }

    // throw out duplicated vertices
    std::vector<unsigned int> considered_vertices;
    GridTools::delete_duplicated_vertices(vertices,
                                          cells,
                                          subcell_data,
                                          considered_vertices,
                                          duplicated_vertex_tolerance);

    // reorder the cells to ensure that they satisfy the convention for
    // edge and face directions
    GridReordering<dim, spacedim>::reorder_cells(cells, true);
    result.clear();
    result.create_triangulation(vertices, cells, subcell_data);
  }



  template <int dim, int spacedim>
  void
  merge_triangulations(const Triangulation<dim, spacedim> &triangulation_1,
                       const Triangulation<dim, spacedim> &triangulation_2,
                       Triangulation<dim, spacedim> &      result,
                       const double duplicated_vertex_tolerance,
                       const bool   copy_manifold_ids)
  {
    // if either Triangulation is empty then merging is just a copy.
    if (triangulation_1.n_cells() == 0)
      {
        result.copy_triangulation(triangulation_2);
        return;
      }
    if (triangulation_2.n_cells() == 0)
      {
        result.copy_triangulation(triangulation_1);
        return;
      }
    merge_triangulations({&triangulation_1, &triangulation_2},
                         result,
                         duplicated_vertex_tolerance,
                         copy_manifold_ids);
  }



  template <int dim, int spacedim>
  void
  create_union_triangulation(
    const Triangulation<dim, spacedim> &triangulation_1,
    const Triangulation<dim, spacedim> &triangulation_2,
    Triangulation<dim, spacedim> &      result)
  {
    Assert(GridTools::have_same_coarse_mesh(triangulation_1, triangulation_2),
           ExcMessage("The two input triangulations are not derived from "
                      "the same coarse mesh as required."));
    Assert((dynamic_cast<
              const parallel::distributed::Triangulation<dim, spacedim> *>(
              &triangulation_1) == nullptr) &&
             (dynamic_cast<
                const parallel::distributed::Triangulation<dim, spacedim> *>(
                &triangulation_2) == nullptr),
           ExcMessage("The source triangulations for this function must both "
                      "be available entirely locally, and not be distributed "
                      "triangulations."));

    // first copy triangulation_1, and
    // then do as many iterations as
    // there are levels in
    // triangulation_2 to refine
    // additional cells. since this is
    // the maximum number of
    // refinements to get from the
    // coarse grid to triangulation_2,
    // it is clear that this is also
    // the maximum number of
    // refinements to get from any cell
    // on triangulation_1 to
    // triangulation_2
    result.clear();
    result.copy_triangulation(triangulation_1);
    for (unsigned int iteration = 0; iteration < triangulation_2.n_levels();
         ++iteration)
      {
        InterGridMap<Triangulation<dim, spacedim>> intergrid_map;
        intergrid_map.make_mapping(result, triangulation_2);

        bool any_cell_flagged = false;
        for (typename Triangulation<dim, spacedim>::active_cell_iterator
               result_cell = result.begin_active();
             result_cell != result.end();
             ++result_cell)
          if (intergrid_map[result_cell]->has_children())
            {
              any_cell_flagged = true;
              result_cell->set_refine_flag();
            }

        if (any_cell_flagged == false)
          break;
        else
          result.execute_coarsening_and_refinement();
      }
  }



  template <int dim, int spacedim>
  void
  create_triangulation_with_removed_cells(
    const Triangulation<dim, spacedim> &input_triangulation,
    const std::set<typename Triangulation<dim, spacedim>::active_cell_iterator>
      &                           cells_to_remove,
    Triangulation<dim, spacedim> &result)
  {
    // simply copy the vertices; we will later strip those
    // that turn out to be unused
    std::vector<Point<spacedim>> vertices = input_triangulation.get_vertices();

    // the loop through the cells and copy stuff, excluding
    // the ones we are to remove
    std::vector<CellData<dim>> cells;
    for (typename Triangulation<dim, spacedim>::active_cell_iterator cell =
           input_triangulation.begin_active();
         cell != input_triangulation.end();
         ++cell)
      if (cells_to_remove.find(cell) == cells_to_remove.end())
        {
          Assert(static_cast<unsigned int>(cell->level()) ==
                   input_triangulation.n_levels() - 1,
                 ExcMessage(
                   "Your input triangulation appears to have "
                   "adaptively refined cells. This is not allowed. You can "
                   "only call this function on a triangulation in which "
                   "all cells are on the same refinement level."));

          CellData<dim> this_cell;
          for (unsigned int v = 0; v < GeometryInfo<dim>::vertices_per_cell;
               ++v)
            this_cell.vertices[v] = cell->vertex_index(v);
          this_cell.material_id = cell->material_id();
          cells.push_back(this_cell);
        }

    // throw out duplicated vertices from the two meshes, reorder vertices as
    // necessary and create the triangulation
    SubCellData               subcell_data;
    std::vector<unsigned int> considered_vertices;
    GridTools::delete_duplicated_vertices(vertices,
                                          cells,
                                          subcell_data,
                                          considered_vertices);

    // then clear the old triangulation and create the new one
    result.clear();
    result.create_triangulation(vertices, cells, subcell_data);
  }



  void
  extrude_triangulation(
    const Triangulation<2, 2> &            input,
    const unsigned int                     n_slices,
    const double                           height,
    Triangulation<3, 3> &                  result,
    const bool                             copy_manifold_ids,
    const std::vector<types::manifold_id> &manifold_priorities)
  {
    Assert(input.n_levels() == 1,
           ExcMessage(
             "The input triangulation must be a coarse mesh, i.e., it must "
             "not have been refined."));
    Assert(result.n_cells() == 0,
           ExcMessage("The output triangulation object needs to be empty."));
    Assert(height > 0,
           ExcMessage("The given height for extrusion must be positive."));
    Assert(n_slices >= 2,
           ExcMessage(
             "The number of slices for extrusion must be at least 2."));

    const double        delta_h = height / (n_slices - 1);
    std::vector<double> slices_z_values;
    for (unsigned int i = 0; i < n_slices; ++i)
      slices_z_values.push_back(i * delta_h);
    extrude_triangulation(
      input, slices_z_values, result, copy_manifold_ids, manifold_priorities);
  }



  void
  extrude_triangulation(
    const Triangulation<2, 2> &            input,
    const std::vector<double> &            slice_coordinates,
    Triangulation<3, 3> &                  result,
    const bool                             copy_manifold_ids,
    const std::vector<types::manifold_id> &manifold_priorities)
  {
    Assert(input.n_levels() == 1,
           ExcMessage(
             "The input triangulation must be a coarse mesh, i.e., it must "
             "not have been refined."));
    Assert(result.n_cells() == 0,
           ExcMessage("The output triangulation object needs to be empty."));
    Assert(slice_coordinates.size() >= 2,
           ExcMessage(
             "The number of slices for extrusion must be at least 2."));
    Assert(std::is_sorted(slice_coordinates.begin(), slice_coordinates.end()),
           ExcMessage("Slice z-coordinates should be in ascending order"));

    const auto priorities = [&]() -> std::vector<types::manifold_id> {
      // if a non-empty (i.e., not the default) vector is given for
      // manifold_priorities then use it (but check its validity in debug
      // mode)
      if (0 < manifold_priorities.size())
        {
#ifdef DEBUG
          // check that the provided manifold_priorities is valid
          std::vector<types::manifold_id> sorted_manifold_priorities =
            manifold_priorities;
          std::sort(sorted_manifold_priorities.begin(),
                    sorted_manifold_priorities.end());
          Assert(std::unique(sorted_manifold_priorities.begin(),
                             sorted_manifold_priorities.end()) ==
                   sorted_manifold_priorities.end(),
                 ExcMessage(
                   "The given vector of manifold ids may not contain any "
                   "duplicated entries."));
          std::vector<types::manifold_id> sorted_manifold_ids =
            input.get_manifold_ids();
          std::sort(sorted_manifold_ids.begin(), sorted_manifold_ids.end());
          if (sorted_manifold_priorities != sorted_manifold_ids)
            {
              std::ostringstream message;
              message << "The given triangulation has manifold ids {";
              for (const types::manifold_id manifold_id : sorted_manifold_ids)
                if (manifold_id != sorted_manifold_ids.back())
                  message << manifold_id << ", ";
              message << sorted_manifold_ids.back() << "}, but \n"
                      << "    the given vector of manifold ids is {";
              for (const types::manifold_id manifold_id : manifold_priorities)
                if (manifold_id != manifold_priorities.back())
                  message << manifold_id << ", ";
              message
                << manifold_priorities.back() << "}.\n"
                << "    These vectors should contain the same elements.\n";
              const std::string m = message.str();
              Assert(false, ExcMessage(m));
            }
#endif
          return manifold_priorities;
        }
      // otherwise use the default ranking: ascending order, but TFI manifolds
      // are at the end.
      std::vector<types::manifold_id> default_priorities =
        input.get_manifold_ids();
      const auto first_tfi_it = std::partition(
        default_priorities.begin(),
        default_priorities.end(),
        [&input](const types::manifold_id &id) {
          return dynamic_cast<const TransfiniteInterpolationManifold<2, 2> *>(
                   &input.get_manifold(id)) == nullptr;
        });
      std::sort(default_priorities.begin(), first_tfi_it);
      std::sort(first_tfi_it, default_priorities.end());

      return default_priorities;
    }();

    const std::size_t        n_slices = slice_coordinates.size();
    std::vector<Point<3>>    points(n_slices * input.n_vertices());
    std::vector<CellData<3>> cells;
    cells.reserve((n_slices - 1) * input.n_active_cells());

    // copy the array of points as many times as there will be slices,
    // one slice at a time. The z-axis value are defined in slices_coordinates
    for (std::size_t slice_n = 0; slice_n < n_slices; ++slice_n)
      {
        for (std::size_t vertex_n = 0; vertex_n < input.n_vertices();
             ++vertex_n)
          {
            const Point<2> vertex = input.get_vertices()[vertex_n];
            points[slice_n * input.n_vertices() + vertex_n] =
              Point<3>(vertex[0], vertex[1], slice_coordinates[slice_n]);
          }
      }

    // then create the cells of each of the slices, one stack at a
    // time
    for (const auto &cell : input.active_cell_iterators())
      {
        for (std::size_t slice_n = 0; slice_n < n_slices - 1; ++slice_n)
          {
            CellData<3> this_cell;
            for (unsigned int vertex_n = 0;
                 vertex_n < GeometryInfo<2>::vertices_per_cell;
                 ++vertex_n)
              {
                this_cell.vertices[vertex_n] =
                  cell->vertex_index(vertex_n) + slice_n * input.n_vertices();
                this_cell
                  .vertices[vertex_n + GeometryInfo<2>::vertices_per_cell] =
                  cell->vertex_index(vertex_n) +
                  (slice_n + 1) * input.n_vertices();
              }

            this_cell.material_id = cell->material_id();
            if (copy_manifold_ids)
              this_cell.manifold_id = cell->manifold_id();
            cells.push_back(this_cell);
          }
      }

    // Next, create face data for all faces that are orthogonal to the x-y
    // plane
    SubCellData               subcell_data;
    std::vector<CellData<2>> &quads           = subcell_data.boundary_quads;
    types::boundary_id        max_boundary_id = 0;
    quads.reserve(input.n_active_lines() * (n_slices - 1) +
                  input.n_active_cells() * 2);
    for (const auto &face : input.active_face_iterators())
      {
        CellData<2> quad;
        quad.boundary_id = face->boundary_id();
        if (face->at_boundary())
          max_boundary_id = std::max(max_boundary_id, quad.boundary_id);
        if (copy_manifold_ids)
          quad.manifold_id = face->manifold_id();
        for (std::size_t slice_n = 0; slice_n < n_slices - 1; ++slice_n)
          {
            quad.vertices[0] =
              face->vertex_index(0) + slice_n * input.n_vertices();
            quad.vertices[1] =
              face->vertex_index(1) + slice_n * input.n_vertices();
            quad.vertices[2] =
              face->vertex_index(0) + (slice_n + 1) * input.n_vertices();
            quad.vertices[3] =
              face->vertex_index(1) + (slice_n + 1) * input.n_vertices();
            quads.push_back(quad);
          }
      }

    // if necessary, create face data for faces parallel to the x-y
    // plane. This is only necessary if we need to set manifolds.
    if (copy_manifold_ids)
      for (const auto &cell : input.active_cell_iterators())
        {
          CellData<2> quad;
          quad.boundary_id = numbers::internal_face_boundary_id;
          quad.manifold_id = cell->manifold_id(); // check is outside loop
          for (std::size_t slice_n = 1; slice_n < n_slices - 1; ++slice_n)
            {
              quad.vertices[0] =
                cell->vertex_index(0) + slice_n * input.n_vertices();
              quad.vertices[1] =
                cell->vertex_index(1) + slice_n * input.n_vertices();
              quad.vertices[2] =
                cell->vertex_index(2) + slice_n * input.n_vertices();
              quad.vertices[3] =
                cell->vertex_index(3) + slice_n * input.n_vertices();
              quads.push_back(quad);
            }
        }

    // then mark the bottom and top boundaries of the extruded mesh
    // with max_boundary_id+1 and max_boundary_id+2. check that this
    // remains valid
    Assert((max_boundary_id != numbers::invalid_boundary_id) &&
             (max_boundary_id + 1 != numbers::invalid_boundary_id) &&
             (max_boundary_id + 2 != numbers::invalid_boundary_id),
           ExcMessage(
             "The input triangulation to this function is using boundary "
             "indicators in a range that do not allow using "
             "max_boundary_id+1 and max_boundary_id+2 as boundary "
             "indicators for the bottom and top faces of the "
             "extruded triangulation."));
    const types::boundary_id bottom_boundary_id = max_boundary_id + 1;
    const types::boundary_id top_boundary_id    = max_boundary_id + 2;
    for (const auto &cell : input.active_cell_iterators())
      {
        CellData<2> quad;
        quad.boundary_id = bottom_boundary_id;
        quad.vertices[0] = cell->vertex_index(0);
        quad.vertices[1] = cell->vertex_index(1);
        quad.vertices[2] = cell->vertex_index(2);
        quad.vertices[3] = cell->vertex_index(3);
        if (copy_manifold_ids)
          quad.manifold_id = cell->manifold_id();
        quads.push_back(quad);

        quad.boundary_id = top_boundary_id;
        for (unsigned int &vertex : quad.vertices)
          vertex += (n_slices - 1) * input.n_vertices();
        if (copy_manifold_ids)
          quad.manifold_id = cell->manifold_id();
        quads.push_back(quad);
      }

    // use all of this to finally create the extruded 3d
    // triangulation.  it is not necessary to call
    // GridReordering<3,3>::reorder_cells because the cells we have
    // constructed above are automatically correctly oriented. this is
    // because the 2d base mesh is always correctly oriented, and
    // extruding it automatically yields a correctly oriented 3d mesh,
    // as discussed in the edge orientation paper mentioned in the
    // introduction to the GridReordering class.
    result.create_triangulation(points, cells, subcell_data);

    for (auto manifold_id_it = priorities.rbegin();
         manifold_id_it != priorities.rend();
         ++manifold_id_it)
      for (const auto &face : result.active_face_iterators())
        if (face->manifold_id() == *manifold_id_it)
          for (unsigned int line_n = 0;
               line_n < GeometryInfo<3>::lines_per_face;
               ++line_n)
            face->line(line_n)->set_manifold_id(*manifold_id_it);
  }


  template <>
  void hyper_cube_with_cylindrical_hole(Triangulation<1> &,
                                        const double,
                                        const double,
                                        const double,
                                        const unsigned int,
                                        const bool)
  {
    Assert(false, ExcNotImplemented());
  }



  template <>
  void hyper_cube_with_cylindrical_hole(Triangulation<2> &triangulation,
                                        const double      inner_radius,
                                        const double      outer_radius,
                                        const double,       // width,
                                        const unsigned int, // width_repetition,
                                        const bool colorize)
  {
    const int dim = 2;

    Assert(inner_radius < outer_radius,
           ExcMessage("outer_radius has to be bigger than inner_radius."));

    Point<dim> center;
    // We create an hyper_shell in two dimensions, and then we modify it.
    hyper_shell(triangulation, center, inner_radius, outer_radius, 8);
    triangulation.set_all_manifold_ids(numbers::flat_manifold_id);
    Triangulation<dim>::active_cell_iterator cell =
                                               triangulation.begin_active(),
                                             endc = triangulation.end();
    std::vector<bool> treated_vertices(triangulation.n_vertices(), false);
    for (; cell != endc; ++cell)
      {
        for (unsigned int f = 0; f < GeometryInfo<dim>::faces_per_cell; ++f)
          if (cell->face(f)->at_boundary())
            {
              for (unsigned int v = 0; v < GeometryInfo<dim>::vertices_per_face;
                   ++v)
                {
                  unsigned int vv = cell->face(f)->vertex_index(v);
                  if (treated_vertices[vv] == false)
                    {
                      treated_vertices[vv] = true;
                      switch (vv)
                        {
                          case 1:
                            cell->face(f)->vertex(v) =
                              center + Point<dim>(outer_radius, outer_radius);
                            break;
                          case 3:
                            cell->face(f)->vertex(v) =
                              center + Point<dim>(-outer_radius, outer_radius);
                            break;
                          case 5:
                            cell->face(f)->vertex(v) =
                              center + Point<dim>(-outer_radius, -outer_radius);
                            break;
                          case 7:
                            cell->face(f)->vertex(v) =
                              center + Point<dim>(outer_radius, -outer_radius);
                            break;
                          default:
                            break;
                        }
                    }
                }
            }
      }
    double eps = 1e-3 * outer_radius;
    cell       = triangulation.begin_active();
    for (; cell != endc; ++cell)
      {
        for (unsigned int f = 0; f < GeometryInfo<dim>::faces_per_cell; ++f)
          if (cell->face(f)->at_boundary())
            {
              double dx = cell->face(f)->center()(0) - center(0);
              double dy = cell->face(f)->center()(1) - center(1);
              if (colorize)
                {
                  if (std::abs(dx + outer_radius) < eps)
                    cell->face(f)->set_boundary_id(0);
                  else if (std::abs(dx - outer_radius) < eps)
                    cell->face(f)->set_boundary_id(1);
                  else if (std::abs(dy + outer_radius) < eps)
                    cell->face(f)->set_boundary_id(2);
                  else if (std::abs(dy - outer_radius) < eps)
                    cell->face(f)->set_boundary_id(3);
                  else
                    {
                      cell->face(f)->set_boundary_id(4);
                      cell->face(f)->set_manifold_id(0);
                    }
                }
              else
                {
                  double d = (cell->face(f)->center() - center).norm();
                  if (d - inner_radius < 0)
                    {
                      cell->face(f)->set_boundary_id(1);
                      cell->face(f)->set_manifold_id(0);
                    }
                  else
                    cell->face(f)->set_boundary_id(0);
                }
            }
      }
    triangulation.set_manifold(0, PolarManifold<2>(center));
  }



  template <int dim>
  void
  concentric_hyper_shells(Triangulation<dim> &triangulation,
                          const Point<dim> &  center,
                          const double        inner_radius,
                          const double        outer_radius,
                          const unsigned int  n_shells,
                          const double        skewness,
                          const unsigned int  n_cells,
                          const bool          colorize)
  {
    Assert(dim == 2 || dim == 3, ExcNotImplemented());
    (void)colorize;
    (void)n_cells;
    Assert(inner_radius < outer_radius,
           ExcMessage("outer_radius has to be bigger than inner_radius."));
    if (n_shells == 0)
      return; // empty Triangulation

    std::vector<double> radii;
    radii.push_back(inner_radius);
    for (unsigned int shell_n = 1; shell_n < n_shells; ++shell_n)
      if (skewness == 0.0)
        // same as below, but works in the limiting case of zero skewness
        radii.push_back(inner_radius +
                        (outer_radius - inner_radius) *
                          (1.0 - (1.0 - double(shell_n) / n_shells)));
      else
        radii.push_back(
          inner_radius +
          (outer_radius - inner_radius) *
            (1.0 - std::tanh(skewness * (1.0 - double(shell_n) / n_shells)) /
                     std::tanh(skewness)));
    radii.push_back(outer_radius);

    double grid_vertex_tolerance = 0.0;
    for (unsigned int shell_n = 0; shell_n < radii.size() - 1; ++shell_n)
      {
        Triangulation<dim> current_shell;
        GridGenerator::hyper_shell(current_shell,
                                   center,
                                   radii[shell_n],
                                   radii[shell_n + 1],
                                   n_cells == 0 ? (dim == 2 ? 8 : 12) :
                                                  n_cells);

        // The innermost shell has the smallest cells: use that to set the
        // vertex merging tolerance
        if (grid_vertex_tolerance == 0.0)
          grid_vertex_tolerance =
            0.5 * internal::minimal_vertex_distance(current_shell);

        Triangulation<dim> temp(std::move(triangulation));
        triangulation.clear();
        GridGenerator::merge_triangulations(current_shell,
                                            temp,
                                            triangulation,
                                            grid_vertex_tolerance);
      }

    const types::manifold_id manifold_id = 0;
    triangulation.set_all_manifold_ids(manifold_id);
    if (dim == 2)
      triangulation.set_manifold(manifold_id, PolarManifold<dim>(center));
    else if (dim == 3)
      triangulation.set_manifold(manifold_id, SphericalManifold<dim>(center));

    // We use boundary vertex positions to see if things are on the inner or
    // outer boundary.
    constexpr double radial_vertex_tolerance =
      100.0 * std::numeric_limits<double>::epsilon();
    auto assert_vertex_distance_within_tolerance =
      [center, radial_vertex_tolerance](
        const TriaIterator<TriaAccessor<dim - 1, dim, dim>> face,
        const double                                        radius) {
        (void)center;
        (void)radial_vertex_tolerance;
        (void)face;
        (void)radius;
        for (unsigned int vertex_n = 0;
             vertex_n < GeometryInfo<dim>::vertices_per_face;
             ++vertex_n)
          {
            Assert(std::abs((face->vertex(vertex_n) - center).norm() - radius) <
                     (center.norm() + radius) * radial_vertex_tolerance,
                   ExcInternalError());
          }
      };
    if (colorize)
      for (const auto &cell : triangulation.active_cell_iterators())
        for (unsigned int face_n = 0;
             face_n < GeometryInfo<dim>::faces_per_cell;
             ++face_n)
          {
            auto face = cell->face(face_n);
            if (face->at_boundary())
              {
                if (((face->vertex(0) - center).norm() - inner_radius) <
                    (center.norm() + inner_radius) * radial_vertex_tolerance)
                  {
                    // we must be at an inner face, but check
                    assert_vertex_distance_within_tolerance(face, inner_radius);
                    face->set_all_boundary_ids(0);
                  }
                else
                  {
                    // we must be at an outer face, but check
                    assert_vertex_distance_within_tolerance(face, outer_radius);
                    face->set_all_boundary_ids(1);
                  }
              }
          }
  }



  template <>
  void hyper_cube_with_cylindrical_hole(Triangulation<3> & triangulation,
                                        const double       inner_radius,
                                        const double       outer_radius,
                                        const double       L,
                                        const unsigned int Nz,
                                        const bool         colorize)
  {
    const int dim = 3;

    Assert(inner_radius < outer_radius,
           ExcMessage("outer_radius has to be bigger than inner_radius."));
    Assert(L > 0, ExcMessage("Must give positive extension L"));
    Assert(Nz >= 1, ExcLowerRange(1, Nz));

    cylinder_shell(triangulation, L, inner_radius, outer_radius, 8, Nz);
    triangulation.set_all_manifold_ids(numbers::flat_manifold_id);

    Triangulation<dim>::active_cell_iterator cell =
                                               triangulation.begin_active(),
                                             endc = triangulation.end();
    std::vector<bool> treated_vertices(triangulation.n_vertices(), false);
    for (; cell != endc; ++cell)
      {
        for (unsigned int f = 0; f < GeometryInfo<dim>::faces_per_cell; ++f)
          if (cell->face(f)->at_boundary())
            {
              for (unsigned int v = 0; v < GeometryInfo<dim>::vertices_per_face;
                   ++v)
                {
                  unsigned int vv = cell->face(f)->vertex_index(v);
                  if (treated_vertices[vv] == false)
                    {
                      treated_vertices[vv] = true;
                      for (unsigned int i = 0; i <= Nz; ++i)
                        {
                          double d = i * L / Nz;
                          switch (vv - i * 16)
                            {
                              case 1:
                                cell->face(f)->vertex(v) =
                                  Point<dim>(outer_radius, outer_radius, d);
                                break;
                              case 3:
                                cell->face(f)->vertex(v) =
                                  Point<dim>(-outer_radius, outer_radius, d);
                                break;
                              case 5:
                                cell->face(f)->vertex(v) =
                                  Point<dim>(-outer_radius, -outer_radius, d);
                                break;
                              case 7:
                                cell->face(f)->vertex(v) =
                                  Point<dim>(outer_radius, -outer_radius, d);
                                break;
                              default:
                                break;
                            }
                        }
                    }
                }
            }
      }
    double eps = 1e-3 * outer_radius;
    cell       = triangulation.begin_active();
    for (; cell != endc; ++cell)
      {
        for (unsigned int f = 0; f < GeometryInfo<dim>::faces_per_cell; ++f)
          if (cell->face(f)->at_boundary())
            {
              double dx = cell->face(f)->center()(0);
              double dy = cell->face(f)->center()(1);
              double dz = cell->face(f)->center()(2);

              if (colorize)
                {
                  if (std::abs(dx + outer_radius) < eps)
                    cell->face(f)->set_boundary_id(0);

                  else if (std::abs(dx - outer_radius) < eps)
                    cell->face(f)->set_boundary_id(1);

                  else if (std::abs(dy + outer_radius) < eps)
                    cell->face(f)->set_boundary_id(2);

                  else if (std::abs(dy - outer_radius) < eps)
                    cell->face(f)->set_boundary_id(3);

                  else if (std::abs(dz) < eps)
                    cell->face(f)->set_boundary_id(4);

                  else if (std::abs(dz - L) < eps)
                    cell->face(f)->set_boundary_id(5);

                  else
                    {
                      cell->face(f)->set_all_boundary_ids(6);
                      cell->face(f)->set_all_manifold_ids(0);
                    }
                }
              else
                {
                  Point<dim> c = cell->face(f)->center();
                  c(2)         = 0;
                  double d     = c.norm();
                  if (d - inner_radius < 0)
                    {
                      cell->face(f)->set_all_boundary_ids(1);
                      cell->face(f)->set_all_manifold_ids(0);
                    }
                  else
                    cell->face(f)->set_boundary_id(0);
                }
            }
      }
    triangulation.set_manifold(0, CylindricalManifold<3>(2));
  }

  template <int dim, int spacedim1, int spacedim2>
  void
  flatten_triangulation(const Triangulation<dim, spacedim1> &in_tria,
                        Triangulation<dim, spacedim2> &      out_tria)
  {
    const parallel::distributed::Triangulation<dim, spacedim1> *pt =
      dynamic_cast<
        const parallel::distributed::Triangulation<dim, spacedim1> *>(&in_tria);

    (void)pt;
    Assert(
      pt == nullptr,
      ExcMessage(
        "Cannot use this function on parallel::distributed::Triangulation."));

    std::vector<Point<spacedim2>> v;
    std::vector<CellData<dim>>    cells;
    SubCellData                   subcelldata;

    const unsigned int spacedim = std::min(spacedim1, spacedim2);
    const std::vector<Point<spacedim1>> &in_vertices = in_tria.get_vertices();

    v.resize(in_vertices.size());
    for (unsigned int i = 0; i < in_vertices.size(); ++i)
      for (unsigned int d = 0; d < spacedim; ++d)
        v[i][d] = in_vertices[i][d];

    cells.resize(in_tria.n_active_cells());
    typename Triangulation<dim, spacedim1>::active_cell_iterator
      cell = in_tria.begin_active(),
      endc = in_tria.end();

    for (unsigned int id = 0; cell != endc; ++cell, ++id)
      {
        for (unsigned int i = 0; i < GeometryInfo<dim>::vertices_per_cell; ++i)
          cells[id].vertices[i] = cell->vertex_index(i);
        cells[id].material_id = cell->material_id();
        cells[id].manifold_id = cell->manifold_id();
      }

    if (dim > 1)
      {
        typename Triangulation<dim, spacedim1>::active_face_iterator
          face = in_tria.begin_active_face(),
          endf = in_tria.end_face();

        // Face counter for both dim == 2 and dim == 3
        unsigned int f = 0;
        switch (dim)
          {
            case 2:
              {
                subcelldata.boundary_lines.resize(in_tria.n_active_faces());
                for (; face != endf; ++face)
                  if (face->at_boundary())
                    {
                      for (unsigned int i = 0;
                           i < GeometryInfo<dim>::vertices_per_face;
                           ++i)
                        subcelldata.boundary_lines[f].vertices[i] =
                          face->vertex_index(i);
                      subcelldata.boundary_lines[f].boundary_id =
                        face->boundary_id();
                      subcelldata.boundary_lines[f].manifold_id =
                        face->manifold_id();
                      ++f;
                    }
                subcelldata.boundary_lines.resize(f);
              }
              break;
            case 3:
              {
                subcelldata.boundary_quads.resize(in_tria.n_active_faces());
                for (; face != endf; ++face)
                  if (face->at_boundary())
                    {
                      for (unsigned int i = 0;
                           i < GeometryInfo<dim>::vertices_per_face;
                           ++i)
                        subcelldata.boundary_quads[f].vertices[i] =
                          face->vertex_index(i);
                      subcelldata.boundary_quads[f].boundary_id =
                        face->boundary_id();
                      subcelldata.boundary_quads[f].manifold_id =
                        face->manifold_id();
                      ++f;
                    }
                subcelldata.boundary_quads.resize(f);
              }
              break;
            default:
              Assert(false, ExcInternalError());
          }
      }
    out_tria.create_triangulation(v, cells, subcelldata);
  }



  template <template <int, int> class MeshType, int dim, int spacedim>
#ifndef _MSC_VER
  std::map<typename MeshType<dim - 1, spacedim>::cell_iterator,
           typename MeshType<dim, spacedim>::face_iterator>
#else
  typename ExtractBoundaryMesh<MeshType, dim, spacedim>::return_type
#endif
  extract_boundary_mesh(const MeshType<dim, spacedim> &     volume_mesh,
                        MeshType<dim - 1, spacedim> &       surface_mesh,
                        const std::set<types::boundary_id> &boundary_ids)
  {
    Assert((dynamic_cast<
              const parallel::distributed::Triangulation<dim, spacedim> *>(
              &volume_mesh.get_triangulation()) == nullptr),
           ExcNotImplemented());

    // This function works using the following assumption:
    //    Triangulation::create_triangulation(...) will create cells that
    //    preserve the order of cells passed in using the CellData argument;
    //    also, that it will not reorder the vertices.

    // dimension of the boundary mesh
    const unsigned int boundary_dim = dim - 1;

    // temporary map for level==0
    // iterator to face is stored along with face number
    // (this is required by the algorithm to adjust the normals of the
    // cells of the boundary mesh)
    std::vector<
      std::pair<typename MeshType<dim, spacedim>::face_iterator, unsigned int>>
      temporary_mapping_level0;

    // vector indicating whether a vertex of the volume mesh has
    // already been visited (necessary to avoid duplicate vertices in
    // boundary mesh)
    std::vector<bool> touched(volume_mesh.get_triangulation().n_vertices(),
                              false);

    // data structures required for creation of boundary mesh
    std::vector<CellData<boundary_dim>> cells;
    SubCellData                         subcell_data;
    std::vector<Point<spacedim>>        vertices;

    // volume vertex indices to surf ones
    std::map<unsigned int, unsigned int> map_vert_index;

    // define swapping of vertices to get proper normal orientation of boundary
    // mesh;
    // the entry (i,j) of swap_matrix stores the index of the vertex of
    // the boundary cell corresponding to the j-th vertex on the i-th face
    // of the underlying volume cell
    // if e.g. face 3 of a volume cell is considered and vertices 1 and 2 of the
    // corresponding boundary cell are swapped to get
    // proper normal orientation, swap_matrix[3]=( 0, 2, 1, 3 )
    Table<2, unsigned int> swap_matrix(
      GeometryInfo<spacedim>::faces_per_cell,
      GeometryInfo<dim - 1>::vertices_per_cell);
    for (unsigned int i1 = 0; i1 < GeometryInfo<spacedim>::faces_per_cell; i1++)
      {
        for (unsigned int i2 = 0; i2 < GeometryInfo<dim - 1>::vertices_per_cell;
             i2++)
          swap_matrix[i1][i2] = i2;
      }
    // vertex swapping such that normals on the surface mesh point out of the
    // underlying volume
    if (dim == 3)
      {
        std::swap(swap_matrix[0][1], swap_matrix[0][2]);
        std::swap(swap_matrix[2][1], swap_matrix[2][2]);
        std::swap(swap_matrix[4][1], swap_matrix[4][2]);
      }
    else if (dim == 2)
      {
        std::swap(swap_matrix[1][0], swap_matrix[1][1]);
        std::swap(swap_matrix[2][0], swap_matrix[2][1]);
      }

    // Create boundary mesh and mapping
    // from only level(0) cells of volume_mesh
    for (typename MeshType<dim, spacedim>::cell_iterator cell =
           volume_mesh.begin(0);
         cell != volume_mesh.end(0);
         ++cell)
      for (unsigned int i = 0; i < GeometryInfo<dim>::faces_per_cell; ++i)
        {
          const typename MeshType<dim, spacedim>::face_iterator face =
            cell->face(i);

          if (face->at_boundary() &&
              (boundary_ids.empty() ||
               (boundary_ids.find(face->boundary_id()) != boundary_ids.end())))
            {
              CellData<boundary_dim> c_data;

              for (unsigned int j = 0;
                   j < GeometryInfo<boundary_dim>::vertices_per_cell;
                   ++j)
                {
                  const unsigned int v_index = face->vertex_index(j);

                  if (!touched[v_index])
                    {
                      vertices.push_back(face->vertex(j));
                      map_vert_index[v_index] = vertices.size() - 1;
                      touched[v_index]        = true;
                    }

                  c_data.vertices[swap_matrix[i][j]] = map_vert_index[v_index];
                }
              c_data.material_id =
                static_cast<types::material_id>(face->boundary_id());
              c_data.manifold_id = face->manifold_id();


              // in 3d, we need to make sure we copy the manifold
              // indicators from the edges of the volume mesh to the
              // edges of the surface mesh
              //
              // we set default boundary ids for boundary lines
              // and numbers::internal_face_boundary_id for internal lines
              if (dim == 3)
                for (unsigned int e = 0; e < 4; ++e)
                  {
                    // see if we already saw this edge from a
                    // neighboring face, either in this or the reverse
                    // orientation. if so, skip it.
                    {
                      bool edge_found = false;
                      for (auto &boundary_line : subcell_data.boundary_lines)
                        if (((boundary_line.vertices[0] ==
                              map_vert_index[face->line(e)->vertex_index(0)]) &&
                             (boundary_line.vertices[1] ==
                              map_vert_index[face->line(e)->vertex_index(
                                1)])) ||
                            ((boundary_line.vertices[0] ==
                              map_vert_index[face->line(e)->vertex_index(1)]) &&
                             (boundary_line.vertices[1] ==
                              map_vert_index[face->line(e)->vertex_index(0)])))
                          {
                            boundary_line.boundary_id =
                              numbers::internal_face_boundary_id;
                            edge_found = true;
                            break;
                          }
                      if (edge_found == true)
                        // try next edge of current face
                        continue;
                    }

                    CellData<1> edge;
                    edge.vertices[0] =
                      map_vert_index[face->line(e)->vertex_index(0)];
                    edge.vertices[1] =
                      map_vert_index[face->line(e)->vertex_index(1)];
                    edge.boundary_id = 0;
                    edge.manifold_id = face->line(e)->manifold_id();

                    subcell_data.boundary_lines.push_back(edge);
                  }

              cells.push_back(c_data);
              temporary_mapping_level0.push_back(std::make_pair(face, i));
            }
        }

    // create level 0 surface triangulation
    Assert(cells.size() > 0, ExcMessage("No boundary faces selected"));
    const_cast<Triangulation<dim - 1, spacedim> &>(
      surface_mesh.get_triangulation())
      .create_triangulation(vertices, cells, subcell_data);

    // in 2d: set default boundary ids for "boundary vertices"
    if (dim == 2)
      {
        for (const auto &cell : surface_mesh.active_cell_iterators())
          for (unsigned int vertex = 0; vertex < 2; vertex++)
            if (cell->face(vertex)->at_boundary())
              cell->face(vertex)->set_boundary_id(0);
      }

    // Make mapping for level 0

    // temporary map between cells on the boundary and corresponding faces of
    // domain mesh (each face is characterized by an iterator to the face and
    // the face number within the underlying cell)
    std::vector<std::pair<
      const typename MeshType<dim - 1, spacedim>::cell_iterator,
      std::pair<typename MeshType<dim, spacedim>::face_iterator, unsigned int>>>
      temporary_map_boundary_cell_face;
    for (const auto &cell : surface_mesh.active_cell_iterators())
      temporary_map_boundary_cell_face.push_back(
        std::make_pair(cell, temporary_mapping_level0.at(cell->index())));


    // refine the boundary mesh according to the refinement of the underlying
    // volume mesh,
    // algorithm:
    //   (1) check which cells on refinement level i need to be refined
    //   (2) do refinement (yields cells on level i+1)
    //   (3) repeat for the next level (i+1->i) until refinement is completed

    // stores the index into temporary_map_boundary_cell_face at which
    // presently deepest refinement level of boundary mesh begins
    unsigned int index_cells_deepest_level = 0;
    do
      {
        bool changed = false;

        // vector storing cells which have been marked for
        // refinement
        std::vector<unsigned int> cells_refined;

        // loop over cells of presently deepest level of boundary triangulation
        for (unsigned int cell_n = index_cells_deepest_level;
             cell_n < temporary_map_boundary_cell_face.size();
             cell_n++)
          {
            // mark boundary cell for refinement if underlying volume face has
            // children
            if (temporary_map_boundary_cell_face[cell_n]
                  .second.first->has_children())
              {
                // algorithm only works for
                // isotropic refinement!
                Assert(temporary_map_boundary_cell_face[cell_n]
                           .second.first->refinement_case() ==
                         RefinementCase<dim - 1>::isotropic_refinement,
                       ExcNotImplemented());
                temporary_map_boundary_cell_face[cell_n]
                  .first->set_refine_flag();
                cells_refined.push_back(cell_n);
                changed = true;
              }
          }

        // if cells have been marked for refinement (i.e., presently deepest
        // level is not the deepest level of the volume mesh)
        if (changed)
          {
            // do actual refinement
            const_cast<Triangulation<dim - 1, spacedim> &>(
              surface_mesh.get_triangulation())
              .execute_coarsening_and_refinement();

            // add new level of cells to temporary_map_boundary_cell_face
            index_cells_deepest_level = temporary_map_boundary_cell_face.size();
            for (const auto &refined_cell_n : cells_refined)
              {
                const typename MeshType<dim - 1, spacedim>::cell_iterator
                  refined_cell =
                    temporary_map_boundary_cell_face[refined_cell_n].first;
                const typename MeshType<dim,
                                        spacedim>::face_iterator refined_face =
                  temporary_map_boundary_cell_face[refined_cell_n].second.first;
                const unsigned int refined_face_number =
                  temporary_map_boundary_cell_face[refined_cell_n]
                    .second.second;
                for (unsigned int child_n = 0;
                     child_n < refined_cell->n_children();
                     ++child_n)
                  // at this point, the swapping of vertices done earlier must
                  // be taken into account to get the right association between
                  // volume faces and boundary cells!
                  temporary_map_boundary_cell_face.push_back(
                    std::make_pair(refined_cell->child(
                                     swap_matrix[refined_face_number][child_n]),
                                   std::make_pair(refined_face->child(child_n),
                                                  refined_face_number)));
              }
          }
        // we are at the deepest level of refinement of the volume mesh
        else
          break;
      }
    while (true);

    // generate the final mapping from the temporary mapping
    std::map<typename MeshType<dim - 1, spacedim>::cell_iterator,
             typename MeshType<dim, spacedim>::face_iterator>
      surface_to_volume_mapping;
    for (unsigned int i = 0; i < temporary_map_boundary_cell_face.size(); i++)
      surface_to_volume_mapping[temporary_map_boundary_cell_face[i].first] =
        temporary_map_boundary_cell_face[i].second.first;

    return surface_to_volume_mapping;
  }

} // namespace GridGenerator

// explicit instantiations
#include "grid_generator.inst"

DEAL_II_NAMESPACE_CLOSE
