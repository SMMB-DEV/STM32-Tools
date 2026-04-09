# Utils.hpp

This file contains utility classes and generic helper functions.

## Classes:
- `ScopeAction`, `ScopeActionF`: To have a piece of code executed when returning from a function or exiting a scope
- `ClampedInt`, `DynClampedInt`: A wrapper for an integer type with a value constrained to a min and max
- `LinkedList`: Similar to `std::forward_list`, useful for queuing work inside an ISR to be handled in the main loop
- `StaticQueue`: A queue with a static (fixed size, pre-allocated) circular buffer, also useful for queuing tasks from an ISR. More robust and performant but memory consuming.
- `PriorityQueue`: Similar to `StaticQueue` but items are removed based on their priority first instead of order of insertion

---

##### [Go Back](./README.md)
