# inventory fix

simple steam launch wrapper for csgo legacy inventory.

## how to use

1. build the exe from `main.cpp`.

2. put the exe somewhere easy to find, like:

   ```txt
   C:\sweepnation_tools\inventory_fix.exe
   ```

3. open steam and go to:

   ```txt
   csgo legacy > properties > launch options
   ```

4. set the launch options to:

   ```txt
   "C:\sweepnation_tools\inventory_fix.exe" %command%
   ```

5. launch the game from steam like normal.

## adding game args

put any extra csgo args after `%command%`, like this:

```txt
"C:\sweepnation_tools\inventory_fix.exe" %command% -insecure -novid
```

## undo it

just remove the launch option from steam.

👍