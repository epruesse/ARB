#Simple makefile to build muscle inside ARB
#The super long SOURCES list could be avoided if all .cpp files of muscle would compile…
#By Arne Boeckmann

CXX ?= g++
CXXFLAGS=-c -O3 -msse2 -mfpmath=sse -D_FILE_OFFSET_BITS=64 -DNDEBUG=1
LDFLAGS= -g
SOURCES=aligngivenpath.cpp aligngivenpathsw.cpp aligntwomsas.cpp aligntwoprofs.cpp aln.cpp alpha.cpp anchors.cpp bittraceback.cpp blosum62.cpp blosumla.cpp clust.cpp cluster.cpp clwwt.cpp color.cpp cons.cpp diaglist.cpp diffobjscore.cpp diffpaths.cpp difftrees.cpp difftreese.cpp distcalc.cpp distfunc.cpp distpwkimura.cpp domuscle.cpp dosp.cpp dpreglist.cpp drawtree.cpp edgelist.cpp enumopts.cpp enumtostr.cpp estring.cpp fasta.cpp fasta2.cpp fastclust.cpp fastdist.cpp fastdistjones.cpp fastdistkbit.cpp fastdistkmer.cpp fastdistmafft.cpp fastdistnuc.cpp fastscorepath2.cpp finddiags.cpp finddiagsn.cpp glbalign.cpp glbalign352.cpp glbaligndiag.cpp glbalignle.cpp glbalignsimple.cpp glbalignsp.cpp glbalignspn.cpp glbalignss.cpp glbalndimer.cpp globals.cpp globalslinux.cpp globalsosx.cpp globalsother.cpp globalswin32.cpp gonnet.cpp henikoffweight.cpp henikoffweightpb.cpp html.cpp hydro.cpp intmath.cpp local.cpp main.cpp makerootmsa.cpp makerootmsab.cpp maketree.cpp mhack.cpp mpam200.cpp msa.cpp msa2.cpp msadistkimura.cpp msf.cpp muscle.cpp muscleout.cpp nucmx.cpp nwdasimple.cpp nwdasimple2.cpp nwdasmall.cpp nwrec.cpp nwsmall.cpp objscore.cpp objscore2.cpp objscoreda.cpp onexception.cpp options.cpp outweights.cpp pam200mafft.cpp params.cpp phy.cpp phy2.cpp phy3.cpp phy4.cpp phyfromclust.cpp phyfromfile.cpp physeq.cpp phytofile.cpp posgap.cpp ppscore.cpp profdb.cpp profile.cpp profilefrommsa.cpp progalign.cpp progress.cpp progressivealign.cpp pwpath.cpp readmx.cpp realigndiffs.cpp realigndiffse.cpp refine.cpp refinehoriz.cpp refinesubfams.cpp refinetree.cpp refinetreee.cpp refinevert.cpp refinew.cpp savebest.cpp scoredist.cpp scoregaps.cpp scorehistory.cpp scorepp.cpp seq.cpp seqvect.cpp setblosumweights.cpp setgscweights.cpp setnewhandler.cpp spfast.cpp sptest.cpp stabilize.cpp subfam.cpp subfams.cpp sw.cpp termgaps.cpp textfile.cpp threewaywt.cpp tomhydro.cpp traceback.cpp tracebackopt.cpp tracebacksw.cpp treefrommsa.cpp typetostr.cpp upgma2.cpp usage.cpp validateids.cpp vtml2.cpp writescorefile.cpp
OBJECTS=$(addprefix obj/,$(notdir $(SOURCES:.cpp=.o)))
PREFIX=$(ARBHOME)

muscle: setup $(OBJECTS)
	$(CXX) $(LDFLAGS) $(OBJECTS) -o muscle


setup:
	mkdir -p obj

obj/%.o: src/%.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<

install:
	cp -f muscle $(PREFIX)/bin

clean:
	rm -rf obj
	rm -f muscle
	rm -f $(PREFIX)/bin/muscle



