# Copyright (c) 2004 - 2005 Kai Bader <baderk@in.tum.de>
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
# AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
# AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
# CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

# CVS REVISION TAG  --  $Revision$

# INCLUDE und GLOBALS MAIN CC cflags werden von aussen uebergeben

.SUFFIXES: .o .cxx .depend

CPP_OBJECTS = main.o dialog.o arb_interface.o main_dialog.o \
import_dialog.o selection_dialog.o file_import.o tiff_image.o \
image_dialog.o plot.o analyze_window.o entry_sel_dialog.o \
config_dialog.o help_dialog.o msgbox.o

$(MAIN): $(CPP_OBJECTS)
	$(LINK_STATIC_LIB) $(MAIN) $(CPP_OBJECTS)

.cxx.o:
	$(CPP) $(cflags) -c -o $@ $(@:.o=.cxx)  $(CPPINCLUDES) $(POST_COMPILE)

clean:
	rm -f $(CPP_OBJECTS) *.a

DEPENDS = $(CPP_OBJECTS:.o=.depend)
depends: $(DEPENDS)
	@cat $(DEPENDS) | grep -v '^#' >>Makefile
	@rm $(DEPENDS)
$(DEPENDS): depend.init
depend.init:
	$(MAKEDEPEND) $(MAKEDEPENDFLAGS) 2>/dev/null # remove dependencies
.c.depend:
	$(MAKEDEPEND) -f- $(MAKEDEPENDFLAGS) $< 2>/dev/null >$@
.cxx.depend:
	$(MAKEDEPEND) -f- $(MAKEDEPENDFLAGS) $< 2>/dev/null >$@

# DO NOT DELETE

# Do not add dependencies manually - use 'make depend' in $ARBHOME
# For formatting issues see SOURCE_TOOLS/fix_depends.pl

analyze_window.o: analyze_window.hxx
analyze_window.o: dialog.hxx
analyze_window.o: plot.hxx

arb_interface.o: arb_interface.hxx
arb_interface.o: xm_defs.hxx
arb_interface.o: $(ARBHOME)/INCLUDE/ad_k_prot.h
arb_interface.o: $(ARBHOME)/INCLUDE/ad_prot.h
arb_interface.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
arb_interface.o: $(ARBHOME)/INCLUDE/arb_assert.h
arb_interface.o: $(ARBHOME)/INCLUDE/arb_core.h
arb_interface.o: $(ARBHOME)/INCLUDE/arb_error.h
arb_interface.o: $(ARBHOME)/INCLUDE/arbdb.h
arb_interface.o: $(ARBHOME)/INCLUDE/arbdb_base.h
arb_interface.o: $(ARBHOME)/INCLUDE/arbdbt.h
arb_interface.o: $(ARBHOME)/INCLUDE/arbtools.h
arb_interface.o: $(ARBHOME)/INCLUDE/attributes.h
arb_interface.o: $(ARBHOME)/INCLUDE/aw_base.hxx
arb_interface.o: $(ARBHOME)/INCLUDE/dupstr.h
arb_interface.o: $(ARBHOME)/INCLUDE/EXP.hxx
arb_interface.o: $(ARBHOME)/INCLUDE/smartptr.h
arb_interface.o: $(ARBHOME)/INCLUDE/test_global.h

config_dialog.o: arb_interface.hxx
config_dialog.o: config_dialog.hxx
config_dialog.o: dialog.hxx
config_dialog.o: $(ARBHOME)/INCLUDE/ad_k_prot.h
config_dialog.o: $(ARBHOME)/INCLUDE/ad_prot.h
config_dialog.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
config_dialog.o: $(ARBHOME)/INCLUDE/arb_assert.h
config_dialog.o: $(ARBHOME)/INCLUDE/arb_core.h
config_dialog.o: $(ARBHOME)/INCLUDE/arb_error.h
config_dialog.o: $(ARBHOME)/INCLUDE/arbdb.h
config_dialog.o: $(ARBHOME)/INCLUDE/arbdb_base.h
config_dialog.o: $(ARBHOME)/INCLUDE/arbdbt.h
config_dialog.o: $(ARBHOME)/INCLUDE/arbtools.h
config_dialog.o: $(ARBHOME)/INCLUDE/attributes.h
config_dialog.o: $(ARBHOME)/INCLUDE/dupstr.h
config_dialog.o: $(ARBHOME)/INCLUDE/smartptr.h
config_dialog.o: $(ARBHOME)/INCLUDE/test_global.h

dialog.o: dialog.hxx
dialog.o: $(ARBHOME)/INCLUDE/ad_k_prot.h
dialog.o: $(ARBHOME)/INCLUDE/ad_prot.h
dialog.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
dialog.o: $(ARBHOME)/INCLUDE/arb_assert.h
dialog.o: $(ARBHOME)/INCLUDE/arb_core.h
dialog.o: $(ARBHOME)/INCLUDE/arb_error.h
dialog.o: $(ARBHOME)/INCLUDE/arbdb.h
dialog.o: $(ARBHOME)/INCLUDE/arbdb_base.h
dialog.o: $(ARBHOME)/INCLUDE/arbdbt.h
dialog.o: $(ARBHOME)/INCLUDE/arbtools.h
dialog.o: $(ARBHOME)/INCLUDE/attributes.h
dialog.o: $(ARBHOME)/INCLUDE/dupstr.h
dialog.o: $(ARBHOME)/INCLUDE/smartptr.h
dialog.o: $(ARBHOME)/INCLUDE/test_global.h

