define sep
        echo ------------------------\n
end
define head
        sep
        echo $arg0
        echo \n
        sep
end

break main
run
break exit
break _fini
continue

set print pretty

head shared_libraries
info sharedlibrary

set verbose

head disassembled
disassemble

head listing
frame 0
set listsize 30
list

head where
where 1000 full

