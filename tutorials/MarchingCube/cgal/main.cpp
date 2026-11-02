#include <iostream>
#include <Eigen/Core>
#include "utils.hpp"
#include "omp.h"

#include <Ponca/Ponca>
#include <CGAL/Simple_cartesian.h>
#include <CGAL/Isosurfacing_3/Cartesian_grid_3.h>
#include <CGAL/Isosurfacing_3/Interpolated_discrete_values_3.h>
#include <CGAL/Isosurfacing_3/Marching_cubes_domain_3.h>
#include <CGAL/Isosurfacing_3/marching_cubes_3.h>
#include <CGAL/IO/polygon_soup_io.h>

using Scalar                      = float;
static constexpr unsigned int Dim = 3;
using Point                       = Ponca::PointPositionNormal<Scalar, Dim>;
using KDTree                      = Ponca::KdTreeSparse<Point>;
using WeightFilter                = Ponca::DistWeightFilter<Point, Ponca::SmoothWeightKernel<Scalar>>;

using Factory = Ponca::Factory<Point, WeightFilter, Ponca::FitSpaceDer>;

using Kernel = CGAL::Simple_cartesian<Scalar>;
using Grid   = CGAL::Isosurfacing::Cartesian_grid_3<Kernel>;
using Values = CGAL::Isosurfacing::Interpolated_discrete_values_3<Grid>;
using Domain = CGAL::Isosurfacing::Marching_cubes_domain_3<Grid, Values>;

int Perform(const BaseOptions& options, const GridOptions<Point>& gridopts, const KDTree& points)
{
    // Get grid data
    const auto [min, max] = GetBounds<Point>(points.points());
    const auto gridDims   = gridopts.GetDimensions(min, max);
    const auto gridPoints = gridopts.GetGridLocations(min, max);
    const auto gridStep   = gridopts.GetGridStep(min, max);

    // Make sure the scale is coherent
    const Scalar minScale = 2 * gridStep.maxCoeff();
    Scalar scale          = static_cast<Scalar>(options.scale);
    if (scale < minScale)
    {
#ifdef VERBOSE
        if (scale > 0)
        {
            std::cerr << "[WARNING] Scale " << scale << " is too small for a grid step of " << gridStep.maxCoeff()
                      << ", using " << minScale << " instead\n";
        }
#endif
        scale = minScale;
    }

#ifdef VERBOSE
    std::cout << "[INFO] Running marching cube with dims: (" << gridDims[0] << ", " << gridDims[1] << ", "
              << gridDims[2] << ") ; selected scale: " << scale << std::endl;
#endif

    bool hasRun = false;
    auto list   = Factory::Filter<Ponca::ImplicitPrimitiveProvider>();

    list.ApplyAt(options.method.c_str(), [&](const auto& method) {
        const std::string outName = GetOutputFilename(options.output, method.name);
        hasRun                    = true;

        // We count allocation in the chrono
        auto now = std::chrono::steady_clock::now();

        std::vector<Scalar> potentials(gridPoints.size());
        {
            ScoppedTimer timer(std::string("Running ") + method.name + " took: ");
#pragma omp parallel for
            for (std::size_t i = 0; i < gridPoints.size(); ++i)
            {
                const auto& location = gridPoints[i];

                auto fit = method.object;
                fit.init();
                fit.setNeighborFilter({location, scale});
                bool stable =
                    fit.computeWithIds(points.rangeNeighbors(location, scale), points.points()) == Ponca::STABLE;
                // CGAL's marching cube does not emit geometry for cubes of 0s...
                // You may want to filter cubes for which all corners are stable,
                // but this is outside the scope of this tutorial
                potentials[i] = stable ? fit.potential() : Scalar(0);

                if (!fit.isSigned())
                    potentials[i] = std::abs(potentials[i]);
            }
        }

#ifdef VERBOSE
        std::cout << "[INFO] Running '" << method.name << "' implicit surface estimation "
                  << gridDims[0] * gridDims[1] * gridDims[2] << " times took " << since(now).count()
                  << " ms (pallelized over " << omp_get_max_threads() << " threads).\n";
#endif

        // Maps data to a CGAL grid
        const auto& first = gridPoints.front();
        const auto& last  = gridPoints.back();
        const Grid grid{Kernel::Point_3(first.x(), first.y(), first.z()), Kernel::Point_3(last.x(), last.y(), last.z()),
                        std::array<std::size_t, Dim>{gridDims[0], gridDims[1], gridDims[2]}};

        // Note: We could have fill `values` automatically, instead of allocating and filling potential.
        // However, this is a better match of other MarchingCube tutorials in this repository.
        Values values{grid};
        for (std::size_t i = 0; i < gridPoints.size(); ++i)
            values(i % gridDims[0], (i / gridDims[0]) % gridDims[1], i / (gridDims[0] * gridDims[1])) = potentials[i];

        std::vector<Kernel::Point_3> V;
        std::vector<std::array<std::size_t, 3>> F;
        CGAL::Isosurfacing::marching_cubes<CGAL::Parallel_if_available_tag>(Domain{grid, values}, 0., V, F);
        CGAL::IO::write_polygon_soup(outName, V, F);
    });

    if (!hasRun)
        std::cerr << "[ERROR] No method providing implicit primitive named '" << options.method << "' was found.\n";

    return !hasRun;
}

int main(int argc, char** argv)
{
    CLI::App app{""};
    argv = app.ensure_utf8(argv);

    BaseOptions options(app);
    GridOptions<Point> gridoptions(app);

    omp_set_num_threads(omp_get_max_threads());

    CLI11_PARSE(app, argc, argv);
    return Perform(options, gridoptions, ReadPLY<Point, KDTree>(options.input));
}
