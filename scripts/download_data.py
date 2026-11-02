#!/usr/bin/env python3
"""
    Downloads a few data in a user specified directory in order 
    to get started with the tutorials 
"""
import shutil
import argparse
import tempfile
from pathlib import Path

import numpy as np
from plyfile import PlyData, PlyElement
from ThreeDARD import dataset

ROOT = Path(__file__).resolve().parent.parent
TMP  = Path(tempfile.gettempdir())

PCLIST = {
    # Name in user folder: (source, source data)
    "Buddah": ("3DARD", "Maitreya_Buddha", "0006", "scan01.ply"), 
    "Charango": ("3DARD", "Charango", "0006", "scan1.ply")
}

def normalize(array):
    """
        Return a an array of normalized vector
    """
    n = np.linalg.norm(array, axis=1, keepdims=True)
    return array / np.where(n == 0, 1, n)

def loadPoints(srcPath, destPath):
    """
        Creates a pointset by estimating the normal from mesh data when possible. 

        If vertices already carry normal data, the file is simply copied. 
        If faces exists, we try to estimate the normals:
            * From face normal if present
            * By re-creating the faces and computing their normals
    """
    ply = PlyData.read(srcPath)
    hasFaces = "face" in ply and len(ply["face"].data) > 0 
    vertices = ply["vertex"]

    
    # If the file already has per vertex normals, just copy the file
    # We also perform the copy if we can't estimate the normals
    if {"x", "y", "z", "nx", "ny", "nz"} <= set(vertices.data.dtype.names) or not hasFaces:
        shutil.copyfile(srcPath, destPath)
        return  

    # Gather positions
    positions = np.column_stack([vertices["x"], vertices["y"], vertices["z"]])
    
    # Try estimating normals from faces
    if hasFaces:
        faces = ply["face"]
        faceIndices = np.vstack(faces.data["vertex_indices"])

        if {"nx", "ny", "nz"} <= set(faces.data.dtype.names):
            # Extract face normals
            triNormals = np.column_stack([faces["nx"], faces["ny"], faces["nz"]])
            triNormals = triNormals[faceIndices]
        else:
            # Compute face normals from triangle data
            p = positions[faceIndices]
            triNormals = normalize(np.cross(p[:, 1] - p[:, 0], p[:, 2] - p[:, 0]))

        # Average of normal per vertex
        pointNormals = normalize(np.column_stack([
            np.bincount(faceIndices.ravel(), weights=np.repeat(triNormals[:, 0], 3), minlength=positions.shape[0]),
            np.bincount(faceIndices.ravel(), weights=np.repeat(triNormals[:, 1], 3), minlength=positions.shape[0]),
            np.bincount(faceIndices.ravel(), weights=np.repeat(triNormals[:, 2], 3), minlength=positions.shape[0])
        ]))

        # Write ply
        # TODO: Write additionnal data ? 
        dtype = [(name, "f4") for name in ["x", "y", "z", "nx", "ny", "nz"]]
        data = np.empty(len(positions), dtype=dtype)
        data ["x"], data ["y"], data ["z"] = positions.T
        data["nx"], data["ny"], data["nz"] = pointNormals.T
        PlyData([PlyElement.describe(data, "vertex")], text=False).write(destPath)

def downloadModels(datadir):
    """Local path of an input, downloading it from 3D-ARD if needed."""
    if not dataset.isInitialized():
        dataset.init(str(TMP))

    for name, spec in PCLIST.items():
        if spec[0] == "3DARD":
            path = Path(dataset.getUnitDataPath(*spec[1:]))
            loadPoints(path, datadir / (name + ".ply"))
        else:
            print(f"warning: can not load from source {spec[0]}")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Download a few ply file from the 3D-ARD repository")
    parser.add_argument("--data-dir", type=Path, default=ROOT / "data", help="Download cache")

    args = parser.parse_args()
    downloadModels(args.data_dir)