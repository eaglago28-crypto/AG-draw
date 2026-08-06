#include "BitmapTracer.h"

#include "PathShape.h"

#include <QHash>
#include <QPoint>
#include <QRect>
#include <algorithm>

namespace agdraw::engine {

namespace {

struct Blob {
    std::vector<QPoint> pixels;
    qint64 sumR = 0;
    qint64 sumG = 0;
    qint64 sumB = 0;
};

struct Edge {
    QPoint a;
    QPoint b;
};

// Clé 64 bits pour indexer les coins de grille dans un QHash, sans dépendre
// d'un éventuel qHash(QPoint) fourni par la version de Qt utilisée.
qint64 cornerKey(const QPoint &p) {
    return (static_cast<qint64>(p.x()) << 32) ^ static_cast<qint64>(static_cast<quint32>(p.y()));
}

// Regroupe les arêtes de contour d'un blob en boucles fermées, en marchant
// de coin en coin via la table d'adjacence. Une région simplement connexe
// produit une seule boucle (le contour extérieur) ; une région avec un trou
// produit une boucle supplémentaire pour le bord du trou.
std::vector<std::vector<QPoint>> groupEdgesIntoLoops(const std::vector<Edge> &edges) {
    QHash<qint64, QVector<int>> cornerToEdges;
    for (int i = 0; i < static_cast<int>(edges.size()); ++i) {
        cornerToEdges[cornerKey(edges[i].a)].append(i);
        cornerToEdges[cornerKey(edges[i].b)].append(i);
    }

    std::vector<bool> edgeUsed(edges.size(), false);
    std::vector<std::vector<QPoint>> loops;
    for (int startIdx = 0; startIdx < static_cast<int>(edges.size()); ++startIdx) {
        if (edgeUsed[startIdx]) {
            continue;
        }
        std::vector<QPoint> loop;
        int currentEdge = startIdx;
        const QPoint startCorner = edges[startIdx].a;
        QPoint currentCorner = startCorner;
        loop.push_back(currentCorner);
        while (true) {
            edgeUsed[currentEdge] = true;
            const Edge &edge = edges[currentEdge];
            const QPoint nextCorner = (edge.a == currentCorner) ? edge.b : edge.a;
            loop.push_back(nextCorner);
            currentCorner = nextCorner;
            if (currentCorner == startCorner) {
                break;
            }
            const QVector<int> &candidates = cornerToEdges.value(cornerKey(currentCorner));
            int nextEdge = -1;
            for (int idx : candidates) {
                if (!edgeUsed[idx]) {
                    nextEdge = idx;
                    break;
                }
            }
            if (nextEdge == -1) {
                break; // sécurité : ne devrait pas arriver sur un contour bien formé.
            }
            currentEdge = nextEdge;
        }
        if (loop.size() >= 4) {
            loops.push_back(std::move(loop));
        }
    }
    return loops;
}

// Retire les points intermédiaires colinéaires d'une boucle fermée (dernier
// point == premier), pour ne garder qu'un nœud par changement de direction.
std::vector<QPoint> simplifyLoop(const std::vector<QPoint> &loop) {
    std::vector<QPoint> simplified;
    const int n = static_cast<int>(loop.size()) - 1;
    if (n < 3) {
        return simplified;
    }
    for (int i = 0; i < n; ++i) {
        const QPoint &prev = loop[(i - 1 + n) % n];
        const QPoint &cur = loop[i];
        const QPoint &next = loop[(i + 1) % n];
        const QPoint d1 = cur - prev;
        const QPoint d2 = next - cur;
        const qint64 cross = static_cast<qint64>(d1.x()) * d2.y() - static_cast<qint64>(d1.y()) * d2.x();
        const qint64 dot = static_cast<qint64>(d1.x()) * d2.x() + static_cast<qint64>(d1.y()) * d2.y();
        if (cross != 0 || dot < 0) {
            simplified.push_back(cur);
        }
    }
    return simplified;
}

} // namespace

std::vector<std::unique_ptr<Shape>> traceBitmap(const QImage &sourceImage, const QRectF &targetRect,
                                                   const TraceOptions &options) {
    std::vector<std::unique_ptr<Shape>> result;
    if (sourceImage.isNull()) {
        return result;
    }

    QImage image = sourceImage;
    if (std::max(image.width(), image.height()) > options.maxDimension) {
        image = image.scaled(options.maxDimension, options.maxDimension, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }
    image = image.convertToFormat(QImage::Format_RGB32);

    const int w = image.width();
    const int h = image.height();
    if (w == 0 || h == 0) {
        return result;
    }

    auto pixelAt = [&](int x, int y) { return reinterpret_cast<const QRgb *>(image.constScanLine(y))[x]; };
    auto luminance = [&](int x, int y) {
        const QRgb px = pixelAt(x, y);
        return (qRed(px) * 299 + qGreen(px) * 587 + qBlue(px) * 114) / 1000;
    };
    auto isForeground = [&](int x, int y) {
        if (x < 0 || x >= w || y < 0 || y >= h) {
            return false;
        }
        return luminance(x, y) < options.threshold;
    };

    std::vector<int> blobId(static_cast<size_t>(w) * h, -1);
    std::vector<Blob> blobs;

    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            if (blobId[static_cast<size_t>(y) * w + x] != -1 || !isForeground(x, y)) {
                continue;
            }
            const int id = static_cast<int>(blobs.size());
            blobs.emplace_back();
            Blob &blob = blobs.back();

            std::vector<QPoint> stack{QPoint(x, y)};
            blobId[static_cast<size_t>(y) * w + x] = id;
            while (!stack.empty()) {
                const QPoint p = stack.back();
                stack.pop_back();
                blob.pixels.push_back(p);
                const QRgb px = pixelAt(p.x(), p.y());
                blob.sumR += qRed(px);
                blob.sumG += qGreen(px);
                blob.sumB += qBlue(px);

                static const int dx[4] = {1, -1, 0, 0};
                static const int dy[4] = {0, 0, 1, -1};
                for (int d = 0; d < 4; ++d) {
                    const int nx = p.x() + dx[d];
                    const int ny = p.y() + dy[d];
                    if (nx < 0 || nx >= w || ny < 0 || ny >= h) {
                        continue;
                    }
                    if (blobId[static_cast<size_t>(ny) * w + nx] != -1 || !isForeground(nx, ny)) {
                        continue;
                    }
                    blobId[static_cast<size_t>(ny) * w + nx] = id;
                    stack.push_back(QPoint(nx, ny));
                }
            }
        }
    }

