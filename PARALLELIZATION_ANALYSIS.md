# OpenMP Parallelization Analysis for Core and Remesh Folders

## Summary

This document analyzes opportunities for OpenMP parallelization in the `core/` and `remesh/` folders. OpenMP support is already available via `core/openmp.h` with the `omp_parallel_for` macro.

## Current OpenMP Usage

- **Already using OpenMP**: `filters/smooth.cpp` uses `omp_parallel_for` for vertex processing
- **OpenMP infrastructure**: `core/openmp.h` provides macros that work with or without OpenMP

---

## REMESH FOLDER

### `remesh/remesh.cpp`

#### ✅ **CAN BE PARALLELIZED:**

1. **Lines 30-36: Compute average edge length**
   ```cpp
   for (int i = 0; i < (int)mesh.numHalfedges(); i++) {
       Halfedge *he = mesh.halfedge(i);
       const double l = he->length();
       Lavg += l;
       Lvar += l * l;
       count += 1;
   }
   ```
   - **Parallelization**: Use reduction for `Lavg`, `Lvar`, and `count`
   - **Note**: Requires `#pragma omp parallel for reduction(+:Lavg,Lvar,count)`

2. **Lines 42-66: Check feature line vertices**
   ```cpp
   for (int i = 0; i < (int)mesh.numVertices(); i++) {
       Vertex *v = mesh.vertex(i);
       // ... compute minDihed ...
       if (minDihed < keepAngleLessThan) {
           v->lock();
       }
   }
   ```
   - **Parallelization**: Yes, each vertex is processed independently
   - **Note**: `v->lock()` should be thread-safe (likely just setting a flag)

3. **Lines 83-85: Build indices array**
   ```cpp
   for (int i = 0; i < (int)mesh.numHalfedges(); i++) {
       indices.push_back(i);
   }
   ```
   - **Parallelization**: Yes, but minimal benefit (simple array fill)

4. **Lines 160-194: Edge flipping loop**
   ```cpp
   for (int i = 0; i < (int)mesh.numHalfedges(); i++) {
       Halfedge *he = mesh.halfedge(i);
       // ... check conditions and flip ...
   }
   ```
   - **Parallelization**: ⚠️ **CAUTION** - Edge flips modify mesh topology
   - **Recommendation**: Only parallelize if operations are independent and thread-safe
   - **Risk**: High - mesh modifications can conflict

#### ❌ **CANNOT BE PARALLELIZED:**

1. **Lines 89-100: Split long edges loop**
   - **Reason**: Modifies mesh structure (adds vertices/faces), operations are not independent
   - **Note**: Already shuffled for randomness, but parallelization would require careful synchronization

2. **Lines 118-151: Collapse short edges loop**
   - **Reason**: Modifies mesh structure, operations are not independent
   - **Note**: Similar to split, requires mesh modification synchronization

---

### `remesh/simplify.cpp`

#### ✅ **CAN BE PARALLELIZED:**

1. **Lines 143-175: Compute quadric metric tensor**
   ```cpp
   for (int i = 0; i < numFaces; i++) {
       Face *f = mesh.face(i);
       // ... compute Kp matrix ...
       Qs[vs[0]->index()] += Kp;
       Qs[vs[1]->index()] += Kp;
       Qs[vs[2]->index()] += Kp;
   }
   ```
   - **Parallelization**: ⚠️ **CAUTION** - Multiple faces can update same vertex's Q matrix
   - **Solution**: Use `#pragma omp critical` or atomic operations for Qs updates
   - **Alternative**: Use per-thread accumulation, then merge (better performance)

2. **Lines 179-192: Compute QEM values**
   ```cpp
   for (int i = 0; i < numHalfedges; i++) {
       Halfedge *he = mesh.halfedge(i);
       // ... compute QEM ...
       que.push(QEMNode(qem, he, v));
   }
   ```
   - **Parallelization**: ⚠️ **CAUTION** - Priority queue insertion needs synchronization
   - **Solution**: Collect results in thread-local arrays, then merge into queue sequentially

3. **Lines 332-352: Edge flipping after simplification**
   ```cpp
   for (int i = 0; i < (int)mesh.numHalfedges(); i++) {
       // ... flip edges ...
   }
   ```
   - **Parallelization**: Same concerns as remesh.cpp edge flipping

#### ❌ **CANNOT BE PARALLELIZED:**

1. **Lines 196-329: QEM-based edge removal loop**
   - **Reason**: Sequential priority queue processing, mesh modifications are interdependent
   - **Note**: This is the core simplification algorithm that must be sequential

---

## CORE FOLDER

### `core/mesh.cpp`

#### ✅ **CAN BE PARALLELIZED:**

1. **Lines 113-118: getVertices()**
   ```cpp
   for (const auto &v : vertices_) {
       ret.push_back(v->pos());
   }
   ```
   - **Parallelization**: Yes, but `push_back` needs synchronization
   - **Solution**: Pre-allocate `ret` and use indexed access: `ret[i] = vertices_[i]->pos()`

