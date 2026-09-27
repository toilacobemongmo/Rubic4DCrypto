"""
4D Tesseract (Hypercube) Geometry and Projection Math
Calculates 4D rotations in 6 planes (XY, XZ, XW, YZ, YW, ZW)
and performs 4D -> 3D -> 2D perspective projection.
"""

import math
from typing import List, Tuple

# 16 vertices of a unit hypercube in 4D: (x, y, z, w) where each coord in {-1, 1}
VERTICES_4D = []
for x in [-1.0, 1.0]:
    for y in [-1.0, 1.0]:
        for z in [-1.0, 1.0]:
            for w in [-1.0, 1.0]:
                VERTICES_4D.append([x, y, z, w])

# 32 edges connecting vertices that differ in exactly 1 coordinate
EDGES_4D = []
for i in range(16):
    for j in range(i + 1, 16):
        diff = 0
        for k in range(4):
            if VERTICES_4D[i][k] != VERTICES_4D[j][k]:
                diff += 1
        if diff == 1:
            EDGES_4D.append((i, j))


class Tesseract4D:
    def __init__(self):
        # 6 Rotation angles (in radians)
        self.angle_xy = 0.0
        self.angle_xz = 0.0
        self.angle_xw = 0.0
        self.angle_yz = 0.0
        self.angle_yw = 0.0
        self.angle_zw = 0.0

        # Distance of 4D camera on W axis
        self.dist_w = 2.5
        # Distance of 3D camera on Z axis
        self.dist_z = 2.8

        # Active vertex permutation (mapping 16 byte indices to 16 vertices)
        self.vertex_labels = list(range(16))

    def set_permutation(self, perm: List[int]):
        """Sets the permutation of the 16 byte indices on the hypercube vertices."""
        if len(perm) == 16:
            self.vertex_labels = list(perm)

    def rotate_point_4d(self, v: List[float]) -> List[float]:
        """Applies 4D rotation matrices to a 4D point (x, y, z, w)."""
        x, y, z, w = v[0], v[1], v[2], v[3]

        # 1. Rotation in XW plane
        if self.angle_xw != 0:
            c, s = math.cos(self.angle_xw), math.sin(self.angle_xw)
            x, w = x * c - w * s, x * s + w * c

        # 2. Rotation in YW plane
        if self.angle_yw != 0:
            c, s = math.cos(self.angle_yw), math.sin(self.angle_yw)
            y, w = y * c - w * s, y * s + w * c

        # 3. Rotation in ZW plane
        if self.angle_zw != 0:
            c, s = math.cos(self.angle_zw), math.sin(self.angle_zw)
            z, w = z * c - w * s, z * s + w * c

        # 4. Rotation in XY plane
        if self.angle_xy != 0:
            c, s = math.cos(self.angle_xy), math.sin(self.angle_xy)
            x, y = x * c - y * s, x * s + y * c

        # 5. Rotation in XZ plane
        if self.angle_xz != 0:
            c, s = math.cos(self.angle_xz), math.sin(self.angle_xz)
            x, z = x * c - z * s, x * s + z * c

        # 6. Rotation in YZ plane
        if self.angle_yz != 0:
            c, s = math.cos(self.angle_yz), math.sin(self.angle_yz)
            y, z = y * c - z * s, y * s + z * c

        return [x, y, z, w]

    def project_to_2d(self, width: int, height: int, scale: float = 120.0) -> List[Tuple[float, float, float, int]]:
        """
        Projects all 16 4D vertices to 2D screen coordinates.
        Returns list of (screen_x, screen_y, depth_factor, byte_label).
        depth_factor is used for glowing colors and vertex size.
        """
        projected = []
        cx, cy = width / 2.0, height / 2.0

        for idx, orig_v in enumerate(VERTICES_4D):
            v4 = self.rotate_point_4d(orig_v)
            x, y, z, w = v4

            # 4D -> 3D Perspective Projection (from W dimension)
            denom_w = self.dist_w - w
            if denom_w < 0.1:
                denom_w = 0.1
            scale_w = 1.0 / denom_w
            x3 = x * scale_w
            y3 = y * scale_w
            z3 = z * scale_w

            # 3D -> 2D Perspective Projection (from Z dimension)
            denom_z = self.dist_z - z3
            if denom_z < 0.1:
                denom_z = 0.1
            scale_z = 1.0 / denom_z

            sx = cx + x3 * scale_z * scale * 2.0
            sy = cy - y3 * scale_z * scale * 2.0  # invert Y for screen coords

            depth_factor = (w + 1.0) / 2.0  # normalized [0, 1]
            label = self.vertex_labels[idx]
            projected.append((sx, sy, depth_factor, label))

        return projected
