---
title: Hello, World!
---

# Hello, World!

You can get started with just a few commands as the demo shows. Let's create a new Poac project:

```console
you:~$ poac new hello_world
     Created binary (application) `hello_world` package
```

Now, you can use the `poac run` command to run your application:

```console
you:~$ cd hello_world
you:~/hello_world$ poac run
 Compiling src/main.cc
   Linking hello_world
  Finished debug target(s) in 0.45386s
   Running poac-out/debug/hello_world
Hello, world!
```