2. **Lines 121-135: getVertexIndices()**
   ```cpp
   for (const auto &f : faces_) {
       // ... extract indices ...
       ret.push_back(i0);
       ret.push_back(i1);
       ret.push_back(i2);
   }
   ```
   - **Parallelization**: Similar to above, pre-allocate and use indexed access

3. **Lines 138-150: getMeanEdgeLength()**
   ```cpp
   for (int i = 0; i < halfedges_.size(); i++) {
       // ... compute mean ...
       mean += halfedges_[i]->length();
   }
   ```
   - **Parallelization**: Use reduction: `#pragma omp parallel for reduction(+:mean)`

4. **Lines 152-174: getMeanDihedralAngle()**
   ```cpp
   for (int i = 0; i < halfedges_.size(); i++) {
       // ... compute dihedral angle ...
       mean += dihedral(...);
   }
   ```
   - **Parallelization**: Use reduction for `mean`

5. **Lines 176-182: getMeanFaceArea()**
   ```cpp
   for (size_t i = 0; i < faces_.size(); i++) {
       mean += faces_[i]->area();
   }
   ```
   - **Parallelization**: Use reduction for `mean`

6. **Lines 488-490: saveOBJ() - vertex output**
   ```cpp
   for (const auto &v : vertices_) {
       writer << "v " << v->pos().x() << " " << v->pos().y() << " " << v->pos().z() << std::endl;
   }
   ```
   - **Parallelization**: ⚠️ **CAUTION** - File I/O needs synchronization
   - **Solution**: Collect strings in parallel, write sequentially

7. **Lines 520-526: savePLY() - vertex data conversion**
   ```cpp
   for (size_t i = 0; i < vertices_.size(); i++) {
       const Vec3 v = vertices_[i]->pos();
       vertexData[i * 3 + 0] = (float)v.x();
       vertexData[i * 3 + 1] = (float)v.y();
       vertexData[i * 3 + 2] = (float)v.z();
   }
   ```
   - **Parallelization**: Yes, independent writes to pre-allocated array

8. **Lines 530-547: savePLY() - index data collection**
   ```cpp
   for (const auto &f : faces_) {
       // ... collect indices ...
       indexData.push_back(i0);
       indexData.push_back(i1);
       indexData.push_back(i2);
   }
   ```
   - **Parallelization**: Pre-allocate and use indexed access

#### ❌ **CANNOT BE PARALLELIZED:**

1. **Lines 185-365: construct() - Mesh construction**
   - **Reason**: Complex sequential algorithm building halfedge structure
   - **Note**: Topology construction requires sequential processing

2. **Lines 562-692: splitHE() - Halfedge splitting**
   - **Reason**: Single operation modifying mesh topology

3. **Lines 694-843: collapseHE() - Halfedge collapse**
   - **Reason**: Single operation modifying mesh topology

4. **Lines 868-932: flipHE() - Halfedge flip**
   - **Reason**: Single operation modifying mesh topology

5. **Lines 955-986: verify() - Mesh verification**
   - **Reason**: Verification loops are independent but typically fast, parallelization overhead may not be worth it

---

### `core/completion.cpp`

#### ✅ **CAN BE PARALLELIZED:**

1. **Lines 108-129: Initial weight computation**
   ```cpp
   for (int i = 0; i < n_verts - 1; i++) {
       // ... compute Warea and Wangle ...
   }
   ```
   - **Parallelization**: Yes, independent computations

2. **Lines 200-203: Triangle creation**
   ```cpp
   for (const auto &t : tris) {
       Face *new_face = addNewTriangle(boundary, t, pair2he, he2pair);
   }
   ```
   - **Parallelization**: ⚠️ **CAUTION** - `addNewTriangle` modifies shared maps
   - **Solution**: Requires synchronization for map access

3. **Lines 550-566: Normal smoothing iterations**
   ```cpp
   for (int kIter = 0; kIter < 1000; kIter++) {
       for (Vertex *v : insideVerts) {
           // ... compute new normal ...
           newNormals[v] = normalize(nn);
       }
   }
   ```
   - **Parallelization**: Yes, inner loop can be parallelized
   - **Note**: Each iteration depends on previous, but within iteration vertices are independent

4. **Lines 589-600: Face rotation computation**
   ```cpp
   for (Face *f : insideFaces) {
       // ... compute rotation ...
       rotations[f] = rotationAxisAngle(theta, axis);
   }
   ```
   - **Parallelization**: Yes, independent face computations

5. **Lines 604-631: Rotated gradient computation**
   ```cpp
   for (Vertex *v : insideVerts) {
       // ... compute sumGrad ...
       rotGrads[v] = sumGrad / (double)count;
   }
   ```
   - **Parallelization**: Yes, independent vertex computations

6. **Lines 640-670: Build sparse matrix triplets**
   ```cpp
   for (Vertex *v : insideVerts) {
       // ... build tripInner and tripOuter ...
   }
   ```
   - **Parallelization**: ⚠️ **CAUTION** - Vector push_back needs synchronization
   - **Solution**: Use thread-local vectors, merge sequentially

