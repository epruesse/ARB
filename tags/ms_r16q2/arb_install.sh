#!/bin/bash

# error message function
err() {
    echo
    echo "********************************* ERROR ***********************" 1>&2
    echo "`basename $0`: $@" 1>&2
    echo "***************************** END OF ERROR ********************" 1>&2
    echo "Please fix and then rerun script !!!" 1>&2
    exit 1
}

cont() {
    echo "Warning: $@" 1>&2
    echo 'Do you want to continue [y]'
    read var
    case "$var" in
        n) err "Script aborted by user" ;;
        *) echo 'Continuing...' ;;
    esac
}

untar() {
        #remove old DEPOT BUG
    rm -f DEPOT/gde/bin

    if [ ! -r $cwd/$1 ]; then
        err "Cannot find or read file  $cwd/$1"
    fi
    if gunzip --stdout $cwd/$1 | tar xfv -; then
        echo ">>> $1 uncompressed and untared"
        if [ ! -e bin/arb_ntree ]; then
            err "bin/arb_ntree not found"
        fi
        if [ ! -x bin/arb_ntree ]; then
            err "bin/arb_ntree not executable"
        fi
    else
        err "Error in uncompressing or untaring $1"
    fi
}

separator() {
    echo ''
    echo '*******************************************'
}

separator
echo 'Welcome to the ARB Package'
separator
echo '  Note:   - You may abort this script with ctrl-"C"'
echo '          - You can rerun this script as often as you like'
echo '          - Old ARB data will be kept if requested (in this case'
echo '            you can simply change some options)'
echo '          - Pressing <return> will select the value in brackets'
separator
echo 'Please answer some questions:'
separator
echo 'Enter path where to install ARB ?'
echo '  ARB is not a single program but a set of programs, datafiles ...'
echo '  To install ARB correctly all files are stored in a single '
echo '  directory. This script creates such a directory and copies all'
echo '  data into it, sets the correct permissions and optionally installs'
echo '  some X11 stuff. Please enter the path of the directory where you want'
echo '  to install ARB.'
echo '  Notes:  This script optionally creates the destination directory.'
echo '          You should have write permission at the destination location.'
echo '                  - To install ARB in your home directory: Enter "'$HOME'/arb"'
echo '                  - Otherwise become root and rerun script.'
echo '                  - On Linux computers this script should be run under root'
echo ''
echo '  Example: If there is enough space (~50 MB) at your /usr partition and'
echo '  you want to install arb at /usr/arb, enter "/usr/arb"'

if [ "$ARBHOME" != "" ]; then
    if test -f $ARBHOME/lib/arb_tcp.dat; then
        echo "  Note:   I found an old arb program at $ARBHOME"
    fi
fi

echo "Enter full installation path: [${ARBHOME:-/usr/arb}]"
read ARBHOMEI
echo
echo

if [ "$ARBHOMEI" = "" ]; then
    ARBHOME="${ARBHOME:-/usr/arb}";
else
    ARBHOME="${ARBHOMEI}";
fi

cwd=`pwd`

if [ ! -f arb.tgz ]; then
    if [ -e arb-*.tgz ]; then
        # link any arb-*.tgz to arb.tgz
        ln -s arb-*.tgz arb.tgz
        ls -al arb.tgz arb-*.tgz
        if [ ! -L arb.tgz ]; then
            err "Failed to link any arb-tarball to arb.tgz (maybe there are multiple arb-*.tgz in this directory)"
        fi
    else
        err "Expected exactly one arb-*.tgz in current directory."
    fi
else
    ls -al arb.tgz arb-*.tgz
fi

if [ ! -f arb.tgz ]; then
    if [ -L arb.tgz ]; then
        rm arb.tgz
        err "arb.tgz does not link to a file - removed link"
    else
        err "arb.tgz is neighter file nor link - can't handle"
    fi
fi

