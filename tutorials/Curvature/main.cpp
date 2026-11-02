#include <Ponca/Ponca>
#include "utils.hpp"

using Scalar                      = float;
static constexpr unsigned int Dim = 3;
using Point                       = Ponca::PointPositionNormal<Scalar, Dim>;
using VectorType                  = typename Point::VectorType;
using KDTree                      = Ponca::KdTreeSparse<Point>;
using WeightFilter                = Ponca::DistWeightFilter<Point, Ponca::SmoothWeightKernel<Scalar>>;

using Factory = Ponca::Factory<Point, WeightFilter, Ponca::FitSpaceDer>;

void Perform(const BaseOptions& options, KDTree&& points)
{
    Scalar scale = static_cast<Scalar>(options.scale);

    auto list = Factory::Filter<Ponca::PrincipalCurvaturesProvider>();
    list.ApplyAt(options.method.c_str(), [&](const auto& method) {
        const auto& pts = points.points();
        std::vector<Scalar> minC(pts.size()), maxC(pts.size()), gaussC(pts.size());
        std::vector<VectorType> minDir(pts.size()), maxDir(pts.size());

        {
            ScoppedTimer timer(std::string("Running ") + method.name + " took: ");

#pragma omp parallel for
            for (std::size_t i = 0; i < pts.size(); ++i)
            {
                auto fit = method.object;
                fit.init();
                fit.setNeighborFilter({pts[i], scale});

                // CNC requires a const object but the range isn't...
                std::vector<typename KDTree::IndexType> neighbors;
                for (auto j : points.rangeNeighbors(pts[i].pos(), scale))
                    neighbors.push_back(j);

                auto status = fit.computeWithIds(neighbors, pts);

                minC[i]   = fit.kmin();
                maxC[i]   = fit.kmax();
                gaussC[i] = fit.GaussianCurvature();

                minDir[i] = fit.kminDirection();
                maxDir[i] = fit.kmaxDirection();
            }
        }
        // Export results now
        happly::PLYData plyOut;
        plyOut.addElement("vertex", pts.size());
        auto& vertices = plyOut.getElement("vertex");

        WritePLYPoints(vertices, pts);
        WritePLYVector<Point>(vertices, "kmindir_", minDir);
        WritePLYVector<Point>(vertices, "kmaxdir_", maxDir);

        vertices.addProperty<Scalar>("kmin", minC);
        vertices.addProperty<Scalar>("kmax", maxC);
        vertices.addProperty<Scalar>("gaussian", gaussC);

        plyOut.write(GetOutputFilename(options.output, method.name), happly::DataFormat::Binary);
    });
}

int main(int argc, char** argv)
{
    CLI::App app{"Template code"};
    argv = app.ensure_utf8(argv);

    BaseOptions options(app);
    CLI11_PARSE(app, argc, argv);

    Perform(options, ReadPLY<Point, KDTree>(options.input));
    return 0;
}

