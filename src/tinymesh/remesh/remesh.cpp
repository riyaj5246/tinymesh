#define TINYMESH_API_EXPORT
#include "remesh.h"

#include <random>
#include <algorithm>
#include <atomic>

#include "core/vec.h"
#include "core/debug.h"
#include "core/openmp.h"
#include "core/mesh.h"
#include "core/vertex.h"
#include "core/halfedge.h"
#include "core/face.h"
#include "filters/filters.h"

namespace tinymesh {

void remeshTriangular(Mesh &mesh, double shortLength, double longLength, double keepAngleLessThan, int iterations,
                      bool verbose) {
    Assertion(mesh.verify(), "Invalid mesh!");

    // Compute average edge length
    int count;
    double Lavg, Lvar;
    Lavg = 0.0;
    Lvar = 0.0;

    count = 0;
    for (int i = 0; i < (int)mesh.numHalfedges(); i++) {
        Halfedge *he = mesh.halfedge(i);
        const double l = he->length();
        Lavg += l;
        Lvar += l * l;
        count += 1;
    }

    Lavg = Lavg / count;
    Lvar = Lvar / count - Lavg * Lavg;

    // Check whether each vertex is on feature line
    for (int i = 0; i < (int)mesh.numVertices(); i++) {
        Vertex *v = mesh.vertex(i);
        std::vector<Vec3> neighbors;
        neighbors.reserve(v->degree());
        for (auto vit = v->v_begin(); vit != v->v_end(); ++vit) {
            neighbors.push_back(vit->pos());
        }

        const auto nn = static_cast<int>(neighbors.size());
        double minDihed = Pi;
        const Vec3 p0 = v->pos();
        for (int j = 0; j < nn; j++) {
            const int k = (j + 1) % nn;
            const int l = (j - 1 + nn) % nn;

            const Vec3 p1 = neighbors[j];
            const Vec3 p2 = neighbors[k];
            const Vec3 p3 = neighbors[l];
            const double dihed = dihedral(p2, p0, p1, p3);
            minDihed = std::min(dihed, minDihed);
        }

        if (minDihed < keepAngleLessThan) {
            v->lock();
        }
    }

    // Initialize random number generator
    std::vector<uint32_t> indices;
    std::random_device randev;
    std::mt19937 rnd(randev());

    // Remesh loop
    for (int k = 0; k < iterations; k++) {
        if (verbose) {
            Info("*** Original #%d ***\n", k + 1);
            Info("#vert: %d\n", (int)mesh.numVertices());
            Info("#face: %d\n", (int)mesh.numFaces());
        }

        // Split long edges
        indices.clear();
        indices.reserve(mesh.numHalfedges());
        for (int i = 0; i < (int)mesh.numHalfedges(); i++) {
            indices.push_back(i);
        }

        std::shuffle(indices.begin(), indices.end(), rnd);

        const double longLength2 = Lavg * Lavg * longLength * longLength;
        for (int i : indices) {
            Halfedge *he = mesh.halfedge(i);
            const Vec3 diff = he->src()->pos() - he->dst()->pos();
            const double l2 = dot(diff, diff);

            if (l2 >= longLength2) {
                mesh.splitHE(he);
            }
        }

        if (verbose) {
            Info("*** After split ***\n");
            Info("#vert: %d\n", (int)mesh.numVertices());
            Info("#face: %d\n", (int)mesh.numFaces());
        }

        mesh.verify();

        // Collapse short edges
        indices.clear();
        indices.reserve(mesh.numHalfedges());
        for (int i = 0; i < (int)mesh.numHalfedges(); i++) {
            indices.push_back(i);
        }

        std::shuffle(indices.begin(), indices.end(), rnd);

        const double shortLength2 = Lavg * Lavg * shortLength * shortLength;
        const double longLength2 = Lavg * Lavg * longLength * longLength;
        for (int i : indices) {
            Halfedge *he = mesh.halfedge(i);
            if (he->face()->isLocked() || he->rev()->face()->isLocked()) {
                continue;
            }

            if (he->src()->isLocked() || he->dst()->isLocked()) {
                continue;
            }

            const Vec3 diff = he->src()->pos() - he->dst()->pos();
            const double l2 = dot(diff, diff);

            if (l2 <= shortLength2) {
                // Check if collapse does not generate long edge
                Vertex *a = he->src();
                const Vec3 aPos = a->pos();
                bool collapseOK = true;
                for (auto vit = he->dst()->v_begin(); vit != he->dst()->v_end(); ++vit) {
                    const Vec3 diff2 = aPos - vit->pos();
                    if (dot(diff2, diff2) >= longLength2) {
                        collapseOK = false;
                        break;
                    }
                }

                // Collapse
                if (collapseOK) {
                    mesh.collapseHE(he);
                }
            }
        }

        if (verbose) {
            Info("*** After collapse ***\n");
            Info("#vert: %d\n", (int)mesh.numVertices());
            Info("#face: %d\n", (int)mesh.numFaces());
        }

        // Flip edges
        // Precompute degrees and boundary flags to avoid repeated traversals
        const int nv = (int)mesh.numVertices();
        std::vector<int> degrees(nv);
        std::vector<bool> isBoundary(nv);
        for (int i = 0; i < nv; i++) {
            Vertex *v = mesh.vertex(i);
            degrees[i] = v->degree();
            isBoundary[i] = v->isBoundary();
        }

        for (int i = 0; i < (int)mesh.numHalfedges(); i++) {
            Halfedge *he = mesh.halfedge(i);
            if (he->face()->isBoundary() || he->rev()->face()->isBoundary()) {
                continue;
            }

            if (he->face()->isLocked() || he->rev()->face()->isLocked()) {
                continue;
            }

            Vertex *v0 = he->src();
            Vertex *v1 = he->dst();
            Vertex *v2 = he->next()->dst();
            Vertex *v3 = he->rev()->next()->dst();

            if (v0->isLocked() || v1->isLocked()) {
                continue;
            }

            const int idx0 = v0->index();
            const int idx1 = v1->index();
            const int idx2 = v2->index();
            const int idx3 = v3->index();

            const int d0 = degrees[idx0];
            const int d1 = degrees[idx1];
            const int d2 = degrees[idx2];
            const int d3 = degrees[idx3];
            const int t0 = isBoundary[idx0] ? 4 : 6;
            const int t1 = isBoundary[idx1] ? 4 : 6;
            const int t2 = isBoundary[idx2] ? 4 : 6;
            const int t3 = isBoundary[idx3] ? 4 : 6;

            const int score = std::abs(d0 - t0) + std::abs(d1 - t1) + std::abs(d2 - t2) + std::abs(d3 - t3);
            const int after =
                std::abs(d0 - 1 - t0) + std::abs(d1 - 1 - t1) + std::abs(d2 + 1 - t2) + std::abs(d3 + 1 - t3);
            if (score > after) {
                mesh.flipHE(he);
            }
        }

        // Smoothing
        smoothTaubin(mesh);
    }
}

}  // namespace tinymesh
