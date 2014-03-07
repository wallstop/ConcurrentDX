ConcurrentDX
============

Lightweight (no-longer-header-only) C++ concurrency library

*Disclaimer: I have no idea what I'm doing.*

# Using ConcurrentDX
ConcurrentDX is a lightweight concurrency library, providing some lock-free containers and utilities. It relies on some C++ 11 features, however. You will need:

  * A (mostly) C++11 standard-compliant C++ compiler (MSVC10 and later works)
  * A copy of this repo
  * To do the following with your project (where the include paths have been set up to point to whatever directory holds the repo):

```c++
#include <ConcurrentDX/ConcurrentDXLib.h>
```
  * and make sure that the project is set up to include either your version of the library, or one of the ones provided on this page

# Documentation
Documentation can be found [here](/html/index.html). The best way to view the documentation is to clone the repo and open up the html off the disk.