if test -d $ARBHOME; then
    echo 'Creating backup copy of arb_tcp.dat ..'
    cp $ARBHOME/lib/arb_tcp.dat arb_tcp_`date +%Y%m%d`.dat

    if test -w $ARBHOME; then
        separator
        echo 'The destination directory'
        echo "    $ARBHOME"
        echo 'already exists!'
        echo '  You can delete the old directory before installing ARB'
        echo '  or only update/change options of the old version.'
        echo 'Delete old directory (y/n)[n]?'
        read delete_dir
        echo
        case "$delete_dir" in
            y)
            if rm -r $ARBHOME/* ;then
                echo ">>> all data in $ARBHOME deleted"
            else
                cont "cannot delete all data in $ARBHOME"
            fi;;
            *)
            if test -f $ARBHOME/lib/arb_tcp.dat; then
                bckup=$ARBHOME/lib/arb_tcp.dat.`date +%y%m%d%H%M%S`
                echo ">>>old $ARBHOME/lib/arb_tcp.dat found"
                echo ">>>backup to $bckup"
                cp $ARBHOME/lib/arb_tcp.dat $bckup
            fi
            echo ">>> old data not deleted";;
        esac
    else
        err "directory exists and is not writable";
    fi
else
    if mkdir -p $ARBHOME; then
        echo ">>> $ARBHOME created"
    else
        err "cannot create directory $ARBHOME";
    fi
fi

cd $ARBHOME
ARBHOME=`pwd`

if test -d lib/pictures; then
    separator
    echo "Old ARB package found (type n to change only some options)."
    echo "  Do you want to update the old package: (y/n)[y]"
    read var;
    case "$var" in
        n)
        echo "Old version unchanged";;
        *)
        echo "updating ARB";
        untar arb.tgz;;
    esac
else
    untar arb.tgz;
fi

separator
echo 'Specify PT_SERVER files location'
echo '  ARB needs a writeable directory to store the pt_server files. '
echo '  Those files are needed for fast database search'
echo '  (by probe_design, probe_match and the automatic aligner)'
echo '  and need a lot of disc space (up to several 100 mega bytes,'
echo '  e.g. 4000 16S RNA sequences require about 40 MB).'
echo '  This files are not created now. They can be build by any user via'
echo '          <ARB_NTREE/Probes/PT_SERVER admin/Build server>'
echo '  You may define a special directory as the pt_server files location.'
echo '  This prevents any loss of data installing a new version of '
echo '  ARB. '

echo 'Where do you want to put your pt_server data'
echo '          1. <CR> - (new installation of ARB)'
echo '                    for placing pt_server data within ARB directory tree'
echo '                    (Default location).'
echo '                  - (updating ARB)'
echo '                    using the previous location'
echo '          2. "Path" to link pt_server data directory to'
echo "              (if you choose this option you won't loose your PT-Servers when doing future software updates)"
echo 'Enter path:'
read pt_dir
echo
case "$pt_dir" in
    "")
    echo "installing the pt_server data in $ARBHOME/lib/pts"
    if test -h ${ARBHOME}/lib/pts ; then
        echo "Are you sure to delete "
        echo ">>> pt_server files at non default location:"
        ls -ld ${ARBHOME}/lib/pts
    else
        if test -d ${ARBHOME}/lib/pts; then
            echo ">>> pt_server files at default location: unchanged"
        else
            (cd lib;rm -f pts;mkdir pts;)
        fi
    fi;;
    *)
    echo "changing your pt_server file location"
    if test -h ${ARBHOME}/lib/pts; then
        echo ">>> non default location found: removing old link"
        rm lib/pts
    else
        if test -d ${ARBHOME}/lib/pts; then
            echo ">>> data in default location found"
            echo 'Do you want to remove old ptserver data (recommended)? [y]'
            read ANSWER
            case "$ANSWER" in
                n) echo 'data not deleted' ;;
                *) rm -r lib/pts
                echo 'data deleted' ;;
            esac
        fi
    fi
    if test ! -d $pt_dir; then
        echo ">>> Creating special PT_SERVER directory $pt_dir"
        if mkdir -p $pt_dir; then
            echo ">>> $pt_dir created"
        else
            err "Couldn't create $pt_dir"
        fi
    fi
    (cd lib;ln -s $pt_dir pts;)
esac

separator
echo 'Who is responsible for the PT_SERVER index files ?'
echo '  Answer  y: if you trust your users (less administration; recommended)'
echo '          n: if PT_SERVER files shall only be changable by administrator'
echo '  or simply press return to keep the settings of an old installation.'
echo 'Should everybody be allowed to build/update PT_SERVER files (y/n/dont_change)[dont_change]?'
read var
echo
case "$var" in
    y)
    echo ">>> all users are allowed to update the PT_SERVER";
    chmod 777 lib/pts
    chmod 666 lib/pts/* 2>/dev/null ;;
    n)
    echo ">>> only `whoami` is allowed to update the pt_server";
    chmod 755 lib/pts
    chmod 644 lib/pts/* 2>/dev/null ;;
    *)
    echo ">>> flags unchanged";;
esac

separator
echo 'NameServer installation'
echo '  The NameServer is a program, that synchronizes all species names'
echo '  of the databases of different users.'
echo '  Users that import foreign data into their database and want to'
echo '  export those data to other ARB users should be allowed to change'
echo '  the names file in $ARBHOME/lib/nas/names.dat'
echo '  Answer  y: if all users may import foreign databases'
echo '          n: if there are some mean untrusty users'
echo '  or simply press return to keep the old settings'
echo 'Do you trust your users (y/n/dont_change)[dont_change]?'
read var
echo
case "$var" in
    y)
    echo ">>> all user are allowed to change the names file";
    chmod 777 lib/nas
    chmod 666 lib/nas/*;;
    n)
    echo ">>> only `whoami` is allowed to change the names file ${ARBHOME}/lib/nas/names.dat"
    chmod 755 lib/nas
    chmod 644 lib/nas/*;;
    *)
    echo ">>> flags unchanged";;
esac

separator
echo 'Networking'
echo '  To speed up calculation one special host can be assigned as'
echo '  the PT_SERVER host. That means that all database search is done'
echo '  on that host. This saves computer resources as different users'
echo '  may share one server.'
echo '  To get the best results this host "H" should:'
echo '                  1.      be fast,'
echo '                  2.      have a lot of real memory (>=64 meg),'
echo '                  3.      have a lot of swap space (>=400 meg),'
echo '                  4.      allow all users to run "ssh H ...",'
echo '                  5.      contain the discs with the PT_SERVER files.'

echo '  n       You want to assign a special Network host'
echo '  s       You have a Stand alone computer'
if test "X$bckup" != "X"; then
    echo '        o       Use information of already installed ARB'
    echo 'Choose (s/n/o)[s]?'
else
    echo 'Choose (s/n)[s]?'

fi
read var
echo



case "$var" in
    o)
    mv $bckup lib/arb_tcp.dat;
    echo ">>> old lib/arb_tcp.dat restored";;
    n)
    separator
    echo "Enter the name of your host for the pt_server"
    read host
    echo "Checking connection to $host"
    if ssh $host ls >/dev/zero; then
        echo ">>> ssh $host ok"
    else
        err ">>> cannot run 'ssh $host'";
    fi
    rm -f lib/arb_tcp.dat;
    cat lib/arb_tcp_org.dat |sed -e "/localhost\:/$host\:/g" >lib/arb_tcp.dat
    echo ">>> server installed";;
    *)
    cp lib/arb_tcp_org.dat lib/arb_tcp.dat
    echo ">>> server installed";;
esac

separator
echo ">>> Installation Complete"

separator
SHELL_ANS=0

while [ "$SHELL_ANS" = "0" ]; do

    echo "Finally, you have to tell your system where to find arb."
    echo "First find out which shell you are using, by opening a new terminal window and typing"
    echo '        echo $SHELL'
    echo ""
    echo "Depending on what is your shell there are three choices:"
    echo ""
    echo "    1. Change your local .profile or .bashrc (if you are using ksh/bash, which"
    echo "                                              is the default shell for Linux)"
    echo "    2. Change your local .cshrc file         (if you are using csh/tcsh)"
    echo "    3. Create an alias for arb               (any shell)"
    echo ""

    echo "Enter (1,2 or 3) to achieve further installation instructions:"

    read var

    echo '';
    echo '**********************************************************************************************';
    echo 'Follow the steps below with care!!!';
    echo '';
    case "$var" in
        1)
        echo '******************************************************';
        echo "add the following lines to your ~/.profile";
        echo "or to your ~/.bashrc for bash-users";
        echo '******************************************************';
        echo "     ARBHOME=$ARBHOME;export ARBHOME";
        echo '     LD_LIBRARY_PATH=${ARBHOME}/lib:${LD_LIBRARY_PATH}';
        echo '     export LD_LIBRARY_PATH';
        echo '     PATH=${ARBHOME}/bin:${PATH}';
        echo '     export PATH';
        echo ' '
        echo 'enter the following command:';
        echo '     . ~/.profile'
        SHELL_ANS=1 ;;
        2)
        echo '******************************************';
        echo "add the following lines to your ~/.cshrc";
        echo '******************************************';
        echo "     setenv ARBHOME $ARBHOME";
        if test "X${LD_LIBRARY_PATH}" != "X"; then
            echo '     setenv LD_LIBRARY_PATH $ARBHOME/lib\:$LD_LIBRARY_PATH';
        else
            echo '     setenv LD_LIBRARY_PATH $ARBHOME/lib';
        fi
        echo '     setenv PATH $ARBHOME/bin\:$PATH';
        echo ' '
        echo 'enter the following command:';
        echo '     source ~/.cshrc'
        SHELL_ANS=1 ;;
        3)
        echo '**************************************************';
        echo "add one of the following lines to your init file";
        echo '**************************************************';
        echo "# bash users add to ~/.bashrc:";
        echo "  alias arb=$ARBHOME/bin/arb";
        echo "# tcsh users add to ~/.cshrc:";
        echo "  alias arb '$ARBHOME/bin/arb'";
        SHELL_ANS=1 ;;
        *)
        echo 'Wrong answer';;
    esac
done

echo ""
echo "Note for sysadmins:"
echo "     In order to provide arb for all users,"
echo "     edit the global shell init file(s) in /etc"
echo "     (/etc/bash.bashrc, /etc/csh.cshrc or similar)"
echo "     in the same manner as described above for the"
echo "     local shell init files."
echo ""
echo "Please read the files"
echo "  $ARBHOME/arb_INSTALL.txt"
echo "  $ARBHOME/arb_UBUNTU.txt    (if you're using ubuntu)"
echo "about additional libraries and tools needed by ARB."
echo ""
echo "When you installed everything and performed the changes to your init file,"
echo "open a new terminal window and start arb by typing"
echo ""
echo "     arb"
echo ""
echo "Have much fun using ARB"
echo "ARB Team <arb@arb-home.de>"
echo ""

unset host
unset cwd
unset ARBHOME
unset delete_dir
unset keep_debug
unset pt_dir
unset var
