# Build instructions for Mac OSX

This document is in a research stage. I don't have a working mac so I will have to borrow one at some points to work through the information

## Make File

The makefile included for building Linux versions have been tested
on macs with the same steps as [building the linux version](linux.MD),
but trying to find more straightforward methods

## XCode External Build Project
XCode can build makefile projects:
https://developers.google.com/j2objc/guides/external-build-projects, https://acecilia.medium.com/setup-a-c-c-makefile-project-in-xcode-syntax-highlight-autocompletion-jump-to-definition-and-80405bd4542e

## Prerequisites

Install command-line tools:

```
xcode-select --install
```

Install glfw:

```
brew install glfw
```

## Build

```
cd src
make
```