    // Transforme le repère pixel de l'image (0,0)-(w,h) vers targetRect, en
    // conservant les proportions (mise à l'échelle unique pour tous les
    // blobs afin qu'ils gardent leur position relative).
    qreal scale = std::min(targetRect.width() / w, targetRect.height() / h);
    if (!(scale > 0.0)) {
        scale = 1.0;
    }
    const qreal offsetX = targetRect.left() + (targetRect.width() - w * scale) / 2.0;
    const qreal offsetY = targetRect.top() + (targetRect.height() - h * scale) / 2.0;
    auto toScene = [&](const QPoint &p) { return QPointF(offsetX + p.x() * scale, offsetY + p.y() * scale); };

    for (const Blob &blob : blobs) {
        if (static_cast<int>(blob.pixels.size()) < options.minBlobArea) {
            continue;
        }

        std::vector<Edge> edges;
        edges.reserve(blob.pixels.size() * 2);
        for (const QPoint &p : blob.pixels) {
            const int x = p.x();
            const int y = p.y();
            if (!isForeground(x, y - 1)) {
                edges.push_back({QPoint(x, y), QPoint(x + 1, y)});
            }
            if (!isForeground(x, y + 1)) {
                edges.push_back({QPoint(x, y + 1), QPoint(x + 1, y + 1)});
            }
            if (!isForeground(x - 1, y)) {
                edges.push_back({QPoint(x, y), QPoint(x, y + 1)});
            }
            if (!isForeground(x + 1, y)) {
                edges.push_back({QPoint(x + 1, y), QPoint(x + 1, y + 1)});
            }
        }

        const std::vector<std::vector<QPoint>> loops = groupEdgesIntoLoops(edges);
        if (loops.empty()) {
            continue;
        }

        // Le contour extérieur est la boucle à la plus grande boîte
        // englobante ; les boucles plus petites (trous internes) sont
        // ignorées (voir limitation documentée dans BitmapTracer.h).
        size_t bestIdx = 0;
        qint64 bestArea = -1;
        for (size_t i = 0; i < loops.size(); ++i) {
            QRect bbox;
            for (const QPoint &pt : loops[i]) {
                bbox = bbox.united(QRect(pt, QSize(1, 1)));
            }
            const qint64 area = static_cast<qint64>(bbox.width()) * bbox.height();
            if (area > bestArea) {
                bestArea = area;
                bestIdx = i;
            }
        }

        const std::vector<QPoint> simplified = simplifyLoop(loops[bestIdx]);
        if (simplified.size() < 3) {
            continue;
        }

        QVector<PathNode> nodes;
        nodes.reserve(static_cast<int>(simplified.size()));
        for (const QPoint &p : simplified) {
            nodes.append(PathNode{toScene(p), QPointF(0, 0)});
        }

        auto shape = std::make_unique<PathShape>(nodes);
        shape->closed = true;
        const auto pixelCount = static_cast<qint64>(blob.pixels.size());
        const QColor averageColor(static_cast<int>(blob.sumR / pixelCount), static_cast<int>(blob.sumG / pixelCount),
                                   static_cast<int>(blob.sumB / pixelCount));
        shape->fillColor = averageColor;
        shape->strokeColor = averageColor;
        shape->strokeWidth = 0.0;
        result.push_back(std::move(shape));
    }

    return result;
}

} // namespace agdraw::engine
