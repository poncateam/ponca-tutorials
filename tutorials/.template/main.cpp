#include <Ponca/Ponca>
#include "utils.hpp"

using Scalar                      = float;
static constexpr unsigned int Dim = 3;
using Point                       = Ponca::PointPositionNormal<Scalar, Dim>;
using VectorType                  = typename Point::VectorType;
using KDTree                      = Ponca::KdTreeSparse<Point>;
using WeightFilter                = Ponca::DistWeightFilter<Point, Ponca::SmoothWeightKernel<Scalar>>;

using Factory = Ponca::Factory<Point, WeightFilter, Ponca::FitSpaceDer>;

void Perform(const BaseOptions& options, const KDTree& points) { /* TODO */ }

int main(int argc, char** argv)
{
    CLI::App app{"Template code"};
    argv = app.ensure_utf8(argv);

    BaseOptions options(app);
    CLI11_PARSE(app, argc, argv);

    Perform(options, ReadPLY<Point, KDTree>(options.input));
    return 0;
}
