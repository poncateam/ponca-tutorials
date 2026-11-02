#pragma once

#include "happly.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <chrono>

// This is a nice idea from:
// https://stackoverflow.com/questions/2808398/easily-measure-elapsed-time
template <class result_t = std::chrono::milliseconds, class clock_t = std::chrono::steady_clock,
          class duration_t = std::chrono::milliseconds>
inline auto since(std::chrono::time_point<clock_t, duration_t> const& start)
{
    return std::chrono::duration_cast<result_t>(clock_t::now() - start);
}

struct ScoppedTimer
{
    ScoppedTimer(const std::string& message) : message(message) {}

    ~ScoppedTimer()
    {
#ifdef VERBOSE
        std::cout << "[INFO] " << message << since(start).count() << " ms." << std::endl;
#endif
    }

    const std::string message;
    const std::chrono::system_clock::time_point start = std::chrono::system_clock::now();
};

template <typename Point, typename PointContainer>
inline std::pair<typename Point::VectorType, typename Point::VectorType> GetBounds(const PointContainer& container)
{
    typename Point::VectorType min, max;
    min.setConstant(std::numeric_limits<typename Point::Scalar>::max());
    max.setConstant(std::numeric_limits<typename Point::Scalar>::lowest());

    for (auto it = container.begin(); it != container.end(); ++it)
    {
        const auto& pos = (*it).pos();
        for (unsigned int i = 0; i < Point::Dim; ++i)
        {
            min(i) = std::min(min(i), pos(i));
            max(i) = std::max(max(i), pos(i));
        }
    }

    return {min, max};
}

template <typename Point, typename KDTree>
inline KDTree ReadPLY(const std::string& filename)
{
    static constexpr unsigned int Dim = Point::Dim;
    using Scalar                      = typename Point::Scalar;

    static const std::string PLYElementName                     = "vertex";
    static const std::array<std::string, Dim> PLYPositionsNames = {"x", "y", "z"};
    static const std::array<std::string, Dim> PLYNormalsNames   = {"nx", "ny", "nz"};

    auto startPLY = std::chrono::steady_clock::now();
    happly::PLYData plyIn(filename.c_str());

    std::vector<Point> points;
    std::vector<std::vector<Scalar>> positions, normals;
    positions.resize(Dim);
    normals.resize(Dim);

    auto& element = plyIn.getElement(PLYElementName);
    for (unsigned int i = 0; i < Dim; ++i)
    {
        positions[i] = element.getProperty<Scalar>(PLYPositionsNames[i]);

        if (element.hasProperty(PLYNormalsNames[i]))
            normals[i] = element.getProperty<Scalar>(PLYNormalsNames[i]);
        else
            normals[i] = std::vector<Scalar>(positions[i].size(), 0.);
    }

    points.resize(element.count);
    for (unsigned int i = 0; i < Dim; ++i)
    {
        for (unsigned int j = 0; j < points.size(); ++j)
        {
            points[j].pos()(i)    = positions[i][j];
            points[j].normal()(i) = normals[i][j];
        }
    }

#ifdef VERBOSE
    std::cout << "[INFO] Read point cloud '" << filename << "': " << points.size() << " points in "
              << since(startPLY).count() << " ms.\n";
#endif

    auto startKDTree = std::chrono::steady_clock::now();
    KDTree tree(points);
#ifdef VERBOSE
    std::cout << "[INFO] KDTree built in " << since(startKDTree).count() << " ms.\n";
#endif
    return tree;
}

template <typename Point, typename T, typename Getter>
inline void WritePLYVector(happly::Element& element, const std::string& prefix, const std::vector<T>& data,
                           Getter&& getter)
{
    static const std::array<std::string, 3> Axes3D = {"x", "y", "z"};

    for (unsigned int d = 0; d < Point::Dim; ++d)
    {
        const std::string name = prefix + ((Point::Dim == 3) ? Axes3D[d] : std::to_string(d));

        std::vector<typename Point::Scalar> component(data.size());
        for (size_t i = 0; i < data.size(); ++i)
            component[i] = getter(data, i, d);
        element.addProperty<typename Point::Scalar>(name, component);
    }
}

template <typename Point>
inline void WritePLYPoints(happly::Element& element, const std::vector<Point>& data)
{
    WritePLYVector<Point>(element, "", data, [](const auto& p, size_t i, size_t d) { return p[i].pos()(d); });
    WritePLYVector<Point>(element, "n", data, [](const auto& p, size_t i, size_t d) { return p[i].normal()(d); });
}

template <typename Point>
inline void WritePLYVector(happly::Element& element, const std::string& prefix,
                           const std::vector<typename Point::VectorType>& data)
{
    WritePLYVector<Point>(element, prefix, data, [](const auto& v, size_t i, size_t d) { return v[i](d); });
}

