# Language Changes

## V0.1
- The GOAL language version has been set to 0.1
- Calling a function with unknown argument/return types is now an error instead of a warning
- Getting a method of an object or type with `method` returns the correct type for methods using the `_type_` feature
- The `object-new` macro will now type check arguments
- The size argument to `(method object new)` is now an `int` instead of `int32`
- Using `set!` incorrectly, like `(set! 1 2)` will now create an error instead of having no effect
- GOOS now has a `fmt` form which wraps `libfmt` for doing string formatting.
- GOOS now has an `error` form for throwing an error with a string to describe it
- GOAL `if` now throws errors on extra arguments instead of silently ignoring them
- The first 1 MB of GOAL memory now cannot be read/written/executed so dereferencing a GOAL null pointer will now segfault
- The runtime now accepts command line boot arguments
- The runtime now defaults to loading `KERNEL.CGO` and using its `kernel-dispatcher` function.
- The runtime now accepts a `-nokernel` parameter for running without `KERNEL.CGO`.
- The runtime will now refuse to load object files from another major GOAL version
- Using `&+` and `&+!` now produces a pointer with the same type as the original.
- There is a `&-` which returns a `uint` and works with basically any input types
- The `&` operator works on fields and elements in arrays
- The `&->` operator has been added
- The `new` operator can create arrays and inline arrays on heaps
- The value of `deftype` is now `none`
- Creating a method with more than 8 arguments is an error instead of a crash.
- The `defconstant` form for defining a constant in GOAL but not GOOS has been added
- Both `defconstant` and `defglobalconstant` throw an error if you define a constant with the same name as a symbol.
- The `uint64` type now uses 8 bytes instead of 81 in a type (this was a typo)
- `deftype` allows basics/structures with a field that is the same type as the basic/structure.
- Doing a `define-extern` with a type of `type` will forward declare the type.
- `deftype` now has a `:no-runtime-type` flag to disable the creation of a runtime type.
- There is a `declare-type` form for forward declaring types to allow circular dependencies.
- Types that are `structure` but not `basic` can request that they be tightly packed when possible with `:pack-me`.
- Using `method` on a forward declared type is an error. The old behavior was to get a method of `type`, which is confusing.
- Loading an `int64`/`uint64` gives a `int`/`uint`, like the other register integers.
- Defining a type with `deftype` will auto-generate an inspect method.
- The `new` operator can now create static structures and basics and set fields to integers or symbols.
- The `neq?` operator now works when used outside of a branch condition (previously it generated a syntax error)
- Methods which do not return a value no longer cause the compiler to abort
- The `&+` form now accepts more than two arguments.
- The `&+` form now works on `inline-array` and `structure`.
- In the case where the type system would use a result type of `lca(none, x)`, the result type is now `none` instead of compiler abort.
- The "none value" is now `(none)` instead of `none`

- Creating a field of 128-bit value type no longer causes a compiler crash
- 128-bit fields are inspected as `<cannot-print>`
- Static fields can now contain floating point values
- Fixed a bug where loading a float from an object and immediately using it math would cause a compiler crash

- Arrays of value types can be created on the stack with `new`.

## V0.2
- Breaking change: return type of a function using `return-from #f` to return a value from the entire function is now the lowest common ancestor of all possible return values.
- Fixed bug where `return-from` could reach outside of an inlined function.
- Fixed bug where `return-from` might not behave correctly when returning from inside a let inside an inlined function.