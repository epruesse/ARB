define sep
        echo ------------------------\n
end
define head
        sep
        echo $arg0\n
        sep
end
define brk
        sep
        echo setting breakpoint $arg0\n
        sep
        break $arg0
end
define atstop
        # now program either terminated, crashed or hit a breakpoint

        head stop_reached
        where 1000 full

        head disassembled
        disassemble

        head listing
        set listsize 10
        list    

        # if program terminated or crashed, 'continue' will cause a script error and
        # the script will stop. Otherwise it loops forever.
        continue
        atstop
end


brk main
run

# now program is in main() - set all breakpoints

set breakpoint pending on

brk exit
brk _fini
brk malloc_error_break

set print pretty
set print array
#set print array-indexes
set print null-stop
set print elements 2000
set print repeats 100
set print object
set print vtbl

head shared_libraries
info sharedlibrary

set verbose

continue
atstop
