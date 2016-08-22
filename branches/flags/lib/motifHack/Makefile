.SUFFIXES: .xpm
.PHONY: all clean

CONVERTER:=./convert_xpm.pl

SOURCEDIR:=../pixmaps
TARGETDIR:=./pixmaps

SOURCE_XPM:= \
	$(wildcard $(SOURCEDIR)/*.xpm) \
	$(filter-out $(SOURCEDIR)/unused/%,$(wildcard $(SOURCEDIR)/*/*.xpm))

TARGET_XPM:=$(subst $(SOURCEDIR),$(TARGETDIR),$(SOURCE_XPM))

all: $(TARGET_XPM)
	@test \! -z "$(TARGET_XPM)"

clean:
	-rm -f $(TARGET_XPM)

$(TARGETDIR)/%.xpm : $(SOURCEDIR)/%.xpm $(CONVERTER) Makefile
	$(CONVERTER) $@ < $< 

