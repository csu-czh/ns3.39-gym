
## Introduction
This is an NS3 study repository. To set it up, you should first install NS-3.39. After that, you need to download the folders "ns-3.39/contrib/" and "ns-3.39/scratch/". Move these two folders to their corresponding locations within the NS3 directory. This is a straightforward method for installing this repository.

Alternatively, you can clone this repository and then configure and build it as shown in the following steps. 

Custom Modules:
Location: ns-3.39/contrib/

Simulation Main Programs:
Location: ns-3.39/scratch/

## Prerequisites
C++ compiler: gcc version 11.4.0

Python: Python 3.10.6

CMake: cmake version 3.22.1

make: GNU Make 4.3

ns3: ns-3.39

## Configure ns3
```
$ cd ns3.39-gym/ns-3.39/
$ ./ns3 configure --enable-examples --enable-tests
```

## Building and testing ns-3

```
$ ./ns3 build
$ ./test.py
```
## Run

```
$ ./ns3 run first
```

The session maintains all multicast states, and the routing incorporates a reference point that represents the session (though not a true simulation).