#include <CLI/CLI.hpp>

inline std::string GetOutputFilename(const std::string& desired, const char* methodName)
{
    static const std::string token = "{method}";

    auto tokenLocation = desired.find(token);
    if (tokenLocation != std::string::npos)
    {
        std::string output = desired;
        output.replace(tokenLocation, token.size(), methodName);
        return output;
    }

    const auto lastSlash = desired.find_last_of("/\\");
    const auto lastDot   = desired.find_last_of('.');

    if (lastDot != std::string::npos && (lastSlash == std::string::npos || lastDot > lastSlash))
        return desired.substr(0, lastDot) + std::string("-") + methodName + desired.substr(lastDot);

    return desired + std::string("-") + methodName;
}

struct BaseOptions
{
    std::string input;
    std::string output;
    std::string method;
    double scale = 0;

    BaseOptions(CLI::App& app, const std::string& defaultMethod = "")
    {
        app.add_option("-i,--input", input, "Input ply point cloud")->required()->check(CLI::ExistingFile);
        app.add_option("-o,--output", output, "Output file. Use {method} as a placeholder.")->required();
        app.add_option("-s,--scale", scale, "Analaysis scale");
        app.add_option("-m,--method", method, "Select method to run")->required()->default_val(defaultMethod);
    }
};

template <typename Point>
struct GridOptions
{
    using Scalar     = typename Point::Scalar;
    using VectorType = typename Point::VectorType;
    using DimVector  = Eigen::Matrix<size_t, Point::Dim, 1>;

    mutable std::vector<size_t> dimensions;
    mutable std::vector<Scalar> steps;

    GridOptions(CLI::App& app)
    {
        auto dim  = app.add_option("-N,--GridSize", dimensions, "Grid size");
        auto step = app.add_option("-S,--GridStep", steps, "Grid step");

        dim->expected(1, Point::Dim);
        dim->excludes(step);

        step->expected(1, Point::Dim);
        step->excludes(dim);
    }

    // The grid covers the bounding box enlarged by this fraction of its size on each side
    static constexpr Scalar Margin          = Scalar(0.1);
    static constexpr size_t DefaultGridSize = 64;

    std::pair<VectorType, VectorType> GetGridBounds(const VectorType& min, const VectorType& max) const
    {
        const VectorType padding = Margin * (max - min);
        return {min - padding, max + padding};
    }

    // Number of nodes along each axis (at least 2)
    const std::vector<size_t>& GetDimensions(const VectorType& min, const VectorType& max) const
    {
        if (dimensions.size() == 0)
        {
            if (steps.size() != 0)
            {
                if (steps.size() == 1)
                    steps = std::vector<Scalar>(Point::Dim, steps[0]);

                const auto [lo, hi] = GetGridBounds(min, max);
                dimensions.resize(Point::Dim);
                for (unsigned int i = 0; i < Point::Dim; ++i)
                    dimensions[i] = static_cast<size_t>(std::ceil((hi(i) - lo(i)) / steps[i])) + 1;
            }
            else
                dimensions = {DefaultGridSize};
        }

        if (dimensions.size() == 1)
            dimensions = std::vector<size_t>(Point::Dim, dimensions[0]);

        for (auto& d : dimensions)
            d = std::max<size_t>(d, 2);

        return dimensions;
    }

    // Distance between two consecutive nodes along each axis
    VectorType GetGridStep(const VectorType& min, const VectorType& max) const
    {
        const auto& dims    = GetDimensions(min, max);
        const auto [lo, hi] = GetGridBounds(min, max);

        VectorType step;
        for (unsigned int i = 0; i < Point::Dim; ++i)
            step(i) = steps.empty() ? (hi(i) - lo(i)) / Scalar(dims[i] - 1) : steps[i];
        return step;
    }

    // Node positions, the first axis varying fastest
    std::vector<VectorType> GetGridLocations(const VectorType& min, const VectorType& max) const
    {
        const auto& dims      = GetDimensions(min, max);
        const auto origin     = GetGridBounds(min, max).first;
        const VectorType step = GetGridStep(min, max);

        size_t totalSize = 1;
        for (unsigned int i = 0; i < Point::Dim; ++i)
            totalSize *= dims[i];

        std::vector<VectorType> gridPoints(totalSize);
#pragma omp parallel for
        for (size_t i = 0; i < gridPoints.size(); ++i)
        {
            size_t index = i;
            for (unsigned int d = 0; d < Point::Dim; ++d)
            {
                gridPoints[i](d) = origin(d) + Scalar(index % dims[d]) * step(d);
                index /= dims[d];
            }
        }

        return gridPoints;
    }
};
