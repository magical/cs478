import os
import hashlib
import binascii
import zlib

def keygen(k, t):
    X = []
    Y = []
    for i in range(t):
        x = os.urandom(k//8)
        y = hashlib.sha256(x).digest()
        X.append(x)
        Y.append(y)
    return X, Y

def sign(m, X):
    sig = []
    h = hashlib.sha256(m).digest()
    for i  in splitbits(h,10):
        sig.append(X[i])
    return sig

def verify(m, sig, Y):
    h = hashlib.sha256(m).digest()
    for i, x in zip(splitbits(h,10), sig):
        y = hashlib.sha256(x).digest()
        if y != Y[i]:
            return False
    return True

def splitbits(bytes, k):
    for bits in groupby(bitsof(bytes), k):
        n = 0
        for b in bits:
            n = (n<<1) + b
        yield n

def groupby(it, k):
    while True:
        group = []
        try:
            for i in range(k):
                group.append(next(it))
            yield group
        except StopIteration:
            yield group
            break

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
    sk, pk = keygen(256, 1024)

    sig = sign(m, sk)
    print("".join(map(tohex, sig)))
    print(sum(map(len, sig)))
    bsig = b"".join(sig)
    print(len(zlib.compress(bsig)))

    print(verify(m, sig, pk))
    print(verify(b'blah', sig, pk))

test()
