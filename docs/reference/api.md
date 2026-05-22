# WalkLang API

Source: `examples/v1.walk`

## examples/math_extra.walk

### Functions

#### `func bump(x int) int`

Increments an integer by one.

Params:

- `x`: value to increment

Returns: the incremented value.

Example:

```walk
out: bump(4)
```

Since: `v4.1.0`

#### `func cube(x int) int`

Cubes an integer.

Params:

- `x`: value to multiply by itself twice

Returns: the cubed value.

Example:

```walk
out: cube(3)
```

Since: `v4.1.0`

### Exports

#### `exp cube`

Exposes cube from the math_extra example module.

Example:

```walk
exp: cube
```

Since: `v4.1.0`