entry_sel_dialog.o: arb_interface.hxx
entry_sel_dialog.o: dialog.hxx
entry_sel_dialog.o: entry_sel_dialog.hxx
entry_sel_dialog.o: $(ARBHOME)/INCLUDE/ad_k_prot.h
entry_sel_dialog.o: $(ARBHOME)/INCLUDE/ad_prot.h
entry_sel_dialog.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
entry_sel_dialog.o: $(ARBHOME)/INCLUDE/arb_assert.h
entry_sel_dialog.o: $(ARBHOME)/INCLUDE/arb_core.h
entry_sel_dialog.o: $(ARBHOME)/INCLUDE/arb_error.h
entry_sel_dialog.o: $(ARBHOME)/INCLUDE/arbdb.h
entry_sel_dialog.o: $(ARBHOME)/INCLUDE/arbdb_base.h
entry_sel_dialog.o: $(ARBHOME)/INCLUDE/arbdbt.h
entry_sel_dialog.o: $(ARBHOME)/INCLUDE/arbtools.h
entry_sel_dialog.o: $(ARBHOME)/INCLUDE/attributes.h
entry_sel_dialog.o: $(ARBHOME)/INCLUDE/dupstr.h
entry_sel_dialog.o: $(ARBHOME)/INCLUDE/smartptr.h
entry_sel_dialog.o: $(ARBHOME)/INCLUDE/test_global.h

file_import.o: arb_interface.hxx
file_import.o: file_import.hxx
file_import.o: $(ARBHOME)/INCLUDE/ad_k_prot.h
file_import.o: $(ARBHOME)/INCLUDE/ad_prot.h
file_import.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
file_import.o: $(ARBHOME)/INCLUDE/arb_assert.h
file_import.o: $(ARBHOME)/INCLUDE/arb_core.h
file_import.o: $(ARBHOME)/INCLUDE/arb_error.h
file_import.o: $(ARBHOME)/INCLUDE/arbdb.h
file_import.o: $(ARBHOME)/INCLUDE/arbdb_base.h
file_import.o: $(ARBHOME)/INCLUDE/arbdbt.h
file_import.o: $(ARBHOME)/INCLUDE/arbtools.h
file_import.o: $(ARBHOME)/INCLUDE/attributes.h
file_import.o: $(ARBHOME)/INCLUDE/dupstr.h
file_import.o: $(ARBHOME)/INCLUDE/smartptr.h
file_import.o: $(ARBHOME)/INCLUDE/test_global.h

help_dialog.o: dialog.hxx
help_dialog.o: help_dialog.hxx

image_dialog.o: arb_interface.hxx
image_dialog.o: config_dialog.hxx
image_dialog.o: dialog.hxx
image_dialog.o: help_dialog.hxx
image_dialog.o: image_dialog.hxx
image_dialog.o: selection_dialog.hxx
image_dialog.o: tiff_image.hxx
image_dialog.o: $(ARBHOME)/INCLUDE/ad_k_prot.h
image_dialog.o: $(ARBHOME)/INCLUDE/ad_prot.h
image_dialog.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
image_dialog.o: $(ARBHOME)/INCLUDE/arb_assert.h
image_dialog.o: $(ARBHOME)/INCLUDE/arb_core.h
image_dialog.o: $(ARBHOME)/INCLUDE/arb_error.h
image_dialog.o: $(ARBHOME)/INCLUDE/arbdb.h
image_dialog.o: $(ARBHOME)/INCLUDE/arbdb_base.h
image_dialog.o: $(ARBHOME)/INCLUDE/arbdbt.h
image_dialog.o: $(ARBHOME)/INCLUDE/arbtools.h
image_dialog.o: $(ARBHOME)/INCLUDE/attributes.h
image_dialog.o: $(ARBHOME)/INCLUDE/dupstr.h
image_dialog.o: $(ARBHOME)/INCLUDE/smartptr.h
image_dialog.o: $(ARBHOME)/INCLUDE/test_global.h

