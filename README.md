<h1 align="center">
  Novae
  <br>
</h1>

<h4 align="center">Productivity manager and tracker.</h4>

<p align="center">
  <a href="#compiling">Compiling</a> 
</p>

![novae_early_dev](https://github.com/user-attachments/assets/3ea95cba-f8db-4956-8ff2-f8777a65d354)


> **Notes**
> + App is very early in development
> + Currently the only supported OS is linux
## Compiling

```bash
# Clone this repository
$ git clone https://github.com/quiet-visage/Novae novae

# Go into the repository
$ cd novae

# Create build directory
$ mkdir -p build

# Generate build files
$ cd build; cmake ..

# Compile
$ make -j8

# Run
$ ./novae
```


