#!/opt/homebrew/bin/python3

# this code fits a track with 6 3D points
# input: six 3D points
# output: a 3D point for intersection point, and a direction vector, both are 3D

import torch
from .text_parser import vec3_t
import scipy.sparse.linalg as spla
import numpy as np

# detailed algorithm for the fitter
# we fit x-z and y-z plane separaterly
# the matrix form is, and similar for the other plane:
#    (z1 1)        (x1)
#    (z2 1)        (x2)
#    (z3 1) (kx) = (x3)
#    (z4 1) (bx)   (x4)
#    (z5 1)        (x5)
#    (z6 1)        (x6)
# and we use normal equation to solve (kx, bx, ky, by)


# a function to print pytorch tensors, the original print function for it is too ugly
def print_tensor(A):
    for row in A:
        formated_row = ["{:{}.{}f}".format(elem, 12, 6) for elem in row]
        print("[", "   ".join(formated_row), "]")
    print("\n")


if torch.backends.mps.is_available():
    device = torch.device('cpu')
else:
    device = torch.device('cpu')

# solve the matrix equation Ax = b, two method: 1) linear 2) svd
# svd is said to be better
# all methods are described in this page:
# https://docs.scipy.org/doc/scipy/reference/sparse.linalg.html#module-scipy.sparse.linalg
class solver:
    def __init__(self):
        pass

    def svd(self, A, b):
        # svd method
        # move to gpu
        A_gpu = A.to(device)
        b_gpu = b.to(device)
        # compute SVD of A
        U, S, Vt = torch.linalg.svd(A_gpu, full_matrices=False)
        # compute pseudo-inverse of the diagonal matrix S
        S_inv = torch.diag(1 / (S + 1e-10)) # adding a small value to avoid division by 0
        # compute the pseudo-inverse of A
        A_pseudo_inv = Vt.T @ S_inv @ U.T
        # solve for x
        x = A_pseudo_inv @ b_gpu
        return x

    def lin(self, A, b):
        # linear method
        # move to gpu
        A_gpu = A.to(device)
        b_gpu = b.to(device)
 
        s = torch.linalg.solve(A_gpu, b_gpu)
        return s

    def cg(self, A, b):
        # conjugate gradient (CG) method
        # convert pytorch tensors to numpy arrays
        A_np = A.cpu().numpy()
        b_np = b.cpu().numpy()

        x_cg_np, info_cg = spla.cg(A_np, b_np)
        x_cg = torch.from_numpy(x_cg_np).to(A.device)
        return x_cg

    def cgs(self, A, b):
        # Conjugate Gradient squared iteration method
        A_np = A.cpu().numpy()
        b_np = b.cpu().numpy()

        x_cgs_np, info_cgs = spla.cgs(A_np, b_np)
        x_cgs = torch.from_numpy(x_cgs_np).to(A.device)
        return x_cgs

    def gmres(self, A, b):
        # Generalized Minimal RESidual iteration method
        A_np = A.cpu().numpy()
        b_np = b.cpu().numpy()

        x_gmres_np, info_gmres = spla.gmres(A_np, b_np)
        x_gmres = torch.from_numpy(x_gmres_np).to(A.device)
        return x_gmres

    def lgmres(self, A, b):
        # LGMRES method
        A_np = A.cpu().numpy()
        b_np = b.cpu().numpy()

        x_lgmres_np, info_lgmres = spla.lgmres(A_np, b_np)
        x_lgmres = torch.from_numpy(x_lgmres_np).to(A.device)
        return x_lgmres

    def minres(self, A, b):
        # Minimal RESidual iteration method
        A_np = A.cpu().numpy()
        b_np = b.cpu().numpy()

        x_minres_np, info_minres = spla.minres(A_np, b_np)
        x_minres = torch.from_numpy(x_minres_np).to(A.device)
        return x_minres

    def bicg(self, A, b):
        # BIConjugate Gradient iteration method
        A_np = A.cpu().numpy()
        b_np = b.cpu().numpy()

        x_bicg_np, info_bicg = spla.bicg(A_np, b_np)
        x_bicg = torch.from_numpy(x_bicg_np).to(A.device)
        return x_bicg

    def bicgstab(self, A, b):
        # BIConjugate Gradient Stablized iteration method
        A_np = A.cpu().numpy()
        b_np = b.cpu().numpy()

        x_bicgstab_np, info_bicgstab = spla.bicgstab(A_np, b_np)
        x_bicgstab = torch.from_numpy(x_bicgstab_np).to(A.device)
        return x_bicgstab

    def qmr(self, A, b):
        # Quasi-Minimal Residual Iteration method
        A_np = A.cpu().numpy()
        b_np = b.cpu().numpy()

        x_qmr_np, info_qmr = spla.qmr(A_np, b_np)
        x_qmr = torch.from_numpy(x_qmr_np).to(A.device)
        return x_qmr

    def gcrotmk(self, A, b):
        # flexible GCROT(m, k) algorithm
        A_np = A.cpu().numpy()
        b_np = b.cpu().numpy()

        x_gcrotmk_np, info_gcrotmk = spla.gcrotmk(A_np, b_np)
        x_gcrotmk = torch.from_numpy(x_gcrotmk_np).to(A.device)
        return x_gcrotmk

    def tfqmr(self, A, b):
        # Transpose-Free Quasi-Minimal Residual Iteration method
        A_np = A.cpu().numpy()
        b_np = b.cpu().numpy()

        x_tfqmr_np, info_tfqmr = spla.tfqmr(A_np, b_np)
        x_tfqmr = torch.from_numpy(x_tfqmr_np).to(A.device)
        return x_tfqmr