7. **Lines 674-684: Build RHS and boundary matrices**
   ```cpp
   for (auto it : uniqueInner) {
       // ... set bbInner ...
   }
   for (auto it : uniqueOuter) {
       // ... set xxOuter ...
   }
   ```
   - **Parallelization**: Yes, independent row writes

#### ❌ **CANNOT BE PARALLELIZED:**

1. **Lines 131-170: Dynamic programming triangulation**
   - **Reason**: Sequential DP algorithm with dependencies

2. **Lines 242-521: Advancing front hole filling**
   - **Reason**: Sequential algorithm with mesh modifications

---

### `core/bvh.cpp`

#### ✅ **CAN BE PARALLELIZED:**

1. **Lines 64-71: Triangle initialization**
   ```cpp
   for (size_t i = 0; i < indices.size(); i += 3) {
       // ... create triangles ...
       buildData.emplace_back(idx, tris[idx].bounds());
   }
   ```
   - **Parallelization**: Yes, independent triangle creation

2. **Lines 118-120: Bounds computation**
   ```cpp
   for (int i = start; i < end; i++) {
       bounds = Bounds3::merge(bounds, buildData[i].bounds);
   }
   ```
   - **Parallelization**: Use reduction pattern (custom reduction for Bounds3)

3. **Lines 129-131: Centroid bounds computation**
   ```cpp
   for (int i = start; i < end; i++) {
       centroidBounds.merge(buildData[i].centroid);
   }
   ```
   - **Parallelization**: Similar to above, use reduction

4. **Lines 146-155: Bucket computation**
   ```cpp
   for (int i = start; i < end; i++) {
       // ... compute bucket ...
       buckets[b].count++;
       buckets[b].bounds = Bounds3::merge(...);
   }
   ```
   - **Parallelization**: ⚠️ **CAUTION** - Array updates need synchronization
   - **Solution**: Use `#pragma omp atomic` for count, critical section for bounds merge

#### ❌ **CANNOT BE PARALLELIZED:**

1. **Lines 111-193: constructRec() - Recursive BVH construction**
   - **Reason**: Recursive tree construction, dependencies between levels
   - **Note**: Could parallelize left/right subtree construction at each level

---

### `core/vertex.cpp` and `core/face.cpp`

#### ✅ **CAN BE PARALLELIZED:**

These files contain mostly member functions that operate on single vertices/faces. When called in loops (from other files), those loops can be parallelized:

- **Vertex::normal()** - Called per-vertex, independent
- **Vertex::volonoiArea()** - Called per-vertex, independent  
- **Vertex::K()** - Called per-vertex, independent
- **Vertex::H()** - Called per-vertex, independent
- **Face::normal()** - Called per-face, independent
- **Face::area()** - Called per-face, independent

**Note**: These functions themselves don't need parallelization, but loops calling them can be parallelized.

---

## RECOMMENDATIONS

### High Priority (Easy wins, good performance impact):

1. **remesh.cpp**: Parallelize feature line detection (lines 42-66)
2. **remesh.cpp**: Parallelize average edge length computation with reduction (lines 30-36)
3. **mesh.cpp**: Parallelize getMeanEdgeLength(), getMeanDihedralAngle(), getMeanFaceArea() with reductions
4. **mesh.cpp**: Parallelize savePLY() vertex data conversion (lines 520-526)
5. **completion.cpp**: Parallelize normal smoothing inner loop (lines 555-566)
6. **completion.cpp**: Parallelize face rotation computation (lines 589-600)

### Medium Priority (Requires careful synchronization):

1. **simplify.cpp**: Parallelize quadric computation with thread-local accumulation (lines 143-175)
2. **completion.cpp**: Parallelize sparse matrix triplet building with thread-local vectors (lines 640-670)
3. **bvh.cpp**: Parallelize bucket computation with atomic operations (lines 146-155)

### Low Priority (Complex or minimal benefit):

1. **remesh.cpp**: Edge flipping loops (requires mesh modification synchronization)
2. **mesh.cpp**: File I/O operations (I/O bottleneck, not CPU)
3. **bvh.cpp**: Recursive BVH construction (could parallelize subtrees but complex)

---

## Implementation Notes

1. **Reduction variables**: Use OpenMP reduction for sum/average computations
2. **Thread-local storage**: For operations that accumulate into shared structures, use thread-local arrays and merge sequentially
3. **Atomic operations**: Use `#pragma omp atomic` for simple counter increments
4. **Critical sections**: Use `#pragma omp critical` sparingly (performance bottleneck)
5. **Pre-allocation**: Always pre-allocate result arrays when parallelizing loops with `push_back()`

---

## Testing Recommendations

After parallelization:
1. Verify correctness with existing test suite
2. Check for race conditions using thread sanitizers
3. Benchmark performance improvements
4. Test with different thread counts
5. Verify results match sequential version exactly

