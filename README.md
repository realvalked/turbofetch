# Turbofetch: Lightning fast and sleek fetch script for Linux ⚡

![image preview](https://i.imgur.com/VIPUCCg.png)

This project started as a way to learn how to pipe command output into a C string. And, of course, growing tired of Neofetch's slowness. Not to mention the absolute behemoth that is Screenfetch (who even uses that anymore?). The ending result, I'd say, is pretty nice.

The main objective of Turbofetch is to look very nice and sleek, while also being able to load ultra-fast. And, just that it accomplishes.

Turbofetch is also very customizable. You can change its colours in the main.c file, and even edit it to use a custom logo if you wish.

## Building

The building process is very simple:

```
gcc main.c -o turbofetch -O2 -lpthread -lpci
sudo mv turbofetch /usr/bin/
```

## Performance

Neofetch loaded in about 0.180s. **That's pretty slow!** We can do better than that, right?

Using [Minifetch](https://github.com/RohanKP1/minifetch), we get results that are a lot better than those of Neofetch, loading in about 0.056s. That's very good, **but we can do even better, while fetching additional information!**

Using [Turbofetch](https://github.com/xnqs/turbofetch/), we get results even better than those of Minifetch! It loads in about **0.007s!!!** That's straight up amazing.

## Conclusion

I'm sure that my code is still heavily unoptimized, and that I still have a lot to optimize, but for now, this is a decent solution to my first world problems.
