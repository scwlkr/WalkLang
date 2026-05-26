# WalkLang API

Source: `examples/stable.walk`

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

Since: `current`

#### `func cube(x int) int`

Cubes an integer.

Params:

- `x`: value to multiply by itself twice

Returns: the cubed value.

Example:

```walk
out: cube(3)
```

Since: `current`

### Exports

#### `exp cube`

Exposes cube from the math_extra example module.

Example:

```walk
exp: cube
```

Since: `current`

