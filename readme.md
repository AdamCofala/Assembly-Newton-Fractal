# Newton Fractal Renderer (C++ / x86-64 Assembly)

Interactive visualization of the Newton fractal for the polynomial

$$
f(z) = z^3 - 1
$$

implemented in C++ and x86-64 Assembly with OpenGL rendering and ImGui controls.

## Overview

This project generates a Newton fractal by applying Newton's method to every pixel of the complex plane. Each pixel represents a starting point (z_0), and the algorithm iteratively searches for one of the roots of

$$
z^3 - 1 = 0.
$$

The resulting convergence pattern produces the characteristic Newton fractal structure.

Features:

* Interactive zooming and panning
* Adjustable iteration count
* OpenGL texture rendering
* ImGui control panel
* x86-64 Assembly implementation of the fractal generator
* Reference C++ implementation

---

## Mathematical Background

### Newton's Method

For a function (f(z)), Newton's iteration is defined as

$$
z_{n+1}=z_n-\frac{f(z_n)}{f'(z_n)}.
$$

For this project

$$
f(z)=z^3-1
$$

and

$$
f'(z)=3z^2.
$$

Substituting into Newton's formula gives

$$
z_{n+1}=z_n - \frac{z_n^3-1}{3z_n^2}.
$$

This iteration is repeated until convergence or until the maximum iteration count is reached.

---

### Complex Arithmetic

A complex number is represented as

$$
z=z_r+i z_i.
$$

#### Squaring

$$
z^2=(z_r+i z_i)^2
$$

$$
z^2=(z_r^2-z_i^2)+i(2z_rz_i).
$$

Therefore

$$
\Re(z^2)=z_r^2-z_i^2
$$

$$
\Im(z^2)=2z_rz_i.
$$

---

#### Cubing

$$
z^3=z^2\cdot z.
$$

Using complex multiplication

$$(a+ib)(c+id)=(ac-bd)+i(ad+bc),$$


we obtain

$$
z^3=
(z_r^3-3z_rz_i^2)
+
i(3z_r^2z_i-z_i^3).
$$

---

### Function Evaluation

For

$$
f(z)=z^3-1
$$

the real and imaginary components are

$$
f_r=\Re(z^3)-1
$$

$$
f_i=\Im(z^3).
$$

---

### Derivative Evaluation

Since

$$
f'(z)=3z^2,
$$

we compute

$$
f'_r=3(z_r^2-z_i^2)
$$

$$
f'_i=6z_rz_i.
$$

---

### Complex Division

Newton's method requires

$$
\frac{f(z)}{f'(z)}.
$$

For

$$
\frac{a+ib}{c+id}
$$

the division formula is

$$
\frac{a+ib}{c+id}=\frac{(ac+bd)+i(bc-ad)}{c^2+d^2}.
$$

This is used directly in the implementation.

---

## Polynomial Roots

The roots of

$$
z^3-1=0
$$

are

$$
z_1=1,
$$

$$
z_2=-\frac12+\frac{\sqrt3}{2}i,
$$

$$
z_3=-\frac12-\frac{\sqrt3}{2}i.
$$

After the iteration process finishes, the closest root is selected.

---

## Coloring

The convergence speed determines pixel brightness.

Let

$$
t=\frac{\text{iterations}}{\text{maxIterations}}.
$$

Brightness is computed as

$$
B=B_{root}(1-t)
$$

where (B_{root}) is a root-dependent brightness coefficient.

Pixels that converge quickly appear brighter, while pixels near basin boundaries appear darker.

---

## Controls

### Mouse

| Action                   | Effect        |
| ------------------------ | ------------- |
| Mouse Wheel              | Zoom In / Out |
| Left Mouse Button + Drag | Pan View      |

### GUI

| Setting        | Description                         |
| -------------- | ----------------------------------- |
| Max Iterations | Maximum number of Newton iterations |

---

## Project Structure

```text
src/
├── main.cpp
├── newton.asm
├── shaders
└── imgui
```

### Main Components

* `newton_cpp()` - reference C++ implementation
* `newton_asm()` - optimized x86-64 assembly implementation
* OpenGL renderer for displaying generated textures
* ImGui interface for runtime parameter adjustment

---

## Rendering Pipeline

1. Convert screen coordinates into complex plane coordinates.
2. Apply Newton iterations.
3. Determine the converged root.
4. Compute pixel brightness.
5. Store RGB values into the framebuffer.
6. Upload framebuffer to an OpenGL texture.
7. Render a fullscreen quad.

---

## Build Requirements

* C++17
* OpenGL 3.3+
* GLFW
* GLAD
* ImGui
* NASM (for Assembly version)
* CMake

---