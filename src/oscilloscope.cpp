#include "../include/oscilloscope.h"

#include "../include/geometry_generator.h"
#include "../include/engine_sim_application.h"
#include "../include/ui_utilities.h"

Oscilloscope::Oscilloscope() {
    m_xMin = m_xMax = 0;
    m_yMin = m_yMax = 0;
    m_lineWidth = 1;

    m_points = nullptr;
    m_renderBuffer = nullptr;
    m_writeIndex = 0;
    m_bufferSize = 0;
    m_pointCount = 0;
    m_drawReverse = true;
    m_checkMouse = true;
    m_drawZero = true;
    m_dynamicallyResizeX = false;
    m_dynamicallyResizeY = true;

    i_color = ysMath::Constants::One;
}

Oscilloscope::~Oscilloscope() {
    assert(m_points == nullptr);
    assert(m_renderBuffer == nullptr);
}

void Oscilloscope::initialize(EngineSimApplication *app) {
    UiElement::initialize(app);

    i_color = m_app->getRed();
}

void Oscilloscope::destroy() {
    delete[] m_points;
    delete[] m_renderBuffer;

    m_points = nullptr;
    m_renderBuffer = nullptr;

    m_writeIndex = 0;
    m_bufferSize = 0;
    m_pointCount = 0;
}

void Oscilloscope::update(float dt) {
    UiElement::update(dt);

    m_mouseBounds = m_bounds;
}

void Oscilloscope::render() {
    render(m_bounds);
}

void Oscilloscope::render(const Bounds &bounds) {
    for (int i = 0; i < m_pointCount; ++i) {
        const int index = (m_writeIndex - m_pointCount + i + m_bufferSize) % m_bufferSize;
        m_renderBuffer[index] = dataPointToRenderPosition(m_points[index], bounds);
    }

    const int start = (m_writeIndex - m_pointCount + m_bufferSize) % m_bufferSize;
    const int n0 = (start + m_pointCount) > m_bufferSize
        ? m_bufferSize - start
        : m_pointCount;
    const int n1 = m_pointCount - n0;

    GeometryGenerator::GeometryIndices lines;
    GeometryGenerator::PathParameters params;
    params.p0 = m_renderBuffer + start;
    params.p1 = m_renderBuffer;
    params.n0 = n0;
    params.n1 = n1;

    m_app->getGeometryGenerator()->startShape();

    params.i = 0;
    params.width = pixelsToUnits(0.5f) * (float)m_lineWidth;
    if (!m_app->getGeometryGenerator()->startPath(params)) {
        return;
    }

    Point prev = params.p0[0];
    bool lastDetached = false;
    for (int i = 1; i < n0 + n1; ++i) {
        Point *p = (i >= n0)
            ? params.p1
            : params.p0;
        const int index = (i >= n0)
            ? i - n0
            : i;
        const float s = (float)(i) / (n0 + n1);
        const Point p_i = p[index];
        params.i = i;
        params.width = (float)m_lineWidth * std::fmaxf(
            pixelsToUnits(1.0f) * s,
            pixelsToUnits(0.5f));

        if (s > 0.95f) {
            params.width += pixelsToUnits(((s - 0.95f) / 0.05f) * 2);
        }

        const bool detached =
            prev.x > p_i.x
            || std::abs(p_i.x - prev.x) > pixelsToUnits(100.0f);
        m_app->getGeometryGenerator()->generatePathSegment(
            params,
            (detached || lastDetached) && !m_drawReverse);

        lastDetached = detached;

        prev = p_i;
    }

    m_app->getGeometryGenerator()->endShape(&lines);

    resetShader();

    if (m_drawZero) {
        GeometryGenerator::GeometryIndices zeroLine;
        const Point zeroA = dataPointToRenderPosition({ (float)m_xMin, 0.0f }, bounds);
        const Point zeroB = dataPointToRenderPosition({ (float)m_xMax, 0.0f }, bounds);

        GeometryGenerator::Line2dParameters params;
        params.x0 = zeroA.x;
        params.x1 = zeroB.x;
        params.y0 = zeroA.y;
        params.y1 = zeroB.y;
        params.lineWidth = pixelsToUnits(0.5f);

        m_app->getGeometryGenerator()->startShape();
        m_app->getGeometryGenerator()->generateLine2d(params);
        m_app->getGeometryGenerator()->endShape(&zeroLine);

        m_app->getShaders()->SetBaseColor(mix(m_app->getForegroundColor(), m_app->getBackgroundColor(), 0.95f));
        m_app->drawGenerated(zeroLine, 0x11, m_app->getShaders()->GetUiFlags());
    }

    m_app->getShaders()->SetBaseColor(i_color);
    m_app->drawGenerated(lines, 0x11, m_app->getShaders()->GetUiFlags());
}

Point Oscilloscope::dataPointToRenderPosition(
    const DataPoint &p,
    const Bounds &bounds) const
{
    const float width = bounds.width();
    const float height = bounds.height();

    const float s_x = (float)((p.x - m_xMin) / (m_xMax - m_xMin));
    const float s_y = (float)((p.y - m_yMin) / (m_yMax - m_yMin));

    const Point local = { s_x * width, s_y * height };

    return getRenderPoint(bounds.getPosition(Bounds::bl) + local);
}

void Oscilloscope::addDataPoint(double x, double y) {
    m_points[m_writeIndex] = { x, y };
    m_writeIndex = (m_writeIndex + 1) % m_bufferSize;
    m_pointCount = (m_pointCount >= m_bufferSize)
        ? m_bufferSize
        : m_pointCount + 1;

    if (m_dynamicallyResizeY) {
        if (y + std::abs(0.1 * y) >= m_yMax) {
            m_yMax = y + std::abs(0.1 * y);
        }
        else if (y - std::abs(0.1 * y) <= m_yMin) {
            m_yMin = y - std::abs(0.1 * y);
        }
    }
    if (m_dynamicallyResizeX) {
        if (x + std::abs(0.1 * x) >= m_xMax) {
            m_xMax = x + std::abs(0.1 * x);
        }
        else if (x - std::abs(0.1 * x) <= m_xMin) {
            m_xMin = x - std::abs(0.1 * x);
        }
    }
}

void Oscilloscope::setBufferSize(int n) {
    m_points = new DataPoint[n];
    m_renderBuffer = new Point[n];
    m_bufferSize = n;

    reset();
}

void Oscilloscope::reset() {
    m_writeIndex = 0;
    m_pointCount = 0;
}
