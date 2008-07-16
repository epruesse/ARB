
$(MAIN): arb_version_update

arb_build.h: arb_version_update

arb_version_update:
	echo '#define DATE' `date '+"%y.%m.%d"'`   >  arb_build.h.tmp
	echo '#define DATE_YEAR' `date '+"%Y"'`    >> arb_build.h.tmp
	echo '#define BUILD_HOST "'`echo $$HOSTNAME|sed -e 's/[.-]/_/g'`'"' >> arb_build.h.tmp
	echo '#define BUILD_USER "'`echo $$USER`'"'    >> arb_build.h.tmp
	../SOURCE_TOOLS/mv_if_diff arb_build.h.tmp arb_build.h

