# How to Clean Build Cache

If you're still seeing compilation errors, try cleaning the build cache:

## In PlatformIO (VS Code):

1. Open the PlatformIO menu (alien icon in the left sidebar)
2. Click "Clean" (trash can icon) or press Ctrl+Alt+C (Cmd+Option+C on Mac)
3. Then click "Build" or press Ctrl+Alt+B

## Or use Terminal:

```bash
pio run --target clean
pio run
```

## Or manually delete build folder:

```bash
rm -rf .pio
pio run
```

This will force a complete rebuild and should fix any caching issues.

