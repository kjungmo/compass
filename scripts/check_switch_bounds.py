#!/usr/bin/env python3
"""Numerical falsification checks for paper recurrences; not a proof or robot test."""
import math
import random
from fractions import Fraction as F
rng=random.Random(5)
for _ in range(2000):
    lam=rng.uniform(.1,.99); h=rng.uniform(.01,.2); delta=.05
    d=rng.uniform(.2,2.8); a=(d-delta)*h
    threshold=rng.uniform(.1,.9)*a/(1-lam)
    cap=threshold+rng.random()
    bound=math.ceil(math.log1p(-threshold*(1-lam)/a)/math.log(lam))
    e=0.; hit=None
    for n in range(1,bound+1):
        e=min(cap,max(0,lam*e+a))
        if e>=threshold: hit=n; break
    assert hit is not None,(lam,h,d,threshold,bound)
for _ in range(2000):
    e=0.; elapsed=0.; delta=.05; dmax=2.8; threshold=.3
    for _ in range(100):
        dt=rng.uniform(.005,.2);elapsed+=dt
        e=min(.5,max(0,rng.uniform(.1,1)*e+(rng.uniform(-2.8,dmax)-delta)*dt))
        assert e <= (dmax-delta)*elapsed+1e-12
        if e>=threshold:
            assert elapsed >= threshold/(dmax-delta)-1e-12
            e=0.;elapsed=0.
# Exact arithmetic for equilibrium equality: no finite crossing from below.
e=F(0);lam=F(1,2);threshold=F(1);a=F(1,2)
for _ in range(100):
    e=lam*e+a
    assert e<threshold
# Rolling-window guard counterexample: infinitely extendable sparse prefix.
for t in range(0,400,4):
    assert sum(t-3<x<=t for x in range(0,t+1,4))==1
print('GREEN: 2000 response cases, 2000 variable-period traces, exact equality, sparse-window counterexample')
