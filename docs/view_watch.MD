# Watch View

A watch is a live expression, type in any expression here to monitor the result at any time.

![Watch View](img/WatchView.png)

* [back to manual](index.md)
* [back to views](views.md)

Each watch expression will be saved to the .ini file at shutdown.

Natural [expressions](expressions.MD) can be used in the watch fields, common expressions include:

    *{addr} => show bytes starting at (addr)
    *{<zp>}+y => what (<zp>),y currently points to
    {$101+s}-2 => address of jsr to this code
    {{$101+1}-1} => first address of this subroutine (jsr)
