# Linux build instructions for IceBro Lite

It is trivial to build IceBro Lite for Linux. In order to avoid missing steps I have followed these steps on a fresh Raspberry PI install of Debian.

---
### Get the files

* Open a terminal
* Go to a, or create a new, development folder such as ~/dev
* clone the repo

```
git clone https://github.com/Sakrac/IceBroLite.git
```

---

### Install dependencies

```
sudo apt-get install libglfw3-dev
sudo apt-get install pkg-config
sudo apt-get install clang
```

### Build the project

```
cd IceBroLite/src
make
```

If successful IceBroLite is now in the project folder!

---

### Updating

To update with the latest at a future time simply go to the project folder and update

```
git pull
make
```