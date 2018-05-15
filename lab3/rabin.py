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

    M = numpy.zeros((t, l))
    for i in range(t):
        for j in range(l):
            M[i,j] = (i+1)**j

    return M.dot(V) % P

def recover(U, l, t):
    M = numpy.zeros((t, l))

    for i in range(t):
        for j in range(l):
            M[i,j] = (i+1)**j

    M = M[l:,] # lose some rows
    U = U[l:]

    det = numpy.linalg.det(M)
    detinv = pow(int(det), P-2, P)
    Minv = numpy.round(numpy.linalg.inv(M)*det)*detinv % P

    print(Minv.dot(M) % P)

    V = Minv.dot(U) % P
    return V


print(dispersal([1, 2, 3, 4], 4, 8))
print(recover(dispersal([1, 2, 3, 4], 4, 8), 4, 8))