class fitter:
    def __init__(self):
        self.solver = solver()

    def solve(self, track, resolution=[]):
        # resolution must be in this format [\sigma_x1, \sigma_y1, \sigma_x2, \sigma_y2, ..., \sigma_xn, \sigma_yn]
        N_on_T = len(track)

        if N_on_T < 3:
            print("ERROR: a track must have >=3 points.")
            print("       unable to fit, return 0's instead.")
            return vec3_t(), vec3_t()

        A = torch.zeros(2*N_on_T, 4)
        b = torch.zeros(2*N_on_T, 1)
        for i in range(N_on_T):
            A[i, 0] = track[i].z; A[i, 1] = 1; b[i, 0] = track[i].x;
            A[i+N_on_T, 2] = track[i].z; A[i+N_on_T, 3] = 1; b[i+N_on_T, 0] = track[i].y;

        # get the normal equation
        AT = A.T
        A2 = torch.matmul(AT, A);
        B = torch.matmul(AT, b);

        # solve the normal equation
        s = self.solver.svd(A2, B)

        # organize the result by using intersection point on focal plane (z=0) and direction vector
        direction = vec3_t(s[0,0], s[2,0], 1);
        cross_point = vec3_t(s[1,0], s[3,0], 0); 

        # calculate the chi2, with default error to be 1
        t1 = torch.matmul(A, s)
        t_diff = t1 - b;

        # if resolution for each plane has been provided
        if len(resolution) == len(track)*2:
            reso_tensor = torch.tensor(resolution)
            reso_tensor = reso_tensor.reshape(-1, 1)
            t_diff = t_diff / reso_tensor

        chi2 = torch.sum(torch.square(t_diff))

        return direction, cross_point, chi2.item()


# get the track projected position at a fixed z plane
class projector:
    def __init__(self):
        pass

    def solve(self, cross_point, direction, z):
        dz = z - cross_point.z
        dx = dz * (direction.x/direction.z)
        dy = dz * (direction.y/direction.z)

        x = cross_point.x + dx; y = cross_point.y + dy; z = cross_point.z + dz

        res = vec3_t(x, y, z)
        return res



# unit test
'''
from text_parser import track_parser
tracks = track_parser();
fit = fitter()
project = projector()
for track in tracks.tracks:
    print(len(track))
    d, p, chi2 = fit.solve(track)
    d.print()
    p.print()
    print("{:.6f}".format(chi2))

    proj_hits = []
    for hit in track:
        h = project.solve(p, d, hit.z)
        proj_hits.append(h)

    print("comparison:")
    for i in range(0, len(track)):
        print("orig_z = {:10.6f}, proj_z = {:10.6f}, orig_x = {:10.6f}, proj_x = {:10.6f}, orig_y = {:10.6f}, proj_y = {:10.6f}".format(
            track[i].z, proj_hits[i].z, track[i].x, proj_hits[i].x, track[i].y, proj_hits[i].y
            ))

    input()
'''
