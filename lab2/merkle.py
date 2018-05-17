import os
import hashlib
import binascii
import zlib
import math

def hash(s):
    return hashlib.sha256(s).digest()

def create(leaves):
    nodes = []
    for leaf in leaves:
        nodes.append(hash(leaf))

    assert len(leaves) & (len(leaves) - 1) == 0 # power of two

    nlevels = int(math.log(len(leaves), 2))
    for level in range(0, nlevels):
        newnodes = []
        for i in range(0, len(nodes), 2):
            newnodes.append(hash(nodes[i] + nodes[i+1]))
        nodes = newnodes
    assert len(nodes) == 1
    return nodes[0]

def create2(leaves):
    stack = []
    for (i, leaf) in enumerate(leaves):
        stack.append(hash(leaf))

        for _ in range(ctz(~i)):
            b = stack.pop()
            a = stack.pop()
            stack.append(hash(a + b))

    assert len(stack) == 1
    return stack[-1]

def ctz(x):
    n = 0
    while x != 0 and x & 1 == 0:
        x >>= 1
        n += 1
    return n

def path(leaves, v):
    stack = []
    path = []
    level = 0
    for (i, leaf) in enumerate(leaves):
        stack.append(hash(leaf))

        # the path should consist of every intermediate node involving the target node
        # AFTER pushing our target node,
        # every time the stack reaches the same level, that means that something
        # was merged with our node to the right
        # every time the stack reaches a lower level, that means that we merged
        # with something to the left
        if v == i:
            level = len(stack)

        for _ in range(ctz(~i)):
            b = stack.pop()
            a = stack.pop()
            stack.append(hash(a + b))

            if i >= v and len(stack) == level:
                path.append(b)
            if i >= v and len(stack) < level:
                path.append(a)
                level = len(stack)

    assert len(stack) == 1
    return path

def verify(root, path, v, index):
    h = hash(v)
    for p in path:
        print(tohex(h))
        if index & 1 == 0:
            h = hash(h + p)
        else:
            h = hash(p + h)
        index >>= 1
    return root == h

def tohex(b):
    return binascii.hexlify(b).decode('latin1')

def test():
    leaves = [str(i) for i in range(8)]
    print(tohex(create(leaves)))
    print(tohex(create2(leaves)))
    print(map(tohex, path(leaves, 0)))
    print(map(tohex, path(leaves, 15)))
    print(map(tohex, path(leaves, 5)))

    print(verify(create2(leaves), path(leaves, 0), str(0), 0))
test()
