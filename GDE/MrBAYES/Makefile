# MrBayes Makefile for ARB
# by Arne Böckmann (2013)

PROG  = MrBAYES
FOLDER =$(ARBHOME)/GDE/MrBAYES/mrbayes_3.2.1
# ARBs standard target
$(MAIN) : mrbayes.stamp


clean :
	rm -f $(ARBHOME)/bin/mb
	rm -f mrbayes.stamp
	cd $(FOLDER) && make clean


mrbayes.stamp: 
	cd $(FOLDER) && make mb
	cp $(FOLDER)/mb $(ARBHOME)/bin/mb
	touch mrbayes.stamp

	

.PHONY : clean


	