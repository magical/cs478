# pollard's rho algorithm

def rho(n):
    x = 2
    y = 2
    d = 1
    while d == 1:
        x = (x*x + 1) % n
        y = (y*y + 1) % n
        y = (y*y + 1) % n
        d = gcd(x - y, n)
    if d == n:
        return None
    return d

def gcd(a, b):
    while b != 0:
        a, b = b, a%b
    return a

def main():
    print(rho(503*509))
    print(rho(2**16))

main()
