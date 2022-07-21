#!/bin/bash
if [ -d "build" ]; then
  rm -rf build/*
fi
if [ -d "bin" ]; then
  rm -rf bin/*
fi
if [ -d "lib" ]; then
  rm -rf lib/*
fi