import_dialog.o: arb_interface.hxx
import_dialog.o: dialog.hxx
import_dialog.o: entry_sel_dialog.hxx
import_dialog.o: file_import.hxx
import_dialog.o: import_dialog.hxx
import_dialog.o: selection_dialog.hxx
import_dialog.o: $(ARBHOME)/INCLUDE/ad_k_prot.h
import_dialog.o: $(ARBHOME)/INCLUDE/ad_prot.h
import_dialog.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
import_dialog.o: $(ARBHOME)/INCLUDE/arb_assert.h
import_dialog.o: $(ARBHOME)/INCLUDE/arb_core.h
import_dialog.o: $(ARBHOME)/INCLUDE/arb_error.h
import_dialog.o: $(ARBHOME)/INCLUDE/arbdb.h
import_dialog.o: $(ARBHOME)/INCLUDE/arbdb_base.h
import_dialog.o: $(ARBHOME)/INCLUDE/arbdbt.h
import_dialog.o: $(ARBHOME)/INCLUDE/arbtools.h
import_dialog.o: $(ARBHOME)/INCLUDE/attributes.h
import_dialog.o: $(ARBHOME)/INCLUDE/dupstr.h
import_dialog.o: $(ARBHOME)/INCLUDE/smartptr.h
import_dialog.o: $(ARBHOME)/INCLUDE/test_global.h

main.o: arb_interface.hxx
main.o: dialog.hxx
main.o: file_import.hxx
main.o: image_dialog.hxx
main.o: import_dialog.hxx
main.o: main_dialog.hxx
main.o: tiff_image.hxx
main.o: $(ARBHOME)/INCLUDE/ad_k_prot.h
main.o: $(ARBHOME)/INCLUDE/ad_prot.h
main.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
main.o: $(ARBHOME)/INCLUDE/arb_assert.h
main.o: $(ARBHOME)/INCLUDE/arb_core.h
main.o: $(ARBHOME)/INCLUDE/arb_error.h
main.o: $(ARBHOME)/INCLUDE/arbdb.h
main.o: $(ARBHOME)/INCLUDE/arbdb_base.h
main.o: $(ARBHOME)/INCLUDE/arbdbt.h
main.o: $(ARBHOME)/INCLUDE/arbtools.h
main.o: $(ARBHOME)/INCLUDE/attributes.h
main.o: $(ARBHOME)/INCLUDE/dupstr.h
main.o: $(ARBHOME)/INCLUDE/smartptr.h
main.o: $(ARBHOME)/INCLUDE/test_global.h

main_dialog.o: analyze_window.hxx
main_dialog.o: arb_interface.hxx
main_dialog.o: config_dialog.hxx
main_dialog.o: dialog.hxx
main_dialog.o: file_import.hxx
main_dialog.o: image_dialog.hxx
main_dialog.o: import_dialog.hxx
main_dialog.o: main_dialog.hxx
main_dialog.o: msgbox.hxx
main_dialog.o: plot.hxx
main_dialog.o: tiff_image.hxx
main_dialog.o: $(ARBHOME)/INCLUDE/ad_k_prot.h
main_dialog.o: $(ARBHOME)/INCLUDE/ad_prot.h
main_dialog.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
main_dialog.o: $(ARBHOME)/INCLUDE/arb_assert.h
main_dialog.o: $(ARBHOME)/INCLUDE/arb_core.h
main_dialog.o: $(ARBHOME)/INCLUDE/arb_error.h
main_dialog.o: $(ARBHOME)/INCLUDE/arbdb.h
main_dialog.o: $(ARBHOME)/INCLUDE/arbdb_base.h
main_dialog.o: $(ARBHOME)/INCLUDE/arbdbt.h
main_dialog.o: $(ARBHOME)/INCLUDE/arbtools.h
main_dialog.o: $(ARBHOME)/INCLUDE/attributes.h
main_dialog.o: $(ARBHOME)/INCLUDE/dupstr.h
main_dialog.o: $(ARBHOME)/INCLUDE/smartptr.h
main_dialog.o: $(ARBHOME)/INCLUDE/test_global.h

msgbox.o: dialog.hxx
msgbox.o: msgbox.hxx
msgbox.o: xm_defs.hxx

plot.o: plot.hxx

selection_dialog.o: arb_interface.hxx
selection_dialog.o: dialog.hxx
selection_dialog.o: selection_dialog.hxx
selection_dialog.o: $(ARBHOME)/INCLUDE/ad_k_prot.h
selection_dialog.o: $(ARBHOME)/INCLUDE/ad_prot.h
selection_dialog.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
selection_dialog.o: $(ARBHOME)/INCLUDE/arb_assert.h
selection_dialog.o: $(ARBHOME)/INCLUDE/arb_core.h
selection_dialog.o: $(ARBHOME)/INCLUDE/arb_error.h
selection_dialog.o: $(ARBHOME)/INCLUDE/arbdb.h
selection_dialog.o: $(ARBHOME)/INCLUDE/arbdb_base.h
selection_dialog.o: $(ARBHOME)/INCLUDE/arbdbt.h
selection_dialog.o: $(ARBHOME)/INCLUDE/arbtools.h
selection_dialog.o: $(ARBHOME)/INCLUDE/attributes.h
selection_dialog.o: $(ARBHOME)/INCLUDE/dupstr.h
selection_dialog.o: $(ARBHOME)/INCLUDE/smartptr.h
selection_dialog.o: $(ARBHOME)/INCLUDE/test_global.h

tiff_image.o: tiff_image.hxx
