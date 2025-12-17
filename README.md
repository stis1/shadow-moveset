# rangers-sdk example

This is an example starter project using the rangers-sdk to build your own DLL mods.

## Setting up the development environment

You will need to have the following prerequisites installed:

* Visual Studio 2022
* CMake 3.28 or higher

Check out the project and make sure to also check out its submodules:

```sh
git clone --recurse-submodules https://github.com/angryzor/sonic-frontiers-modding-framework.git
```

Now let CMake do its thing:

```sh
cmake -A x64 -B build
```

If you have Sonic Frontiers installed in a non-standard location, you can specify that location
with the `SFMF_GAME_FOLDER` variable:

```sh
cmake -A x64 -B build -DSFMF_GAME_FOLDER="C:\ShadowFrontiers"
```

Once CMake is finished, navigate to the build directory and open `SFMF.sln` with VS2022.
You should have a fully working environment available.

Building the INSTALL project will install the mod into HedgeModManager's `Mods` directory.
