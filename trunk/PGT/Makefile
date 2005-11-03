# Copyright (c) 2004 - 2005 Kai Bader <baderk@in.tum.de>
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
# AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
# AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
# CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

# CVS REVISION TAG  --  $Revision$

CFILES = main.cxx dialog.cxx arb_interface.cxx main_dialog.cxx \
import_dialog.cxx selection_dialog.cxx file_import.cxx tiff_image.cxx \
image_dialog.cxx plot.cxx analyze_window.cxx entry_sel_dialog.cxx \
config_dialog.cxx help_dialog.cxx msgbox.cxx

HFILES = dialog.hxx arb_interface.hxx main_dialog.hxx import_dialog.hxx \
selection_dialog.hxx file_import.hxx tiff_image.hxx image_dialog.hxx plot.hxx \
analyze_window.hxx entry_sel_dialog.hxx config_dialog.hxx help_dialog.hxx \
msgbox.hxx

ARCHS = $(ARBHOME)/SERVERCNTRL/SERVERCNTRL.a $(ARBHOME)/PROBE_COM/client.a

INCLUDES = -I/usr/X11R6/include -I$(ARBHOME)/INCLUDE -I$(ARBHOME)/AISC -I.

LIBS = -L/usr/X11R6/lib -lXm -lXt -lX11 -ltiff -lARBDB -L$(ARBHOME)/LIBLINK

all: revisions pgtoolkit

pgtoolkit:
	g++ -W -Wall -O2 -o pgt $(CFILES) $(LIBS) $(INCLUDES) $(ARCHS)

debug: clean revisions
	g++ -W -Wall -g -ggdb -o pgt $(CFILES) $(LIBS) $(INCLUDES) $(ARCHS)

efence: clean revisions
	g++ -W -Wall -g -ggdb -o pgt $(CFILES) -lefence $(LIBS) $(INCLUDES) $(ARCHS)

revisions:
	rm -f revision.hxx
	echo 'static const char *REVISION=' >>revision.hxx
	grep -I 'Revision\:' *.?xx | sed -e 's/^\([^:]*\).*Revision: \([0-9\.]*\).*/\"\\t- \1  v\2\\n"/' >>revision.hxx
	echo '"\n";' >>revision.hxx

package:
	rm -rf pgtools/ pgtools.tar.gz
	mkdir pgtools pgtools/pics
	cp $(CFILES) $(HFILES) Makefile pgtools/
	cp pics/*.xpm pgtools/pics/
	tar cvf pgtools.tar pgtools/ && gzip -9 pgtools.tar
	rm -rf pgtools/

clean:
	rm -f *.o *~ pgt
