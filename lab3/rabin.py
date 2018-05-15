# rabin's information dispersal

import numpy
import numpy.linalg

P = 257

def dispersal(V, l, t):
    """l-out-of-t dispersal"""
    assert 2 < l < t
    alpha = 2 # ???

    # consider message as elements of some field
    # break message into N/t chunks, each with t elements
    # for each chunk, perform dispersal

    M = vandermonde(t, l)

    return M.dot(V) % P

def recover(U, l, t):
    M = vandermonde(t, l)

    M = M[l:,] # lose some rows
    U = U[l:]

    V = solve(M, U)

    return V

def vandermonde(t, l):
    M = numpy.zeros((t, l), dtype=int)
    for i in range(t):
        for j in range(l):
            M[i,j] = pow(i+1, j, P)
    return M

def solve(M, U):
    """gaussian elimination"""
    n = M.shape[0]
    M = augment(M, U)

    def cancel(k,i):
        """cancel row i using row k"""
        d = (-M[i,k] * inv(M[k,k], P)) % P
        for j in range(0, n+1):
            M[i,j] = (M[i,j] + M[k,j] * d) % P
        assert M[i,k] == 0

    print(M)

    for k in range(n):
        for i in range(k+1, n):
            cancel(k,i)

    for k in reversed(range(n)):
        for i in reversed(range(0, k)):
            cancel(k,i)

    print(M)

    V = [M[k,n]*inv(M[k,k], P)%P for k in range(n)]

    return numpy.array(V)

def inv(n, p):
    """find n^-1 mod p"""
    return pow(n, p-2, p)

def augment(M, V):
    """creates an augmented matrix out of a square matrix and a vector"""
    return numpy.append(M, numpy.transpose([V]), axis=1)


print(dispersal([1, 2, 3, 4], 4, 8))
print(recover(dispersal([1, 2, 3, 4], 4, 8), 4, 8))
