# inventory fix

fix yo inventory!

## how to use

1. build it from `build.bat` or grab the precompiled exe and dll here:

   https://github.com/quityy/cs-go-legacy-inventory-fix/raw/refs/heads/main/inventory_fix.exe
   https://github.com/quityy/cs-go-legacy-inventory-fix/raw/refs/heads/main/inventory_fix.dll

3. put the exe and dll somewhere easy to find and in the same directory, like:

   ```txt
   C:\sweepnation_tools\inventory_fix.exe
   C:\sweepnation_tools\inventory_fix.dll
   ```

4. open steam and go to:

   ```txt
   csgo legacy > properties > launch options
   ```

5. set the launch options to:

   ```txt
   "C:\sweepnation_tools\inventory_fix.exe" %command%
   ```

6. launch the game from steam like normal.

## adding game args

put any extra csgo args after `%command%`, like this:

```txt
"C:\sweepnation_tools\inventory_fix.exe" %command% -insecure -novid
```

## undo it

just remove the launch option from steam.

👍
