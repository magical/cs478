import os
import hashlib
import binascii
import zlib

def keygen(k):
    X = []
    Y = []
    for i in range(k):
        x0 = os.urandom(k//8)
        x1 = os.urandom(k//8)
        y0 = hashlib.sha256(x0).digest()
        y1 = hashlib.sha256(x1).digest()
        X.append((x0, x1))
        Y.append((y0, y1))
    return X, Y

def sign(m, X):
    sig = []
    h = hashlib.sha256(m).digest()
    for i,b  in enumerate(bitsof(h)):
        sig.append(X[i][b])
    return sig

def verify(m, sig, Y):
    h = hashlib.sha256(m).digest()
    for i, b in enumerate(bitsof(h)):
        x = sig[i]
        y = hashlib.sha256(x).digest()
        if y != Y[i][b]:
            return False
    return True

def bitsof(m):
    for x in m:
        yield x & 1
        yield (x>>1) & 1
        yield (x>>2) & 1
        yield (x>>3) & 1
        yield (x>>4) & 1
        yield (x>>5) & 1
        yield (x>>6) & 1
        yield (x>>7) & 1

def tohex(b):
    return binascii.hexlify(b).decode('latin1')

def test():
    m = b"test"
    sk, pk = keygen(256)

    sig = sign(m, sk)
    print("".join(map(tohex, sig)))
    print(sum(map(len, sig)))
    bsig = b"".join(sig)
    print(len(zlib.compress(bsig)))

    print(verify(m, sig, pk))
    print(verify(b'blah', sig, pk))

test()
