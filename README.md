on_idle
=======

`on_idle` reads a list of time-outs (in seconds) paired with commands
from standard input, then executes them when the mouse cursor becomes
idle.

Lines should look like this:

> 20 echo Stop slacking off, you lazy bum!
> 40 echo If you don't move the mouse cursor right now ...
> 41 echo I'm going to delete your ssh keys from memory!
> 50 echo Last chance!
> 60 ssh-add -D
