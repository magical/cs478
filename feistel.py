#!/usr/bin/python3

import hmac
import hashlib
import binascii

def tohex(x):
    return binascii.hexlify(x).decode('ascii')

def feistel(f, l, r, keys):
    for k in keys:
        t = f(k, r)
        print(tohex(l), tohex(r), tohex(t))
        l, r = r, xor(l, t)
    return l, r

def hmacround(k, x):
    return hmac.new(k, x, digestmod=hashlib.sha256).digest()

def expandkey(k, n):
    return hmac.new(k, bytes([n]), digestmod=hashlib.sha256).digest()

def encrypt(k, m, rounds=4):
    assert len(m) == 64
    l = m[0:32]
    r = m[32:64]
    keys = [expandkey(k, i) for i in range(rounds)]
    l0, r0 = feistel(hmacround, l, r, keys)
    return l0+r0

def decrypt(k, m, rounds=4):
    assert len(m) == 64
    l = m[0:32]
    r = m[32:64]
    keys = [expandkey(k, i) for i in reversed(range(rounds))]
    r0, l0 = feistel(hmacround, r, l, keys)
    return l0 + r0

def xor(a, b):
    assert len(a) == len(b)
    return bytes(x ^ y for x, y in zip(a, b))

def main():
    key = b"1"*32
    msg = b"0"*64
    c = encrypt(key, msg)
    print(tohex(c))
    print("decrypt")
    print(decrypt(key, c))


main()
