use ExtUtils::MakeMaker;
WriteMakefile(
        NAME    => 'ARB',
        OBJECT  => 'ARB_wrap.o',
        INC     => '-I../../INCLUDE',
        LDFROM  => 'ARB_wrap.o -L../../lib -lARBDB -lCORE',
        DEFINE  => $ENV{'DFLAGS'},
        INSTALLSITELIB => '../../lib/perl5/',
        INSTALLSITEARCH => '../../lib/perl5/'
);
